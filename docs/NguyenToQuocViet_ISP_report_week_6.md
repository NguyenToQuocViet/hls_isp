<!--
Project: Adaptive Directional BPC and BLC
Module: Week 6 ISP Progress Report
Description: Summarize RAW10 pipeline integration and camera-to-FPGA research.
Author: Viet Nguyen To Quoc
-->

# Báo cáo tiến độ Week 4 (Week 6 ISP)

## Những việc đã làm

| Công việc | Trạng thái hiện tại |
|---|---|
| Chuyển các khối sang RAW10 | Cập nhật input, reference, BLC/BPC HLS và flow dữ liệu; chạy lại các bước kiểm tra liên quan. |
| Ghép BLC → BPC bằng HLS DATAFLOW | Đã chạy CSim, C Synthesis và CoSim cho top ghép với một frame 1920 × 1080. |
| Camera input cho ZCU102 | Tìm hiểu đường MIPI CSI-2/D-PHY, adapter FMC và cách nối các IP trong Vivado. Chưa đưa camera lên board. |
| Chọn sensor | IMX219 không có sẵn; sau khi tìm hiểu thêm IMX477 và IMX708, hiện em nghiêng về IMX477. |

## RAW10 và kiểm tra lại pipeline

Em chuyển dữ liệu pixel từ RAW12 `[0, 4095]` xuống RAW10 `[0, 1023]` để khớp hơn với camera đang tìm hiểu. BLC và BPC HLS đã đổi các kiểu dữ liệu liên quan sang 10-bit. Cổng AXI4-Stream vẫn dùng word 16-bit, trong đó pixel nằm ở `TDATA[9:0]`. Ở phía dữ liệu thử, injector đưa ảnh DNG về RAW10 và scale Black Level theo cùng miền giá trị. Evaluator, flow tuning và testbench cũng được cập nhật theo thay đổi này.

Ở Week 5.2, BLC và BPC độc lập đã pass Reference, CSim, C Synthesis và CoSim với RAW10. Sau đó em tách phần xử lý stream để ghép hai block vào một top. Bản mới đã C synthesis được cho cả các top độc lập lẫn top ghép; CoSim riêng từng block thì chưa chạy lại sau lần sửa này. Với top ghép, em đã chạy CSim và CoSim trên một frame RAW10. Test so output với reference chạy BLC rồi BPC, gồm cả pixel và các tín hiệu `TUSER`, `TLAST`, `TKEEP`, `TSTRB`. Cả hai lần chạy đều không báo lỗi trên 2.073.600 pixel.

C Synthesis nhận top ghép là `dataflow`; các loop xử lý pixel đạt II=1 trong báo cáo HLS. Phần này mới được kiểm tra với một frame, chưa có kết quả timing sau place-and-route, backpressure ngẫu nhiên hay nhiều frame liên tục. Config RAW10 và kết quả tuning đã ghi ở báo cáo Week 5.2.

## Ghép BLC và BPC trong DATAFLOW

Trước khi ghép pipeline, BLC chạy được ở bài test riêng. Nhưng khi đưa vào top-level DATAFLOW, dữ liệu lại không đi vào BLC như em mong đợi. Em phải xem lại cách HLS chia các hàm thành stage, nối producer với consumer qua `hls::stream`, và cách các lệnh đọc/ghi stream ảnh hưởng tới việc các stage chạy cùng lúc. Log của lần gặp lỗi không còn đủ để chỉ ra chính xác nó bị kẹt ở lệnh đọc/ghi hay ở cách HLS lập lịch.

Trong code hiện tại, top ghép có hai adapter ở cổng AXI4-Stream và ba stream nội bộ. Đường đi của một pixel là:

```text
AXI4-Stream input → axis_to_internal_frame → blc_process_frame
                  → bpc_process_frame → internal_to_axis_frame
                  → AXI4-Stream output
```

Các hàm trên là những stage riêng trong vùng `#pragma HLS DATAFLOW`. Adapter đầu đọc packet AXI4-Stream rồi ghi vào `raw_pixels`. BLC lấy pixel từ đó, trừ Black Level theo CFA phase và ghi kết quả sang `blc_to_bpc`. BPC đọc tiếp stream này, xử lý pixel rồi chuyển cho adapter đầu ra. Ba stream nội bộ là FIFO depth 2; packet vẫn mang dữ liệu pixel và các sideband cần thiết. Các top BLC/BPC độc lập cũng gọi chính các hàm xử lý này.

Với cách nối này, một frame đã đi qua cả BLC và BPC trong CSim và RTL CoSim. Test hiện nạp sẵn toàn bộ frame, chưa thử trường hợp input ngắt quãng hoặc output bị backpressure.

## Tìm hiểu đường camera vào FPGA

