<!--
Project: Adaptive Directional BPC and BLC
Module: BLC Handshake HLS Contract
Description: Define the accepted finite-frame BLC contract with reusable engine and wrapper-owned AXI interfaces.
Author: Viet Nguyen To Quoc
-->

# Contract BLC dùng ap_ctrl_hs

Trạng thái: **đã chốt làm căn cứ implement**, ngày 2026-09-30. Source handshake đã được clone/sửa từ bản none theo contract; CSim, CoSim và synthesis chưa chạy cho bản này. Tài liệu mô tả hành vi yêu cầu, không phải bằng chứng correctness hoặc throughput.

## 1. Phạm vi và authority

Tài liệu này sở hữu TOP/ENGINE boundary, transaction, configuration, frame admission và concurrency của bản BLC finite-frame. [ADR 0004](adr/0004-blc-handshake-boundary.md) ghi lý do chọn kiến trúc.

- Giữ tên `blc_engine`; giữ thuật toán `blc_pixel()` và `BlcConfig` hiện có.
- [Interface chung](blc_bpc_interface.md) sở hữu format RAW10/AXIS/CFA. Phần BLC free-running trong đó tiếp tục áp dụng cho bản `ap_ctrl_none`, không bị contract này thay thế.
- [BPC streaming contract](bpc_streaming_contract.md) giữ nguyên authority và phạm vi; không chuyển BPC sang finite-frame trong quyết định này.
- Source BLC chia thành `hls/blc/none/` và `hls/blc/handshake/`, mỗi thư mục giữ bộ `blc_top.{hpp,cpp}` và `isp_blc.{hpp,cpp}`. Chỉ chọn một variant trong mỗi build; không link hai implementation của `blc_top`/`blc_engine` cùng nhau.
- Bản `none` giữ nguyên nội dung source trước khi chia thư mục. Bản `handshake` clone từ `none`, giữ code/logic/comment và chỉ sửa những phần cần cho contract này; không viết lại pixel algorithm hoặc refactor không cần thiết.
- Việt đã giao implement source handshake và chuyển đường dẫn của bản none. CSim và CoSim không thuộc turn triển khai này; các điều kiện nghiệm thu bên dưới vẫn chưa được xác minh.

Implementation khởi đầu dùng hai FIFO pixel depth 2, ba process giữ boundary bằng `INLINE off`, các vòng lặp yêu cầu `PIPELINE II=1 style=flp`. Đây là lựa chọn triển khai cần kiểm bằng synthesis/RTL, không phải bằng chứng depth đủ hoặc achieved II=1.

## 2. Quyết định đã chốt

| ID | Quyết định |
|---|---|
| BLC-HS-01 | Một transaction start xử lý một frame `N = W × H`, rồi hoàn tất; không tự lặp vô hạn |
| BLC-HS-02 | TOP sở hữu AXIS, AXI-Lite, `ap_ctrl_hs`, configuration snapshot và wiring |
| BLC-HS-03 | Ba process hữu hạn: `input_adapter`, `blc_engine`, `output_adapter`, chạy overlap dưới `DATAFLOW` |
| BLC-HS-04 | Boundary giữa process là FIFO `hls::stream<IspPixelPacket<10>>`; chuyển từng pixel, không buffer cả frame |
| BLC-HS-05 | Guard SOF đặt tại ingress chung; giữ và chuyển chính packet SOF đầu tiên |
| BLC-HS-06 | Sau SOF, hình học quyết định bằng counter; không dùng incoming EOL để điều khiển tọa độ |
| BLC-HS-07 | Engine đọc đúng N packet, ghi đúng N packet, tạo SOF/EOL theo tọa độ nội bộ |
| BLC-HS-08 | Config chốt một lần mỗi transaction và ổn định suốt transaction; không publisher/config FIFO/startup token |
| BLC-HS-09 | Stream I/O blocking; không dùng `hls::task`, DirectIO hoặc non-blocking API trong bản này |
| BLC-HS-10 | Standalone và ISP dùng cùng engine; AXI adapters chỉ ở boundary ngoài của ISP |

## 3. Hierarchy và ownership

```text
CPU/controller
    │ AXI-Lite: black levels + start/done control
    ▼
blc_top — standalone wrapper, ap_ctrl_hs
    ├── Interface/config: AXIS ports, AXI-Lite registers, transaction snapshot
    └── DATAFLOW region — ba process chạy overlap
        AXIS input
            │
        input_adapter: guard SOF + AXIS sang packet, forward N
            │ FIFO<IspPixelPacket<10>>
        blc_engine: loop N pixel, local row/col, blc_pixel, tạo SOF/EOL
            │ FIFO<IspPixelPacket<10>>
        output_adapter: packet sang AXIS, forward N
            │
        AXIS output

ISP TOP — khi tích hợp
    ingress chung → packet FIFO → blc_engine → các engine tiếp theo → egress chung
```

