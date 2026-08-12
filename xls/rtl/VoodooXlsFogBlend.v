module VoodooXlsFogBlend(
  input wire [23:0] src_rgb,
  input wire [23:0] fog_rgb,
  input wire [7:0] factor,
  input wire enable,
  input wire add,
  input wire mult,
  input wire constant,
  output wire [23:0] out
);
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [17:0] smul18b_9b_x_10b (input reg [8:0] lhs, input reg [9:0] rhs);
    reg signed [8:0] signed_lhs;
    reg signed [9:0] signed_rhs;
    reg signed [17:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul18b_9b_x_10b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  wire [8:0] concat_510;
  wire [8:0] sign_ext_511;
  wire [8:0] concat_512;
  wire [8:0] sign_ext_513;
  wire [8:0] concat_516;
  wire [8:0] concat_517;
  wire [8:0] concat_518;
  wire [8:0] concat_519;
  wire [8:0] base__3;
  wire [8:0] add_522;
  wire [8:0] base__4;
  wire [8:0] base__5;
  wire [8:0] difference__1;
  wire [9:0] concat_528;
  wire [8:0] difference__2;
  wire [8:0] difference__3;
  wire [17:0] smul_532;
  wire [17:0] smul_533;
  wire [17:0] smul_534;
  wire [9:0] scaled_squeezed;
  wire [9:0] scaled__1_squeezed;
  wire [9:0] scaled__2_squeezed;
  wire [9:0] add_541;
  wire [9:0] add_542;
  wire [9:0] add_543;
  wire [9:0] result_squeezed;
  wire [9:0] result__1_squeezed;
  wire [9:0] result__2_squeezed;
  wire [8:0] sum;
  wire [8:0] sum__1;
  wire [8:0] sum__2;
  wire [23:0] concat_587;
  assign concat_510 = {1'h0, fog_rgb[23:16]};
  assign sign_ext_511 = {9{~add}};
  assign concat_512 = {1'h0, src_rgb[23:16]};
  assign sign_ext_513 = {9{~mult}};
  assign concat_516 = {1'h0, fog_rgb[15:8]};
  assign concat_517 = {1'h0, src_rgb[15:8]};
  assign concat_518 = {1'h0, fog_rgb[7:0]};
  assign concat_519 = {1'h0, src_rgb[7:0]};
  assign base__3 = concat_510 & sign_ext_511;
  assign add_522 = {1'h0, factor} + 9'h001;
  assign base__4 = concat_516 & sign_ext_511;
  assign base__5 = concat_518 & sign_ext_511;
  assign difference__1 = base__3 - (concat_512 & sign_ext_513);
  assign concat_528 = {1'h0, add_522};
  assign difference__2 = base__4 - (concat_517 & sign_ext_513);
  assign difference__3 = base__5 - (concat_519 & sign_ext_513);
  assign smul_532 = smul18b_9b_x_10b(difference__1, concat_528);
  assign smul_533 = smul18b_9b_x_10b(difference__2, concat_528);
  assign smul_534 = smul18b_9b_x_10b(difference__3, concat_528);
  assign scaled_squeezed = smul_532[17:8];
  assign scaled__1_squeezed = smul_533[17:8];
  assign scaled__2_squeezed = smul_534[17:8];
  assign add_541 = {2'h0, src_rgb[23:16]} + scaled_squeezed;
  assign add_542 = {2'h0, src_rgb[15:8]} + scaled__1_squeezed;
  assign add_543 = {2'h0, src_rgb[7:0]} + scaled__2_squeezed;
  assign result_squeezed = mult ? scaled_squeezed : add_541;
  assign result__1_squeezed = mult ? scaled__1_squeezed : add_542;
  assign result__2_squeezed = mult ? scaled__2_squeezed : add_543;
  assign sum = concat_512 + concat_510;
  assign sum__1 = concat_517 + concat_516;
  assign sum__2 = concat_519 + concat_518;
  assign concat_587 = {constant ? (sum[8] ? 8'hff : sum[7:0]) : ($signed(result_squeezed) > $signed(10'h0ff) ? 8'hff : result_squeezed[7:0]) & {8{~result_squeezed[9]}}, constant ? (sum__1[8] ? 8'hff : sum__1[7:0]) : ($signed(result__1_squeezed) > $signed(10'h0ff) ? 8'hff : result__1_squeezed[7:0]) & {8{~result__1_squeezed[9]}}, constant ? (sum__2[8] ? 8'hff : sum__2[7:0]) : ($signed(result__2_squeezed) > $signed(10'h0ff) ? 8'hff : result__2_squeezed[7:0]) & {8{~result__2_squeezed[9]}}};
  assign out = enable ? concat_587 : src_rgb;
endmodule
