<!--
Project: Adaptive Directional BPC and BLC
Module: Handshake Test Contract
Description: Define shared CSim and CoSim tests for the BLC and BPC handshake variants.
Author: Viet Nguyen To Quoc
-->

# Contract test BLC/BPC handshake

Đã chốt ngày 2026-09-30; testbench/runner đã implement. Việt đã chạy CSim/CoSim;
các mode và evidence PASS đã kiểm tra được ghi trong `verification-status.md`.
Tài liệu này sở hữu
cách chạy và tiêu chí PASS/FAIL của bộ test random đầu tiên. Hành vi DUT theo
[BLC hs](blc_handshake_contract.md) và [BPC hs](bpc_handshake_contract.md);
kết quả thực tế thuộc [verification-status.md](verification-status.md).

## 1. Phạm vi và quyền chạy

- Test riêng `blc_top` và `bpc_top` trong `hls/*/handshake/`; không sửa bản none.
- **Codex không tự động chạy CSim, synthesis hoặc CoSim**, kể cả sau khi implement.
  Chỉ chạy khi Việt yêu cầu rõ ràng; Việt tự chạy script theo hướng dẫn bàn giao.
- Implementation được giao sau khi chốt contract. Cho phép kiểm tra tĩnh/whitespace;
  không coi đó là test chức năng đã pass.

## 2. Ba mode

Runner có một arg `--mode` với ba giá trị; lựa chọn block và stage CSim/CoSim
tách khỏi mode. Geometry được chọn khi build, không thêm geometry runtime vào IP.

| Mode | Geometry | Frame | Cách chạy |
|---|---|---:|---|
| `single` | 1920×1080 | 1 | Một process, một transaction đầy đủ |
| `boundary` | 16×16 | 3 | Cùng process, ba lần gọi TOP, không reset giữa frame |
| `multi` | 1920×1080 | Mặc định 5 | Một frame/process, chạy tuần tự; process trước thoát rồi mới chạy lượt sau |

- `boundary` kiểm frame boundary, config mới và stale history của BPC.
- `multi` tăng số mẫu random; cho phép `--frames N` với N nguyên dương.
  Mỗi lượt dùng seed khác; không xem đây là nhiều transaction không reset.
- CSim và CoSim giữ cùng semantics mode. Build 16×16 và full-size tách biệt;
  tái sử dụng synthesis khi source và toàn bộ cấu hình synthesis không đổi.

## 3. Stimulus, Golden và testbench chung

- Random đều pixel RAW10 `[0,1023]`; frame đầy đủ, row-major RGGB.
  BPC test độc lập trong miền RAW10 sau BLC, không cần TOP tích hợp BLC→BPC.
- Input AXIS hợp lệ: bit cao bằng 0, `keep=strb=0b11`, SOF tại pixel đầu,
  EOL tại cột cuối. Mỗi transaction chỉ cấp N pixel; BPC tự drain hết tail.
- Dữ liệu khác nhau giữa frame. Dùng các config hợp lệ, xác định trước và khác nhau
  giữa frame; ổn định trong transaction. Đợt đầu random pixel, không random toàn miền config.
- Sinh vector một lần từ seed được ghi lại, lưu input/config dưới `build/` hoặc
  `artifacts/`, rồi CSim/CoSim đọc lại cùng bộ. Không lấy seed từ thời gian hoặc
  `random_device` trong testbench replay.
- Mỗi block dùng cùng C++ testbench và checker cho hai stage; chỉ thay backend
  C++ DUT / RTL do Vitis sinh. Golden là `blc::blc_frame()` và
  `adaptive_bpc::bpc_frame()`, không dùng pixel function HLS làm oracle.
- Golden BPC đã được tham số hóa geometry để dùng 16×16,
  giữ nguyên thuật toán và mặc định 1920×1080. Golden và DUT phải cùng geometry.
- CoSim dùng comparator C/RTL của Vitis cùng checker Golden trong testbench;
  không chỉ dựa vào log CSim hoặc dòng PASS của giai đoạn tạo vector.

## 4. Checker và kết quả

- So bit-exact toàn bộ N pixel mỗi frame, theo thứ tự; kiểm `user`, `last`,
  `keep`, `strb`, zero-extension và không thiếu/thừa packet.
- Thống kê số pixel BLC clamp về 0, BPC sửa/giữ theo Golden để nhìn mức kích hoạt
  nhánh; không ép expected output hoặc điều chỉnh stimulus để che mismatch.
- Mỗi frame báo index, seed, config, số pixel đã kiểm và số mismatch. Chỉ in vài
  mismatch đầu kèm tọa độ/expected/actual; không dump toàn frame ra console.
