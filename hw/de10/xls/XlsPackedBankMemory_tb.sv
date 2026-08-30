`timescale 1ns/1ps

module XlsPackedBankMemoryTb;
  reg clk = 0;
  reg reset = 1;
  reg [58:0] command = 0;
  reg command_valid = 0;
  wire command_ready;
   wire [64:0] response;
  wire response_valid;
  reg response_ready = 0;
  reg [58:0] slow_command = 0;
  reg slow_valid = 0;
  wire slow_ready;
  wire [64:0] slow_response;
  wire slow_response_valid;
  reg slow_response_ready = 0;
  reg [58:0] fast_command = 0;
  reg fast_valid = 0;
  wire fast_ready;
  wire [64:0] fast_response;
  wire fast_response_valid;
  reg fast_response_ready = 0;

  always #5 clk = ~clk;

  XlsPackedBankMemory dut (
    .clk(clk), .reset(reset), .command(command), .command_valid(command_valid),
    .command_ready(command_ready), .response(response), .response_valid(response_valid),
    .response_ready(response_ready)
  );

  XlsConcurrentPackedBankMemory concurrent_dut (
    .clk(clk), .reset(reset),
    .slow_command(slow_command), .slow_command_valid(slow_valid),
    .slow_command_ready(slow_ready), .slow_response(slow_response),
    .slow_response_valid(slow_response_valid), .slow_response_ready(slow_response_ready),
    .fast_command(fast_command), .fast_command_valid(fast_valid),
    .fast_command_ready(fast_ready), .fast_response(fast_response),
    .fast_response_valid(fast_response_valid), .fast_response_ready(fast_response_ready)
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

  task concurrent_write(input [10:0] address, input [31:0] data);
    begin
      slow_command = 0;
      slow_command[58] = 1;
      slow_command[46:36] = address;
      slow_command[35:4] = data;
      slow_command[3:0] = 4'hf;
      slow_valid = 1;
      while (!slow_ready) @(posedge clk);
      @(posedge clk);
      #1;
      slow_valid = 0;
      while (!slow_response_valid) begin @(posedge clk); #1; end
      slow_response_ready = 1;
      @(posedge clk);
      #1;
      slow_response_ready = 0;
    end
  endtask

  task concurrent_read(input [10:0] address0, input [10:0] address1,
                       input second, input [63:0] expected);
    begin
      fast_command = 0;
      fast_command[46:36] = address0;
      fast_command[14:4] = address1;
      fast_command[0] = second;
      fast_valid = 1;
      while (!fast_ready) @(posedge clk);
      @(posedge clk);
      #1;
      fast_valid = 0;
      while (!fast_response_valid) begin @(posedge clk); #1; end
      if (!fast_response[0] || fast_response[64:1] != expected)
        $fatal(1, "fast read expected=%h actual=%h", expected, fast_response[64:1]);
      fast_response_ready = 1;
      @(posedge clk);
      #1;
      fast_response_ready = 0;
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

    response_ready = 1;
    value = 0;
    value[58] = 1;
    value[57:36] = (5 << 6) | 7;
    value[35:4] = 32'h11223344;
    value[3:0] = 4'hf;
    command = value;
    command_valid = 1;
    while (!command_ready) @(posedge clk);
    @(posedge clk);
    #1;
    if (!response_valid || response[32:1] != 32'h11223344 || !command_ready) $fatal(1);

    value[35:4] = 32'haabbccdd;
    value[3:0] = 4'h5;
    command = value;
    @(posedge clk);
    #1;
    if (!response_valid || response[32:1] != 32'h11bb33dd) $fatal(1);
    command_valid = 0;
    @(posedge clk);
    #1;
    response_ready = 0;

    value = 0;
    value[57:36] = (5 << 6) | 7;
    transact(value, 32'h11bb33dd);

    concurrent_write(11'h041, 32'h01234567);
    concurrent_write(11'h082, 32'h89abcdef);
    concurrent_write(11'h401, 32'h13579bdf);
    concurrent_write(11'h441, 32'h2468ace0);
    concurrent_write(11'h481, 32'h55aa33cc);
    concurrent_write(11'h4c1, 32'h0badc0de);
    concurrent_write(11'h501, 32'hdecafbad);

    concurrent_read(11'h041, 11'h000, 0, 64'h0000000001234567);
    concurrent_read(11'h041, 11'h082, 1, 64'h89abcdef01234567);
    concurrent_read(11'h401, 11'h082, 1, 64'h89abcdef13579bdf);
    concurrent_read(11'h441, 11'h082, 1, 64'h89abcdef2468ace0);
    concurrent_read(11'h481, 11'h082, 1, 64'h89abcdef55aa33cc);
    concurrent_read(11'h4c1, 11'h082, 1, 64'h89abcdef0badc0de);
    concurrent_read(11'h501, 11'h082, 1, 64'h89abcdefdecafbad);


    slow_response_ready = 1;
    slow_command = 0;
    slow_command[46:36] = 11'h041;
    slow_valid = 1;
    while (!slow_ready) @(posedge clk);
    @(posedge clk);
    #1;
    if (!slow_response_valid || slow_response[32:1] != 32'h01234567) $fatal(1);
    slow_command = {1'b1, 11'b0, 11'h043, 32'h76543210, 4'hf};
    #1;
    if (slow_response[32:1] != 32'h01234567) $fatal(1);
    @(posedge clk);
    #1;
    slow_valid = 0;
    if (!slow_response_valid || !slow_response[0]) $fatal(1);
    @(posedge clk);
    #1;
    slow_response_ready = 0;
    concurrent_read(11'h043, 11'h000, 0, 64'h0000000076543210);

    slow_command = {1'b1, 11'b0, 11'h403, 32'h55aa55aa, 4'hf};
    fast_command = 0;
    fast_command[46:36] = 11'h041;
    fast_command[14:4] = 11'h082;
    fast_command[0] = 1;
    slow_valid = 1;
    fast_valid = 1;
    while (!slow_ready || !fast_ready) @(posedge clk);
    @(posedge clk);
    #1;
    slow_valid = 0;
    fast_valid = 0;
    while (!slow_response_valid || !fast_response_valid) begin @(posedge clk); #1; end
    if (fast_response[64:1] != 64'h89abcdef01234567) $fatal(1);
    slow_response_ready = 1;
    fast_response_ready = 1;
    @(posedge clk);
    #1;
    slow_response_ready = 0;
    fast_response_ready = 0;
    concurrent_read(11'h403, 11'h000, 0, 64'h0000000055aa55aa);

    slow_command = {1'b1, 11'b0, 11'h0c3, 32'ha55aa55a, 4'hf};
    fast_command = 0;
    fast_command[46:36] = 11'h401;
    fast_command[14:4] = 11'h082;
    fast_command[0] = 1;
    slow_valid = 1;
    fast_valid = 1;
    while (!slow_ready || !fast_ready) @(posedge clk);
    @(posedge clk);
    #1;
    slow_valid = 0;
    fast_valid = 0;
    while (!slow_response_valid || !fast_response_valid) begin @(posedge clk); #1; end
    if (fast_response[64:1] != 64'h89abcdef13579bdf) $fatal(1);
    slow_response_ready = 1;
    fast_response_ready = 1;
    @(posedge clk);
    #1;
    slow_response_ready = 0;
    fast_response_ready = 0;
    concurrent_read(11'h0c3, 11'h000, 0, 64'h00000000a55aa55a);

    $display("XlsPackedBankMemory passed");
    $finish;
  end
endmodule
