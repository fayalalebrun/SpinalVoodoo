module VoodooXlsTextureWriteAddress(
  input wire [22:0] pci_address,
  input wire is_16_bit,
  input wire sequential_8_bit,
  input wire packed_layout,
  input wire [21:0] lod_base,
  input wire [3:0] lod_shift,
  input wire [23:0] base0,
  input wire [23:0] base1,
  input wire [23:0] base2,
  input wire [23:0] base38,
  input wire multibase,
  output wire [26:0] out
);
  wire [3:0] lod;
  wire [12:0] packed_column_squeezed_const_msb_bits;
  wire [6:0] packed_column_squeezed;
  wire [7:0] t;
  wire [1:0] address_squeezed_const_msb_bits;
  wire [18:0] linear_base__1;
  wire [19:0] add_352;
  wire [4:0] add_355;
  wire [21:0] packed_row;
  wire [20:0] add_365;
  wire [21:0] add_367;
  wire [23:0] linear_address_squeezed;
  wire [23:0] packed_address_squeezed;
  wire valid;
  wire [23:0] address_squeezed;
  assign lod = pci_address[20:17];
  assign packed_column_squeezed_const_msb_bits = 13'h0000;
  assign packed_column_squeezed = is_16_bit ? pci_address[8:2] : {1'h0, sequential_8_bit ? pci_address[7:2] : pci_address[8:3]};
  assign t = pci_address[16:9];
  assign address_squeezed_const_msb_bits = 2'h0;
  assign linear_base__1 = multibase & lod == 4'h1 ? base1[18:0] : (multibase & lod == 4'h2 ? base2[18:0] : (multibase & lod > 4'h2 ? base38[18:0] : base0[18:0]));
  assign add_352 = lod_base[21:2] + {packed_column_squeezed_const_msb_bits, packed_column_squeezed};
  assign add_355 = {1'h0, lod_shift} + {4'h0, is_16_bit};
  assign packed_row = add_355 >= 5'h16 ? 22'h00_0000 : {14'h0000, t} << add_355;
  assign add_365 = {address_squeezed_const_msb_bits, linear_base__1} + {1'h0, pci_address[22:3]};
  assign add_367 = {add_352, lod_base[1:0]} + packed_row;
  assign linear_address_squeezed = {add_365, pci_address[2:0]};
  assign packed_address_squeezed = {address_squeezed_const_msb_bits, add_367};
  assign valid = ~(pci_address[21] | pci_address[22]) & ~(packed_layout & lod > 4'h8);
  assign address_squeezed = packed_layout ? packed_address_squeezed : linear_address_squeezed;
  assign out = {valid, address_squeezed_const_msb_bits, address_squeezed};
endmodule
