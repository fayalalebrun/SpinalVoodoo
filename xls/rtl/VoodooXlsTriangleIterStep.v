module VoodooXlsTriangleIterStep(
  input wire [65:0] accumulator,
  input wire [65:0] dx_term,
  input wire [65:0] dy_term,
  input wire [1:0] dx_chunk,
  input wire [1:0] dy_chunk,
  output wire [197:0] out
);
  wire [64:0] add_194;
  wire [2:0] concat_199;
  wire [64:0] add_206;
  wire [2:0] concat_210;
  wire [65:0] add_213;
  wire [3:0] one_hot_225;
  wire [3:0] one_hot_231;
  wire [65:0] _next;
  wire eq_227;
  wire eq_233;
  assign add_194 = dx_term[65:1] + dx_term[64:0];
  assign concat_199 = {dx_chunk == 2'h3, dx_chunk == 2'h2, dx_chunk == 2'h1};
  assign add_206 = dy_term[65:1] + dy_term[64:0];
  assign concat_210 = {dy_chunk == 2'h3, dy_chunk == 2'h2, dy_chunk == 2'h1};
  assign add_213 = accumulator + (dx_term & {66{concat_199[0]}} | {dx_term[64:0], 1'h0} & {66{concat_199[1]}} | {add_194, dx_term[0]} & {66{concat_199[2]}});
  assign one_hot_225 = {concat_210[2:0] == 3'h0, concat_210[2] && concat_210[1:0] == 2'h0, concat_210[1] && !concat_210[0], concat_210[0]};
  assign one_hot_231 = {concat_199[2:0] == 3'h0, concat_199[2] && concat_199[1:0] == 2'h0, concat_199[1] && !concat_199[0], concat_199[0]};
  assign _next = add_213 + (dy_term & {66{concat_210[0]}} | {dy_term[64:0], 1'h0} & {66{concat_210[1]}} | {add_206, dy_term[0]} & {66{concat_210[2]}});
  assign eq_227 = concat_210 == one_hot_225[2:0];
  assign eq_233 = concat_199 == one_hot_231[2:0];
  assign out = {_next, dx_term[63:0], 2'h0, dy_term[63:0], 2'h0};
endmodule
