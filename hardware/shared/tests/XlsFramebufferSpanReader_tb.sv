`timescale 1ns/1ps

module XlsFramebufferSpanReaderTb;
  reg clk = 0;
  reg reset = 1;
  reg [45:0] span = 0;
  reg span_valid = 0;
  wire span_ready;
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
  wire debug_data_stall;
  wire debug_inflight_stall;
  wire debug_unissued_stall;
  wire debug_response_stall;

  always #5 clk = ~clk;

  XlsFramebufferSpanReader #(
    .BASE_ADDRESS(32'h1000), .ADDRESS_WORDS(128), .SPAN_DEPTH(4),
    .DATA_DEPTH(16), .ROUTE_DEPTH(4), .RESPONSE_DEPTH(4), .MAX_BURST(4)
  ) dut (
    .clk(clk), .reset(reset), .span(span), .span_valid(span_valid), .span_ready(span_ready),
    .request(request), .request_valid(request_valid), .request_ready(request_ready),
    .response(response), .response_valid(response_valid), .response_ready(response_ready),
    .mem_read(mem_read), .mem_write(mem_write), .mem_waitRequestn(mem_waitRequestn),
    .mem_burstCount(mem_burstCount), .mem_address(mem_address),
    .mem_byteEnable(mem_byteEnable), .mem_writeData(mem_writeData),
    .mem_readDataValid(mem_readDataValid), .mem_readData(mem_readData),
    .invalidate(invalidate), .debug_data_stall(debug_data_stall),
    .debug_inflight_stall(debug_inflight_stall),
    .debug_unissued_stall(debug_unissued_stall),
    .debug_response_stall(debug_response_stall)
  );

  function automatic [65:0] make_request(input [21:0] address);
    make_request = {7'd1, 1'b0, address, 32'b0, 4'hf};
  endfunction

  task automatic push_span(input [22:0] first_lane, input [22:0] last_lane,
                           input [31:0] address, input [10:0] count);
    begin
      @(negedge clk);
      span <= {first_lane, last_lane};
      span_valid <= 1;
      #1;
      while (!span_ready) begin
        @(posedge clk);
        @(negedge clk);
      end
      @(posedge clk);
      @(negedge clk);
      span_valid <= 0;
      while (!mem_read) @(negedge clk);
      if (mem_write || mem_address !== address || mem_burstCount !== count)
        $fatal(1, "bad span command");
      @(posedge clk);
    end
  endtask

  task automatic send_word(input [31:0] data);
    begin
      @(negedge clk);
      mem_readData <= data;
      mem_readDataValid <= 1;
      @(posedge clk);
      @(negedge clk);
      mem_readDataValid <= 0;
    end
  endtask

  task automatic push_request(input [21:0] address);
    begin
      @(negedge clk);
      request <= make_request(address);
      request_valid <= 1;
      #1;
      while (!request_ready) begin
        @(posedge clk);
        @(negedge clk);
      end
      @(posedge clk);
      @(negedge clk);
      request_valid <= 0;
    end
  endtask

  task automatic pop_response(input [31:0] data);
    begin
      @(negedge clk);
      response_ready <= 1;
      #1;
      while (!response_valid) begin
        @(posedge clk);
        @(negedge clk);
      end
      if (response !== {data, 1'b1}) $fatal(1, "bad response");
      @(posedge clk);
      @(negedge clk);
      response_ready <= 0;
    end
  endtask

  initial begin
    #100000;
    $fatal(1, "timeout span=%0d/%0d issue=%0d/%0d consume=%0d/%0d data=%0d route=%0d response=%0d direct=%0d request=%0d/%0d mem=%0d",
           span_valid, span_ready, dut.issue_active, dut.issue_words_remaining,
           dut.consume_active, dut.consume_lane, dut.data_count, dut.route_count,
           dut.response_count, dut.direct_outstanding, request_valid, request_ready, mem_read);
  end

  initial begin
    repeat (3) @(posedge clk);
    reset <= 0;

    push_span(23'd3, 23'd6, 32'h1004, 11'd3);
    repeat (2) @(posedge clk);
    @(negedge clk);
    request <= make_request(22'd1);
    request_valid <= 1;
    #1;
    if (request_ready || !debug_data_stall || !debug_inflight_stall || debug_unissued_stall)
      $fatal(1, "data stall classification failed");
    request_valid <= 0;
    send_word(32'h11110000);
    send_word(32'h22220000);
    send_word(32'h33330000);
    push_request(22'd1);
    push_request(22'd2);
    push_request(22'd2);
    push_request(22'd3);
    @(negedge clk);
    request <= make_request(22'd3);
    request_valid <= 1;
    #1;
    if (request_ready || !debug_response_stall)
      $fatal(1, "response stall classification failed");
    request_valid <= 0;
    pop_response(32'h11110000);
    pop_response(32'h22220000);
    pop_response(32'h22220000);
    pop_response(32'h33330000);

    push_span(23'd8, 23'd15, 32'h1010, 11'd4);
    send_word(32'h44440000);
    send_word(32'h55550000);
    send_word(32'h66660000);
    send_word(32'h77770000);
    push_request(22'd4);
    push_request(22'd6);
    pop_response(32'h44440000);
    pop_response(32'h66660000);

    @(negedge clk);
    request <= make_request(22'd20);
    request_valid <= 1;
    while (!mem_read || mem_burstCount != 1) @(negedge clk);
    if (mem_address !== 32'h1050) $fatal(1, "bad direct address");
    @(posedge clk);
    @(negedge clk);
    request_valid <= 0;
    send_word(32'hdeadbeef);
    pop_response(32'hdeadbeef);



    $display("XlsFramebufferSpanReaderTb PASS");
    $finish;
  end
endmodule
