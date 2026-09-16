`timescale 1ns/1ps

module XlsH2fCommandFifoTb #(
  parameter integer FIFO_ADDRESS_WIDTH = 6
);
  localparam integer INITIAL_WRITES = (1 << FIFO_ADDRESS_WIDTH) + 144;
  reg h2f_clk = 1'b0;
  reg core_clk = 1'b0;
  reg h2f_reset = 1'b1;
  reg core_reset = 1'b1;
  reg [21:0] h2f_address = 0;
  reg h2f_read = 1'b0;
  reg h2f_write = 1'b0;
  reg [3:0] h2f_byteenable = 0;
  reg [31:0] h2f_writedata = 0;
  wire h2f_waitrequest;
  wire [31:0] h2f_readdata;
  wire h2f_readdatavalid;
  wire [21:0] core_address;
  wire core_read;
  wire core_write;
  wire [3:0] core_byteenable;
  wire [31:0] core_writedata;
  wire core_waitrequest;
  reg [31:0] core_readdata = 0;
  reg core_readdatavalid = 1'b0;
  reg core_stall = 1'b1;
  reg [2:0] read_delay = 0;
  integer accepted = 0;
  integer responses = 0;
  integer errors = 0;
  reg saw_full = 1'b0;
  reg stalled = 1'b0;
  reg [59:0] stalled_request;
  wire [59:0] core_request = {core_address, core_read, core_write, core_byteenable, core_writedata};

  always #5 h2f_clk = ~h2f_clk;
  always #7 core_clk = ~core_clk;

  assign core_waitrequest = core_stall || read_delay != 0;

  XlsH2fCommandFifo #(
    .FIFO_ADDRESS_WIDTH(FIFO_ADDRESS_WIDTH)
  ) dut (
    .h2f_clk(h2f_clk),
    .h2f_reset(h2f_reset),
    .h2f_address(h2f_address),
    .h2f_read(h2f_read),
    .h2f_write(h2f_write),
    .h2f_byteenable(h2f_byteenable),
    .h2f_writedata(h2f_writedata),
    .h2f_waitrequest(h2f_waitrequest),
    .h2f_readdata(h2f_readdata),
    .h2f_readdatavalid(h2f_readdatavalid),
    .core_clk(core_clk),
    .core_reset(core_reset),
    .core_address(core_address),
    .core_read(core_read),
    .core_write(core_write),
    .core_byteenable(core_byteenable),
    .core_writedata(core_writedata),
    .core_waitrequest(core_waitrequest),
    .core_readdata(core_readdata),
    .core_readdatavalid(core_readdatavalid)
  );

  task send_write(input [21:0] address, input [31:0] data);
    begin
      @(negedge h2f_clk);
      h2f_address = address;
      h2f_writedata = data;
      h2f_byteenable = 4'hf;
      h2f_write = 1'b1;
      #1;
      while (h2f_waitrequest) @(negedge h2f_clk);
      @(negedge h2f_clk);
      h2f_write = 1'b0;
    end
  endtask

  task send_read(input [21:0] address);
    begin
      @(negedge h2f_clk);
      h2f_address = address;
      h2f_byteenable = 4'hf;
      h2f_read = 1'b1;
      #1;
      while (h2f_waitrequest) @(negedge h2f_clk);
      @(negedge h2f_clk);
      h2f_read = 1'b0;
    end
  endtask

  always @(posedge h2f_clk) begin
    if (h2f_waitrequest) saw_full <= 1'b1;
    if (h2f_readdatavalid) begin
      if (h2f_readdata !== 32'hc0000222) errors <= errors + 1;
      responses <= responses + 1;
    end
  end

  always @(posedge core_clk) begin
    if (!core_reset && stalled && core_request !== stalled_request)
      $fatal(1, "core request changed under backpressure");
    stalled <= !core_reset && (core_read || core_write) && core_waitrequest;
    stalled_request <= core_request;
    core_readdatavalid <= 1'b0;
    if (read_delay != 0) begin
      read_delay <= read_delay - 1'b1;
      if (read_delay == 1) begin
        core_readdata <= 32'hc0000222;
        core_readdatavalid <= 1'b1;
      end
    end
    if ((core_read || core_write) && !core_waitrequest) begin
      if (accepted < INITIAL_WRITES) begin
        if (!core_write || core_read || core_address !== 22'h100 + accepted ||
            core_writedata !== 32'h10000000 + accepted || core_byteenable !== 4'hf)
          errors <= errors + 1;
      end else if (accepted < INITIAL_WRITES + 3) begin
        if (!core_write || core_address !== 22'h200 + accepted - INITIAL_WRITES)
          errors <= errors + 1;
      end else if (accepted == INITIAL_WRITES + 3) begin
        if (!core_read || core_write || core_address !== 22'h222)
          errors <= errors + 1;
        read_delay <= 3;
      end else begin
        if (!core_write || core_address !== 22'h203 + accepted - INITIAL_WRITES - 4)
          errors <= errors + 1;
      end
      accepted <= accepted + 1;
    end
  end

  initial begin
    repeat (4) @(posedge h2f_clk);
    h2f_reset = 1'b0;
    core_reset = 1'b0;

    fork
      begin
        integer i;
        for (i = 0; i < INITIAL_WRITES; i = i + 1)
          send_write(22'h100 + i, 32'h10000000 + i);
      end
      begin
        wait (h2f_waitrequest);
        repeat (8) @(posedge core_clk);
        core_stall = 1'b0;
      end
    join

    send_write(22'h200, 32'h20000000);
    send_write(22'h201, 32'h20000001);
    send_write(22'h202, 32'h20000002);
    send_read(22'h222);
    send_write(22'h203, 32'h20000003);
    send_write(22'h204, 32'h20000004);

    wait (accepted == INITIAL_WRITES + 6 && responses == 1);
    repeat (8) @(posedge core_clk);
    if (!saw_full || errors != 0) begin
      $display("FAIL accepted=%0d responses=%0d saw_full=%0d errors=%0d",
               accepted, responses, saw_full, errors);
      $fatal(1);
    end
    @(negedge core_clk);
    core_stall = 1'b1;
    send_write(22'h3ff, 32'hdeadbeef);
    wait (core_write);
    repeat (4) @(negedge core_clk);
    h2f_reset = 1'b1;
    core_reset = 1'b1;
    repeat (4) @(negedge core_clk);
    if (core_read || core_write) $fatal(1, "queued command survived reset");
    h2f_reset = 1'b0;
    core_reset = 1'b0;
    core_stall = 1'b0;
    send_write(22'h205, 32'h20000005);
    wait (accepted == INITIAL_WRITES + 7);
    repeat (8) @(negedge core_clk);
    if (errors != 0 || core_read || core_write) $fatal(1, "warm reset ordering");
    $display("PASS");
    $finish;
  end

  initial begin
    #5000000;
    $display("FAIL timeout accepted=%0d responses=%0d errors=%0d",
             accepted, responses, errors);
    $fatal(1);
  end
endmodule
