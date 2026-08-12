module VoodooXlsFogTableFactor(
  input wire [15:0] w_depth,
  input wire [15:0] table_entry,
  output wire [7:0] out
);
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [16:0] smul17b_8b_x_9b (input reg [7:0] lhs, input reg [8:0] rhs);
    reg signed [7:0] signed_lhs;
    reg signed [8:0] signed_rhs;
    reg signed [16:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul17b_8b_x_9b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  wire [7:0] blend;
  wire [7:0] delta;
  wire [16:0] smul_104;
  wire [7:0] base;
  wire [8:0] bit_slice_107;
  wire [9:0] add_110;
  assign blend = w_depth[9:2];
  assign delta = table_entry[15:8];
  assign smul_104 = smul17b_8b_x_9b(delta, {1'h0, blend});
  assign base = table_entry[7:0];
  assign bit_slice_107 = smul_104[16:8];
  assign add_110 = {2'h0, base} + {{1{bit_slice_107[8]}}, bit_slice_107};
  assign out = ($signed(add_110) > $signed(10'h0ff) ? 8'hff : add_110[7:0]) & {8{~add_110[9]}};
endmodule
