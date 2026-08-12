module VoodooXlsTmuGradient(
  input wire [31:0] ds_dx,
  input wire [31:0] dt_dx,
  input wire [31:0] ds_dy,
  input wire [31:0] dt_dy,
  output wire [63:0] out
);
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [63:0] smul64b_32b_x_32b (input reg [31:0] lhs, input reg [31:0] rhs);
    reg signed [31:0] signed_lhs;
    reg signed [31:0] signed_rhs;
    reg signed [63:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul64b_32b_x_32b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  wire [63:0] sx;
  wire [63:0] tx;
  wire [63:0] sy;
  wire [63:0] ty;
  wire [63:0] dx;
  wire [63:0] dy;
  assign sx = smul64b_32b_x_32b(ds_dx, ds_dx);
  assign tx = smul64b_32b_x_32b(dt_dx, dt_dx);
  assign sy = smul64b_32b_x_32b(ds_dy, ds_dy);
  assign ty = smul64b_32b_x_32b(dt_dy, dt_dy);
  assign dx = sx + tx;
  assign dy = sy + ty;
  assign out = dx > dy ? dx : dy;
endmodule
