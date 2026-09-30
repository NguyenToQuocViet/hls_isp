<!--
Project: Adaptive Directional BPC and BLC
Module: BPC Handshake HLS Contract
Description: Define the accepted finite-frame BPC architecture, reusable engine boundary, storage lifecycle, and frame-tail drain.
Author: Viet Nguyen To Quoc
-->

# Contract BPC dùng ap_ctrl_hs

Trạng thái: **đã chốt làm contract triển khai**, ngày 2026-09-30. Việt duyệt toàn bộ bốn quyết định về transaction/drain, boundary, storage và variant layout. Turn chốt contract chỉ sửa tài liệu. Source none/handshake và standalone TOP đã được hiện thực theo contract trong turn tiếp theo; CSim, synthesis và CoSim của bản hs chưa được thực hiện.

## 1. Phạm vi và authority

Tài liệu này sở hữu hành vi ngoài thuật toán của BPC finite-frame: TOP/ENGINE boundary, configuration, frame admission, storage lifecycle, scheduling/drain, reset, backpressure và tiêu chí nghiệm thu. [ADR 0005](adr/0005-bpc-handshake-frame-drain.md) ghi rationale và trade-off đã chấp nhận.

- [bpc-adaptive-directional.md](bpc-adaptive-directional.md), mục 1–10, tiếp tục sở hữu thuật toán RAW10, same-CFA window, arithmetic, tie-break và border behavior. Giữ `bpc_pixel()`, `abs_diff()`, `BpcConfig` và `BpcWindow` hiện có.
- [blc_bpc_interface.md](blc_bpc_interface.md) sở hữu format chung. [BLC handshake contract](blc_handshake_contract.md) là mẫu boundary/configuration/DATAFLOW đã được kế thừa.
- [BPC streaming contract](bpc_streaming_contract.md) tiếp tục áp dụng riêng cho bản `ap_ctrl_none`; không bị thay thế hoặc chuyển thành frame transaction.
- Phạm vi implementation là chia source variant, BPC handshake engine và standalone TOP. Không làm TOP none, TOP tích hợp toàn ISP, các khối của thành viên khác hoặc thay thuật toán trong đợt này.
- Verification có các acceptance gates ở mục 9; việc chạy các gate phải theo phạm vi Việt giao ở turn triển khai, không tự gộp mọi gate vào implementation source.

## 2. Các quyết định đã chốt

| ID | Quyết định |
|---|---|
| BPC-HS-01 | Một start = một frame `N = W*H`; engine tự drain cuối frame rồi kết thúc |
| BPC-HS-02 | Engine thực hiện `N+D` storage advances, `D = 2*W+2`; đọc N pixel thật và thêm D synthetic zero |
| BPC-HS-03 | Không batch/frame_count, không nhận frame sau để đẩy tail frame trước; không yêu cầu zero-gap giữa transaction |
| BPC-HS-04 | TOP sở hữu AXIS, scalar AXI-Lite, `ap_ctrl_hs`, config snapshot và wiring |
| BPC-HS-05 | Ba process hữu hạn chạy overlap dưới DATAFLOW: ingress, `bpc_engine`, egress; plain packet FIFO ở giữa |
| BPC-HS-06 | Guard SOF tại ingress chung; engine nhận đúng N packet đã aligned và không đọc bỏ packet |
| BPC-HS-07 | Output ownership theo step; đúng N output, marker theo tọa độ output; incoming SOF/EOL không điều khiển engine |
| BPC-HS-08 | Bốn line-buffer bank và horizontal window giữ static; không clear RAM/window theo frame |
| BPC-HS-09 | Input/output counters, `lb_addr` và step khởi tạo lại mỗi invocation; không giữ control tiến độ frame cũ |
| BPC-HS-10 | Warmup và border bypass phải ngăn history cũ ảnh hưởng output; không per-pixel valid bits hoặc valid shift register |
| BPC-HS-11 | Blocking stream I/O; không `read_nb`, `hls::task`, DirectIO, publisher, `config_valid` hoặc startup token |
| BPC-HS-12 | Giữ bản hiện tại nguyên nội dung trong `none`; clone sang `handshake` và chỉ sửa những phần cần cho contract |

## 3. Source layout và mental model triển khai

