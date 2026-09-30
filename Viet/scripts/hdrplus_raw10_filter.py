#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: HDR Plus RAW10 Filter
# Description: Download candidate DNGs and export only exact RGGB RAW10 BlackLevel 64 crops.
# Author: Viet Nguyen To Quoc

import argparse
import csv
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import urllib.request


ROOT = Path(__file__).resolve().parents[1]


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def download(url, path):
    if path.is_file() and path.stat().st_size:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(".part")
    with urllib.request.urlopen(url, timeout=120) as response, temporary.open("wb") as output:
        shutil.copyfileobj(response, output)
        expected = response.headers.get("Content-Length")
    if expected is not None and temporary.stat().st_size != int(expected):
        temporary.unlink()
        raise ValueError(f"Truncated download: {url}")
    temporary.replace(path)


def verify_pgm(path):
    with path.open("rb") as stream:
        if [stream.readline().strip() for _ in range(3)] != [b"P5", b"1920 1080", b"1023"]:
            raise ValueError(f"Invalid RAW10 PGM header: {path}")
        pixels = stream.read()
    if len(pixels) != 1920 * 1080 * 2:
        raise ValueError(f"Invalid RAW10 PGM size: {path}")
    if any(pixels[index] > 3 for index in range(0, len(pixels), 2)):
        raise ValueError(f"PGM contains a sample above 1023: {path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidates", type=Path,
                        default=ROOT / "scripts/hdrplus_bl64_candidates.tsv")
    parser.add_argument("--source-root", type=Path,
                        default=ROOT / "artifacts/hdrplus_bl64/source")
    parser.add_argument("--output", type=Path,
                        default=ROOT / "artifacts/hdrplus_bl64/clean")
    parser.add_argument("--injector", type=Path, default=ROOT / "build/defect_injector")
    parser.add_argument("--download", action="store_true",
                        help="fetch original payload_N000.dng files listed in the candidate table")
    args = parser.parse_args()
    with args.candidates.open(newline="") as stream:
        candidates = list(csv.DictReader(stream, delimiter="\t"))
    if not candidates or any(not row["url"].startswith(
            "https://storage.googleapis.com/hdrplusdata/20171106_subset/bursts/")
            or not row["url"].endswith("/payload_N000.dng") for row in candidates):
        parser.error("Candidate table must contain original HDR+ curated payload_N000 DNG URLs")
    args.output.mkdir(parents=True, exist_ok=True)
    manifest_path = args.output.parent / "manifest.json"
    previous = {row["url"]: row for row in json.loads(manifest_path.read_text())} \
        if manifest_path.is_file() else {}
    results = dict(previous)
    for index, row in enumerate(candidates, 1):
        burst = row["url"].split("/")[-2]
        source = args.source_root / burst / "payload_N000.dng"
        target = args.output / burst
        record = {"burst": burst, "url": row["url"], "source": str(source)}
        try:
            if args.download:
                download(row["url"], source)
            if not source.is_file():
                raise FileNotFoundError(source)
            record["source_sha256"] = sha256(source)
            if target.exists():
                prior = previous.get(row["url"], {})
                if (prior.get("status") != "accepted" or
                        prior.get("source_sha256") != record["source_sha256"] or
                        prior.get("output_sha256") != sha256(target / "clean_rggb.pgm")):
                    raise ValueError("Existing export has no matching source/output hashes in prior manifest")
                metadata = json.loads((target / "input_metadata.json").read_text())
                verify_pgm(target / "clean_rggb.pgm")
            else:
                temporary = Path(tempfile.mkdtemp(prefix=".export-", dir=args.output))
                try:
                    result = subprocess.run(
                        [str(args.injector), "--export-clean-raw10-bl64", str(source.resolve())],
                        cwd=temporary, text=True, capture_output=True)
                    if result.returncode:
                        raise ValueError(result.stderr.strip() or result.stdout.strip())
                    metadata = json.loads((temporary / "input_metadata.json").read_text())
                    verify_pgm(temporary / "clean_rggb.pgm")
                    temporary.rename(target)
                finally:
                    shutil.rmtree(temporary, ignore_errors=True)
            if (metadata["width"], metadata["height"], metadata["cfa"],
                    metadata["bit_depth"], metadata["pixel_max"], metadata["white_level"],
                    metadata["native_black_r_gr_gb_b"]) != (
                    1920, 1080, "RGGB", 10, 1023, 1023, [64, 64, 64, 64]):
                raise ValueError("Export metadata violates the exact RAW10 BL64 contract")
            record.update(status="accepted", output=str(target),
                          output_sha256=sha256(target / "clean_rggb.pgm"))
        except (OSError, ValueError, KeyError) as error:
            record.update(status="rejected", reason=str(error))
        results[row["url"]] = record
        print(f"[{index}/{len(candidates)}] {burst}: {record['status']}", flush=True)
        manifest_path.write_text(json.dumps(list(results.values()), indent=2) + "\n")
    accepted = sum(row["status"] == "accepted" for row in results.values())
    print(f"Accepted {accepted}/{len(results)}; manifest: {manifest_path}")
    if accepted == 0:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
