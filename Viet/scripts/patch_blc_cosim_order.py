# Project: Adaptive Directional BPC and BLC
# Module: BLC CoSim AXI-Lite Ordering
# Description: Make Vitis's generated CoSim driver publish config_valid after four completed black-level writes.
# Author: Viet Nguyen To Quoc

import sys
from pathlib import Path


def replace_once(source: str, old: str, new: str) -> str:
    count = source.count(old)
    if count != 1:
        raise RuntimeError(f"Expected one generated CoSim marker, found {count}: {old!r}")
    return source.replace(old, new, 1)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: patch_blc_cosim_order.py <generated_sequence.sv>")

    path = Path(sys.argv[1])
    source = path.read_text()
    if "bit bl_r_written = 0;" in source:
        raise RuntimeError("CoSim sequence is already patched")

    declarations = """                                bit bl_r_written = 0;
                                bit bl_gr_written = 0;
                                bit bl_gb_written = 0;
                                bit bl_b_written = 0;
"""
    source = replace_once(
        source,
        "                                logic[32-1:0] databusbit_config_valid[$];\n",
        "                                logic[32-1:0] databusbit_config_valid[$];\n"
        + declarations,
    )

    # Vitis forks the five AXI-Lite writers. A completed uvm_send marks the
    # write transaction's end; waiting for the DirectIO ack here deadlocks,
    # because the publisher only reads levels after config_valid becomes 1.
    for name in ("bl_r", "bl_gr", "bl_gb", "bl_b"):
        marker = f"                                            `uvm_send(axi_master_wr_control_{name}_seq);\n"
        source = replace_once(
            source,
            marker,
            marker + f"                                            {name}_written = 1;\n",
        )

    marker = "                                        int control_config_valid_data_size = refm.TVIN_control_config_valid_data_size_queue[0];\n"
    source = replace_once(
        source,
        marker,
        marker
        + "                                        wait(bl_r_written && bl_gr_written && bl_gb_written && bl_b_written);\n",
    )
    path.write_text(source)


if __name__ == "__main__":
    main()
