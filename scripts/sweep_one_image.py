#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: One Image Adaptive BPC Sweep
# Description: Run the documented coarse/refine schedule using exact C++ metric tables.
# Author: Viet Nguyen To Quoc

import argparse
import csv
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
import time

import numpy as np


ALGORITHM = "adaptive_v2"


def neighborhood(center, radius, step, maximum):
    return sorted({max(0, min(maximum, center + offset)) for offset in range(-radius, radius + 1, step)})


def table_sums(bank, config):
    red, green, blue, signal, activity = config
    return bank[signal, activity, 0, red] + bank[signal, activity, 1, green] + bank[signal, activity, 2, green] + bank[signal, activity, 3, blue]


def metrics(sums, metadata):
    hot = 1 - int(sums[0]) / metadata["initial"][0]
    dead = 1 - int(sums[1]) / metadata["initial"][1]
    return {
        "RG_hot": hot, "RG_dead": dead, "BRG": (hot + dead) / 2,
        "BCR": int(sums[2]) / metadata["valid_count"],
        "IOTCR": int(sums[3]) / metadata["unmodified_count"],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="Injector output directory containing the post-BLC pair")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--evaluator", type=Path, default=Path(__file__).resolve().parents[1] / "build/bpc_evaluate")
    parser.add_argument("--baseline-threshold", type=int, default=1063)
    args = parser.parse_args()
    args.input = args.input.resolve()
    args.evaluator = args.evaluator.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    bank_path = args.output / "metrics.bin"
    metadata = json.loads(subprocess.check_output([str(args.evaluator), str(args.input), "--bank", str(bank_path), str(args.baseline_threshold)], text=True))
    bank = np.memmap(bank_path, mode="r", dtype=np.int64, shape=(13, 13, 4, 4096, 4))

    # Exact integer comparisons before any ranking; includes cutoff extremes and independent colors.
    configs = [(0, 0, 0, 0, 0), (0, 0, 0, 12, 12), (4095, 4095, 4095, 12, 12), (64, 128, 256, 3, 6)]
    rng = np.random.default_rng(20260907)
    configs += [tuple(map(int, list(rng.integers(0, 4096, 3)) + list(rng.integers(0, 13, 2)))) for _ in range(8)]

    def verify(config):
        observed = json.loads(subprocess.check_output([str(args.evaluator), str(args.input), "--oracle", *map(str, config)], text=True))
        expected = table_sums(bank, config).tolist()
        if observed != expected:
            raise RuntimeError(f"Metric bank differs from reference at {config}: {expected} != {observed}")

    for config in configs:
        verify(config)
    print(f"Reference equivalence: {len(configs)} configurations PASS", flush=True)

    seen = {}
    stage_counts = {}
    baseline = metadata["baseline"]
    fieldnames = ["stage", "T0_R", "T0_G", "T0_B", "k_s", "k_a", "RG_hot", "RG_dead", "BRG", "BCR", "IOTCR", "eligible"]

    with (args.output / "candidates.csv").open("w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fieldnames)
        writer.writeheader()

        def evaluate(configs, stage):
            result = []
            added = 0
            for config in configs:
                if config not in seen:
                    sums = table_sums(bank, config)
                    eligible = int(sums[2]) <= baseline[2] and int(sums[3]) <= baseline[3]
                    row = dict(zip(fieldnames[1:6], config))
                    row.update(stage=stage, **metrics(sums, metadata), eligible=eligible)
                    writer.writerow(row)
                    seen[config] = row
                    added += 1
                result.append((config, seen[config]))
            stage_counts[stage] = added
            file.flush()
            print(f"{stage}: {added} new candidates", flush=True)
            return sorted((entry for entry in result if entry[1]["eligible"]), key=lambda entry: (-entry[1]["BRG"], entry[1]["IOTCR"], entry[1]["BCR"], entry[0]))

        stage1 = evaluate(((threshold, threshold, threshold, signal, activity) for threshold in list(range(0, 4096, 256)) + [4095] for signal in range(13) for activity in range(13)), "coarse")
        selected = None
        if stage1:
            regions = []
            used_thresholds = set()
            for config, _ in stage1:
                if config[0] not in used_thresholds:
                    regions.append(config)
                    used_thresholds.add(config[0])
                    if len(regions) == 3:
                        break
            stage2_configs = itertools.chain.from_iterable(itertools.product(neighborhood(r, 256, 64, 4095), neighborhood(g, 256, 64, 4095), neighborhood(b, 256, 64, 4095), neighborhood(s, 2, 1, 12), neighborhood(a, 2, 1, 12)) for r, g, b, s, a in regions)
            stage2 = evaluate(stage2_configs, "per_cfa")
            r, g, b, s, a = stage2[0][0]
            stage3 = evaluate(itertools.product(neighborhood(r, 64, 16, 4095), neighborhood(g, 64, 16, 4095), neighborhood(b, 64, 16, 4095), neighborhood(s, 2, 1, 12), neighborhood(a, 2, 1, 12)), "refine")
            r, g, b, _, _ = stage3[0][0]
            final = evaluate(((r, g, b, s, a) for s in range(13) for a in range(13)), "shift_audit")
            selected = final[0]
            verify(selected[0])

    source_hashes = {name: hashlib.sha256((args.input / name).read_bytes()).hexdigest() for name in ("reference_blc.pgm", "corrupted_blc.pgm", "defects.csv", "input_metadata.json")}
    summary = {
        "algorithm": ALGORITHM,
        "claim": "One-image adaptive_v2 exploratory sweep; no held-out evaluation or generalization claim",
        "input": str(args.input), "input_sha256": source_hashes,
        "evaluator_sha256": hashlib.sha256(args.evaluator.read_bytes()).hexdigest(),
        "metadata": metadata, "baseline_metrics": metrics(baseline, metadata),
        "stage_new_candidates": stage_counts, "unique_candidates": len(seen),
        "oracle_configs": configs, "selected": selected,
        "seconds": time.monotonic() - started,
    }
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({"unique_candidates": len(seen), "selected": selected, "seconds": summary["seconds"]}, indent=2))


if __name__ == "__main__":
    main()
