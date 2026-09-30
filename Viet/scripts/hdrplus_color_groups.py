#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: HDR Plus Color Group Reports
# Description: Report the largest two disjoint exact-CCM groups within stated WB tolerances.
# Author: Viet Nguyen To Quoc

import csv
import json
from collections import defaultdict

from hdrplus_color_stats import write_csv, write_json
from hdrplus_raw10_filter import ROOT, sha256


def describe(members):
    low = [min(r[c] for r in members) for c in ['r', 'b']]
    high = [max(r[c] for r in members) for c in ['r', 'b']]
    center = [(a + b) / 2 for a, b in zip(low, high)]
    deviations = [(b - a) / (b + a) for a, b in zip(low, high)]
    return {'frames': len(members), 'bursts': len({r['burst'] for r in members}),
            'ccm_profile': members[0]['ccm_profile'], 'ccm_row_major': json.loads(members[0]['ccm']),
            'wb_common_r_g_b': [center[0], 1.0, center[1]],
            'wb_r_range': [low[0], high[0]], 'wb_b_range': [low[1], high[1]],
            'max_relative_deviation_pct_r_b': [100 * d for d in deviations],
            'burst_ids': sorted({r['burst'] for r in members}),
            'frame_ids': sorted(r['id'] for r in members)}


def largest(rows, tolerance):
    profiles = defaultdict(list)
    for row in rows:
        profiles[row['ccm_profile']].append(row)
    ratio = (1 + tolerance) / (1 - tolerance)
    best, best_key = None, None
    for profile, members in sorted(profiles.items()):
        # Any feasible group's minima are observed coordinates. Expanding the
        # corresponding rectangle cannot decrease its membership count.
        for low_r in sorted({r['r'] for r in members}):
            for low_b in sorted({r['b'] for r in members}):
                group = [r for r in members if low_r <= r['r'] <= low_r * ratio
                         and low_b <= r['b'] <= low_b * ratio]
                if not group:
                    continue
                stats = describe(group)
                spread = stats['max_relative_deviation_pct_r_b']
                key = (-stats['frames'], -stats['bursts'], max(spread), sum(spread),
                       profile, tuple(stats['frame_ids']))
                if best_key is None or key < best_key:
                    best, best_key = group, key
    return best


def main():
    base = ROOT / 'artifacts/hdrplus_bl64/color_stats'
    with (base / 'frames.csv').open(newline='') as stream:
        rows = list(csv.DictReader(stream))
    if any(r['wb_status'] != 'valid' for r in rows):
        raise ValueError('Input contains invalid WB')
    for row in rows:
        row.update(r=float(row['wb_r_over_g']), b=float(row['wb_b_over_g']))
    for tolerance in [0.05, 0.10]:
        directory = base / f'groups_pm{int(tolerance * 100):02d}'
        directory.mkdir(exist_ok=True)
        remaining = rows.copy()
        groups = []
        for rank in [1, 2]:
            group = largest(remaining, tolerance)
            stats = describe(group)
            assert all(d <= tolerance * 100 + 1e-10
                       for d in stats['max_relative_deviation_pct_r_b'])
            assert len({r['ccm'] for r in group}) == 1
            stats['rank'] = rank
            groups.append(stats)
            common = stats['wb_common_r_g_b']
            for row in group:
                row['wb_r_deviation_pct'] = (row['r'] / common[0] - 1) * 100
                row['wb_b_deviation_pct'] = (row['b'] / common[2] - 1) * 100
            write_csv(directory / f'group_{rank}_frames.csv', group,
                      ['id', 'burst', 'frame', 'source', 'output', 'model_label', 'ccm_profile',
                       'wb_r_over_g', 'wb_g', 'wb_b_over_g',
                       'wb_r_deviation_pct', 'wb_b_deviation_pct'])
            ids = set(stats['frame_ids'])
            remaining = [r for r in remaining if r['id'] not in ids]
        assert not set(groups[0]['frame_ids']) & set(groups[1]['frame_ids'])
        report = {'tolerance_pm_pct': tolerance * 100, 'input_frames': len(rows),
                  'input_csv_sha256': sha256(base / 'frames.csv'),
                  'ccm_rule': 'All nine published rgb2rgb coefficients exactly equal',
                  'wb_rule': 'abs(gain/common_gain - 1) <= tolerance independently for R/G and B/G; G=1',
                  'wb_common_rule': 'Midpoint of minimum and maximum per channel; minimizes maximum relative deviation',
                  'ranking': 'Maximize frames, then bursts; minimize largest deviation, then sum; deterministic profile/id tie-break',
                  'second_group_rule': 'Largest group after removing group 1; groups do not overlap',
                  'method': 'Enumerate all rectangles anchored at observed WB minima with max/min <= (1+t)/(1-t)',
                  'scope': 'Metadata compatibility; no rendered-color validation or 32-frame sampling',
                  'groups': groups}
        write_json(directory / 'report.json', report)
        lines = [f'# WB ±{int(tolerance * 100)}% / exact CCM', '',
                 'Groups are disjoint. Group 2 is maximal after removing group 1.',
                 'CCM coefficients must be identical. R/G and B/G are each compared to their common gain.',
                 'A ±5% interval can span more than 5% between its endpoints; ±10% is interpreted likewise.',
                 'This is a metadata report, not a rendered-color quality claim or a 32-image selection.', '',
                 '| Group | Frames | Bursts | CCM | Common WB (R,G,B) | Max deviation R/B |',
                 '|---|---:|---:|---|---|---|']
        for g in groups:
            wb = ', '.join(f'{v:.6f}' for v in g['wb_common_r_g_b'])
            dev = ' / '.join(f'{v:.4f}%' for v in g['max_relative_deviation_pct_r_b'])
            lines.append(f'| {g["rank"]} | {g["frames"]} | {g["bursts"]} | {g["ccm_profile"]} | {wb} | {dev} |')
        for g in groups:
            lines.extend(['', f'## Group {g["rank"]} CCM', '', '```text'])
            matrix = g['ccm_row_major']
            lines.extend(' '.join(str(v) for v in matrix[i:i + 3]) for i in [0, 3, 6])
            lines.extend(['```', '', 'Bursts:', ''])
            lines.extend('- ' + burst for burst in g['burst_ids'])
        (directory / 'report.md').write_text('\n'.join(lines) + '\n')
        print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
