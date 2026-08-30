`timescale 1ns/1ps

module De10PlatformTop (
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

  wire        h2f_waitrequest;
  wire [31:0] h2f_readdata;
  wire        h2f_readdatavalid;
  wire [0:0]  h2f_burstcount;
  wire [31:0] h2f_writedata;
  wire [23:0] h2f_address;
  wire        h2f_write;
  wire        h2f_read;
  wire [3:0]  h2f_byteenable;
  wire        h2f_reset_n;
  wire        core_reset;
  wire [21:0] core_h2f_address;
  wire        core_h2f_read;
  wire        core_h2f_write;
  wire [3:0]  core_h2f_byteenable;
  wire [31:0] core_h2f_writedata;
  wire        core_h2f_waitrequest;
  wire [31:0] core_h2f_readdata;
  wire        core_h2f_readdatavalid;

  wire        fb_write_waitrequest;
  wire [31:0] fb_write_readdata;
  wire        fb_write_readdatavalid;
  wire [10:0] fb_write_burstcount;
  wire [31:0] fb_write_writedata;
  wire [31:0] fb_write_address;
  wire        fb_write_write;
  wire        fb_write_read;
  wire [3:0]  fb_write_byteenable;
  wire        core_fb_waitrequest;
  wire        core_fb_waitrequestn;
  wire [31:0] core_fb_readdata;
  wire        core_fb_readdatavalid;
  wire [10:0] core_fb_burstcount;
  wire [31:0] core_fb_writedata;
  wire [31:0] core_fb_address;
  wire        core_fb_write;
  wire        core_fb_read;
  wire [3:0]  core_fb_byteenable;
  wire        fifo_mem_waitrequestn;
  wire        fifo_mem_readdatavalid;
  wire [31:0] fifo_mem_readdata;
  wire [10:0] fifo_mem_burstcount;
  wire [31:0] fifo_mem_writedata;
  wire [31:0] fifo_mem_address;
  wire        fifo_mem_write;
  wire        fifo_mem_read;
  wire [3:0]  fifo_mem_byteenable;

  wire        fb_color_waitrequest;
  wire [31:0] fb_color_readdata;
  wire        fb_color_readdatavalid;
  wire [10:0] fb_color_burstcount;
  wire [31:0] fb_color_writedata;
  wire [31:0] fb_color_address;
  wire        fb_color_write;
  wire        fb_color_read;
  wire [3:0]  fb_color_byteenable;

  wire        fb_color_wr_waitrequest;
   wire [31:0] fb_color_wr_readdata;
   wire        fb_color_wr_readdatavalid;
   wire [10:0] fb_color_wr_burstcount;
   wire [31:0] fb_color_wr_writedata;
   wire [31:0] fb_color_wr_address;
   wire        fb_color_wr_write;
   wire        fb_color_wr_read;
   wire [3:0]  fb_color_wr_byteenable;

  wire        fb_aux_waitrequest;
  wire [31:0] fb_aux_readdata;
  wire        fb_aux_readdatavalid;
  wire [10:0] fb_aux_burstcount;
  wire [31:0] fb_aux_writedata;
  wire [31:0] fb_aux_address;
  wire        fb_aux_write;
  wire        fb_aux_read;
  wire [3:0]  fb_aux_byteenable;

  wire        fb_aux_wr_waitrequest;
   wire [31:0] fb_aux_wr_readdata;
   wire        fb_aux_wr_readdatavalid;
   wire [10:0] fb_aux_wr_burstcount;
   wire [31:0] fb_aux_wr_writedata;
   wire [31:0] fb_aux_wr_address;
   wire        fb_aux_wr_write;
   wire        fb_aux_wr_read;
   wire [3:0]  fb_aux_wr_byteenable;

  wire        scanout_waitrequest;
  wire [31:0] scanout_readdata;
  wire        scanout_readdatavalid;
  wire [10:0] scanout_burstcount;
  wire [31:0] scanout_writedata;
  wire [31:0] scanout_address;
  wire        scanout_write;
  wire        scanout_read;
  wire [3:0]  scanout_byteenable;

  wire        tex_waitrequest;
  wire [31:0] tex_readdata;
  wire        tex_readdatavalid;
  wire [10:0] tex_burstcount;
  wire [31:0] tex_writedata;
  wire [31:0] tex_address;
  wire        tex_write;
  wire        tex_read;
  wire [3:0]  tex_byteenable;

   wire        core_clk;
   wire        h2f_clk;
   wire        h2f_fifo_reset;
   wire        hdmi_clk;
  wire        hdmi_pll_locked;
  wire        pll_locked;
  wire [7:0]  hdmi_rgb_r;
  wire [7:0]  hdmi_rgb_g;
  wire [7:0]  hdmi_rgb_b;
  wire        hdmi_de;
  wire        hdmi_hs;
  wire        hdmi_vs;
  wire [0:0]  hdmi_status_displayed_buffer;
  wire [25:0] hdmi_status_displayed_base;
  wire        hdmi_status_swap_pending;
  wire [1:0]  hdmi_status_swap_count;
  wire        hdmi_status_new_frame;
  wire        hdmi_status_active;
  wire [9:0]  hdmi_status_x;
  wire [9:0]  hdmi_status_y;
  wire        hdmi_underflow;
  wire [12:0] hdmi_fifo_level;
  wire        hdmi_scl_en;
  wire        hdmi_sda_en;


  assign HDMI_I2S0 = 1'bz;
  assign HDMI_MCLK = 1'bz;
  assign HDMI_LRCLK = 1'bz;
  assign HDMI_SCLK = 1'bz;

  de10_core_pll core_pll_0 (
     .refclk   (clk),
     .rst      (1'b0),
     .outclk_0 (core_clk),
     .outclk_1 (h2f_clk),
     .locked   (pll_locked)
  );

  de10_hdmi_pll hdmi_pll_0 (
    .refclk   (clk),
    .rst      (1'b0),
    .outclk_0 (hdmi_clk),
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
    .outclock(hdmi_clk),
    .dataout(HDMI_TX_CLK),
    .aclr(1'b0),
    .aset(1'b0),
    .oe(1'b1),
    .outclocken(1'b1),
    .sclr(1'b0),
    .sset(1'b0)
  );

  assign HDMI_TX_D = {hdmi_rgb_r, hdmi_rgb_g, hdmi_rgb_b};
  assign HDMI_TX_DE = hdmi_de;
  assign HDMI_TX_HS = hdmi_hs;
  assign HDMI_TX_VS = hdmi_vs;

  assign I2C_SCL = hdmi_scl_en ? 1'b0 : 1'bz;
  assign I2C_SDA = hdmi_sda_en ? 1'b0 : 1'bz;

  cyclonev_hps_interface_peripheral_i2c hdmi_i2c (
    .out_clk  (hdmi_scl_en),
    .scl      (I2C_SCL),
    .out_data (hdmi_sda_en),
    .sda      (I2C_SDA)
  );

  assign LED[0] = pll_locked & hdmi_pll_locked;
  assign LED[1] = hdmi_status_new_frame;
  assign LED[2] = hdmi_de;
  assign LED[3] = hdmi_underflow;
  assign LED[4] = I2C_SCL;
  assign LED[5] = I2C_SDA;
  assign LED[6] = hdmi_fifo_level[11];
  assign LED[7] = hdmi_status_active;




   de10_soc soc_0 (
     .clk_clk                     (core_clk),
     .h2f_clock_clk               (h2f_clk),
     .h2f_clock_reset_reset_n     (pll_locked),
    .fb_write_mem_waitrequest    (fb_write_waitrequest),
    .fb_write_mem_readdata       (fb_write_readdata),
    .fb_write_mem_readdatavalid  (fb_write_readdatavalid),
    .fb_write_mem_burstcount     (fb_write_burstcount),
    .fb_write_mem_writedata      (fb_write_writedata),
    .fb_write_mem_address        (fb_write_address),
    .fb_write_mem_write          (fb_write_write),
    .fb_write_mem_read           (fb_write_read),
    .fb_write_mem_byteenable     (fb_write_byteenable),
    .fb_write_mem_debugaccess    (1'b0),
     .fb_color_read_mem_waitrequest   (fb_color_waitrequest),
     .fb_color_read_mem_readdata      (fb_color_readdata),
     .fb_color_read_mem_readdatavalid (fb_color_readdatavalid),
     .fb_color_read_mem_burstcount    (fb_color_burstcount),
     .fb_color_read_mem_writedata     (fb_color_writedata),
     .fb_color_read_mem_address       (fb_color_address),
     .fb_color_read_mem_write         (fb_color_write),
     .fb_color_read_mem_read          (fb_color_read),
     .fb_color_read_mem_byteenable    (fb_color_byteenable),
    .fb_color_read_mem_debugaccess   (1'b0),
     .fb_color_write_mem_waitrequest   (fb_color_wr_waitrequest),
     .fb_color_write_mem_readdata      (fb_color_wr_readdata),
     .fb_color_write_mem_readdatavalid (fb_color_wr_readdatavalid),
     .fb_color_write_mem_burstcount    (fb_color_wr_burstcount),
     .fb_color_write_mem_writedata     (fb_color_wr_writedata),
     .fb_color_write_mem_address       (fb_color_wr_address),
     .fb_color_write_mem_write         (fb_color_wr_write),
     .fb_color_write_mem_read          (fb_color_wr_read),
     .fb_color_write_mem_byteenable    (fb_color_wr_byteenable),
    .fb_color_write_mem_debugaccess   (1'b0),
     .fb_aux_read_mem_waitrequest   (fb_aux_waitrequest),
     .fb_aux_read_mem_readdata      (fb_aux_readdata),
     .fb_aux_read_mem_readdatavalid (fb_aux_readdatavalid),
     .fb_aux_read_mem_burstcount    (fb_aux_burstcount),
     .fb_aux_read_mem_writedata     (fb_aux_writedata),
     .fb_aux_read_mem_address       (fb_aux_address),
     .fb_aux_read_mem_write         (fb_aux_write),
     .fb_aux_read_mem_read          (fb_aux_read),
     .fb_aux_read_mem_byteenable    (fb_aux_byteenable),
    .fb_aux_read_mem_debugaccess   (1'b0),
     .fb_aux_write_mem_waitrequest   (fb_aux_wr_waitrequest),
     .fb_aux_write_mem_readdata      (fb_aux_wr_readdata),
     .fb_aux_write_mem_readdatavalid (fb_aux_wr_readdatavalid),
     .fb_aux_write_mem_burstcount    (fb_aux_wr_burstcount),
     .fb_aux_write_mem_writedata     (fb_aux_wr_writedata),
     .fb_aux_write_mem_address       (fb_aux_wr_address),
     .fb_aux_write_mem_write         (fb_aux_wr_write),
     .fb_aux_write_mem_read          (fb_aux_wr_read),
     .fb_aux_write_mem_byteenable    (fb_aux_wr_byteenable),
    .fb_aux_write_mem_debugaccess   (1'b0),
    .scanout_mem_waitrequest       (scanout_waitrequest),
    .scanout_mem_readdata          (scanout_readdata),
    .scanout_mem_readdatavalid     (scanout_readdatavalid),
    .scanout_mem_burstcount        (scanout_burstcount),
    .scanout_mem_writedata         (scanout_writedata),
    .scanout_mem_address           (scanout_address),
    .scanout_mem_write             (scanout_write),
    .scanout_mem_read              (scanout_read),
    .scanout_mem_byteenable        (scanout_byteenable),
    .scanout_mem_debugaccess       (1'b0),
    .h2f_avalon_waitrequest      (h2f_waitrequest),
    .h2f_avalon_readdata         (h2f_readdata),
    .h2f_avalon_readdatavalid    (h2f_readdatavalid),
    .h2f_avalon_burstcount       (h2f_burstcount),
    .h2f_avalon_writedata        (h2f_writedata),
    .h2f_avalon_address          (h2f_address),
    .h2f_avalon_write            (h2f_write),
    .h2f_avalon_read             (h2f_read),
    .h2f_avalon_byteenable       (h2f_byteenable),
    .h2f_avalon_debugaccess      (),
    .h2f_mpu_events_eventi       (1'b0),
    .h2f_mpu_events_evento       (),
    .h2f_mpu_events_standbywfe   (),
    .h2f_mpu_events_standbywfi   (),
    .h2f_reset_reset_n           (h2f_reset_n),
    .memory_mem_a                (hps_memory_mem_a),
    .memory_mem_ba               (hps_memory_mem_ba),
    .memory_mem_ck               (hps_memory_mem_ck),
    .memory_mem_ck_n             (hps_memory_mem_ck_n),
    .memory_mem_cke              (hps_memory_mem_cke),
    .memory_mem_cs_n             (hps_memory_mem_cs_n),
    .memory_mem_ras_n            (hps_memory_mem_ras_n),
    .memory_mem_cas_n            (hps_memory_mem_cas_n),
    .memory_mem_we_n             (hps_memory_mem_we_n),
    .memory_mem_reset_n          (hps_memory_mem_reset_n),
    .memory_mem_dq               (hps_memory_mem_dq),
    .memory_mem_dqs              (hps_memory_mem_dqs),
    .memory_mem_dqs_n            (hps_memory_mem_dqs_n),
    .memory_mem_odt              (hps_memory_mem_odt),
    .memory_mem_dm               (hps_memory_mem_dm),
    .memory_oct_rzqin            (hps_memory_oct_rzqin),
    .reset_reset_n               (pll_locked),
    .tex_mem_waitrequest         (tex_waitrequest),
    .tex_mem_readdata            (tex_readdata),
    .tex_mem_readdatavalid       (tex_readdatavalid),
    .tex_mem_burstcount          (tex_burstcount),
    .tex_mem_writedata           (tex_writedata),
    .tex_mem_address             (tex_address),
    .tex_mem_write               (tex_write),
    .tex_mem_read                (tex_read),
    .tex_mem_byteenable          (tex_byteenable),
    .tex_mem_debugaccess         (1'b0)
  );

  reg pll_locked_meta = 1'b0;
  reg pll_locked_sync = 1'b0;
  reg h2f_reset_meta = 1'b0;
  reg h2f_reset_sync = 1'b0;
  // Keep the fabric-side masters idle long enough for Linux fpga-region to
  // finish re-enabling the HPS/FPGA bridges after configuration.  Without
  // this, the scanout/core can issue F2SDRAM reads while the bridge is still
  // disabled, which wedges the HPS on this board/kernel.
  reg [27:0] core_reset_release = 28'h0000000;

  always @(posedge core_clk or negedge pll_locked) begin
    if (!pll_locked) begin
      pll_locked_meta <= 1'b0;
      pll_locked_sync <= 1'b0;
      h2f_reset_meta <= 1'b0;
      h2f_reset_sync <= 1'b0;
      core_reset_release <= 28'h0000000;
    end else begin
      pll_locked_meta <= 1'b1;
      pll_locked_sync <= pll_locked_meta;
      h2f_reset_meta <= h2f_reset_n;
      h2f_reset_sync <= h2f_reset_meta;
      if (!h2f_reset_sync) begin
        core_reset_release <= 28'h0000000;
      end else if (!core_reset_release[27]) begin
        core_reset_release <= core_reset_release + 8'h01;
      end
    end
  end

  assign core_reset = ~pll_locked_sync | ~h2f_reset_sync | ~core_reset_release[27];

  reg h2f_fifo_reset_meta = 1'b1;
  reg h2f_fifo_reset_sync = 1'b1;
  always @(posedge h2f_clk or posedge core_reset) begin
    if (core_reset) begin
      h2f_fifo_reset_meta <= 1'b1;
      h2f_fifo_reset_sync <= 1'b1;
    end else begin
      h2f_fifo_reset_meta <= 1'b0;
      h2f_fifo_reset_sync <= h2f_fifo_reset_meta;
    end
  end
  assign h2f_fifo_reset = h2f_fifo_reset_sync;

`ifdef PURE_XLS_DE10
  XlsH2fCommandFifo #(
    .FIFO_ADDRESS_WIDTH(6),
    .MEMORY_FIFO_ENABLE(1)
  ) h2f_command_fifo (
    .h2f_clk(h2f_clk),
    .h2f_reset(h2f_fifo_reset),
    .h2f_address(h2f_address[23:2]),
    .h2f_read(h2f_read),
    .h2f_write(h2f_write),
    .h2f_byteenable(h2f_byteenable),
    .h2f_writedata(h2f_writedata),
    .h2f_waitrequest(h2f_waitrequest),
    .h2f_readdata(h2f_readdata),
    .h2f_readdatavalid(h2f_readdatavalid),
    .core_clk(core_clk),
    .core_reset(core_reset),
    .core_address(core_h2f_address),
    .core_read(core_h2f_read),
    .core_write(core_h2f_write),
    .core_byteenable(core_h2f_byteenable),
    .core_writedata(core_h2f_writedata),
    .core_waitrequest(core_h2f_waitrequest),
    .core_readdata(core_h2f_readdata),
    .core_readdatavalid(core_h2f_readdatavalid),
    .mem_read(fifo_mem_read),
    .mem_write(fifo_mem_write),
    .mem_waitRequestn(fifo_mem_waitrequestn),
    .mem_burstCount(fifo_mem_burstcount),
    .mem_address(fifo_mem_address),
    .mem_byteEnable(fifo_mem_byteenable),
    .mem_writeData(fifo_mem_writedata),
    .mem_readDataValid(fifo_mem_readdatavalid),
    .mem_readData(fifo_mem_readdata)
  );
