`timescale 1ns/1ps

module XlsRenderCycleProfilerTb;
  reg clk = 0;
  reg reset = 1;
  reg clear = 0;
  reg active = 0;
  reg fast_bank_request_blocked = 0;
  reg fast_bank_response_blocked = 0;
  reg [5:0] read_index = 0;
  wire [31:0] read_data;

  XlsRenderCycleProfiler dut (
    .clk(clk), .reset(reset), .clear(clear), .active(active),
    .fast_bank_request_blocked(fast_bank_request_blocked),
    .fast_bank_response_blocked(fast_bank_response_blocked),
    .read_index(read_index), .read_data(read_data)
  );

  always #5 clk = ~clk;

  task step;
    begin
      @(posedge clk);
      #1;
    end
  endtask

  task expect_counter(input [5:0] index, input [31:0] expected);
    begin
      read_index = index;
      #1;
      if (read_data !== expected)
        $fatal(1, "counter %0d got %0d expected %0d", index, read_data, expected);
    end
  endtask

  initial begin
    repeat (2) step();
    reset = 0;
    active = 1;
    fast_bank_request_blocked = 1;
    fast_bank_response_blocked = 1;
    step();
    fast_bank_request_blocked = 0;
    step();
    fast_bank_response_blocked = 0;
    step();
    active = 0;

    expect_counter(1, 1);
    expect_counter(2, 2);

    clear = 1;
    step();
    clear = 0;
    expect_counter(1, 0);
    expect_counter(2, 0);
    $display("XlsRenderCycleProfilerTb PASS");
    $finish;
  end
endmodule
