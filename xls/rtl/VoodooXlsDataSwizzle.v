module VoodooXlsDataSwizzle(
  input wire [31:0] data,
  input wire word_swap,
  input wire byte_swizzle,
  output wire [31:0] out
);
  wire [31:0] words;
  assign words = word_swap ? {data[15:0], data[31:16]} : data;
  assign out = byte_swizzle ? {words[7:0], words[15:8], words[23:16], words[31:24]} : words;
endmodule
