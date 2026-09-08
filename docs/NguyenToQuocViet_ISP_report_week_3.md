<!--
Project: Adaptive Directional BPC and BLC
Module: Week 3 ISP Progress Report
Description: Summarize defect injection, fixed-threshold BPC, and initial BLC progress.
Author: Viet Nguyen To Quoc
-->

# Báo cáo tiến độ Week 1 (Week 3 ISP)

## Mục lục

1. [Tổng quan](#1-tổng-quan)
2. [Nguồn ảnh và defect injector](#2-nguồn-ảnh-và-defect-injector)
3. [BPC fixed-threshold baseline](#3-bpc-fixed-threshold-baseline)
4. [Flow test và kết quả ban đầu](#4-flow-test-và-kết-quả-ban-đầu)
5. [BLC baseline](#5-blc-baseline)

## 1. Tổng quan

Trong tuần này em tập trung vào việc dựng dữ liệu lỗi giả lập và hoàn thiện BPC fixed-threshold baseline để làm mốc so sánh. Flow từ ảnh RAW, chèn defect, chạy BPC và đối chiếu kết quả đã chạy được trên một ảnh thử.

| Hạng mục | Trạng thái | Ghi chú |
|---|---|---|
| Defect injector | Đã hoàn thành | Tạo được ảnh sạch, ảnh lỗi và ground truth lặp lại được |
| BPC fixed-threshold baseline | Đã hoàn thành bản C++ | Đã chạy full flow và quét ngưỡng trên một ảnh |
| BLC baseline | Đã hoàn thành thuật toán C++ | Chưa test, chưa HLS compile và chưa optimize |

## 2. Nguồn ảnh và defect injector

Ảnh thử là file RAW `a0001-jmac_DSC1459.dng` từ bộ [MIT-Adobe FiveK](https://data.csail.mit.edu/graphics/fivek/). Đây là ảnh Bayer CFA và được LibRaw đọc ở dạng raw plane. Ảnh được crop về RGGB 1920 × 1080, sau đó chuẩn hóa về RAW12 trong khoảng `[0, 4095]`.

Defect injector chèn 150 hot pixel, 150 dead pixel và 25 stuck pixel. Các vị trí lỗi nằm ngoài viền hai pixel và có khoảng cách tối thiểu để tránh chồng lên nhau. Random seed được cố định để có thể chạy lại cùng một bộ dữ liệu.

Injector tạo ra ba file:

- `clean_rggb.pgm`: ảnh trước khi chèn lỗi;
- `corrupted_rggb.pgm`: ảnh sau khi chèn lỗi;
- `defects.csv`: tọa độ, loại defect, giá trị trước và sau khi chèn lỗi.

Trong lần chạy hiện tại, cả 325 defect đều làm thay đổi giá trị pixel. File CSV được dùng làm ground truth khi đánh giá BPC.

## 3. BPC fixed-threshold baseline

Baseline đang dùng cửa sổ 5 × 5. Với mỗi center pixel, em lấy tám pixel cùng CFA ở khoảng cách hai pixel theo các hướng ngang, dọc và chéo. Tám giá trị được sort, sau đó median được tính từ trung bình của hai phần tử giữa.

Độ lệch giữa center pixel và median được so với ngưỡng cấu hình cho từng phase R/G/B. Nếu độ lệch lớn hơn ngưỡng, pixel được đánh dấu là defect và thay bằng median. Hai hàng và hai cột ngoài cùng được giữ nguyên vì không đủ cửa sổ 5 × 5.

## 4. Flow test và kết quả ban đầu

Flow đang dùng:

```text
MIT-Adobe FiveK DNG
        |
        v
Defect injector
        |
        +-- clean_rggb.pgm
        +-- corrupted_rggb.pgm
        +-- defects.csv
        |
        v
BPC fixed-threshold baseline
        |
        v
Đối chiếu detection với ảnh sạch và defects.csv
```

Em đã quét toàn bộ ngưỡng nguyên từ 0 đến 4095, dùng cùng một ngưỡng cho R/G/B. Điểm F1 tốt nhất trên ảnh thử hiện tại nằm ở ngưỡng 1063.

| Kết quả tại ngưỡng 1063 | Giá trị |
|---|---:|
| True positive | 108/325 |
| False positive | 37 |
| Precision | 74,5% |
| Recall | 33,2% |
| F1 | 46,0% |
| Hot detect được | 86/150 |
| Dead detect được | 3/150 |
| Stuck detect được | 19/25 |

Kết quả này ưu tiên giảm false positive nhưng bỏ sót nhiều dead pixel. Em thử thêm một operating point ưu tiên dead pixel tại ngưỡng 79.

| Kết quả tại ngưỡng 79 | Giá trị |
|---|---:|
| True positive | 310/325 |
| False positive | 547471 |
| Precision | 0,057% |
| Recall | 95,4% |
| F1 | 0,113% |
| Hot detect được | 150/150 |
| Dead detect được | 135/150 |
| Stuck detect được | 25/25 |

Ngưỡng 79 bắt được gần hết defect, nhưng precision và F1 giảm rất mạnh vì số pixel bình thường bị detect nhầm quá lớn.

Hai điểm trên cho thấy trade-off của fixed threshold khá rõ: ngưỡng cao giữ false positive thấp nhưng bỏ sót defect có độ lệch nhỏ; ngưỡng thấp bắt được nhiều dead pixel hơn nhưng dễ sửa nhầm edge và texture tự nhiên.

Các số liệu này mới được đo trên một ảnh và ngưỡng đang dùng giống nhau cho ba phase. Vì vậy ngưỡng 1063 hiện chỉ là kết quả thử ban đầu, chưa phải cấu hình cuối để dùng cho toàn bộ tập ảnh.

## 5. BLC baseline

BLC hiện có bản C++ baseline. Thuật toán xác định phase R/Gr/Gb/B từ tọa độ pixel, chọn black level tương ứng rồi trừ khỏi giá trị RAW. Nếu giá trị RAW nhỏ hơn black level thì output được clamp về 0.

Phần BLC hiện mới dừng ở mức thuật toán C++. Em chưa chạy test, chưa đưa qua HLS compile và chưa tối ưu pipeline, initiation interval hoặc tài nguyên phần cứng.

## Kết luận

**Kết quả BPC baseline hiện tại càng khẳng định giới hạn của fixed threshold: một ngưỡng cố định là không đủ để vừa bắt được nhiều bad pixel, vừa tránh sửa nhầm trên ảnh thực tế. Ngưỡng cao bỏ sót phần lớn dead pixel, trong khi ngưỡng thấp bắt được dead pixel tốt hơn nhưng tạo ra quá nhiều false correction.**