```text
hls/bpc/
    none/
        isp_bpc.hpp, isp_bpc.cpp       ← hai file hiện tại, chuyển nguyên nội dung
    handshake/
        isp_bpc.hpp, isp_bpc.cpp       ← clone từ none, sửa control/lifecycle cần thiết
        bpc_top.hpp, bpc_top.cpp       ← standalone wrapper theo mẫu BLC hs
```

Layout này đã được tạo trong turn implement sau khi chốt contract. Mỗi build chọn đúng một variant; không link hai implementation cùng tên `bpc_engine`/`bpc_pixel`. Bản none chưa có TOP và không thêm TOP none trong đợt này.

BPC handshake là bản modify từ engine hiện tại, không phải viết lại từ source lịch sử. Giữ phần lớn code, logic, tên biến và comment; không thay pixel algorithm, tái tổ chức bank/window hoặc refactor không cần thiết. BPC cũ trong Git chỉ dùng làm căn cứ cho lifecycle hữu hạn: `f9fa1d6` có loop một frame `N+D`; `b55eb38` có worker đã tách khỏi TOP cùng thử nghiệm nhiều frame. Không mang batch/overlap của thử nghiệm đó vào bản này.

## 4. Hierarchy và interface

```text
CPU/controller — AXI-Lite: năm config fields + start/done
    │
bpc_top — ap_ctrl_hs, AXIS ports, config snapshot
    └── DATAFLOW: ba process overlap, chuyển từng pixel
        AXIS input
            │
        input_adapter: guard SOF, chuyển N packet
            │ FIFO<IspPixelPacket<10>>
        bpc_engine: N real + D synthetic advances → N output
            │ FIFO<IspPixelPacket<10>>
        output_adapter: chuyển N packet sang AXIS
            │
        AXIS output

ISP TOP — khi tích hợp sau này
    ingress chung → các engine → bpc_engine → các engine → egress chung
```

Interface engine giữ nguyên:

```cpp
void bpc_engine(
    hls::stream<IspPixelPacket<10>>& input,
    hls::stream<IspPixelPacket<10>>& output,
    const BpcConfig& config
);
```

Engine sở hữu vòng lặp hữu hạn, line buffers/window, storage address, tọa độ, warmup/drain, border bypass và gọi `bpc_pixel()`. Nó không nhận AXI packet, register address, DirectIO hoặc control của standalone IP. TOP chỉ tổ chức interface/config và nối function, không chứa storage/window hoặc pixel processing loop.

Giữ boundary các process bằng `INLINE off`; DATAFLOW tổ chức overlap, không chờ ingress nhận cả frame rồi mới xử lý. FIFO phải hữu hạn, một producer/một consumer; implementation hiện khai báo depth=2 cho hai pixel FIFO như BLC hs, không thêm whole-frame buffer. Khả năng tiến dưới stall và depth thực tế vẫn cần kiểm bằng synthesis/RTL. Các vòng xử lý hướng tới II=1; achieved II và khả năng tiến dưới stall phải được kiểm bằng synthesis/RTL.

## 5. Data, configuration và admission

Input là RGGB RAW10 sau BLC, `(0,0)` là R. Geometry compile-time dùng `isp_frame.hpp`, production mặc định 1920×1080; verification nhỏ 16×16 và 16×12. Giữ tiền đề frame đầy đủ, `N>D` và geometry có nội vùng 5×5; counter phải đủ bounds. Không thêm geometry runtime.

AXIS ngoài dùng `ap_axiu<16,1,0,0>`: RAW10 ở `TDATA[9:0]`, bit cao bằng 0, `keep=strb=0b11`. Ingress giữ `data,user,last` trong packet plain. Egress zero-extend RAW10 và tạo byte-enable cố định; marker lấy từ engine. Không quy định recovery cho byte-enable/payload ngoài contract.

TOP ánh xạ ba threshold 10 bit và hai shift 4 bit qua scalar AXI-Lite; `BpcConfig` giữ nguyên kiểu hiện có. Return/control dùng `ap_ctrl_hs` và được bundle vào AXI-Lite như BLC hs. Địa chỉ register lấy từ generated register map khi implement, không đoán offset.

