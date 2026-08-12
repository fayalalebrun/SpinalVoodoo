module VoodooXlsLfbDecode(
  input wire [31:0] data32,
  input wire [15:0] data16,
  input wire [3:0] format,
  input wire [1:0] lanes,
  input wire [15:0] default_depth,
  input wire alpha_planes,
  output wire [49:0] out
);
  function automatic [7:0] priority_sel_8b_3way (input reg [2:0] sel, input reg [7:0] case0, input reg [7:0] case1, input reg [7:0] case2, input reg [7:0] default_value);
    begin
      casez (sel)
        3'b??1: begin
          priority_sel_8b_3way = case0;
        end
        3'b?10: begin
          priority_sel_8b_3way = case1;
        end
        3'b100: begin
          priority_sel_8b_3way = case2;
        end
        3'b000: begin
          priority_sel_8b_3way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_8b_3way = 8'dx;
        end
      endcase
    end
  endfunction
  function automatic [15:0] priority_sel_16b_2way (input reg [1:0] sel, input reg [15:0] case0, input reg [15:0] case1, input reg [15:0] default_value);
    begin
      casez (sel)
        2'b?1: begin
          priority_sel_16b_2way = case0;
        end
        2'b10: begin
          priority_sel_16b_2way = case1;
        end
        2'b00: begin
          priority_sel_16b_2way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_16b_2way = 16'dx;
        end
      endcase
    end
  endfunction
  function automatic [7:0] priority_sel_8b_4way (input reg [3:0] sel, input reg [7:0] case0, input reg [7:0] case1, input reg [7:0] case2, input reg [7:0] case3, input reg [7:0] default_value);
    begin
      casez (sel)
        4'b???1: begin
          priority_sel_8b_4way = case0;
        end
        4'b??10: begin
          priority_sel_8b_4way = case1;
        end
        4'b?100: begin
          priority_sel_8b_4way = case2;
        end
        4'b1000: begin
          priority_sel_8b_4way = case3;
        end
        4'b0000: begin
          priority_sel_8b_4way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_8b_4way = 8'dx;
        end
      endcase
    end
  endfunction
  wire eq_720;
  wire eq_721;
  wire eq_722;
  wire nor_723;
  wire eq_729;
  wire or_732;
  wire or_733;
  wire eq_761;
  wire eq_763;
  wire eq_754;
  wire eq_737;
  wire eq_738;
  wire [2:0] concat_749;
  wire or_778;
  wire nor_765;
  wire [7:0] swizzled__24_to_32;
  wire [7:0] swizzled__16_to_24;
  wire [4:0] data16_portion_0_width_5;
  wire [2:0] data16_portion_2_width_3;
  wire eq_785;
  wire [1:0] concat_809;
  wire [5:0] concat_798;
  wire [3:0] concat_792;
  wire [2:0] one_hot_825;
  wire [6:0] one_hot_831;
  wire [4:0] one_hot_837;
  wire [15:0] rgb__8_to_24;
  wire [7:0] rgb__0_to_8;
  wire [15:0] depth;
  wire [7:0] alpha;
  wire rgb_write;
  wire aux_write;
  wire eq_827;
  wire eq_833;
  wire eq_839;
  assign eq_720 = format == 4'h4;
  assign eq_721 = format == 4'h5;
  assign eq_722 = lanes == 2'h1;
  assign nor_723 = ~(lanes[0] | lanes[1]);
  assign eq_729 = lanes == 2'h2;
  assign or_732 = eq_720 | eq_721;
  assign or_733 = eq_722 | nor_723;
  assign eq_761 = format == 4'h0;
  assign eq_763 = format == 4'h2;
  assign eq_754 = format == 4'hc;
  assign eq_737 = format == 4'hd;
  assign eq_738 = format == 4'he;
  assign concat_749 = {eq_729, eq_722, nor_723};
  assign or_778 = eq_754 | eq_737 | eq_738;
  assign nor_765 = ~((|format[3:2]) | (&format[1:0]));
  assign swizzled__24_to_32 = priority_sel_8b_3way(concat_749, data32[23:16], data32[7:0], data32[31:24], data32[15:8]);
  assign swizzled__16_to_24 = or_733 ? data32[15:8] : data32[23:16];
  assign data16_portion_0_width_5 = data16[4:0];
  assign data16_portion_2_width_3 = data16[4:2];
  assign eq_785 = format == 4'hf;
  assign concat_809 = {format > 4'hb, eq_761 | format == 4'h1 | eq_763 | eq_720 | eq_721};
  assign concat_798 = {or_778, or_732 & ~(eq_729 | eq_722) & (lanes[0] | lanes[1]), {3{or_732}} & concat_749, nor_765};
  assign concat_792 = {eq_737 | eq_738, eq_754, or_732, nor_765};
  assign one_hot_825 = {concat_809[1:0] == 2'h0, concat_809[1] && !concat_809[0], concat_809[0]};
  assign one_hot_831 = {concat_798[5:0] == 6'h00, concat_798[5] && concat_798[4:0] == 5'h00, concat_798[4] && concat_798[3:0] == 4'h0, concat_798[3] && concat_798[2:0] == 3'h0, concat_798[2] && concat_798[1:0] == 2'h0, concat_798[1] && !concat_798[0], concat_798[0]};
  assign one_hot_837 = {concat_792[3:0] == 4'h0, concat_792[3] && concat_792[2:0] == 3'h0, concat_792[2] && concat_792[1:0] == 2'h0, concat_792[1] && !concat_792[0], concat_792[0]};
  assign rgb__8_to_24 = (eq_761 ? {data16[15:11], data16[15:13], data16[10:5], data16[10:9]} : {data16[14:10], data16[14:12], data16[9:5], data16[9:7]}) & {16{concat_792[0]}} | {swizzled__24_to_32, swizzled__16_to_24} & {16{concat_792[1]}} | {data32[15:11], data32[15:13], data32[10:5], data32[10:9]} & {16{concat_792[2]}} | {data32[14:10], data32[14:12], data32[9:5], data32[9:7]} & {16{concat_792[3]}};
  assign rgb__0_to_8 = {data16_portion_0_width_5, data16_portion_2_width_3} & {8{concat_798[0]}} | data32[7:0] & {8{concat_798[1]}} | data32[23:16] & {8{concat_798[2]}} | data32[15:8] & {8{concat_798[3]}} | data32[31:24] & {8{concat_798[4]}} | {data32[4:0], data32[4:2]} & {8{concat_798[5]}};
  assign depth = priority_sel_16b_2way({eq_785, or_778}, data32[31:16], data16, default_depth);
  assign alpha = priority_sel_8b_4way({eq_738, eq_763, eq_721 & ~or_733, eq_721 & or_733}, data32[31:24], data32[7:0], {8{data16[15]}}, {8{data32[15]}}, 8'hff);
  assign rgb_write = ~eq_785;
  assign aux_write = alpha_planes & concat_809[0] | 1'h1 & concat_809[1];
  assign eq_827 = concat_809 == one_hot_825[1:0];
  assign eq_833 = concat_798 == one_hot_831[5:0];
  assign eq_839 = concat_792 == one_hot_837[3:0];
  assign out = {rgb__8_to_24, rgb__0_to_8, depth, alpha, rgb_write, aux_write};
endmodule
