<!--
Project: Adaptive Directional BPC and BLC
Module: BPC Handshake Frame Drain Decision
Description: Record the accepted BPC finite-frame trade-off and reuse of current storage with BLC handshake boundaries.
Author: Viet Nguyen To Quoc
-->

# BPC handshake với drain riêng mỗi frame

Status: accepted; source variants and standalone handshake TOP implemented. Functional HLS/RTL verification pending.
Date: 2026-09-30

## Context

Việt cần BPC dễ tích hợp theo frame trong thời gian ngắn. Việt cho biết LTM và CNN Denoise của nhóm đang yêu cầu mô hình control có start/done; đây là bối cảnh lựa chọn, chưa phải kết quả audit implementation hoặc contract toàn nhóm. BLC đã có boundary handshake riêng; BPC hiện tại có engine free-running nhưng chưa có standalone TOP.

Git `f9fa1d6` cho thấy mô hình một-frame N+D hữu hạn; `b55eb38` có worker đã tách cùng thử nghiệm multi-frame. Engine hiện tại là baseline cần giữ code/logic/comment và pixel algorithm, thay scheduling tối thiểu.

## Decision and rationale

Adopt [BPC handshake contract](../bpc_handshake_contract.md). Việt duyệt toàn bộ bốn quyết định: một frame/start và drain riêng; boundary/config/ingress guard theo BLC hs; static storage không clear RAM cùng control khởi tạo mỗi invocation; giữ none nguyên nội dung và clone sang handshake.

Finite loop biết trước lượng real input và output ownership nên dùng blocking I/O, bỏ arbitration real/next-frame/synthetic của bản none. Cùng engine phải dùng được từ ISP khi standalone TOP được bỏ ra. Mục tiêu là giảm complexity của lifecycle và integration; đây không phải bằng chứng bản hs đã ổn định trên RTL.

## Alternatives considered

- Indefinite free-running/overlap: có thể dùng frame mới đẩy tail frame trước và tránh drain riêng mỗi frame, nhưng cần quản lý progress/frame ownership và config lifetime khác. [ADR 0003](0003-reuse-blc-boundaries-for-bpc.md) tiếp tục áp dụng riêng cho variant none.
- Batch nhiều frame mỗi start: giảm drain theo batch nhưng thêm frame_count/control và thay đổi transaction contract; không chọn cho bản này.
- Storage local với guarded history như bản cũ: có thể triển khai, nhưng khác baseline hiện tại hơn. Chọn giữ static banks/window và chứng minh frame independence bằng warmup/ownership/border behavior.

## Consequences and evidence boundary

Mỗi frame cần D=2W+2 synthetic advances sau N real input; engine vẫn xuất tail nhưng chưa nhận frame mới. D/N xấp xỉ 0,185% ở 1920×1080 trong mô hình advances lý tưởng, không phải số đo RTL. Chấp nhận inter-transaction gap, không yêu cầu zero-gap.

Config có thể đổi giữa transaction và phải ổn định qua drain. Không SOF/thiếu pixel làm transaction chờ; không phục hồi frame mất/chèn pixel. Static storage còn history phải được kiểm bằng frame khác nhau và restart sau reset, không dựa vào RAM zero. Turn này chỉ ghi contract; các acceptance gates vẫn pending.

## Revisit condition

Revisit với Việt nếu hệ thống yêu cầu zero-gap/batch, input không thể chịu backpressure, frame geometry thay đổi, stale history ảnh hưởng output, hoặc tool không đáp ứng finite DATAFLOW/memory scheduling/reset contract. Báo evidence trước khi đổi kiến trúc; không tự thêm valid metadata hoặc đổi thuật toán.
