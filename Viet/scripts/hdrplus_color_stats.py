#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: HDR Plus Color Statistics
# Description: Extract accepted DNG color metadata and summarize WB and published CCM without selecting images.
# Author: Viet Nguyen To Quoc

import argparse
from collections import Counter, defaultdict
import csv
import json
import math
from pathlib import Path
import statistics
import subprocess

from hdrplus_raw10_filter import ROOT, download, sha256

TAGS = ['AsShotNeutral', 'ColorMatrix1', 'ColorMatrix2', 'CameraCalibration1',
        'CameraCalibration2', 'ForwardMatrix1', 'ForwardMatrix2',
        'CalibrationIlluminant1', 'CalibrationIlluminant2', 'AnalogBalance',
        'Make', 'Model', 'UniqueCameraModel', 'ExposureTime', 'ISO']


def vector(value, count):
    values = value if isinstance(value, list) else str(value).split()
    result = tuple(float(v) for v in values)
    if len(result) != count or not all(math.isfinite(v) for v in result):
        raise ValueError(f'Expected {count} finite numbers: {value}')
    return result


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2, allow_nan=False) + '\n')


def write_csv(path, rows, fields):
    with path.open('w', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for row in rows:
            writer.writerow({key: json.dumps(row[key]) if isinstance(row.get(key), (list, tuple, dict))
                             else row.get(key, '') for key in fields})


def distribution(values):
    return {'count': len(values), 'min': min(values), 'median': statistics.median(values),
            'max': max(values)} if values else {'count': 0}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--download', action='store_true', help='Download missing burst CCM sidecars')
    args = parser.parse_args()
    base = ROOT / 'artifacts/hdrplus_bl64'
    output = base / 'color_stats'
    output.mkdir(exist_ok=True)
    accepted = sorted((r for r in json.loads((base / 'manifest.json').read_text())
                       if r['status'] == 'accepted'), key=lambda r: r['source'])
    if not accepted:
        raise ValueError('No accepted source images')
    by_burst = defaultdict(list)
    for row in accepted:
        by_burst[row['burst']].append(row)
    sidecars = {}
    for index, (burst, rows) in enumerate(sorted(by_burst.items()), 1):
        url = rows[0]['url'].rsplit('/', 1)[0] + '/rgb2rgb.txt'
        path = output / 'sidecars' / burst / 'rgb2rgb.txt'
        if args.download:
            download(url, path)
        if not path.is_file():
            raise FileNotFoundError(f'Missing {path}; rerun with --download')
        matrix = vector(path.read_text(), 9)
        sidecars[burst] = {'url': url, 'path': str(path), 'sha256': sha256(path), 'ccm': matrix}
        print(f'CCM {index}/{len(by_burst)}: {burst}', flush=True)
    extraction = subprocess.run(['exiftool', '-j', '-n', *['-' + tag for tag in TAGS],
                                 *[r['source'] for r in accepted]],
                                check=True, capture_output=True, text=True)
    raw = json.loads(extraction.stdout)
    write_json(output / 'exiftool_metadata.json', raw)
    write_json(output / 'sidecar_manifest.json', sidecars)
    metadata = {str(Path(r['SourceFile']).resolve()): r for r in raw}
    if len(metadata) != len(accepted):
        raise ValueError('ExifTool source count mismatch')
    matrices = sorted({v['ccm'] for v in sidecars.values()})
    profiles = {v: f'C{index:02d}' for index, v in enumerate(matrices, 1)}
    records = []
    issues = []
    for source in accepted:
        tags = metadata[str(Path(source['source']).resolve())]
        row = {'id': Path(source['output']).name, 'burst': source['burst'],
               'frame': Path(source['source']).name, 'source': source['source'],
               'output': source['output'], 'source_sha256': source['source_sha256'],
               'ccm_profile': profiles[sidecars[source['burst']]['ccm']],
               'ccm': sidecars[source['burst']]['ccm'],
               'model_label': tags.get('Model') or tags.get('UniqueCameraModel') or 'unknown',
               **{tag: tags[tag] for tag in TAGS if tag in tags}}
        try:
            neutral = vector(tags.get('AsShotNeutral', ''), 3)
            if any(v <= 0 for v in neutral):
                raise ValueError('AsShotNeutral must be positive')
            row.update(wb_r_over_g=neutral[1] / neutral[0], wb_g=1.0,
                       wb_b_over_g=neutral[1] / neutral[2], wb_status='valid')
        except ValueError as error:
            row['wb_status'] = 'invalid_or_missing'
            issues.append({'id': row['id'], 'tag': 'AsShotNeutral', 'reason': str(error)})
        for tag in ['ColorMatrix1', 'ColorMatrix2', 'CameraCalibration1',
                    'CameraCalibration2', 'ForwardMatrix1', 'ForwardMatrix2', 'AnalogBalance']:
            if tag in tags:
                try:
                    vector(tags[tag], 3 if tag == 'AnalogBalance' else 9)
                except ValueError as error:
                    issues.append({'id': row['id'], 'tag': tag, 'reason': str(error)})
        records.append(row)
    burst_stats = []
    for burst in sorted(by_burst):
        frames = [r for r in records if r['burst'] == burst]
        valid = [r for r in frames if r['wb_status'] == 'valid']
        stats = {'burst': burst, 'frames': len(frames), 'valid_wb_frames': len(valid),
                 'ccm_profile': frames[0]['ccm_profile'],
                 'model_labels': sorted({r['model_label'] for r in frames})}
        for channel in ['r', 'b']:
            values = [r[f'wb_{channel}_over_g'] for r in valid]
            if values:
                stats.update({f'wb_{channel}_{key}': value for key, value in distribution(values).items()
                              if key != 'count'})
                stats[f'wb_{channel}_spread_pct'] = (max(values) / min(values) - 1) * 100
        stats['distinct_wb_pairs'] = len({(r['wb_r_over_g'], r['wb_b_over_g']) for r in valid})
        for tag in ['ExposureTime', 'ISO']:
            values = [float(r[tag]) for r in frames if tag in r and float(r[tag]) > 0]
            stats[tag + '_positive_count'] = len(values)
            stats[tag + '_distinct_positive_values'] = sorted(set(values))
        burst_stats.append(stats)
    profile_stats = []
    for matrix in matrices:
        members = [r for r in records if r['ccm_profile'] == profiles[matrix]]
        valid = [r for r in members if r['wb_status'] == 'valid']
        profile_stats.append({'profile': profiles[matrix], 'frames': len(members),
                              'bursts': len({r['burst'] for r in members}), 'ccm': matrix,
                              'models': dict(Counter(r['model_label'] for r in members)),
                              'wb_r': distribution([r['wb_r_over_g'] for r in valid]),
                              'wb_b': distribution([r['wb_b_over_g'] for r in valid])})
    valid = [r for r in records if r['wb_status'] == 'valid']
    summary = {
        'frames': len(records), 'bursts': len(by_burst), 'valid_wb_frames': len(valid),
        'distinct_wb_pairs': len({(r['wb_r_over_g'], r['wb_b_over_g']) for r in valid}),
        'ccm_profiles': len(matrices), 'ccm_grouping': 'Exact equality of all nine parsed numeric coefficients; no tolerance or normalization',
        'wb_formula': '(nG/nR, 1, nG/nB) from AsShotNeutral',
        'wb_frame_weighted': {c: distribution([r[f'wb_{c}_over_g'] for r in valid]) for c in ['r', 'b']},
        'wb_burst_weighted': {c: distribution([r[f'wb_{c}_median'] for r in burst_stats
                                             if f'wb_{c}_median' in r]) for c in ['r', 'b']},
        'models_frame_counts': dict(Counter(r['model_label'] for r in records)),
        'tag_coverage': {tag: {'present': sum(tag in r for r in records),
                             'distinct_present_values': len({json.dumps(r[tag], sort_keys=True)
                                                            for r in records if tag in r})} for tag in TAGS},
        'positive_exposure_frames': sum(float(r.get('ExposureTime', 0)) > 0 for r in records),
        'positive_iso_frames': sum(float(r.get('ISO', 0)) > 0 for r in records),
        'bursts_with_varying_wb': sum(r['distinct_wb_pairs'] > 1 for r in burst_stats),
        'max_within_burst_wb_spread_pct': {c: max((r.get(f'wb_{c}_spread_pct', 0) for r in burst_stats), default=0)
                                         for c in ['r', 'b']},
        'issues': issues, 'profiles': profile_stats,
        'selection_performed': False,
        'exiftool_version': subprocess.check_output(['exiftool', '-ver'], text=True).strip(),
        'manifest_sha256': sha256(base / 'manifest.json'),
        'source_documentation': 'https://hdrplusdata.org/dataset.html',
    }
    write_json(output / 'summary.json', summary)
    write_csv(output / 'frames.csv', records,
              ['id', 'burst', 'frame', 'source', 'output', 'source_sha256', 'model_label',
               'wb_status', 'wb_r_over_g', 'wb_g', 'wb_b_over_g', 'ccm_profile', 'ccm', *TAGS])
    write_csv(output / 'bursts.csv', burst_stats, list(dict.fromkeys(k for r in burst_stats for k in r)))
    write_csv(output / 'ccm_profiles.csv', profile_stats,
              ['profile', 'frames', 'bursts', 'models', 'ccm', 'wb_r', 'wb_b'])
    report = ['# HDR+ WB / CCM statistics', '',
              f'- Frames: {len(records)}; bursts: {len(by_burst)}; valid WB: {len(valid)}.',
              f'- Exact numeric CCM profiles: {len(matrices)}.',
              '- CCM: published rgb2rgb.txt, sensor RGB to linear sRGB, row-major, excluding WB.',
              '- WB: (nG/nR, 1, nG/nB). Statistics use valid tags only; no imputation.',
              '- Frame-weighted results count correlated frames; burst-weighted results count each burst median once.',
              '- Missing camera names remain unknown; zero exposure/ISO values are not treated as usable measurements.',
              '- ColorMatrix1/2 are retained as DNG calibration metadata, not substituted for rgb2rgb.txt.',
              '- No image subset, common WB, approximate CCM cluster, or tolerance sweep selected.', '',
              '| CCM | Frames | Bursts |', '|---|---:|---:|']
    report += [f'| {r["profile"]} | {r["frames"]} | {r["bursts"]} |' for r in profile_stats]
    report += ['', 'See summary.json for WB ranges, tag coverage, issues and provenance;',
               'frames.csv for all frames and bursts.csv for within-burst differences.', '',
               'Source: https://hdrplusdata.org/dataset.html']
    (output / 'report.md').write_text('\n'.join(report) + '\n')
    print(json.dumps({k: v for k, v in summary.items() if k != 'profiles'}, indent=2))


if __name__ == '__main__':
    main()
