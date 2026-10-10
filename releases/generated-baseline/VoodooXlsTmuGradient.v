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
  wire [8:0] sum1;
  wire [8:0] sum1__1;
  wire [8:0] sum0;
  wire [8:0] sum0__1;
  wire [8:0] sum2;
  wire carry1;
  wire [8:0] sum2__1;
  wire carry1__1;
  wire [8:0] sum3;
  wire carry2;
  wire [8:0] sum3__1;
  wire carry2__1;
  wire [8:0] sum4;
  wire carry3;
  wire [8:0] sum4__1;
  wire carry3__1;
  wire [8:0] sum5;
  wire carry4;
  wire [8:0] sum5__1;
  wire carry4__1;
  wire [8:0] sum6;
  wire carry5;
  wire [8:0] sum6__1;
  wire carry5__1;
  wire carry6;
  wire carry6__1;
  wire carry7;
  wire carry7__1;
  wire [7:0] sum7__1;
  wire [7:0] sum7__2;
  wire [7:0] byte7;
  wire [7:0] byte6;
  wire [7:0] byte5;
  wire [7:0] byte4;
  wire [7:0] byte3;
  wire [7:0] byte2;
  wire [7:0] byte1;
  wire [7:0] byte0;
  wire [7:0] byte7__1;
  wire [7:0] byte6__1;
  wire [7:0] byte5__1;
  wire [7:0] byte4__1;
  wire [7:0] byte3__1;
  wire [7:0] byte2__1;
  wire [7:0] byte1__1;
  wire [7:0] byte0__1;
  wire [63:0] dx;
  wire [63:0] dy;
  assign sx = smul64b_32b_x_32b(ds_dx, ds_dx);
  assign tx = smul64b_32b_x_32b(dt_dx, dt_dx);
  assign sy = smul64b_32b_x_32b(ds_dy, ds_dy);
  assign ty = smul64b_32b_x_32b(dt_dy, dt_dy);
  assign sum1 = {1'h0, sx[15:8]} + {1'h0, tx[15:8]};
  assign sum1__1 = {1'h0, sy[15:8]} + {1'h0, ty[15:8]};
  assign sum0 = {1'h0, sx[7:0]} + {1'h0, tx[7:0]};
  assign sum0__1 = {1'h0, sy[7:0]} + {1'h0, ty[7:0]};
  assign sum2 = {1'h0, sx[23:16]} + {1'h0, tx[23:16]};
  assign carry1 = sum0[8];
  assign sum2__1 = {1'h0, sy[23:16]} + {1'h0, ty[23:16]};
  assign carry1__1 = sum0__1[8];
  assign sum3 = {1'h0, sx[31:24]} + {1'h0, tx[31:24]};
  assign carry2 = sum1[8] | sum1[7:0] == 8'hff & carry1;
  assign sum3__1 = {1'h0, sy[31:24]} + {1'h0, ty[31:24]};
  assign carry2__1 = sum1__1[8] | sum1__1[7:0] == 8'hff & carry1__1;
  assign sum4 = {1'h0, sx[39:32]} + {1'h0, tx[39:32]};
  assign carry3 = sum2[8] | sum2[7:0] == 8'hff & carry2;
  assign sum4__1 = {1'h0, sy[39:32]} + {1'h0, ty[39:32]};
  assign carry3__1 = sum2__1[8] | sum2__1[7:0] == 8'hff & carry2__1;
  assign sum5 = {1'h0, sx[47:40]} + {1'h0, tx[47:40]};
  assign carry4 = sum3[8] | sum3[7:0] == 8'hff & carry3;
  assign sum5__1 = {1'h0, sy[47:40]} + {1'h0, ty[47:40]};
  assign carry4__1 = sum3__1[8] | sum3__1[7:0] == 8'hff & carry3__1;
  assign sum6 = {1'h0, sx[55:48]} + {1'h0, tx[55:48]};
  assign carry5 = sum4[8] | sum4[7:0] == 8'hff & carry4;
  assign sum6__1 = {1'h0, sy[55:48]} + {1'h0, ty[55:48]};
  assign carry5__1 = sum4__1[8] | sum4__1[7:0] == 8'hff & carry4__1;
  assign carry6 = sum5[8] | sum5[7:0] == 8'hff & carry5;
  assign carry6__1 = sum5__1[8] | sum5__1[7:0] == 8'hff & carry5__1;
  assign carry7 = sum6[8] | sum6[7:0] == 8'hff & carry6;
  assign carry7__1 = sum6__1[8] | sum6__1[7:0] == 8'hff & carry6__1;
  assign sum7__1 = sx[63:56] + tx[63:56];
  assign sum7__2 = sy[63:56] + ty[63:56];
  assign byte7 = sum7__1 + {7'h00, carry7};
  assign byte6 = sum6[7:0] + {7'h00, carry6};
  assign byte5 = sum5[7:0] + {7'h00, carry5};
  assign byte4 = sum4[7:0] + {7'h00, carry4};
  assign byte3 = sum3[7:0] + {7'h00, carry3};
  assign byte2 = sum2[7:0] + {7'h00, carry2};
  assign byte1 = sum1[7:0] + {7'h00, carry1};
  assign byte0 = sum0[7:0];
  assign byte7__1 = sum7__2 + {7'h00, carry7__1};
  assign byte6__1 = sum6__1[7:0] + {7'h00, carry6__1};
  assign byte5__1 = sum5__1[7:0] + {7'h00, carry5__1};
  assign byte4__1 = sum4__1[7:0] + {7'h00, carry4__1};
  assign byte3__1 = sum3__1[7:0] + {7'h00, carry3__1};
  assign byte2__1 = sum2__1[7:0] + {7'h00, carry2__1};
  assign byte1__1 = sum1__1[7:0] + {7'h00, carry1__1};
  assign byte0__1 = sum0__1[7:0];
  assign dx = {byte7, byte6, byte5, byte4, byte3, byte2, byte1, byte0};
  assign dy = {byte7__1, byte6__1, byte5__1, byte4__1, byte3__1, byte2__1, byte1__1, byte0__1};
  assign out = dx > dy ? dx : dy;
endmodule
