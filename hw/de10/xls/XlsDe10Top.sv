module XlsDe10Top (
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
  reg mmio_register_read_pending;
  reg mmio_response_delayed;
  wire [59:0] core_request = {
    io_h2fLw_address,
    io_h2fLw_read,
    io_h2fLw_write,
    io_h2fLw_byteenable,
    io_h2fLw_writedata
  };
  wire core_request_valid = mmio_request && legal_mmio && !mmio_outstanding;
  wire core_request_ready;
  wire [32:0] core_response;
  wire core_response_valid;
  wire core_response_ready = mmio_outstanding;
  wire [39:0] core_scanout;
  wire core_scanout_valid;
  reg [39:0] scanout_state;
  reg retrace_pending;
  wire retrace_ready;

  wire [65:0] framebuffer_transfer;
  wire framebuffer_transfer_valid;
  wire framebuffer_transfer_ready;
  wire [2048:0] framebuffer_response;
  wire framebuffer_response_valid;
  wire framebuffer_response_ready;
  wire [65:0] color_transfer;
  wire color_transfer_valid;
  wire color_transfer_ready;
  wire [2048:0] color_response;
  wire color_response_valid;
  wire color_response_ready;
  wire [65:0] aux_transfer;
  wire aux_transfer_valid;
  wire aux_transfer_ready;
  wire [2048:0] aux_response;
  wire aux_response_valid;
  wire aux_response_ready;
  wire [65:0] texture_transfer;
  wire texture_transfer_valid;
  wire texture_transfer_ready;
  wire [2048:0] texture_response;
  wire texture_response_valid;
  wire texture_response_ready;
   wire [58:0] bank0_command;
  wire bank0_command_valid;
  wire bank0_command_ready;
  wire [32:0] bank0_response;
  wire bank0_response_valid;
  wire bank0_response_ready;
   wire [58:0] bank1_command;
  wire bank1_command_valid;
  wire bank1_command_ready;
  wire [32:0] bank1_response;
  wire bank1_response_valid;
  wire bank1_response_ready;
   wire [58:0] bank2_command;
  wire bank2_command_valid;
  wire bank2_command_ready;
  wire [32:0] bank2_response;
  wire bank2_response_valid;
  wire bank2_response_ready;
   wire [58:0] bank3_command;
  wire bank3_command_valid;
  wire bank3_command_ready;
  wire [32:0] bank3_response;
  wire bank3_response_valid;
  wire bank3_response_ready;

  assign io_h2fLw_waitrequest = mmio_request &&
      !(core_request_valid && core_request_ready);
  assign io_memFbColorWrite_read = 1'b0;
  assign io_memFbColorWrite_write = 1'b0;
  assign io_memFbColorWrite_address = 32'b0;
  assign io_memFbColorWrite_burstCount = 11'b0;
  assign io_memFbColorWrite_byteEnable = 4'b0;
  assign io_memFbColorWrite_writeData = 32'b0;
  assign io_memFbAuxWrite_read = 1'b0;
  assign io_memFbAuxWrite_write = 1'b0;
  assign io_memFbAuxWrite_address = 32'b0;
  assign io_memFbAuxWrite_burstCount = 11'b0;
  assign io_memFbAuxWrite_byteEnable = 4'b0;
  assign io_memFbAuxWrite_writeData = 32'b0;

  always @(posedge clk) begin
    if (reset) begin
      mmio_outstanding <= 1'b0;
      mmio_read_pending <= 1'b0;
      mmio_register_read_pending <= 1'b0;
      mmio_response_delayed <= 1'b0;
      io_h2fLw_readdata <= 32'b0;
      io_h2fLw_readdatavalid <= 1'b0;
      scanout_state <= {26'b0, 11'd640, 1'b0, 2'b0};
      retrace_pending <= 1'b0;
    end else begin
      io_h2fLw_readdatavalid <= 1'b0;
      if (core_request_valid && core_request_ready) begin
        mmio_outstanding <= 1'b1;
        mmio_read_pending <= io_h2fLw_read;
        mmio_register_read_pending <= io_h2fLw_read && io_h2fLw_address < 22'h100000;
      end
      if (core_response_valid && core_response_ready) begin
        if (mmio_register_read_pending) begin
          mmio_response_delayed <= 1'b1;
        end else begin
          mmio_outstanding <= 1'b0;
          if (mmio_read_pending) begin
            io_h2fLw_readdata <= core_response[0] ? core_response[32:1] : 32'b0;
            io_h2fLw_readdatavalid <= 1'b1;
          end
          mmio_read_pending <= 1'b0;
        end
      end
      if (mmio_response_delayed) begin
        mmio_outstanding <= 1'b0;
        mmio_read_pending <= 1'b0;
        mmio_register_read_pending <= 1'b0;
        mmio_response_delayed <= 1'b0;
        io_h2fLw_readdata <= core_response[0] ? core_response[32:1] : 32'b0;
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
    ._color_transfer_s(color_transfer),
    ._color_transfer_s_vld(color_transfer_valid),
    ._color_transfer_s_rdy(color_transfer_ready),
    ._color_transfer_response_r(color_response),
    ._color_transfer_response_r_vld(color_response_valid),
    ._color_transfer_response_r_rdy(color_response_ready),
    ._aux_transfer_s(aux_transfer),
    ._aux_transfer_s_vld(aux_transfer_valid),
    ._aux_transfer_s_rdy(aux_transfer_ready),
    ._aux_transfer_response_r(aux_response),
    ._aux_transfer_response_r_vld(aux_response_valid),
    ._aux_transfer_response_r_rdy(aux_response_ready),
    ._texture_transfer_s(texture_transfer),
    ._texture_transfer_s_vld(texture_transfer_valid),
    ._texture_transfer_s_rdy(texture_transfer_ready),
    ._texture_transfer_response_r(texture_response),
    ._texture_transfer_response_r_vld(texture_response_valid),
    ._texture_transfer_response_r_rdy(texture_response_ready),
    ._bank0_command_s(bank0_command),
    ._bank0_command_s_vld(bank0_command_valid),
    ._bank0_command_s_rdy(bank0_command_ready),
    ._bank0_response_r(bank0_response),
    ._bank0_response_r_vld(bank0_response_valid),
    ._bank0_response_r_rdy(bank0_response_ready),
    ._bank1_command_s(bank1_command),
    ._bank1_command_s_vld(bank1_command_valid),
    ._bank1_command_s_rdy(bank1_command_ready),
    ._bank1_response_r(bank1_response),
    ._bank1_response_r_vld(bank1_response_valid),
    ._bank1_response_r_rdy(bank1_response_ready),
    ._bank2_command_s(bank2_command),
    ._bank2_command_s_vld(bank2_command_valid),
    ._bank2_command_s_rdy(bank2_command_ready),
    ._bank2_response_r(bank2_response),
    ._bank2_response_r_vld(bank2_response_valid),
    ._bank2_response_r_rdy(bank2_response_ready),
    ._bank3_command_s(bank3_command),
    ._bank3_command_s_vld(bank3_command_valid),
    ._bank3_command_s_rdy(bank3_command_ready),
    ._bank3_response_r(bank3_response),
    ._bank3_response_r_vld(bank3_response_valid),
    ._bank3_response_r_rdy(bank3_response_ready)
  );

  XlsAvalonTransferMaster #(.BASE_ADDRESS(32'h3f400000), .SPAN_WORDS(1048576)) fb_master (
    .clk(clk), .reset(reset), .request(framebuffer_transfer),
    .request_valid(framebuffer_transfer_valid), .request_ready(framebuffer_transfer_ready),
    .response(framebuffer_response), .response_valid(framebuffer_response_valid),
    .response_ready(framebuffer_response_ready), .mem_read(io_memFbWrite_read),
    .mem_write(io_memFbWrite_write), .mem_waitRequestn(io_memFbWrite_waitRequestn),
    .mem_burstCount(io_memFbWrite_burstCount), .mem_address(io_memFbWrite_address),
    .mem_byteEnable(io_memFbWrite_byteEnable), .mem_writeData(io_memFbWrite_writeData),
    .mem_readDataValid(io_memFbWrite_readDataValid), .mem_readData(io_memFbWrite_readData)
  );

  XlsAvalonTransferMaster #(.BASE_ADDRESS(32'h3f400000), .SPAN_WORDS(1048576)) color_master (
    .clk(clk), .reset(reset), .request(color_transfer), .request_valid(color_transfer_valid),
    .request_ready(color_transfer_ready), .response(color_response),
    .response_valid(color_response_valid), .response_ready(color_response_ready),
    .mem_read(io_memFbColorRead_read), .mem_write(io_memFbColorRead_write),
    .mem_waitRequestn(io_memFbColorRead_waitRequestn),
    .mem_burstCount(io_memFbColorRead_burstCount), .mem_address(io_memFbColorRead_address),
    .mem_byteEnable(io_memFbColorRead_byteEnable), .mem_writeData(io_memFbColorRead_writeData),
    .mem_readDataValid(io_memFbColorRead_readDataValid),
    .mem_readData(io_memFbColorRead_readData)
  );

  XlsAvalonTransferMaster #(.BASE_ADDRESS(32'h3f400000), .SPAN_WORDS(1048576)) aux_master (
    .clk(clk), .reset(reset), .request(aux_transfer), .request_valid(aux_transfer_valid),
    .request_ready(aux_transfer_ready), .response(aux_response),
    .response_valid(aux_response_valid), .response_ready(aux_response_ready),
    .mem_read(io_memFbAuxRead_read), .mem_write(io_memFbAuxRead_write),
    .mem_waitRequestn(io_memFbAuxRead_waitRequestn), .mem_burstCount(io_memFbAuxRead_burstCount),
    .mem_address(io_memFbAuxRead_address), .mem_byteEnable(io_memFbAuxRead_byteEnable),
    .mem_writeData(io_memFbAuxRead_writeData),
    .mem_readDataValid(io_memFbAuxRead_readDataValid), .mem_readData(io_memFbAuxRead_readData)
  );

  XlsAvalonTransferMaster #(.BASE_ADDRESS(32'h3f800000), .SPAN_WORDS(2097152)) texture_master (
    .clk(clk), .reset(reset), .request(texture_transfer), .request_valid(texture_transfer_valid),
    .request_ready(texture_transfer_ready), .response(texture_response),
    .response_valid(texture_response_valid), .response_ready(texture_response_ready),
    .mem_read(io_memTex_read), .mem_write(io_memTex_write),
    .mem_waitRequestn(io_memTex_waitRequestn), .mem_burstCount(io_memTex_burstCount),
    .mem_address(io_memTex_address), .mem_byteEnable(io_memTex_byteEnable),
    .mem_writeData(io_memTex_writeData), .mem_readDataValid(io_memTex_readDataValid),
    .mem_readData(io_memTex_readData)
  );

  XlsPackedBankMemory bank0(
    .clk(clk), .reset(reset), .command(bank0_command), .command_valid(bank0_command_valid),
    .command_ready(bank0_command_ready), .response(bank0_response),
    .response_valid(bank0_response_valid), .response_ready(bank0_response_ready)
  );
  XlsPackedBankMemory bank1(
    .clk(clk), .reset(reset), .command(bank1_command), .command_valid(bank1_command_valid),
    .command_ready(bank1_command_ready), .response(bank1_response),
    .response_valid(bank1_response_valid), .response_ready(bank1_response_ready)
  );
  XlsPackedBankMemory bank2(
    .clk(clk), .reset(reset), .command(bank2_command), .command_valid(bank2_command_valid),
    .command_ready(bank2_command_ready), .response(bank2_response),
    .response_valid(bank2_response_valid), .response_ready(bank2_response_ready)
  );
  XlsPackedBankMemory bank3(
    .clk(clk), .reset(reset), .command(bank3_command), .command_valid(bank3_command_valid),
    .command_ready(bank3_command_ready), .response(bank3_response),
    .response_valid(bank3_response_valid), .response_ready(bank3_response_ready)
  );

  XlsDe10Scanout scanout (
    .io_base(scanout_state[39:14]),
    .io_pixelStride(scanout_state[13:3]),
    .io_enable(!reset),
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
endmodule
