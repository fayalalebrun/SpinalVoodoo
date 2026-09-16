`timescale 1ns/1ps

module XlsPackedBankAddressTb;
  reg clk = 0;
  reg read1_enable = 0;
  reg write_enable = 0;
  reg [3:0] read1_address = 0;
  reg [3:0] write_address = 0;
  wire [31:0] unused0, unused1, unused2, unused3;
  XlsPackedBankMemoryArray #(.NUMWORDS(16), .ADDRESS_WIDTH(4)) shared_port (
    .clk(clk), .read0_enable(1'b0), .read0_address(4'b0),
    .read1_enable(read1_enable), .read1_address(read1_address),
    .write_enable(write_enable), .write_address(write_address),
    .write_data(32'b0), .write_mask(4'b0),
    .read0_data(unused0), .read1_data(unused1)
  );
  XlsPackedBankMemoryArray #(.NUMWORDS(16), .ADDRESS_WIDTH(4)) write_only (
    .clk(clk), .read0_enable(1'b0), .read0_address(4'b0),
    .read1_enable(1'b0), .read1_address(4'b0),
    .write_enable(write_enable), .write_address(write_address),
    .write_data(32'b0), .write_mask(4'b0),
    .read0_data(unused2), .read1_data(unused3)
  );
  initial begin
    for (integer value = 0; value < 1024; value = value + 1) begin
      {read1_enable, write_enable, read1_address, write_address} = value[9:0];
      #1;
      if (shared_port.port_a_address !==
          ((read1_enable && !write_enable) ? read1_address : write_address))
        $fatal(1, "shared port address selection mismatch %h", value);
      if ((read1_enable || write_enable) && shared_port.port_a_address !==
          (write_enable ? write_address : read1_address))
        $fatal(1, "enabled operation changed address %h", value);
      if (write_only.port_a_address !== write_address)
        $fatal(1, "write-only address depends on write enable %h", value);
    end
    $display("XlsPackedBankAddressTb PASS (1024 combinations)");
    $finish;
  end
endmodule
