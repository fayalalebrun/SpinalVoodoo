`timescale 1ns/1ps

module XlsPaletteMemoryTb;
  reg clk = 0;
  reg reset = 1;
  reg [31:0] read_request = 0;
  reg read_request_valid = 0;
  wire read_request_ready;
  reg [31:0] write = 0;
  reg write_valid = 0;
  wire write_ready;
  wire [95:0] response;
  wire response_valid;
  reg response_ready = 1;

  XlsPaletteMemory dut (
    .clk(clk),
    .reset(reset),
    .read_request(read_request),
    .read_request_valid(read_request_valid),
    .read_request_ready(read_request_ready),
    .write(write),
    .write_valid(write_valid),
    .write_ready(write_ready),
    .response(response),
    .response_valid(response_valid),
    .response_ready(response_ready)
  );

  always #5 clk = ~clk;

  task automatic send_read(input [31:0] value);
    begin
      @(negedge clk);
      read_request = value;
      read_request_valid = 1;
      while (!read_request_ready) @(negedge clk);
      @(posedge clk);
      @(negedge clk);
      read_request_valid = 0;
      read_request = 0;
    end
  endtask

  task automatic send_write(input [31:0] value);
    begin
      @(negedge clk);
      write = value;
      write_valid = 1;
      while (!write_ready) @(negedge clk);
      @(posedge clk);
      @(negedge clk);
      write_valid = 0;
      write = 0;
    end
  endtask

  task automatic expect_response(input [95:0] value);
    begin
      while (!response_valid) @(negedge clk);
      if (response !== value) $fatal(1, "response mismatch: %h != %h", response, value);
      @(posedge clk);
      @(negedge clk);
    end
  endtask

  initial begin
    repeat (3) @(posedge clk);
    @(negedge clk);
    reset = 0;

    send_write({8'h01, 24'h112233});
    send_write({8'h02, 24'h445566});
    send_write({8'h03, 24'h778899});
    send_write({8'h04, 24'haabbcc});

    response_ready = 0;
    send_read({8'h01, 8'h02, 8'h03, 8'h04});
    while (!response_valid) @(negedge clk);
    if (response !== 96'h112233445566778899aabbcc) $fatal(1, "parallel read mismatch");
    repeat (3) begin
      @(negedge clk);
      if (!response_valid || response !== 96'h112233445566778899aabbcc)
        $fatal(1, "response changed under backpressure");
      if (read_request_ready) $fatal(1, "request accepted while response blocked");
    end
    response_ready = 1;
    @(posedge clk);
    @(negedge clk);
    if (response_valid) $fatal(1, "response did not clear");

    $display("XlsPaletteMemoryTb PASS");
    $finish;
  end
endmodule
