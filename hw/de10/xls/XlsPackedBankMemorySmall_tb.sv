`timescale 1ns/1ps

module XlsPackedBankMemorySmallTb #(
  parameter integer PRIMARY_NUMWORDS = 2048,
  parameter integer PRIMARY_ADDRESS_WIDTH = 11
);
  reg clk = 0;
  reg reset = 1;
  reg [261:0] slow_request = 0;
  reg slow_request_valid = 0;
  wire slow_request_ready;
  wire [280:0] slow_response;
  wire slow_response_valid;
  reg slow_response_ready = 0;
  reg [108:0] fast_request = 0;
  reg fast_request_valid = 0;
  wire fast_request_ready;
  wire [280:0] fast_response;
  wire fast_response_valid;
  reg fast_response_ready = 0;

  always #5 clk = ~clk;

  XlsPackedBankMemoryVector #(
    .PRIMARY_NUMWORDS(PRIMARY_NUMWORDS), .PRIMARY_ADDRESS_WIDTH(PRIMARY_ADDRESS_WIDTH)
  ) dut (.*);

  task write_word(input integer bank, input [21:0] address,
                  input [31:0] data, input [3:0] mask);
    begin
      @(negedge clk);
      slow_request = 0;
      slow_request[21 + bank * 59 +: 59] = {1'b1, address, data, mask};
      slow_request[257 + bank] = 1;
      slow_request_valid = 1;
      do @(posedge clk); while (!slow_request_ready);
      @(negedge clk);
      slow_request_valid = 0;
      while (!slow_response_valid) @(negedge clk);
      if (!slow_response[21 + bank * 65]) $fatal(1, "write failed");
      slow_response_ready = 1;
      @(negedge clk);
      slow_response_ready = 0;
    end
  endtask

  task read_pair(input integer bank, input [21:0] first, input [21:0] second,
                 input [63:0] expected);
    reg [280:0] saved_response;
    begin
      @(negedge clk);
      fast_request = 0;
      fast_request[42:21] = first;
      fast_request[64:43] = second;
      fast_request[8:5] = bank << 1;
      fast_request[12:9] = bank << 1;
      fast_request[4:1] = 3;
      fast_request[0] = 1;
      fast_request_valid = 1;
      do @(posedge clk); while (!fast_request_ready);
      @(negedge clk);
      fast_request_valid = 0;
      while (!fast_response_valid) @(negedge clk);
      if (!fast_response[21 + bank * 65] ||
          fast_response[22 + bank * 65 +: 64] !== expected)
        $fatal(1, "bank=%0d first=%h second=%h expected=%h actual=%h",
               bank, first, second, expected, fast_response[22 + bank * 65 +: 64]);
      saved_response = fast_response;
      repeat (3) begin
        @(negedge clk);
        if (!fast_response_valid || fast_response !== saved_response)
          $fatal(1, "response changed under backpressure");
      end
      fast_response_ready = 1;
      @(negedge clk);
      fast_response_ready = 0;
    end
  endtask

  initial begin
    repeat (3) @(negedge clk);
    reset = 0;
    for (integer bank = 0; bank < 4; bank = bank + 1) begin
      write_word(bank, 22'h000, 32'h12345678 + bank, 4'hf);
      write_word(bank, 22'h3ff, 32'habcdef00 + bank, 4'hf);
      write_word(bank, 22'h400, 32'h56789abc + bank, 4'hf);
      write_word(bank, 22'h157ff, 32'h11223344 + bank, 4'hf);
    end
    for (integer bank = 0; bank < 4; bank = bank + 1) begin
      read_pair(bank, 22'h1000, 22'h17ff,
                {32'h11223344 + bank, 32'h12345678 + bank});
      read_pair(bank, 22'h13ff, 22'h1400,
                {32'h56789abc + bank, 32'habcdef00 + bank});
      read_pair(bank, 22'h000, 22'h17f,
                {32'h11223344 + bank, 32'h12345678 + bank});
      write_word(bank, 22'h157ff, 32'haabbccdd, 4'h5);
      read_pair(bank, 22'h17ff, 22'h17f, 64'h11bb33dd11bb33dd);
      read_pair(bank, 22'h1000, 22'h13ff,
                {32'habcdef00 + bank, 32'h12345678 + bank});
    end
    if (PRIMARY_NUMWORDS == 4096) begin
      for (integer bank = 0; bank < 4; bank = bank + 1) begin
        write_word(bank, 22'h800, 32'h87654321 + bank, 4'hf);
        write_word(bank, 22'h15fff, 32'h55667788 + bank, 4'hf);
        read_pair(bank, 22'h1000, 22'h1800,
                  {32'h87654321 + bank, 32'h12345678 + bank});
        read_pair(bank, 22'h17ff, 22'h1fff,
                  {32'h55667788 + bank, 32'h11bb33dd});
        read_pair(bank, 22'h17f, 22'h1fff,
                  {32'h55667788 + bank, 32'h55667788 + bank});
        write_word(bank, 22'h15fff, 32'haabbccdd, 4'h5);
        read_pair(bank, 22'h1fff, 22'h17f, 64'h55bb77dd55bb77dd);
      end
    end
    $display("XlsPackedBankMemorySmallTb PASS words=%0d", PRIMARY_NUMWORDS);
    $finish;
  end

  initial begin
    #100000;
    $fatal(1, "timeout");
  end
endmodule
