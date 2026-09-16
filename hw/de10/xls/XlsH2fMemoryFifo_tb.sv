`timescale 1ns/1ps

module XlsH2fMemoryFifoTb;
  reg clk = 0;
  reg reset = 1;
  reg [59:0] ingress_data = 0;
  reg ingress_empty = 1;
  wire ingress_pop;
  wire [59:0] request_data;
  wire request_empty;
  reg consume = 0;
  integer cycles = 0;
  wire request_pop = consume && !request_empty && cycles[3:0] != 0;
  reg stalled = 0;
  reg [59:0] stalled_data;
  wire mem_read;
  wire mem_write;
  reg mem_waitRequestn = 1;
  wire [10:0] mem_burstCount;
  wire [31:0] mem_address;
  wire [3:0] mem_byteEnable;
  wire [31:0] mem_writeData;
  reg mem_readDataValid = 0;
  reg [31:0] mem_readData = 0;
  reg [31:0] memory [0:511];
  reg [8:0] read_index = 0;
  reg [10:0] read_remaining = 0;
  reg read_active = 0;
  reg [9:0] write_index = 0;
  reg [10:0] write_remaining = 0;
  reg write_active = 0;
  integer received = 0;
  integer read_bursts = 0;
  integer writes = 0;

  always #5 clk = ~clk;

  XlsH2fMemoryFifo #(
    .EGRESS_ADDRESS_WIDTH(4),
    .RING_ADDRESS_WIDTH(8),
    .BASE_ADDRESS(32'h00001000)
  ) dut (
    .clk(clk), .reset(reset),
    .ingress_data(ingress_data), .ingress_empty(ingress_empty), .ingress_pop(ingress_pop),
    .request_data(request_data), .request_empty(request_empty), .request_pop(request_pop),
    .mem_read(mem_read), .mem_write(mem_write), .mem_waitRequestn(mem_waitRequestn),
    .mem_burstCount(mem_burstCount), .mem_address(mem_address),
    .mem_byteEnable(mem_byteEnable), .mem_writeData(mem_writeData),
    .mem_readDataValid(mem_readDataValid), .mem_readData(mem_readData)
  );

  task push(input integer value);
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

  task push_batch(input integer first, input integer count);
    integer i;
    begin
      for (i = 0; i < count; i = i + 1) push(first + i);
    end
  endtask

  always @(posedge clk) begin
    cycles <= cycles + 1;
    mem_waitRequestn <= cycles[2:0] != 0;
    mem_readDataValid <= 0;
    if (mem_write && mem_waitRequestn) begin
      if (mem_address < 32'h1000 || mem_address >= 32'h1800 || mem_byteEnable != 4'hf)
        $fatal(1);
      if (write_active && write_index >= 512) $fatal(1);
      memory[write_active ? write_index : (mem_address - 32'h1000) >> 2] <= mem_writeData;
      writes <= writes + 1;
      if (!write_active) begin
        write_index <= ((mem_address - 32'h1000) >> 2) + 1'b1;
        write_remaining <= mem_burstCount - 1'b1;
        write_active <= mem_burstCount != 1;
      end else begin
        write_index <= write_index + 1'b1;
        write_remaining <= write_remaining - 1'b1;
        if (write_remaining == 1) write_active <= 0;
      end
    end
    if (mem_read && mem_waitRequestn) begin
      if (read_active || write_active || mem_burstCount == 0 || mem_burstCount > 128) $fatal(1);
      read_index <= (mem_address - 32'h1000) >> 2;
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
    if (!reset && stalled && (request_empty || request_data !== stalled_data))
      $fatal(1, "request changed under backpressure");
    stalled <= !reset && !request_empty && !request_pop;
    stalled_data <= request_data;
    if (request_pop) begin
      if (request_data !== received) begin
        $display("FAIL received=%0d data=%h", received, request_data);
        $fatal(1);
      end
      received <= received + 1;
    end
  end

  initial begin
    repeat (5) @(posedge clk);
    reset = 0;
    push_batch(0, 200);
    consume = 1;
    wait (received == 200);
    consume = 0;
    push_batch(200, 200);
    consume = 1;
    wait (received == 400);
    repeat (20) @(posedge clk);
    if (!request_empty || read_bursts == 0 || writes == 0) $fatal(1);
    consume = 0;
    push_batch(400, 20);
    repeat (100) @(negedge clk);
    reset = 1;
    repeat (3) @(negedge clk);
    if (!request_empty) $fatal(1, "reset retained a queued request");
    reset = 0;
    push_batch(400, 40);
    consume = 1;
    wait (received == 440);
    repeat (20) @(posedge clk);
    if (!request_empty) $fatal(1, "request duplicated after warm reset");
    $display("XlsH2fMemoryFifoTb PASS bursts=%0d writes=%0d", read_bursts, writes);
    $finish;
  end

  initial begin
    #2000000;
    $fatal(1, "timeout received=%0d", received);
  end
endmodule
