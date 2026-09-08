#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: FiveK Experiment Directed Tests
# Description: Check manifest reuse, deterministic splitting, and parallel image execution.
# Author: Viet Nguyen To Quoc

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
from types import SimpleNamespace

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import fivek_experiment


def identity(path):
    stat = path.stat()
    return {"path": str(path.resolve()), "size": stat.st_size, "mtime_ns": stat.st_mtime_ns}


def main():
    # Select adaptive for maximum BRG first, then train baseline under the
    # selected adaptive operating point's BCR/IOTCR budget.
    baseline = np.array([
        [0.0, 0.0, 0.99, 0.90, 0.10],
        [0.0, 0.0, 0.80, 0.01, 0.001],
        [0.0, 0.0, 0.70, 0.005, 0.0005],
    ])
    adaptive = np.array([
        [0.0, 0.0, 0.95, 0.80, 0.08],
        [0.0, 0.0, 0.85, 0.009, 0.0009],
        [0.0, 0.0, 0.75, 0.004, 0.0004],
    ])
    baseline_winner, adaptive_winner, baseline_eligible = \
        fivek_experiment.select_matched_operating_points(
            baseline, adaptive, [(0,), (1,), (2,)])
    assert baseline_winner == 1 and baseline_eligible == [1, 2]
    assert adaptive_winner == 0

    for command in ("sweep", "select", "test"):
        result = subprocess.run(["python3", str(ROOT / "scripts/fivek_experiment.py"), command, "--help"],
                                capture_output=True, text=True)
        assert result.returncode == 0 and "--jobs JOBS" in result.stdout

    with tempfile.TemporaryDirectory(prefix="fivek-v2-") as temporary:
        directory = Path(temporary)
        images = []
        for index in range(5):
            source = directory / f"image_{index}.dng"
            source.write_bytes(bytes([index]))
            images.append({"id": hashlib.sha256(str(source.resolve()).encode()).hexdigest()[:20],
                           "path": str(source.resolve()), "source": identity(source),
                           "compatible": True, "returncode": 0, "reason": "passed"})
        manifest = directory / "manifest.json"
        manifest.write_text(json.dumps({"scan": {"injector_sha256": "historical-scan-binary"},
                                        "images": images}))

        output = directory / "split"
        command = ["python3", str(ROOT / "scripts/fivek_experiment.py"), "split",
                   str(manifest), "--seed", "20260907", "--output", str(output)]
        subprocess.run(command, check=True)
        split = json.loads((output / "split.json").read_text())
        assigned = split["tuning"] + split["test"]
        assert split["algorithm"] == "adaptive_v2"
        assert len(split["tuning"]) == 4 and len(split["test"]) == 1
        assert sorted(row["injection_seed"] for row in assigned) == [1, 2, 3, 4, 5]
        assert {row["path"] for row in assigned} == {row["path"] for row in images}
        loaded = fivek_experiment.load_split(SimpleNamespace(split=output / "split.json"))
        assert loaded == split

        result = subprocess.run(command, capture_output=True, text=True)
        assert result.returncode == 0

        parallel = directory / "parallel"
        parallel.mkdir()
        barrier = threading.Barrier(2, timeout=2)

        def action(row, temporary, target, log):
            del temporary, target, log
            barrier.wait()
            return {"path": row["path"]}

        fivek_experiment.image_rows(SimpleNamespace(injector=Path("/bin/true"), jobs=2),
                                    parallel, assigned[:2], action)
        assert all((parallel / row["id"] / "done.json").exists() for row in assigned[:2])

    print("Adaptive-budget baseline selection, manifest reuse, deterministic split, resume and parallel execution: PASS")


if __name__ == "__main__":
    main()
