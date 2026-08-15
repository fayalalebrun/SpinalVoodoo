pub fn float32_to_fixed(value: u32, fraction_bits: u6) -> s50 {
    let sign = value[31+:u1];
    let exponent = value[23+:u8];
    let fraction = value[0+:u23];
    let normal = exponent != u8:0;
    let mantissa = if normal { u24:0x800000 | (fraction as u24) } else { fraction as u24 };
    let unbiased = if normal { (exponent as s10) - s10:127 } else { s10:-126 };
    let shift = unbiased + (fraction_bits as s10) - s10:23;
    let magnitude = if shift >= s10:0 {
        (mantissa as u64) << (shift as u6)
    } else {
        (mantissa as u64) >> ((-shift) as u6)
    };
    if sign == u1:1 { -(magnitude as s50) } else { magnitude as s50 }
}


fn negate_u32(value: u32) -> u32 {
    let byte0 = -value[0+:u8];
    let byte1 = !value[8+:u8] + ((value[0+:u8] == u8:0) as u8);
    let byte2 = !value[16+:u8] + ((value[0+:u16] == u16:0) as u8);
    let byte3 = !value[24+:u8] + ((value[0+:u24] == u24:0) as u8);
    byte3 ++ byte2 ++ byte1 ++ byte0
}

pub fn float32_to_fixed32_fast(value: u32, fraction_bits: u6) -> s32 {
    let sign = value[31+:u1];
    let exponent = value[23+:u8];
    let fraction = value[0+:u23];
    let normal = exponent != u8:0;
    let mantissa = if normal { u24:0x800000 | (fraction as u24) } else { fraction as u24 };
    let threshold = (if normal { u9:150 } else { u9:149 }) - (fraction_bits as u9);
    let scaled_exponent = if normal { exponent as u9 } else { u9:0 };
    let left = scaled_exponent >= threshold;
    let shift = if left {
        scaled_exponent - threshold
    } else {
        threshold - scaled_exponent
    };
    let magnitude = if left {
        (mantissa as u32) << (shift as u6)
    } else {
        (mantissa as u32) >> (shift as u6)
    };
    if sign == u1:1 { negate_u32(magnitude) as s32 } else { magnitude as s32 }
}

pub fn alias(address: u10) -> (u10, u6, u1) {
    match address {
        u10:0x088 | u10:0x08c | u10:0x090 | u10:0x094 | u10:0x098 | u10:0x09c => (
            address - u10:0x080, u6:4, u1:1
        ),
        u10:0x0a0 |
        u10:0x0a4 |
        u10:0x0a8 |
        u10:0x0ac |
        u10:0x0b0 |
        u10:0x0c0 |
        u10:0x0c4 |
        u10:0x0c8 |
        u10:0x0cc |
        u10:0x0d0 |
        u10:0x0e0 |
        u10:0x0e4 |
        u10:0x0e8 |
        u10:0x0ec |
        u10:0x0f0 => (address - u10:0x080, u6:12, u1:1),
        u10:0x0b4 | u10:0x0b8 | u10:0x0d4 | u10:0x0d8 | u10:0x0f4 | u10:0x0f8 => (
            address - u10:0x080, u6:18, u1:1
        ),
        u10:0x0bc | u10:0x0dc | u10:0x0fc => (address - u10:0x080, u6:30, u1:1),
        _ => (address, u6:0, u1:0),
    }
}

#[test]
fn converts_positive_and_negative_values() {
    assert_eq(float32_to_fixed(u32:0x3fc00000, u6:4), s50:24);
    assert_eq(float32_to_fixed(u32:0xbfc00000, u6:4), s50:-24);
    assert_eq(float32_to_fixed(u32:0x3f800000, u6:30), s50:0x40000000);
    assert_eq(float32_to_fixed(u32:0xbf000000, u6:12), s50:-2048);
    assert_eq(
        float32_to_fixed32_fast(u32:0x00000001, u6:30) as u32,
        float32_to_fixed(u32:0x00000001, u6:30) as u32);
    assert_eq(
        float32_to_fixed32_fast(u32:0x00800000, u6:18) as u32,
        float32_to_fixed(u32:0x00800000, u6:18) as u32);
    assert_eq(
        float32_to_fixed32_fast(u32:0x3f800000, u6:30) as u32,
        float32_to_fixed(u32:0x3f800000, u6:30) as u32);
    assert_eq(
        float32_to_fixed32_fast(u32:0xbf000000, u6:12) as u32,
        float32_to_fixed(u32:0xbf000000, u6:12) as u32);
    assert_eq(
        float32_to_fixed32_fast(u32:0x7f7fffff, u6:4) as u32,
        float32_to_fixed(u32:0x7f7fffff, u6:4) as u32);
    assert_eq(
        float32_to_fixed32_fast(u32:0xff7fffff, u6:4) as u32,
        float32_to_fixed(u32:0xff7fffff, u6:4) as u32);
}

#[test]
fn negates_u32_in_parallel_bytes() {
    assert_eq(negate_u32(u32:0), u32:0);
    assert_eq(negate_u32(u32:1), u32:0xffffffff);
    assert_eq(negate_u32(u32:0x00000100), u32:0xffffff00);
    assert_eq(negate_u32(u32:0x01000000), u32:0xff000000);
    assert_eq(negate_u32(u32:0x80000000), u32:0x80000000);
    assert_eq(negate_u32(u32:0xffffffff), u32:1);
}

#[test]
fn aliases_geometry_registers() {
    assert_eq(alias(u10:0x088), (u10:0x008, u6:4, u1:1));
    assert_eq(alias(u10:0x0b4), (u10:0x034, u6:18, u1:1));
    assert_eq(alias(u10:0x0dc), (u10:0x05c, u6:30, u1:1));
    assert_eq(alias(u10:0x200), (u10:0x200, u6:0, u1:0));
}
