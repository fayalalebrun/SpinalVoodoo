module VoodooXlsTmuCoordinates(
  input wire [17:0] tex_s,
  input wire [17:0] tex_t,
  input wire [3:0] level,
  input wire clamp_zero,
  output wire [63:0] out
);
  // lint_off SIGNED_TYPE
  function automatic [17:0] shra_18b_by_5b (input reg [17:0] to_shift, input reg [4:0] shift_amount);
    reg signed [17:0] signed_result;
    begin
      signed_result = $signed(to_shift) >>> shift_amount;
      shra_18b_by_5b = $unsigned(signed_result);
    end
  endfunction
  // lint_on SIGNED_TYPE
  // lint_off SIGNED_TYPE
  function automatic [17:0] shra_18b_by_4b (input reg [17:0] to_shift, input reg [3:0] shift_amount);
    reg signed [17:0] signed_result;
    begin
      signed_result = $signed(to_shift) >>> shift_amount;
      shra_18b_by_4b = $unsigned(signed_result);
    end
  endfunction
  // lint_on SIGNED_TYPE
  wire [4:0] half_shift__1;
  wire [17:0] decode_176;
  wire [2:0] add_183;
  wire [14:0] sub_185;
  wire [14:0] sub_187;
  wire [4:0] point_shift__1;
  wire [17:0] adjusted_s;
  wire [17:0] adjusted_t;
  wire [17:0] shra_193;
  wire [17:0] shra_195;
  wire [17:0] scaled_s;
  wire [17:0] scaled_t;
  wire [13:0] point_s;
  wire [13:0] sign_ext_199;
  wire [13:0] point_t;
  wire [13:0] integer_s;
  wire [13:0] integer_t;
  wire [3:0] fraction_s;
  wire [3:0] sign_ext_204;
  wire [3:0] fraction_t;
  wire [13:0] final_point_s;
  wire [13:0] final_point_t;
  wire [13:0] final_integer_s;
  wire [13:0] final_integer_t;
  wire [3:0] final_fraction_s;
  wire [3:0] final_fraction_t;
  assign half_shift__1 = {1'h0, level} + 5'h03;
  assign decode_176 = half_shift__1 >= 5'h12 ? 18'h0_0000 : 18'h0_0001 << half_shift__1;
  assign add_183 = {1'h0, level[3:2]} + 3'h1;
  assign sub_185 = tex_s[17:3] - decode_176[17:3];
  assign sub_187 = tex_t[17:3] - decode_176[17:3];
  assign point_shift__1 = {add_183, level[1:0]};
  assign adjusted_s = {sub_185, tex_s[2:0]};
  assign adjusted_t = {sub_187, tex_t[2:0]};
  assign shra_193 = shra_18b_by_5b(tex_s, point_shift__1);
  assign shra_195 = shra_18b_by_5b(tex_t, point_shift__1);
  assign scaled_s = shra_18b_by_4b(adjusted_s, level);
  assign scaled_t = shra_18b_by_4b(adjusted_t, level);
  assign point_s = shra_193[13:0];
  assign sign_ext_199 = {14{~clamp_zero}};
  assign point_t = shra_195[13:0];
  assign integer_s = scaled_s[17:4];
  assign integer_t = scaled_t[17:4];
  assign fraction_s = scaled_s[3:0];
  assign sign_ext_204 = {4{~clamp_zero}};
  assign fraction_t = scaled_t[3:0];
  assign final_point_s = point_s & sign_ext_199;
  assign final_point_t = point_t & sign_ext_199;
  assign final_integer_s = integer_s & sign_ext_199;
  assign final_integer_t = integer_t & sign_ext_199;
  assign final_fraction_s = fraction_s & sign_ext_204;
  assign final_fraction_t = fraction_t & sign_ext_204;
  assign out = {final_point_s, final_point_t, final_integer_s, final_integer_t, final_fraction_s, final_fraction_t};
endmodule
