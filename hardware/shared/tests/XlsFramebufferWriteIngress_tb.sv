`timescale 1ns/1ps

module XlsFramebufferWriteIngressTb;
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
  wire busy;

  always #5 clk = ~clk;

  always @(negedge clk) begin
    if (!reset && (dut.ingress_count != 0 || dut.engine_busy) && !busy)
      $fatal(1, "busy dropped while writes were pending");
  end

  initial begin
    #100000;
    $fatal(1, "write ingress test timed out");
  end

  XlsFramebufferWriteIngress #(
    .BASE_ADDRESS(32'h3f400000), .SPAN_WORDS(128), .RESPONSE_DEPTH(8),
    .INGRESS_DEPTH(8), .START_WATERMARK(4), .IDLE_TIMEOUT(8)
  ) dut (
    .clk(clk), .reset(reset), .request(request), .request_valid(request_valid),
    .request_ready(request_ready), .response(response), .response_valid(response_valid),
    .response_ready(response_ready), .mem_read(mem_read), .mem_write(mem_write),
    .mem_waitRequestn(mem_waitRequestn), .mem_burstCount(mem_burstCount),
    .mem_address(mem_address), .mem_byteEnable(mem_byteEnable),
    .mem_writeData(mem_writeData), .mem_readDataValid(1'b0), .mem_readData(32'b0),
    .flush(flush), .block_line_valid(1'b0), .block_line_tag(16'b0), .busy(busy),
    .update_valid(), .update_address(), .update_data(), .update_mask(),
    .pending_line0_valid(), .pending_line0_tag(),
    .pending_line1_valid(), .pending_line1_tag()
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
      request_valid <= 0;
    end
  endtask

  task automatic expect_beat(
    input [10:0] count, input [31:0] address, input [31:0] data, input [3:0] mask
  );
    begin
      while (!mem_write) @(posedge clk);
      #1;
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
      while (!response_valid) @(posedge clk);
      #1;
      if (response !== {data, success})
        $fatal(1, "bad response expected=%09x actual=%09x", {data, success}, response);
      response_ready <= 1;
      @(posedge clk);
      response_ready <= 0;
    end
  endtask

  initial begin
    repeat (3) @(posedge clk);
    reset <= 0;

    push_write(22'h10, 32'h0000ccdd, 4'b0011);
    repeat (2) @(posedge clk);
    if (!response_valid || mem_write) $fatal(1, "write was not acknowledged and buffered");
    push_write(22'h10, 32'h11220000, 4'b1100);
    push_write(22'h11, 32'h55667788, 4'hf);
    if (mem_write) $fatal(1, "queue drained below watermark");
    push_write(22'h12, 32'h99aabbcc, 4'hf);
    expect_beat(11'd3, 32'h3f400040, 32'h1122ccdd, 4'hf);
    expect_beat(11'd3, 32'h3f400040, 32'h55667788, 4'hf);
    expect_beat(11'd3, 32'h3f400040, 32'h99aabbcc, 4'hf);
    pop_response(1'b1, 32'h0000ccdd);
    pop_response(1'b1, 32'h11220000);
    pop_response(1'b1, 32'h55667788);
    pop_response(1'b1, 32'h99aabbcc);
    while (busy) @(posedge clk);

    push_write(22'h20, 32'hdeadbeef, 4'hf);
    repeat (5) begin
      @(posedge clk);
      if (mem_write) $fatal(1, "idle timeout fired early");
    end
    expect_beat(11'd1, 32'h3f400080, 32'hdeadbeef, 4'hf);
    pop_response(1'b1, 32'hdeadbeef);
    while (busy) @(posedge clk);

    request <= {7'd2, 1'b1, 22'h7f, 32'hcafebabe, 4'hf};
    request_valid <= 1;
    do @(posedge clk); while (!request_ready);
    request_valid <= 0;
    pop_response(1'b0, 32'hcafebabe);
    if (mem_write) $fatal(1, "illegal request reached memory");
    if (busy) $fatal(1, "illegal request made ingress busy");

    push_write(22'h30, 32'h01234567, 4'hf);
    flush <= 1;
    request <= make_write(22'h31, 32'h89abcdef, 4'hf);
    request_valid <= 1;
    @(posedge clk);
    if (request_ready) $fatal(1, "flush accepted a request");
    request_valid <= 0;
    expect_beat(11'd1, 32'h3f4000c0, 32'h01234567, 4'hf);
    while (busy) @(posedge clk);
    flush <= 0;
    pop_response(1'b1, 32'h01234567);

    push_write(22'h40, 32'hdeadbeef, 4'hf);
    repeat (2) @(posedge clk);
    if (!busy) $fatal(1, "queued write did not assert busy");
    reset <= 1;
    repeat (3) @(posedge clk);
    #1;
    if (busy || response_valid || mem_write) $fatal(1, "warm reset retained pending writes");
    reset <= 0;
    push_write(22'h50, 32'h89abcdef, 4'hf);
    expect_beat(11'd1, 32'h3f400140, 32'h89abcdef, 4'hf);
    pop_response(1'b1, 32'h89abcdef);
    while (busy) @(posedge clk);

    $display("XlsFramebufferWriteIngressTb PASS");
    $finish;
  end
endmodule
