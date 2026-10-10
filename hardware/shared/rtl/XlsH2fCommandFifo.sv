module XlsH2fAsyncFifo #(
  parameter WIDTH = 8,
  parameter ADDRESS_WIDTH = 6
) (
  input  wire                 aclr,
  input  wire [WIDTH-1:0]     data,
  input  wire                 rdclk,
  input  wire                 rdreq,
  input  wire                 wrclk,
  input  wire                 wrreq,
  output wire [WIDTH-1:0]     q,
  output wire                 rdempty,
  output wire                 wrfull
);
  wire [WIDTH-1:0] raw_q;
  wire raw_empty;
  reg output_valid;
  wire output_consume = rdreq && output_valid;
  wire output_fetch = !raw_empty && (!output_valid || output_consume);

  assign q = raw_q;
  assign rdempty = !output_valid;

  always @(posedge rdclk or posedge aclr) begin
    if (aclr) begin
      output_valid <= 1'b0;
    end else if (output_fetch) begin
      output_valid <= 1'b1;
    end else if (output_consume) begin
      output_valid <= 1'b0;
    end
  end

`ifdef ALTERA_RESERVED_QIS
  dcfifo #(
    .intended_device_family("Cyclone V"),
    .lpm_numwords(1 << ADDRESS_WIDTH),
    .lpm_showahead("OFF"),
    .lpm_type("dcfifo"),
    .lpm_width(WIDTH),
    .lpm_widthu(ADDRESS_WIDTH),
    .overflow_checking("ON"),
    .rdsync_delaypipe(2),
    .underflow_checking("ON"),
    .use_eab("ON"),
    .wrsync_delaypipe(2)
  ) fifo (
    .aclr(aclr),
    .data(data),
    .rdclk(rdclk),
    .rdreq(output_fetch),
    .wrclk(wrclk),
    .wrreq(wrreq),
    .q(raw_q),
    .rdempty(raw_empty),
    .wrfull(wrfull),
    .rdfull(),
    .rdusedw(),
    .wrempty(),
    .wrusedw()
  );
`else
  localparam POINTER_WIDTH = ADDRESS_WIDTH + 1;
  reg [WIDTH-1:0] memory [0:(1 << ADDRESS_WIDTH)-1];
  reg [POINTER_WIDTH-1:0] write_binary;
  reg [POINTER_WIDTH-1:0] write_gray;
  reg [POINTER_WIDTH-1:0] read_binary;
  reg [POINTER_WIDTH-1:0] read_gray;
  reg [POINTER_WIDTH-1:0] read_gray_sync1;
  reg [POINTER_WIDTH-1:0] read_gray_sync2;
  reg [POINTER_WIDTH-1:0] write_gray_sync1;
  reg [POINTER_WIDTH-1:0] write_gray_sync2;
  reg full;
  reg empty;
  reg [WIDTH-1:0] read_output;

  wire write_advance = wrreq && !full;
  wire read_advance = output_fetch && !empty;
  wire [POINTER_WIDTH-1:0] write_binary_next = write_binary + write_advance;
  wire [POINTER_WIDTH-1:0] read_binary_next = read_binary + read_advance;
  wire [POINTER_WIDTH-1:0] write_gray_next =
      (write_binary_next >> 1) ^ write_binary_next;
  wire [POINTER_WIDTH-1:0] read_gray_next =
      (read_binary_next >> 1) ^ read_binary_next;
  wire full_next = write_gray_next ==
      {~read_gray_sync2[POINTER_WIDTH-1:POINTER_WIDTH-2],
       read_gray_sync2[POINTER_WIDTH-3:0]};
  wire empty_next = read_gray_next == write_gray_sync2;

  assign raw_q = read_output;
  assign raw_empty = empty;
  assign wrfull = full;

  always @(posedge wrclk or posedge aclr) begin
    if (aclr) begin
      write_binary <= 0;
      write_gray <= 0;
      read_gray_sync1 <= 0;
      read_gray_sync2 <= 0;
      full <= 1'b0;
    end else begin
      read_gray_sync1 <= read_gray;
      read_gray_sync2 <= read_gray_sync1;
      if (write_advance) memory[write_binary[ADDRESS_WIDTH-1:0]] <= data;
      write_binary <= write_binary_next;
      write_gray <= write_gray_next;
      full <= full_next;
    end
  end

  always @(posedge rdclk or posedge aclr) begin
    if (aclr) begin
      read_binary <= 0;
      read_gray <= 0;
      write_gray_sync1 <= 0;
      write_gray_sync2 <= 0;
      empty <= 1'b1;
      read_output <= 0;
    end else begin
      write_gray_sync1 <= write_gray;
      write_gray_sync2 <= write_gray_sync1;
      if (read_advance) read_output <= memory[read_binary[ADDRESS_WIDTH-1:0]];
      read_binary <= read_binary_next;
      read_gray <= read_gray_next;
      empty <= empty_next;
    end
  end
