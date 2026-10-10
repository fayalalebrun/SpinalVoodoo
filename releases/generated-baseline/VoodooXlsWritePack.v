module VoodooXlsWritePack(
  input wire [25:0] base,
  input wire [15:0] x,
  input wire [15:0] y,
  input wire [10:0] stride,
  input wire [15:0] data,
  output wire [61:0] out
);
  wire [17:0] p0__1;
  wire [16:0] p1__2;
  wire [18:0] p2__1;
  wire [16:0] add_518;
  wire [15:0] p3__2;
  wire [15:0] add_522;
  wire [16:0] add_527;
  wire [18:0] add_532;
  wire [24:0] add_538;
  wire [2:0] lit_3_squeezed;
  wire [2:0] lit_12_squeezed;
  wire [2:0] sel_546;
  wire [31:0] packed_data;
  assign p0__1 = {2'h0, y} & {18{stride[6]}};
  assign p1__2 = {1'h0, y} & {17{stride[7]}};
  assign p2__1 = {1'h0, y, 2'h0} & {19{stride[8]}};
  assign add_518 = p0__1[17:1] + p1__2;
  assign p3__2 = y & {16{stride[9]}};
  assign add_522 = p2__1[18:3] + p3__2;
  assign add_527 = {1'h0, add_518[16:1]} + {add_522, p2__1[2]};
  assign add_532 = base[25:7] + {add_527, add_518[0], p0__1[0]};
  assign add_538 = {add_532, base[6:1]} + {9'h000, x};
  assign lit_3_squeezed = 3'h3;
  assign lit_12_squeezed = 3'h4;
  assign sel_546 = add_538[0] ? lit_12_squeezed : lit_3_squeezed;
  assign packed_data = add_538[0] ? {data, 16'h0000} : {16'h0000, data};
  assign out = {add_538[24:1], 2'h0, packed_data, {{1{sel_546[2]}}, sel_546}};
endmodule
