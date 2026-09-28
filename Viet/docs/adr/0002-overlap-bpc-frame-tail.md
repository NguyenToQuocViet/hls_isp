<!--
Project: Adaptive Directional BPC and BLC
Module: BPC Frame Tail Overlap Decision
Description: Record the decision to retire a frame tail while accepting the next frame.
Author: Viet Nguyen To Quoc
-->

# Overlap BPC frame retirement with next-frame input

Status: experimental finite-frame decision; superseded as production target by the [streaming contract](../bpc-adaptive-directional.md#11-hls-streaming-architecture)
Date: 2026-09-24

## Context and decision

The original one-frame BPC loop reads no input during its final `2 * FRAME_WIDTH + 2` iterations. Its adapters and BLC also stop after one frame, and the AXI4-Lite controlled top cannot begin a new invocation until the current one completes. Loop II=1 therefore does not imply zero inter-frame input gap.

Use a separate AXI4-Lite controlled, finite `isp_top_frames` entry point. All stages process the requested number of frames in one invocation. The BPC keeps one continuous line-buffer/window state, accepts the next frame while retiring the previous frame's bypass-border tail, and drains only after the final frame. The one-frame entry points remain available with their existing interface.

## Rationale and alternatives

The tail needs center pixels from the horizontal registers and the two newest line-buffer banks, but does not run the BPC kernel. Each bank already requires one read and one write per accepted pixel and currently infers `ram_s2p`. Reading the old value before updating the same column can retire the old frame while filling the next frame without another frame-sized buffer. This read/write ordering and II=1 must be confirmed by synthesis and RTL CoSim.

A prefetch FIFO of at least the delayed input span would add storage and would not by itself make the one-frame top accept another invocation. Ping-pong line buffers would add four more banks despite the single read/write access pattern. A separate window-generation DATAFLOW stage adds a window FIFO and frame-state coordination; it is reserved for a demonstrated timing or scheduling problem. A free-running top would change the block-control and configuration contract and needs an explicit end/flush policy.

## Consequences and revisit condition

Input and output frame counters now have independent lifetimes. The line-buffer validity counter saturates after the first four input rows and does not reset at SOF. The output coordinates wrap at each frame for CFA phase and AXI sidebands. The last frame still incurs one final drain and the next invocation can have a restart gap. Revisit this decision if two-frame RTL handshakes show a gap, the inferred memory cannot sustain one read and one write per cycle, or the project requires a truly free-running IP.

This ADR records why the finite `frame_count` experiment was introduced; it is not the normative production-video behavior. The current target is in [the BPC streaming contract](../bpc-adaptive-directional.md#11-hls-streaming-architecture); observed results belong in `verification-status.md`.
