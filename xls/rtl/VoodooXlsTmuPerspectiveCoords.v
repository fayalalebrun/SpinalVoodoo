module VoodooXlsTmuPerspectiveCoords(
  input wire [47:0] s,
  input wire [47:0] t,
  input wire [16:0] interpolated,
  input wire [6:0] msb,
  input wire perspective,
  output wire [35:0] out
);
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [65:0] smul66b_48b_x_18b (input reg [47:0] lhs, input reg [17:0] rhs);
    reg signed [47:0] signed_lhs;
    reg signed [17:0] signed_rhs;
    reg signed [65:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul66b_48b_x_18b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  // lint_off SIGNED_TYPE
  function automatic [65:0] shra_66b_by_7b (input reg [65:0] to_shift, input reg [6:0] shift_amount);
    reg signed [65:0] signed_result;
    begin
      signed_result = $signed(to_shift) >>> shift_amount;
      shra_66b_by_7b = $unsigned(signed_result);
    end
  endfunction
  // lint_on SIGNED_TYPE
  wire [17:0] concat_98;
  wire [6:0] bias_shift;
  wire [65:0] product_s;
  wire [65:0] bias;
  wire [65:0] product_t;
  wire [65:0] add_104;
  wire [65:0] add_105;
  wire [65:0] shra_106;
  wire [65:0] shra_107;
  wire [17:0] tex_s;
  wire [17:0] tex_t;
  assign concat_98 = {1'h0, interpolated};
  assign bias_shift = msb + 7'h7f;
  assign product_s = smul66b_48b_x_18b(s, concat_98);
  assign bias = bias_shift >= 7'h42 ? 66'h0_0000_0000_0000_0000 : 66'h0_0000_0000_0000_0001 << bias_shift;
  assign product_t = smul66b_48b_x_18b(t, concat_98);
  assign add_104 = product_s + bias;
  assign add_105 = product_t + bias;
  assign shra_106 = shra_66b_by_7b(add_104, msb);
  assign shra_107 = shra_66b_by_7b(add_105, msb);
  assign tex_s = perspective ? shra_106[17:0] : s[31:14];
  assign tex_t = perspective ? shra_107[17:0] : t[31:14];
  assign out = {tex_s, tex_t};
endmodule
