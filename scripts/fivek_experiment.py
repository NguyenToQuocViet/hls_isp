#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: FiveK Experiment Runner
# Description: Filter, split, tune, and evaluate compatible DNG images with resumable records.
# Author: Viet Nguyen To Quoc

import argparse
from concurrent.futures import as_completed, ThreadPoolExecutor
import csv
import hashlib
import json
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile

import numpy as np

from sweep_one_image import table_sums


ROOT = Path(__file__).resolve().parents[1]
ALGORITHM = "adaptive_v2"
SELECTION_POLICY = "adaptive_operating_point_budget_v1"
NAMES = ("RG_hot", "RG_dead", "BRG", "BCR", "IOTCR")
OUTPUT_NAMES = {"split": "split", "sweep": "sweeps", "select": "selection", "test": "test"}


def read(path):
    return json.loads(Path(path).read_text())


def save(path, value):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n")
    temporary.replace(path)


def digest(path):
    with Path(path).open("rb") as file:
        result = hashlib.sha256()
        for block in iter(lambda: file.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def identity(path):
    path = Path(path).resolve()
    stat = path.stat()
    return {"path": str(path), "size": stat.st_size, "mtime_ns": stat.st_mtime_ns}


def check_source(row):
    if identity(row["path"]) != row["source"]:
        raise ValueError(f"Source changed since scan: {row['path']}")


def workspace(path, specification):
    specification = json.loads(json.dumps(specification))
    path = path.resolve()
    path.mkdir(parents=True, exist_ok=True)
    state = path / "run.json"
    if state.exists():
        if read(state) != specification:
            raise ValueError(f"Run inputs changed; use a fresh output directory: {path}")
    else:
        if any(path.iterdir()):
            raise ValueError(f"Output directory is not empty: {path}")
        save(state, specification)
    return path


def specification(args, **extra):
    result = {"command": args.command, "runner_sha256": digest(__file__), **extra}
    if args.command == "scan":
        result["injector_sha256"] = digest(args.injector)
    else:
        result.update(algorithm=ALGORITHM, injector_sha256=digest(args.injector),
                      evaluator_sha256=digest(args.evaluator),
                      sweep_sha256=digest(ROOT / "scripts/sweep_one_image.py"))
    return result


def invoke(command, log, cwd=None):
    with log.open("a") as file:
        file.write(json.dumps(list(map(str, command))) + "\n")
        file.flush()
        result = subprocess.run(list(map(str, command)), cwd=cwd, text=True,
                                stdout=subprocess.PIPE, stderr=file)
        file.write(result.stdout + "\n")
    if result.returncode:
        raise RuntimeError(f"Exit {result.returncode}; see {log}")
    return result.stdout


def csv_write(path, rows, fields):
    with path.open("w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def scan(args):
    sources = sorted(p.resolve() for p in args.root.rglob("*")
                     if p.is_file() and p.suffix.lower() == ".dng")
    if not sources:
        raise ValueError("No DNG files found")
    entries = [{"id": hashlib.sha256(str(p).encode()).hexdigest()[:20],
                "path": str(p), "source": identity(p)} for p in sources]
    out = workspace(args.output, specification(args, sources=entries))
    records = []
    for row in entries:
        target = out / (row["id"] + ".json")
        if target.exists():
            records.append(read(target))
            continue
        # A compatibility failure is a recorded rejection, not a batch failure.
        result = subprocess.run([str(args.injector), "--check-only", row["path"]],
                                capture_output=True, text=True)
        record = {**row, "compatible": result.returncode == 0,
                  "returncode": result.returncode,
                  "reason": (result.stderr + result.stdout).strip()}
        save(target, record)
        records.append(record)
        print(f"{len(records)}/{len(entries)} {row['path']}: {record['compatible']}", flush=True)
    save(out / "manifest.json", {"scan": read(out / "run.json"), "images": records})
    csv_write(out / "compatibility.csv",
              [{k: r[k] for k in ("path", "compatible", "returncode", "reason")} for r in records],
              ["path", "compatible", "returncode", "reason"])


def split(args):
    manifest = read(args.manifest)
    images = [r for r in manifest["images"] if r["compatible"]]
    # Persist assignment before shuffling; every stage reuses the same image seed.
    images = [{**row, "injection_seed": seed}
              for seed, row in enumerate(sorted(images, key=lambda row: row["path"]), 1)]
    groups = {}
    if args.groups:
        with args.groups.open(newline="") as file:
            for row in csv.DictReader(file):
                path = str(Path(row["path"]).resolve())
                if not row["group"] or path in groups:
                    raise ValueError("Group CSV contains empty group or duplicate path")
                groups[path] = row["group"]
        if any(r["path"] not in groups for r in images):
            raise ValueError("Group CSV must cover every compatible image")
    buckets = {}
    for row in images:
        buckets.setdefault(groups.get(row["path"], row["id"]), []).append(row)
    keys = sorted(buckets)
    if len(keys) < 2:
        raise ValueError("Need at least two independent images/groups")
    random.Random(args.seed).shuffle(keys)
    # Choose a group boundary nearest 20% of images while keeping both sets nonempty.
    sizes = np.cumsum([len(buckets[k]) for k in keys])
    cut = min(range(1, len(keys)), key=lambda n: abs(int(sizes[n - 1]) - len(images) * 0.2))
    test_keys = set(keys[:cut])
    result = {"algorithm": ALGORITHM, "manifest_sha256": digest(args.manifest), "seed": args.seed,
              "grouping": "explicit" if args.groups else "per-image; near-duplicates not checked",
              "groups_sha256": digest(args.groups) if args.groups else None,
              "scan": manifest["scan"],
              "tuning": [r for k in keys if k not in test_keys for r in buckets[k]],
              "test": [r for k in keys if k in test_keys for r in buckets[k]]}
    out = workspace(args.output, {"runner_sha256": digest(__file__), **result})
    save(out / "split.json", result)


def load_split(args):
    data = read(args.split)
    if data.get("algorithm") != ALGORITHM:
        raise ValueError(f"Split is not for {ALGORITHM}; derive a fresh split from the existing compatibility manifest")
    for row in data["tuning"] + data["test"]:
        if "injection_seed" not in row:
            raise ValueError("Split has no per-image injection seeds; create a fresh split")
        check_source(row)
    return data


def rates(sums, metadata):
    sums = np.asarray(sums, dtype=np.float64)
    hot = 1 - sums[..., 0] / metadata["initial"][0]
    dead = 1 - sums[..., 1] / metadata["initial"][1]
    return np.stack((hot, dead, (hot + dead) / 2,
                     sums[..., 2] / metadata["valid_count"],
                     sums[..., 3] / metadata["unmodified_count"]), axis=-1)


def select_under_budget(values, keys, bcr_budget, iotcr_budget):
    eligible = [i for i, metrics in enumerate(values)
                if metrics[3] <= bcr_budget and metrics[4] <= iotcr_budget]
    winner = (min(eligible, key=lambda i: (-values[i, 2], values[i, 4], values[i, 3], keys[i]))
              if eligible else None)
    return winner, eligible


def select_matched_operating_points(baseline_values, adaptive_values, configs):
    adaptive_winner = min(range(len(configs)),
                          key=lambda i: (-adaptive_values[i, 2], adaptive_values[i, 4],
                                         adaptive_values[i, 3], configs[i]))
    bcr_budget = adaptive_values[adaptive_winner, 3]
    iotcr_budget = adaptive_values[adaptive_winner, 4]
    baseline_winner, baseline_eligible = select_under_budget(
        baseline_values, range(len(baseline_values)), bcr_budget, iotcr_budget)
    return baseline_winner, adaptive_winner, baseline_eligible


def baseline_bank(args, temp, log):
    path = temp / "baseline.bin"
    meta = json.loads(invoke([args.evaluator, temp, "--baseline-bank", path], log))
    return np.fromfile(path, dtype=np.int64).reshape(4096, 4), meta


def image_rows(args, out, images, action):
    def process(index, row):
        directory = out / row["id"]
        directory.mkdir(exist_ok=True)
        done = directory / "done.json"
        if done.exists():
            return index, row["path"], None
        log = directory / "run.log"
        try:
            check_source(row)
            with tempfile.TemporaryDirectory(prefix="work-", dir=directory) as name:
                temp = Path(name)
                invoke([args.injector, "--seed", row["injection_seed"], row["path"]], log, cwd=temp)
                result = action(row, temp, directory, log)
            save(done, result)
            (directory / "error.json").unlink(missing_ok=True)
            return index, row["path"], None
        except (OSError, ValueError, RuntimeError) as error:
            save(directory / "error.json", {"path": row["path"], "error": str(error)})
            return index, row["path"], str(error)

    failed = []
    executor = ThreadPoolExecutor(max_workers=args.jobs)
    try:
        futures = [executor.submit(process, index, row) for index, row in enumerate(images, 1)]
        for completed, future in enumerate(as_completed(futures), 1):
            index, path, error = future.result()
            if error is not None:
                failed.append((index, path))
            print(f"{completed}/{len(images)} [{index}/{len(images)}] {path}", flush=True)
    except BaseException:
        executor.shutdown(wait=True, cancel_futures=True)
        raise
    else:
        executor.shutdown()
    failed = [path for _, path in sorted(failed)]
    save(out / "failures.json", failed)
    if failed:
        raise RuntimeError(f"{len(failed)} images failed; rerun the same command to retry. No aggregate selection/report produced.")


def sweep(args):
    data = load_split(args)
    out = workspace(args.output, specification(args, split_sha256=digest(args.split),
                                              baseline_threshold=args.baseline_threshold))

    def action(row, temp, directory, log):
        target = temp / "sweep"
        invoke([sys.executable, ROOT / "scripts/sweep_one_image.py", temp,
                "--output", target, "--evaluator", args.evaluator,
                "--baseline-threshold", args.baseline_threshold], log)
        sums, meta = baseline_bank(args, temp, log)
        np.save(directory / "baseline.npy", rates(sums, meta))
        for name in ("summary.json", "candidates.csv"):
            shutil.copyfile(target / name, directory / name)
        return {"path": row["path"], "selected": read(target / "summary.json")["selected"]}

    image_rows(args, out, data["tuning"], action)
    rows = []
    for row in data["tuning"]:
        summary = read(out / row["id"] / "summary.json")
        selected = summary["selected"]
        entry = {"path": row["path"], "config": json.dumps(selected[0]) if selected else "",
                 "status": "selected" if selected else "no eligible candidate"}
        for name in NAMES:
            entry["baseline_" + name] = summary["baseline_metrics"][name]
            entry["adaptive_" + name] = selected[1][name] if selected else ""
        rows.append(entry)
    csv_write(out / "per_image_best.csv", rows,
              ["path", "config", "status"] + [p + n for n in NAMES for p in ("baseline_", "adaptive_")])


def select(args):
    data = load_split(args)
    previous = read(args.sweeps / "run.json")
    expected = specification(args, split_sha256=digest(args.split),
                             baseline_threshold=previous["baseline_threshold"])
    expected["command"] = "sweep"
    if previous != expected:
        raise ValueError("Sweep provenance differs from current split/tools")
    configs = set()
    baseline_values = np.zeros((4096, 5))
    sweep_hashes = {}
    for row in data["tuning"]:
        directory = args.sweeps / row["id"]
        result = read(directory / "done.json")
        if result["selected"]:
            configs.add(tuple(result["selected"][0]))
        baseline_values += np.load(directory / "baseline.npy", allow_pickle=False)
        sweep_hashes[row["id"]] = {n: digest(directory / n) for n in ("done.json", "baseline.npy")}
    if not configs:
        raise ValueError("No eligible per-image winner; no common candidate pool")
    configs = sorted(configs)
    baseline_values /= len(data["tuning"])
    out = workspace(args.output, specification(args, split_sha256=digest(args.split),
                                              sweep_hashes=sweep_hashes, configs=configs,
                                              selection_policy=SELECTION_POLICY))

    def action(row, temp, directory, log):
        path = temp / "metrics.bin"
        meta = json.loads(invoke([args.evaluator, temp, "--bank", path,
                                  previous["baseline_threshold"]], log))
        bank = np.memmap(path, mode="r", dtype=np.int64, shape=(13, 13, 4, 4096, 4))
        values = rates(np.array([table_sums(bank, c) for c in configs]), meta)
        del bank
        np.save(directory / "adaptive.npy", values)
        return {"path": row["path"]}

    image_rows(args, out, data["tuning"], action)
    means = np.zeros((len(configs), 5))
    for row in data["tuning"]:
        means += np.load(out / row["id"] / "adaptive.npy", allow_pickle=False)
    means /= len(data["tuning"])
    baseline_t, winner, baseline_eligible = select_matched_operating_points(
        baseline_values, means, configs)
    if baseline_t is None:
        raise RuntimeError("No baseline threshold satisfies the selected adaptive safety budget")
    baseline_mean = baseline_values[baseline_t]
    adaptive_mean = means[winner]
    bcr_budget = adaptive_mean[3]
    iotcr_budget = adaptive_mean[4]
    baseline_eligible = set(baseline_eligible)
    csv_write(out / "baseline_candidates.csv",
              [{"threshold": t, **dict(zip(NAMES, map(float, m))), "eligible": t in baseline_eligible}
               for t, m in enumerate(baseline_values)], ["threshold", *NAMES, "eligible"])
    csv_write(out / "common_candidates.csv",
              [{"config": json.dumps(c), **dict(zip(NAMES, map(float, m))), "eligible": True}
               for c, m in zip(configs, means)], ["config", *NAMES, "eligible"])
    save(out / "selection.json", {
        "algorithm": ALGORITHM, "selection_policy": SELECTION_POLICY,
        "split_sha256": digest(args.split), "injector_sha256": digest(args.injector),
        "evaluator_sha256": digest(args.evaluator), "tuning_images": len(data["tuning"]),
        "candidate_pool": "union of eligible per-image tuning winners",
        "criterion": "adaptive maximizes mean BRG; baseline maximizes mean BRG under the selected adaptive BCR/IOTCR budget; equal image weights",
        "safety_reference": "selected adaptive operating point",
        "safety_budget": {"BCR": float(bcr_budget), "IOTCR": float(iotcr_budget)},
        "baseline_eligible_thresholds": len(baseline_eligible),
        "adaptive_candidates": len(configs),
        "baseline_threshold": baseline_t, "baseline_mean": dict(zip(NAMES, map(float, baseline_mean))),
        "adaptive_config": configs[winner] if winner is not None else None,
        "adaptive_mean": dict(zip(NAMES, map(float, adaptive_mean))) if winner is not None else None,
        "status": "selected" if winner is not None else "no eligible common candidate"})


def test(args):
    data = load_split(args)
    chosen = read(args.selection)
    if chosen.get("algorithm") != ALGORITHM:
        raise ValueError(f"Selection is not for {ALGORITHM}")
    if chosen.get("selection_policy") != SELECTION_POLICY:
        raise ValueError(f"Selection does not use {SELECTION_POLICY}")
    for key, value in (("split_sha256", digest(args.split)), ("injector_sha256", digest(args.injector)),
                       ("evaluator_sha256", digest(args.evaluator))):
        if chosen[key] != value:
            raise ValueError("Selection provenance differs from current split/tools")
    if chosen["adaptive_config"] is None:
        raise ValueError("No common config selected; test cannot run")
    out = workspace(args.output, specification(args, split_sha256=digest(args.split),
                                              selection_sha256=digest(args.selection)))

    def action(row, temp, directory, log):
        result = json.loads(invoke([args.evaluator, temp, "--fixed", chosen["baseline_threshold"],
                                    *chosen["adaptive_config"]], log))
        return {"path": row["path"], "raw": result,
                "baseline": rates(result["baseline"], result).tolist(),
                "adaptive": rates(result["adaptive"], result).tolist()}

    image_rows(args, out, data["test"], action)
    results = [read(out / r["id"] / "done.json") for r in data["test"]]
    baseline = np.array([r["baseline"] for r in results])
    adaptive = np.array([r["adaptive"] for r in results])
    rows = [{"path": r["path"], **{p + "_" + n: r[p][i] for p in ("baseline", "adaptive")
                                     for i, n in enumerate(NAMES)}} for r in results]
    csv_write(out / "per_image.csv", rows,
              ["path"] + [p + "_" + n for p in ("baseline", "adaptive") for n in NAMES])
    report = {"images": len(results), "selection": chosen, "aggregation": "equal image weights",
              "BRG_better_fraction": float(np.mean(adaptive[:, 2] > baseline[:, 2])),
              "BRG_worse_fraction": float(np.mean(adaptive[:, 2] < baseline[:, 2])),
              "BRG_equal_fraction": float(np.mean(adaptive[:, 2] == baseline[:, 2]))}
    for label, values in (("baseline", baseline), ("adaptive", adaptive)):
        mean = values.mean(axis=0)
        report[label] = {"mean": dict(zip(NAMES, map(float, mean))),
                         "median": dict(zip(NAMES, map(float, np.median(values, axis=0)))),
                         "within_tuning_safety_budget": bool(
                             mean[3] <= chosen["safety_budget"]["BCR"] and
                             mean[4] <= chosen["safety_budget"]["IOTCR"])}
    save(out / "report.json", report)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--injector", type=Path, default=ROOT / "build/defect_injector")
    parser.add_argument("--evaluator", type=Path, default=ROOT / "build/bpc_evaluate")
    commands = parser.add_subparsers(dest="command", required=True)
    p = commands.add_parser("scan")
    p.add_argument("root", type=Path)
    p = commands.add_parser("split")
    p.add_argument("manifest", type=Path)
    p.add_argument("--groups", type=Path, help="CSV path,group; absolute source paths, all compatible images")
    p.add_argument("--seed", type=int, default=20260907)
    for name in ("sweep", "select", "test"):
        p = commands.add_parser(name)
        p.add_argument("split", type=Path)
        p.add_argument("--jobs", type=int, default=1, help="number of images processed concurrently")
        if name == "sweep":
            p.add_argument("--baseline-threshold", type=int, default=1063)
        elif name == "select":
            p.add_argument("--sweeps", type=Path, required=True)
        else:
            p.add_argument("--selection", type=Path, required=True)
    commands.choices["scan"].add_argument("--output", type=Path, required=True)
    for name, directory in OUTPUT_NAMES.items():
        commands.choices[name].add_argument("--output", type=Path,
                                            default=ROOT / "artifacts" / ALGORITHM / directory)
    args = parser.parse_args()
    args.injector = args.injector.resolve()
    args.evaluator = args.evaluator.resolve()
    if hasattr(args, "baseline_threshold") and not 0 <= args.baseline_threshold <= 4095:
        parser.error("baseline threshold must be in [0,4095]")
    if hasattr(args, "jobs") and args.jobs < 1:
        parser.error("jobs must be at least 1")
    try:
        globals()[args.command](args)
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
