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
  input  wire        hps_memory_oct_rzqin
);

  wire        h2f_lw_waitrequest;
  wire [31:0] h2f_lw_readdata;
  wire        h2f_lw_readdatavalid;
  wire [0:0]  h2f_lw_burstcount;
  wire [31:0] h2f_lw_writedata;
  wire [23:0] h2f_lw_address;
  wire        h2f_lw_write;
  wire        h2f_lw_read;
  wire [3:0]  h2f_lw_byteenable;

  wire        fb_waitrequest;
  wire [31:0] fb_readdata;
  wire        fb_readdatavalid;
  wire [0:0]  fb_burstcount;
  wire [31:0] fb_writedata;
  wire [31:0] fb_address;
  wire        fb_write;
  wire        fb_read;
  wire [3:0]  fb_byteenable;

  wire        tex_waitrequest;
  wire [31:0] tex_readdata;
  wire        tex_readdatavalid;
  wire [0:0]  tex_burstcount;
  wire [31:0] tex_writedata;
  wire [31:0] tex_address;
  wire        tex_write;
  wire        tex_read;
  wire [3:0]  tex_byteenable;

  wire        core_clk;
  wire        pll_locked;

  de10_core_pll core_pll_0 (
    .refclk   (clk),
    .rst      (1'b0),
    .outclk_0 (core_clk),
    .locked   (pll_locked)
  );

  de10_soc soc_0 (
    .clk_clk                     (core_clk),
    .fb_mem_waitrequest          (fb_waitrequest),
    .fb_mem_readdata             (fb_readdata),
    .fb_mem_readdatavalid        (fb_readdatavalid),
    .fb_mem_burstcount           (fb_burstcount),
    .fb_mem_writedata            (fb_writedata),
    .fb_mem_address              (fb_address),
    .fb_mem_write                (fb_write),
    .fb_mem_read                 (fb_read),
    .fb_mem_byteenable           (fb_byteenable),
    .fb_mem_debugaccess          (1'b0),
    .h2f_lw_avalon_waitrequest   (h2f_lw_waitrequest),
    .h2f_lw_avalon_readdata      (h2f_lw_readdata),
    .h2f_lw_avalon_readdatavalid (h2f_lw_readdatavalid),
    .h2f_lw_avalon_burstcount    (h2f_lw_burstcount),
    .h2f_lw_avalon_writedata     (h2f_lw_writedata),
    .h2f_lw_avalon_address       (h2f_lw_address),
    .h2f_lw_avalon_write         (h2f_lw_write),
    .h2f_lw_avalon_read          (h2f_lw_read),
    .h2f_lw_avalon_byteenable    (h2f_lw_byteenable),
    .h2f_lw_avalon_debugaccess   (),
    .h2f_mpu_events_eventi       (1'b0),
    .h2f_mpu_events_evento       (),
    .h2f_mpu_events_standbywfe   (),
    .h2f_mpu_events_standbywfi   (),
    .h2f_reset_reset_n           (),
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

  De10Top core_0 (
    .io_h2fLw_address        (h2f_lw_address[23:2]),
    .io_h2fLw_read           (h2f_lw_read),
    .io_h2fLw_write          (h2f_lw_write),
    .io_h2fLw_byteenable     (h2f_lw_byteenable),
    .io_h2fLw_writedata      (h2f_lw_writedata),
    .io_h2fLw_waitrequest    (h2f_lw_waitrequest),
    .io_h2fLw_readdata       (h2f_lw_readdata),
    .io_h2fLw_readdatavalid  (h2f_lw_readdatavalid),
    .io_memFb_read           (fb_read),
    .io_memFb_write          (fb_write),
    .io_memFb_waitRequestn   (~fb_waitrequest),
    .io_memFb_address        (fb_address),
    .io_memFb_byteEnable     (fb_byteenable),
    .io_memFb_writeData      (fb_writedata),
    .io_memFb_readDataValid  (fb_readdatavalid),
    .io_memFb_readData       (fb_readdata),
    .io_memTex_read          (tex_read),
    .io_memTex_write         (tex_write),
    .io_memTex_waitRequestn  (~tex_waitrequest),
    .io_memTex_address       (tex_address),
    .io_memTex_byteEnable    (tex_byteenable),
    .io_memTex_writeData     (tex_writedata),
    .io_memTex_readDataValid (tex_readdatavalid),
    .io_memTex_readData      (tex_readdata),
    .clk                     (core_clk)
  );

  assign h2f_lw_burstcount = 1'b1;
  assign fb_burstcount = 1'b1;
  assign tex_burstcount = 1'b1;

endmodule
