#!/usr/bin/env python3
# Integrated bit-accurate ISP golden model (RGGB, RAW10 in -> RGB8 out):
#   BLC -> BPC -> CNN denoise -> WB -> Demosaic -> CCM -> LTM -> Gamma
# Each stage reproduces one team member's fixed-point golden model:
#   BLC, BPC     : Viet     golden/Viet/BLC/blc.cpp, golden/Viet/BPC/bpc_adaptive.cpp
#   CNN denoise  : Hoang    golden/Hoang/CNN_golden_model/CNN_Golden_Model_Int8.py
#   WB, Demosaic : Nhan     golden/Nhan/WB/golden_wb_model.cpp, golden/Nhan/Demosaic/golden_demosaic_fixed.cpp
#   CCM          : Nhan     golden/Nhan/CCM/golden_ccm_model.cpp
#   LTM, Gamma   : Tuan Anh golden/Tuan Anh/LTM/golden_ltm_fixed.py, golden/Tuan Anh/Gamma/golden_gamma_fixed.py
from __future__ import annotations

import argparse
import json
import math
import struct
import time
import zlib
from dataclasses import asdict, dataclass, field
from functools import lru_cache
from pathlib import Path

import numpy as np

SCRIPT_DIR = Path(__file__).resolve().parent

RAW10_MAX = 1023
RAW12_MAX = 4095
ACT_QMIN, ACT_QMAX = -127, 127
INT32_MIN, INT32_MAX = -(1 << 31), (1 << 31) - 1

STAGES = ("blc", "bpc", "cnn", "wb", "demosaic", "ccm", "ltm", "gamma")
BYPASSABLE = {"blc", "bpc", "cnn", "wb", "ccm", "ltm"}


# ============================================================================
# Configuration
# ============================================================================

@dataclass
class BlcConfig:
    black_level_r: int = 64
    black_level_gr: int = 64
    black_level_gb: int = 64
    black_level_b: int = 64


@dataclass
class BpcConfig:
    base_threshold_r: int = 32
    base_threshold_g: int = 32
    base_threshold_b: int = 32
    signal_shift: int = 3
    activity_shift: int = 1


@dataclass
class CnnConfig:
    blocks: int = 2
    param_dir: str = str(SCRIPT_DIR / "params")


@dataclass
class WbConfig:
    gain_r: float = 1.512804
    gain_gr: float = 1.0
    gain_gb: float = 1.0
    gain_b: float = 1.916974


@dataclass
class DemosaicConfig:
    edge_threshold: int = 270
    edge_mag: int = 23


@dataclass
class CcmConfig:
    matrix: list = field(default_factory=lambda: [[1.6, -0.3, -0.3],
                                                  [-0.2, 1.4, -0.2],
                                                  [-0.1, -0.6, 1.7]])


@dataclass
class LtmConfig:
    exposure_gain: float = 1.0


@dataclass
class GammaConfig:
    gamma: float = 0.45


@dataclass
class IspConfig:
    blc: BlcConfig = field(default_factory=BlcConfig)
    bpc: BpcConfig = field(default_factory=BpcConfig)
    cnn: CnnConfig = field(default_factory=CnnConfig)
    wb: WbConfig = field(default_factory=WbConfig)
    demosaic: DemosaicConfig = field(default_factory=DemosaicConfig)
    ccm: CcmConfig = field(default_factory=CcmConfig)
    ltm: LtmConfig = field(default_factory=LtmConfig)
    gamma: GammaConfig = field(default_factory=GammaConfig)
    bypass: list = field(default_factory=list)

    @classmethod
    def from_json(cls, path: str | Path) -> "IspConfig":
        cfg = cls()
        for section_name, values in json.loads(Path(path).read_text()).items():
            if section_name == "bypass":
                cfg.bypass = list(values)
                continue
            if section_name not in cls.__dataclass_fields__:
                raise KeyError(f"Unknown config section '{section_name}'")
            section = getattr(cfg, section_name)
            for key, value in values.items():
                if key not in type(section).__dataclass_fields__:
                    raise KeyError(f"Unknown config key '{section_name}.{key}'")
                setattr(section, key, value)
        cfg.validate()
        return cfg

    def validate(self) -> None:
        bad = set(self.bypass) - BYPASSABLE
        if bad:
            raise ValueError(f"Stages {sorted(bad)} cannot be bypassed; bypassable: {sorted(BYPASSABLE)}")


