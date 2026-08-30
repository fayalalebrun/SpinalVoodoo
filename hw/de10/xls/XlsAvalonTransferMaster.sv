module XlsAvalonTransferMaster #(
  parameter [31:0] BASE_ADDRESS = 32'h00000000,
  parameter integer SPAN_WORDS = 1,
  parameter integer WRAP_BURST64 = 0
) (
  input  wire          clk,
  input  wire          reset,
  input  wire [65:0]   request,
  input  wire          request_valid,
  output wire          request_ready,
  output wire [32:0]   response,
  output wire          response_valid,
  input  wire          response_ready,
  output wire          mem_read,
  output wire          mem_write,
  input  wire          mem_waitRequestn,
  output wire [10:0]   mem_burstCount,
  output wire [31:0]   mem_address,
  output wire [3:0]    mem_byteEnable,
  output wire [31:0]   mem_writeData,
  input  wire          mem_readDataValid,
  input  wire [31:0]   mem_readData
);
  localparam [22:0] SPAN_WORDS_VALUE = SPAN_WORDS;

  XlsAvalonTransferMasterCore core (
    .request(request),
    .request_valid(request_valid),
    .request_ready(request_ready),
    .response(response),
    .response_valid(response_valid),
    .response_ready(response_ready),
    .mem_read(mem_read),
    .mem_write(mem_write),
    .mem_waitRequestn(mem_waitRequestn),
    .mem_burstCount(mem_burstCount),
    .mem_address(mem_address),
    .mem_byteEnable(mem_byteEnable),
    .mem_writeData(mem_writeData),
    .mem_readDataValid(mem_readDataValid),
    .mem_readData(mem_readData),
    .base_address(BASE_ADDRESS),
    .span_words(SPAN_WORDS_VALUE),
    .wrap_burst64(WRAP_BURST64 != 0),
    .clk(clk),
    .reset(reset)
  );
endmodule
