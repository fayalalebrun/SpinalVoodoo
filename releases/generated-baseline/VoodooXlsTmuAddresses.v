module VoodooXlsTmuAddresses(
  input wire [13:0] point_s,
  input wire [13:0] point_t,
  input wire [13:0] integer_s,
  input wire [13:0] integer_t,
  input wire [3:0] width_bits,
  input wire [3:0] height_bits,
  input wire clamp_s,
  input wire clamp_t,
  input wire is_16_bit,
  input wire packed_layout,
  input wire [21:0] lod_base,
  input wire [3:0] lod_shift,
  input wire [23:0] base0,
  input wire [23:0] base1,
  input wire [23:0] base2,
  input wire [23:0] base38,
  input wire multibase,
  input wire [3:0] level,
  output wire [129:0] out
);
  wire [9:0] size;
  wire [13:0] add_1045;
  wire [9:0] size__1;
  wire [13:0] concat_1049;
  wire [9:0] shll_1050;
  wire [13:0] add_1054;
  wire [13:0] concat_1055;
  wire [9:0] shll_1056;
  wire [9:0] maximum;
  wire [9:0] maximum__1;
  wire [9:0] wrapped;
  wire [9:0] clamped;
  wire [9:0] wrapped__2;
  wire [9:0] clamped__2;
  wire [9:0] wrapped__4;
  wire [9:0] clamped__4;
  wire [9:0] wrapped__1;
  wire [9:0] clamped__1;
  wire [9:0] point_x;
  wire [9:0] wrapped__3;
  wire [9:0] clamped__3;
  wire [9:0] x0;
  wire [9:0] x1;
  wire [9:0] wrapped__5;
  wire [9:0] clamped__5;
  wire [9:0] point_y;
  wire [9:0] y0;
  wire [9:0] y1;
  wire [10:0] column_squeezed_const_msb_bits;
  wire [10:0] column_squeezed;
  wire [10:0] column__2_squeezed;
  wire [10:0] column__4_squeezed;
  wire [1:0] offset__4;
  wire [1:0] offset__3;
  wire [7:0] offset__2;
  wire [5:0] point_x_portion_2_width_6;
  wire [21:0] column;
  wire [4:0] row_shift;
  wire [1:0] offset__5;
  wire [7:0] offset__6;
  wire [5:0] x0_portion_2_width_6;
  wire [21:0] column__2;
  wire [5:0] x1_portion_2_width_6;
  wire [21:0] column__4;
  wire [1:0] offset__11;
  wire [7:0] offset__12;
  wire [22:0] linear_base__1;
  wire [21:0] add_1184;
  wire [21:0] row;
  wire [21:0] add_1189;
  wire [21:0] row__1;
  wire [21:0] add_1194;
  wire [21:0] row__3;
  wire [22:0] add_1198;
  wire [2:0] column__1_squeezed_portion_0_width_3;
  wire [21:0] add_1200;
  wire [22:0] add_1201;
  wire [2:0] column__3_squeezed_portion_0_width_3;
  wire [21:0] add_1203;
  wire [22:0] add_1204;
  wire [2:0] column__5_squeezed_portion_0_width_3;
  wire [21:0] add_1206;
  wire [22:0] add_1207;
  wire [21:0] add_1208;
  wire [22:0] add_1209;
  wire [21:0] add_1210;
  wire [25:0] point;
  wire [25:0] bi0;
  wire [25:0] bi1;
  wire [25:0] bi2;
  wire [25:0] bi3;
  assign size = width_bits >= 4'ha ? 10'h000 : 10'h001 << width_bits;
  assign add_1045 = integer_s + 14'h0001;
  assign size__1 = height_bits >= 4'ha ? 10'h000 : 10'h001 << height_bits;
  assign concat_1049 = {4'h0, size};
  assign shll_1050 = width_bits >= 4'ha ? 10'h000 : 10'h3ff << width_bits;
  assign add_1054 = integer_t + 14'h0001;
  assign concat_1055 = {4'h0, size__1};
  assign shll_1056 = height_bits >= 4'ha ? 10'h000 : 10'h3ff << height_bits;
  assign maximum = ~shll_1050;
  assign maximum__1 = ~shll_1056;
  assign wrapped = ~(~point_s[9:0] | shll_1050);
  assign clamped = (point_s >= concat_1049 ? maximum : point_s[9:0]) & {10{~point_s[13]}};
  assign wrapped__2 = ~(~integer_s[9:0] | shll_1050);
  assign clamped__2 = (integer_s >= concat_1049 ? maximum : integer_s[9:0]) & {10{~integer_s[13]}};
  assign wrapped__4 = ~(~add_1045[9:0] | shll_1050);
  assign clamped__4 = (add_1045 >= concat_1049 ? maximum : add_1045[9:0]) & {10{~add_1045[13]}};
  assign wrapped__1 = ~(~point_t[9:0] | shll_1056);
  assign clamped__1 = (point_t >= concat_1055 ? maximum__1 : point_t[9:0]) & {10{~point_t[13]}};
  assign point_x = clamp_s ? clamped : wrapped;
  assign wrapped__3 = ~(~integer_t[9:0] | shll_1056);
  assign clamped__3 = (integer_t >= concat_1055 ? maximum__1 : integer_t[9:0]) & {10{~integer_t[13]}};
  assign x0 = clamp_s ? clamped__2 : wrapped__2;
  assign x1 = clamp_s ? clamped__4 : wrapped__4;
  assign wrapped__5 = ~(~add_1054[9:0] | shll_1056);
  assign clamped__5 = (add_1054 >= concat_1055 ? maximum__1 : add_1054[9:0]) & {10{~add_1054[13]}};
  assign point_y = clamp_t ? clamped__1 : wrapped__1;
  assign y0 = clamp_t ? clamped__3 : wrapped__3;
  assign y1 = clamp_t ? clamped__5 : wrapped__5;
  assign column_squeezed_const_msb_bits = 11'h000;
  assign column_squeezed = is_16_bit ? {point_x, 1'h0} : {1'h0, point_x};
  assign column__2_squeezed = is_16_bit ? {x0, 1'h0} : {1'h0, x0};
  assign column__4_squeezed = is_16_bit ? {x1, 1'h0} : {1'h0, x1};
  assign offset__4 = level[3:2];
  assign offset__3 = level[1:0] | point_y[9:8];
  assign offset__2 = point_y[7:0];
  assign point_x_portion_2_width_6 = point_x[7:2];
  assign column = {column_squeezed_const_msb_bits, column_squeezed};
  assign row_shift = {1'h0, lod_shift} + {4'h0, is_16_bit};
  assign offset__5 = level[1:0] | y0[9:8];
  assign offset__6 = y0[7:0];
  assign x0_portion_2_width_6 = x0[7:2];
  assign column__2 = {column_squeezed_const_msb_bits, column__2_squeezed};
  assign x1_portion_2_width_6 = x1[7:2];
  assign column__4 = {column_squeezed_const_msb_bits, column__4_squeezed};
  assign offset__11 = level[1:0] | y1[9:8];
  assign offset__12 = y1[7:0];
  assign linear_base__1 = multibase & level == 4'h1 ? base1[22:0] : (multibase & level == 4'h2 ? base2[22:0] : (multibase & level > 4'h2 ? base38[22:0] : base0[22:0]));
  assign add_1184 = lod_base + column;
  assign row = row_shift >= 5'h16 ? 22'h00_0000 : {12'h000, point_y} << row_shift;
  assign add_1189 = lod_base + column__2;
  assign row__1 = row_shift >= 5'h16 ? 22'h00_0000 : {12'h000, y0} << row_shift;
  assign add_1194 = lod_base + column__4;
  assign row__3 = row_shift >= 5'h16 ? 22'h00_0000 : {12'h000, y1} << row_shift;
  assign add_1198 = linear_base__1 + {5'h00, offset__4, offset__3, offset__2, point_x_portion_2_width_6};
  assign column__1_squeezed_portion_0_width_3 = is_16_bit ? {point_x[1:0], 1'h0} : {1'h0, point_x[1:0]};
  assign add_1200 = add_1184 + row;
  assign add_1201 = linear_base__1 + {5'h00, offset__4, offset__5, offset__6, x0_portion_2_width_6};
  assign column__3_squeezed_portion_0_width_3 = is_16_bit ? {x0[1:0], 1'h0} : {1'h0, x0[1:0]};
  assign add_1203 = add_1189 + row__1;
  assign add_1204 = linear_base__1 + {5'h00, offset__4, offset__5, offset__6, x1_portion_2_width_6};
  assign column__5_squeezed_portion_0_width_3 = is_16_bit ? {x1[1:0], 1'h0} : {1'h0, x1[1:0]};
  assign add_1206 = add_1194 + row__1;
  assign add_1207 = linear_base__1 + {5'h00, offset__4, offset__11, offset__12, x0_portion_2_width_6};
  assign add_1208 = add_1189 + row__3;
  assign add_1209 = linear_base__1 + {5'h00, offset__4, offset__11, offset__12, x1_portion_2_width_6};
  assign add_1210 = add_1194 + row__3;
  assign point = packed_layout ? {4'h0, add_1200} : {add_1198, column__1_squeezed_portion_0_width_3};
  assign bi0 = packed_layout ? {4'h0, add_1203} : {add_1201, column__3_squeezed_portion_0_width_3};
  assign bi1 = packed_layout ? {4'h0, add_1206} : {add_1204, column__5_squeezed_portion_0_width_3};
  assign bi2 = packed_layout ? {4'h0, add_1208} : {add_1207, column__3_squeezed_portion_0_width_3};
  assign bi3 = packed_layout ? {4'h0, add_1210} : {add_1209, column__5_squeezed_portion_0_width_3};
  assign out = {point, bi0, bi1, bi2, bi3};
endmodule
