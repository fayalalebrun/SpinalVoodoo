module XlsDe10Top #(parameter EXTERNAL_SCANOUT = 0) (
  input wire io_externalRetrace,
  input  wire [21:0] io_h2fLw_address,
  input  wire        io_h2fLw_read,
  input  wire        io_h2fLw_write,
  input  wire [3:0]  io_h2fLw_byteenable,
  input  wire [31:0] io_h2fLw_writedata,
  output wire        io_h2fLw_waitrequest,
  output reg  [31:0] io_h2fLw_readdata,
  output reg         io_h2fLw_readdatavalid,
  output wire        io_memFbWrite_read,
  output wire        io_memFbWrite_write,
  input  wire        io_memFbWrite_waitRequestn,
  output wire [31:0] io_memFbWrite_address,
  output wire [10:0] io_memFbWrite_burstCount,
  output wire [3:0]  io_memFbWrite_byteEnable,
  output wire [31:0] io_memFbWrite_writeData,
  input  wire        io_memFbWrite_readDataValid,
  input  wire [31:0] io_memFbWrite_readData,
  output wire        io_memFbColorWrite_read,
  output wire        io_memFbColorWrite_write,
  input  wire        io_memFbColorWrite_waitRequestn,
  output wire [31:0] io_memFbColorWrite_address,
  output wire [10:0] io_memFbColorWrite_burstCount,
   output wire [3:0]  io_memFbColorWrite_byteEnable,
   output wire [31:0] io_memFbColorWrite_writeData,
   input  wire        io_memFbColorWrite_readDataValid,
   input  wire [31:0] io_memFbColorWrite_readData,
  output wire        io_memFbAuxWrite_read,
  output wire        io_memFbAuxWrite_write,
  input  wire        io_memFbAuxWrite_waitRequestn,
  output wire [31:0] io_memFbAuxWrite_address,
  output wire [10:0] io_memFbAuxWrite_burstCount,
   output wire [3:0]  io_memFbAuxWrite_byteEnable,
   output wire [31:0] io_memFbAuxWrite_writeData,
   input  wire        io_memFbAuxWrite_readDataValid,
   input  wire [31:0] io_memFbAuxWrite_readData,
  output wire        io_memFbColorRead_read,
  output wire        io_memFbColorRead_write,
  input  wire        io_memFbColorRead_waitRequestn,
  output wire [31:0] io_memFbColorRead_address,
  output wire [10:0] io_memFbColorRead_burstCount,
  output wire [3:0]  io_memFbColorRead_byteEnable,
  output wire [31:0] io_memFbColorRead_writeData,
  input  wire        io_memFbColorRead_readDataValid,
  input  wire [31:0] io_memFbColorRead_readData,
  output wire        io_memFbAuxRead_read,
  output wire        io_memFbAuxRead_write,
  input  wire        io_memFbAuxRead_waitRequestn,
  output wire [31:0] io_memFbAuxRead_address,
  output wire [10:0] io_memFbAuxRead_burstCount,
  output wire [3:0]  io_memFbAuxRead_byteEnable,
  output wire [31:0] io_memFbAuxRead_writeData,
  input  wire        io_memFbAuxRead_readDataValid,
  input  wire [31:0] io_memFbAuxRead_readData,
  output wire        io_memTex_read,
  output wire        io_memTex_write,
  input  wire        io_memTex_waitRequestn,
  output wire [31:0] io_memTex_address,
  output wire [10:0] io_memTex_burstCount,
  output wire [3:0]  io_memTex_byteEnable,
  output wire [31:0] io_memTex_writeData,
  input  wire        io_memTex_readDataValid,
  input  wire [31:0] io_memTex_readData,
  output wire        io_memScanout_read,
  output wire        io_memScanout_write,
  input  wire        io_memScanout_waitRequestn,
  output wire [31:0] io_memScanout_address,
  output wire [10:0] io_memScanout_burstCount,
  output wire [3:0]  io_memScanout_byteEnable,
  output wire [31:0] io_memScanout_writeData,
  input  wire        io_memScanout_readDataValid,
  input  wire [31:0] io_memScanout_readData,
  input  wire        io_hdmi_clock,
  input  wire        io_hdmi_reset,
  output wire [7:0]  io_hdmi_video_rgb_r,
  output wire [7:0]  io_hdmi_video_rgb_g,
  output wire [7:0]  io_hdmi_video_rgb_b,
  output wire        io_hdmi_video_de,
  output wire        io_hdmi_video_hSync,
  output wire        io_hdmi_video_vSync,
  output wire [0:0]  io_hdmi_status_displayedBuffer,
  output wire [25:0] io_hdmi_status_displayedBase,
  output wire        io_hdmi_status_swapPending,
  output wire [1:0]  io_hdmi_status_swapCount,
  output wire        io_hdmi_status_newFrame,
  output wire        io_hdmi_status_active,
  output wire [9:0]  io_hdmi_status_x,
  output wire [9:0]  io_hdmi_status_y,
  output wire        io_hdmi_underflow,
  output wire [12:0] io_hdmi_fifoLevel,
  input  wire        reset,
  input  wire        clk
);
  wire legal_mmio = io_h2fLw_read ^ io_h2fLw_write;
  wire mmio_request = io_h2fLw_read || io_h2fLw_write;
  reg mmio_outstanding;
  reg mmio_read_pending;

  reg perf_response_pending;
   wire [8:0] performance_event;
   wire performance_event_valid;
   wire [5:0] profile_pending;
   wire profile_pending_valid;
   wire profile_interpolation_start_valid;
   wire profile_pipeline_start_valid;
   wire profile_cache_done_valid;
   wire profile_read_done_valid;
    wire profile_write_done_valid;
     wire [31:0] profile_read_data;
    wire [31:0] render_profile_read_data;

  wire [63:0] render_cycles;
  wire [63:0] pixels_in;
  wire [63:0] pixels_out;
  wire [23:0] fbi_pixels_in;
  wire [23:0] fbi_pixels_out;
  wire triangle_active;
   wire profile_counter_read = io_h2fLw_read &&
       io_h2fLw_address >= 22'h040000 && io_h2fLw_address < 22'h040040;
   wire [5:0] profile_read_index = io_h2fLw_address - 22'h040000;
   wire batch_capability_read = io_h2fLw_read && io_h2fLw_address == 22'h0ffc01;
   wire perf_counter_read = io_h2fLw_read &&
       (io_h2fLw_address == 22'h000053 || io_h2fLw_address == 22'h000057 ||
        (io_h2fLw_address >= 22'h0000f4 && io_h2fLw_address <= 22'h0000f9) ||
        profile_counter_read || batch_capability_read);
  wire perf_counter_clear = io_h2fLw_write && io_h2fLw_address == 22'h0000f3;
     wire [31:0] perf_counter_data = batch_capability_read ? 32'h42415431 :
       profile_counter_read ? profile_read_data :
       io_h2fLw_address == 22'h000053 ? {8'b0, fbi_pixels_in} :
      io_h2fLw_address == 22'h000057 ? {8'b0, fbi_pixels_out} :
      io_h2fLw_address == 22'h0000f4 ? render_cycles[31:0] :
      io_h2fLw_address == 22'h0000f5 ? render_cycles[63:32] :
      io_h2fLw_address == 22'h0000f6 ? pixels_in[31:0] :
      io_h2fLw_address == 22'h0000f7 ? pixels_in[63:32] :
      io_h2fLw_address == 22'h0000f8 ? pixels_out[31:0] : pixels_out[63:32];
  wire [59:0] core_request = {
    io_h2fLw_address,
    io_h2fLw_read,
    io_h2fLw_write,
    io_h2fLw_byteenable,
    io_h2fLw_writedata
  };
  wire core_request_valid = mmio_request && legal_mmio && !mmio_outstanding &&
      !perf_counter_read && !perf_counter_clear && !framebuffer_writes_busy;
  wire core_request_ready;
  wire core_request_accepted = core_request_valid && core_request_ready;
  wire perf_request_accepted = perf_counter_read && !mmio_outstanding && !triangle_active &&
      !framebuffer_writes_busy;
  wire perf_clear_accepted = perf_counter_clear && !mmio_outstanding && !triangle_active &&
      !framebuffer_writes_busy;
  wire stats_clear = core_request_accepted && io_h2fLw_write &&
      io_h2fLw_address == (22'h120 >> 2);
  wire triangle_start = core_request_accepted && io_h2fLw_write &&
      (io_h2fLw_address == (22'h080 >> 2) || io_h2fLw_address == (22'h100 >> 2));
  wire [32:0] core_response;
  wire core_response_valid;
  wire core_response_ready = 1'b1;
  wire [39:0] core_scanout;
  wire core_scanout_valid;
  reg [39:0] scanout_state;
  reg retrace_pending;
  reg [11:0] scanout_idle_cycles;
  wire retrace_ready;
  wire scanout_enable = !reset && !triangle_start && !triangle_active &&
      !framebuffer_writes_busy && (&scanout_idle_cycles);

  wire [65:0] framebuffer_transfer;
  wire framebuffer_transfer_valid;
  wire framebuffer_transfer_ready;
  wire [32:0] framebuffer_response;
  wire framebuffer_response_valid;
  wire framebuffer_response_ready;
  wire [45:0] color_span;
  wire color_span_valid;
   wire color_span_ready;
   wire [45:0] aux_span;
   wire aux_span_valid;
   wire aux_span_ready;
  wire [65:0] color_read_transfer;
  wire color_read_transfer_valid;
  wire color_read_transfer_ready;
  wire [32:0] color_read_response;
  wire color_read_response_valid;
  wire color_read_response_ready;
   wire color_read_miss_waiting = 1'b0;
   wire color_read_fill_active = 1'b0;
    wire [15:0] color_read_fill_line_tag = 16'b0;
    wire color_read_data_stall;
    wire color_read_inflight_stall;
    wire color_read_unissued_stall;
    wire color_read_response_stall;
     wire color_write_busy;

   wire color_write_line0_valid;
  wire [15:0] color_write_line0_tag;
  wire color_write_line1_valid;
  wire [15:0] color_write_line1_tag;
  wire [65:0] color_write_transfer;
  wire color_write_transfer_valid;
  wire color_write_transfer_ready;
  wire [32:0] color_write_response;
  wire color_write_response_valid;
   wire color_write_response_ready;
   wire [8:0] color_write_ingress_count;
  wire [65:0] aux_read_transfer;
  wire aux_read_transfer_valid;
  wire aux_read_transfer_ready;
  wire [32:0] aux_read_response;
  wire aux_read_response_valid;
  wire aux_read_response_ready;
   wire aux_read_miss_waiting = 1'b0;
   wire aux_read_fill_active = 1'b0;
    wire [15:0] aux_read_fill_line_tag = 16'b0;
    wire aux_read_data_stall;
    wire aux_read_inflight_stall;
    wire aux_read_unissued_stall;
    wire aux_read_response_stall;
     wire aux_write_busy;

   wire aux_write_line0_valid;
  wire [15:0] aux_write_line0_tag;
  wire aux_write_line1_valid;
  wire [15:0] aux_write_line1_tag;
  wire [65:0] aux_write_transfer;
  wire aux_write_transfer_valid;
  wire aux_write_transfer_ready;
  wire [32:0] aux_write_response;
  wire aux_write_response_valid;
   wire aux_write_response_ready;
   wire [8:0] aux_write_ingress_count;
  wire [65:0] texture_transfer;
  wire texture_transfer_valid;
  wire texture_transfer_ready;
  wire [32:0] texture_response;
   wire texture_response_valid;
   wire texture_response_ready;
   wire framebuffer_master_request_ready;
  wire framebuffer_barrier = framebuffer_transfer_valid;
  wire framebuffer_writes_busy = color_write_busy || aux_write_busy;
   wire color_write_hazard = 1'b0;
   wire aux_write_hazard = 1'b0;
  wire framebuffer_direct_write = framebuffer_transfer_valid && framebuffer_transfer_ready &&
      framebuffer_transfer[58];
  wire color_cache_update = color_write_transfer_valid && color_write_transfer_ready &&
      color_write_transfer[58];
  wire aux_cache_update = aux_write_transfer_valid && aux_write_transfer_ready &&
      aux_write_transfer[58];
  wire framebuffer_cache_invalidate = perf_clear_accepted || framebuffer_direct_write;
  wire any_read_request_stall =
      (color_read_transfer_valid && !color_read_transfer_ready) ||
      (aux_read_transfer_valid && !aux_read_transfer_ready);
  wire any_write_request_stall =
      (color_write_transfer_valid && !color_write_transfer_ready) ||
      (aux_write_transfer_valid && !aux_write_transfer_ready);
   wire any_write_avalon_stall =
       (io_memFbColorWrite_write && !io_memFbColorWrite_waitRequestn) ||
       (io_memFbAuxWrite_write && !io_memFbAuxWrite_waitRequestn);

    wire [31:0] pipeline_profile_read_data =
        profile_read_index == 0 ? 32'h00140000 : render_profile_read_data;
   assign profile_read_data = pipeline_profile_read_data;
   assign framebuffer_transfer_ready = framebuffer_master_request_ready &&
       !framebuffer_writes_busy;

   wire [261:0] slow_bank_request;
  wire slow_bank_request_valid;
  wire slow_bank_request_ready;
   wire [108:0] fast_bank_request;
  wire fast_bank_request_valid;
  wire fast_bank_request_ready;
   wire [280:0] slow_bank_response;
  wire slow_bank_response_valid;
  wire slow_bank_response_ready;
   wire [280:0] fast_bank_response;
  wire fast_bank_response_valid;
  wire fast_bank_response_ready;
  wire [24:0] tag_read;
  wire tag_read_valid, tag_read_ready;
  wire [55:0] tag_response;
  wire tag_response_valid, tag_response_ready;
  wire [26:0] tag_write;
  wire tag_write_valid, tag_write_ready;
  XlsTextureTagMemory tag_memory (
    .clk(clk), .reset(reset),
    .read_request(tag_read), .read_valid(tag_read_valid), .read_ready(tag_read_ready),
    .response(tag_response), .response_valid(tag_response_valid), .response_ready(tag_response_ready),
    .write_request(tag_write), .write_valid(tag_write_valid), .write_ready(tag_write_ready)
  );
  wire [31:0] palette_read_request;
  wire palette_read_request_valid;
  wire palette_read_request_ready;
  wire [32:0] palette_write;
  wire palette_write_valid;
  wire palette_write_ready;
  wire [95:0] palette_response;
  wire palette_response_valid;
  wire palette_response_ready;

   assign io_h2fLw_waitrequest = mmio_request &&
       !(core_request_accepted || perf_request_accepted || perf_clear_accepted);


  always @(posedge clk) begin
    if (reset) begin
      mmio_outstanding <= 1'b0;
      mmio_read_pending <= 1'b0;

       perf_response_pending <= 1'b0;
       io_h2fLw_readdata <= 32'b0;
      io_h2fLw_readdatavalid <= 1'b0;
      scanout_state <= {26'b0, 11'd640, 1'b0, 2'b0};
      retrace_pending <= 1'b0;
      scanout_idle_cycles <= 12'hfff;
    end else begin
      if (triangle_start || triangle_active) begin
        scanout_idle_cycles <= 12'b0;
      end else if (scanout_idle_cycles != 12'hfff) begin
        scanout_idle_cycles <= scanout_idle_cycles + 1'b1;
      end
       io_h2fLw_readdatavalid <= 1'b0;
       if (perf_request_accepted) begin
         mmio_outstanding <= 1'b1;
         perf_response_pending <= 1'b1;
         io_h2fLw_readdata <= perf_counter_data;
       end
       if (core_request_accepted) begin
        mmio_outstanding <= 1'b1;
        mmio_read_pending <= io_h2fLw_read;

      end
      if (core_response_valid && (mmio_outstanding || core_request_accepted)) begin
        mmio_outstanding <= 1'b0;
        if (mmio_read_pending || (core_request_accepted && io_h2fLw_read)) begin
          io_h2fLw_readdata <= core_response[0] ? core_response[32:1] : 32'b0;
          io_h2fLw_readdatavalid <= 1'b1;
        end
        mmio_read_pending <= 1'b0;
      end
       if (perf_response_pending) begin
         mmio_outstanding <= 1'b0;
         perf_response_pending <= 1'b0;
         io_h2fLw_readdatavalid <= 1'b1;
       end

      if (core_scanout_valid) begin
        scanout_state <= core_scanout;
      end
      if (io_hdmi_status_newFrame) begin
        retrace_pending <= 1'b1;
      end else if (retrace_pending && retrace_ready) begin
        retrace_pending <= 1'b0;
      end
    end
  end

   CoreSimDe10 core (
    .clk(clk),
    .rst(reset),
    ._de10_request_r(core_request),
    ._de10_request_r_vld(core_request_valid),
    ._de10_request_r_rdy(core_request_ready),
    ._v_retrace_r(1'b1),
    ._v_retrace_r_vld(retrace_pending),
    ._v_retrace_r_rdy(retrace_ready),
    ._response_s(core_response),
    ._response_s_vld(core_response_valid),
    ._response_s_rdy(core_response_ready),
    ._scanout_s(core_scanout),
    ._scanout_s_vld(core_scanout_valid),
    ._scanout_s_rdy(1'b1),
    ._framebuffer_transfer_s(framebuffer_transfer),
    ._framebuffer_transfer_s_vld(framebuffer_transfer_valid),
    ._framebuffer_transfer_s_rdy(framebuffer_transfer_ready),
    ._framebuffer_transfer_response_r(framebuffer_response),
    ._framebuffer_transfer_response_r_vld(framebuffer_response_valid),
    ._framebuffer_transfer_response_r_rdy(framebuffer_response_ready),
    ._color_span_s(color_span), ._color_span_s_vld(color_span_valid),
    ._color_span_s_rdy(color_span_ready),
    ._aux_span_s(aux_span), ._aux_span_s_vld(aux_span_valid),
    ._aux_span_s_rdy(aux_span_ready),
    ._color_read_transfer_s(color_read_transfer),
    ._color_read_transfer_s_vld(color_read_transfer_valid),
    ._color_read_transfer_s_rdy(color_read_transfer_ready),
    ._color_read_transfer_response_r(color_read_response),
    ._color_read_transfer_response_r_vld(color_read_response_valid),
    ._color_read_transfer_response_r_rdy(color_read_response_ready),
    ._color_write_transfer_s(color_write_transfer),
    ._color_write_transfer_s_vld(color_write_transfer_valid),
    ._color_write_transfer_s_rdy(color_write_transfer_ready),
    ._color_write_transfer_response_r(color_write_response),
    ._color_write_transfer_response_r_vld(color_write_response_valid),
    ._color_write_transfer_response_r_rdy(color_write_response_ready),
    ._aux_read_transfer_s(aux_read_transfer),
    ._aux_read_transfer_s_vld(aux_read_transfer_valid),
    ._aux_read_transfer_s_rdy(aux_read_transfer_ready),
    ._aux_read_transfer_response_r(aux_read_response),
    ._aux_read_transfer_response_r_vld(aux_read_response_valid),
    ._aux_read_transfer_response_r_rdy(aux_read_response_ready),
    ._aux_write_transfer_s(aux_write_transfer),
    ._aux_write_transfer_s_vld(aux_write_transfer_valid),
    ._aux_write_transfer_s_rdy(aux_write_transfer_ready),
    ._aux_write_transfer_response_r(aux_write_response),
    ._aux_write_transfer_response_r_vld(aux_write_response_valid),
    ._aux_write_transfer_response_r_rdy(aux_write_response_ready),
    ._texture_transfer_s(texture_transfer),
    ._texture_transfer_s_vld(texture_transfer_valid),
    ._texture_transfer_s_rdy(texture_transfer_ready),
    ._texture_transfer_response_r(texture_response),
    ._texture_transfer_response_r_vld(texture_response_valid),
    ._texture_transfer_response_r_rdy(texture_response_ready),
    ._slow_bank_request_s(slow_bank_request),
    ._slow_bank_request_s_vld(slow_bank_request_valid),
    ._slow_bank_request_s_rdy(slow_bank_request_ready),
    ._fast_bank_request_s(fast_bank_request),
    ._fast_bank_request_s_vld(fast_bank_request_valid),
    ._fast_bank_request_s_rdy(fast_bank_request_ready),
    ._slow_bank_response_r(slow_bank_response),
    ._slow_bank_response_r_vld(slow_bank_response_valid),
    ._slow_bank_response_r_rdy(slow_bank_response_ready),
    ._fast_bank_response_r(fast_bank_response),
    ._fast_bank_response_r_vld(fast_bank_response_valid),
    ._fast_bank_response_r_rdy(fast_bank_response_ready),
    ._tag_read_s(tag_read), ._tag_read_s_vld(tag_read_valid), ._tag_read_s_rdy(tag_read_ready),
    ._tag_response_r(tag_response), ._tag_response_r_vld(tag_response_valid), ._tag_response_r_rdy(tag_response_ready),
    ._tag_write_s(tag_write), ._tag_write_s_vld(tag_write_valid), ._tag_write_s_rdy(tag_write_ready),
    ._palette_read_request_s(palette_read_request),
    ._palette_read_request_s_vld(palette_read_request_valid),
    ._palette_read_request_s_rdy(palette_read_request_ready),
    ._palette_response_r(palette_response),
    ._palette_response_r_vld(palette_response_valid),
    ._palette_response_r_rdy(palette_response_ready),
    ._palette_write_s(palette_write),
    ._palette_write_s_vld(palette_write_valid),
    ._palette_write_s_rdy(palette_write_ready),
    ._performance_s(performance_event),
    ._performance_s_vld(performance_event_valid),
    ._performance_s_rdy(1'b1),
    ._profile_pending_s(profile_pending),
    ._profile_pending_s_vld(profile_pending_valid),
    ._profile_pending_s_rdy(1'b1),
    ._profile_interpolation_start_s(),
    ._profile_interpolation_start_s_vld(profile_interpolation_start_valid),
    ._profile_interpolation_start_s_rdy(1'b1),
    ._profile_pipeline_start_s(),
    ._profile_pipeline_start_s_vld(profile_pipeline_start_valid),
    ._profile_pipeline_start_s_rdy(1'b1),
    ._profile_cache_done_s(),
    ._profile_cache_done_s_vld(profile_cache_done_valid),
    ._profile_cache_done_s_rdy(1'b1),
    ._profile_read_done_s(),
    ._profile_read_done_s_vld(profile_read_done_valid),
    ._profile_read_done_s_rdy(1'b1),
    ._profile_write_done_s(),
    ._profile_write_done_s_vld(profile_write_done_valid),
    ._profile_write_done_s_rdy(1'b1)
  );


  XlsAvalonTransferMaster #(.BASE_ADDRESS(32'h3f400000), .SPAN_WORDS(1048576)) fb_master (
    .clk(clk), .reset(reset), .request(framebuffer_transfer),
    .request_valid(framebuffer_transfer_valid && !framebuffer_writes_busy),
    .request_ready(framebuffer_master_request_ready),
    .response(framebuffer_response), .response_valid(framebuffer_response_valid),
    .response_ready(framebuffer_response_ready), .mem_read(io_memFbWrite_read),
    .mem_write(io_memFbWrite_write), .mem_waitRequestn(io_memFbWrite_waitRequestn),
    .mem_burstCount(io_memFbWrite_burstCount), .mem_address(io_memFbWrite_address),
    .mem_byteEnable(io_memFbWrite_byteEnable), .mem_writeData(io_memFbWrite_writeData),
    .mem_readDataValid(io_memFbWrite_readDataValid), .mem_readData(io_memFbWrite_readData)
  );

    XlsFramebufferSpanReader #(
      .BASE_ADDRESS(32'h3f400000), .ADDRESS_WORDS(1048576), .SPAN_DEPTH(8),
      .DATA_DEPTH(256), .ROUTE_DEPTH(8), .RESPONSE_DEPTH(8)
    ) color_read_cache (
     .clk(clk), .reset(reset), .span(color_span), .span_valid(color_span_valid),
     .span_ready(color_span_ready), .request(color_read_transfer),
     .request_valid(color_read_transfer_valid), .request_ready(color_read_transfer_ready),
     .response(color_read_response), .response_valid(color_read_response_valid),
     .response_ready(color_read_response_ready), .mem_read(io_memFbColorRead_read),
     .mem_write(io_memFbColorRead_write), .mem_waitRequestn(io_memFbColorRead_waitRequestn),
     .mem_burstCount(io_memFbColorRead_burstCount), .mem_address(io_memFbColorRead_address),
     .mem_byteEnable(io_memFbColorRead_byteEnable), .mem_writeData(io_memFbColorRead_writeData),
     .mem_readDataValid(io_memFbColorRead_readDataValid),
     .mem_readData(io_memFbColorRead_readData), .invalidate(framebuffer_cache_invalidate),
     .debug_data_stall(color_read_data_stall),
     .debug_inflight_stall(color_read_inflight_stall),
     .debug_unissued_stall(color_read_unissued_stall),
     .debug_response_stall(color_read_response_stall)
   );

  XlsFramebufferWriteIngress #(
    .BASE_ADDRESS(32'h3f400000), .SPAN_WORDS(1048576), .INGRESS_DEPTH(128)
  ) color_write_master (
    .clk(clk), .reset(reset), .request(color_write_transfer),
    .request_valid(color_write_transfer_valid), .request_ready(color_write_transfer_ready),
     .response(color_write_response), .response_valid(color_write_response_valid),
     .response_ready(color_write_response_ready), .mem_read(io_memFbColorWrite_read),
     .mem_write(io_memFbColorWrite_write), .mem_waitRequestn(io_memFbColorWrite_waitRequestn),
     .mem_burstCount(io_memFbColorWrite_burstCount), .mem_address(io_memFbColorWrite_address),
     .mem_byteEnable(io_memFbColorWrite_byteEnable), .mem_writeData(io_memFbColorWrite_writeData),
     .mem_readDataValid(io_memFbColorWrite_readDataValid),
     .mem_readData(io_memFbColorWrite_readData),
    .flush(framebuffer_barrier || perf_clear_accepted || color_write_hazard),
    .block_line_valid(color_read_fill_active), .block_line_tag(color_read_fill_line_tag),
    .busy(color_write_busy), .update_valid(), .update_address(), .update_data(), .update_mask(),
     .pending_line0_valid(color_write_line0_valid), .pending_line0_tag(color_write_line0_tag),
     .pending_line1_valid(color_write_line1_valid), .pending_line1_tag(color_write_line1_tag),
     .debug_count(color_write_ingress_count)
   );



    XlsFramebufferSpanReader #(
      .BASE_ADDRESS(32'h3f400000), .ADDRESS_WORDS(1048576), .SPAN_DEPTH(8),
      .DATA_DEPTH(256), .ROUTE_DEPTH(8), .RESPONSE_DEPTH(8)
    ) aux_read_cache (
     .clk(clk), .reset(reset), .span(aux_span), .span_valid(aux_span_valid),
     .span_ready(aux_span_ready), .request(aux_read_transfer),
     .request_valid(aux_read_transfer_valid), .request_ready(aux_read_transfer_ready),
     .response(aux_read_response), .response_valid(aux_read_response_valid),
     .response_ready(aux_read_response_ready), .mem_read(io_memFbAuxRead_read),
     .mem_write(io_memFbAuxRead_write), .mem_waitRequestn(io_memFbAuxRead_waitRequestn),
     .mem_burstCount(io_memFbAuxRead_burstCount), .mem_address(io_memFbAuxRead_address),
     .mem_byteEnable(io_memFbAuxRead_byteEnable), .mem_writeData(io_memFbAuxRead_writeData),
     .mem_readDataValid(io_memFbAuxRead_readDataValid), .mem_readData(io_memFbAuxRead_readData),
     .invalidate(framebuffer_cache_invalidate), .debug_data_stall(aux_read_data_stall),
     .debug_inflight_stall(aux_read_inflight_stall),
     .debug_unissued_stall(aux_read_unissued_stall),
     .debug_response_stall(aux_read_response_stall)
   );

  XlsFramebufferWriteIngress #(
    .BASE_ADDRESS(32'h3f400000), .SPAN_WORDS(1048576), .INGRESS_DEPTH(128)
  ) aux_write_master (
    .clk(clk), .reset(reset), .request(aux_write_transfer),
    .request_valid(aux_write_transfer_valid), .request_ready(aux_write_transfer_ready),
     .response(aux_write_response), .response_valid(aux_write_response_valid),
     .response_ready(aux_write_response_ready), .mem_read(io_memFbAuxWrite_read),
     .mem_write(io_memFbAuxWrite_write), .mem_waitRequestn(io_memFbAuxWrite_waitRequestn),
     .mem_burstCount(io_memFbAuxWrite_burstCount), .mem_address(io_memFbAuxWrite_address),
     .mem_byteEnable(io_memFbAuxWrite_byteEnable), .mem_writeData(io_memFbAuxWrite_writeData),
     .mem_readDataValid(io_memFbAuxWrite_readDataValid), .mem_readData(io_memFbAuxWrite_readData),
    .flush(framebuffer_barrier || perf_clear_accepted || aux_write_hazard),
    .block_line_valid(aux_read_fill_active), .block_line_tag(aux_read_fill_line_tag),
    .busy(aux_write_busy), .update_valid(), .update_address(), .update_data(), .update_mask(),
     .pending_line0_valid(aux_write_line0_valid), .pending_line0_tag(aux_write_line0_tag),
     .pending_line1_valid(aux_write_line1_valid), .pending_line1_tag(aux_write_line1_tag),
     .debug_count(aux_write_ingress_count)
   );



  XlsAvalonTransferMaster #(
    .BASE_ADDRESS(32'h3f800000), .SPAN_WORDS(2097152), .WRAP_BURST64(1)
  ) texture_master (
    .clk(clk), .reset(reset), .request(texture_transfer), .request_valid(texture_transfer_valid),
    .request_ready(texture_transfer_ready), .response(texture_response),
    .response_valid(texture_response_valid), .response_ready(texture_response_ready),
    .mem_read(io_memTex_read), .mem_write(io_memTex_write),
    .mem_waitRequestn(io_memTex_waitRequestn), .mem_burstCount(io_memTex_burstCount),
    .mem_address(io_memTex_address), .mem_byteEnable(io_memTex_byteEnable),
    .mem_writeData(io_memTex_writeData), .mem_readDataValid(io_memTex_readDataValid),
    .mem_readData(io_memTex_readData)
  );

   XlsPackedBankMemoryVector #(
     .PRIMARY_NUMWORDS(4096),
     .PRIMARY_ADDRESS_WIDTH(12)
   ) bank_memory (
    .clk(clk), .reset(reset),
    .slow_request(slow_bank_request), .slow_request_valid(slow_bank_request_valid),
    .slow_request_ready(slow_bank_request_ready),
    .fast_request(fast_bank_request), .fast_request_valid(fast_bank_request_valid),
    .fast_request_ready(fast_bank_request_ready),
    .slow_response(slow_bank_response), .slow_response_valid(slow_bank_response_valid),
    .slow_response_ready(slow_bank_response_ready),
    .fast_response(fast_bank_response), .fast_response_valid(fast_bank_response_valid),
    .fast_response_ready(fast_bank_response_ready)
  );

  XlsPerformanceCounters performance_counters (
    .clk(clk), .reset(reset), .clear(perf_clear_accepted), .stats_clear(stats_clear),
    .triangle_start(triangle_start), .event_data(performance_event),
    .event_valid(performance_event_valid), .render_cycles(render_cycles),
    .pixels_in(pixels_in), .pixels_out(pixels_out), .fbi_pixels_in(fbi_pixels_in),
    .fbi_pixels_out(fbi_pixels_out), .triangle_active(triangle_active)
  );

   assign render_profile_read_data = 32'b0;

  XlsPaletteMemory palette(
    .clk(clk), .reset(reset), .read_request(palette_read_request),
    .read_request_valid(palette_read_request_valid),
    .read_request_ready(palette_read_request_ready), .write(palette_write),
    .write_valid(palette_write_valid), .write_ready(palette_write_ready),
    .response(palette_response), .response_valid(palette_response_valid),
    .response_ready(palette_response_ready)
  );

  generate if (EXTERNAL_SCANOUT) begin : external_scanout
    assign io_memScanout_read = 0;
    assign io_memScanout_write = 0;
    assign io_memScanout_address = 0;
    assign io_memScanout_burstCount = 0;
    assign io_memScanout_byteEnable = 0;
    assign io_memScanout_writeData = 0;
    assign {io_hdmi_video_rgb_r, io_hdmi_video_rgb_g, io_hdmi_video_rgb_b} = 0;
    assign {io_hdmi_video_de, io_hdmi_video_hSync, io_hdmi_video_vSync} = 0;
    assign io_hdmi_status_displayedBuffer = 0;
    assign io_hdmi_status_displayedBase = 0;
    assign io_hdmi_status_swapPending = 0;
    assign io_hdmi_status_swapCount = 0;
    assign io_hdmi_status_newFrame = io_externalRetrace;
    assign io_hdmi_status_active = 0;
    assign io_hdmi_status_x = 0;
    assign io_hdmi_status_y = 0;
    assign io_hdmi_underflow = 0;
    assign io_hdmi_fifoLevel = 0;
  end else begin : internal_scanout
  XlsDe10Scanout scanout (
    .io_base(scanout_state[39:14]),
    .io_pixelStride(scanout_state[13:3]),
    .io_enable(scanout_enable),
    .io_mem_read(io_memScanout_read),
    .io_mem_write(io_memScanout_write),
    .io_mem_waitRequestn(io_memScanout_waitRequestn),
    .io_mem_address(io_memScanout_address),
    .io_mem_burstCount(io_memScanout_burstCount),
    .io_mem_byteEnable(io_memScanout_byteEnable),
    .io_mem_writeData(io_memScanout_writeData),
    .io_mem_readDataValid(io_memScanout_readDataValid),
    .io_mem_readData(io_memScanout_readData),
    .io_hdmi_clock(io_hdmi_clock),
    .io_hdmi_reset(io_hdmi_reset),
    .io_hdmi_video_rgb_r(io_hdmi_video_rgb_r),
    .io_hdmi_video_rgb_g(io_hdmi_video_rgb_g),
    .io_hdmi_video_rgb_b(io_hdmi_video_rgb_b),
    .io_hdmi_video_de(io_hdmi_video_de),
    .io_hdmi_video_hSync(io_hdmi_video_hSync),
    .io_hdmi_video_vSync(io_hdmi_video_vSync),
    .io_hdmi_status_displayedBuffer(io_hdmi_status_displayedBuffer),
    .io_hdmi_status_displayedBase(io_hdmi_status_displayedBase),
    .io_hdmi_status_swapPending(io_hdmi_status_swapPending),
    .io_hdmi_status_swapCount(io_hdmi_status_swapCount),
    .io_hdmi_status_newFrame(io_hdmi_status_newFrame),
    .io_hdmi_status_active(io_hdmi_status_active),
    .io_hdmi_status_x(io_hdmi_status_x),
    .io_hdmi_status_y(io_hdmi_status_y),
    .io_hdmi_underflow(io_hdmi_underflow),
    .io_hdmi_fifoLevel(io_hdmi_fifoLevel),
    .clk(clk),
    .reset(reset)
  );
  end endgenerate
endmodule