Phần camera tuần này chủ yếu là tìm hiểu đường đi của dữ liệu từ sensor vào FPGA. Trước hết, em phân biệt RAW10/RAW12 là format pixel, CSI-2 là protocol truyền dữ liệu camera, còn D-PHY là lớp tín hiệu vật lý bên dưới. Trên D-PHY, mỗi cặp `+/-` tạo thành một lane, chẳng hạn cặp clock hoặc các cặp data `D0`, `D1`; hai dây trong một cặp không phải hai lane riêng. Tín hiệu vi sai giúp giảm ảnh hưởng của nhiễu chung khi truyền ở tốc độ cao.

Cáp FFC/FPC và đầu nối camera chỉ là phần kết nối vật lý. Camera dùng đầu 15-pin hay 22-pin vẫn có thể truyền CSI-2, nhưng phải xem đúng pinout, nguồn và số lane của module đó. Với ZCU102, em đang tìm hiểu adapter camera-to-FMC để đưa các cặp MIPI, I2C/control và power/GND sang board. FMC làm nhiệm vụ nối tín hiệu, không chuyển đổi protocol. XDC cũng không tạo ra đường nối mới: nó gán port trong thiết kế vào package pin đã được PCB nối tới FMC. Khi chọn chân còn phải kiểm tra cặp vi sai, I/O bank, điện áp và loại I/O.

Đường xử lý dự kiến ở FPGA là:

```text
Sensor → MIPI CSI-2 / D-PHY → camera connector / adapter FMC
       → ZCU102 → AMD MIPI CSI-2 RX → AXI4-Stream RAW pixel → ISP
```

Phía FPGA sẽ nhận tín hiệu qua D-PHY RX và CSI-2 RX IP của AMD. Sau bước này mới có pixel stream để đưa vào ISP của nhóm. Theo [tài liệu MIPI CSI-2 RX của AMD](https://docs.amd.com/r/en-US/ug1449-multimedia/MIPI-CSI-2-Receiver-Subsystem), đầu ra có thể là AXI4-Stream video; D-PHY có thể nằm trong subsystem hoặc là IP riêng tùy phiên bản và cách cấu hình. Trước khi nối với top ISP hiện tại, em còn phải kiểm tra format pixel, số pixel mỗi clock và sideband ở đầu ra RX.

Một câu hỏi khác là có cần Video Frame Buffer trước ISP không. Một frame 1920 × 1080 có 2.073.600 pixel; nếu chạy 30 fps thì cần xử lý khoảng 62,2 triệu pixel mỗi giây. Nếu các interface tương thích, stream từ CSI-2 RX có thể đi thẳng vào ISP. Còn nếu cần lưu frame để debug, cho CPU truy cập hoặc tách các miền xử lý, có thể ghi vào DDR rồi đọc lại. Cách qua DDR sẽ tốn thêm bandwidth và tăng latency. Trong Vivado Block Design, em sẽ phải nối MIPI RX, ISP IP, clock/reset, AXI và DDR/DMA nếu dùng frame buffer; logic xử lý pixel vẫn nằm trong các block RTL/HLS.

## So sánh IMX219, IMX477 và IMX708

Ban đầu em xem IMX219. Tuy nhiên, IMX219 hiện không có sẵn nên em không chọn module này để triển khai. Em chuyển sang tìm hiểu IMX477 trên [Raspberry Pi High Quality Camera](https://www.raspberrypi.com/products/hqcam/); [Raspberry Pi liệt kê](https://www.raspberrypi.com/documentation/computers/compute-module.html) cả mode RAW10 và RAW12. IMX477 dùng Bayer RAW truyền thống, phù hợp với mục tiêu đưa dữ liệu sensor qua ISP tự xây dựng. Camera sẽ được cấu hình qua I2C, còn dữ liệu ảnh đi qua CSI-2/D-PHY. Phần adapter và cấu hình trên ZCU102 vẫn cần làm rõ.

Em cũng xem IMX708 của Camera Module 3. Sensor này dùng [Quad Bayer](https://forums.raspberrypi.com/viewtopic.php?p=2072376#p2072376), có các mode binning/remosaic nên dữ liệu đưa ra có thể đã qua xử lý trong sensor. Binning có thể giúp giảm noise ở một số điều kiện, nhưng khi đánh giá BPC tự thiết kế, em muốn dễ phân biệt phần nào do sensor và phần nào do ISP xử lý. Vì vậy hiện em nghiêng về IMX477. Đây vẫn là lựa chọn ở giai đoạn nghiên cứu; chưa có kết quả nối camera với ZCU102 hay đo chất lượng ảnh trên board.

Hướng em đang tính là IMX477 → MIPI CSI-2/D-PHY → FMC/ZCU102 → AMD CSI-2 RX → AXI4-Stream RAW10 → ISP. Pipeline đã chuyển sang RAW10, nhưng trước khi nối camera thật em vẫn cần chốt pinout, cấu hình RX IP và cách ghép stream với top ISP.
