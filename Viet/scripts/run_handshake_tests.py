#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: Handshake HLS Test Runner
# Description: Generate reproducible vectors and run bounded CSim or CoSim batches with explicit verdicts.
# Author: Viet Nguyen To Quoc

import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import re
import shlex
import shutil
import signal
import struct
import subprocess
import sys
import time
from datetime import datetime


REPO_ROOT = Path(__file__).resolve().parents[2]
PROFILES = {
    "blc": ((64, 66, 65, 68, 0), (32, 36, 40, 44, 0), (96, 100, 104, 108, 0)),
    "bpc": ((16, 20, 24, 4, 3), (48, 56, 64, 3, 4), (96, 80, 112, 5, 2)),
}
PART = "xczu7ev-ffvc1156-2-e"


class RunFailure(Exception):
    def __init__(self, kind, message):
        super().__init__(message)
        self.kind = kind


def positive_int(value):
    number = int(value)
    if number <= 0:
        raise argparse.ArgumentTypeError("must be a positive integer")
    return number


def seed_int(value):
    number = int(value, 0)
    if not 0 <= number < 2**64:
        raise argparse.ArgumentTypeError("seed must be in [0, 2^64-1]")
    return number


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__ or "BLC/BPC handshake Golden tests")
    parser.add_argument("--block", choices=PROFILES, required=True)
    parser.add_argument("--stage", choices=("csim", "cosim"), required=True)
    parser.add_argument("--mode", choices=("single", "boundary", "multi"), required=True)
    parser.add_argument("--seed", type=seed_int, default=None, help="default: 20260930")
    parser.add_argument("--frames", type=positive_int, help="multi only; default: 5")
    parser.add_argument("--timeout", type=positive_int, default=3600, help="seconds per tool process")
    parser.add_argument("--synth-timeout", type=positive_int, default=1800)
    parser.add_argument("--replay", type=Path, help="reuse a saved vectors.json, including config")
    args = parser.parse_args()
    if args.frames is not None and args.mode != "multi":
        parser.error("--frames is valid only in multi mode")
    if args.replay is not None and args.seed is not None:
        parser.error("--replay retains the recorded seeds; omit --seed")
    return args


def write_json(path, data):
    path.write_text(json.dumps(data, indent=4) + "\n", encoding="utf-8")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def source_hashes(block):
    paths = list((REPO_ROOT / "Viet/hls" / block / "handshake").glob("*.[ch]pp"))
    paths += list((REPO_ROOT / "Viet/hls").glob("*.hpp"))
    paths += [REPO_ROOT / "isp_pixel_packet.hpp", Path(__file__).resolve(),
              REPO_ROOT / "Viet/tests/test_handshake_hls.cpp"]
    reference = "bpc_adaptive" if block == "bpc" else "blc"
    paths += [REPO_ROOT / f"Viet/reference/{reference}.{suffix}" for suffix in ("hpp", "cpp")]
    return {str(path.relative_to(REPO_ROOT)): sha256(path) for path in sorted(set(paths))}


def validate_frame(frame, block):
    if type(frame["index"]) is not int or frame["index"] < 0:
        raise ValueError("invalid frame index")
    if type(frame["seed"]) is not int or not 0 <= frame["seed"] < 2**64:
        raise ValueError("invalid frame seed")
    config = frame["config"]
    limits = (1023, 1023, 1023, 15, 15) if block == "bpc" else (1023, 1023, 1023, 1023, 0)
    if len(config) != 5 or any(type(v) is not int or not 0 <= v <= limit
                               for v, limit in zip(config, limits)):
        raise ValueError("invalid recorded config")


