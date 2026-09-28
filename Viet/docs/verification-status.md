<!--
Project: Adaptive Directional BPC and BLC
Module: Verification Status
Description: Summarize block-level and integrated HLS verification evidence for BLC and adaptive BPC.
Author: Viet Nguyen To Quoc
-->

# Tình hình Verification BLC và BPC

Các kết quả full-HD bên dưới là bằng chứng lịch sử của source đã lưu ở branch `archive/bpc-agent-hls-2026-09-26`. Branch rebuild hiện có standalone `blc_top` streaming; bằng chứng 16×16 cho bản mới được ghi riêng ở cuối tài liệu. Số liệu cũ không xác nhận contract streaming mới.

Verification được chia thành ba lớp: kiểm tra reference bằng expected value tính tay, dùng reference làm golden model cho HLS CSim, rồi dùng cùng testbench để kiểm tra RTL sinh ra bằng CoSim.

## Tổng quan

| IP | Reference | CSim | CoSim | Trạng thái |
|---|---:|---:|---:|---|
| BLC | 64 pass, 0 fail | 2.074.622 pass, 0 fail | 2.074.622 pass, 0 fail | Pass; sau đó chỉ inline type alias |
| BPC | 54 pass, 0 fail | 2.083.600 pass, 0 fail | 2.073.600 pass, 0 fail | Pass trên revision trước tối ưu critical path |

Ở lượt chạy được ghi nhận trước khi rebuild, reference test đã được build và chạy lại: BLC `64/64`, BPC `54/54`.

## BLC

### Reference

`tests/blc_reference_test.cpp` kiểm tra kết quả bằng expected value tính tay, không dùng chính công thức BLC để sinh expected:

- 8 cụm RGGB, gồm 4 góc frame và 4 vị trí cố định: 32 kiểm tra.
- Underflow/saturation về 0 cho R, Gr, Gb và B: 4 kiểm tra.
- Frame RGGB 5 × 5: 1 kiểm tra kích thước và 25 pixel output.
- Tái sử dụng output vector với frame 1 × 1: 2 kiểm tra.

Tổng cộng: **64 pass, 0 fail**.

### CSim

`tests/blc_hls_test.cpp` dùng `reference/blc.cpp` làm golden model:

- 22 directed pixel cases cho bốn CFA phase, biên Black Level và RAW10 endpoints.
- 1.000 random pixel cases, seed `20260914`, random input, row, col và bốn Black Level.
- Một frame `1920 × 1080`, tương đương 2.073.600 pixel, kiểm tra payload và AXI4-Stream sideband.

Tổng cộng: **2.074.622 pass, 0 fail**, `CSim done with 0 errors`.

### CoSim

CoSim chạy generated Verilog của `isp_blc_top` với một full-frame transaction. Toàn bộ 2.073.600 RTL output được so với reference, gồm data, `TUSER`, `TLAST`, `TKEEP` và `TSTRB`. Testbench còn chạy 1.022 pixel checks phía C nên summary chung là **2.074.622 pass, 0 fail**.

Kết quả: `C/RTL co-simulation finished: PASS`, thời gian khoảng **9 phút 37 giây**.

## Adaptive BPC

### Reference

`tests/bpc_reference_test.cpp` kiểm tra `bpc_pixel` và `bpc_frame` bằng directed expected values:

- Chọn hướng H/V/D1/D2, tie priority và prediction: 11 kiểm tra.
- Strict detection boundary `abs(X-P) > T`: 5 kiểm tra.
- Signal/activity shift, truncation và wide arithmetic: 8 kiểm tra.
- Threshold theo bốn CFA phase: 8 kiểm tra.
- Same-CFA neighborhood addressing: 2 kiểm tra.
- Hai-pixel border và bốn interior corners: 16 kiểm tra.
- Flat frame, isolated hot/dead pixels, border extremes và alias rejection: 4 kiểm tra.

Tổng cộng: **54 pass, 0 fail**.

### CSim

