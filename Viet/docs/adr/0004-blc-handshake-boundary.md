<!--
Project: Adaptive Directional BPC and BLC
Module: BLC Handshake Boundary Decision
Description: Record the accepted finite-frame BLC architecture and the placement of SOF admission at common ingress.
Author: Viet Nguyen To Quoc
-->

# BLC finite-frame với TOP/ENGINE boundary

Status: accepted; source implementation prepared, synthesis and behavioral verification pending.
Date: 2026-09-30

## Context

Việt muốn một bản BLC `ap_ctrl_hs` đơn giản như mô hình frame transaction cũ, đồng thời giữ engine tái sử dụng trực tiếp trong ISP. Standalone TOP phải sở hữu interface/config/wiring; pixel algorithm giữ nguyên. Kiến trúc free-running hiện tại tiếp tục có contract riêng.

## Decision and rationale

Adopt [BLC handshake contract](../blc_handshake_contract.md) as the authority for this variant. Use three finite processes under DATAFLOW: ingress, `blc_engine`, egress. Move SOF admission to common ingress and use fixed frame geometry inside the engine. Snapshot configuration per transaction using the start boundary.

Một vòng lặp đủ frame trong engine giúp caller ISP không cần standalone task runner. FIFO truyền từng pixel để các process overlap; finite-frame không yêu cầu buffer cả frame. Khi ingress đã cấp đúng N packet, engine phải tạo đúng N output; guard đọc bỏ tại engine sẽ phá invariant đó.

## Alternatives considered

- Trust packet đầu sau start là SOF: đơn giản hơn nhưng không giữ khả năng bỏ packet trước SOF tại boundary ngoài.
- Guard SOF trong finite engine: muốn đúng phải đồng bộ lại admission/count giữa các process; không chọn vì ingress chung đã sở hữu việc này.
- Dùng SOF/EOL làm authority hình học: cần thêm quy tắc marker sớm/muộn và termination; không cần cho frame cố định.
- Giữ task/publisher/startup token của free-running: thêm lifecycle không cần thiết cho transaction hữu hạn; bản free-running vẫn giữ thiết kế riêng.

## Evidence and consequences

Việt đã chốt contract sau khi xem hierarchy và làm rõ rằng ba function chạy overlap trong DATAFLOW. Quyết định này là architecture acceptance, không phải kết quả chạy HLS. Bản free-running một pixel mỗi invocation được giữ nguyên nội dung trong `none`; source finite-frame trong `handshake` được clone và chỉ sửa các phần control/lifecycle/boundary cần thiết.

Bản mới chấp nhận khoảng nghỉ transaction và không yêu cầu zero-gap. Config có thể đổi giữa transaction. Input thiếu SOF/pixel làm transaction chờ; không có recovery cho mất/chèn pixel giữa frame.

Việt chốt bố trí source tại `hls/blc/none/` và `hls/blc/handshake/`: chuyển bản hiện tại vào `none` nguyên nội dung, clone sang `handshake` và chỉ sửa phần cần thiết cho contract. Mỗi build chọn một variant để tránh link hai semantics khác nhau dưới cùng tên engine. Implementation source được giao riêng; CSim và CoSim chưa thuộc turn triển khai này.

## Revisit condition

Revisit với Việt nếu yêu cầu chuyển sang stream vô hạn/zero-gap, geometry runtime, recovery marker hoặc nếu generated RTL không đáp ứng snapshot ordering, finite DATAFLOW hay backpressure contract. Dùng evidence cụ thể trước khi đổi contract.
