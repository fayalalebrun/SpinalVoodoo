module VoodooXlsDepthCompare(
  input wire [2:0] comparison,
  input wire [15:0] new_depth,
  input wire [15:0] old_depth,
  output wire out
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
  wire ule_89;
  wire eq_90;
  wire ult_91;
  assign ule_89 = new_depth <= old_depth;
  assign eq_90 = new_depth == old_depth;
  assign ult_91 = new_depth < old_depth;
  assign out = priority_sel_1b_7way({comparison == 3'h6, comparison == 3'h5, comparison == 3'h4, comparison == 3'h3, comparison == 3'h2, comparison == 3'h1, comparison == 3'h0}, 1'h0, ult_91, eq_90, ule_89, ~ule_89, ~eq_90, ~ult_91, 1'h1);
endmodule