Các thao tác loop/counter/`blc_pixel`/marker bên trong engine thuộc cùng process; không phải các task độc lập. TOP chỉ tổ chức interface, config và kết nối các function; không chứa pixel algorithm hoặc vòng lặp xử lý pixel riêng.

Interface engine:

```cpp
void blc_engine(
    hls::stream<IspPixelPacket<10>>& input,
    hls::stream<IspPixelPacket<10>>& output,
    const BlcConfig& config
);
```

Engine không nhận AXI packet, DirectIO, `config_valid`, start/done hoặc địa chỉ register. Không đặt pragma AXI/interface của standalone IP trong engine. Engine có vòng lặp hữu hạn của chính nó; gọi trực tiếp từ ISP vẫn xử lý đủ một frame, không phụ thuộc standalone TOP để lặp từng pixel.

## 4. Concurrency và blocking

`DATAFLOW` phải tổ chức ba function thành các process có thể chạy đồng thời. Dòng gọi function trong C++ không có nghĩa RTL phải hoàn tất ingress trước khi bắt đầu engine.

- Ingress ghi mỗi packet ngay khi có pixel hợp lệ; engine có packet thì xử lý; egress có kết quả thì chuyển ra AXIS.
- “Forward N” là điều kiện kết thúc process, không phải yêu cầu gom N pixel trước khi chuyển tiếp.
- FIFO rỗng làm reader chờ; FIFO đầy làm writer chờ. Các process còn lại tiếp tục nếu dependency của chúng cho phép.
- Không thêm bộ nhớ whole-frame hoặc giả định FIFO vô hạn để làm cho ba function tuần tự chạy được.
- Mỗi FIFO có một producer và một consumer. Depth phải được khai báo/kiểm tra khi implement; không chốt depth hoặc II chỉ từ sơ đồ này.

Mục tiêu xử lý một pixel mỗi cycle khi đủ input và output không stall; achieved II, latency và khoảng nghỉ giữa transaction phải được đo bằng tool/RTL, không mặc định là 3–4 cycle. Zero-gap giữa hai transaction không phải yêu cầu của bản này.

## 5. Frame admission, pixel đầu và EOL

Geometry compile-time lấy từ `isp_frame.hpp`: mặc định `W=1920`, `H=1080`; row-major RGGB, `(0,0)` là R. Không thêm width/height runtime.

1. Mỗi transaction, ingress bắt đầu ở trạng thái chờ SOF. Các packet có `user=0` trước SOF bị đọc bỏ và không tính vào N.
2. Packet đầu có `user=1` được giữ, chuyển ngay vào engine và tính là pixel số 0 tại `(0,0)`. Không đọc thêm để thay thế packet này.
3. Ingress chuyển tiếp N−1 packet kế tiếp, đúng thứ tự, rồi kết thúc. Không cần SOF của frame tiếp theo để hoàn tất.
4. Engine bắt đầu tọa độ local `(0,0)`, xử lý đúng N packet. Không có guard đọc bỏ SOF trong engine; caller chịu trách nhiệm cấp frame đã aligned.
5. Mỗi output dùng `blc_pixel(input.data, row, col, config)`. Output `user=1` chỉ tại `(0,0)`; output `last=1` tại `col=W−1` mỗi dòng.
6. Incoming `last` và SOF xuất hiện thêm trong frame không đổi counter, không restart và không rút ngắn transaction. Không lưu cờ lỗi sideband.

Invariant: ingress có thể đọc thêm packet rác trước SOF, nhưng chỉ ghi N packet nội bộ; engine đọc/ghi N; egress đọc/ghi N. Guard trong engine mà không bù input có thể làm mất output và khiến egress chờ, nên không được thêm guard đó vào bản finite-frame này.

Contract yêu cầu đủ N pixel liên tiếp của frame đã nhận. Không có SOF hoặc thiếu pixel thì transaction tiếp tục chờ; không timeout hoặc tự tạo pixel. Không phục hồi frame bị chèn/mất pixel giữa frame. Sai EOL không làm đổi hình học; output marker vẫn được tạo theo geometry cố định.

## 6. Configuration và block control