`else
  assign core_h2f_address = h2f_address[23:2];
  assign core_h2f_read = h2f_read;
  assign core_h2f_write = h2f_write;
  assign core_h2f_byteenable = h2f_byteenable;
  assign core_h2f_writedata = h2f_writedata;
  assign h2f_waitrequest = core_h2f_waitrequest;
  assign h2f_readdata = core_h2f_readdata;
  assign h2f_readdatavalid = core_h2f_readdatavalid;
`endif

`ifdef PURE_XLS_DE10
  XlsAvalonArbiter2 memory_fifo_arbiter (
    .clk(core_clk), .reset(core_reset),
    .m0_read(core_fb_read), .m0_write(core_fb_write),
    .m0_waitRequestn(core_fb_waitrequestn), .m0_burstCount(core_fb_burstcount),
    .m0_address(core_fb_address), .m0_byteEnable(core_fb_byteenable),
    .m0_writeData(core_fb_writedata), .m0_readDataValid(core_fb_readdatavalid),
    .m0_readData(core_fb_readdata),
    .m1_read(fifo_mem_read), .m1_write(fifo_mem_write),
    .m1_waitRequestn(fifo_mem_waitrequestn), .m1_burstCount(fifo_mem_burstcount),
    .m1_address(fifo_mem_address), .m1_byteEnable(fifo_mem_byteenable),
    .m1_writeData(fifo_mem_writedata), .m1_readDataValid(fifo_mem_readdatavalid),
    .m1_readData(fifo_mem_readdata),
    .s_read(fb_write_read), .s_write(fb_write_write),
    .s_waitRequestn(~fb_write_waitrequest), .s_burstCount(fb_write_burstcount),
    .s_address(fb_write_address), .s_byteEnable(fb_write_byteenable),
    .s_writeData(fb_write_writedata), .s_readDataValid(fb_write_readdatavalid),
    .s_readData(fb_write_readdata)
  );
  assign core_fb_waitrequest = (core_fb_read || core_fb_write) && !core_fb_waitrequestn;
`else
  assign fb_write_read = core_fb_read;
  assign fb_write_write = core_fb_write;
  assign fb_write_burstcount = core_fb_burstcount;
  assign fb_write_address = core_fb_address;
  assign fb_write_byteenable = core_fb_byteenable;
  assign fb_write_writedata = core_fb_writedata;
  assign core_fb_waitrequest = fb_write_waitrequest;
  assign core_fb_readdatavalid = fb_write_readdatavalid;
  assign core_fb_readdata = fb_write_readdata;
