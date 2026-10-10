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
  reg [261:0] vector_slow_request = 0;
  reg vector_slow_valid = 0;
  wire vector_slow_ready;
  wire [280:0] vector_slow_response;
  wire vector_slow_response_valid;
  reg vector_slow_response_ready = 0;
  reg [108:0] vector_fast_request = 0;
  reg vector_fast_valid = 0;
  wire vector_fast_ready;
  wire [280:0] vector_fast_response;
  wire vector_fast_response_valid;
  reg vector_fast_response_ready = 0;

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

  XlsPackedBankMemoryVector vector_dut (
    .clk(clk), .reset(reset),
    .slow_request(vector_slow_request), .slow_request_valid(vector_slow_valid),
    .slow_request_ready(vector_slow_ready), .slow_response(vector_slow_response),
    .slow_response_valid(vector_slow_response_valid),
    .slow_response_ready(vector_slow_response_ready),
    .fast_request(vector_fast_request), .fast_request_valid(vector_fast_valid),
    .fast_request_ready(vector_fast_ready), .fast_response(vector_fast_response),
    .fast_response_valid(vector_fast_response_valid),
    .fast_response_ready(vector_fast_response_ready)
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

  task concurrent_write(input [11:0] address, input [31:0] data);
    begin
      slow_command = 0;
      slow_command[58] = 1;
      slow_command[47:36] = address;
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

  task concurrent_shadow_write(input [11:0] address, input [3:0] shadow,
                               input [31:0] data);
    begin
      slow_command = 0;
      slow_command[58] = 1;
      slow_command[52] = 1;
      slow_command[51:48] = shadow;
      slow_command[47:36] = address;
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

  task concurrent_read(input [11:0] address0, input [11:0] address1,
                       input second, input [63:0] expected);
    begin
      fast_command = 0;
      fast_command[48] = |address0[11:10];
      fast_command[47:36] = address0;
      fast_command[16] = |address1[11:10];
      fast_command[15:4] = address1;
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

  task vector_write(input [11:0] address, input [31:0] data);
    reg [58:0] vector_command;
    begin
      vector_command = 0;
      vector_command[58] = 1;
      vector_command[47:36] = address;
      vector_command[35:4] = data;
      vector_command[3:0] = 4'hf;
      vector_slow_request = 0;
      vector_slow_request[79:21] = vector_command;
      vector_slow_request[257] = 1;
      vector_slow_valid = 1;
      while (!vector_slow_ready) @(posedge clk);
      @(posedge clk);
      #1;
      vector_slow_valid = 0;
      while (!vector_slow_response_valid) begin @(posedge clk); #1; end
      vector_slow_response_ready = 1;
      @(posedge clk);
      #1;
      vector_slow_response_ready = 0;
    end
  endtask

  task vector_shadow_write(input [11:0] address, input [3:0] shadow,
                           input [31:0] data);
    reg [58:0] vector_command;
    begin
      vector_command = 0;
      vector_command[58] = 1;
      vector_command[52] = 1;
      vector_command[51:48] = shadow;
      vector_command[47:36] = address;
      vector_command[35:4] = data;
      vector_command[3:0] = 4'hf;
      vector_slow_request = 0;
      vector_slow_request[79:21] = vector_command;
      vector_slow_request[257] = 1;
      vector_slow_valid = 1;
      while (!vector_slow_ready) @(posedge clk);
      @(posedge clk);
      #1;
      vector_slow_valid = 0;
      while (!vector_slow_response_valid) begin @(posedge clk); #1; end
      vector_slow_response_ready = 1;
      @(posedge clk);
      #1;
      vector_slow_response_ready = 0;
    end
  endtask

  task vector_read(input [11:0] address, input [31:0] expected);
    begin
      vector_fast_request = 0;
      vector_fast_request[42:21] = {9'b0, |address[11:10], address};
      vector_fast_request[4:1] = 4'h1;
      vector_fast_request[0] = 1;
      vector_fast_valid = 1;
      while (!vector_fast_ready) @(posedge clk);
      @(posedge clk);
      #1;
      vector_fast_valid = 0;
      while (!vector_fast_response_valid) begin @(posedge clk); #1; end
      if (vector_fast_response[53:22] != expected) $fatal(1);
      vector_fast_response_ready = 1;
      @(posedge clk);
      #1;
      vector_fast_response_ready = 0;
    end
  endtask

  task vector_stream_reads;
    integer sent;
    integer received;
    integer cycles;
    reg stalled;
    reg [280:0] held;
    begin
      sent = 0;
      received = 0;
      cycles = 0;
      stalled = 0;
      while (received < 16) begin
        @(negedge clk);
        vector_fast_request = 0;
        vector_fast_request[42:21] = sent[0] ? 22'h1bc1 : 22'h1581;
        vector_fast_request[4:1] = 4'h1;
        vector_fast_request[0] = sent[0];
        vector_fast_valid = sent < 16;
        vector_fast_response_ready = cycles % 5 == 4;
        @(posedge clk);
        if (stalled && (!vector_fast_response_valid || vector_fast_response != held))
          $fatal(1, "vector response changed under backpressure");
        stalled = vector_fast_response_valid && !vector_fast_response_ready;
        held = vector_fast_response;
        if (vector_fast_valid && vector_fast_ready) sent = sent + 1;
        if (vector_fast_response_valid && vector_fast_response_ready) begin
          if (vector_fast_response[53:22] != (received[0] ? 32'hfeedface : 32'h7654cdef) ||
              vector_fast_response[0] != received[0] || vector_fast_response[4:1] != 4'h1)
            $fatal(1, "vector streaming response out of order sent=%0d received=%0d data=%h metadata=%h",
                   sent, received, vector_fast_response[53:22], vector_fast_response[20:0]);
          received = received + 1;
        end
        cycles = cycles + 1;
        if (cycles > 200) $fatal(1, "vector streaming test timed out");
      end
      @(negedge clk);
      vector_fast_valid = 0;
      vector_fast_response_ready = 0;
    end
  endtask

  reg [58:0] value;
  initial begin
    #10000;
    $fatal(1, "packed bank tests timed out");
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

    concurrent_write(12'h041, 32'h01234567);
    concurrent_write(12'h082, 32'h89abcdef);
    concurrent_write(12'h401, 32'h13579bdf);
    concurrent_write(12'h441, 32'h2468ace0);
    concurrent_write(12'h481, 32'h55aa33cc);
    concurrent_write(12'h4c1, 32'h0badc0de);
    concurrent_write(12'h501, 32'hdecafbad);
    concurrent_write(12'hbc1, 32'hfeedface);
    concurrent_shadow_write(12'h541, 4'h3, 32'hd00df00d);

    concurrent_read(12'h541, 12'h000, 0, 64'h00000000d00df00d);
    concurrent_read(12'h0c1, 12'h000, 0, 64'h00000000d00df00d);
    concurrent_read(12'h041, 12'h000, 0, 64'h0000000001234567);
    concurrent_read(12'h041, 12'h082, 1, 64'h89abcdef01234567);
    concurrent_read(12'h401, 12'h082, 1, 64'h89abcdef13579bdf);
    concurrent_read(12'h441, 12'h082, 1, 64'h89abcdef2468ace0);
    concurrent_read(12'h481, 12'h082, 1, 64'h89abcdef55aa33cc);
    concurrent_read(12'h4c1, 12'h082, 1, 64'h89abcdef0badc0de);
    concurrent_read(12'h501, 12'h082, 1, 64'h89abcdefdecafbad);
    concurrent_read(12'hbc1, 12'h082, 1, 64'h89abcdeffeedface);


    slow_response_ready = 1;
    slow_command = 0;
    slow_command[47:36] = 12'h041;
    slow_valid = 1;
    while (!slow_ready) @(posedge clk);
    @(posedge clk);
    #1;
    if (!slow_response_valid || slow_response[32:1] != 32'h01234567) $fatal(1);
    slow_command = {1'b1, 10'b0, 12'h043, 32'h76543210, 4'hf};
    #1;
    if (slow_response[32:1] != 32'h01234567) $fatal(1);
    @(posedge clk);
    #1;
    slow_valid = 0;
    if (!slow_response_valid || !slow_response[0]) $fatal(1);
    @(posedge clk);
    #1;
    slow_response_ready = 0;
    concurrent_read(12'h043, 12'h000, 0, 64'h0000000076543210);

    slow_command = {1'b1, 10'b0, 12'h403, 32'h55aa55aa, 4'hf};
    fast_command = 0;
    fast_command[47:36] = 12'h041;
    fast_command[15:4] = 12'h082;
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
    concurrent_read(12'h403, 12'h000, 0, 64'h0000000055aa55aa);

    slow_command = {1'b1, 10'b0, 12'h4c3, 32'ha55aa55a, 4'hf};
    fast_command = 0;
    fast_command[48] = 1;
    fast_command[47:36] = 12'h401;
    fast_command[15:4] = 12'h082;
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
    concurrent_read(12'h4c3, 12'h000, 0, 64'h00000000a55aa55a);

    vector_shadow_write(12'h581, 4'h4, 32'h7654cdef);
    vector_read(12'h581, 32'h7654cdef);
    vector_read(12'h101, 32'h7654cdef);
    vector_write(12'hbc1, 32'h01020304);
    value = 0;
    value[58] = 1;
    value[47:36] = 12'hbc1;
    value[35:4] = 32'hfeedface;
    value[3:0] = 4'hf;
    vector_slow_request = 0;
    vector_slow_request[79:21] = value;
    vector_slow_request[257] = 1;
    vector_fast_request = 0;
    vector_fast_request[42:21] = 22'h1bc1;
    vector_fast_request[4:1] = 4'h1;
    vector_fast_request[0] = 1;
    vector_slow_valid = 1;
    vector_fast_valid = 1;
    while (!vector_slow_ready || !vector_fast_ready) @(posedge clk);
    @(posedge clk);
    #1;
    vector_slow_valid = 0;
    vector_fast_valid = 0;
    while (!vector_slow_response_valid || !vector_fast_response_valid) begin
      @(posedge clk);
      #1;
    end
    if (vector_fast_response[53:22] != 32'hfeedface)
      $fatal(1, "colliding fast read was not serialized");
    vector_slow_response_ready = 1;
    vector_fast_response_ready = 1;
    @(posedge clk);
    #1;

    vector_stream_reads();
    vector_fast_valid = 1;
    repeat (3) @(posedge clk);
    @(negedge clk);
    vector_fast_valid = 0;
    if (!vector_dut.fast_queued_valid || !vector_dut.fast_skid_valid)
      $fatal(1, "fast requests were not queued");
    reset = 1;
    repeat (2) @(posedge clk);
    @(negedge clk);
    reset = 0;
    if (vector_fast_response_valid || vector_dut.fast_queued_valid || vector_dut.fast_skid_valid)
      $fatal(1, "reset retained vector reads");
    #1;
    vector_read(12'hbc1, 32'hfeedface);
    vector_stream_reads();

    $display("XlsPackedBankMemory passed");
    $finish;
  end
endmodule
