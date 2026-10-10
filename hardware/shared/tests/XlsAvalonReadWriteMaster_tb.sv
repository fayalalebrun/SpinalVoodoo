`timescale 1ns/1ps

module XlsAvalonReadWriteMasterTb;
  reg clk = 0;
  reg reset = 1;
  reg [65:0] request = 0;
  reg request_valid = 0;
  wire request_ready;
  wire [32:0] response;
  wire response_valid;
  reg response_ready = 0;
  wire read_mem_read;
  wire read_mem_write;
  wire [10:0] read_mem_burstCount;
  wire [31:0] read_mem_address;
  wire [3:0] read_mem_byteEnable;
  wire [31:0] read_mem_writeData;
  reg read_mem_readDataValid = 0;
  reg [31:0] read_mem_readData = 0;
  wire write_mem_read;
  wire write_mem_write;
  wire [10:0] write_mem_burstCount;
  wire [31:0] write_mem_address;
  wire [3:0] write_mem_byteEnable;
  wire [31:0] write_mem_writeData;
  integer reads = 0;
  integer writes = 0;

  always #5 clk = ~clk;

  XlsAvalonReadWriteMaster #(.BASE_ADDRESS(32'h1000), .SPAN_WORDS(128)) dut (
    .clk(clk), .reset(reset), .request(request), .request_valid(request_valid),
    .request_ready(request_ready), .response(response), .response_valid(response_valid),
    .response_ready(response_ready), .read_mem_read(read_mem_read),
    .read_mem_write(read_mem_write), .read_mem_waitRequestn(1'b1),
    .read_mem_burstCount(read_mem_burstCount), .read_mem_address(read_mem_address),
    .read_mem_byteEnable(read_mem_byteEnable), .read_mem_writeData(read_mem_writeData),
    .read_mem_readDataValid(read_mem_readDataValid), .read_mem_readData(read_mem_readData),
    .write_mem_read(write_mem_read), .write_mem_write(write_mem_write),
    .write_mem_waitRequestn(1'b1), .write_mem_burstCount(write_mem_burstCount),
    .write_mem_address(write_mem_address), .write_mem_byteEnable(write_mem_byteEnable),
    .write_mem_writeData(write_mem_writeData), .write_mem_readDataValid(1'b0),
    .write_mem_readData(32'b0)
  );

  function automatic [65:0] make_request(
    input write,
    input [21:0] address,
    input [31:0] data
  );
    make_request = {7'd1, write, address, data, 4'hf};
  endfunction

  task automatic queue_request(input [65:0] value);
    begin
      @(negedge clk);
      request = value;
      request_valid = 1;
      while (!request_ready) @(negedge clk);
      @(posedge clk);
      #1;
      request_valid = 0;
    end
  endtask

  task automatic expect_response(input [31:0] expected_data);
    begin
      while (!response_valid) @(negedge clk);
      if (!response[0] || response[32:1] !== expected_data) $fatal(1);
      response_ready = 1;
      @(posedge clk);
      #1;
      response_ready = 0;
    end
  endtask

  always @(posedge clk) begin
    if (!reset && read_mem_read) begin
      if (read_mem_write || read_mem_burstCount != 1 || read_mem_byteEnable != 4'hf) $fatal(1);
      if (read_mem_address !== (reads == 0 ? 32'h1004 : 32'h100c)) $fatal(1);
      reads <= reads + 1;
    end
    if (!reset && write_mem_write) begin
      if (write_mem_read || write_mem_burstCount != 1 || write_mem_byteEnable != 4'hf) $fatal(1);
      if (write_mem_address !== (writes == 0 ? 32'h1008 : 32'h1010)) $fatal(1);
      if (write_mem_writeData !== (writes == 0 ? 32'h22222222 : 32'h44444444)) $fatal(1);
      writes <= writes + 1;
    end
  end

  initial begin
    #20000;
    $fatal(1);
  end

  initial begin
    repeat (2) @(posedge clk);
    @(negedge clk);
    reset = 0;
    queue_request(make_request(0, 1, 0));
    queue_request(make_request(1, 2, 32'h22222222));
    queue_request(make_request(0, 3, 0));
    queue_request(make_request(1, 4, 32'h44444444));
    wait (reads == 2 && writes == 2);
    if (response_valid) $fatal(1);
    @(negedge clk);
    read_mem_readDataValid = 1;
    read_mem_readData = 32'h11111111;
    @(negedge clk);
    read_mem_readData = 32'h33333333;
    @(negedge clk);
    read_mem_readDataValid = 0;
    expect_response(32'h11111111);
    expect_response(32'h22222222);
    expect_response(32'h33333333);
    expect_response(32'h44444444);
    $display("XlsAvalonReadWriteMaster passed");
    $finish;
  end
endmodule
