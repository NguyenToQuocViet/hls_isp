#!/usr/bin/env python3
# Project: Adaptive Directional BPC and BLC
# Module: Evaluator Directed Tests
# Description: Check metric tables against hand-computable restoration and nonzero off-target cases.
# Author: Viet Nguyen To Quoc

import json
from pathlib import Path
import subprocess
import sys
import tempfile

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from sweep_one_image import table_sums


def pgm(path, data):
    path.write_bytes(b'P5\n1920 1080\n1023\n' + data.astype('>u2').tobytes())


def main():
    executable = str(ROOT / 'build/bpc_evaluate')
    with tempfile.TemporaryDirectory(prefix='bpc-metrics-') as temporary:
        directory = Path(temporary)
        reference = np.full((1080, 1920), 250, dtype=np.uint16)
        for textured in (False, True):
            if textured:
                reference[10, 10] = 25
                reference[8, 10] = reference[12, 10] = 500
                reference[8, 8] = reference[12, 12] = 750
                reference[8, 12] = reference[12, 8] = 1000
            corrupted = reference.copy()
            corrupted[10, 8] = 1023
            corrupted[40, 40] = 0
            pgm(directory / 'reference_blc.pgm', reference)
            pgm(directory / 'corrupted_blc.pgm', corrupted)
            (directory / 'defects.csv').write_text('x,y,cfa,type,input_value,output_value,a,b\n8,10,R,hot,250,1023,1,773\n40,40,R,dead,250,0,0,0\n')
            metadata = json.loads(subprocess.check_output([executable, str(directory), '--bank', str(directory / 'bank.bin'), '266'], text=True, stderr=subprocess.DEVNULL))
            assert metadata['initial'] == [773, 250]
            bank = np.memmap(directory / 'bank.bin', mode='r', dtype=np.int64, shape=(11, 11, 4, 1024, 4))
            for config in [(0, 0, 0, 10, 10), (1023, 1023, 1023, 10, 10), (125, 10, 20, 4, 5), (224, 0, 0, 10, 10), (225, 0, 0, 10, 10)]:
                expected = table_sums(bank, config).tolist()
                actual = json.loads(subprocess.check_output([executable, str(directory), '--oracle', *map(str, config)], text=True))
                assert actual == expected, (config, actual, expected)
            corrected = table_sums(bank, (0, 0, 0, 10, 10)).tolist()
            unchanged = table_sums(bank, (1023, 1023, 1023, 10, 10)).tolist()
            assert unchanged == [773, 250, 0, 0]
            if not textured:
                assert corrected == [0, 0, 0, 0]
                assert metadata['baseline'] == [0, 250, 0, 0]
            else:
                # At (10,10), injection breaks the H tie and moves prediction 250 -> 500.
                assert corrected[2] > 0 and corrected[3] > 0
            del bank
    print('Hand-computable restoration, no-op, nonzero BCR/IOTCR and oracle equivalence: PASS')


if __name__ == '__main__':
    main()
