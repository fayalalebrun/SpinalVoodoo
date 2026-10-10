module XlsTextureTagMemoryTb;
  reg clk = 0;
  always #5 clk = ~clk;
  reg reset = 1;
  reg [24:0] read_request = 0;
  reg read_valid = 0;
  wire read_ready;
  wire [55:0] response;
  wire response_valid;
  reg response_ready = 0;
  reg [26:0] write_request = 0;
  reg write_valid = 0;
  wire write_ready;
  reg [13:0] primary [0:63];
  reg [13:0] shadow [0:15];
  reg [55:0] expected [0:1];
  integer expected_count = 0;
  integer consecutive = 0;
  integer max_consecutive = 0;
  integer checks = 0;
  integer pslot;
  integer sslot;
  integer slot;
  integer i;
  integer cycle;
  reg [31:0] random_state = 32'h413d28b7;
  XlsTextureTagMemory dut (.*);
  always @(posedge clk) begin
    if (reset) begin
      expected_count = 0;
      consecutive = 0;
    end else begin
      if (response_valid !== (expected_count != 0)) $fatal(1, "response valid mismatch");
      if (expected_count != 0 && response !== expected[0]) $fatal(1, "tag data mismatch");
      if (response_valid && response_ready) begin
        checks = checks + 1;
        consecutive = consecutive + 1;
        if (consecutive > max_consecutive) max_consecutive = consecutive;
        expected[0] = expected[1];
        expected_count = expected_count - 1;
      end else consecutive = 0;
      if (write_valid && write_ready) begin
        pslot = ((write_request >> 9) ^ (write_request >> 15) ^ (write_request >> 21)) & 63;
        sslot = ((write_request >> 9) ^ (write_request >> 13) ^ (write_request >> 17) ^
                 (write_request >> 21) ^ (write_request >> 25)) & 15;
        if (write_request[0]) primary[pslot] = {2'b0, write_request[26:15]};
        shadow[sslot] = write_request[26:13];
      end
      if (read_valid && read_ready) begin
        if (expected_count >= 2) $fatal(1, "tag response overflow");
        for (i = 0; i < 4; i = i + 1) begin
          slot = (read_request >> (1 + 6*i)) & 63;
          expected[expected_count][14*i +: 14] = read_request[0] ? primary[slot] : shadow[slot & 15];
        end
        expected_count = expected_count + 1;
      end
    end
  end
  initial begin
    repeat (3) @(negedge clk);
    reset = 0;
    for (cycle = 0; cycle < 64; cycle = cycle + 1) begin
      write_valid = 1;
      write_request = ((26'h1230000 | (cycle << 8)) << 1) | 1;
      @(negedge clk);
    end
    write_valid = 0;
    response_ready = 1;
    for (cycle = 0; cycle < 128; cycle = cycle + 1) begin
      while (!read_ready) @(negedge clk);
      read_valid = 1;
      read_request = {cycle[5:0], (cycle[5:0] ^ 6'd1),
          (cycle[5:0] ^ 6'd2), (cycle[5:0] ^ 6'd3), (cycle < 64)};
      @(negedge clk);
      read_valid = 0;
    end
    for (cycle = 0; cycle < 2048; cycle = cycle + 1) begin
      random_state = {random_state[30:0], random_state[31] ^ random_state[21] ^ random_state[1] ^ random_state[0]};
      read_valid = cycle < 128 || random_state[0];
      response_ready = cycle < 128 || random_state[1];
      write_valid = cycle >= 128 && random_state[2];
      write_request = random_state[26:0];
      read_request = cycle < 128 ? {4{cycle[5:0]}} << 1 | (cycle < 64) : random_state[24:0];
      if (cycle >= 128 && random_state[3]) begin
        slot = write_request[0] ?
            ((write_request >> 9) ^ (write_request >> 15) ^ (write_request >> 21)) & 63 :
            ((write_request >> 9) ^ (write_request >> 13) ^ (write_request >> 17) ^
             (write_request >> 21) ^ (write_request >> 25)) & 15;
        read_request = {4{slot[5:0]}} << 1 | write_request[0];
      end
      if (cycle == 1000) reset = 1;
      if (cycle == 1002) reset = 0;
      @(negedge clk);
    end
    read_valid = 0;
    write_valid = 0;
    response_ready = 1;
    repeat (3) @(negedge clk);
    if (checks < 400) $fatal(1, "insufficient read coverage");
    if (max_consecutive < 128) $fatal(1, "tag RAM did not sustain one lookup per cycle");
    $display("XlsTextureTagMemory PASS (%0d responses)", checks);
    $finish;
  end
  initial begin
    #30000;
    $fatal(1, "timeout");
  end
endmodule
