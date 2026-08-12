`timescale 1ns/1ps

module XlsAvalonTransferMasterTb;
  reg clk = 0;
  reg reset = 1;
  reg [65:0] request = 0;
  reg request_valid = 0;
  wire request_ready;
  wire [2048:0] response;
  wire response_valid;
  reg response_ready = 0;
  wire mem_read;
  wire mem_write;
  reg mem_waitRequestn = 1;
  wire [10:0] mem_burstCount;
  wire [31:0] mem_address;
  wire [3:0] mem_byteEnable;
  wire [31:0] mem_writeData;
  reg mem_readDataValid = 0;
  reg [31:0] mem_readData = 0;

  always #5 clk = ~clk;

  XlsAvalonTransferMaster #(.BASE_ADDRESS(32'h1000), .SPAN_WORDS(16)) dut (
    .clk(clk), .reset(reset), .request(request), .request_valid(request_valid),
    .request_ready(request_ready), .response(response), .response_valid(response_valid),
    .response_ready(response_ready), .mem_read(mem_read), .mem_write(mem_write),
    .mem_waitRequestn(mem_waitRequestn), .mem_burstCount(mem_burstCount),
    .mem_address(mem_address), .mem_byteEnable(mem_byteEnable), .mem_writeData(mem_writeData),
    .mem_readDataValid(mem_readDataValid), .mem_readData(mem_readData)
  );

  task send_request(
    input [6:0] count,
    input write,
    input [21:0] address,
    input [31:0] data,
    input [3:0] mask
  );
    begin
      request = {count, write, address, data, mask};
      request_valid = 1;
      while (!request_ready) @(posedge clk);
      #1;
    end
  endtask


  initial begin
    #1000;
    $fatal(1);
  end

  initial begin
    repeat (2) @(posedge clk);
    reset = 0;

    send_request(1, 1, 3, 32'h89abcdef, 4'h5);
    if (!mem_write || mem_address != 32'h100c || mem_burstCount != 1 ||
        mem_byteEnable != 4'h5 || mem_writeData != 32'h89abcdef) $fatal(1);
    @(posedge clk);
    #1;
    request_valid = 0;
    if (!response_valid || !response[0]) $fatal(1);
    response_ready = 1;
    @(posedge clk);
    #1;
    response_ready = 0;

    send_request(3, 0, 4, 0, 0);
    if (!mem_read || mem_address != 32'h1010 || mem_burstCount != 3 ||
        mem_byteEnable != 4'hf) $fatal(1);
    @(posedge clk);
    #1;
    request_valid = 0;
    mem_readData = 32'h11111111;
    mem_readDataValid = 1;
    @(posedge clk);
    #1;
    mem_readData = 32'h22222222;
    @(posedge clk);
    #1;
    mem_readData = 32'h33333333;
    @(posedge clk);
    #1;
    mem_readDataValid = 0;
    @(posedge clk);
    #1;
    if (!response_valid || !response[0] || response[32:1] != 32'h11111111 ||
        response[64:33] != 32'h22222222 || response[96:65] != 32'h33333333) $fatal(1);
    response_ready = 1;
    @(posedge clk);
    #1;
    response_ready = 0;

    send_request(2, 0, 15, 0, 0);
    @(posedge clk);
    #1;
    request_valid = 0;
    if (!response_valid || response[0] || mem_read || mem_write) $fatal(1);
    $display("XlsAvalonTransferMaster passed");
    $finish;
  end
endmodule
