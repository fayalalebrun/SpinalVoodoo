`timescale 1ns/1ps

module XlsFramebufferReadPrefetchTb;
  reg clk = 0;
  reg reset = 1;
  reg [65:0] request = 0;
  reg request_valid = 0;
  wire request_ready;
  wire [32:0] response;
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
  reg invalidate = 0;
  reg write_pending = 0;
  wire miss_waiting;
  wire fill_active;
  wire [15:0] fill_line_tag;
  reg update_valid = 0;
  reg [21:0] update_address = 0;
  reg [31:0] update_data = 0;
  reg [3:0] update_mask = 0;
  reg debug_clear = 0;
  reg [3:0] debug_read_index = 0;
  wire [31:0] debug_read_data;
  integer i;

  always #5 clk = ~clk;

  XlsFramebufferReadPrefetch #(
    .BASE_ADDRESS(32'h00001000), .ADDRESS_WORDS(128), .LINE_WORDS(4),
    .LINE_COUNT(4), .REQUEST_DEPTH(8)
  ) dut (
    .clk(clk), .reset(reset), .request(request), .request_valid(request_valid),
    .request_ready(request_ready), .response(response), .response_valid(response_valid),
    .response_ready(response_ready), .mem_read(mem_read), .mem_write(mem_write),
    .mem_waitRequestn(mem_waitRequestn), .mem_burstCount(mem_burstCount),
    .mem_address(mem_address), .mem_byteEnable(mem_byteEnable),
    .mem_writeData(mem_writeData), .mem_readDataValid(mem_readDataValid),
    .mem_readData(mem_readData), .invalidate(invalidate), .write_pending(write_pending),
    .miss_waiting(miss_waiting), .fill_active(fill_active), .fill_line_tag(fill_line_tag),
    .update_valid(update_valid), .update_address(update_address), .update_data(update_data),
    .update_mask(update_mask), .debug_clear(debug_clear), .debug_read_index(debug_read_index),
    .debug_read_data(debug_read_data)
  );

  function automatic [65:0] make_request(input [21:0] address, input [6:0] count, input write);
    make_request = {count, write, address, 32'b0, 4'hf};
  endfunction

  task automatic push_request(input [21:0] address, input [6:0] count, input write,
                              input expect_memory, input [31:0] expected_memory_address);
    begin
      @(negedge clk);
      request <= make_request(address, count, write);
      request_valid <= 1;
      #1;
      while (!request_ready) @(negedge clk);
      if (expect_memory) begin
        if (!mem_read || mem_write || mem_burstCount !== 11'd4 ||
            mem_address !== expected_memory_address || mem_byteEnable !== 4'hf)
          $fatal(1, "bad prefetch request");
      end else if (mem_read) begin
        $fatal(1, "unexpected prefetch request");
      end
      @(posedge clk);
      @(negedge clk);
      request_valid <= 0;
    end
  endtask

  task automatic send_fill(input [31:0] base);
    begin
      for (i = 0; i < 4; i = i + 1) begin
        @(negedge clk);
        mem_readData <= base + i;
        mem_readDataValid <= 1;
        @(posedge clk);
      end
      @(negedge clk);
      mem_readDataValid <= 0;
    end
  endtask

  task automatic clear_debug;
    begin
      @(negedge clk);
      debug_clear <= 1;
      @(posedge clk);
      @(negedge clk);
      debug_clear <= 0;
    end
  endtask

  task automatic expect_debug_nonzero(input [3:0] index);
    begin
      debug_read_index = index;
      #1;
      if (debug_read_data == 0) $fatal(1, "debug counter %0d is zero", index);
    end
  endtask

  task automatic pop_response(input success, input [31:0] data);
    begin
      @(negedge clk);
      response_ready <= 1;
      while (!response_valid) @(negedge clk);
      if (response[0] !== success || response[32:1] !== data) $fatal(1, "bad response");
      @(posedge clk);
      @(negedge clk);
      response_ready <= 0;
    end
  endtask

  initial begin
    repeat (3) @(posedge clk);
    reset <= 0;

    push_request(22'd5, 7'd1, 1'b0, 1'b1, 32'h00001010);
    push_request(22'd6, 7'd1, 1'b0, 1'b0, 32'b0);
    push_request(22'd12, 7'd1, 1'b0, 1'b1, 32'h00001030);
    send_fill(32'h10000000);
    send_fill(32'h20000000);
    pop_response(1'b1, 32'h10000001);
    pop_response(1'b1, 32'h10000002);
    pop_response(1'b1, 32'h20000000);

    @(negedge clk);
    update_address <= 22'd5;
    update_data <= 32'hdeadbeef;
    update_mask <= 4'hf;
    update_valid <= 1;
    @(posedge clk);
    @(negedge clk);
    update_valid <= 0;
    push_request(22'd5, 7'd1, 1'b0, 1'b0, 32'b0);
    response_ready <= 0;
    while (!response_valid) @(negedge clk);
    repeat (3) begin
      @(negedge clk);
      if (!response_valid || response[32:1] !== 32'h10000001)
        $fatal(1, "stale response not held");
    end
    pop_response(1'b1, 32'h10000001);

    push_request(22'd5, 7'd2, 1'b0, 1'b0, 32'b0);
    pop_response(1'b0, 32'b0);

    @(negedge clk);
    invalidate <= 1;
    @(posedge clk);
    @(negedge clk);
    invalidate <= 0;
    push_request(22'd5, 7'd1, 1'b0, 1'b1, 32'h00001010);
    send_fill(32'h30000000);
    pop_response(1'b1, 32'h30000001);

    if (miss_waiting || fill_active || fill_line_tag != 0)
      $fatal(1, "coherence outputs enabled");

    clear_debug();
    @(negedge clk);
    invalidate <= 1;
    @(posedge clk);
    @(negedge clk);
    invalidate <= 0;
    push_request(22'd0, 7'd1, 1'b0, 1'b1, 32'h00001000);
    push_request(22'd4, 7'd1, 1'b0, 1'b1, 32'h00001010);
    push_request(22'd8, 7'd1, 1'b0, 1'b1, 32'h00001020);
    push_request(22'd12, 7'd1, 1'b0, 1'b1, 32'h00001030);
    @(negedge clk);
    request <= make_request(22'd16, 7'd1, 1'b0);
    request_valid <= 1;
    #1;
    if (request_ready) $fatal(1, "fill saturation did not stall");
    repeat (3) @(posedge clk);
    @(negedge clk);
    request_valid <= 0;
    expect_debug_nonzero(0);
    expect_debug_nonzero(2);
    send_fill(32'h40000000);
    send_fill(32'h50000000);
    send_fill(32'h60000000);
    send_fill(32'h70000000);
    expect_debug_nonzero(5);
    expect_debug_nonzero(6);
    expect_debug_nonzero(8);
    expect_debug_nonzero(9);
    @(negedge clk);
    request <= make_request(22'd16, 7'd1, 1'b0);
    request_valid <= 1;
    #1;
    if (request_ready) $fatal(1, "pinned lines did not stall");
    repeat (2) @(posedge clk);
    @(negedge clk);
    request_valid <= 0;
    expect_debug_nonzero(3);
    pop_response(1'b1, 32'h40000000);
    pop_response(1'b1, 32'h50000000);
    pop_response(1'b1, 32'h60000000);
    pop_response(1'b1, 32'h70000000);

    clear_debug();
    for (i = 0; i < 8; i = i + 1)
      push_request(22'd0, 7'd1, 1'b0, 1'b0, 32'b0);
    @(negedge clk);
    request <= make_request(22'd0, 7'd1, 1'b0);
    request_valid <= 1;
    repeat (2) @(posedge clk);
    @(negedge clk);
    request_valid <= 0;
    expect_debug_nonzero(1);
    expect_debug_nonzero(7);
    for (i = 0; i < 8; i = i + 1)
      pop_response(1'b1, 32'h40000000);

    clear_debug();
    @(negedge clk);
    invalidate <= 1;
    @(posedge clk);
    @(negedge clk);
    invalidate <= 0;
    mem_waitRequestn <= 0;
    request <= make_request(22'd20, 7'd1, 1'b0);
    request_valid <= 1;
    repeat (3) @(posedge clk);
    @(negedge clk);
    expect_debug_nonzero(4);
    mem_waitRequestn <= 1;
    @(posedge clk);
    @(negedge clk);
    request_valid <= 0;
    send_fill(32'h80000000);
    pop_response(1'b1, 32'h80000000);
    expect_debug_nonzero(5);
    expect_debug_nonzero(8);
    expect_debug_nonzero(9);

    $display("XlsFramebufferReadPrefetchTb PASS");
    $finish;
  end
endmodule
