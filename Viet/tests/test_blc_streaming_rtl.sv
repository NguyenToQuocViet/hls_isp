// Project: Adaptive Directional BPC and BLC
// Module: Standalone BLC RTL Timing Test
// Description: Check ordered configuration, exact stream counts, stalls, and reset on generated BLC RTL.
// Author: Viet Nguyen To Quoc

`timescale 1ns/1ps

module test_blc_streaming_rtl;
    reg clk = 0;
    always #5 clk = ~clk;
    reg rst_n = 0;

    reg awvalid = 0;
    wire awready;
    reg [5:0] awaddr = 0;
    reg wvalid = 0;
    wire wready;
    reg [31:0] wdata = 0;
    reg [3:0] wstrb = 4'hf;
    wire bvalid;
    reg bready = 0;
    wire [1:0] bresp;
    wire arready, rvalid;
    wire [31:0] rdata;
    wire [1:0] rresp;

    reg [15:0] in_data = 0;
    reg [1:0] in_keep = 2'b11, in_strb = 2'b11;
    reg [0:0] in_user = 0, in_last = 0;
    reg in_valid = 0;
    wire in_ready;
    wire [15:0] out_data;
    wire [1:0] out_keep, out_strb;
    wire [0:0] out_user, out_last;
    wire out_valid;
    reg out_ready = 1;

    blc_top dut (
        .s_axi_control_AWVALID(awvalid), .s_axi_control_AWREADY(awready),
        .s_axi_control_AWADDR(awaddr), .s_axi_control_WVALID(wvalid),
        .s_axi_control_WREADY(wready), .s_axi_control_WDATA(wdata),
        .s_axi_control_WSTRB(wstrb), .s_axi_control_ARVALID(1'b0),
        .s_axi_control_ARREADY(arready), .s_axi_control_ARADDR(6'b0),
        .s_axi_control_RVALID(rvalid), .s_axi_control_RREADY(1'b0),
        .s_axi_control_RDATA(rdata), .s_axi_control_RRESP(rresp),
        .s_axi_control_BVALID(bvalid), .s_axi_control_BREADY(bready),
        .s_axi_control_BRESP(bresp), .ap_clk(clk), .ap_rst_n(rst_n),
        .input_r_TDATA(in_data), .input_r_TKEEP(in_keep),
        .input_r_TSTRB(in_strb), .input_r_TUSER(in_user),
        .input_r_TLAST(in_last), .input_r_TVALID(in_valid),
        .input_r_TREADY(in_ready), .output_r_TDATA(out_data),
        .output_r_TKEEP(out_keep), .output_r_TSTRB(out_strb),
        .output_r_TUSER(out_user), .output_r_TLAST(out_last),
        .output_r_TVALID(out_valid), .output_r_TREADY(out_ready)
    );

    integer cycle = 0;
    integer input_count = 0;
    integer output_count = 0;
    integer case_id = 0;
    integer last_input_cycle = -1;
    integer first_next_input_cycle = -1;
    integer last_output_cycle = -1;
    integer first_next_output_cycle = -1;
    integer black_r = 64, black_gr = 66, black_gb = 65, black_b = 68;

    function automatic integer source_data(input integer index, input integer id);
        source_data = (index * 37 + id * 101 + 103) & 1023;
    endfunction

    function automatic integer expected_data(input integer index, input integer id);
        integer row, col, black, raw;
        begin
            row = (index % 256) / 16;
            col = index % 16;
            if ((row & 1) == 0)
                black = (col & 1) ? black_gr : black_r;
            else
                black = (col & 1) ? black_b : black_gb;
            raw = source_data(index, id);
            expected_data = raw > black ? raw - black : 0;
        end
    endfunction

    always @(posedge clk) begin
        cycle = cycle + 1;
        if (cycle > 10000) $fatal(1, "Watchdog: in=%0d out=%0d", input_count, output_count);
        if (rst_n) begin
            if (in_valid && in_ready) begin
                if (input_count == 255) last_input_cycle = cycle;
                if (input_count == 256) first_next_input_cycle = cycle;
                input_count = input_count + 1;
            end
            if (out_valid && out_ready) begin
                if (output_count >= input_count)
                    $fatal(1, "Output without accepted input at beat %0d", output_count);
                if (out_data !== expected_data(output_count, case_id) ||
                    out_keep !== 2'b11 || out_strb !== 2'b11 ||
                    out_user !== (output_count % 256 == 0) ||
                    out_last !== (output_count % 16 == 15))
                    $fatal(1, "Mismatch case=%0d beat=%0d got=%0d expected=%0d user=%0d last=%0d",
                           case_id, output_count, out_data,
                           expected_data(output_count, case_id), out_user, out_last);
                if (output_count == 255) last_output_cycle = cycle;
                if (output_count == 256) first_next_output_cycle = cycle;
                output_count = output_count + 1;
            end
        end
    end

    task automatic reset_dut(input integer id);
        begin
            @(negedge clk);
            rst_n = 0;
            in_valid = 0;
            out_ready = 1;
            awvalid = 0;
            wvalid = 0;
            bready = 0;
            repeat (5) @(negedge clk);
            case_id = id;
            input_count = 0;
            output_count = 0;
            last_input_cycle = -1;
            first_next_input_cycle = -1;
            last_output_cycle = -1;
            first_next_output_cycle = -1;
            rst_n = 1;
        end
    endtask

    task automatic axil_write(input reg [5:0] address, input reg [31:0] value);
        reg aw_done, w_done;
        begin
            @(negedge clk);
            awaddr = address;
            wdata = value;
            awvalid = 1;
            wvalid = 1;
            aw_done = 0;
            w_done = 0;
            while (!aw_done || !w_done) begin
                @(posedge clk);
                if (awvalid && awready) aw_done = 1;
                if (wvalid && wready) w_done = 1;
                @(negedge clk);
                if (aw_done) awvalid = 0;
                if (w_done) wvalid = 0;
            end
            while (!bvalid) @(negedge clk);
            if (bresp !== 2'b00) $fatal(1, "AXI-Lite write error at %h", address);
            bready = 1;
            @(negedge clk);
            bready = 0;
        end
    endtask

    task automatic configure(input integer delay_cycles);
        begin
            repeat (delay_cycles) @(negedge clk);
            axil_write(6'h10, black_r);
            axil_write(6'h18, black_gr);
            axil_write(6'h20, black_gb);
            axil_write(6'h28, black_b);
            axil_write(6'h30, 1);
        end
    endtask

    task automatic send_pixels(input integer count, input bit gaps);
        integer i;
        begin
            @(negedge clk);
            for (i = 0; i < count; i = i + 1) begin
                if (gaps && i % 19 == 0) repeat (3) @(negedge clk);
                in_data = source_data(i, case_id);
                in_user = (i % 256 == 0);
                in_last = (i % 16 == 15);
                in_valid = 1;
                do @(posedge clk); while (!in_ready);
                @(negedge clk);
                in_valid = 0;
            end
        end
    endtask

    task automatic wait_outputs(input integer count);
        begin
            while (output_count < count) @(negedge clk);
            repeat (12) @(negedge clk);
            if (output_count != count || input_count != count)
                $fatal(1, "Count mismatch: in=%0d out=%0d expected=%0d",
                       input_count, output_count, count);
        end
    endtask

    initial begin
        reset_dut(1);
        fork
            send_pixels(512, 0);
            configure(30);
        join
        wait_outputs(512);
        if (first_next_input_cycle != last_input_cycle + 1 ||
            first_next_output_cycle != last_output_cycle + 1)
            $fatal(1, "Frame boundary bubble: input %0d->%0d output %0d->%0d",
                   last_input_cycle, first_next_input_cycle,
                   last_output_cycle, first_next_output_cycle);

        if ($test$plusargs("two_frames_only")) begin
            $display("PASS: two 16x16 BLC frames, 512 beats, continuous frame boundary");
            $finish;
        end

        reset_dut(2);
        black_r = 11; black_gr = 13; black_gb = 17; black_b = 19;
        fork
            send_pixels(23, 1);
            configure(20);
        join
        wait_outputs(23);

        reset_dut(3); // Abort the incomplete frame, then require a fresh SOF.
        fork
            send_pixels(256, 1);
            configure(20);
            begin
                forever begin
                    @(negedge clk);
                    out_ready = (cycle % 7 < 4);
                end
            end
        join_any
        wait_outputs(256);
        $display("PASS: ordered AXI-Lite config, 512 zero-gap beats, stalls, reset, and 256 new-frame beats");
        $finish;
    end
endmodule