def prepare_vectors(args, campaign, width, height):
    vector_dir = campaign / "inputs"
    vector_dir.mkdir()
    frames = []
    if args.replay:
        replay_path = args.replay.resolve()
        saved = json.loads(replay_path.read_text(encoding="utf-8"))
        if (saved["format"] != "HS_V1" or saved["block"] != args.block or
                saved["width"] != width or saved["height"] != height):
            raise ValueError("replay block/geometry/format does not match the requested mode")
        frames = saved["frames"]
        count = len(frames)
        required = 3 if args.mode == "boundary" else 1 if args.mode == "single" else args.frames
        if count == 0 or (required is not None and count != required):
            raise ValueError("replay frame count does not match mode/--frames")
        for position, frame in enumerate(frames):
            validate_frame(frame, args.block)
            source = (replay_path.parent / frame["input"]).resolve()
            if source.stat().st_size != width * height * 2 or sha256(source) != frame["sha256"]:
                raise ValueError(f"input size/hash mismatch: {source}")
            target = vector_dir / f"frame_{position:04d}.raw10le"
            shutil.copyfile(source, target)
            frame["input"] = str(target.relative_to(campaign))
    else:
        count = 3 if args.mode == "boundary" else 1 if args.mode == "single" else (args.frames or 5)
        seed = 20260930 if args.seed is None else args.seed
        for position in range(count):
            frame_seed = (seed + position) % 2**64
            rng = random.Random(frame_seed)
            target = vector_dir / f"frame_{position:04d}.raw10le"
            #Generate independently of DUT/Golden, in modest chunks, exactly once.
            with target.open("wb") as file:
                for start in range(0, width * height, 8192):
                    size = min(8192, width * height - start)
                    pixels = [rng.getrandbits(10) for _ in range(size)]
                    file.write(struct.pack(f"<{size}H", *pixels))
            frames.append({"index": position, "seed": frame_seed,
                           "config": list(PROFILES[args.block][position % 3]),
                           "input": str(target.relative_to(campaign)), "sha256": sha256(target)})
    if len({frame["index"] for frame in frames}) != len(frames):
        raise ValueError("duplicate frame indices")
    if len({frame["seed"] for frame in frames}) != len(frames):
        raise ValueError("duplicate frame seeds")
    if args.mode == "boundary" and (len({tuple(f["config"]) for f in frames}) != 3 or
                                     len({f["sha256"] for f in frames}) != 3):
        raise ValueError("boundary requires distinct data and configs")
    vectors = {"format": "HS_V1", "block": args.block, "width": width, "height": height,
               "generator": "Python random.Random(seed).getrandbits(10)", "frames": frames}
    write_json(campaign / "vectors.json", vectors)
    return vectors


def write_manifest(path, vectors, frames, campaign):
    lines = [f"HS_V1 {vectors['block']} {vectors['width']} {vectors['height']} {len(frames)}"]
    for frame in frames:
        pixel_path = campaign / frame["input"]
        fields = [frame["index"], frame["seed"], *frame["config"], pixel_path]
        lines.append(" ".join(str(value) for value in fields))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def config_text(block, width, height, mode, manifest=None):
    hls_dir = REPO_ROOT / "Viet/hls"
    variant = hls_dir / block / "handshake"
    flags = f"-std=c++17 -I{REPO_ROOT} -I{hls_dir} -I{variant} -DISP_FRAME_WIDTH={width} -DISP_FRAME_HEIGHT={height}"
    reference = "bpc_adaptive" if block == "bpc" else "blc"
    trace_level = "all" if mode == "boundary" else "none"
    lines = [f"part={PART}", "", "[hls]", f"syn.top={block}_top", "clock=150MHz",
             "clock_uncertainty=10%", f"syn.cflags={flags}",
             f"syn.file={variant / (block + '_top.cpp')}",
             f"syn.file={variant / ('isp_' + block + '.cpp')}",
             f"tb.cflags={flags} -I{REPO_ROOT / 'Viet/reference'} -DHS_TEST_BPC={int(block == 'bpc')}",
             f"tb.file={REPO_ROOT / 'Viet/tests/test_handshake_hls.cpp'}",
             f"tb.file={REPO_ROOT / ('Viet/reference/' + reference + '.cpp')}",
             "csim.O=true", "cosim.O=true", "cosim.rtl=verilog", "cosim.tool=xsim",
             f"cosim.trace_level={trace_level}", "cosim.wave_debug=false",
             "cosim.enable_dataflow_profiling=false", "cosim.random_stall=false"]
    if manifest:
        lines += [f"csim.argv={manifest}", f"cosim.argv={manifest}"]
    return "\n".join(lines) + "\n"


