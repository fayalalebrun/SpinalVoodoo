`timescale 1ns/1ps

module XlsFramebufferWriteBufferTb;
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
  reg mem_waitRequestn = 0;
  wire [10:0] mem_burstCount;
  wire [31:0] mem_address;
  wire [3:0] mem_byteEnable;
  wire [31:0] mem_writeData;
  reg flush = 0;
  reg block_line_valid = 0;
  reg [15:0] block_line_tag = 0;
  wire busy;
  wire update_valid;
  wire [21:0] update_address;
  wire [31:0] update_data;
  wire [3:0] update_mask;

  always #5 clk = ~clk;

  XlsFramebufferWriteBuffer #(
    .BASE_ADDRESS(32'h3f400000), .SPAN_WORDS(128), .MAX_REQUESTS_PER_RUN(8),
    .RESPONSE_DEPTH(4)
  ) dut (
    .clk(clk), .reset(reset), .request(request), .request_valid(request_valid),
    .request_ready(request_ready), .response(response), .response_valid(response_valid),
    .response_ready(response_ready), .mem_read(mem_read), .mem_write(mem_write),
    .mem_waitRequestn(mem_waitRequestn), .mem_burstCount(mem_burstCount),
    .mem_address(mem_address), .mem_byteEnable(mem_byteEnable),
    .mem_writeData(mem_writeData), .mem_readDataValid(1'b0), .mem_readData(32'b0),
    .flush(flush), .block_line_valid(block_line_valid), .block_line_tag(block_line_tag),
    .busy(busy), .update_valid(update_valid),
    .update_address(update_address), .update_data(update_data), .update_mask(update_mask)
  );

  function automatic [65:0] make_write(
    input [21:0] address, input [31:0] data, input [3:0] mask
  );
    make_write = {7'd1, 1'b1, address, data, mask};
  endfunction

  task automatic push_write(input [21:0] address, input [31:0] data, input [3:0] mask);
    begin
      request <= make_write(address, data, mask);
      request_valid <= 1;
      do @(posedge clk); while (!request_ready);
      if (!update_valid || update_address != address || update_data != data ||
          update_mask != mask) $fatal(1, "bad cache update");
      request_valid <= 0;
    end
  endtask

  task automatic expect_beat(
    input [10:0] count, input [31:0] address, input [31:0] data, input [3:0] mask
  );
    begin
      #1;
      while (!mem_write) begin
        @(posedge clk);
        #1;
      end
      if (mem_read || mem_burstCount !== count || mem_address !== address ||
          mem_writeData !== data || mem_byteEnable !== mask)
        $fatal(1, "bad write beat count=%0d address=%08x data=%08x mask=%x",
            mem_burstCount, mem_address, mem_writeData, mem_byteEnable);
      mem_waitRequestn <= 1;
      @(posedge clk);
      mem_waitRequestn <= 0;
    end
  endtask

  task automatic pop_response(input success, input [31:0] data);
    begin
      response_ready <= 1;
      #1;
      while (!response_valid) begin
        @(posedge clk);
        #1;
      end
      if (response[0] !== success || response[32:1] !== data)
        $fatal(1, "bad response success=%x data=%08x", response[0], response[32:1]);
      @(posedge clk);
      response_ready <= 0;
    end
  endtask

  initial begin
    repeat (3) @(posedge clk);
    reset <= 0;

    push_write(22'h10, 32'haabbccdd, 4'b0011);
    push_write(22'h10, 32'h11220000, 4'b1100);
    push_write(22'h11, 32'h55667788, 4'b1111);
    #1;
    if (!response_valid) $fatal(1, "response waited for memory drain");
    flush <= 1;
    request <= make_write(22'h12, 32'habcdef01, 4'hf);
    request_valid <= 1;
    @(posedge clk);
    if (request_ready) $fatal(1, "flush accepted a request");
    request_valid <= 0;
    while (!mem_write) @(posedge clk);
    if (!busy) $fatal(1, "busy dropped before drain");
    if (mem_burstCount !== 11'd2 || mem_address !== 32'h3f400040 ||
        mem_writeData !== 32'h1122ccdd || mem_byteEnable !== 4'hf)
      $fatal(1, "bad merged beat");
    repeat (2) begin
      @(posedge clk);
      if (!mem_write || mem_burstCount !== 11'd2 || mem_address !== 32'h3f400040 ||
          mem_writeData !== 32'h1122ccdd || mem_byteEnable !== 4'hf)
        $fatal(1, "stalled beat changed");
    end
    mem_waitRequestn <= 1;
    @(posedge clk);
    mem_waitRequestn <= 0;
    expect_beat(11'd2, 32'h3f400040, 32'h55667788, 4'hf);
    flush <= 0;
    pop_response(1'b1, 32'haabbccdd);
    pop_response(1'b1, 32'h11220000);
    pop_response(1'b1, 32'h55667788);
    while (busy) @(posedge clk);

    block_line_valid <= 1;
    block_line_tag <= 16'h0;
    request <= make_write(22'h10, 32'h01234567, 4'hf);
    request_valid <= 1;
    @(posedge clk);
    if (request_ready) $fatal(1, "hazard accepted a request");
    request_valid <= 0;
    block_line_valid <= 0;
    @(posedge clk);

    push_write(22'h20, 32'h01020304, 4'hf);
    push_write(22'h30, 32'h11121314, 4'hf);
    request <= make_write(22'h40, 32'h21222324, 4'hf);
    request_valid <= 1;
    @(posedge clk);
    if (request_ready) $fatal(1, "full slots accepted a request");
    request_valid <= 0;
    expect_beat(11'd1, 32'h3f400080, 32'h01020304, 4'hf);
    expect_beat(11'd1, 32'h3f4000c0, 32'h11121314, 4'hf);
    pop_response(1'b1, 32'h01020304);
    pop_response(1'b1, 32'h11121314);
    while (busy) @(posedge clk);

    push_write(22'h50, 32'h00000050, 4'hf);
    push_write(22'h51, 32'h00000051, 4'hf);
    push_write(22'h52, 32'h00000052, 4'hf);
    push_write(22'h53, 32'h00000053, 4'hf);
    request <= make_write(22'h54, 32'h00000054, 4'hf);
    request_valid <= 1;
    @(posedge clk);
    if (request_ready) $fatal(1, "full response queue accepted a request");
    request_valid <= 0;
    expect_beat(11'd4, 32'h3f400140, 32'h00000050, 4'hf);
    expect_beat(11'd4, 32'h3f400140, 32'h00000051, 4'hf);
    expect_beat(11'd4, 32'h3f400140, 32'h00000052, 4'hf);
    expect_beat(11'd4, 32'h3f400140, 32'h00000053, 4'hf);
    pop_response(1'b1, 32'h00000050);
    pop_response(1'b1, 32'h00000051);
    pop_response(1'b1, 32'h00000052);
    pop_response(1'b1, 32'h00000053);
    while (busy) @(posedge clk);

    request <= {7'd2, 1'b1, 22'h7f, 32'hdeadbeef, 4'hf};
    request_valid <= 1;
    do @(posedge clk); while (!request_ready);
    request_valid <= 0;
    if (mem_write) $fatal(1, "illegal request reached memory");
    pop_response(1'b0, 32'hdeadbeef);

    push_write(22'h30, 32'h12345678, 4'hf);
    while (!mem_write) @(posedge clk);
    push_write(22'h40, 32'hcafebabe, 4'hf);
    repeat (2) @(posedge clk);
    push_write(22'h41, 32'h0badf00d, 4'hf);
    expect_beat(11'd1, 32'h3f4000c0, 32'h12345678, 4'hf);
    expect_beat(11'd2, 32'h3f400100, 32'hcafebabe, 4'hf);
    expect_beat(11'd2, 32'h3f400100, 32'h0badf00d, 4'hf);
    pop_response(1'b1, 32'h12345678);
    pop_response(1'b1, 32'hcafebabe);
    pop_response(1'b1, 32'h0badf00d);
    while (busy) @(posedge clk);

    push_write(22'h60, 32'h00000060, 4'hf);
    push_write(22'h61, 32'h00000061, 4'hf);
    push_write(22'h62, 32'h00000062, 4'hf);
    push_write(22'h63, 32'h00000063, 4'hf);
    while (!response_valid) @(posedge clk);
    response_ready <= 1;
    for (integer response_index = 0; response_index < 4; response_index = response_index + 1) begin
      #1;
      if (!response_valid) $fatal(1, "response stream had a bubble");
      if (response !== {32'h00000060 + response_index, 1'b1})
        $fatal(1, "response stream was reordered");
      @(posedge clk);
    end
    response_ready <= 0;
    reset <= 1;
    @(posedge clk);
    reset <= 0;

    push_write(22'h50, 32'hfeedface, 4'hf);
    reset <= 1;
    @(posedge clk);
    reset <= 0;
    #1;
    if (busy || mem_write || response_valid) $fatal(1, "reset did not discard pending state");

    $display("XlsFramebufferWriteBufferTb PASS");
    $finish;
  end
endmodule
