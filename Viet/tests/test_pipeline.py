#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: Loader Pipeline Tests
# Description: Generate controlled DNG fixtures and verify crop, RAW10 scaling, BLC and injection.
# Author: Viet Nguyen To Quoc

import csv
import json
from pathlib import Path
import struct
import subprocess
import tempfile

import numpy as np

ROOT = Path(__file__).resolve().parents[1]


def write_dng(path, black, delta=False):
    """Minimal uncompressed Bayer DNG fixture; all sample values are software-generated."""
    height, width = 1090, 1930
    rows, cols = np.indices((height, width))
    native = np.array(black).reshape(2, 2) if len(black) == 4 else np.full((2, 2), black[0])
    raw = (native[rows % 2, cols % 2] + (rows * 13 + cols * 7) % 8000).astype('<u2')
    raw[5:7, 5:7] = [[0, 200], [300, 16380]]
    tags = []

    def tag(number, kind, values):
        if kind == 2:
            encoded = values.encode() + b'\0'
            count = len(encoded)
        else:
            codes = {1: 'B', 3: 'H', 4: 'I', 5: 'I', 10: 'i'}
            encoded = struct.pack('<' + codes[kind] * len(values), *values)
            count = len(values) // 2 if kind in (5, 10) else len(values)
        tags.append((number, kind, count, encoded))

    tag(256, 4, [width]); tag(257, 4, [height]); tag(258, 3, [16])
    tag(259, 3, [1]); tag(262, 3, [32803]); tag(273, 4, [0])
    tag(274, 3, [1]); tag(277, 3, [1]); tag(278, 4, [height])
    tag(279, 4, [raw.nbytes]); tag(284, 3, [1])
    tag(33421, 3, [2, 2]); tag(33422, 1, [2, 1, 1, 0])
    tag(50706, 1, [1, 4, 0, 0]); tag(50707, 1, [1, 1, 0, 0])
    tag(50708, 2, 'Synthetic BLC fixture'); tag(50710, 1, [0, 1, 2]); tag(50711, 3, [1])
    tag(50713, 3, [2, 2] if len(black) == 4 else [1, 1])
    tag(50714, 5, [item for value in black for item in (int(value * 2), 2)])
    tag(50717, 4, [16380])
    tag(50721, 10, [item for value in (1, 0, 0, 0, 1, 0, 0, 0, 1) for item in (value, 1)])
    tag(50728, 5, [1, 1, 1, 1, 1, 1]); tag(50778, 3, [21])
    tag(50829, 4, [2, 2, height - 2, width - 2])
    if delta:
        tag(50716, 10, [1, 1])
    tags.sort()
    base = 8 + 2 + 12 * len(tags) + 4
    payload = bytearray()
    entries = []
    for number, kind, count, encoded in tags:
        if len(encoded) <= 4:
            value = encoded.ljust(4, b'\0')
        else:
            value = struct.pack('<I', base + len(payload))
            payload.extend(encoded)
            if len(payload) % 2:
                payload.append(0)
        entries.append((number, struct.pack('<HHI', number, kind, count), value))
    strip = base + len(payload)
    ifd = b''.join(header + (struct.pack('<I', strip) if number == 273 else value) for number, header, value in entries)
    path.write_bytes(b'II' + struct.pack('<HIH', 42, 8, len(tags)) + ifd + b'\0' * 4 + payload + raw.tobytes())
    return raw, native


def read_pgm(path):
    header = path.read_bytes().split(b'\n', 3)
    assert header[:3] == [b'P5', b'1920 1080', b'1023']
    return np.frombuffer(header[3], dtype='>u2').astype(np.int32).reshape(1080, 1920)


def main():
    with tempfile.TemporaryDirectory(prefix='blc-pipeline-') as temporary:
        directory = Path(temporary)
        for name, black in [('scalar', [212.5]), ('four_phase', [100.5, 200.5, 300.5, 400.5])]:
            source = directory / (name + '.dng')
            raw, native = write_dng(source, black)
            first = directory / name
            first.mkdir()
            command = [str(ROOT / 'build/defect_injector')]
            subprocess.run(command + ['--check-only', str(source)], cwd=first, check=True)
            assert not list(first.iterdir())
            subprocess.run(command + [str(source)], cwd=first, check=True)
            metadata = json.loads((first / 'input_metadata.json').read_text())
            assert metadata['bit_depth'] == 10 and metadata['pixel_max'] == 1023
            assert metadata['hot_offset_min'] == 64 and metadata['hot_offset_max'] == 512
            assert metadata['dead_gain_min'] == 0 and metadata['dead_gain_max'] == 0.75
            top, left = metadata['crop_top'], metadata['crop_left']
            native_phase = [native[(top + row - 2) % 2, (left + col - 2) % 2] for row in range(2) for col in range(2)]
            expected_black = np.floor(np.array(native_phase) * 1023 / 16380 + 0.5).astype(int).tolist()
            assert metadata['native_black_r_gr_gb_b'] == native_phase
            assert metadata['black_r_gr_gb_b'] == expected_black
            clean = read_pgm(first / 'clean_rggb.pgm')
            cropped = np.minimum(raw[top:top + 1080, left:left + 1920].astype(np.uint64), 16380)
            expected = ((2 * cropped * 1023 + 16380) // (2 * 16380)).astype(int)
            np.testing.assert_array_equal(clean, expected)
            corrupted = read_pgm(first / 'corrupted_rggb.pgm')
            rr, cc = np.indices(clean.shape)
            pedestal = np.array(expected_black).reshape(2, 2)[rr % 2, cc % 2]
            np.testing.assert_array_equal(read_pgm(first / 'reference_blc.pgm'), np.maximum(clean - pedestal, 0))
            np.testing.assert_array_equal(read_pgm(first / 'corrupted_blc.pgm'), np.maximum(corrupted - pedestal, 0))
            with (first / 'defects.csv').open() as file:
                defects = list(csv.DictReader(file))
            assert len(defects) == 300
            assert sum(row['type'] == 'hot' for row in defects) == 150
            assert sum(row['type'] == 'dead' for row in defects) == 150
            mask = np.zeros(clean.shape, dtype=bool)
            coordinates = []
            for defect in defects:
                x, y = int(defect['x']), int(defect['y'])
                assert 2 <= x < 1918 and 2 <= y < 1078
                assert all(max(abs(x - xx), abs(y - yy)) >= 10 for xx, yy in coordinates)
                coordinates.append((x, y))
                mask[y, x] = True
                assert int(defect['input_value']) == clean[y, x]
                assert int(defect['output_value']) == corrupted[y, x]
                expected_defect = min(1023, max(0, int(np.floor(float(defect['a']) * clean[y, x] + float(defect['b']) + 0.5))))
                assert corrupted[y, x] == expected_defect
            np.testing.assert_array_equal(clean[~mask], corrupted[~mask])
            second = directory / (name + '_repeat')
            second.mkdir()
            subprocess.run(command + [str(source)], cwd=second, check=True)
            assert all(file.read_bytes() == (second / file.name).read_bytes() for file in first.iterdir())
        rejected = directory / 'delta.dng'
        write_dng(rejected, [100], delta=True)
        result = subprocess.run([str(ROOT / 'build/defect_injector'), '--check-only', str(rejected)], cwd=directory, text=True, capture_output=True)
        assert result.returncode != 0 and 'BlackLevelDelta' in result.stderr
    print('Synthetic DNG scalar/four-phase, crop/scale, BLC, injection, repeatability and delta rejection: PASS')


if __name__ == '__main__':
    main()