def run_command(command, directory, timeout, label, work_dir):
    directory.mkdir(parents=True, exist_ok=True)
    print(f"{label}: {shlex.join(command)}", flush=True)
    record = {"command": command, "timeout_seconds": timeout,
              "started": datetime.now().astimezone().isoformat(), "peak_rss_kib": None}
    timed_command = command
    time_bin = Path("/usr/bin/time")
    if time_bin.is_file():
        timed_command = [str(time_bin), "-f", "%M", "-o", str(directory / "peak_rss.txt"), *command]
    start = time.monotonic()
    timed_out = False
    with (directory / "console.log").open("w", encoding="utf-8") as log:
        process = subprocess.Popen(timed_command, cwd=work_dir, stdout=log,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = process.wait(timeout=timeout)
        except (subprocess.TimeoutExpired, KeyboardInterrupt):
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            timed_out = True
            code = process.returncode
    record.update(returncode=code, elapsed_seconds=round(time.monotonic() - start, 3), timeout=timed_out)
    rss_path = directory / "peak_rss.txt"
    if rss_path.exists():
        values = rss_path.read_text().splitlines()
        if values and values[-1].isdigit():
            record["peak_rss_kib"] = int(values[-1])
    write_json(directory / "command.json", record)
    if timed_out:
        raise RunFailure("timeout_or_interrupted", f"{label} stopped; see {directory / 'console.log'}")
    if code != 0:
        raise RunFailure("tool_or_environment", f"{label} exit={code}; see {directory / 'console.log'}")


def check_golden_log(text, vectors, frames):
    expected_pass = (f"HS_CHECKER_PASS block={vectors['block']} frames={len(frames)} "
                     f"pixels={len(frames) * vectors['width'] * vectors['height']}")
    if expected_pass not in text.splitlines() or "HS_CHECKER_FAIL" in text or "HS_TEST_ERROR" in text:
        raise RunFailure("checker_or_missing_verdict", "Golden checker did not complete successfully")
    rows = re.findall(r"^HS_FRAME (.+)$", text, re.MULTILINE)
    if len(rows) != len(frames):
        raise RunFailure("checker_or_missing_verdict", "missing/extra frame summaries")
    for row, frame in zip(rows, frames):
        actual = dict(field.split("=", 1) for field in row.split())
        expected = {"block": vectors["block"], "index": str(frame["index"]), "seed": str(frame["seed"]),
                    "checked": str(vectors["width"] * vectors["height"]), "missing": "0",
                    "mismatches": "0", "extras": "0", "remaining_input": "0"}
        if any(actual.get(key) != value for key, value in expected.items()):
            raise RunFailure("golden_or_packet_mismatch", f"bad frame summary: {row}")


def classify_tool_failure(error, work, stage, directory):
    if getattr(error, "kind", None) != "tool_or_environment":
        return error
    text = ""
    for path in (directory / "console.log", work / "logs" / f"hls_run_{stage}.log"):
        if path.is_file():
            text += path.read_text(encoding="utf-8", errors="replace")
    if "HS_CHECKER_FAIL" in text or "HS_MISMATCH" in text:
        return RunFailure("golden_or_packet_mismatch", str(error))
    if re.search(r"out of memory|cannot allocate memory|bad_alloc|oom-kill", text, re.IGNORECASE):
        return RunFailure("out_of_memory", str(error))
    if "HS_TEST_ERROR" in text:
        return RunFailure("testbench_or_input", str(error))
    if re.search(r"deadlock", text, re.IGNORECASE):
        return RunFailure("deadlock", str(error))
    return error


def check_stage(work, stage, vectors, frames):
    log = work / "logs" / f"hls_run_{stage}.log"
    if not log.is_file():
        raise RunFailure("missing_verdict", f"missing Vitis log: {log}")
    text = log.read_text(encoding="utf-8", errors="replace")
    if stage == "csim":
        check_golden_log(text, vectors, frames)
    else:
        #The first checker is C vector generation; the second consumes RTL results.
        split = text.find("Starting C post checking")
        if split < 0:
            raise RunFailure("missing_rtl_verdict", "Vitis did not reach C post checking")
        check_golden_log(text[:split], vectors, frames)
        check_golden_log(text[split:], vectors, frames)
        if not re.search(r"C/RTL co-simulation finished:\s*PASS", text):
            raise RunFailure("rtl_cosim_failure", "Vitis C/RTL verdict is not PASS")
        report = work / "hls/sim/report" / f"{vectors['block']}_top_cosim.rpt"
        if not report.is_file() or not re.search(r"\|\s*Verilog\s*\|\s*Pass\s*\|", report.read_text()):
            raise RunFailure("missing_rtl_verdict", "fresh Verilog CoSim report is not Pass")


def archive_outputs(work, destination):
    #Preserve each simulator run before the next vitis-run recreates these paths.
    paths = [Path(name) for name in ("logs", "reports", "hls/sim", "hls/csim")]
    paths += [path.relative_to(work) for pattern in ("*.log", "*.jou") for path in work.glob(pattern)]
    for relative in paths:
        source = work / relative
        if source.exists():
            target = destination / "tool_results" / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(str(source), str(target))


def main():
    args = parse_args()
    width, height = (16, 16) if args.mode == "boundary" else (1920, 1080)
    #The plain manifest and HLS config deliberately avoid shell/path quoting ambiguities.
    if any(char.isspace() for char in str(REPO_ROOT)):
        raise SystemExit("FAIL: repository path must not contain whitespace")
    root = REPO_ROOT / "Viet/build/handshake_tests" / args.block / args.mode
    root.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().astimezone().strftime("%Y%m%d_%H%M%S_%f")
    campaign = root / f"{stamp}_{args.stage}_{os.getpid()}"
    campaign.mkdir()
    work = campaign / "work"
    work.mkdir()
    result = {"status": "FAIL", "failure_kind": "incomplete", "block": args.block,
              "mode": args.mode, "stage": args.stage, "width": width, "height": height,
              "completed_frames": 0, "python": sys.version, "started": datetime.now().astimezone().isoformat()}
    write_json(campaign / "result.json", result)
    print(f"Results: {campaign}", flush=True)
    active = None
    try:
        for binary in (("vitis-run", "v++") if args.stage == "cosim" else ("vitis-run",)):
            if shutil.which(binary) is None:
                raise RunFailure("environment", f"{binary} is unavailable; source Vitis settings64.sh")
            run_command([binary, "--version"], campaign / f"version_{binary}", 30, "Tool version", work)
        result["revision"] = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO_ROOT, text=True).strip()
        result["git_status"] = subprocess.check_output(["git", "status", "--short"], cwd=REPO_ROOT, text=True)
        hashes = source_hashes(args.block)
        result["source_sha256"] = hashes
        vectors = prepare_vectors(args, campaign, width, height)
        result["planned_frames"] = len(vectors["frames"])
        write_json(campaign / "result.json", result)
        synth_cfg = campaign / "synthesis.cfg"
        synth_cfg.write_text(config_text(args.block, width, height, args.mode), encoding="utf-8")
        if args.stage == "cosim":
            active = campaign / "synthesis"
            run_command(["v++", "-c", "--mode", "hls", "--config", str(synth_cfg),
                         "--work_dir", str(work)], active, args.synth_timeout, "Synthesis", work)
            archive_outputs(work, active)
        groups = [vectors["frames"]] if args.mode != "multi" else [[frame] for frame in vectors["frames"]]
        for position, frames in enumerate(groups):
            active = campaign / f"run_{position + 1:04d}"
            active.mkdir()
            if source_hashes(args.block) != hashes:
                raise RunFailure("source_changed", "source changed during this batch; synthesis/vector provenance is stale")
            for frame in frames:
                if sha256(campaign / frame["input"]) != frame["sha256"]:
                    raise RunFailure("input_changed", "saved input changed before replay")
            manifest = active / "manifest.txt"
            write_manifest(manifest, vectors, frames, campaign)
            subset = dict(vectors, frames=[dict(frame, input=str(Path("..") / frame["input"])) for frame in frames])
            write_json(active / "vectors.json", subset)
            cfg = active / "test.cfg"
            cfg.write_text(config_text(args.block, width, height, args.mode, manifest), encoding="utf-8")
            print(f"{args.mode} {position + 1}/{len(groups)}: seeds={[f['seed'] for f in frames]}", flush=True)
            try:
                run_command(["vitis-run", "--mode", "hls", f"--{args.stage}", "--config", str(cfg),
                             "--work_dir", str(work)], active, args.timeout, args.stage.upper(), work)
                if source_hashes(args.block) != hashes:
                    raise RunFailure("source_changed", "source changed while the tool was running")
                for frame in frames:
                    if sha256(campaign / frame["input"]) != frame["sha256"]:
                        raise RunFailure("input_changed", "saved input changed while the tool was running")
                check_stage(work, args.stage, vectors, frames)
                write_json(active / "result.json", {"status": "PASS", "frames": len(frames), "stage": args.stage})
            except Exception as error:
                error = classify_tool_failure(error, work, args.stage, active)
                write_json(active / "result.json", {"status": "FAIL", "failure_kind": getattr(error, "kind", "runner_or_input"),
                                                    "message": str(error)})
                raise error
            finally:
                archive_outputs(work, active)
            result["completed_frames"] += len(frames)
            print(f"PASS {position + 1}/{len(groups)} ({result['completed_frames']}/{len(vectors['frames'])} frames)", flush=True)
            write_json(campaign / "result.json", result)
        result.update(status="PASS", failure_kind=None)
        print(f"PASS: {args.block} {args.stage} {args.mode}, {result['completed_frames']} frames", flush=True)
        return 0
    except (Exception, KeyboardInterrupt) as error:
        result.update(failure_kind=getattr(error, "kind", "runner_or_input"), message=str(error))
        print(f"FAIL [{result['failure_kind']}]: {error}", file=sys.stderr, flush=True)
        return 1
    finally:
        if active is not None:
            archive_outputs(work, active)
        result["finished"] = datetime.now().astimezone().isoformat()
        write_json(campaign / "result.json", result)


if __name__ == "__main__":
    sys.exit(main())
