`timescale 1ns/1ps

module XlsAvalonArbiter2Tb;
  reg clk = 0;
  reg reset = 1;
  reg m0_read = 0;
  reg m0_write = 0;
  wire m0_waitRequestn;
  reg [10:0] m0_burstCount = 1;
  reg [31:0] m0_address = 0;
  reg [3:0] m0_byteEnable = 4'hf;
  reg [31:0] m0_writeData = 0;
  wire m0_readDataValid;
  wire [31:0] m0_readData;
  reg m1_read = 0;
  reg m1_write = 0;
  wire m1_waitRequestn;
  reg [10:0] m1_burstCount = 1;
  reg [31:0] m1_address = 0;
  reg [3:0] m1_byteEnable = 4'hf;
  reg [31:0] m1_writeData = 0;
  wire m1_readDataValid;
  wire [31:0] m1_readData;
  wire s_read;
  wire s_write;
  reg s_waitRequestn = 0;
  wire [10:0] s_burstCount;
  wire [31:0] s_address;
  wire [3:0] s_byteEnable;
  wire [31:0] s_writeData;
  reg s_readDataValid = 0;
  reg [31:0] s_readData = 0;
  integer responses = 0;

  always #5 clk = ~clk;

  XlsAvalonArbiter2 dut (
    .clk(clk), .reset(reset),
    .m0_read(m0_read), .m0_write(m0_write), .m0_waitRequestn(m0_waitRequestn),
    .m0_burstCount(m0_burstCount), .m0_address(m0_address),
    .m0_byteEnable(m0_byteEnable), .m0_writeData(m0_writeData),
    .m0_readDataValid(m0_readDataValid), .m0_readData(m0_readData),
    .m1_read(m1_read), .m1_write(m1_write), .m1_waitRequestn(m1_waitRequestn),
    .m1_burstCount(m1_burstCount), .m1_address(m1_address),
    .m1_byteEnable(m1_byteEnable), .m1_writeData(m1_writeData),
    .m1_readDataValid(m1_readDataValid), .m1_readData(m1_readData),
    .s_read(s_read), .s_write(s_write), .s_waitRequestn(s_waitRequestn),
    .s_burstCount(s_burstCount), .s_address(s_address),
    .s_byteEnable(s_byteEnable), .s_writeData(s_writeData),
    .s_readDataValid(s_readDataValid), .s_readData(s_readData)
  );

  always @(posedge clk) begin
    if (m1_readDataValid) begin
      if (m0_readDataValid || m1_readData != 32'h90000000 + responses) $fatal(1);
      responses <= responses + 1;
    end
  end

  initial begin
    repeat (3) @(posedge clk);
    reset = 0;
    m1_write = 1;
    m1_address = 32'h2000;
    #1;
    if (m1_waitRequestn || !s_write || s_address != 32'h2000) $fatal(1);
    s_waitRequestn = 1;
    #1;
    if (!m1_waitRequestn) $fatal(1);
    @(posedge clk);
    m0_write = 1;
    m0_address = 32'h1000;
    #1;
    if (!m0_waitRequestn || m1_waitRequestn || s_address != 32'h1000) $fatal(1);
    @(posedge clk);
    m0_write = 0;
    m1_address = 32'h4000;
    m1_burstCount = 3;
    @(posedge clk);
    m0_write = 1;
    #1;
    if (m0_waitRequestn || !m1_waitRequestn || s_address != 32'h4000) $fatal(1);
    @(posedge clk);
    #1;
    if (m0_waitRequestn || !m1_waitRequestn || s_address != 32'h4000) $fatal(1);
    @(posedge clk);
    m1_write = 0;
    #1;
    if (!m0_waitRequestn) $fatal(1);
    @(posedge clk);
    m0_write = 0;
    m1_read = 1;
    m1_address = 32'h3000;
    m1_burstCount = 3;
    @(posedge clk);
    m1_read = 0;
    m0_write = 1;
    #1;
    if (m0_waitRequestn || s_write) $fatal(1);
    repeat (3) begin
      @(negedge clk);
      s_readData = 32'h90000000 + responses;
      s_readDataValid = 1;
      @(posedge clk);
      @(negedge clk);
      s_readDataValid = 0;
    end
    @(posedge clk);
    #1;
    if (!m0_waitRequestn || responses != 3) $fatal(1);
    $display("XlsAvalonArbiter2Tb PASS");
    $finish;
  end
endmodule