`endif
endmodule

module XlsH2fCommandFifo #(
  parameter FIFO_ADDRESS_WIDTH = 6,
  parameter MEMORY_FIFO_ENABLE = 0
) (
  input  wire        h2f_clk,
  input  wire        h2f_reset,
  input  wire [21:0] h2f_address,
  input  wire        h2f_read,
  input  wire        h2f_write,
  input  wire [3:0]  h2f_byteenable,
  input  wire [31:0] h2f_writedata,
  output wire        h2f_waitrequest,
  output wire [31:0] h2f_readdata,
  output wire        h2f_readdatavalid,
  input  wire        core_clk,
  input  wire        core_reset,
  output wire [21:0] core_address,
  output wire        core_read,
  output wire        core_write,
  output wire [3:0]  core_byteenable,
  output wire [31:0] core_writedata,
  input  wire        core_waitrequest,
  input  wire [31:0] core_readdata,
  input  wire        core_readdatavalid,
  output wire        mem_read,
  output wire        mem_write,
  input  wire        mem_waitRequestn,
  output wire [10:0] mem_burstCount,
  output wire [31:0] mem_address,
  output wire [3:0]  mem_byteEnable,
  output wire [31:0] mem_writeData,
  input  wire        mem_readDataValid,
  input  wire [31:0] mem_readData
);
  localparam REQUEST_WIDTH = 60;
  wire [REQUEST_WIDTH-1:0] request_data = {
    h2f_address, h2f_read, h2f_write, h2f_byteenable, h2f_writedata
  };
  wire [REQUEST_WIDTH-1:0] request_q;
  wire request_empty;
  wire request_full;
  wire request_pop;
  wire [REQUEST_WIDTH-1:0] buffered_request;
  wire buffered_empty;
  wire buffered_pop;
  wire [REQUEST_WIDTH-1:0] expanded_request;
  wire expanded_empty;
  reg [REQUEST_WIDTH-1:0] queued_request;
  reg queued_valid;
  wire queued_empty = !queued_valid;

  wire fifo_mem_read;
  wire fifo_mem_write;
  wire fifo_mem_waitRequestn;
  wire [10:0] fifo_mem_burstCount;
  wire [31:0] fifo_mem_address;
  wire [3:0] fifo_mem_byteEnable;
  wire [31:0] fifo_mem_writeData;
  wire fifo_mem_readDataValid;
  wire [31:0] fifo_mem_readData;
  wire batch_mem_read;
  wire batch_mem_write;
  wire batch_mem_waitRequestn;
  wire [10:0] batch_mem_burstCount;
  wire [31:0] batch_mem_address;
  wire [3:0] batch_mem_byteEnable;
  wire [31:0] batch_mem_writeData;
  wire batch_mem_readDataValid;
  wire [31:0] batch_mem_readData;
  wire request_valid = h2f_read ^ h2f_write;
  wire response_empty;
  wire response_full;
  wire [31:0] response_q;
  wire queued_read = queued_request[37];
  wire queued_write = queued_request[36];
  wire core_request_valid = !queued_empty && (!queued_read || !response_full);
  wire core_request_accepted = core_request_valid && !core_waitrequest;
  wire expanded_pop = !expanded_empty && (!queued_valid || core_request_accepted);

  assign h2f_waitrequest = request_valid && request_full;
  assign h2f_readdata = response_q;
  assign h2f_readdatavalid = !response_empty;
  assign core_address = queued_request[59:38];
  assign core_read = core_request_valid && queued_read;
  assign core_write = core_request_valid && queued_write;
  assign core_byteenable = queued_request[35:32];
  assign core_writedata = queued_request[31:0];

  XlsH2fAsyncFifo #(
    .WIDTH(REQUEST_WIDTH),
    .ADDRESS_WIDTH(FIFO_ADDRESS_WIDTH)
  ) request_fifo (
    .aclr(h2f_reset),
    .data(request_data),
    .rdclk(core_clk),
    .rdreq(request_pop),
    .wrclk(h2f_clk),
    .wrreq(request_valid && !request_full),
    .q(request_q),
    .rdempty(request_empty),
    .wrfull(request_full)
  );

  generate
    if (MEMORY_FIFO_ENABLE) begin : memory_fifo_enabled
      XlsH2fMemoryFifo memory_fifo (
        .clk(core_clk),
        .reset(core_reset),
        .ingress_data(request_q),
        .ingress_empty(request_empty),
        .ingress_pop(request_pop),
        .request_data(buffered_request),
        .request_empty(buffered_empty),
        .request_pop(buffered_pop),
        .mem_read(fifo_mem_read),
        .mem_write(fifo_mem_write),
        .mem_waitRequestn(fifo_mem_waitRequestn),
        .mem_burstCount(fifo_mem_burstCount),
        .mem_address(fifo_mem_address),
        .mem_byteEnable(fifo_mem_byteEnable),
        .mem_writeData(fifo_mem_writeData),
        .mem_readDataValid(fifo_mem_readDataValid),
        .mem_readData(fifo_mem_readData)
      );
    end else begin : memory_fifo_disabled
      assign request_pop = buffered_pop;
      assign buffered_request = request_q;
      assign buffered_empty = request_empty;
      assign fifo_mem_read = 1'b0;
      assign fifo_mem_write = 1'b0;
      assign fifo_mem_burstCount = 11'd1;
      assign fifo_mem_address = 32'b0;
      assign fifo_mem_byteEnable = 4'hf;
      assign fifo_mem_writeData = 32'b0;
    end
  endgenerate

  always @(posedge core_clk or posedge core_reset) begin
    if (core_reset) begin
      queued_valid <= 1'b0;
    end else if (!queued_valid || core_request_accepted) begin
      queued_valid <= !expanded_empty;
    end
  end

  always @(posedge core_clk) begin
    if (expanded_pop) queued_request <= expanded_request;
  end

  XlsH2fBatchExpander batch_expander (
    .clk(core_clk),
    .reset(core_reset),
    .ingress_data(buffered_request),
    .ingress_empty(buffered_empty),
    .ingress_pop(buffered_pop),
    .request_data(expanded_request),
    .request_empty(expanded_empty),
    .request_pop(expanded_pop),
    .mem_read(batch_mem_read),
    .mem_write(batch_mem_write),
    .mem_waitRequestn(batch_mem_waitRequestn),
    .mem_burstCount(batch_mem_burstCount),
    .mem_address(batch_mem_address),
    .mem_byteEnable(batch_mem_byteEnable),
    .mem_writeData(batch_mem_writeData),
    .mem_readDataValid(batch_mem_readDataValid),
    .mem_readData(batch_mem_readData)
  );

  XlsAvalonArbiter2 memory_arbiter (
    .clk(core_clk), .reset(core_reset),
    .m0_read(fifo_mem_read), .m0_write(fifo_mem_write),
    .m0_waitRequestn(fifo_mem_waitRequestn), .m0_burstCount(fifo_mem_burstCount),
    .m0_address(fifo_mem_address), .m0_byteEnable(fifo_mem_byteEnable),
    .m0_writeData(fifo_mem_writeData), .m0_readDataValid(fifo_mem_readDataValid),
    .m0_readData(fifo_mem_readData),
    .m1_read(batch_mem_read), .m1_write(batch_mem_write),
    .m1_waitRequestn(batch_mem_waitRequestn), .m1_burstCount(batch_mem_burstCount),
    .m1_address(batch_mem_address), .m1_byteEnable(batch_mem_byteEnable),
    .m1_writeData(batch_mem_writeData), .m1_readDataValid(batch_mem_readDataValid),
    .m1_readData(batch_mem_readData),
    .s_read(mem_read), .s_write(mem_write), .s_waitRequestn(mem_waitRequestn),
    .s_burstCount(mem_burstCount), .s_address(mem_address),
    .s_byteEnable(mem_byteEnable), .s_writeData(mem_writeData),
    .s_readDataValid(mem_readDataValid), .s_readData(mem_readData)
  );

  XlsH2fAsyncFifo #(
    .WIDTH(32),
    .ADDRESS_WIDTH(2)
  ) response_fifo (
    .aclr(h2f_reset),
    .data(core_readdata),
    .rdclk(h2f_clk),
    .rdreq(!response_empty),
    .wrclk(core_clk),
    .wrreq(core_readdatavalid && !response_full),
    .q(response_q),
    .rdempty(response_empty),
    .wrfull(response_full)
  );
endmodule
