#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: HDR Plus Curated Scanner
# Description: Discover curated DNG frames and retain exact RAW10 BL64 exports with resumable evidence.
# Author: Viet Nguyen To Quoc

import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
import urllib.parse
import urllib.request

from hdrplus_raw10_filter import ROOT, download, sha256, verify_pgm

PREFIX = '20171106_subset/bursts/'
TAGS = ['WhiteLevel', 'BlackLevel', 'BlackLevelDeltaH', 'BlackLevelDeltaV',
        'Orientation', 'ImageWidth', 'ImageHeight', 'CFARepeatPatternDim',
        'CFAPattern', 'BitsPerSample', 'Make', 'Model']


def save(path, value):
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(value, indent=2) + '\n')
    temporary.replace(path)


def request(url, headers=None):
    for attempt in range(3):
        try:
            return urllib.request.urlopen(urllib.request.Request(url, headers=headers or {}), timeout=60)
        except OSError:
            if attempt == 2:
                raise
            time.sleep(1 + attempt)


def list_frames(path):
    if path.exists():
        return json.loads(path.read_text())
    frames = []
    token = None
    while True:
        query = {'prefix': PREFIX, 'maxResults': 1000,
                 'fields': 'items(name,size),nextPageToken'}
        if token:
            query['pageToken'] = token
        with request('https://storage.googleapis.com/storage/v1/b/hdrplusdata/o?' +
                     urllib.parse.urlencode(query)) as response:
            data = json.load(response)
        frames.extend(row for row in data.get('items', [])
                      if re.fullmatch(PREFIX + r'[^/]+/payload_N\d{3}\.dng', row['name']))
        token = data.get('nextPageToken')
        if not token:
            break
    frames.sort(key=lambda row: (row['name'].split('/')[-1], row['name']))
    save(path, frames)
    return frames


def numbers(value):
    if isinstance(value, (int, float)):
        return [float(value)]
    if isinstance(value, list):
        return [float(item) for item in value]
    return [float(item) for item in str(value).split()]


def metadata(url):
    with request(url, {'Range': 'bytes=0-65535'}) as response:
        if response.status != 206:
            raise ValueError('Server did not honor bounded HTTP Range')
        header = response.read(65536)
    result = subprocess.run(['exiftool', '-j', '-n', *['-' + tag for tag in TAGS], '-'],
                            input=header, capture_output=True)
    rows = json.loads(result.stdout)
    if not rows:
        raise ValueError('No metadata in DNG prefix')
    return rows[0]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target', type=int, default=200)
    args = parser.parse_args()
    if args.target < 1:
        parser.error('--target must be positive')
    base = ROOT / 'artifacts/hdrplus_bl64'
    manifest_path = base / 'manifest.json'
    rows = json.loads(manifest_path.read_text()) if manifest_path.exists() else []
    known = {row['url']: row for row in rows}
    ledger_path = base / 'curated_scan.json'
    ledger = json.loads(ledger_path.read_text()) if ledger_path.exists() else []
    scanned = {row['url']: row for row in ledger}
    frames = list_frames(base / 'curated_objects.json')
    accepted = sum(row['status'] == 'accepted' for row in rows)
    print(f'Listed {len(frames)} frames in {len({row["name"].split("/")[-2] for row in frames})} bursts; existing accepted={accepted}', flush=True)
    for index, item in enumerate(frames, 1):
        if accepted >= args.target:
            break
        url = 'https://storage.googleapis.com/hdrplusdata/' + item['name']
        if url in known or (url in scanned and scanned[url]['status'] != 'error'):
            continue
        burst, frame = item['name'].split('/')[-2:]
        identifier = burst if frame == 'payload_N000.dng' else burst + '__' + Path(frame).stem
        record = {'url': url, 'burst': burst, 'frame': frame, 'id': identifier}
        source = base / 'source' / burst / frame
        target = base / 'clean' / identifier
        try:
            tags = metadata(url)
            record['prefilter_metadata'] = tags
            if (not numbers(tags.get('WhiteLevel', '')) or
                    any(value != 1023 for value in numbers(tags.get('WhiteLevel', ''))) or
                    not numbers(tags.get('BlackLevel', '')) or
                    any(value != 64 for value in numbers(tags.get('BlackLevel', ''))) or
                    'BlackLevelDeltaH' in tags or 'BlackLevelDeltaV' in tags):
                record.update(status='prefilter_rejected', reason='Native white/black metadata does not match exact RAW10 BL64 profile')
            elif 'Orientation' in tags and tags['Orientation'] != 1:
                record.update(status='prefilter_rejected', reason='Unsupported orientation')
            else:
                if shutil.disk_usage(base).free < int(item['size']) + 2 * 1024**3:
                    raise RuntimeError('Less than 2 GiB reserve; stop before downloading')
                download(url, source)
                if source.stat().st_size != int(item['size']):
                    raise ValueError('Source size differs from bucket listing')
                record.update(source=str(source), source_sha256=sha256(source))
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.exists():
                    raise RuntimeError(f'Unrecorded output already exists: {target}')
                with tempfile.TemporaryDirectory(prefix='.scan-', dir=target.parent) as directory:
                    temporary = Path(directory)
                    result = subprocess.run([str(ROOT / 'build/defect_injector'),
                                             '--export-clean-raw10-bl64', str(source)],
                                            cwd=temporary, text=True, capture_output=True)
                    if result.returncode:
                        record.update(status='rejected', reason=result.stderr.strip() or result.stdout.strip())
                    else:
                        output_metadata = json.loads((temporary / 'input_metadata.json').read_text())
                        verify_pgm(temporary / 'clean_rggb.pgm')
                        if (output_metadata['width'], output_metadata['height'], output_metadata['cfa'],
                                output_metadata['bit_depth'], output_metadata['pixel_max'],
                                output_metadata['white_level'], output_metadata['native_black_r_gr_gb_b']) != (
                                1920, 1080, 'RGGB', 10, 1023, 1023, [64, 64, 64, 64]):
                            raise ValueError('Export violates contract')
                        # Move individual outputs; TemporaryDirectory retains ownership of its folder.
                        target.mkdir()
                        for path in temporary.iterdir():
                            path.rename(target / path.name)
                        record.update(status='accepted', output=str(target),
                                      output_sha256=sha256(target / 'clean_rggb.pgm'))
                        accepted += 1
                rows.append(record.copy())
                save(manifest_path, rows)
                known[url] = record
                if record['status'] == 'rejected':
                    source.unlink()
                    record['source_deleted'] = True
            if record['status'] == 'prefilter_rejected':
                print(f'[{index}/{len(frames)}] {identifier}: prefilter rejected; accepted={accepted}', flush=True)
            else:
                print(f'[{index}/{len(frames)}] {identifier}: {record["status"]}; accepted={accepted}', flush=True)
        except (OSError, ValueError, KeyError) as error:
            record.update(status='error', reason=str(error))
            print(f'[{index}/{len(frames)}] {identifier}: error {error}', flush=True)
            if source.exists() and url not in known:
                source.unlink()
            partial = source.with_suffix('.part')
            if partial.exists():
                partial.unlink()
        scanned[url] = record
        save(ledger_path, list(scanned.values()))
    print(f'Finished: accepted={accepted}, target={args.target}, scanned={len(scanned)}; {manifest_path}', flush=True)


if __name__ == '__main__':
    main()
