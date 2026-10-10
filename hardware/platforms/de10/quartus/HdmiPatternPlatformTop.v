module HdmiPatternPlatformTop(
    input  wire        clk,
    output wire [14:0] hps_memory_mem_a,
    output wire [2:0]  hps_memory_mem_ba,
    output wire        hps_memory_mem_ck,
    output wire        hps_memory_mem_ck_n,
    output wire        hps_memory_mem_cke,
    output wire        hps_memory_mem_cs_n,
    output wire        hps_memory_mem_ras_n,
    output wire        hps_memory_mem_cas_n,
    output wire        hps_memory_mem_we_n,
    output wire        hps_memory_mem_reset_n,
    inout  wire [31:0] hps_memory_mem_dq,
    inout  wire [3:0]  hps_memory_mem_dqs,
    inout  wire [3:0]  hps_memory_mem_dqs_n,
    output wire        hps_memory_mem_odt,
    output wire [3:0]  hps_memory_mem_dm,
    input  wire        hps_memory_oct_rzqin,
    output wire [23:0] HDMI_TX_D,
    output wire        HDMI_TX_CLK,
    output wire        HDMI_TX_DE,
    output wire        HDMI_TX_HS,
    output wire        HDMI_TX_VS,
    input  wire        HDMI_TX_INT,
    output wire        HDMI_I2S0,
    output wire        HDMI_MCLK,
    output wire        HDMI_LRCLK,
    output wire        HDMI_SCLK,
    output wire [7:0]  LED,
    inout  wire        I2C_SCL,
    inout  wire        I2C_SDA
);
    localparam [31:0] DDR_FRAMEBUFFER_BASE = 32'h3f00_0000;
    localparam LANE_FIFO_AW = 10;

    assign HDMI_I2S0  = 1'bz;
    assign HDMI_MCLK  = 1'bz;
    assign HDMI_LRCLK = 1'bz;
    assign HDMI_SCLK  = 1'bz;

    wire pix_clk;
    wire hdmi_pll_locked;

    de10_hdmi_pll hdmi_pll (
        .refclk   (clk),
        .rst      (1'b0),
        .outclk_0 (pix_clk),
        .locked   (hdmi_pll_locked)
    );

    altddio_out #(
        .extend_oe_disable("OFF"),
        .intended_device_family("Cyclone V"),
        .invert_output("OFF"),
        .lpm_hint("UNUSED"),
        .lpm_type("altddio_out"),
        .oe_reg("UNREGISTERED"),
        .power_up_high("OFF"),
        .width(1)
    ) hdmi_clock_forward (
        .datain_h(1'b0),
        .datain_l(1'b1),
        .outclock(pix_clk),
        .dataout(HDMI_TX_CLK),
        .aclr(1'b0),
        .aset(1'b0),
        .oe(1'b1),
        .outclocken(1'b1),
        .sclr(1'b0),
        .sset(1'b0)
    );

    wire        fb_color_waitrequest;
    wire [31:0] fb_color_readdata;
    wire        fb_color_readdatavalid;
    wire        scan_read_req_valid;
    wire        scan_read_req_ready;
    wire [25:0] scan_read_req_address;
    wire        scan_read_rsp_valid;
    wire        scan_read_rsp_ready;
    wire [15:0] scan_read_rsp_data;
    wire [7:0]  scan_r;
    wire [7:0]  scan_g;
    wire [7:0]  scan_b;
    wire        scan_de;
    wire        scan_hs;
    wire        scan_vs;
    wire        scan_active;
    wire        scan_new_frame;
    wire [9:0]  scan_x;
    wire [9:0]  scan_y;
    wire [12:0] fifo_push_level;
    wire [12:0] fifo_pop_level;
    wire        fifo_underflow;

    reg [LANE_FIFO_AW:0] lane_count = {(LANE_FIFO_AW+1){1'b0}};
    reg [LANE_FIFO_AW-1:0] lane_wr_ptr = {LANE_FIFO_AW{1'b0}};
    reg [LANE_FIFO_AW-1:0] lane_rd_ptr = {LANE_FIFO_AW{1'b0}};
    reg lane_fifo [0:(1 << LANE_FIFO_AW)-1];

    wire lane_fifo_full = lane_count == (1 << LANE_FIFO_AW);
    wire ddr_read_accept = scan_read_req_valid && scan_read_req_ready;
    wire ddr_response = fb_color_readdatavalid;
    wire req_lane = scan_read_req_address[1];
    wire rsp_lane = lane_fifo[lane_rd_ptr];
    wire [31:0] ddr_read_address = DDR_FRAMEBUFFER_BASE + {4'd0, scan_read_req_address[25:2], 2'b00};

    assign scan_read_req_ready = !fb_color_waitrequest && !lane_fifo_full;
    assign scan_read_rsp_valid = fb_color_readdatavalid;
    assign scan_read_rsp_data = rsp_lane ? fb_color_readdata[31:16] : fb_color_readdata[15:0];

    always @(posedge clk) begin
        if (!hdmi_pll_locked) begin
            lane_count <= {(LANE_FIFO_AW+1){1'b0}};
            lane_wr_ptr <= {LANE_FIFO_AW{1'b0}};
            lane_rd_ptr <= {LANE_FIFO_AW{1'b0}};
        end else begin
            if (ddr_read_accept) begin
                lane_fifo[lane_wr_ptr] <= req_lane;
                lane_wr_ptr <= lane_wr_ptr + {{(LANE_FIFO_AW-1){1'b0}}, 1'b1};
            end
            if (ddr_response) begin
                lane_rd_ptr <= lane_rd_ptr + {{(LANE_FIFO_AW-1){1'b0}}, 1'b1};
            end
            lane_count <= lane_count + (ddr_read_accept ? {{LANE_FIFO_AW{1'b0}}, 1'b1} : {(LANE_FIFO_AW+1){1'b0}})
                                   - (ddr_response ? {{LANE_FIFO_AW{1'b0}}, 1'b1} : {(LANE_FIFO_AW+1){1'b0}});
        end
    end

    HdmiCdcFramebufferScanout scanout (
        .io_hdmiClock                (pix_clk),
        .io_hdmiReset                (!hdmi_pll_locked),
        .io_regs_frontBase           (26'd0),
        .io_regs_backBase            (26'd0),
        .io_regs_pixelStride         (11'd768),
        .io_regs_framebufferEnable   (1'b1),
        .io_regs_testPatternEnable   (1'b0),
        .io_readReq_valid            (scan_read_req_valid),
        .io_readReq_ready            (scan_read_req_ready),
        .io_readReq_payload_address  (scan_read_req_address),
        .io_readRsp_valid            (scan_read_rsp_valid),
        .io_readRsp_ready            (scan_read_rsp_ready),
        .io_readRsp_payload_data     (scan_read_rsp_data),
        .io_video_rgb_r              (scan_r),
        .io_video_rgb_g              (scan_g),
        .io_video_rgb_b              (scan_b),
        .io_video_de                 (scan_de),
        .io_video_hSync              (scan_hs),
        .io_video_vSync              (scan_vs),
        .io_status_displayedBuffer   (),
        .io_status_displayedBase     (),
        .io_status_swapPending       (),
        .io_status_swapCount         (),
        .io_status_newFrame          (scan_new_frame),
        .io_status_active            (scan_active),
        .io_status_x                 (scan_x),
        .io_status_y                 (scan_y),
        .io_fifoPushOccupancy        (fifo_push_level),
        .io_fifoPopOccupancy         (fifo_pop_level),
        .io_underflow                (fifo_underflow),
        .clk                         (clk),
        .reset                       (!hdmi_pll_locked)
    );

    assign HDMI_TX_D  = {scan_r, scan_g, scan_b};
    assign HDMI_TX_DE = scan_de;
    assign HDMI_TX_HS = scan_hs;
    assign HDMI_TX_VS = scan_vs;

    de10_soc soc_0 (
        .clk_clk                         (clk),
        .reset_reset_n                   (hdmi_pll_locked),
        .memory_mem_a                    (hps_memory_mem_a),
        .memory_mem_ba                   (hps_memory_mem_ba),
        .memory_mem_ck                   (hps_memory_mem_ck),
        .memory_mem_ck_n                 (hps_memory_mem_ck_n),
        .memory_mem_cke                  (hps_memory_mem_cke),
        .memory_mem_cs_n                 (hps_memory_mem_cs_n),
        .memory_mem_ras_n                (hps_memory_mem_ras_n),
        .memory_mem_cas_n                (hps_memory_mem_cas_n),
        .memory_mem_we_n                 (hps_memory_mem_we_n),
        .memory_mem_reset_n              (hps_memory_mem_reset_n),
        .memory_mem_dq                   (hps_memory_mem_dq),
        .memory_mem_dqs                  (hps_memory_mem_dqs),
        .memory_mem_dqs_n                (hps_memory_mem_dqs_n),
        .memory_mem_odt                  (hps_memory_mem_odt),
        .memory_mem_dm                   (hps_memory_mem_dm),
        .memory_oct_rzqin                (hps_memory_oct_rzqin),
        .fb_color_read_mem_waitrequest   (fb_color_waitrequest),
        .fb_color_read_mem_readdata      (fb_color_readdata),
        .fb_color_read_mem_readdatavalid (fb_color_readdatavalid),
        .fb_color_read_mem_burstcount    (11'd1),
        .fb_color_read_mem_writedata     (32'd0),
        .fb_color_read_mem_address       (ddr_read_address),
        .fb_color_read_mem_write         (1'b0),
        .fb_color_read_mem_read          (ddr_read_accept),
        .fb_color_read_mem_byteenable    (4'hf),
        .fb_color_read_mem_debugaccess   (1'b0),
        .fb_write_mem_burstcount         (11'd1),
        .fb_write_mem_writedata          (32'd0),
        .fb_write_mem_address            (32'd0),
        .fb_write_mem_write              (1'b0),
        .fb_write_mem_read               (1'b0),
        .fb_write_mem_byteenable         (4'd0),
        .fb_write_mem_debugaccess        (1'b0),
        .fb_write_mem_waitrequest        (),
        .fb_write_mem_readdata           (),
        .fb_write_mem_readdatavalid      (),
        .fb_aux_read_mem_burstcount      (11'd1),
        .fb_aux_read_mem_writedata       (32'd0),
        .fb_aux_read_mem_address         (32'd0),
        .fb_aux_read_mem_write           (1'b0),
        .fb_aux_read_mem_read            (1'b0),
        .fb_aux_read_mem_byteenable      (4'd0),
        .fb_aux_read_mem_debugaccess     (1'b0),
        .fb_aux_read_mem_waitrequest     (),
        .fb_aux_read_mem_readdata        (),
        .fb_aux_read_mem_readdatavalid   (),
        .tex_mem_burstcount              (11'd1),
        .tex_mem_writedata               (32'd0),
        .tex_mem_address                 (32'd0),
        .tex_mem_write                   (1'b0),
        .tex_mem_read                    (1'b0),
        .tex_mem_byteenable              (4'd0),
        .tex_mem_debugaccess             (1'b0),
        .tex_mem_waitrequest             (),
        .tex_mem_readdata                (),
        .tex_mem_readdatavalid           (),
        .h2f_avalon_waitrequest          (1'b0),
        .h2f_avalon_readdata             (32'd0),
        .h2f_avalon_readdatavalid        (1'b0),
        .h2f_avalon_burstcount           (),
        .h2f_avalon_writedata            (),
        .h2f_avalon_address              (),
        .h2f_avalon_write                (),
        .h2f_avalon_read                 (),
        .h2f_avalon_byteenable           (),
        .h2f_avalon_debugaccess          (),
        .h2f_mpu_events_eventi           (1'b0),
        .h2f_mpu_events_evento           (),
        .h2f_mpu_events_standbywfe       (),
        .h2f_mpu_events_standbywfi       (),
        .h2f_reset_reset_n               ()
    );

    wire hdmi_scl_en;
    wire hdmi_sda_en;

    assign I2C_SCL = hdmi_scl_en ? 1'b0 : 1'bz;
    assign I2C_SDA = hdmi_sda_en ? 1'b0 : 1'bz;

    cyclonev_hps_interface_peripheral_i2c hdmi_i2c (
        .out_clk(hdmi_scl_en),
        .scl(I2C_SCL),
        .out_data(hdmi_sda_en),
        .sda(I2C_SDA)
    );

    assign LED[0] = hdmi_pll_locked;
    assign LED[1] = scan_new_frame;
    assign LED[2] = scan_de;
    assign LED[3] = fifo_underflow;
    assign LED[4] = I2C_SCL;
    assign LED[5] = I2C_SDA;
    assign LED[6] = fifo_push_level[11];
    assign LED[7] = scan_active;
endmodule
