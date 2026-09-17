#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: OpenISP BPC Held-Out Benchmark
# Description: Evaluate the fixed published OpenISP gradient DPC operating point on the frozen adaptive-v2 held-out split.
# Author: Viet Nguyen To Quoc

import argparse
from concurrent.futures import as_completed, ThreadPoolExecutor
import csv
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
WIDTH = 1920
HEIGHT = 1080
PIXELS = WIDTH * HEIGHT
NAMES = ("RG_hot", "RG_dead", "BRG", "BCR", "IOTCR")
ALGORITHMS = ("baseline", "openisp", "adaptive")
OPENISP_COMMIT = "d4947e1aa5f4af83c3640131dbca8a675b613ec6"
OPENISP_SOURCE = (
    "https://github.com/cruxopen/openISP/blob/"
    f"{OPENISP_COMMIT}/model/dpc.py"
)
SOURCE_THRESHOLD_10BIT = 30
DEFAULT_THRESHOLD_RAW12 = SOURCE_THRESHOLD_10BIT * 4


def read_json(path):
    return json.loads(Path(path).read_text())


def save_json(path, value):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n")
    temporary.replace(path)


def digest(path):
    result = hashlib.sha256()
    with Path(path).open("rb") as file:
        for block in iter(lambda: file.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def identity(path):
    path = Path(path).resolve()
    stat = path.stat()
    return {"path": str(path), "size": stat.st_size, "mtime_ns": stat.st_mtime_ns}


def read_pgm(path):
    def token(file):
        value = bytearray()
        while True:
            character = file.read(1)
            if not character:
                raise ValueError(f"Incomplete PGM header: {path}")
            if character == b"#":
                file.readline()
            elif not character.isspace():
                value.extend(character)
                break
        while True:
            character = file.read(1)
            if not character or character.isspace():
                return bytes(value)
            value.extend(character)

    with Path(path).open("rb") as file:
        magic = token(file)
        width = int(token(file))
        height = int(token(file))
        maximum = int(token(file))
        payload = file.read()
    if magic != b"P5" or width != WIDTH or height != HEIGHT or maximum != 4095:
        raise ValueError(f"Expected injector P5 RAW12 frame: {path}")
    if len(payload) != PIXELS * 2:
        raise ValueError(f"Invalid RAW12 PGM payload length: {path}")
    values = np.frombuffer(payload, dtype=">u2").astype(np.uint16)
    if np.any(values > 4095):
        raise ValueError(f"RAW12 sample exceeds 4095: {path}")
    return values


def read_labels(path):
    labels = np.full(PIXELS, -1, dtype=np.int8)
    with Path(path).open(newline="") as file:
        reader = csv.DictReader(file)
        expected = {"x", "y", "cfa", "type", "input_value", "output_value", "a", "b"}
        if set(reader.fieldnames or ()) != expected:
            raise ValueError("Invalid defects.csv header")
        for row in reader:
            col = int(row["x"])
            line = int(row["y"])
            kind = 0 if row["type"] == "hot" else 1 if row["type"] == "dead" else -1
            if (col < 2 or col >= WIDTH - 2 or line < 2 or line >= HEIGHT - 2 or kind < 0):
                raise ValueError("Invalid hot/dead coordinate")
            index = line * WIDTH + col
            if labels[index] >= 0:
                raise ValueError("Duplicate defect coordinate")
            labels[index] = kind
    return labels


def load_library(path):
    library = ctypes.CDLL(str(Path(path).resolve()))
    function = library.openisp_bpc_process_raw12
    pointer = ctypes.POINTER(ctypes.c_uint16)
    function.argtypes = (pointer, pointer, ctypes.c_size_t, ctypes.c_uint16)
    function.restype = ctypes.c_int
    return library, function


def run_openisp(function, frame, threshold):
    source = np.ascontiguousarray(frame, dtype=np.uint16)
    output = np.empty_like(source)
    pointer = ctypes.POINTER(ctypes.c_uint16)
    status = function(
        source.ctypes.data_as(pointer),
        output.ctypes.data_as(pointer),
        source.size,
        threshold,
    )
    if status != 0:
        raise RuntimeError(f"openisp_bpc_process_raw12 failed with status {status}")
    return output


def rates(reference, corrupted, output_r, output_c, labels):
    valid = np.zeros(PIXELS, dtype=bool)
    valid.reshape(HEIGHT, WIDTH)[2:-2, 2:-2] = True
    modified = labels >= 0
    if np.any(reference[~modified] != corrupted[~modified]):
        raise ValueError("Reference/corrupted pair differs outside injection labels")

    difference = np.abs(corrupted.astype(np.int32) - reference.astype(np.int32))
    residual = np.abs(output_c.astype(np.int32) - reference.astype(np.int32))
    initial = np.array([
        difference[labels == kind].sum(dtype=np.int64) for kind in (0, 1)
    ], dtype=np.int64)
    remaining = np.array([
        residual[labels == kind].sum(dtype=np.int64) for kind in (0, 1)
    ], dtype=np.int64)
    if np.any(initial == 0):
        raise ValueError("Zero initial hot/dead error; regenerate injection")

    unmodified = valid & ~modified
    hot = 1.0 - float(remaining[0]) / float(initial[0])
    dead = 1.0 - float(remaining[1]) / float(initial[1])
    return [
        hot,
        dead,
        (hot + dead) / 2.0,
        float(np.count_nonzero((output_r != reference) & valid)) / float(np.count_nonzero(valid)),
        float(np.count_nonzero((output_c != output_r) & unmodified)) / float(np.count_nonzero(unmodified)),
    ]


def frozen_rows(path):
    with Path(path).open(newline="") as file:
        rows = list(csv.DictReader(file))
    required = {"path"} | {
        f"{algorithm}_{name}"
        for algorithm in ("baseline", "adaptive")
        for name in NAMES
    }
    if not rows or not required.issubset(rows[0]):
        raise ValueError("Frozen per-image CSV has an incompatible schema")
    return {
        row["path"]: {
            algorithm: [float(row[f"{algorithm}_{name}"]) for name in NAMES]
            for algorithm in ("baseline", "adaptive")
        }
        for row in rows
    }


def summarize(values, budget):
    array = np.asarray(values, dtype=np.float64)
    mean = array.mean(axis=0)
    median = np.median(array, axis=0)
    return {
        "mean": dict(zip(NAMES, map(float, mean))),
        "median": dict(zip(NAMES, map(float, median))),
        "within_adaptive_tuning_safety_budget": bool(
            mean[3] <= budget["BCR"] and mean[4] <= budget["IOTCR"]
        ),
    }


def fractions(left, right):
    left = np.asarray(left, dtype=np.float64)[:, 2]
    right = np.asarray(right, dtype=np.float64)[:, 2]
    return {
        "BRG_better_fraction": float(np.mean(left > right)),
        "BRG_worse_fraction": float(np.mean(left < right)),
        "BRG_equal_fraction": float(np.mean(left == right)),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--split", type=Path, default=ROOT / "artifacts/adaptive_v2/split/split.json")
    parser.add_argument("--selection", type=Path, default=ROOT / "artifacts/adaptive_v2/adaptive_budget/selection/selection.json")
    parser.add_argument("--frozen-test", type=Path, default=ROOT / "artifacts/adaptive_v2/adaptive_budget/test/per_image.csv")
    parser.add_argument("--frozen-report", type=Path, default=ROOT / "artifacts/adaptive_v2/adaptive_budget/test/report.json")
    parser.add_argument("--injector", type=Path, default=ROOT / "build/defect_injector")
    parser.add_argument("--library", type=Path, default=ROOT / "build/libbpc_openisp.so")
    parser.add_argument("--output", type=Path, default=ROOT / "artifacts/openisp_bpc/test")
    parser.add_argument("--threshold", type=int, default=DEFAULT_THRESHOLD_RAW12)
    parser.add_argument("--jobs", type=int, default=1)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("jobs must be at least 1")
    if not 0 <= args.threshold <= 4095:
        parser.error("threshold must be in [0, 4095]")

    split = read_json(args.split)
    selection = read_json(args.selection)
    frozen_report = read_json(args.frozen_report)
    if split.get("algorithm") != "adaptive_v2":
        raise ValueError("Split is not the frozen adaptive_v2 split")
    if selection.get("selection_policy") != "adaptive_operating_point_budget_v1":
        raise ValueError("Selection is not the adaptive-budget operating point")
    if selection.get("split_sha256") != digest(args.split):
        raise ValueError("Selection does not match the supplied split")
    if selection.get("injector_sha256") != digest(args.injector):
        raise ValueError("Injector does not match the binary used by the frozen selection")
    if frozen_report.get("selection") != selection:
        raise ValueError("Frozen report does not use the supplied selection")

    frozen = frozen_rows(args.frozen_test)
    test_paths = [row["path"] for row in split["test"]]
    if set(test_paths) != set(frozen) or len(test_paths) != len(frozen):
        raise ValueError("Frozen per-image CSV does not exactly cover the held-out split")
    if frozen_report.get("images") != len(test_paths):
        raise ValueError("Frozen report image count does not match the held-out split")
    for row in split["test"]:
        if identity(row["path"]) != row["source"]:
            raise ValueError(f"Source changed since scan: {row['path']}")

    specification = {
        "algorithm": "cruxopen_openisp_gradient_dpc",
        "comparison": "fixed published OpenISP operating point on frozen adaptive_v2 held-out split",
        "openisp_source": OPENISP_SOURCE,
        "openisp_commit": OPENISP_COMMIT,
        "openisp_mode": "gradient",
        "source_threshold_10bit": SOURCE_THRESHOLD_10BIT,
        "source_clip_10bit": 1023,
        "threshold_raw12": args.threshold,
        "threshold_mapping": "published 10-bit threshold multiplied by four for RAW12",
        "runner_sha256": digest(__file__),
        "openisp_reference_header_sha256": digest(ROOT / "reference/bpc_openisp.hpp"),
        "openisp_reference_sha256": digest(ROOT / "reference/bpc_openisp.cpp"),
        "openisp_library_sha256": digest(args.library),
        "injector_sha256": digest(args.injector),
        "split_sha256": digest(args.split),
        "selection_sha256": digest(args.selection),
        "frozen_test_sha256": digest(args.frozen_test),
        "frozen_report_sha256": digest(args.frozen_report),
    }
    args.output.mkdir(parents=True, exist_ok=True)
    state = args.output / "run.json"
    if state.exists():
        if read_json(state) != specification:
            raise ValueError(f"Run inputs changed; use a fresh output directory: {args.output}")
    else:
        if any(args.output.iterdir()):
            raise ValueError(f"Output directory is not empty: {args.output}")
        save_json(state, specification)

    library, function = load_library(args.library)

    def process(row):
        directory = args.output / row["id"]
        directory.mkdir(parents=True, exist_ok=True)
        done = directory / "done.json"
        if done.exists():
            return row["path"], None
        try:
            with tempfile.TemporaryDirectory(prefix="openisp-bpc-") as temporary:
                temporary = Path(temporary)
                result = subprocess.run(
                    [str(args.injector), "--seed", str(row["injection_seed"]), row["path"]],
                    cwd=temporary,
                    text=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                (directory / "run.log").write_text(result.stderr + result.stdout)
                if result.returncode:
                    raise RuntimeError(f"Injector exited with status {result.returncode}")
                reference = read_pgm(temporary / "reference_blc.pgm")
                corrupted = read_pgm(temporary / "corrupted_blc.pgm")
                labels = read_labels(temporary / "defects.csv")
                values = rates(
                    reference,
                    corrupted,
                    run_openisp(function, reference, args.threshold),
                    run_openisp(function, corrupted, args.threshold),
                    labels,
                )
            save_json(done, {
                "path": row["path"],
                "threshold_raw12": args.threshold,
                "openisp": dict(zip(NAMES, values)),
            })
            (directory / "error.json").unlink(missing_ok=True)
            return row["path"], None
        except (OSError, ValueError, RuntimeError) as error:
            save_json(directory / "error.json", {"path": row["path"], "error": str(error)})
            return row["path"], str(error)

    failures = []
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = [executor.submit(process, row) for row in split["test"]]
        for completed, future in enumerate(as_completed(futures), 1):
            path, error = future.result()
            if error is not None:
                failures.append(path)
            print(f"{completed}/{len(futures)} {path}", flush=True)
    save_json(args.output / "failures.json", sorted(failures))
    if failures:
        raise RuntimeError(f"{len(failures)} images failed; rerun the same command to retry")

    rows = []
    values = {algorithm: [] for algorithm in ALGORITHMS}
    for row in split["test"]:
        openisp = read_json(args.output / row["id"] / "done.json")["openisp"]
        metrics = {
            "baseline": frozen[row["path"]]["baseline"],
            "openisp": [float(openisp[name]) for name in NAMES],
            "adaptive": frozen[row["path"]]["adaptive"],
        }
        values_row = {"path": row["path"]}
        for algorithm in ALGORITHMS:
            values[algorithm].append(metrics[algorithm])
            values_row.update({
                f"{algorithm}_{name}": metrics[algorithm][index]
                for index, name in enumerate(NAMES)
            })
        rows.append(values_row)

    fields = ["path"] + [
        f"{algorithm}_{name}" for algorithm in ALGORITHMS for name in NAMES
    ]
    with (args.output / "per_image.csv").open("w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)

    budget = selection["safety_budget"]
    report = {
        "images": len(rows),
        "aggregation": "equal image weights",
        "selection": selection,
        "provenance": specification,
        "comparison": {
            "openisp_vs_baseline": fractions(values["openisp"], values["baseline"]),
            "openisp_vs_adaptive": fractions(values["openisp"], values["adaptive"]),
        },
    }
    for algorithm in ALGORITHMS:
        report[algorithm] = summarize(values[algorithm], budget)
    save_json(args.output / "report.json", report)

    # Keep the CDLL alive until every worker and aggregate operation is complete.
    del library


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        raise SystemExit(str(error))
