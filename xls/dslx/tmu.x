fn abs_s48(value: s48) -> u48 { if value < s48:0 { (-value) as u48 } else { value as u48 } }

pub fn tmu_recip_meta(w: s48, clamp_w: u1) -> u25 {
    let magnitude = abs_s48(w) as u64;
    let leading = clz(magnitude) as u7;
    let msb = u7:63 - leading;
    let normalized = magnitude << leading;
    let index = normalized[55+:u8];
    let fraction = normalized[47+:u8];
    let valid = (w >= s48:0) && magnitude != u64:0;
    let clamp = clamp_w && w < s48:0;
    index ++ fraction ++ msb ++ (valid as u1) ++ (clamp as u1)
}

pub fn tmu_recip
    (base: u17, difference: u17, fraction: u8, log_fraction: u8, msb: u7, valid: u1) -> u33 {
    let product = (difference as u25) * (fraction as u25);
    let interpolated = base - ((product >> u8:8) as u17);
    let exponent = s8:27 - (msb as s8);
    let adjust = if valid {
        (((exponent as s16) << u8:8) - (log_fraction as s16) - s16:1) as u16
    } else {
        u16:0
    };
    (if valid { interpolated } else { u17:0 }) ++ adjust
}

pub fn tmu_perspective_coords(s: s48, t: s48, interpolated: u17, msb: u7, perspective: u1) -> u36 {
    let reciprocal = interpolated as sN[66];
    let product_s = (s as sN[66]) * reciprocal;
    let product_t = (t as sN[66]) * reciprocal;
    let bias_shift = if msb == u7:0 { u7:127 } else { msb - u7:1 };
    let bias = sN[66]:1 << bias_shift;
    let tex_s = if perspective { ((product_s + bias) >> msb) as s18 } else { (s >> u8:14) as s18 };
    let tex_t = if perspective { ((product_t + bias) >> msb) as s18 } else { (t >> u8:14) as s18 };
    (tex_s as u18) ++ (tex_t as u18)
}

pub fn tmu_gradient(ds_dx: s32, dt_dx: s32, ds_dy: s32, dt_dy: s32) -> u64 {
    let sx = (ds_dx as s64) * (ds_dx as s64);
    let tx = (dt_dx as s64) * (dt_dx as s64);
    let sy = (ds_dy as s64) * (ds_dy as s64);
    let ty = (dt_dy as s64) * (dt_dy as s64);
    let dx = (sx as u64) + (tx as u64);
    let dy = (sy as u64) + (ty as u64);
    if dx > dy { dx } else { dy }
}

pub fn tmu_lod_index(temp_lod: u64) -> u15 {
    let high = temp_lod[32+:u32];
    let low = temp_lod[0+:u32];
    let use_high = high != u32:0;
    let leading = clz(if use_high { high } else { low });
    let msb = if use_high { u7:63 - (leading as u7) } else { u7:31 - (leading as u7) };
    let shift = if msb >= u7:8 { (msb - u7:8) as u6 } else { u6:0 };
    let index = (temp_lod >> shift)[0+:u8];
    msb ++ index
}

pub fn tmu_base_lod(temp_lod: u64, msb: u7, log_fraction: u8) -> s16 {
    if temp_lod == u64:0 {
        s16:0
    } else {
        let raw = (((msb as s16) << u8:8) + (log_fraction as s16) - s16:9216) as s16;
        raw >> u8:2
    }
}

pub fn tmu_lod_select(base_lod: s16, perspective_adjust: s16, t_lod: u27) -> u12 {
    let lod_min = (t_lod[0+:u6] as u12) << u8:6;
    let lod_max_raw = (t_lod[6+:u6] as u12) << u8:6;
    let lod_max = if lod_max_raw > u12:0x800 { u12:0x800 } else { lod_max_raw };
    let bias = (t_lod[12+:u6] as s6) as s16 << u8:6;
    let lod = (base_lod + perspective_adjust + bias) as s16;
    let clamped = if lod < (lod_min as s16) {
        lod_min as s16
    } else if lod > (lod_max as s16) {
        lod_max as s16
    } else {
        lod
    };
    let integer = clamped >> u8:8;
    let level = if integer < s16:0 {
        u4:0
    } else if integer > s16:8 {
        u4:8
    } else {
        integer as u4
    };
    let dimension = u4:8 - level;
    let aspect = t_lod[21+:u2] as u4;
    let narrow = if dimension >= aspect { dimension - aspect } else { u4:0 };
    let s_wider = t_lod[20+:u1];
    let width = if s_wider {
        dimension
    } else if aspect != u4:0 {
        narrow
    } else {
        dimension
    };
    let height = if s_wider { narrow } else { dimension };
    level ++ width ++ height
}