1. Phần mềm đợi transaction trước hoàn tất, ghi đủ năm tham số và bảo đảm MMIO write completion/order trước start.
2. TOP tạo một `BpcConfig` snapshot cho transaction; engine phải nhận snapshot trước pixel đầu. Giữ config ổn định suốt frame và drain.
3. Có thể đổi config sau hoàn tất và trước start kế tiếp, không cần reset giữa các frame. Baseline không dựa vào auto-restart hoặc nhiều transaction chồng nhau.
4. Mỗi invocation ingress chờ SOF, đọc bỏ packet `user=0` trước SOF; không tính chúng vào N. Giữ và chuyển ngay packet SOF đầu như pixel `(0,0)`, rồi N−1 packet tiếp theo.
5. Engine không có SOF guard đọc bỏ; incoming SOF thừa/EOL sai không restart, không thay đổi tọa độ hoặc rút ngắn transaction. Output `user` chỉ ở `(0,0)`, output `last` ở cột W−1.

Ingress có thể đọc thêm packet trước SOF, nhưng chỉ forward N packet. Engine đọc N packet thật, không đọc SOF/pixel frame tiếp theo để hoàn tất frame hiện tại; egress đọc/ghi N output. Interface buffering có thể nhận beat sớm nhưng phải giữ thứ tự và backpressure khi đầy.

Không SOF hoặc thiếu pixel làm transaction tiếp tục chờ. Input gaps trong frame không tạo synthetic advance. Không timeout, dummy/preamble bắt buộc, pixel tự tạo để bù input thiếu hoặc recovery cho frame mất/chèn pixel.

## 6. Processing, output ownership và drain

`step` đánh số zero-based các storage advances của invocation: `0 .. N+D−1`.

| Phase | Step | Input engine | Output engine |
|---|---|---|---|
| Warmup | `0 .. D−1` | Blocking-read real pixel | Không phát output |
| Overlap | `D .. N−1` | Blocking-read real pixel | Phát center thật tiếp theo |
| Drain | `N .. N+D−1` | Không đọc input; dùng zero | Phát tail center thật tiếp theo |

Điều kiện đọc real là `step < N`; điều kiện center được phát là `step >= D`. Synthetic zero là token advance nội bộ, không phải AXIS beat và không tạo output riêng ngoài N center của frame. Mỗi advance có output phải bảo toàn kết quả dưới blocking write; không mất/lặp center hoặc cập nhật tọa độ sai khi stall.

Giữ sequence storage của engine hiện tại: read-old bank values trước write-new, shift bốn bank, shift/load `horizontal_window[3][5]`, dựng sparse `BpcWindow` từ cột 0/2/4. Last column vẫn là `{old_lb3, old_lb1, new_pixel}`, center là `horizontal_window[1][2]`.

Input coordinates chỉ tiến khi real pixel được commit; `lb_addr` tiến modulo W cho real và synthetic; output coordinates tiến một lần cho mỗi center thật được giữ bằng output write. Không dùng input coordinates thay output coordinates trong drain. Border hai hàng/cột ngoài cùng bypass center; nội vùng gọi `bpc_pixel()` với tọa độ output và config snapshot.

Ví dụ W=16, H=16, N=256, D=34: real input ở step 0..255; output index 0..255 ở step 34..289; step 256..289 dùng 34 synthetic zero. Không cần frame tiếp theo để tạo output cuối.

D là storage-advance delay, không phải latency clock cố định. Loop có N+D advances, không phải N+2D vì đọc input/tạo output đã overlap. Trong drain engine vẫn làm việc và tạo output, nhưng chưa đọc frame mới. Baseline chấp nhận khoảng nghỉ này cộng control/pipeline overhead. Ở geometry mặc định, D=3842, tương đương khoảng 0,185% N; đây là tỷ lệ bước lý tưởng, không phải số đo throughput/RTL.

## 7. Storage lifetime, reset và backpressure

