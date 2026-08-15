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
  function automatic [64:0] smul65b_48b_x_18b (input reg [47:0] lhs, input reg [17:0] rhs);
    reg signed [47:0] signed_lhs;
    reg signed [17:0] signed_rhs;
    reg signed [64:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul65b_48b_x_18b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  // lint_off SIGNED_TYPE
  function automatic [64:0] shra_65b_by_7b (input reg [64:0] to_shift, input reg [6:0] shift_amount);
    reg signed [64:0] signed_result;
    begin
      signed_result = $signed(to_shift) >>> shift_amount;
      shra_65b_by_7b = $unsigned(signed_result);
    end
  endfunction
  // lint_on SIGNED_TYPE
  wire [17:0] concat_163;
  wire [64:0] product_s_shift_bits;
  wire [6:0] add_168;
  wire [64:0] product_t_shift_bits;
  wire [64:0] shra_171;
  wire [64:0] shra_172;
  wire ne_173;
  wire [64:0] shra_176;
  wire [64:0] shra_179;
  wire [17:0] shifted;
  wire [17:0] shifted__1;
  wire [17:0] add_187;
  wire [17:0] add_189;
  wire [17:0] tex_s;
  wire [17:0] tex_t;
  assign concat_163 = {1'h0, interpolated};
  assign product_s_shift_bits = smul65b_48b_x_18b(s, concat_163);
  assign add_168 = msb + 7'h7f;
  assign product_t_shift_bits = smul65b_48b_x_18b(t, concat_163);
  assign shra_171 = shra_65b_by_7b(product_s_shift_bits, add_168);
  assign shra_172 = shra_65b_by_7b(product_t_shift_bits, add_168);
  assign ne_173 = msb != 7'h00;
  assign shra_176 = shra_65b_by_7b(product_s_shift_bits, msb);
  assign shra_179 = shra_65b_by_7b(product_t_shift_bits, msb);
  assign shifted = shra_176[17:0];
  assign shifted__1 = shra_179[17:0];
  assign add_187 = shifted + {17'h0_0000, ne_173 & shra_171[0]};
  assign add_189 = shifted__1 + {17'h0_0000, ne_173 & shra_172[0]};
  assign tex_s = perspective ? add_187 : s[31:14];
  assign tex_t = perspective ? add_189 : t[31:14];
  assign out = {tex_s, tex_t};
endmodule