- Runner kết luận `PASS` chỉ khi mọi frame dự kiến đã được kiểm đủ, checker không
  lỗi và stage/tool hoàn tất thành công. Với CoSim phải có kết quả RTL PASS.
- Mismatch, thiếu/thừa output, deadlock, timeout, hết RAM hoặc lỗi build/tool đều
  là `FAIL`; ghi rõ loại lỗi, không quy lỗi tool thành mismatch thuật toán.
  Exit code 0 chỉ cho PASS toàn run; mọi FAIL trả nonzero. Không có verdict là FAIL.
- Fail thì dừng ngay, giữ input/config/log để tái hiện; `multi` báo tiến độ
  `k/N` và chỉ PASS tổng khi đủ N lượt. Có timeout hữu hạn cho từng lượt.
- Log lưu revision, tool/version, geometry, mode, stage, config, seed, command,
  kết quả và peak RAM nếu đo được. Mỗi lượt có thư mục kết quả riêng, không ghi đè.

## 5. CoSim và bàn giao

- Chạy batch tuần tự, không GUI. CoSim chỉ bật waveform đầy đủ
  (`cosim.trace_level=all`) cho `boundary`; `single` và `multi` dùng
  `cosim.trace_level=none`. DATAFLOW profiling vẫn tắt ở cả ba mode.
  Không cam kết RAM đủ chỉ từ cấu hình này.
- `single`/`multi` kiểm full-size; `boundary` kiểm nhiều transaction ở geometry nhỏ.
  Bộ test này chưa chứng minh full-size nhiều transaction không reset, reset giữa
  frame, malformed markers/SOF guard, random backpressure hoặc toàn bộ ISP.
  Các acceptance gates tương ứng trong contract thiết kế vẫn còn nguyên.
- Sau implement, Codex phải hướng dẫn Việt **script thực tế**: chọn block/stage,
  ba mode, seed, `--frames`, nơi xem PASS/FAIL/log và cách chạy lại seed lỗi.
  Nêu rõ script nào tự chạy synthesis trước CoSim và khi nào tái sử dụng build.
  Hướng dẫn không đồng nghĩa Codex đã chạy; chưa chạy phải ghi rõ chưa xác minh.

## 6. Implementation và cách chạy

Runner: [`run_handshake_tests.py`](../scripts/run_handshake_tests.py).
Checker chung: [`test_handshake_hls.cpp`](../tests/test_handshake_hls.cpp);
`HS_TEST_BPC` chỉ chọn TOP/Golden của block khi compile. Golden BPC nhận thêm
`width,height` với mặc định 1920×1080; thuật toán không đổi.

Chạy từ repo root, sau khi nạp môi trường Vitis nếu terminal chưa có tool:

```sh
source /opt/Xilinx/2026.1/Vitis/settings64.sh

# Single: một full frame.
python3 Viet/scripts/run_handshake_tests.py --block blc --stage csim --mode single --seed 20260930

# Boundary: ba frame 16×16, cùng process, ba TOP calls, config khác nhau.
python3 Viet/scripts/run_handshake_tests.py --block bpc --stage csim --mode boundary --seed 20260930

# Multi: năm full frame, một frame/process, chạy tuần tự.
python3 Viet/scripts/run_handshake_tests.py --block bpc --stage cosim --mode multi --frames 5 --seed 20260930
```

- Đổi `--block blc|bpc`, `--stage csim|cosim` cho cả ba mode. Bỏ `--seed` thì dùng
  20260930; frame tiếp theo dùng seed + 1 (modulo 2^64).
- CSim không synthesis. CoSim **tự synthesis một lần đầu batch**, sau đó dùng lại
  cùng workspace synthesis cho từng process CoSim mới; không chạy nhiều simulator
  cùng lúc. Batch mới luôn synthesis lại, không dùng cache từ lệnh trước.
- Target theo script BLC hiện có: `xczu7ev-ffvc1156-2-e`, 150 MHz,
  clock uncertainty 10%. Đây là cấu hình build, không phải kết quả timing.
- Timeout mặc định 3600 giây/process test và 1800 giây/synthesis;
  chỉnh bằng `--timeout SECONDS --synth-timeout SECONDS` nếu cần. Timeout là FAIL.
- Config theo profile cố định, xoay vòng mỗi ba frame:

| Profile | BLC: R, Gr, Gb, B | BPC: threshold R, G, B; shift signal, gradient |
|---|---|---|
| 0 | 64, 66, 65, 68 | 16, 20, 24; 4, 3 |
| 1 | 32, 36, 40, 44 | 48, 56, 64; 3, 4 |
| 2 | 96, 100, 104, 108 | 96, 80, 112; 5, 2 |

