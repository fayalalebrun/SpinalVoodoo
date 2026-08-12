`timescale 1ns/1ps

module XlsPackedBankMemoryTb;
  reg clk = 0;
  reg reset = 1;
  reg [58:0] command = 0;
  reg command_valid = 0;
  wire command_ready;
  wire [32:0] response;
  wire response_valid;
  reg response_ready = 0;

  always #5 clk = ~clk;

  XlsPackedBankMemory dut (
    .clk(clk), .reset(reset), .command(command), .command_valid(command_valid),
    .command_ready(command_ready), .response(response), .response_valid(response_valid),
    .response_ready(response_ready)
  );

  task transact(input [58:0] value, input [31:0] expected);
    begin
      command = value;
      command_valid = 1;
      while (!command_ready) @(posedge clk);
      @(posedge clk);
      #1;
      command_valid = 0;
      while (!response_valid) begin
        @(posedge clk);
        #1;
      end
      if (!response[0] || response[32:1] != expected) $fatal(1);
      response_ready = 1;
      @(posedge clk);
      #1;
      response_ready = 0;
    end
  endtask

  reg [58:0] value;
  initial begin
    #1000;
    $fatal(1);
  end

  initial begin
    repeat (2) @(posedge clk);
    reset = 0;

    value = 0;
    value[58] = 1;
    value[57:36] = (2 << 6) | 3;
    value[35:4] = 32'h12345678;
    value[3:0] = 4'hf;
    transact(value, 32'h12345678);

    value = 0;
    value[57:36] = (2 << 6) | 3;
    transact(value, 32'h12345678);

    value = 0;
    value[58] = 1;
    value[57:36] = (2 << 6) | 3;
    value[35:4] = 32'haabbccdd;
    value[3:0] = 4'h5;
    transact(value, 32'h12bb56dd);

    value = 0;
    value[58] = 1;
    value[57:36] = (5 << 6) | 7;
    value[35:4] = 32'hcafebabe;
    value[3:0] = 4'hf;
    transact(value, 32'hcafebabe);

    value = 0;
    value[58] = 1;
    value[57:36] = (5 << 6) | 7;
    value[35:4] = 32'hdeadbeef;
    value[3:0] = 4'h3;
    transact(value, 32'hcafebeef);

    value = 0;
    value[57:36] = (5 << 6) | 7;
    transact(value, 32'hcafebeef);

    $display("XlsPackedBankMemory passed");
    $finish;
  end
endmodule