`tests/bpc_hls_test.cpp` dùng `reference/bpc_adaptive.cpp` làm golden model:

- 8.000 broad-random `bpc_pixel` cases.
- 2.000 constrained-random cases cho từng hướng, tie priority, strict threshold và RAW10/shift endpoints.
- Pixel seed: `20260915`.
- Một random frame `1920 × 1080`, seed `20260916`, config `(16,16,16,3,0)`.
- Full-frame test so output của `isp_bpc_top` với `bpc_frame`, đồng thời kiểm tra output count và AXI4-Stream sideband.

Tổng cộng: **2.083.600 pass, 0 fail**. Reference phát hiện 1.395.282 outlier trên random frame; đây là test stimulus, không phải defect-rate claim.

### CoSim

CoSim dùng mode `top`, vì vậy 10.000 pixel-level cases không bị đưa qua RTL simulator. Generated Verilog của `isp_bpc_top` chỉ chạy đúng một full-frame transaction và so **2.073.600 output pixels** với reference.

Kết quả: **2.073.600 pass, 0 fail**, `C/RTL co-simulation finished: PASS`, thời gian khoảng **9 phút 54 giây**.

## Tích hợp HLS BLC → BPC (2026-09-23)

Top ghép của bản HLS cũ là `isp_top` trong `hls/isp_top.hpp/.cpp`; config local khi đó là `build/isp_top_hls.cfg`. Bằng chứng bên dưới được tạo trước lần đổi tên, khi top còn là `isp_blc_bpc_top`. Tên, đường dẫn và hash trong bản ghi của lượt chạy đó được giữ nguyên để truy xuất đúng artifact lịch sử; chưa có CoSim cho top dưới tên mới.

Trong lượt chạy này, `tests/test_blc_bpc_hls.cpp` gọi duy nhất `isp_blc_bpc_top`. Stimulus là một frame RAW10 phẳng giá trị 100, Black Level theo `R/Gr/Gb/B = 20/30/40/50`, một hot và một dead pixel nội vùng, và một hot pixel ở biên. Test tính expected qua `blc::blc_frame` rồi `adaptive_bpc::bpc_frame`, kiểm tra thêm các giá trị tính tay, và so toàn bộ 2.073.600 output cùng `TUSER`, `TLAST`, `TKEEP`, `TSTRB`.

- Vitis 2026.1 CSim: **2.073.600 pixel, 0 fail**; `CSim done with 0 errors`.
- C synthesis trên `xczu7ev-ffvc1156-2-e`, clock 150 MHz: `isp_blc_bpc_top` được nhận là `dataflow`; ba stream nội bộ là FIFO 22-bit sâu 2; các loop xử lý pixel đều đạt II=1 trong báo cáo. Ước tính: 8 BRAM18K, 2.147 FF, 3.443 LUT, Fmax 168,95 MHz. Đây không phải timing/resource sau place-and-route.
- Verilog CoSim: **2.073.600 pixel, 0 fail**, `C/RTL co-simulation finished: PASS`; một full-frame transaction, tổng thời gian khoảng **10 phút 29 giây**.

Các lệnh dùng config local `build/isp_blc_bpc_hls.cfg` và cùng work directory `build/vitis_workspace/hls_blc_bpc`:

```sh
v++ -c --mode hls --config build/isp_blc_bpc_hls.cfg --work_dir build/vitis_workspace/hls_blc_bpc
vitis-run --mode hls --csim --config build/isp_blc_bpc_hls.cfg --work_dir build/vitis_workspace/hls_blc_bpc
vitis-run --mode hls --cosim --config build/isp_blc_bpc_hls.cfg --work_dir build/vitis_workspace/hls_blc_bpc
```