- Bốn line-buffer bank `lb_0..lb_3` và `horizontal_window` giữ `static`; không clear toàn bộ RAM/window ở đầu frame hoặc thêm RAM reset để đạt correctness.
- Input/output row/col, `lb_addr` và step có tiến độ local mỗi invocation, bắt đầu từ 0. Storage có thể chứa history frame cũ; logic warmup/output ownership và border bypass phải bảo đảm mọi output thuộc frame hiện tại, mọi neighbor được dùng cho interior đã thuộc frame hiện tại.
- Không dựa vào RAM luôn zero sau power-up/reset để chứng minh correctness. Không thêm per-pixel valid metadata hoặc valid-bit shift register. Nếu stale history ảnh hưởng output trong kiểm chứng, dừng để báo evidence cho Việt thay vì tự đổi contract.
- Blocking read/write và finite FIFO bảo toàn data/marker. Output stall có thể cho phép operation tiến nếu pipeline/FIFO còn capacity; không giả định mọi register dừng ngay khi AXIS `TREADY=0`.
- Hardware reset hủy transaction dở và control/in-flight state. Các beat downstream đã nhận không thể thu hồi. Sau reset, phần mềm cấu hình/start lại và nguồn cung cấp frame mới tại SOF; local counters bắt đầu lại, không cần clear RAM để frame mới đúng.
- `done` là hoàn tất block transaction theo RTL HLS sinh; external output retirement đo tại `TVALID && TREADY`. Không dùng riêng done làm bằng chứng downstream đã nhận beat cuối.

Memory-port schedule và read-old/write-new ordering phải được kiểm khi synthesis/RTL. Không thêm `DEPENDENCE false` chỉ để đạt II nếu chưa chứng minh dependency cần bỏ là không có thật.

## 8. ISP reuse và giới hạn tích hợp

Caller ISP cấp một frame đã aligned, plain packet FIFO và config ổn định qua cả drain, rồi gọi cùng `bpc_engine`. Engine tự đọc N/tạo N và drain; không phụ thuộc standalone TOP, publisher hoặc runner để lặp từng pixel.

Ingress/egress chỉ ở boundary ngoài của ISP; không gọi standalone `bpc_top` hoặc đổi qua lại AXIS giữa engine. BPC không tự quyết định register map/control chung của pipeline chín khối. Khi caller dùng DATAFLOW, các process cần được khởi động để đọc/ghi overlap; không đợi upstream done rồi mới cho downstream đọc qua FIFO nhỏ.

## 9. Acceptance gates và handoff

Đây là yêu cầu kiểm chứng cho bản hs, chưa có gate nào được đánh dấu pass:

1. **Preservation:** bốn bank/window update và pixel algorithm khớp baseline; hai file chuyển vào none nguyên nội dung, diff handshake chỉ thuộc thay đổi cần cho contract.
2. **Control/storage trace:** chứng minh N real + D synthetic advances, D warmup, N output, tọa độ/marker và stale-history suppression. Trace advances không được gọi là clock trace.
3. **Independent-reference CSim:** kiểm geometry nhỏ và production phù hợp, count/data/order/SOF/EOL, packet trước SOF, marker sai trong frame và nguồn ngừng ngay sau N pixel. Cùng engine direct-call và standalone wrapper phải có cùng kết quả.
4. **Multi-transaction independence:** nhiều frame khác dữ liệu (bao gồm giá trị cực trị/defect), đổi config giữa transaction, bảo đảm RAM/window còn history không làm nhiễm frame mới. Không dùng chính `bpc_pixel()` HLS làm oracle.
5. **Synthesis:** xác nhận AXIS/AXI-Lite/hs, ba process DATAFLOW, finite FIFO, memory ports/dependencies, bounds và achieved II. Không coi yêu cầu pragma II=1 là kết quả.
6. **Generated-RTL CoSim/traffic:** input gaps, output backpressure, stop-after-N, transaction liên tiếp, config-before-start và ổn định qua drain; scoreboard theo accepted AXIS beats, xác nhận output cuối không cần frame kế tiếp. Ghi riêng inter-frame interval, done và external retirement.
7. **Reset:** reset khi đang warmup/processing/drain hoặc còn output in-flight, restart với frame/config khác; chứng minh output epoch mới không chứa data/state cũ mà không clear RAM. CSim không thay thế bằng chứng hardware reset.

Ghi kết quả thực tế với revision, geometry, tool/version, command và loại harness vào [verification-status.md](verification-status.md). Report cũ hoặc bản none/BLC pass không chứng minh BPC hs đã pass.

Việt đã giao implement source ở turn tiếp theo sau khi chốt contract. Dùng contract này và source none nguyên nội dung làm baseline; không tự chuyển sang batch/overlap, thay storage lifetime, thêm valid bits, đổi SOF/reset policy hoặc viết lại algorithm để né vấn đề tool. Mọi ngã rẽ làm đổi behavior phải đưa Việt quyết định trước.
