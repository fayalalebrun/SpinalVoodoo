module VoodooXlsTmuRecip(
  input wire [16:0] base,
  input wire [16:0] difference,
  input wire [7:0] fraction,
  input wire [7:0] log_fraction,
  input wire [6:0] msb,
  input wire valid,
  output wire [32:0] out
);
  // lint_off MULTIPLY
  function automatic [24:0] umul25b_17b_x_8b (input reg [16:0] lhs, input reg [7:0] rhs);
    begin
      umul25b_17b_x_8b = lhs * rhs;
    end
  endfunction
  // lint_on MULTIPLY
  wire [5:0] sub_110;
  wire [24:0] product;
  wire [15:0] sub_120;
  wire [16:0] interpolated;
  wire [15:0] add_124;
  wire [15:0] adjust;
  assign sub_110 = 6'h06 - {1'h0, msb[6:2]};
  assign product = umul25b_17b_x_8b(difference, fraction);
  assign sub_120 = {sub_110, ~msb[1:0], 8'h00} - {8'h00, log_fraction};
  assign interpolated = base - product[24:8];
  assign add_124 = sub_120 + 16'hffff;
  assign adjust = add_124 & {16{valid}};
  assign out = {interpolated & {17{valid}}, adjust};
endmodule