pub fn tmu_coordinates(tex_s: s18, tex_t: s18, level: u4, clamp_zero: u1) -> u64 {
    let point_shift = u5:4 + (level as u5);
    let half_shift = u5:3 + (level as u5);
    let point_s = (tex_s >> point_shift) as s14;
    let point_t = (tex_t >> point_shift) as s14;
    let adjusted_s = (tex_s - (s18:1 << half_shift)) as s18;
    let adjusted_t = (tex_t - (s18:1 << half_shift)) as s18;
    let scaled_s = (adjusted_s >> level) as s18;
    let scaled_t = (adjusted_t >> level) as s18;
    let integer_s = (scaled_s >> u8:4) as s14;
    let integer_t = (scaled_t >> u8:4) as s14;
    let fraction_s = scaled_s as u4;
    let fraction_t = scaled_t as u4;
    let final_point_s = if clamp_zero { s14:0 } else { point_s };
    let final_point_t = if clamp_zero { s14:0 } else { point_t };
    let final_integer_s = if clamp_zero { s14:0 } else { integer_s };
    let final_integer_t = if clamp_zero { s14:0 } else { integer_t };
    let final_fraction_s = if clamp_zero { u4:0 } else { fraction_s };
    let final_fraction_t = if clamp_zero { u4:0 } else { fraction_t };
    (final_point_s as u14) ++ (final_point_t as u14) ++ (final_integer_s as u14) ++
    (final_integer_t as u14) ++ final_fraction_s ++ final_fraction_t
}

fn wrap_or_clamp(coord: s14, size_bits: u4, clamp_enable: u1) -> u10 {
    let size = u10:1 << size_bits;
    let maximum = size - u10:1;
    let wrapped = (coord as u10) & maximum;
    let clamped = if coord < s14:0 {
        u10:0
    } else if (coord as u14) >= (size as u14) {
        maximum
    } else {
        coord as u10
    };
    if clamp_enable { clamped } else { wrapped }
}

fn texel_address
    (x: u10, y: u10, is_16_bit: u1, packed_layout: u1, lod_base: u22, lod_shift: u4,
     linear_base: u24, level: u4) -> u26 {
    if packed_layout {
        let column = if is_16_bit { (x as u22) << u8:1 } else { x as u22 };
        let row_shift = (lod_shift as u5) + (is_16_bit as u5);
        let row = (y as u22) << row_shift;
        (lod_base + column + row) as u26
    } else {
        let x8 = x as u8;
        let column = if is_16_bit {
            (x as u9) << u8:1
        } else {
            ((((x8 & u8:0xfc) as u9) << u8:1) | ((x8 & u8:3) as u9)) as u9
        };
        let offset = (level as u21) << u8:17 | (y as u21) << u8:9 | (column as u21);
        ((linear_base as u26) << u8:3) + (offset as u26)
    }
}

pub fn tmu_addresses
    (point_s: s14, point_t: s14, integer_s: s14, integer_t: s14, width_bits: u4, height_bits: u4,
     clamp_s: u1, clamp_t: u1, is_16_bit: u1, packed_layout: u1, lod_base: u22, lod_shift: u4,
     base0: u24, base1: u24, base2: u24, base38: u24, multibase: u1, level: u4) -> uN[130] {
    let linear_base = if multibase && level == u4:1 {
        base1
    } else if multibase && level == u4:2 {
        base2
    } else if multibase && level >= u4:3 {
        base38
    } else {
        base0
    };
    let point_x = wrap_or_clamp(point_s, width_bits, clamp_s);
    let point_y = wrap_or_clamp(point_t, height_bits, clamp_t);
    let x0 = wrap_or_clamp(integer_s, width_bits, clamp_s);
    let x1 = wrap_or_clamp((integer_s + s14:1) as s14, width_bits, clamp_s);
    let y0 = wrap_or_clamp(integer_t, height_bits, clamp_t);
    let y1 = wrap_or_clamp((integer_t + s14:1) as s14, height_bits, clamp_t);
    let point = texel_address(
        point_x, point_y, is_16_bit, packed_layout, lod_base, lod_shift, linear_base, level);
    let bi0 =
        texel_address(x0, y0, is_16_bit, packed_layout, lod_base, lod_shift, linear_base, level);
    let bi1 =
        texel_address(x1, y0, is_16_bit, packed_layout, lod_base, lod_shift, linear_base, level);
    let bi2 =
        texel_address(x0, y1, is_16_bit, packed_layout, lod_base, lod_shift, linear_base, level);
    let bi3 =
        texel_address(x1, y1, is_16_bit, packed_layout, lod_base, lod_shift, linear_base, level);
    point ++ bi0 ++ bi1 ++ bi2 ++ bi3
}

