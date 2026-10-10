module VoodooXlsPixelTests(
  input wire [23:0] color,
  input wire [23:0] chroma,
  input wire [7:0] alpha,
  input wire [7:0] alpha_reference,
  input wire [2:0] alpha_function,
  output wire [1:0] out
);
  function automatic priority_sel_1b_7way (input reg [6:0] sel, input reg case0, input reg case1, input reg case2, input reg case3, input reg case4, input reg case5, input reg case6, input reg default_value);
    begin
      casez (sel)
        7'b??????1: begin
          priority_sel_1b_7way = case0;
        end
        7'b?????10: begin
          priority_sel_1b_7way = case1;
        end
        7'b????100: begin
          priority_sel_1b_7way = case2;
        end
        7'b???1000: begin
          priority_sel_1b_7way = case3;
        end
        7'b??10000: begin
          priority_sel_1b_7way = case4;
        end
        7'b?100000: begin
          priority_sel_1b_7way = case5;
        end
        7'b1000000: begin
          priority_sel_1b_7way = case6;
        end
        7'b000_0000: begin
          priority_sel_1b_7way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_1b_7way = 1'dx;
        end
      endcase
    end
  endfunction
  wire ule_145;
  wire eq_146;
  wire ult_147;
  wire chroma_match;
  wire alpha_pass;
  assign ule_145 = alpha <= alpha_reference;
  assign eq_146 = alpha == alpha_reference;
  assign ult_147 = alpha < alpha_reference;
  assign chroma_match = color == chroma;
  assign alpha_pass = priority_sel_1b_7way({alpha_function == 3'h6, alpha_function == 3'h5, alpha_function == 3'h4, alpha_function == 3'h3, alpha_function == 3'h2, alpha_function == 3'h1, alpha_function == 3'h0}, 1'h0, ult_147, eq_146, ule_145, ~ule_145, ~eq_146, ~ult_147, 1'h1);
  assign out = {chroma_match, alpha_pass};
endmodule
