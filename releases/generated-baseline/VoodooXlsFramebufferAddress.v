module VoodooXlsFramebufferAddress(
  input wire [25:0] base,
  input wire [15:0] x,
  input wire [15:0] y,
  input wire [10:0] stride,
  output wire [25:0] out
);
  wire [17:0] p0__1;
  wire [16:0] p1__2;
  wire [18:0] p2__1;
  wire [16:0] add_353;
  wire [15:0] p3__2;
  wire [15:0] add_357;
  wire [16:0] add_362;
  wire [18:0] add_367;
  wire [24:0] add_373;
  assign p0__1 = {2'h0, y} & {18{stride[6]}};
  assign p1__2 = {1'h0, y} & {17{stride[7]}};
  assign p2__1 = {1'h0, y, 2'h0} & {19{stride[8]}};
  assign add_353 = p0__1[17:1] + p1__2;
  assign p3__2 = y & {16{stride[9]}};
  assign add_357 = p2__1[18:3] + p3__2;
  assign add_362 = {1'h0, add_353[16:1]} + {add_357, p2__1[2]};
  assign add_367 = base[25:7] + {add_362, add_353[0], p0__1[0]};
  assign add_373 = {add_367, base[6:1]} + {9'h000, x};
  assign out = {add_373, base[0]};
endmodule