pub fn texture_write_address
    (pci_address: u23, is_16_bit: u1, sequential_8_bit: u1, packed_layout: u1, lod_base: u22,
     lod_shift: u4, base0: u24, base1: u24, base2: u24, base38: u24, multibase: u1) -> u27 {
    let lod = pci_address[17+:u4];
    let linear_base = if multibase && lod == u4:1 {
        base1
    } else if multibase && lod == u4:2 {
        base2
    } else if multibase && lod >= u4:3 {
        base38
    } else {
        base0
    };
    let t = pci_address[9+:u8];
    let s = if is_16_bit {
        ((pci_address >> u8:1) as u8) & u8:0xfe
    } else if sequential_8_bit {
        (pci_address as u8) & u8:0xfc
    } else {
        ((pci_address >> u8:1) as u8) & u8:0xfc
    };
    let packed_column = if is_16_bit { (s as u22) << u8:1 } else { s as u22 };
    let packed_row = (t as u22) << ((lod_shift as u5) + (is_16_bit as u5));
    let packed_address = (lod_base + packed_column + packed_row) as u26;
    let linear_address = (((linear_base[0+:u19] as u26) << u8:3) + (pci_address as u26)) as u26;
    let address = if packed_layout { packed_address } else { linear_address };
    let valid = pci_address[21+:u2] == u2:0 && (!packed_layout || lod <= u4:8);
    (valid as u1) ++ address
}

fn packed_value22(values: uN[198], lod: u4) -> u22 {
    (values >> (((u4:8 - lod) as u8) * u8:22)) as u22
}

fn packed_value4(values: u36, lod: u4) -> u4 { (values >> (((u4:8 - lod) as u6) * u6:4)) as u4 }

fn corrected_end(bases: uN[198], ends: uN[198], is_16_bit: u1, lod: u4) -> u22 {
    let base = packed_value22(bases, lod);
    let raw = packed_value22(ends, lod);
    let fallback = if lod < u4:8 {
        packed_value22(bases, lod + u4:1)
    } else {
        base + if is_16_bit { u22:2 } else { u22:1 }
    };
    if raw > base { raw } else { fallback }
}

fn select_location
    (addr: u22, lod: u4, bases: uN[198], ends: uN[198], shifts: u36, is_16_bit: u1,
     selected: (u1, u22, u4)) -> (u1, u22, u4) {
    let base = packed_value22(bases, lod);
    let end = corrected_end(bases, ends, is_16_bit, lod);
    if addr >= base && addr < end { (u1:1, base, packed_value4(shifts, lod)) } else { selected }
}

fn packed_location
    (addr: u22, is_16_bit: u1, lod_base: u22, lod_shift: u4, bases: uN[198], ends: uN[198],
     shifts: u36) -> u4 {
    let initial = (u1:0, lod_base, lod_shift);
    let sel0 = select_location(addr, u4:0, bases, ends, shifts, is_16_bit, initial);
    let sel1 = select_location(addr, u4:1, bases, ends, shifts, is_16_bit, sel0);
    let sel2 = select_location(addr, u4:2, bases, ends, shifts, is_16_bit, sel1);
    let sel3 = select_location(addr, u4:3, bases, ends, shifts, is_16_bit, sel2);
    let sel4 = select_location(addr, u4:4, bases, ends, shifts, is_16_bit, sel3);
    let sel5 = select_location(addr, u4:5, bases, ends, shifts, is_16_bit, sel4);
    let sel6 = select_location(addr, u4:6, bases, ends, shifts, is_16_bit, sel5);
    let sel7 = select_location(addr, u4:7, bases, ends, shifts, is_16_bit, sel6);
    let selected = select_location(addr, u4:8, bases, ends, shifts, is_16_bit, sel7);
    let offset = (addr as u10) - (selected.1 as u10);
    let bank_msb = if is_16_bit {
        offset[selected.2+:u1] | offset[(selected.2 + u4:1) as u4+:u1]
    } else {
        offset[selected.2+:u1]
    };
    let bank_lsb = if is_16_bit { offset[0+:u1] | offset[1+:u1] } else { offset[0+:u1] };
    let pair = !is_16_bit && offset[1+:u1];
    selected.0 ++ bank_msb ++ bank_lsb ++ pair
}

