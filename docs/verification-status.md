<!--
Project: Adaptive Directional BPC and BLC
Module: Verification Status
Description: Summarize block-level and integrated HLS verification evidence for BLC and adaptive BPC.
Author: Viet Nguyen To Quoc
-->

# Tình hình Verification BLC và BPC

Verification được chia thành ba lớp: kiểm tra reference bằng expected value tính tay, dùng reference làm golden model cho HLS CSim, rồi dùng cùng testbench để kiểm tra RTL sinh ra bằng CoSim.

## Tổng quan

| IP | Reference | CSim | CoSim | Trạng thái |
|---|---:|---:|---:|---|
| BLC | 64 pass, 0 fail | 2.074.622 pass, 0 fail | 2.074.622 pass, 0 fail | Pass; sau đó chỉ inline type alias |
| BPC | 54 pass, 0 fail | 2.083.600 pass, 0 fail | 2.073.600 pass, 0 fail | Pass trên revision trước tối ưu critical path |

Reference test đã được build và chạy lại từ source hiện tại: BLC `64/64`, BPC `54/54`.

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

Top ghép hiện tại là `isp_top` trong `hls/isp_top.hpp/.cpp`; config local hiện tại là `build/isp_top_hls.cfg`. Bằng chứng bên dưới được tạo trước lần đổi tên, khi top còn là `isp_blc_bpc_top`. Tên, đường dẫn và hash trong bản ghi của lượt chạy đó được giữ nguyên để truy xuất đúng artifact lịch sử; chưa có CoSim cho top dưới tên mới.

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