TOP dùng scalar black-level arguments ánh xạ AXI-Lite; control `ap_ctrl_hs` được ánh xạ vào cùng interface AXI-Lite qua return/control. AXIS input/output thuộc TOP. Địa chỉ register lấy từ generated register map khi implement, không đoán offset.

1. Phần mềm đợi transaction trước hoàn tất, ghi đủ bốn black levels và bảo đảm MMIO write completion/order trước khi start.
2. Mỗi transaction TOP tạo một `BlcConfig` snapshot từ các tham số đã ổn định. Snapshot phải sẵn sàng trước khi engine xử lý pixel đầu và giữ nguyên đến hết transaction.
3. Phần mềm giữ các tham số ổn định trong transaction. Có thể đổi config sau hoàn tất và trước start tiếp theo; không yêu cầu reset giữa các frame.
4. Không `config_valid`, `publish_config`, config channel hoặc `ingress_start` token. Không dùng dummy input hoặc cycle delay để bảo đảm config ordering.
5. Baseline dùng start theo từng frame, không dựa vào auto-restart hoặc nhiều transaction đang chồng nhau. `DATAFLOW` overlap giữa process trong một frame vẫn được giữ.

`done` là hoàn tất transaction theo block-control do HLS sinh. Không dùng riêng `done` để khẳng định downstream đã nhận beat cuối: external retirement được đo tại `TVALID && TREADY`, có tính đến interface buffering thực tế.

## 7. AXIS, reset và ISP reuse

- AXIS ngoài dùng `ap_axiu<16,1,0,0>`; payload `TDATA[9:0]`. Input hợp lệ có bit cao bằng 0, `keep=strb=0b11`; không quy định phục hồi byte-enable sai.
- Ingress chuyển `data,user,last` sang packet plain. Egress zero-extend RAW10, đặt `keep=strb=0b11` và dùng marker engine đã tạo. Không dùng `ap_axiu` làm payload FIFO giữa engine.
- Reset hủy transaction đang dở; không bảo đảm đủ N output cho frame bị hủy. Các beat đã được downstream nhận trước reset không thể thu hồi. Sau reset, phần mềm cấu hình/start lại; nguồn bắt đầu frame mới với SOF.
- Frame counters của engine là local cho mỗi lần gọi, không giữ tiến độ frame cũ qua transaction. Reset phải xóa control/in-flight channel state theo cấu hình HLS được kiểm tra khi implement.
- Khi tích hợp ISP, dùng ingress chung để align SOF một lần, đưa N packet vào chuỗi engine, cấp snapshot config phù hợp và dùng egress chung. Không gọi standalone `blc_top` hoặc lặp lại AXIS adapters giữa các engine.
- `blc_engine` dùng được độc lập khi caller cấp đúng packet/config contract. Nó không tự free-run hoặc tự tìm lại SOF khi bỏ TOP.

## 8. Điều kiện nghiệm thu implementation

1. **CSim/reference:** dữ liệu RAW10 đúng với reference độc lập; đúng N output, thứ tự, SOF/EOL; pixel SOF đầu được xử lý; packet trước SOF bị bỏ; incoming EOL sai/SOF thừa không đổi counter.
2. **Reuse:** cùng engine được gọi trực tiếp với plain packet stream và qua standalone wrapper, cho cùng kết quả; không cần wrapper-specific token/register trong engine.
3. **Synthesis:** xác nhận AXIS + AXI-Lite + `ap_ctrl_hs`, ba process DATAFLOW và FIFO hữu hạn; kiểm achieved II và không buffer whole-frame ngoài yêu cầu.
4. **Generated-RTL CoSim/traffic:** input gaps, output backpressure, dừng nguồn ngay sau N pixel, hai transaction liên tiếp, đổi config giữa transaction, kiểm config-before-start và frame đầu sau reset.
5. **RTL handshakes:** scoreboard theo accepted AXIS beats; đếm riêng packet bỏ trước SOF và N output; xác nhận không mất/lặp/đổi thứ tự, pixel đầu và cuối đúng. Quan sát riêng done và external beat cuối.
6. **Reset giữa frame:** frame dở bị hủy, restart với config/SOF mới không dùng tọa độ hoặc packet cũ. Không tuyên bố reset behavior từ CSim đơn thuần.

Ghi kết quả thực tế vào [verification-status.md](verification-status.md) với revision, geometry, tool/version và loại harness. Các báo cáo BLC cũ chỉ là evidence lịch sử, không chứng minh bản contract này đã pass.
