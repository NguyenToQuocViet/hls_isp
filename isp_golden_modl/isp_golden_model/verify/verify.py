#!/usr/bin/env python3
# Bit-exact check of isp_golden_model.py against every member's original golden model.
# Usage (inside the cnn-train container): python verify/verify.py
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
MODEL_DIR = HERE.parent
GOLDEN = MODEL_DIR.parent
VIET, NHAN, TUAN_ANH, HOANG = (GOLDEN / "Vi#U1ec7t", GOLDEN / "Nh#U00e2n", GOLDEN / "Tu#U1ea5n Anh",
                               GOLDEN / "Ho#U00e0ng" / "CNN_golden_model")
sys.path.insert(0, str(MODEL_DIR))
import isp_golden_model as isp  # noqa: E402

W, H = 1920, 1080
WORK = Path(tempfile.mkdtemp(prefix="isp_verify_"))
rng = np.random.default_rng(1234)
failures = []


def check(name, got, ref):
    got, ref = np.asarray(got).astype(np.int64), np.asarray(ref).astype(np.int64)
    ok = got.shape == ref.shape and np.array_equal(got, ref)
    ndiff = -1 if got.shape != ref.shape else int((got != ref).sum())
    print(f"[{'PASS' if ok else 'FAIL'}] {name:58s} shape={got.shape} mismatches={ndiff}")
    if not ok:
        failures.append(name)


def build_harness() -> Path:
    exe = WORK / "harness"
    srcs = [HERE / "harness.cpp", VIET / "BLC/blc.cpp", VIET / "BPC/bpc_adaptive.cpp",
            NHAN / "WB/golden_wb_model.cpp", NHAN / "Demosaic/golden_demosaic_fixed.cpp"]
    incs = [VIET / "BLC", VIET / "BPC", NHAN / "WB", NHAN / "Demosaic"]
    subprocess.run(["g++", "-O2", "-std=c++17", *[f"-I{d}" for d in incs], *map(str, srcs), "-o", str(exe)],
                   check=True)
    return exe


HARNESS = build_harness()


def cpp(mode, arr, w, h, *params, out_shape=None):
    src, dst = WORK / f"in_{mode}.bin", WORK / f"out_{mode}.bin"
    arr.astype("<u2").tofile(src)
    r = subprocess.run([str(HARNESS), mode, str(src), str(dst), str(w), str(h), *map(str, params)],
                       check=True, capture_output=True, text=True)
    if r.stdout.strip():
        print("       C++:", r.stdout.strip())
    return np.fromfile(dst, dtype="<u2").reshape(out_shape or (h, w))


scene10 = isp.synthetic_raw10(W, H, seed=7)
noise10 = rng.integers(0, 1024, (H, W), dtype=np.uint16)

print("== BLC (Viet C++) ==")
for bl in [(64, 64, 64, 64), (60, 70, 66, 80)]:
    cfg = isp.BlcConfig(*bl)
    for tag, frame in [("scene", scene10), ("uniform", noise10)]:
        check(f"BLC {bl} {tag}", isp.blc(frame, cfg), cpp("blc", frame, W, H, *bl))

print("== BPC (Viet C++, fixed 1920x1080) ==")
post_blc = isp.blc(scene10, isp.BlcConfig())
for p in [(32, 32, 32, 3, 1), (20, 40, 25, 2, 2), (0, 0, 0, 8, 8)]:
    cfg = isp.BpcConfig(*p)
    for tag, frame in [("scene", post_blc), ("uniform", noise10)]:
        got, det = isp.bpc(frame, cfg)
        print(f"       python detections={int(det.sum())}")
        check(f"BPC {p} {tag}", got, cpp("bpc", frame, W, H, *p))

print("== WB (Nhan C++) ==")
for g in [(1.0, 1.0, 1.0, 1.0), (1.93, 1.0, 1.0, 1.57), (4.2, 3.9, 3.9, 5.5)]:
    cfg = isp.WbConfig(*g)
    for tag, frame in [("scene", post_blc), ("uniform", noise10)]:
        check(f"WB {g} {tag}", isp.wb(frame, cfg), cpp("wb", frame, W, H, *g))

