module VoodooXlsTextureFilter(
  input wire [31:0] t0,
  input wire [31:0] t1,
  input wire [31:0] t2,
  input wire [31:0] t3,
  input wire [3:0] ds,
  input wire [3:0] dt,
  input wire bilinear,
  output wire [31:0] out
);
  // lint_off MULTIPLY
  function automatic [7:0] umul8b_4b_x_5b (input reg [3:0] lhs, input reg [4:0] rhs);
    begin
      umul8b_4b_x_5b = lhs * rhs;
    end
  endfunction
  // lint_on MULTIPLY
  // lint_off MULTIPLY
  function automatic [7:0] umul8b_5b_x_4b (input reg [4:0] lhs, input reg [3:0] rhs);
    begin
      umul8b_5b_x_4b = lhs * rhs;
    end
  endfunction
  // lint_on MULTIPLY
  // lint_off MULTIPLY
  function automatic [7:0] umul8b_4b_x_4b (input reg [3:0] lhs, input reg [3:0] rhs);
    begin
      umul8b_4b_x_4b = lhs * rhs;
    end
  endfunction
  // lint_on MULTIPLY
  // lint_off MULTIPLY
  function automatic [8:0] umul9b_5b_x_5b (input reg [4:0] lhs, input reg [4:0] rhs);
    begin
      umul9b_5b_x_5b = lhs * rhs;
    end
  endfunction
  // lint_on MULTIPLY
  // lint_off MULTIPLY
  function automatic [15:0] umul16b_8b_x_8b (input reg [7:0] lhs, input reg [7:0] rhs);
    begin
      umul16b_8b_x_8b = lhs * rhs;
    end
  endfunction
  // lint_on MULTIPLY
  // lint_off MULTIPLY
  function automatic [16:0] umul17b_8b_x_9b (input reg [7:0] lhs, input reg [8:0] rhs);
    begin
      umul17b_8b_x_9b = lhs * rhs;
    end
  endfunction
  // lint_on MULTIPLY
  wire [4:0] y;
  wire [4:0] x;
  wire [4:0] iy;
  wire [4:0] ix;
  wire [7:0] umul_577;
  wire [7:0] umul_579;
  wire [7:0] umul_581;
  wire [8:0] umul_592;
  wire [15:0] umul_593;
  wire [15:0] umul_594;
  wire [15:0] umul_595;
  wire [15:0] umul_597;
  wire [15:0] umul_598;
  wire [15:0] umul_599;
  wire [15:0] umul_601;
  wire [15:0] umul_602;
  wire [15:0] umul_603;
  wire [15:0] umul_605;
  wire [15:0] umul_606;
  wire [15:0] umul_607;
  wire [16:0] umul_608;
  wire [16:0] umul_612;
  wire [16:0] umul_616;
  wire [16:0] umul_620;
  wire [16:0] add_624;
  wire [16:0] add_625;
  wire [16:0] add_626;
  wire [16:0] add_627;
  wire [16:0] add_628;
  wire [16:0] add_629;
  wire [16:0] add_630;
  wire [16:0] add_631;
  wire [17:0] sum_associative_element;
  wire [17:0] sum_associative_element__1;
  wire [17:0] sum_associative_element__2;
  wire [17:0] sum_associative_element__3;
  wire [17:0] sum_associative_element__4;
  wire [17:0] sum_associative_element__5;
  wire [17:0] sum_associative_element__6;
  wire [17:0] sum_associative_element__7;
  wire [17:0] sum;
  wire [17:0] sum__1;
  wire [17:0] sum__2;
  wire [17:0] sum__3;
  assign y = {1'h0, dt};
  assign x = {1'h0, ds};
  assign iy = 5'h10 - y;
  assign ix = 5'h10 - x;
  assign umul_577 = umul8b_4b_x_5b(ds, iy);
  assign umul_579 = umul8b_5b_x_4b(ix, dt);
  assign umul_581 = umul8b_4b_x_4b(ds, dt);
  assign umul_592 = umul9b_5b_x_5b(ix, iy);
  assign umul_593 = umul16b_8b_x_8b(t1[31:24], umul_577);
  assign umul_594 = umul16b_8b_x_8b(t2[31:24], umul_579);
  assign umul_595 = umul16b_8b_x_8b(t3[31:24], umul_581);
  assign umul_597 = umul16b_8b_x_8b(t1[23:16], umul_577);
  assign umul_598 = umul16b_8b_x_8b(t2[23:16], umul_579);
  assign umul_599 = umul16b_8b_x_8b(t3[23:16], umul_581);
  assign umul_601 = umul16b_8b_x_8b(t1[15:8], umul_577);
  assign umul_602 = umul16b_8b_x_8b(t2[15:8], umul_579);
  assign umul_603 = umul16b_8b_x_8b(t3[15:8], umul_581);
  assign umul_605 = umul16b_8b_x_8b(t1[7:0], umul_577);
  assign umul_606 = umul16b_8b_x_8b(t2[7:0], umul_579);
  assign umul_607 = umul16b_8b_x_8b(t3[7:0], umul_581);
  assign umul_608 = umul17b_8b_x_9b(t0[31:24], umul_592);
  assign umul_612 = umul17b_8b_x_9b(t0[23:16], umul_592);
  assign umul_616 = umul17b_8b_x_9b(t0[15:8], umul_592);
  assign umul_620 = umul17b_8b_x_9b(t0[7:0], umul_592);
  assign add_624 = umul_608 + {1'h0, umul_593};
  assign add_625 = {1'h0, umul_594} + {1'h0, umul_595};
  assign add_626 = umul_612 + {1'h0, umul_597};
  assign add_627 = {1'h0, umul_598} + {1'h0, umul_599};
  assign add_628 = umul_616 + {1'h0, umul_601};
  assign add_629 = {1'h0, umul_602} + {1'h0, umul_603};
  assign add_630 = umul_620 + {1'h0, umul_605};
  assign add_631 = {1'h0, umul_606} + {1'h0, umul_607};
  assign sum_associative_element = {1'h0, add_624};
  assign sum_associative_element__1 = {1'h0, add_625};
  assign sum_associative_element__2 = {1'h0, add_626};
  assign sum_associative_element__3 = {1'h0, add_627};
  assign sum_associative_element__4 = {1'h0, add_628};
  assign sum_associative_element__5 = {1'h0, add_629};
  assign sum_associative_element__6 = {1'h0, add_630};
  assign sum_associative_element__7 = {1'h0, add_631};
  assign sum = sum_associative_element + sum_associative_element__1;
  assign sum__1 = sum_associative_element__2 + sum_associative_element__3;
  assign sum__2 = sum_associative_element__4 + sum_associative_element__5;
  assign sum__3 = sum_associative_element__6 + sum_associative_element__7;
  assign out = bilinear ? {sum[15:8], sum__1[15:8], sum__2[15:8], sum__3[15:8]} : t0;
endmodule