def cfa_plane(height: int, width: int, r, gr, gb, b, dtype) -> np.ndarray:
    """Per-pixel RGGB parameter map: R at (even,even), Gr (even,odd), Gb (odd,even), B (odd,odd)."""
    tile = np.array([[r, gr], [gb, b]], dtype=dtype)
    return np.tile(tile, ((height + 1) // 2, (width + 1) // 2))[:height, :width]


# ============================================================================
# BLC (Viet) - per-CFA black level subtraction, floor at 0
# ============================================================================

def blc(raw: np.ndarray, cfg: BlcConfig) -> np.ndarray:
    level = cfa_plane(*raw.shape, cfg.black_level_r, cfg.black_level_gr,
                      cfg.black_level_gb, cfg.black_level_b, np.int32)
    x = raw.astype(np.int32)
    return np.where(x > level, x - level, 0).astype(np.uint16)


# ============================================================================
# BPC (Viet) - adaptive directional bad pixel correction, 2-pixel border bypass
# ============================================================================

def bpc(raw: np.ndarray, cfg: BpcConfig) -> tuple[np.ndarray, np.ndarray]:
    """Returns (corrected frame, detection mask)."""
    h, w = raw.shape
    out = raw.copy()
    detected_full = np.zeros((h, w), dtype=bool)
    if h < 5 or w < 5:
        return out, detected_full

    x = raw.astype(np.int64)

    def nb(dy: int, dx: int) -> np.ndarray:
        return x[2 + dy:h - 2 + dy, 2 + dx:w - 2 + dx]

    # Same-phase pairs in H, V, D1, D2 order; argmin keeps the first minimum, matching the C++ tie priority.
    pairs = ((nb(0, -2), nb(0, 2)), (nb(-2, 0), nb(2, 0)), (nb(-2, -2), nb(2, 2)), (nb(-2, 2), nb(2, -2)))
    activity = np.stack([np.abs(p - q) for p, q in pairs])
    direction = np.argmin(activity, axis=0)[None]
    min_activity = np.take_along_axis(activity, direction, axis=0)[0]
    prediction = np.take_along_axis(np.stack([p + q for p, q in pairs]), direction, axis=0)[0] >> 1

    base = cfa_plane(h, w, cfg.base_threshold_r, cfg.base_threshold_g, cfg.base_threshold_g,
                     cfg.base_threshold_b, np.int64)[2:h - 2, 2:w - 2]
    threshold = base + (prediction >> cfg.signal_shift) + (min_activity >> cfg.activity_shift)

    center = nb(0, 0)
    detected = np.abs(center - prediction) > threshold
    out[2:h - 2, 2:w - 2] = np.where(detected, prediction, center).astype(np.uint16)
    detected_full[2:h - 2, 2:w - 2] = detected
    return out, detected_full


# ============================================================================
# CNN denoise (Hoang) - LocalResNet-Micro true-integer W8A8 inference
# ============================================================================

def q31_requant(x: np.ndarray, q31: int, exponent: int) -> np.ndarray:
    """x * q31 * 2^(exponent-31) with signed round-half-to-even (matches HLS q31_round)."""
    product = x.astype(np.int64) * np.int64(q31)
    shift = 31 - int(exponent)
    if shift <= 0:
        y = product << (-shift)
    else:
        mag = np.abs(product)
        q = mag >> shift
        rem = mag & ((1 << shift) - 1)
        half = 1 << (shift - 1)
        q = q + ((rem > half) | ((rem == half) & ((q & 1) == 1))).astype(np.int64)
        y = np.where(product < 0, -q, q)
    if y.size and (y.min() < INT32_MIN or y.max() > INT32_MAX):
        raise OverflowError("Requantization output exceeds INT32 range.")
    return y


def q31_requant_per_channel(x: np.ndarray, q31: np.ndarray, exponent: np.ndarray) -> np.ndarray:
    return np.stack([q31_requant(x[c], int(q31[c]), int(exponent[c])) for c in range(x.shape[0])])


def sat8(x: np.ndarray) -> np.ndarray:
    return np.clip(x, ACT_QMIN, ACT_QMAX)


class CnnDenoiseW8A8:
    ROW_BAND = 64

    def __init__(self, cfg: CnnConfig):
        if cfg.blocks not in (2, 4):
            raise ValueError(f"cnn.blocks must be 2 or 4, got {cfg.blocks}")
        stem = f"local_resnet_micro_b{cfg.blocks}__w8a8_integer"
        param_dir = Path(cfg.param_dir)
        manifest = json.loads((param_dir / f"{stem}__manifest.json").read_text())
        if (manifest.get("model") != f"local_resnet_micro_b{cfg.blocks}"
                or manifest.get("precision") != "w8a8_integer"
                or len(manifest.get("blocks", [])) != cfg.blocks):
            raise ValueError(f"Manifest does not describe local_resnet_micro_b{cfg.blocks} w8a8_integer")
        with np.load(param_dir / f"{stem}__integer_artifacts.npz", allow_pickle=False) as z:
            art = {k: z[k].copy() for k in z.files}

        def scalar(key: str):
            return np.asarray(art[key]).item()

        self.blocks = cfg.blocks
        self.post_blc_min, self.post_blc_max = (int(v) for v in manifest["raw_input"]["post_blc_range"])
        self.raw_q = (int(scalar("raw_q31")), int(scalar("raw_exp")))
        self.head = self._conv(art, "head")
        self.head_q = (art["head_q31"].astype(np.int64), art["head_exp"].astype(np.int64))
        self.res_blocks = [
            {
                "conv1": self._conv(art, f"block{i}__conv1"),
                "conv2": self._conv(art, f"block{i}__conv2"),
                "conv1_q": (art[f"block{i}__conv1_q31"].astype(np.int64), art[f"block{i}__conv1_exp"].astype(np.int64)),
                "main_q": (art[f"block{i}__main_q31"].astype(np.int64), art[f"block{i}__main_exp"].astype(np.int64)),
                "skip_q": (int(scalar(f"block{i}__skip_q31")), int(scalar(f"block{i}__skip_exp"))),
            }
            for i in range(cfg.blocks)
        ]
        self.tail = self._conv(art, "tail")
        self.tail_q = (art["tail_q31"].astype(np.int64), art["tail_exp"].astype(np.int64))
        self.global_skip_q = (int(scalar("global_skip_q31")), int(scalar("global_skip_exp")))
        # float32 arithmetic mirrors the torch golden: round(q * float32(output_scale * POST_BLC_MAX)).
        self.output_raw_scale = np.float32(np.float32(float(scalar("output_scale"))) * np.float32(self.post_blc_max))

    @staticmethod
    def _conv(art: dict, prefix: str) -> tuple[np.ndarray, np.ndarray]:
        weight, bias = art[f"{prefix}__weight_int"], art[f"{prefix}__bias_int32"]
        if weight.dtype != np.int8 or bias.dtype != np.int32 or weight.shape[-2:] != (3, 3):
            raise TypeError(f"{prefix}: expected int8 3x3 weights and int32 bias")
        bound = ACT_QMAX * np.abs(weight.astype(np.int64)).reshape(weight.shape[0], -1).sum(1) + np.abs(bias.astype(np.int64))
        if bound.max() > INT32_MAX:
            raise OverflowError(f"{prefix}: INT32 accumulator unsafe")
        return weight, bias

    def _conv3x3(self, x: np.ndarray, conv: tuple[np.ndarray, np.ndarray]) -> np.ndarray:
        # Zero-padded 3x3 cross-correlation (torch conv2d, padding=1). float64 GEMM is exact:
        # every partial sum is an integer far below 2^53.
        weight, bias = conv
        cin, h, w = x.shape
        xp = np.pad(x.astype(np.float64), ((0, 0), (1, 1), (1, 1)))
        wm = weight.reshape(weight.shape[0], -1).astype(np.float64)
        out = np.empty((weight.shape[0], h, w), dtype=np.int64)
        for y0 in range(0, h, self.ROW_BAND):
            y1 = min(h, y0 + self.ROW_BAND)
            cols = np.stack([xp[:, y0 + ky:y1 + ky, kx:kx + w] for ky in range(3) for kx in range(3)], axis=1)
            out[:, y0:y1] = (wm @ cols.reshape(cin * 9, -1)).reshape(-1, y1 - y0, w).astype(np.int64)
        return out + bias.astype(np.int64)[:, None, None]

    def quantize_input(self, packed_post_blc: np.ndarray) -> np.ndarray:
        raw = np.clip(packed_post_blc.astype(np.int64), self.post_blc_min, self.post_blc_max)
        return sat8(q31_requant(raw, *self.raw_q))

    def forward_quantized(self, q_in: np.ndarray) -> np.ndarray:
        """INT8 packed [4,H/2,W/2] -> INT8 packed [4,H/2,W/2]."""
        y = sat8(q31_requant_per_channel(np.maximum(self._conv3x3(q_in, self.head), 0), *self.head_q))
        for blk in self.res_blocks:
            mid = sat8(q31_requant_per_channel(np.maximum(self._conv3x3(y, blk["conv1"]), 0), *blk["conv1_q"]))
            main = q31_requant_per_channel(self._conv3x3(mid, blk["conv2"]), *blk["main_q"])
            y = sat8(main + q31_requant(y, *blk["skip_q"]))
        main = q31_requant_per_channel(self._conv3x3(y, self.tail), *self.tail_q)
        return sat8(main + q31_requant(q_in, *self.global_skip_q))

    def output_to_raw(self, q_out: np.ndarray) -> np.ndarray:
        raw = np.round(q_out.astype(np.float32) * self.output_raw_scale)
        return np.clip(raw, 0, self.post_blc_max).astype(np.uint16)

    def __call__(self, raw_post_blc: np.ndarray) -> np.ndarray:
        h, w = raw_post_blc.shape
        if h % 2 or w % 2:
            raise ValueError(f"CNN needs even frame size, got {w}x{h}")
        packed = np.stack((raw_post_blc[0::2, 0::2], raw_post_blc[0::2, 1::2],
                           raw_post_blc[1::2, 0::2], raw_post_blc[1::2, 1::2]))
        out_packed = self.output_to_raw(self.forward_quantized(self.quantize_input(packed)))
        out = np.empty((h, w), dtype=np.uint16)
        out[0::2, 0::2], out[0::2, 1::2], out[1::2, 0::2], out[1::2, 1::2] = out_packed
        return out


# ============================================================================
# WB (Nhan) - Q4.12 per-CFA gain and RAW10-to-RAW12 x4 mapping
# ============================================================================

def quantize_gain_q4_12(gain: float) -> int:
    return int(max(0.0, min(65535.0, math.floor(gain * 4096.0 + 0.5))))


def wb(raw: np.ndarray, cfg: WbConfig) -> np.ndarray:
    if raw.max(initial=0) > RAW10_MAX:
        raise ValueError("WB: input is not RAW10")
    gains = cfa_plane(*raw.shape, quantize_gain_q4_12(cfg.gain_r), quantize_gain_q4_12(cfg.gain_gr),
                      quantize_gain_q4_12(cfg.gain_gb), quantize_gain_q4_12(cfg.gain_b), np.int64)
    # gain is Q4.12. Multiplying by four maps RAW10 codes onto the RAW12
    # numerical range before saturation. This is equivalent to
    # (((raw * gain_q12) << 2) + 2048) >> 12, simplified bit-exactly below.
    return np.minimum((raw.astype(np.int64) * gains + 512) >> 10, RAW12_MAX).astype(np.uint16)


# ============================================================================
# Demosaic (Nhan) - edge-directed G with raster direction vote, color-difference R/B, Q12.4
# ============================================================================

def _mirror101(idx: np.ndarray, limit: int) -> np.ndarray:
    idx = idx.copy()
    idx[idx < 0] = 1
    idx[idx >= limit] = limit - 2
    return idx


def _round_sat_q4(v: np.ndarray) -> np.ndarray:
    return np.where(v <= 0, 0, np.minimum((v + 8) >> 4, RAW12_MAX)).astype(np.uint16)


def demosaic(raw: np.ndarray, cfg: DemosaicConfig) -> np.ndarray:
    """RAW12 [H,W] -> RGB12 [H,W,3]."""
    h, w = raw.shape
    if h < 2 or w < 2:
        raise ValueError("Demosaic: invalid frame size")
    if raw.max(initial=0) > RAW12_MAX:
        raise ValueError("Demosaic: input is not RAW12")

    x = raw.astype(np.int64)
    rows, cols = np.arange(h), np.arange(w)
    cl, cr = _mirror101(cols - 1, w), _mirror101(cols + 1, w)
    rt, rb = _mirror101(rows - 1, h), _mirror101(rows + 1, h)

    # Stage 1: G interpolation. Candidates are computed everywhere, used only at R/B sites.
    left, right, top, bottom = x[:, cl], x[:, cr], x[rt, :], x[rb, :]
    gh_q4 = ((left + right) >> 1) << 4
    gv_q4 = ((top + bottom) >> 1) << 4
    grad_h, grad_v = np.abs(left - right), np.abs(top - bottom)
    flat = np.maximum(grad_h, grad_v) < cfg.edge_mag
    strong = np.abs(grad_h - grad_v) > cfg.edge_threshold
    strong_dir = (grad_h > grad_v).astype(np.uint8)

    # The direction map is a raster recurrence (left neighbour + vote from the row above), so it runs per pixel.
    direction = np.zeros((h, w), dtype=np.uint8)
    for r in range(h):
        rb_cols = np.arange(r & 1, w, 2)
        if r:
            prev = direction[r - 1].astype(np.int64)
            vote_prev = (prev[cl[rb_cols]] + prev[rb_cols] + prev[cr[rb_cols]]).tolist()
        else:
            vote_prev = [0] * len(rb_cols)
        fl, st, sd = flat[r, rb_cols].tolist(), strong[r, rb_cols].tolist(), strong_dir[r, rb_cols].tolist()
        chosen = [0] * len(rb_cols)
        dir_left = 0
        for k in range(len(rb_cols)):
            if fl[k]:
                c = dir_left
            elif st[k]:
                c = sd[k]
            else:
                c = 1 if dir_left + vote_prev[k] > 2 else 0
            chosen[k] = c
            dir_left = c
        row_dir = direction[r]
        row_dir[rb_cols] = chosen
        # A G site inherits dir_left, i.e. the R/B site just before it (0 at column 0).
        g_cols = np.arange(1 - (r & 1), w, 2)
        row_dir[g_cols] = np.where(g_cols >= 1, row_dir[np.maximum(g_cols - 1, 0)], 0)

    rb_site = (rows[:, None] & 1) == (cols[None, :] & 1)
    green = np.where(rb_site,
                     np.where(flat, (gh_q4 + gv_q4) >> 1, np.where(direction == 1, gv_q4, gh_q4)),
                     x << 4)

    # Stage 2: color-difference interpolation of the missing R/B, all in Q12.4.
    diag_raw = x[rt][:, cl] + x[rt][:, cr] + x[rb][:, cl] + x[rb][:, cr]
    diag_g = green[rt][:, cl] + green[rt][:, cr] + green[rb][:, cl] + green[rb][:, cr]
    hor_raw, hor_g = x[:, cl] + x[:, cr], green[:, cl] + green[:, cr]
    ver_raw, ver_g = x[rt, :] + x[rb, :], green[rt, :] + green[rb, :]
    diag_q4 = green + ((diag_raw >> 2) << 4) - (diag_g >> 2)
    hor_q4 = green + ((hor_raw >> 1) << 4) - (hor_g >> 1)
    ver_q4 = green + ((ver_raw >> 1) << 4) - (ver_g >> 1)

    row_even, col_even = (rows[:, None] & 1) == 0, (cols[None, :] & 1) == 0
    sites = [row_even & col_even, row_even & ~col_even, ~row_even & col_even, ~row_even & ~col_even]  # R, Gr, Gb, B
    r_q4 = np.select(sites, [x << 4, hor_q4, ver_q4, diag_q4])
    b_q4 = np.select(sites, [diag_q4, ver_q4, hor_q4, x << 4])
    return np.stack([_round_sat_q4(r_q4), _round_sat_q4(green), _round_sat_q4(b_q4)], axis=-1)


# ============================================================================
# CCM (Nhan) - 3x3 color matrix, signed Q4.12 coefficients, RGB12 -> RGB12
# ============================================================================

def quantize_coeff_q4_12(value: float) -> int:
    return int(max(-32768.0, min(32767.0, math.floor(value * 4096.0 + 0.5))))


def ccm(img_in: np.ndarray, cfg: CcmConfig) -> np.ndarray:
    m = np.array([[quantize_coeff_q4_12(v) for v in row] for row in cfg.matrix], dtype=np.int64)
    if m.shape != (3, 3):
        raise ValueError(f"ccm.matrix must be 3x3, got {m.shape}")
    acc = img_in.astype(np.int64) @ m.T
    return np.where(acc <= 0, 0, np.minimum((acc + 2048) >> 12, RAW12_MAX)).astype(np.uint16)


# ============================================================================
# LTM (Tuan Anh) - log-domain adaptive guided filter + extended Reinhard, RGB12 -> RGB12
# ============================================================================

@lru_cache(maxsize=1)
def _ltm_luts(exposure_gain: float) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    log_lut = np.zeros(4096, dtype=np.int64)
    for i in range(4096):
        val = np.log(i + 1.0)
        v = int(np.floor(val * 2048)) & 0xFFFF  # ap_fixed<16,5>, F=11
        if v >= 32768:
            v -= 65536
        log_lut[i] = v

    reinhard_lut = np.zeros(1024, dtype=np.int64)
    target_max = 4095.0
    log_target_max = np.log(target_max + 1.0)
    for i in range(1024):
        log_I_base = i / 123.0
        L = (log_I_base / log_target_max) * exposure_gain
        L_white = exposure_gain
        L_mapped = (L * (1.0 + (L / (L_white * L_white)))) / (1.0 + L)
        log_I_mapped_base = L_mapped * log_target_max
        v = int(np.floor(log_I_mapped_base * 2048)) & 0xFFFF
        if v >= 32768:
            v -= 65536
        reinhard_lut[i] = v

    exp_lut = np.zeros(1024, dtype=np.int64)
    for i in range(1024):
        log_I_final = i / 123.0
        I_final = np.exp(log_I_final) - 1.0
        if I_final > 4095.0:
            I_final = 4095.0
        if I_final < 0.0:
            I_final = 0.0
        exp_lut[i] = int(np.floor(I_final * 64)) & 0x3FFFF
    return log_lut, reinhard_lut, exp_lut


def _box_filter_sum_exact(img: np.ndarray) -> np.ndarray:
    padded = np.pad(img, ((3, 3), (3, 3)), mode="edge")
    row_sum = np.zeros_like(padded)
    for i in range(7):
        row_sum[:, 3:-3] += padded[:, i:padded.shape[1] - 6 + i]
    col_sum = np.zeros((img.shape[0], img.shape[1]), dtype=np.int64)
    for i in range(7):
        col_sum += row_sum[i:padded.shape[0] - 6 + i, 3:-3]
    return col_sum


def _cpp_div(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    return (np.abs(a) // np.abs(b)) * np.sign(a) * np.sign(b)


def _wrap_signed(v: np.ndarray, bits: int) -> np.ndarray:
    v = v & ((1 << bits) - 1)
    return np.where(v >= (1 << (bits - 1)), v - (1 << bits), v)


def ltm(img_in: np.ndarray, cfg: LtmConfig) -> np.ndarray:
    log_lut, reinhard_lut, exp_lut = _ltm_luts(float(cfg.exposure_gain))
    r_val, g_val, b_val = (img_in[:, :, c].astype(np.int64) for c in range(3))

    # Luminance with ap_ufixed<16,0> coefficients, result F=6.
    y_sum = ((r_val * 19595) >> 10) + ((g_val * 38469) >> 10) + ((b_val * 7471) >> 10) + 32
    y_int = np.clip(y_sum >> 6, 0, 4095)
    log_y = log_lut[y_int]  # F=11

    # Guided filter, pass 1.
    pixel_in_var = log_y >> 1  # F=10
    pixel_sq_in = _wrap_signed((pixel_in_var * pixel_in_var) >> 10, 18)
    mean_I = (_box_filter_sum_exact(log_y) * 1337) >> 16  # 1/49 as ap_ufixed<16,0>, F=11
    mean_II = (_box_filter_sum_exact(pixel_sq_in) * 1337) >> 16  # F=10
    mean_I_sq = _wrap_signed((mean_I * mean_I) >> 12, 18)
    var_I = np.maximum(mean_II - mean_I_sq, 0)

    eps_map = _wrap_signed(_cpp_div(512 << 20, var_I + 1) >> 10, 20)  # eps_base 0.5, eps1 0.001, F=10
    a_ext = _wrap_signed(_cpp_div(var_I << 20, var_I + eps_map) >> 10, 20)
    a_ext = np.minimum(a_ext, 614)  # a_max 0.6
    out_a_val = np.clip(a_ext << 1, 0, 4095)  # F=11
    out_b_val = _wrap_signed((mean_I - (((out_a_val * mean_I) >> 12) << 1)) >> 1, 15)  # F=10

    # Guided filter, pass 2.
    mean_a = np.clip((_box_filter_sum_exact(out_a_val) * 1337 + 32768) >> 16, 0, 4095)  # AP_RND, AP_SAT
    mean_b = (_box_filter_sum_exact(out_b_val) * 1337) >> 16

    # Base/detail split, global Reinhard curve on the base, detail recombination.
    log_I_base = (((mean_a >> 1) * (log_y >> 1)) >> 10) + mean_b  # F=10
    log_I_detail = _wrap_signed(log_y - (log_I_base << 1), 16)
    lut_idx = np.clip(((((log_I_base >> 2) * 31488) >> 8) + 128) >> 8, 0, 1023)
    log_I_mapped_base = reinhard_lut[lut_idx]  # F=11
    w = _wrap_signed(((3194 << 11) - log_I_mapped_base * 64) >> 11, 14)
    log_I_final = _wrap_signed(((log_I_mapped_base << 11) + w * log_I_detail) >> 11, 16)

    exp_lut_idx = np.clip(((((log_I_final << 1) * 503808) >> 12) + 2048) >> 12, 0, 1023)
    I_final = exp_lut[exp_lut_idx]  # F=6
    gain = (I_final // np.where(y_int == 0, 1, y_int)) << 6  # F=12

    out = [np.clip((ch * gain + 2048) >> 12, 0, 4095).astype(np.uint16) for ch in (r_val, g_val, b_val)]
    return np.stack(out, axis=-1)


# ============================================================================
# Gamma (Tuan Anh) - 4096-entry LUT, RGB12 -> RGB8
# ============================================================================

@lru_cache(maxsize=8)
def gamma_lut(gamma: float) -> np.ndarray:
    lut = np.zeros(4096, dtype=np.uint8)
    for i in range(4096):
        lut[i] = int(np.round(np.power(i / 4095.0, gamma) * 255.0))
    return lut


def gamma(img_in: np.ndarray, cfg: GammaConfig) -> np.ndarray:
    return gamma_lut(float(cfg.gamma))[np.clip(img_in, 0, 4095).astype(np.int64)]


# ============================================================================
# Pipeline
# ============================================================================

class IspGoldenModel:
    def __init__(self, cfg: IspConfig | None = None):
        self.cfg = cfg or IspConfig()
        self.cfg.validate()
        self.cnn = None if "cnn" in self.cfg.bypass else CnnDenoiseW8A8(self.cfg.cnn)

    def run(self, raw10: np.ndarray, verbose: bool = False) -> dict[str, np.ndarray]:
        """RAW10 [H,W] -> dict of every stage output; 'gamma' is the final RGB8 [H,W,3]."""
        if raw10.ndim != 2:
            raise ValueError(f"Expected a 2-D Bayer RAW frame, got shape {raw10.shape}")
        if raw10.min(initial=0) < 0 or raw10.max(initial=0) > RAW10_MAX:
            raise ValueError(f"Input must be RAW10 [0,{RAW10_MAX}]")
        cfg = self.cfg
        x = raw10.astype(np.uint16)
        outputs: dict[str, np.ndarray] = {}
        for stage in STAGES:
            t0 = time.perf_counter()
            if stage in cfg.bypass:
                outputs[stage] = x
                if verbose:
                    print(f"  {stage:9s} bypassed")
                continue
            if stage == "blc":
                x = blc(x, cfg.blc)
            elif stage == "bpc":
                x, outputs["bpc_detections"] = bpc(x, cfg.bpc)
            elif stage == "cnn":
                x = self.cnn(x)
            elif stage == "wb":
                x = wb(x, cfg.wb)
            elif stage == "demosaic":
                x = demosaic(x, cfg.demosaic)
            elif stage == "ccm":
                x = ccm(x, cfg.ccm)
            elif stage == "ltm":
                x = ltm(x, cfg.ltm)
            elif stage == "gamma":
                x = gamma(x, cfg.gamma)
            outputs[stage] = x
            if verbose:
                extra = f" detections={int(outputs['bpc_detections'].sum())}" if stage == "bpc" else ""
                print(f"  {stage:9s} {time.perf_counter() - t0:7.2f}s  shape={x.shape} "
                      f"range=[{int(x.min())},{int(x.max())}]{extra}")
        return outputs


# ============================================================================
# CLI
# ============================================================================

RAW_FORMATS = ("auto", "u16", "packed3x10")


def load_raw(path: Path, width: int, height: int, raw_format: str = "auto") -> np.ndarray:
    """.npy [H,W], or binary: 'u16' = one little-endian uint16 per pixel,
    'packed3x10' = 3 pixels per little-endian 32-bit word, pixel0 in bits 9:0 (Xilinx frame-buffer RAW10)."""
    if path.suffix.lower() == ".npy":
        raw = np.load(path, allow_pickle=False)
        if not np.issubdtype(raw.dtype, np.integer):
            raise TypeError(f"RAW .npy must be integer, got {raw.dtype}")
        return raw
    size = path.stat().st_size
    words_per_row = (width + 2) // 3
    sizes = {"u16": width * height * 2, "packed3x10": words_per_row * 4 * height}
    if raw_format == "auto":
        matches = [fmt for fmt, n in sizes.items() if n == size]
        if not matches:
            raise ValueError(f"{path}: {size} bytes matches no RAW10 layout for {width}x{height} "
                             f"(u16={sizes['u16']}, packed3x10={sizes['packed3x10']})")
        raw_format = matches[0]
    elif size != sizes[raw_format]:
        raise ValueError(f"{path}: {size} bytes, expected {sizes[raw_format]} for {raw_format} {width}x{height}")
    print(f"Reading {path.name} as {raw_format} RAW10 {width}x{height}")
    if raw_format == "u16":
        return np.fromfile(path, dtype="<u2").reshape(height, width)
    words = np.fromfile(path, dtype="<u4").reshape(height, words_per_row)
    pixels = np.stack([(words >> s) & 0x3FF for s in (0, 10, 20)], axis=-1).reshape(height, words_per_row * 3)
    return pixels[:, :width].astype(np.uint16)


def synthetic_raw10(width: int, height: int, seed: int) -> np.ndarray:
    """Smooth RGGB scene + Gaussian noise + sparse hot/dead pixels, sensor RAW10 with black level 64."""
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:height, 0:width] / np.array([height, width])[:, None, None]
    scene = 0.5 + 0.35 * np.sin(6.0 * xx + 2.0 * yy) * np.cos(4.0 * yy)
    gain = cfa_plane(height, width, 0.8, 1.0, 1.0, 0.6, np.float64)
    raw = 64 + scene * gain * 900 + rng.normal(0, 12, (height, width))
    defects = rng.random((height, width)) < 1e-4
    raw[defects] = rng.choice([0, 1023], defects.sum())
    return np.clip(np.round(raw), 0, RAW10_MAX).astype(np.uint16)


def write_png(path: Path, rgb8: np.ndarray) -> None:
    h, w, _ = rgb8.shape

    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    rows = np.concatenate([np.zeros((h, 1), np.uint8), np.ascontiguousarray(rgb8, np.uint8).reshape(h, w * 3)], axis=1)
    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(rows.tobytes(), 6))
                     + chunk(b"IEND", b""))


def main() -> None:
    parser = argparse.ArgumentParser(description="Integrated bit-accurate ISP golden model: "
                                                 "BLC -> BPC -> CNN -> WB -> Demosaic -> CCM -> LTM -> Gamma")
    src = parser.add_mutually_exclusive_group()
    src.add_argument("--input", type=Path, help="Sensor RAW10 RGGB frame: .npy [H,W] or binary (see --raw-format)")
    src.add_argument("--synthetic", action="store_true", help="Use a synthetic RAW10 frame instead of --input")
    parser.add_argument("--width", type=int, default=1920, help="Frame width (binary input / synthetic)")
    parser.add_argument("--height", type=int, default=1080, help="Frame height (binary input / synthetic)")
    parser.add_argument("--raw-format", choices=RAW_FORMATS, default="auto",
                        help="Binary layout: u16 (uint16 per pixel) or packed3x10 (3 pixels per 32-bit word); "
                             "auto picks by file size")
    parser.add_argument("--seed", type=int, default=0, help="Seed for --synthetic")
    parser.add_argument("--config", type=Path, help="JSON config (see --print-config)")
    parser.add_argument("--output-dir", type=Path, default=Path("output_image"), help="Where to write results")
    parser.add_argument("--dump-stages", action="store_true", help="Also save every stage output as .npy")
    parser.add_argument("--print-config", action="store_true", help="Print the default config as JSON and exit")
    args = parser.parse_args()

    if args.print_config:
        print(json.dumps(asdict(IspConfig()), indent=2))
        return
    if not args.input and not args.synthetic:
        parser.error("give --input FILE or --synthetic")

    cfg = IspConfig.from_json(args.config) if args.config else IspConfig()
    if args.synthetic:
        raw, name = synthetic_raw10(args.width, args.height, args.seed), "synthetic"
    else:
        raw, name = load_raw(args.input, args.width, args.height, args.raw_format), args.input.stem

    print(f"ISP golden model: input {raw.shape[1]}x{raw.shape[0]}, bypass={cfg.bypass or 'none'}")
    outputs = IspGoldenModel(cfg).run(raw, verbose=True)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    png_path = args.output_dir / f"{name}.png"
    write_png(png_path, outputs["gamma"])
    if args.dump_stages:
        stage_dir = args.output_dir / f"{name}_stages"
        stage_dir.mkdir(exist_ok=True)
        np.save(stage_dir / "00_input_raw10.npy", raw)
        for i, stage in enumerate(STAGES, start=1):
            np.save(stage_dir / f"{i:02d}_{stage}.npy", outputs[stage])
        np.save(stage_dir / "02_bpc_detections.npy", outputs.get("bpc_detections", np.zeros(raw.shape, bool)))
        print(f"Stage dumps in {stage_dir}")
    print(f"Wrote {png_path}")


if __name__ == "__main__":
    main()