print("== Demosaic (Nhan C++) ==")
scene12 = np.clip(post_blc.astype(np.int64) * 4 + rng.integers(-40, 40, (H, W)), 0, 4095).astype(np.uint16)
noise12 = rng.integers(0, 4096, (H, W), dtype=np.uint16)
edges12 = (((np.arange(W)[None, :] // 7 + np.arange(H)[:, None] // 5) % 2) * 3000 + 500).astype(np.uint16)
for p in [(32, 16), (0, 0), (200, 100), (4095, 4095)]:
    cfg = isp.DemosaicConfig(*p)
    for tag, frame in [("scene", scene12), ("uniform", noise12), ("edges", edges12)]:
        check(f"Demosaic {p} {tag}", isp.demosaic(frame, cfg), cpp("demosaic", frame, W, H, *p, out_shape=(H, W, 3)))
for (w, h) in [(37, 29), (2, 2), (3, 5)]:
    small = rng.integers(0, 4096, (h, w), dtype=np.uint16)
    check(f"Demosaic (8,4) small {w}x{h}", isp.demosaic(small, isp.DemosaicConfig(8, 4)),
          cpp("demosaic", small, w, h, 8, 4, out_shape=(h, w, 3)))

print("== LTM / Gamma (Tuan Anh Python) ==")
sys.path.insert(0, str(TUAN_ANH / "LTM"))
sys.path.insert(0, str(TUAN_ANH / "Gamma"))
import golden_gamma_fixed  # noqa: E402
import golden_ltm_fixed  # noqa: E402

rgb_scene = isp.demosaic(scene12, isp.DemosaicConfig())
rgb_noise = rng.integers(0, 4096, (H, W, 3), dtype=np.uint16)
rgb_dark = rng.integers(0, 40, (H, W, 3), dtype=np.uint16)
for tag, img in [("scene", rgb_scene), ("uniform", rgb_noise), ("dark", rgb_dark)]:
    check(f"LTM {tag}", isp.ltm(img), golden_ltm_fixed.process_frame(img))
for gm in [0.45, 1 / 2.2]:
    lut = golden_gamma_fixed.generate_gamma_lut(gm)
    for tag, img in [("scene", rgb_scene), ("uniform", rgb_noise)]:
        check(f"Gamma {gm:.4f} {tag}", isp.gamma(img, isp.GammaConfig(gm)), golden_gamma_fixed.process_frame(img, lut))

print("== CNN (Hoang torch W8A8) ==")
import torch  # noqa: E402

os.chdir(WORK)  # the torch golden writes logs/ into cwd
sys.path.insert(0, str(HOANG))
import CNN_Golden_Model_Int8 as hoang  # noqa: E402

for blocks in (2, 4):
    ref_model = hoang.build_w8a8_from_dir(HOANG, blocks)
    mine = isp.CnnDenoiseW8A8(isp.CnnConfig(blocks=blocks))
    q_in = rng.integers(-127, 128, (4, 40, 56)).astype(np.int8)
    ref_q = ref_model.forward_quantized(torch.from_numpy(q_in)[None]).numpy()[0]
    check(f"CNN b{blocks} forward_quantized random int8 40x56", mine.forward_quantized(q_in.astype(np.int64)), ref_q)
    t0 = time.perf_counter()
    got = mine(post_blc)
    t1 = time.perf_counter()
    ref = hoang.infer_fullhd_raw10_w8a8(ref_model, torch.from_numpy(post_blc.astype(np.int32)),
                                        already_black_corrected=True).numpy()
    t2 = time.perf_counter()
    print(f"       numpy {t1 - t0:.1f}s, torch {t2 - t1:.1f}s")
    check(f"CNN b{blocks} full-HD post-BLC scene", got, ref)

# HLS (isp_cnn_denoise) uses Q31 multipliers for the RAW<->INT8 conversions: compare over every code.
m2 = isp.CnnDenoiseW8A8(isp.CnnConfig(blocks=2))
codes = np.arange(-127, 128)
check("CNN output INT8->RAW10: float golden vs HLS L9_OUT_RAW_Q31", m2.output_to_raw(codes),
      np.clip(isp.q31_requant(codes, 1979141884, 3), 0, 959))
raws = np.arange(0, 1024)
check("CNN input RAW->INT8: raw_q31/raw_exp vs HLS L0_RAW_Q31/EXP", m2.quantize_input(raws),
      np.clip(isp.q31_requant(np.minimum(raws, 959), 1218475039, -2), 0, 127))

print("== Full pipeline (synthetic full-HD) ==")
t0 = time.perf_counter()
out = isp.IspGoldenModel().run(scene10, verbose=True)
print(f"       total {time.perf_counter() - t0:.1f}s, final {out['gamma'].shape} {out['gamma'].dtype}")

print()
print("ALL PASS" if not failures else f"FAILURES ({len(failures)}): {failures}")
sys.exit(1 if failures else 0)
