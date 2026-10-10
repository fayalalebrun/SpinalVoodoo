module VoodooXlsSignedMagnitude16(
  input wire [15:0] value,
  output wire [15:0] out
);
  assign out = value[15] ? -value : value;
endmodule
