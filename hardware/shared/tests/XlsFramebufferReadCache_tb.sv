`timescale 1ns/1ps

module XlsFramebufferReadCacheTb;
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
  integer i;

  always #5 clk = ~clk;

  XlsFramebufferReadCache dut (
    .clk(clk), .reset(reset), .request(request), .request_valid(request_valid),
    .request_ready(request_ready), .response(response), .response_valid(response_valid),
    .response_ready(response_ready), .mem_read(mem_read), .mem_write(mem_write),
    .mem_waitRequestn(mem_waitRequestn), .mem_burstCount(mem_burstCount),
    .mem_address(mem_address), .mem_byteEnable(mem_byteEnable),
    .mem_writeData(mem_writeData), .mem_readDataValid(mem_readDataValid),
    .mem_readData(mem_readData), .invalidate(invalidate),
    .write_pending(write_pending), .miss_waiting(miss_waiting),
    .fill_active(fill_active), .fill_line_tag(fill_line_tag),
    .update_valid(update_valid), .update_address(update_address), .update_data(update_data),
    .update_mask(update_mask)
  );

  function automatic [65:0] make_request(input [21:0] address, input [6:0] count, input write);
    make_request = {count, write, address, 32'b0, 4'hf};
  endfunction

  task automatic push_request(input [21:0] address, input [6:0] count, input write);
    begin
      request <= make_request(address, count, write);
      request_valid <= 1;
      do @(posedge clk); while (!request_ready);
      request_valid <= 0;
    end
  endtask

  task automatic accept_memory_request(input [21:0] expected_address);
    begin
      while (!mem_read) @(posedge clk);
      if (mem_write || mem_burstCount !== 11'd64 ||
          mem_address !== 32'h00000000 + (expected_address << 2) ||
          mem_byteEnable !== 4'hf) $fatal(1, "bad memory request");
      mem_waitRequestn <= 1;
      @(posedge clk);
      mem_waitRequestn <= 0;
    end
  endtask

  task automatic send_fill(input [31:0] base, input success);
    begin
      for (i = 0; i < 64; i = i + 1) begin
        mem_readData <= base + i;
        mem_readDataValid <= 1;
        @(posedge clk);
      end
      mem_readDataValid <= 0;
      if (!success) invalidate <= 1;
      @(posedge clk);
      invalidate <= 0;
    end
  endtask

  task automatic pop_response(input success, input [31:0] data);
    begin
      response_ready <= 1;
      while (!response_valid) @(posedge clk);
      if (response[0] !== success || response[32:1] !== data) $fatal(1, "bad response");
      @(posedge clk);
      response_ready <= 0;
    end
  endtask

  task automatic pulse_update(input [21:0] address, input [31:0] data, input [3:0] mask);
    begin
      update_address <= address;
      update_data <= data;
      update_mask <= mask;
      update_valid <= 1;
      @(posedge clk);
      update_valid <= 0;
    end
  endtask

  initial begin
    repeat (3) @(posedge clk);
    reset <= 0;

    push_request(22'h45, 7'd1, 1'b0);
    accept_memory_request(22'h40);
    send_fill(32'h10000000, 1'b1);
    pop_response(1'b1, 32'h10000005);

    push_request(22'h46, 7'd1, 1'b0);
    if (mem_read) $fatal(1, "unexpected hit miss");
    pop_response(1'b1, 32'h10000006);

    pulse_update(22'h46, 32'haabbccdd, 4'b0101);
    push_request(22'h46, 7'd1, 1'b0);
    pop_response(1'b1, 32'h10bb00dd);

    push_request(22'h47, 7'd1, 1'b0);
    while (!response_valid) @(posedge clk);
    repeat (3) begin
      @(posedge clk);
      if (!response_valid || response[32:1] !== 32'h10000007) $fatal(1, "response not held");
    end
    pop_response(1'b1, 32'h10000007);

    push_request(22'h48, 7'd2, 1'b0);
    if (mem_read) $fatal(1, "invalid request reached memory");
    pop_response(1'b0, 32'b0);

    invalidate <= 1;
    @(posedge clk);
    invalidate <= 0;
    request <= make_request(22'h46, 7'd1, 1'b0);
    request_valid <= 1;
    write_pending <= 1;
    repeat (3) begin
      @(posedge clk);
      if (request_ready || !miss_waiting || mem_read)
        $fatal(1, "miss passed pending write");
    end
    write_pending <= 0;
    do @(posedge clk); while (!request_ready);
    request_valid <= 0;
    accept_memory_request(22'h40);
    pulse_update(22'h46, 32'hdeadbeef, 4'hf);
    send_fill(32'h20000000, 1'b1);
    pop_response(1'b1, 32'h20000006);

    push_request(22'h46, 7'd1, 1'b0);
    accept_memory_request(22'h40);
    send_fill(32'h30000000, 1'b0);
    pop_response(1'b1, 32'h30000006);

    push_request(22'h46, 7'd1, 1'b0);
    accept_memory_request(22'h40);
    send_fill(32'h40000000, 1'b1);
    pop_response(1'b1, 32'h40000006);

    $display("XlsFramebufferReadCacheTb PASS");
    $finish;
  end
endmodule
