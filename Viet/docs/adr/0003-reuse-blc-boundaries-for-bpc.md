<!--
Project: Adaptive Directional BPC and BLC
Module: BPC Streaming Boundary Decision
Description: Record the accepted reuse of BLC wrapper boundaries with BPC-specific storage and drain control.
Author: Viet Nguyen To Quoc
-->

# Reuse BLC boundaries for streaming BPC

Status: accepted for implementation; BPC streaming verification pending.
Date: 2026-09-28

## Context

Việt requested carrying the established BLC HLS mental model into BPC while retaining the existing pixel algorithm. BLC already separates AXI/configuration publication, persistent task execution, stream engine state and pixel arithmetic. BPC additionally needs delayed-center ownership, line-buffer advancement and frame-tail drain.

## Decision and rationale

Adopt the [BPC streaming contract](../bpc_streaming_contract.md) as the authority for non-algorithmic BPC behavior. Reuse the BLC wrapper/config/task boundaries and standalone free-running control. Keep BPC storage, real/synthetic arbitration and output ownership inside its engine, with independent progress domains.

This preserves the existing algorithm and avoids repeating the BLC configuration design work. Copying BLC's unconditional one-pixel read/process/write step would prevent autonomous BPC tail drain when no next frame arrives. A finite frame-count transaction would not meet the accepted indefinite-stream contract. FLP alone cannot advance centers still held in the line buffers.

## Evidence and consequences

The [BLC evidence](../verification-status.md#standalone-blc-streaming-trên-branch-rebuild-2026-09-2728) records bounded 16×16 scheduling and RTL results, including ordered configuration and continuous frame boundaries. This motivates reuse; it does not verify BPC behavior or throughput. The new contract defines separate BPC acceptance gates.

Standalone BPC configuration is immutable until reset. Full-ISP register mapping and commit policy remain integration work. FSM/validity encoding may be selected during implementation within the contract; the older proposal's descriptor mechanism is not mandatory.

## Revisit condition

Revisit with Việt if generated RTL cannot meet the accepted stream behavior, memory scheduling prevents the required throughput, or system requirements demand runtime configuration updates, different geometry, or a new external protocol. Record concrete evidence before changing the contract.
