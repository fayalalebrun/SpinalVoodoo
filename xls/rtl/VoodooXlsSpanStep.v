module VoodooXlsSpanStep(
  input wire [47:0] current,
  input wire [47:0] dx,
  input wire [47:0] dy,
  input wire [1:0] mode,
  output wire [47:0] out
);
  function automatic [47:0] priority_sel_48b_3way (input reg [2:0] sel, input reg [47:0] case0, input reg [47:0] case1, input reg [47:0] case2, input reg [47:0] default_value);
    begin
      casez (sel)
        3'b??1: begin
          priority_sel_48b_3way = case0;
        end
        3'b?10: begin
          priority_sel_48b_3way = case1;
        end
        3'b100: begin
          priority_sel_48b_3way = case2;
        end
        3'b000: begin
          priority_sel_48b_3way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_48b_3way = 48'dx;
        end
      endcase
    end
  endfunction
  wire [47:0] add_59;
  wire [47:0] add_61;
  wire [47:0] sub_62;
  wire [47:0] sub_63;
  assign add_59 = current + dy;
  assign add_61 = current + dx;
  assign sub_62 = current - dx;
  assign sub_63 = add_59 - dx;
  assign out = priority_sel_48b_3way({mode == 2'h2, mode == 2'h1, ~(mode[0] | mode[1])}, add_61, sub_62, add_59, sub_63);
endmodule
