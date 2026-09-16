`timescale 1ns/1ps

module XlsH2fBatchExpanderTb;
  localparam [21:0] DOORBELL_ADDRESS = 22'h0ffc00;
  localparam [31:0] BATCH_BASE_ADDRESS = 32'h00002000;
  reg clk = 0;
  reg reset = 1;
  reg [59:0] ingress_data = 0;
  reg ingress_empty = 1;
  wire ingress_pop;
  wire [59:0] request_data;
  wire request_empty;
  reg consume = 1;
  wire request_pop = consume && !request_empty;
  wire mem_read;
  wire mem_write;
  reg mem_waitRequestn = 1;
  wire [10:0] mem_burstCount;
  wire [31:0] mem_address;
  wire [3:0] mem_byteEnable;
  wire [31:0] mem_writeData;
  reg mem_readDataValid = 0;
  reg [31:0] mem_readData = 0;
  reg [31:0] memory [0:1023];
  reg [9:0] read_index = 0;
  reg [10:0] read_remaining = 0;
  reg read_active = 0;
  integer cycles = 0;
  integer received = 0;
  integer read_bursts = 0;
  integer i;

  always #5 clk = ~clk;

  function [59:0] make_write(input [21:0] address, input [31:0] data);
    make_write = {address, 1'b0, 1'b1, 4'hf, data};
  endfunction

  task set_batch_record(input integer slot, input integer index,
                        input [21:0] address, input [31:0] data);
    reg [59:0] record;
    integer word_index;
    begin
      record = make_write(address, data);
      word_index = slot * 128 + index * 2;
      memory[word_index] = record[31:0];
      memory[word_index + 1] = {4'b0, record[59:32]};
    end
  endtask

  task push(input [59:0] value);
    begin
      @(negedge clk);
      ingress_data = value;
      ingress_empty = 0;
      @(posedge clk);
      while (!ingress_pop) @(posedge clk);
      @(negedge clk);
      ingress_empty = 1;
    end
  endtask

  task push_batch(input integer slot, input integer count);
    reg [31:0] value;
    begin
      value = (slot << 20) | (count - 1);
      push(make_write(DOORBELL_ADDRESS, value));
    end
  endtask

  XlsH2fBatchExpander #(
    .DOORBELL_ADDRESS(DOORBELL_ADDRESS),
    .BATCH_BASE_ADDRESS(BATCH_BASE_ADDRESS)
  ) dut (
    .clk(clk), .reset(reset),
    .ingress_data(ingress_data), .ingress_empty(ingress_empty), .ingress_pop(ingress_pop),
    .request_data(request_data), .request_empty(request_empty), .request_pop(request_pop),
    .mem_read(mem_read), .mem_write(mem_write), .mem_waitRequestn(mem_waitRequestn),
    .mem_burstCount(mem_burstCount), .mem_address(mem_address),
    .mem_byteEnable(mem_byteEnable), .mem_writeData(mem_writeData),
    .mem_readDataValid(mem_readDataValid), .mem_readData(mem_readData)
  );

  always @(posedge clk) begin
    cycles <= cycles + 1;
    mem_waitRequestn <= cycles[2:0] != 0;
    mem_readDataValid <= 0;
    if (mem_write) $fatal(1);
    if (mem_read && mem_waitRequestn) begin
      if (read_active || mem_burstCount == 0 || mem_burstCount > 128 ||
          mem_address < BATCH_BASE_ADDRESS || mem_byteEnable != 4'hf)
        $fatal(1);
      read_index <= (mem_address - BATCH_BASE_ADDRESS) >> 2;
      read_remaining <= mem_burstCount;
      read_active <= 1;
      read_bursts <= read_bursts + 1;
    end else if (read_active) begin
      mem_readData <= memory[read_index];
      mem_readDataValid <= 1;
      read_index <= read_index + 1'b1;
      read_remaining <= read_remaining - 1'b1;
      if (read_remaining == 1) read_active <= 0;
    end
    if (request_pop) begin
      if (received == 0 && request_data !== make_write(22'h010, 32'h100)) $fatal(1);
      if (received >= 1 && received <= 3 &&
          request_data !== make_write(22'h020 + received - 1, 32'h200 + received - 1)) $fatal(1);
      if (received == 4 && request_data !== make_write(22'h030, 32'h300)) $fatal(1);
      if (received >= 5 && received < 37 &&
          request_data !== make_write(22'h100 + received - 5, 32'h1000 + received - 5)) $fatal(1);
      if (received >= 37 && received < 61 &&
          request_data !== make_write(22'h180 + received - 37, 32'h1800 + received - 37)) $fatal(1);
      if (received == 61 && request_data !== make_write(22'h200, 32'h2000)) $fatal(1);
      if (received >= 62 && received < 126 &&
          request_data !== make_write(22'h240 + received - 62, 32'h2400 + received - 62)) $fatal(1);
      received <= received + 1;
    end
  end

  initial begin
    for (i = 0; i < 3; i = i + 1)
      set_batch_record(1, i, 22'h020 + i, 32'h200 + i);
    for (i = 0; i < 32; i = i + 1)
      set_batch_record(2, i, 22'h100 + i, 32'h1000 + i);
    for (i = 0; i < 24; i = i + 1)
      set_batch_record(3, i, 22'h180 + i, 32'h1800 + i);
    for (i = 0; i < 64; i = i + 1)
      set_batch_record(4, i, 22'h240 + i, 32'h2400 + i);

    repeat (5) @(posedge clk);
    reset = 0;
    push(make_write(22'h010, 32'h100));
    push_batch(1, 3);
    push(make_write(22'h030, 32'h300));
    wait (received == 5);

    consume = 0;
    push_batch(2, 32);
    push_batch(3, 24);
    wait (!read_active && dut.occupancy == 56);
    if (request_empty || read_bursts != 3) $fatal(1);
    repeat (8) @(posedge clk);
    if (dut.occupancy != 56) $fatal(1);
    consume = 1;
    wait (received == 61);
    push(make_write(22'h200, 32'h2000));
    wait (received == 62);

    consume = 0;
    push_batch(4, 64);
    wait (!read_active && dut.occupancy == 64);
    if (read_bursts != 4) $fatal(1);
    consume = 1;
    wait (received == 126);
    repeat (5) @(posedge clk);
    if (!request_empty || !ingress_empty) $fatal(1);
    $display("XlsH2fBatchExpanderTb PASS bursts=%0d", read_bursts);
    $finish;
  end

  initial begin
    #2000000;
    $fatal(1, "timeout received=%0d occupancy=%0d", received, dut.occupancy);
  end
endmodule
