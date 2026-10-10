`timescale 1ns/1ps

module XlsAvalonTransferMasterTb;
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
  integer i;
  reg [32:0] held_response;

  always #5 clk = ~clk;

  XlsAvalonTransferMaster #(
    .BASE_ADDRESS(32'h1000), .SPAN_WORDS(128), .WRAP_BURST64(1)
  ) dut (
    .clk(clk), .reset(reset), .request(request), .request_valid(request_valid),
    .request_ready(request_ready), .response(response), .response_valid(response_valid),
    .response_ready(response_ready), .mem_read(mem_read), .mem_write(mem_write),
    .mem_waitRequestn(mem_waitRequestn), .mem_burstCount(mem_burstCount),
    .mem_address(mem_address), .mem_byteEnable(mem_byteEnable), .mem_writeData(mem_writeData),
    .mem_readDataValid(mem_readDataValid), .mem_readData(mem_readData)
  );

  function automatic [65:0] make_request(
    input [6:0] count,
    input write,
    input [21:0] address,
    input [31:0] data,
    input [3:0] mask
  );
    make_request = {count, write, address, data, mask};
  endfunction

  task automatic queue_request(input [65:0] value);
    begin
      @(negedge clk);
      request = value;
      request_valid = 1;
      while (!request_ready) @(negedge clk);
      @(posedge clk);
      #1;
      request_valid = 0;
    end
  endtask

  task automatic expect_response(
    input expected_success,
    input [31:0] expected_data
  );
    begin
      while (!response_valid) @(negedge clk);
      if (response[0] !== expected_success || response[32:1] !== expected_data) $fatal(1);
      response_ready = 1;
      @(posedge clk);
      #1;
      response_ready = 0;
    end
  endtask

  initial begin
    #20000;
    $fatal(1);
  end

  initial begin
    repeat (2) @(posedge clk);
    @(negedge clk);
    reset = 0;

    response_ready = 0;
    fork
      begin : write_driver
        integer wi;
        request_valid = 1;
        for (wi = 0; wi < 8; wi = wi + 1) begin
          request = make_request(1, 1, wi[21:0], 32'h10000000 + wi, 4'hf);
          @(posedge clk);
          if (!request_ready) $fatal(1);
          @(negedge clk);
        end
        request_valid = 0;
      end
      begin : write_monitor
        integer wm;
        while (!mem_write) @(negedge clk);
        for (wm = 0; wm < 8; wm = wm + 1) begin
          if (!mem_write || mem_read || mem_address !== 32'h1000 + wm * 4 ||
              mem_burstCount !== 1 || mem_writeData !== 32'h10000000 + wm) $fatal(1);
          if (wm != 7) @(negedge clk);
        end
      end
    join
    @(posedge clk);
    #1;
    if (!response_valid) $fatal(1);
    held_response = response;
    repeat (4) begin
      @(posedge clk);
      #1;
      if (!response_valid || response !== held_response) $fatal(1);
    end
    for (i = 0; i < 8; i = i + 1) begin
      expect_response(1, 32'h10000000 + i);
    end

    @(negedge clk);
    fork
      begin : read_driver
        integer ri;
        request_valid = 1;
        for (ri = 0; ri < 8; ri = ri + 1) begin
          request = make_request(1, 0, (16 + ri), 0, 0);
          @(posedge clk);
          if (!request_ready) $fatal(1);
          @(negedge clk);
        end
        request_valid = 0;
      end
      begin : read_monitor
        integer rm;
        while (!mem_read) @(negedge clk);
        for (rm = 0; rm < 8; rm = rm + 1) begin
          if (!mem_read || mem_write || mem_address !== 32'h1040 + rm * 4 ||
              mem_burstCount !== 1 || mem_byteEnable !== 4'hf) $fatal(1);
          if (rm != 7) @(negedge clk);
        end
      end
    join
    @(posedge clk);
    @(negedge clk);
    mem_readDataValid = 1;
    for (i = 0; i < 8; i = i + 1) begin
      mem_readData = 32'h20000000 + i;
      @(posedge clk);
      @(negedge clk);
    end
    mem_readDataValid = 0;
    for (i = 0; i < 8; i = i + 1) begin
      expect_response(1, 32'h20000000 + i);
    end

    response_ready = 0;
    queue_request(make_request(3, 0, 40, 0, 0));
    while (!mem_read) @(negedge clk);
    if (mem_burstCount !== 3 || mem_address !== 32'h10a0) $fatal(1);
    queue_request(make_request(1, 0, 48, 0, 0));
    while (!mem_read || mem_burstCount !== 1) @(negedge clk);
    if (mem_address !== 32'h10c0) $fatal(1);
    @(posedge clk);
    @(negedge clk);
    mem_readDataValid = 1;
    mem_readData = 32'h30000000;
    @(posedge clk);
    @(negedge clk);
    mem_readData = 32'h30000001;
    @(posedge clk);
    @(negedge clk);
    mem_readData = 32'h30000002;
    @(posedge clk);
    @(negedge clk);
    mem_readData = 32'h40000000;
    @(posedge clk);
    @(negedge clk);
    mem_readDataValid = 0;
    expect_response(1, 32'h30000000);
    expect_response(1, 32'h30000001);
    expect_response(1, 32'h30000002);
    expect_response(1, 32'h40000000);

    mem_waitRequestn = 0;
    queue_request(make_request(1, 1, 60, 32'h5555aaaa, 4'h5));
    while (!mem_write) @(negedge clk);
    repeat (3) begin
      if (!mem_write || mem_address !== 32'h10f0 || mem_writeData !== 32'h5555aaaa ||
          mem_byteEnable !== 4'h5) $fatal(1);
      @(posedge clk);
    end
    @(negedge clk);
    mem_waitRequestn = 1;
    @(posedge clk);
    #1;
    if (mem_write) $fatal(1);
    expect_response(1, 32'h5555aaaa);

    mem_waitRequestn = 0;
    queue_request(make_request(2, 0, 127, 0, 0));
    repeat (2) begin
      @(posedge clk);
      #1;
      if (mem_read || mem_write) $fatal(1);
    end
    expect_response(0, 0);
    expect_response(0, 0);
    mem_waitRequestn = 1;

    queue_request(make_request(64, 0, 0, 0, 0));
    while (!mem_read) @(negedge clk);
    if (mem_burstCount !== 64 || mem_address !== 32'h1000) $fatal(1);
    @(posedge clk);
    @(negedge clk);
    mem_readDataValid = 1;
    for (i = 0; i < 64; i = i + 1) begin
      mem_readData = 32'h60000000 + i;
      @(posedge clk);
      @(negedge clk);
    end
    mem_readDataValid = 0;
    for (i = 0; i < 64; i = i + 1) begin
      expect_response(1, 32'h60000000 + i);
    end

    mem_waitRequestn = 0;
    queue_request(make_request(64, 0, 65, 0, 0));
    mem_waitRequestn = 1;
    while (!mem_read) @(negedge clk);
    if (mem_burstCount !== 63 || mem_address !== 32'h1104) $fatal(1);
    @(posedge clk);
    #1;
    if (!mem_read || mem_burstCount !== 1 || mem_address !== 32'h1100) $fatal(1);
    @(posedge clk);
    #1;
    if (mem_read) $fatal(1);
    @(negedge clk);
    mem_readDataValid = 1;
    for (i = 0; i < 64; i = i + 1) begin
      mem_readData = 32'h70000000 + i;
      @(posedge clk);
      @(negedge clk);
    end
    mem_readDataValid = 0;
    for (i = 0; i < 64; i = i + 1) begin
      expect_response(1, 32'h70000000 + i);
    end

    $display("XlsAvalonTransferMaster passed");
    $finish;
  end
endmodule