`endif

`ifdef PURE_XLS_DE10
  XlsDe10Top core_0 (
`else
  De10Top core_0 (
`endif
    .io_h2fLw_address        (core_h2f_address),
    .io_h2fLw_read           (core_h2f_read),
    .io_h2fLw_write          (core_h2f_write),
    .io_h2fLw_byteenable     (core_h2f_byteenable),
    .io_h2fLw_writedata      (core_h2f_writedata),
    .io_h2fLw_waitrequest    (core_h2f_waitrequest),
    .io_h2fLw_readdata       (core_h2f_readdata),
    .io_h2fLw_readdatavalid  (core_h2f_readdatavalid),
    .io_memFbWrite_read          (core_fb_read),
    .io_memFbWrite_write         (core_fb_write),
    .io_memFbWrite_waitRequestn  (~core_fb_waitrequest),
    .io_memFbWrite_burstCount    (core_fb_burstcount),
    .io_memFbWrite_address       (core_fb_address),
    .io_memFbWrite_byteEnable    (core_fb_byteenable),
    .io_memFbWrite_writeData     (core_fb_writedata),
    .io_memFbWrite_readDataValid (core_fb_readdatavalid),
    .io_memFbWrite_readData      (core_fb_readdata),
    .io_memFbColorRead_read          (fb_color_read),
    .io_memFbColorRead_write         (fb_color_write),
    .io_memFbColorRead_waitRequestn  (~fb_color_waitrequest),
    .io_memFbColorRead_burstCount    (fb_color_burstcount),
    .io_memFbColorRead_address       (fb_color_address),
    .io_memFbColorRead_byteEnable    (fb_color_byteenable),
    .io_memFbColorRead_writeData     (fb_color_writedata),
    .io_memFbColorRead_readDataValid (fb_color_readdatavalid),
    .io_memFbColorRead_readData      (fb_color_readdata),
`ifdef PURE_XLS_DE10
    .io_memFbColorWrite_read          (fb_color_wr_read),
    .io_memFbColorWrite_write         (fb_color_wr_write),
    .io_memFbColorWrite_waitRequestn  (~fb_color_wr_waitrequest),
    .io_memFbColorWrite_burstCount    (fb_color_wr_burstcount),
    .io_memFbColorWrite_address       (fb_color_wr_address),
    .io_memFbColorWrite_byteEnable    (fb_color_wr_byteenable),
    .io_memFbColorWrite_writeData     (fb_color_wr_writedata),
    .io_memFbColorWrite_readDataValid (fb_color_wr_readdatavalid),
    .io_memFbColorWrite_readData      (fb_color_wr_readdata),
`endif
    .io_memFbAuxRead_read          (fb_aux_read),
    .io_memFbAuxRead_write         (fb_aux_write),
    .io_memFbAuxRead_waitRequestn  (~fb_aux_waitrequest),
    .io_memFbAuxRead_burstCount    (fb_aux_burstcount),
    .io_memFbAuxRead_address       (fb_aux_address),
    .io_memFbAuxRead_byteEnable    (fb_aux_byteenable),
    .io_memFbAuxRead_writeData     (fb_aux_writedata),
    .io_memFbAuxRead_readDataValid (fb_aux_readdatavalid),
    .io_memFbAuxRead_readData      (fb_aux_readdata),
`ifdef PURE_XLS_DE10
    .io_memFbAuxWrite_read          (fb_aux_wr_read),
    .io_memFbAuxWrite_write         (fb_aux_wr_write),
    .io_memFbAuxWrite_waitRequestn  (~fb_aux_wr_waitrequest),
    .io_memFbAuxWrite_burstCount    (fb_aux_wr_burstcount),
    .io_memFbAuxWrite_address       (fb_aux_wr_address),
    .io_memFbAuxWrite_byteEnable    (fb_aux_wr_byteenable),
    .io_memFbAuxWrite_writeData     (fb_aux_wr_writedata),
    .io_memFbAuxWrite_readDataValid (fb_aux_wr_readdatavalid),
    .io_memFbAuxWrite_readData      (fb_aux_wr_readdata),
`endif
    .io_memTex_read          (tex_read),
    .io_memTex_write         (tex_write),
    .io_memTex_waitRequestn  (~tex_waitrequest),
    .io_memTex_burstCount    (tex_burstcount),
    .io_memTex_address       (tex_address),
    .io_memTex_byteEnable    (tex_byteenable),
    .io_memTex_writeData     (tex_writedata),
    .io_memTex_readDataValid (tex_readdatavalid),
    .io_memTex_readData      (tex_readdata),
`ifdef PURE_XLS_DE10
    .io_memScanout_read          (scanout_read),
    .io_memScanout_write         (scanout_write),
    .io_memScanout_waitRequestn  (~scanout_waitrequest),
    .io_memScanout_burstCount    (scanout_burstcount),
    .io_memScanout_address       (scanout_address),
    .io_memScanout_byteEnable    (scanout_byteenable),
    .io_memScanout_writeData     (scanout_writedata),
    .io_memScanout_readDataValid (scanout_readdatavalid),
    .io_memScanout_readData      (scanout_readdata),
`endif
    .io_hdmi_clock                    (hdmi_clk),
    .io_hdmi_reset                    (~hdmi_pll_locked),
    .io_hdmi_video_rgb_r              (hdmi_rgb_r),
    .io_hdmi_video_rgb_g              (hdmi_rgb_g),
    .io_hdmi_video_rgb_b              (hdmi_rgb_b),
    .io_hdmi_video_de                 (hdmi_de),
    .io_hdmi_video_hSync              (hdmi_hs),
    .io_hdmi_video_vSync              (hdmi_vs),
    .io_hdmi_status_displayedBuffer   (hdmi_status_displayed_buffer),
    .io_hdmi_status_displayedBase     (hdmi_status_displayed_base),
    .io_hdmi_status_swapPending       (hdmi_status_swap_pending),
    .io_hdmi_status_swapCount         (hdmi_status_swap_count),
    .io_hdmi_status_newFrame          (hdmi_status_new_frame),
    .io_hdmi_status_active            (hdmi_status_active),
    .io_hdmi_status_x                 (hdmi_status_x),
    .io_hdmi_status_y                 (hdmi_status_y),
    .io_hdmi_underflow                (hdmi_underflow),
    .io_hdmi_fifoLevel                (hdmi_fifo_level),
    .reset                   (core_reset),
    .clk                     (core_clk)
  );

`ifndef PURE_XLS_DE10
  assign scanout_read = 1'b0;
  assign scanout_write = 1'b0;
  assign scanout_burstcount = 11'b0;
  assign scanout_address = 32'b0;
  assign scanout_byteenable = 4'b0;
  assign scanout_writedata = 32'b0;
  assign fb_color_wr_read = 1'b0;
  assign fb_color_wr_write = 1'b0;
  assign fb_color_wr_burstcount = 11'b0;
  assign fb_color_wr_address = 32'b0;
  assign fb_color_wr_byteenable = 4'b0;
  assign fb_color_wr_writedata = 32'b0;
  assign fb_aux_wr_read = 1'b0;
  assign fb_aux_wr_write = 1'b0;
  assign fb_aux_wr_burstcount = 11'b0;
  assign fb_aux_wr_address = 32'b0;
  assign fb_aux_wr_byteenable = 4'b0;
  assign fb_aux_wr_writedata = 32'b0;
`endif
endmodule
