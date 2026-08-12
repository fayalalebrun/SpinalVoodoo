module VoodooXlsTriangleIterLoad(
  input wire [47:0] start,
  input wire [47:0] dx,
  input wire [47:0] dy,
  input wire [3:0] dx_subpixel,
  input wire [3:0] dy_subpixel,
  input wire adjust,
  input wire dx_negative,
  input wire dy_negative,
  output wire [197:0] out
);
  wire [53:0] wide;
  wire [53:0] wide__1;
  wire [49:0] p0__1;
  wire [51:0] p2__1;
  wire [49:0] p0__2;
  wire [51:0] p2__2;
  wire [48:0] p1__3;
  wire [48:0] p3__3;
  wire [48:0] p1__4;
  wire [48:0] p3__4;
  wire [48:0] add_411;
  wire [48:0] add_412;
  wire [48:0] add_414;
  wire [48:0] add_415;
  wire [47:0] bit_slice_417;
  wire [49:0] concat_418;
  wire [47:0] bit_slice_419;
  wire [49:0] concat_420;
  wire [50:0] add_425;
  wire [50:0] add_428;
  wire [52:0] concat_431;
  wire [52:0] concat_432;
  wire [53:0] products;
  wire [49:0] correction;
  wire [50:0] sign_ext_438;
  wire [50:0] add_441;
  wire [50:0] accumulator_squeezed;
  wire [47:0] sel_447;
  wire [47:0] sel_448;
  wire [65:0] accumulator;
  wire [65:0] dx_term;
  wire [65:0] dy_term;
  assign wide = {{6{dx[47]}}, dx};
  assign wide__1 = {{6{dy[47]}}, dy};
  assign p0__1 = wide[49:0] & {50{dx_subpixel[0]}};
  assign p2__1 = {wide[49:0], 2'h0} & {52{dx_subpixel[2]}};
  assign p0__2 = wide__1[49:0] & {50{dy_subpixel[0]}};
  assign p2__2 = {wide__1[49:0], 2'h0} & {52{dy_subpixel[2]}};
  assign p1__3 = wide[48:0] & {49{dx_subpixel[1]}};
  assign p3__3 = wide[48:0] & {49{dx_subpixel[3]}};
  assign p1__4 = wide__1[48:0] & {49{dy_subpixel[1]}};
  assign p3__4 = wide__1[48:0] & {49{dy_subpixel[3]}};
  assign add_411 = p0__1[49:1] + p1__3;
  assign add_412 = p2__1[51:3] + p3__3;
  assign add_414 = p0__2[49:1] + p1__4;
  assign add_415 = p2__2[51:3] + p3__4;
  assign bit_slice_417 = add_411[48:1];
  assign concat_418 = {add_412, p2__1[2]};
  assign bit_slice_419 = add_414[48:1];
  assign concat_420 = {add_415, p2__2[2]};
  assign add_425 = {{3{bit_slice_417[47]}}, bit_slice_417} + {{1{concat_418[49]}}, concat_418};
  assign add_428 = {{3{bit_slice_419[47]}}, bit_slice_419} + {{1{concat_420[49]}}, concat_420};
  assign concat_431 = {add_425, add_411[0], p0__1[0]};
  assign concat_432 = {add_428, add_414[0], p0__2[0]};
  assign products = {{1{concat_431[52]}}, concat_431} + {{1{concat_432[52]}}, concat_432};
  assign correction = products[53:4];
  assign sign_ext_438 = {{3{start[47]}}, start};
  assign add_441 = sign_ext_438 + {{1{correction[49]}}, correction};
  assign accumulator_squeezed = adjust ? add_441 : sign_ext_438;
  assign sel_447 = dx_negative ? -dx : dx;
  assign sel_448 = dy_negative ? -dy : dy;
  assign accumulator = {{15{accumulator_squeezed[50]}}, accumulator_squeezed};
  assign dx_term = {{18{sel_447[47]}}, sel_447};
  assign dy_term = {{18{sel_448[47]}}, sel_448};
  assign out = {accumulator, dx_term, dy_term};
endmodule