Raw evidence nằm ở `build/vitis_workspace/hls_blc_bpc/hls/syn/report/csynth.rpt`, `build/vitis_workspace/hls_blc_bpc/hls/csim/report/isp_blc_bpc_top_csim.log` và `build/vitis_workspace/hls_blc_bpc/hls/sim/report/isp_blc_bpc_top_cosim.rpt`. Lượt chạy dùng Git HEAD `f9fa1d602e59e2d5ce9e839b856a7ad51f86f9c7` với worktree chưa commit; SHA-256 của các source/test HLS liên quan:

```text
6608ddaaab24e5ce8c437e44b3553d5e5d24bfd61d38fc1af84dcc927d95b2a5  hls/isp_frame.hpp
e577d21ccf09921c26e9324db8159a1fc6ec732e192976bc356de38b9f355590  hls/isp_stream.hpp
0c52ac3859da5c9f909e50d9c4f5ec1583c254f7e5384ce4318b28bc68c187fc  hls/isp_blc_bpc.hpp
7dd9008c07662fceca1721735c378116085c78065ce708d2cb5348ec22f64f63  hls/isp_blc_bpc.cpp
baf92683d74a49e56dbc3c340e97bd9bb04fd590d507d60efa13f6d52f601917  hls/blc/isp_blc.hpp
f2a185721341cf29576bbf247509f2844d0f64dbd324ce8bd57ec7730a426077  hls/blc/isp_blc.cpp
f4fd8993c27d28de4793db8e139c0c693d2c3c5d5c6fb4907d798ad507e7bc05  hls/bpc/isp_bpc.hpp
53b876fd648d79239bed5d9f9b86d0c7bb8ef0b85814c01ef0076c98545849f0  hls/bpc/isp_bpc.cpp
302e9bd680f14458d3d9f23e8d3d25a6341f41c5f6b0cb3faa5646c70a22463b  tests/test_blc_bpc_hls.cpp
```

## Trạng thái hiện tại và giới hạn

Các CoSim riêng BLC/BPC ở trên là evidence lịch sử trước khi tách worker. Trên source hiện tại, host test BLC đạt **2.074.622 pass, 0 fail**; mode `top` của host test BPC đạt **2.073.600 pass, 0 fail**; cả hai top độc lập và top ghép đều C synthesis thành công. CoSim riêng từng top chưa được chạy lại sau refactor.

CoSim ghép xác nhận đúng một frame với stimulus đã nêu. Nó chưa kiểm tra random AXI backpressure/stall, nhiều frame liên tục, hoặc timing sau place-and-route. Một CoSim transaction không đo được transaction II; II=1 ở đây là kết quả loop trong C synthesis.

Sau khi đổi tên top ghép thành `isp_top` (2026-09-24), C synthesis và CSim Vitis 2026.1 đều pass với config local `build/isp_top_hls.cfg` và work directory `build/vitis_workspace/hls_isp_top`. CSim kiểm tra **2.073.600 pixel, 0 fail**. Hai lệnh đã chạy:

```sh
v++ -c --mode hls --config build/isp_top_hls.cfg --work_dir build/vitis_workspace/hls_isp_top
vitis-run --mode hls --csim --config build/isp_top_hls.cfg --work_dir build/vitis_workspace/hls_isp_top
```

Kết quả này xác nhận tên top, danh sách source và hành vi C++ sau đổi tên; CoSim của `isp_top` chưa được chạy.

## Standalone BLC streaming trên branch rebuild (2026-09-27–28)

Git HEAD `401d290` cùng worktree chưa commit. Chạy Vitis/Vivado 2026.1 trên `xczu7ev-ffvc1156-2-e`, clock constraint 150 MHz, frame 16×16. Lệnh tái lập CSim, synthesis, RTL timing test và CoSim có thứ tự ghi cấu hình là `rtk proxy bash Viet/scripts/run_blc_streaming_checks.sh` từ repository root. Generated reports và RTL ở `Viet/build/blc_streaming_gate/` (ignored).

SHA-256 của source/test tại lượt chạy:

