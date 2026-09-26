<!--
Project: Adaptive Directional BPC and BLC
Module: RAW10 HLS C Synthesis Evidence
Description: Record timing and resource estimates from the RAW10 BLC and BPC C synthesis runs.
Author: Viet Nguyen To Quoc
-->

# HLS RAW10 C Synthesis — Week 5.2

Đây là kết quả C Synthesis lịch sử của source RAW10 trước branch rebuild. Các top trong bảng đã được lưu ở branch `archive/bpc-agent-hls-2026-09-26`; branch rebuild hiện chưa có top HLS để tổng hợp.

## Run configuration

| Trường | Giá trị |
|---|---|
| Tool | Vitis HLS 2026.1 (Build 6493734, Jun 16 2026) |
| Device | `xczu7ev-ffvc1156-2-e` |
| Target clock | 6,67 ns; 6,00 ns sau uncertainty |
| BLC top | `isp_blc_top` |
| BPC top | `isp_bpc_top` |

## Timing và latency

| Khối | Target period | Estimated period | Slack | Achieved II | Latency |
|---|---:|---:|---:|---:|---:|
| BLC | 6,67 ns | 3,119 ns | +2,881 ns sau uncertainty | 1 | 13,83 ms |
| Adaptive Directional BPC | 6,67 ns | 5,613 ns | +0,387 ns sau uncertainty | 1 | 13,85 ms |

## Resource utilization

| Khối | LUT | FF | BRAM18K | DSP | URAM |
|---|---:|---:|---:|---:|---:|
| BLC | 444 | 161 | 0 | 0 | — |
| Adaptive Directional BPC | 1.843 | 836 | 8 | 0 | 0 |
