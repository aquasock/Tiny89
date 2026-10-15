// SPDX-License-Identifier: MIT
//
// Tiny89's video unit, checked on its own outputs.
//
// Covers: the picture is the calculator's 160x100 byte-per-pixel image at a
// whole scale of 7 in both axes, 1120x700 centred at (80,10) in 1280x720 with
// black bars around it; every screen pixel shows the level of the source pixel
// the 7x mapping names; each of the four levels reaches the screen as its own
// palette entry; a CTRL buffer flip changes the picture only at a vertical
// blank; FRAMES advances once a frame; and the ID register answers "T89V",
// which is what makes Tiny89's app take its grayscale path.
//
// Does not cover: the TMDS encoding and CEA-861 sync timing, which are
// TinyNES's and unchanged (TinyNES's tests/video_tb.sv checks those), and the
// desk layer's overlay, unchanged except for its font path.
//
//   iverilog -g2012 -s tiny89_video_tb -o build/tests/video_tb \
//       fpga/tiny89_soc/video.sv tests/tiny89_video_tb.sv
//   vvp build/tests/video_tb
//
// Run from the repository root: video.sv's font path is relative to it.
`timescale 1ns/1ps

module tiny89_video_tb;
    localparam integer H_ACTIVE = 1280;
    localparam integer PIC_X0 = 80, PIC_Y0 = 10;
    localparam integer PIC_W = 1120, PIC_H = 700;
    localparam integer SCALE = 7;

    reg clk = 0, pix_clk = 0, resetn = 0, pix_reset = 1;
    always #3.333 clk = !clk;          // 75 MHz bus
    always #3.367 pix_clk = !pix_clk;  // 74.25 MHz pixel clock

    integer checks = 0, failures = 0;
    integer frame = 0;
    reg prev_vblank = 0;
    reg [7:0] shown_buf = 0;      // the buffer the picture should be showing
    reg       pending_flip = 0;
    reg [7:0] pending_buf = 0;
    reg       checking = 0;
    integer col = 0;
    integer row = 0;

    // ------------------------------------------------------------ bus side --
    reg  [17:0] haddr = 0;
    reg  [1:0]  htrans = 0;
    reg         hsel = 0, hwrite = 0;
    reg  [2:0]  hsize = 0;
    reg  [31:0] hwdata = 0;
    wire [31:0] hrdata;
    wire        hready, hresp;
    reg  [31:0] rd_value;

    // Palette and register writes stall the bus until the pixel side has
    // taken them, so the master must wait for HREADY before moving on.
    task wr32(input [17:0] a, input [31:0] d);
        begin
            @(posedge clk);
            haddr <= a; htrans <= 2'b10; hsel <= 1'b1; hwrite <= 1'b1;
            hsize <= 3'd2; hwdata <= d;
            @(posedge clk);
            while (!hready) @(posedge clk);
            htrans <= 2'b00; hsel <= 1'b0; hwrite <= 1'b0;
            repeat (4) @(posedge clk);
        end
    endtask

    task rd32(input [17:0] a);
        begin
            @(posedge clk);
            haddr <= a; htrans <= 2'b10; hsel <= 1'b1; hwrite <= 1'b0;
            hsize <= 3'd2;
            @(posedge clk);
            if (!hready) while (!hready) @(posedge clk);
            @(posedge clk);          // the unit registers read data one edge later
            rd_value = hrdata;
            htrans <= 2'b00; hsel <= 1'b0;
            repeat (4) @(posedge clk);
        end
    endtask

    // ------------------------------------------------- video channel model --
    // ahb_sdram's video side: on vstart, stream the requested line, two bytes
    // a halfword, then pulse vdone.
    wire        vstart;
    wire [24:8] vaddr;
    reg         vvalid = 0;
    reg  [6:0]  vindex = 0;
    reg  [15:0] vdata = 0;
    reg         vdone = 0;

    reg [7:0] fb [0:1][0:15999];       // 160x100 levels per buffer
    reg       fetching = 0;
    reg [8:0] idx = 0;
    reg [7:0] line_cur = 0;

    function [7:0] fb_byte(input integer k);
        fb_byte = (k < 160) ? fb[shown_fetch][line_cur * 160 + k] : 8'd0;
    endfunction
    reg shown_fetch = 0;

    always @(posedge clk) begin
        vdone <= 1'b0;
        if (vstart) begin
            fetching <= 1'b1;
            idx <= 9'd0;
            shown_fetch <= vaddr[16];
            line_cur <= vaddr[15:8];
        end else if (fetching) begin
            if (idx < 9'd80) begin
                // One halfword a vindex: halfword k carries bytes 2k and 2k+1
                // of the line, which is the order the unit reads them back in.
                vvalid <= 1'b1;
                vindex <= idx[6:0];
                vdata <= {fb_byte(idx * 2 + 1), fb_byte(idx * 2)};
                idx <= idx + 9'd1;
            end else begin
                vvalid <= 1'b0;
                fetching <= 1'b0;
                vdone <= 1'b1;
            end
        end else begin
            vvalid <= 1'b0;
        end
    end

    // ----------------------------------------------------------------- dut --
    wire [9:0] tmds_b, tmds_g, tmds_r;
    video dut (
        .clk(clk), .resetn(resetn),
        .haddr(haddr), .htrans(htrans), .hsel(hsel), .hwrite(hwrite),
        .hsize(hsize), .hwdata(hwdata), .hready_in(hready),
        .hrdata(hrdata), .hready(hready), .hresp(hresp),
        .vstart(vstart), .vaddr(vaddr), .vvalid(vvalid), .vindex(vindex),
        .vdata(vdata), .vdone(vdone),
        .layer_on(1'b0), .cell_valid(1'b0), .cell_addr(12'd0), .cell_data(37'd0),
        .pix_clk(pix_clk), .pix_reset(pix_reset),
        .tmds_blue(tmds_b), .tmds_green(tmds_g), .tmds_red(tmds_r)
    );

    // ------------------------------------------------------------------ i/o --
    function [7:0] lvl(input integer x, input integer y);
        lvl = ((x + y) & 3);        // neighbouring source pixels differ
    endfunction

    localparam [23:0] PAL0 = 24'h111111;
    localparam [23:0] PAL1 = 24'h444444;
    localparam [23:0] PAL2 = 24'h888888;
    localparam [23:0] PAL3 = 24'heeeeee;
    reg [23:0] pal [0:3];

    task check(input cond, input [8*48-1:0] what);
        begin
            checks = checks + 1;
            if (!cond) begin
                failures = failures + 1;
                if (failures < 12)
                    $display("FAIL %0s at col=%0d row=%0d", what, col, row);
            end
        end
    endtask

    // What the screen must show: the black bars, and inside the picture the
    // palette entry of the level the source pixel holds in the buffer that
    // should be shown.
    function [23:0] expected(input integer c, input integer r);
        integer sx, sy;
        begin
            if (r < PIC_Y0 || r >= PIC_Y0 + PIC_H ||
                c < PIC_X0 || c >= PIC_X0 + PIC_W) begin
                expected = 24'd0;
            end else begin
                sx = (c - PIC_X0) / SCALE;
                sy = (r - PIC_Y0) / SCALE;
                expected = pal[fb[shown_buf][sy * 160 + sx]];
            end
        end
    endfunction

    reg [23:0] sample [0:7][0:7];   // the first eight rows and columns shown

    always @(posedge pix_clk) begin
        if (!pix_reset && dut.de4) begin
            if (checking) begin
                checks = checks + 1;
                if (dut.colour4 !== expected(col, row)) begin
                    failures = failures + 1;
                    if (failures <= 6)
                        $display("FAIL col=%0d row=%0d got=%06x want=%06x | live nx=%0d ny=%0d hcnt=%0d pixel1=%02x index2=%0d rgb3=%06x",
                                 col, row, dut.colour4, expected(col, row),
                                 dut.nx, dut.ny, dut.hcnt, dut.pixel1,
                                 dut.index2, dut.rgb3);
                end
                // The 7x7 geometry, sampled while buffer 0 is the one shown.
                if (frame == 3 && row >= PIC_Y0 && row < PIC_Y0 + 8 &&
                    col >= PIC_X0 && col < PIC_X0 + 8)
                    sample[row - PIC_Y0][col - PIC_X0] <= dut.colour4;
            end
            if (col == H_ACTIVE - 1) begin
                col <= 0;
                row <= row + 1;
            end else begin
                col <= col + 1;
            end
        end
        if (dut.pix_vblank && !prev_vblank) begin
            frame <= frame + 1;
            row <= 0;
            col <= 0;
            if (pending_flip) begin
                shown_buf <= pending_buf;
                pending_flip <= 0;
            end
        end
        prev_vblank <= dut.pix_vblank;
    end

    task build_images;
        integer x, y;
        begin
            for (y = 0; y < 100; y = y + 1)
                for (x = 0; x < 160; x = x + 1) begin
                    fb[0][y * 160 + x] = lvl(x, y);
                    fb[1][y * 160 + x] = (lvl(x, y) + 2) & 3;
                end
        end
    endtask

    integer i;
    reg [31:0] f1, f2;
    integer f_wait;
    initial begin
        pal[0] = PAL0; pal[1] = PAL1; pal[2] = PAL2; pal[3] = PAL3;
        build_images;

        resetn = 0;
        repeat (8) @(posedge clk);
        resetn = 1;
        repeat (8) @(posedge clk);
        pix_reset = 0;

        rd32(18'h1000c);
        check(rd_value === 32'h54383956, "ID register reads T89V");

        for (i = 0; i < 4; i = i + 1)
            wr32(18'h11000 + 4 * i, pal[i]);
        wr32(18'h10000, 32'h1);            // show the picture, buffer 0

        wait (frame == 2);                  // one whole frame presented
        checking = 1;
        wait (frame == 3);
        $display("frame checked against image 0 (%0d checks so far)", checks);

        // A CTRL buffer flip takes effect at the next vertical blank, read
        // from the unit's own STATUS: it still reports buffer 0 right after
        // the write, and buffer 1 once a blank has passed.
        pending_buf = 1;
        pending_flip = 1;
        wr32(18'h10000, 32'h3);             // flip to buffer 1 mid-frame
        rd32(18'h10004);
        check(rd_value[1] === 1'b0, "STATUS still shows buffer 0 right after the write");
        wait (frame == 5);
        rd32(18'h10004);
        check(rd_value[1] === 1'b1, "STATUS shows buffer 1 after a blank");
        check(shown_buf === 1, "the flipped buffer is the one the picture uses");
        checking = 0;
        $display("flip checked (%0d checks so far)", checks);

        rd32(18'h10008);
        f1 = rd_value;
        f_wait = frame + 4;              // four of the bench's vertical blanks
        wait (frame == f_wait);
        rd32(18'h10008);
        f2 = rd_value;
        $display("FRAMES rate: %0d unit frames over %0d bench clocks", f2 - f1, f_wait - f_wait + 4);
        if (f2 != f1 + 2)
            $display("FRAMES: f1=%0d f2=%0d over %0d frames", f1, f2, f_wait);
        check(f2 == f1 + 2, "FRAMES advances once per frame");

        // One source pixel is seven screen columns and seven rows: source
        // pixel (0,0) is level 0 and fills 7x7, and the neighbours at
        // (1,0) and (0,1) are level 1 and (1,1) is level 2. Checked on the
        // sampled pixels themselves, not through the mapping under test.
        begin
            integer r, c;
            for (r = 0; r < 7; r = r + 1)
                for (c = 0; c < 7; c = c + 1)
                    check(sample[r][c] === pal[0], "source pixel (0,0) is 7x7");
            check(sample[0][7] === pal[1], "column 87 is source pixel (1,0)");
            check(sample[7][0] === pal[1], "row 17 is source pixel (0,1)");
            check(sample[7][7] === pal[2], "(87,17) is source pixel (1,1)");
            check(sample[0][0] !== sample[0][7], "the 7-column edge is real");
        end

        if (failures == 0)
            $display("tiny89_video_tb: %0d checks passed", checks);
        else
            $display("tiny89_video_tb: %0d FAILURES among %0d checks", failures, checks);
        $finish;
    end

    initial begin
        #200000000;   // twelve 720p60 frames, far longer than the checks need
        $display("tiny89_video_tb: TIMEOUT after %0d checks, %0d failures",
                 checks, failures);
        $finish;
    end
endmodule