fn startup_decode
    (line_base: u22, is_16_bit: u1, lod_base: u22, lod_shift: u4, bases: uN[198], ends: uN[198],
     shifts: u36) -> u16 {
    packed_location(line_base, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    packed_location(line_base + u22:1, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    packed_location(line_base + u22:2, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    packed_location(line_base + u22:3, is_16_bit, lod_base, lod_shift, bases, ends, shifts)
}

pub fn tmu_packed_decode
    (line0: u22, line1: u22, line2: u22, line3: u22, addr0: u22, addr1: u22, addr2: u22, addr3: u22,
     is_16_bit: u1, lod_base: u22, lod_shift: u4, bases: uN[198], ends: uN[198], shifts: u36)
    -> uN[80] {
    startup_decode(line0, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    startup_decode(line1, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    startup_decode(line2, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    startup_decode(line3, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    packed_location(addr0, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    packed_location(addr1, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    packed_location(addr2, is_16_bit, lod_base, lod_shift, bases, ends, shifts) ++
    packed_location(addr3, is_16_bit, lod_base, lod_shift, bases, ends, shifts)
}

#[test]
fn recip_meta_test() {
    let result = tmu_recip_meta(s48:0x40000000, u1:0);
    assert_eq(result[2+:u7], u7:30);
    assert_eq(result[1+:u1], u1:1);
}

#[test]
fn gradient_test() { assert_eq(tmu_gradient(s32:3, s32:4, s32:5, s32:12), u64:169); }

#[test]
fn wrap_address_test() {
    let result = tmu_addresses(
        s14:2, s14:3, s14:2, s14:3, u4:8, u4:8, u1:0, u1:0, u1:1, u1:1, u22:0x100, u4:8, u24:0,
        u24:0, u24:0, u24:0, u1:0, u4:0);
    assert_eq(result[104+:u26], u26:0x704);
}

#[test]
fn lod_and_coordinate_test() {
    assert_eq(tmu_lod_select(s16:0x300, s16:0, u27:0x500800), u12:0x353);
    let coordinates = tmu_coordinates(s18:160, s18:176, u4:0, u1:0);
    assert_eq(coordinates[50+:u14] as s14, s14:10);
    assert_eq(coordinates[22+:u14] as s14, s14:9);
    assert_eq(coordinates[4+:u4], u4:8);
    assert_eq(
        tmu_perspective_coords(s48:0x28000, s48:-0x14000, u17:0, u7:0, u1:0),
        (u18:10 ++ (s18:-5 as u18)));
}

#[test]
fn texture_write_address_test() {
    let packed = texture_write_address(
        u23:0x604, u1:1, u1:0, u1:1, u22:0x100, u4:8, u24:0, u24:0, u24:0, u24:0, u1:0);
    assert_eq(packed[26+:u1], u1:1);
    assert_eq(packed[0+:u26], u26:0x704);
    let linear = texture_write_address(
        u23:4, u1:1, u1:0, u1:0, u22:0, u4:0, u24:0x20, u24:0, u24:0, u24:0, u1:0);
    assert_eq(linear[0+:u26], u26:0x104);
}

#[test]
fn packed_decode_test() {
    let bases = u22:0x100 ++ u22:0x200 ++ u22:0x300 ++ u22:0x400 ++ u22:0x500 ++ u22:0x600 ++
                u22:0x700 ++ u22:0x800 ++ u22:0x900;
    let ends = u22:0x200 ++ u22:0x300 ++ u22:0x400 ++ u22:0x500 ++ u22:0x600 ++ u22:0x700 ++
               u22:0x800 ++ u22:0x900 ++ u22:0xa00;
    let shifts = u4:8 ++ u4:7 ++ u4:6 ++ u4:5 ++ u4:4 ++ u4:3 ++ u4:2 ++ u4:1 ++ u4:0;
    let decoded = tmu_packed_decode(
        u22:0x100, u22:0x200, u22:0x300, u22:0x400, u22:0x104, u22:0x106, u22:0x204, u22:0x206,
        u1:1, u22:0x100, u4:8, bases, ends, shifts);
    assert_eq(decoded[12+:u4], u4:8);
    assert_eq(decoded[8+:u4], u4:0xa);
    let decoded8 = tmu_packed_decode(
        u22:0x100, u22:0x200, u22:0x300, u22:0x400, u22:0x102, u22:0x100, u22:0x200, u22:0x300,
        u1:0, u22:0x100, u4:8, bases, ends, shifts);
    assert_eq(decoded8[12+:u4], u4:9);
}