```text
d9e83ebcdc805ba223afede47bb0bd33598b37340d4ea4877d71760bdefe213d  Viet/hls/blc/blc_top.cpp
4314d59190e367f974d411a9fe2f7cbd6f4d68bcc09bee8576cedfe970e50bb9  Viet/hls/blc/blc_top.hpp
c65b8b68320ef281ddcacd68c4221fa5d68c136fb1e6cffb4cba6892b4144b1f  Viet/hls/blc/isp_blc.cpp
0a15d9b0e06f9113bb75f55f554c93d540eea9ccefc48032d99fd1cf21b3b30e  Viet/hls/blc/isp_blc.hpp
6a81aa6d88229fcf3a04e5bb0d2a55a5c2f58a7392d3cc99c05617f9c7184938  Viet/tests/test_blc_two_frames_hls.cpp
417ebf786820442d321c485359ff8cbc6f0f171672deaa10157c7db94fd2bb01  Viet/tests/test_blc_streaming_rtl.sv
1fac34cc659d7d273997d6dfb4abf35c6d5ebe73bc9e4ebdf033a2163fd8dbb9  Viet/scripts/run_blc_streaming_checks.sh
302800e1f57eddc069b3b5f89eef3d351ad482c906c1e1b670d0647468bc1b81  Viet/scripts/patch_blc_cosim_order.py
```

- CSim: test hai frame không có pixel preamble, nguồn dừng sau đúng pixel 512; so với BLC reference độc lập và kiểm data, `user`, `last`, `keep`, `strb`: **512/512 đúng**.
- C synthesis: `blc_top` dùng `ap_ctrl_none`; bốn black level và `config_valid` là AXI-Lite. `config_valid` reset về 0 trong RTL; các state `published`, `started`, `configured`, `in_row`, `in_col` có nhánh reset. Config FIFO 40-bit và token FIFO 1-bit đều depth 2; ingress, runner và egress được schedule II=1, `style=flp`. Đây là kết quả scheduling, không phải timing sau place-and-route.
- XSIM trên chính Verilog vừa tổng hợp, dùng `tests/test_blc_streaming_rtl.sv`: **PASS**. Test ghi AXI-Lite từng giao dịch, đợi write response rồi mới ghi `config_valid`; SOF đã được đưa tới trước commit. Scoreboard kiểm 512 beat của hai frame liền nhau, exact count sau khi nguồn dừng, data/sideband, handshake `TVALID && TREADY`, gap input, stall output, và reset sau 23 pixel của frame dở rồi chạy lại một frame 256 pixel với cấu hình khác. Trong lượt luôn sẵn sàng, handshake cuối frame 0 và đầu frame 1 cách nhau đúng một chu kỳ ở cả input và output.
- HLS CoSim mặc định trên cùng source **FAIL**: 383/512 pixel sai ở post-check dù RTL tạo đủ 512 output. Sequence SV do Vitis sinh đặt năm AXI-Lite write thread trong cùng một `fork`, nên có thể ghi `config_valid` trước khi bốn black level hoàn tất.
- CoSim với harness đã sửa thứ tự **PASS**: `cosim.setup=true` tạo test vectors và sequence SV; `scripts/patch_blc_cosim_order.py` chỉ thêm điều kiện đợi bốn write sequence hoàn tất trước write `config_valid`; `sim.sh` chạy lại cùng generated RTL và C post-check báo **512/512 đúng, 0 failures**. Patch có marker/count guard để dừng nếu định dạng harness sinh ra thay đổi. DUT, test vectors và golden không bị sửa ở bước này.

Giới hạn: test reset sau khi 23 output đã drain, chưa kiểm reset khi còn beat in-flight; backpressure chỉ là mẫu stall xác định, không phải stress ngẫu nhiên dài. Synthesis vẫn cảnh báo `HLS 200-656` về khả năng deadlock của rewind pipeline dưới `ap_ctrl_none`; các traffic đã test chạy đúng nhưng chưa bao phủ mọi lịch stall. Kết quả BLC standalone không xác nhận timing hay hành vi của BPC hoặc TOP ISP.