Các profile chỉ phục vụ test equivalence/config transition; không phải calibration
hoặc kết quả tuning. Pixel random đều không phụ thuộc profile hay Golden output.

Console in đường dẫn kết quả ngay đầu run:
`Viet/build/handshake_tests/<block>/<mode>/<timestamp>_<stage>_<pid>/`.

- `vectors.json`, `inputs/*.raw10le`: input/config/seed/SHA-256 đã lưu;
  RAW10 dùng hai byte little-endian/pixel, không phải PGM hoặc packed RAW10.
- `result.json`: verdict tổng, số frame dự kiến/hoàn tất, failure kind,
  revision/Git status, hash source, thời gian và Python version.
- `version_*/console.log`: tool versions; `synthesis/`: lệnh/log synthesis nếu có.
- `run_0001/`, `run_0002/`, …: `manifest.txt`, `vectors.json`, `test.cfg`,
  `console.log`, `command.json`, `result.json` và `tool_results/` riêng từng lượt.
  Input paths trong manifest là absolute để hai giai đoạn CoSim cùng đọc được.
- `tool_results/logs/hls_run_<stage>.log`: checker stats/mismatch đầu tiên.
  CoSim còn giữ `tool_results/hls/sim/report/<block>_top_cosim.rpt`.
  Simulator artifacts/logs được chuyển vào lượt vừa chạy trước khi lượt sau bắt đầu;
  synthesis còn ở workspace chung `work/`.
- CoSim `boundary` tự lưu waveform đầy đủ trong
  `run_*/tool_results/hls/sim/verilog/`; không cần thêm arg. CoSim `single` và
  `multi` không lưu waveform. `cosim.wave_debug=false` giữ batch không GUI;
  DATAFLOW profiling tắt. CSim không sinh waveform RTL.
- `command.json` ghi exit/timeout và peak RSS KiB nếu `/usr/bin/time` có sẵn.
  RSS này là số đo do `time` báo cho tool command, không khẳng định tổng RAM mọi
  process đồng thời hoặc cam kết CoSim full-size vừa 32 GB.

`HS_CHECKER_PASS` trong log chỉ là checker Golden. Runner chỉ in **PASS tổng**
và exit 0 khi đủ frame; CoSim còn đòi checker sau `Starting C post checking`,
verdict `C/RTL co-simulation finished: PASS` và report Verilog `Pass`.
Mọi lỗi/dừng thiếu verdict trả nonzero; `FAIL` giữ vector và log, không tiếp tục
frame kế tiếp. Lỗi tool bị kill mà không có bằng chứng OOM được ghi lỗi tool/environment,
không tự kết luận thuật toán sai hoặc hết RAM.

Replay cùng vector/config giữa CSim và CoSim bằng đường dẫn runner đã in:

```sh
# Ví dụ: thay đường dẫn này bằng thư mục kết quả CSim thực tế.
python3 Viet/scripts/run_handshake_tests.py --block bpc --stage cosim --mode boundary \
    --replay 'Viet/build/handshake_tests/bpc/boundary/<run_csim>/vectors.json'

# Chạy lại đúng một frame lỗi của multi, giữ cả config (không chỉ seed).
python3 Viet/scripts/run_handshake_tests.py --block bpc --stage cosim --mode single \
    --replay 'Viet/build/handshake_tests/bpc/multi/<run_multi>/run_0003/vectors.json'
```

`--replay` không nhận thêm `--seed`; input được kiểm size/hash và copy vào batch mới.
Replay một batch multi tự lấy số frame đã lưu, hoặc dùng `--frames N` khớp số đó.
Giữ thư mục input gốc khi replay. Repo path cần không có whitespace để manifest/config
giữ định dạng đơn giản; runner báo lỗi nếu không đáp ứng.

Kết quả chức năng từ các lượt Việt chạy và việc kiểm tra artifact được ghi tại
[verification-status.md](verification-status.md#standalone-blcbpc-handshake-2026-09-30).
Chỉ kết luận theo mode đã có evidence; kiểm tra tĩnh không thay thế CSim/CoSim.

Tham chiếu cấu hình runner: AMD UG1399
[C-Simulation Configuration](https://docs.amd.com/r/en-US/ug1399-vitis-hls/C-Simulation-Configuration)
và [Co-Simulation Configuration](https://docs.amd.com/r/en-US/ug1399-vitis-hls/Co-Simulation-Configuration).
Các path log/report lấy từ output Vitis 2026.1 đang có trong workspace; runner sẽ
FAIL nếu tool không cung cấp đủ evidence tại các path đó.
