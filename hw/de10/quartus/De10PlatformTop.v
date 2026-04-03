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

  wire        fb_write_waitrequest;
  wire [31:0] fb_write_readdata;
  wire        fb_write_readdatavalid;
  wire [10:0] fb_write_burstcount;
  wire [31:0] fb_write_writedata;
  wire [31:0] fb_write_address;
  wire        fb_write_write;
  wire        fb_write_read;
  wire [3:0]  fb_write_byteenable;

  wire        fb_color_waitrequest;
  wire [31:0] fb_color_readdata;
  wire        fb_color_readdatavalid;
  wire [10:0] fb_color_burstcount;
  wire [31:0] fb_color_writedata;
  wire [31:0] fb_color_address;
  wire        fb_color_write;
  wire        fb_color_read;
  wire [3:0]  fb_color_byteenable;

  wire        fb_aux_waitrequest;
  wire [31:0] fb_aux_readdata;
  wire        fb_aux_readdatavalid;
  wire [10:0] fb_aux_burstcount;
  wire [31:0] fb_aux_writedata;
  wire [31:0] fb_aux_address;
  wire        fb_aux_write;
  wire        fb_aux_read;
  wire [3:0]  fb_aux_byteenable;

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
  wire        pll_locked;

  de10_core_pll core_pll_0 (
    .refclk   (clk),
    .rst      (1'b0),
    .outclk_0 (core_clk),
    .locked   (pll_locked)
  );

  de10_soc soc_0 (
    .clk_clk                     (core_clk),
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
  reg [7:0] core_reset_release = 8'h00;

  always @(posedge core_clk or negedge pll_locked) begin
    if (!pll_locked) begin
      pll_locked_meta <= 1'b0;
      pll_locked_sync <= 1'b0;
      h2f_reset_meta <= 1'b0;
      h2f_reset_sync <= 1'b0;
      core_reset_release <= 8'h00;
    end else begin
      pll_locked_meta <= 1'b1;
      pll_locked_sync <= pll_locked_meta;
      h2f_reset_meta <= h2f_reset_n;
      h2f_reset_sync <= h2f_reset_meta;
      if (!h2f_reset_sync) begin
        core_reset_release <= 8'h00;
      end else if (!core_reset_release[7]) begin
        core_reset_release <= core_reset_release + 8'h01;
      end
    end
  end

  assign core_reset = ~pll_locked_sync | ~h2f_reset_sync | ~core_reset_release[7];

  De10Top core_0 (
    .io_h2fLw_address        (h2f_address[23:2]),
    .io_h2fLw_read           (h2f_read),
    .io_h2fLw_write          (h2f_write),
    .io_h2fLw_byteenable     (h2f_byteenable),
    .io_h2fLw_writedata      (h2f_writedata),
    .io_h2fLw_waitrequest    (h2f_waitrequest),
    .io_h2fLw_readdata       (h2f_readdata),
    .io_h2fLw_readdatavalid  (h2f_readdatavalid),
    .io_memFbWrite_read          (fb_write_read),
    .io_memFbWrite_write         (fb_write_write),
    .io_memFbWrite_waitRequestn  (~fb_write_waitrequest),
    .io_memFbWrite_burstCount    (fb_write_burstcount),
    .io_memFbWrite_address       (fb_write_address),
    .io_memFbWrite_byteEnable    (fb_write_byteenable),
    .io_memFbWrite_writeData     (fb_write_writedata),
    .io_memFbWrite_readDataValid (fb_write_readdatavalid),
    .io_memFbWrite_readData      (fb_write_readdata),
    .io_memFbColorRead_read          (fb_color_read),
    .io_memFbColorRead_write         (fb_color_write),
    .io_memFbColorRead_waitRequestn  (~fb_color_waitrequest),
    .io_memFbColorRead_burstCount    (fb_color_burstcount),
    .io_memFbColorRead_address       (fb_color_address),
    .io_memFbColorRead_byteEnable    (fb_color_byteenable),
    .io_memFbColorRead_writeData     (fb_color_writedata),
    .io_memFbColorRead_readDataValid (fb_color_readdatavalid),
    .io_memFbColorRead_readData      (fb_color_readdata),
    .io_memFbAuxRead_read          (fb_aux_read),
    .io_memFbAuxRead_write         (fb_aux_write),
    .io_memFbAuxRead_waitRequestn  (~fb_aux_waitrequest),
    .io_memFbAuxRead_burstCount    (fb_aux_burstcount),
    .io_memFbAuxRead_address       (fb_aux_address),
    .io_memFbAuxRead_byteEnable    (fb_aux_byteenable),
    .io_memFbAuxRead_writeData     (fb_aux_writedata),
    .io_memFbAuxRead_readDataValid (fb_aux_readdatavalid),
    .io_memFbAuxRead_readData      (fb_aux_readdata),
    .io_memTex_read          (tex_read),
    .io_memTex_write         (tex_write),
    .io_memTex_waitRequestn  (~tex_waitrequest),
    .io_memTex_burstCount    (tex_burstcount),
    .io_memTex_address       (tex_address),
    .io_memTex_byteEnable    (tex_byteenable),
    .io_memTex_writeData     (tex_writedata),
    .io_memTex_readDataValid (tex_readdatavalid),
    .io_memTex_readData      (tex_readdata),
    .reset                   (core_reset),
    .clk                     (core_clk)
  );
  assign h2f_burstcount = 1'b1;

endmodule
