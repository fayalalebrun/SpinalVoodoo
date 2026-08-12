fn clamp_s18_to_u8(value: s18) -> u8 {
    if value < s18:0 {
        u8:0
    } else if value > s18:255 {
        u8:255
    } else {
        value as u8
    }
}

fn min_u8(a: u8, b: u8) -> u8 { if a < b { a } else { b } }

fn rgb_r(rgb: u24) -> u8 { rgb[16+:u8] }

fn rgb_g(rgb: u24) -> u8 { rgb[8+:u8] }

fn rgb_b(rgb: u24) -> u8 { rgb[0+:u8] }

fn pack_rgb(r: u8, g: u8, b: u8) -> u24 { r ++ g ++ b }

fn mul_u16_by_u4(value: u16, coefficient: u4) -> u20 {
    let wide = value as u20;
    let p0 = if coefficient[0+:u1] { wide } else { u20:0 };
    let p1 = if coefficient[1+:u1] { wide << u8:1 } else { u20:0 };
    let p2 = if coefficient[2+:u1] { wide << u8:2 } else { u20:0 };
    let p3 = if coefficient[3+:u1] { wide << u8:3 } else { u20:0 };
    p0 + p1 + p2 + p3
}

pub fn framebuffer_address(base: u26, x: u16, y: u16, stride: u11) -> u26 {
    let tiles_x = (stride >> u11:6) as u4;
    let row_tiles = mul_u16_by_u4(y, tiles_x) as u26;
    base + (row_tiles << u26:7) + ((x as u26) << u26:1)
}

pub fn write_pack(base: u26, x: u16, y: u16, stride: u11, data: u16) -> u62 {
    let plane_address = framebuffer_address(base, x, y, stride);
    let address = plane_address & !u26:3;
    let high = (plane_address & u26:2) != u26:0;
    let packed_data = if high { (data as u32) << u32:16 } else { data as u32 };
    let mask = if high { u4:0xc } else { u4:0x3 };
    ((address as u62) << u62:36) | ((packed_data as u62) << u62:4) | (mask as u62)
}

pub fn depth_compare(comparison: u3, new_depth: u16, old_depth: u16) -> u1 {
    match comparison {
        u3:0 => u1:0,
        u3:1 => new_depth < old_depth,
        u3:2 => new_depth == old_depth,
        u3:3 => new_depth <= old_depth,
        u3:4 => new_depth > old_depth,
        u3:5 => new_depth != old_depth,
        u3:6 => new_depth >= old_depth,
        _ => u1:1,
    }
}

fn div_by_255(value: u16) -> u8 {
    let wide = value as u17;
    ((wide + u17:1 + (wide >> u17:8)) >> u17:8) as u8
}

fn blend_factor
    (selector: u4, src_alpha: u8, dest_alpha: u8, color: u8, before_fog: u8, is_src: u1) -> u8 {
    match selector {
        u4:0 => u8:0,
        u4:1 => src_alpha,
        u4:2 => color,
        u4:3 => dest_alpha,
        u4:4 => u8:255,
        u4:5 => u8:255 - src_alpha,
        u4:6 => u8:255 - color,
        u4:7 => u8:255 - dest_alpha,
        u4:15 => if is_src { min_u8(src_alpha, u8:255 - dest_alpha) } else { before_fog },
        _ => if is_src { u8:255 } else { u8:0 },
    }
}

fn blend_channel
    (src: u8, dest: u8, before_fog: u8, src_alpha: u8, src_selector: u4, dest_selector: u4) -> u8 {
    let dest_alpha = u8:255;
    let src_scale = blend_factor(src_selector, src_alpha, dest_alpha, dest, before_fog, u1:1);
    let dest_scale = blend_factor(dest_selector, src_alpha, dest_alpha, src, before_fog, u1:0);
    let src_value = div_by_255((src as u16) * (src_scale as u16));
    let dest_value = div_by_255((dest as u16) * (dest_scale as u16));
    let sum = (src_value as u9) + (dest_value as u9);
    if sum > u9:255 { u8:255 } else { sum as u8 }
}

pub fn alpha_blend
    (src_rgb: u24, dest_rgb: u24, before_fog_rgb: u24, src_alpha: u8, src_selector: u4,
     dest_selector: u4, enable: u1) -> u24 {
    if !enable {
        src_rgb
    } else {
        pack_rgb(
            blend_channel(
                rgb_r(src_rgb), rgb_r(dest_rgb), rgb_r(before_fog_rgb), src_alpha, src_selector,
                dest_selector),
            blend_channel(
                rgb_g(src_rgb), rgb_g(dest_rgb), rgb_g(before_fog_rgb), src_alpha, src_selector,
                dest_selector),
            blend_channel(
                rgb_b(src_rgb), rgb_b(dest_rgb), rgb_b(before_fog_rgb), src_alpha, src_selector,
                dest_selector))
    }
}

fn leading_zeroes_16(value: u16) -> u5 { (clz(value) as u5) }

pub fn fog_w_depth(raw_w: s32) -> u16 {
    let raw_bits = raw_w as u32;
    let upper = raw_bits[14+:u16];
    let exponent = leading_zeroes_16(upper);
    let shifted = raw_bits << u32:2;
    let inverted = !shifted;
    let shift_amount = u5:19 - exponent;
    let mantissa = ((inverted >> shift_amount) & u32:0xfff) as u12;
    let depth_raw = (((exponent as u17) << u17:12) | (mantissa as u17)) + u17:1;
    let clamped = if depth_raw > u17:0xffff { u16:0xffff } else { depth_raw as u16 };
    if (raw_w < s32:0) || ((raw_bits & u32:0xc0000000) != u32:0) {
        u16:0
    } else if upper == u16:0 {
        u16:0xf001
    } else {
        clamped
    }
}

pub fn fog_table_factor(w_depth: u16, table_entry: u16) -> u8 {
    let base = table_entry[0+:u8];
    let delta = table_entry[8+:u8] as s8;
    let blend = w_depth[2+:u8];
    let product = ((delta as s18) * (blend as s18)) >> u8:8;
    clamp_s18_to_u8((base as s18) + product)
}

fn fog_channel(src: u8, fog: u8, factor: u8, add: u1, mult: u1, constant: u1) -> u8 {
    if constant {
        let sum = (src as u9) + (fog as u9);
        if sum > u9:255 { u8:255 } else { sum as u8 }
    } else {
        let base = if add { s18:0 } else { fog as s18 };
        let difference = if mult { base } else { base - (src as s18) };
        let scaled = (difference * (((factor as u9) + u9:1) as s18)) >> u8:8;
        let result = if mult { scaled } else { (src as s18) + scaled };
        clamp_s18_to_u8(result)
    }
}

pub fn fog_blend
    (src_rgb: u24, fog_rgb: u24, factor: u8, enable: u1, add: u1, mult: u1, constant: u1) -> u24 {
    if !enable {
        src_rgb
    } else {
        pack_rgb(
            fog_channel(rgb_r(src_rgb), rgb_r(fog_rgb), factor, add, mult, constant),
            fog_channel(rgb_g(src_rgb), rgb_g(fog_rgb), factor, add, mult, constant),
            fog_channel(rgb_b(src_rgb), rgb_b(fog_rgb), factor, add, mult, constant))
    }
}

fn select_rgb(selector: u2, iterated: u24, texture: u24, color1: u24) -> u24 {
    match selector {
        u2:0 => iterated,
        u2:1 => texture,
        u2:2 => color1,
        _ => u24:0,
    }
}

fn select_alpha(selector: u2, iterated: u8, texture: u8, color1: u8) -> u8 {
    match selector {
        u2:0 => iterated,
        u2:1 => texture,
        u2:2 => color1,
        _ => u8:0,
    }
}

fn select_factor
    (selector: u3, local_color: u8, other_alpha: s10, local_alpha: u8, texture_alpha: u8,
     texture_color: u8) -> u8 {
    match selector {
        u3:0 => u8:0,
        u3:1 => local_color,
        u3:2 => clamp_s18_to_u8(other_alpha as s18),
        u3:3 => local_alpha,
        u3:4 => texture_alpha,
        u3:5 => texture_color,
        _ => u8:0,
    }
}

fn combine_channel
    (other: u8, local: u8, local_alpha: u8, texture_color: u8, texture_alpha: u8, zero_other: u1,
     sub_local: u1, factor_selector: u3, reverse_blend: u1, add_mode: u2, invert: u1) -> u8 {
    let zeroed = if zero_other { s10:0 } else { other as s10 };
    let subtracted = if sub_local { zeroed - (local as s10) } else { zeroed };
    let factor_pre = select_factor(
        factor_selector, local, subtracted, local_alpha, texture_alpha, texture_color);
    let factor_base = if reverse_blend { factor_pre } else { !factor_pre };
    let factor = (factor_base as u9) + u9:1;
    let multiplied = ((subtracted as s19) * (factor as s19)) >> u8:8;
    let added = match add_mode {
        u2:1 => multiplied + (local as s19),
        u2:2 => multiplied + (local_alpha as s19),
        _ => multiplied,
    };
    let clamped = clamp_s18_to_u8(added as s18);
    if invert { !clamped } else { clamped }
}

pub fn color_combine
    (iterated_rgb: u24, iterated_alpha: u8, iterated_z: u8, texture_rgb: u24, texture_alpha: u8,
     color0_rgb: u24, color0_alpha: u8, color1_rgb: u24, color1_alpha: u8, rgb_sel: u2,
     local_sel: u1, local_override: u1, zero_other: u1, sub_local: u1, factor_sel: u3,
     reverse_blend: u1, add_mode: u2, invert_rgb: u1, alpha_sel: u2, alpha_local_sel: u2,
     alpha_zero_other: u1, alpha_sub_local: u1, alpha_factor_sel: u3, alpha_reverse_blend: u1,
     alpha_add_mode: u2, invert_alpha: u1) -> u32 {
    let other_rgb = select_rgb(rgb_sel, iterated_rgb, texture_rgb, color1_rgb);
    let local_rgb = if local_override && ((texture_alpha & u8:0x80) != u8:0) {
        color0_rgb
    } else if local_sel {
        color0_rgb
    } else {
        iterated_rgb
    };
    let other_alpha = select_alpha(alpha_sel, iterated_alpha, texture_alpha, color1_alpha);
    let local_alpha = match alpha_local_sel {
        u2:0 => iterated_alpha,
        u2:1 => color0_alpha,
        _ => iterated_z,
    };
    let alpha_zeroed = if alpha_zero_other { s10:0 } else { other_alpha as s10 };
    let alpha_subtracted =
        if alpha_sub_local { alpha_zeroed - (local_alpha as s10) } else { alpha_zeroed };
    let alpha_factor_pre = select_factor(
        alpha_factor_sel, local_alpha, alpha_subtracted, local_alpha, texture_alpha, texture_alpha);
    let alpha_factor_base = if alpha_reverse_blend { alpha_factor_pre } else { !alpha_factor_pre };
    let alpha_factor = (alpha_factor_base as u9) + u9:1;
    let alpha_multiplied = ((alpha_subtracted as s19) * (alpha_factor as s19)) >> u8:8;
    let alpha_added = if alpha_add_mode == u2:0 {
        alpha_multiplied
    } else {
        alpha_multiplied + (local_alpha as s19)
    };
    let alpha_clamped = clamp_s18_to_u8(alpha_added as s18);
    let alpha_out = if invert_alpha { !alpha_clamped } else { alpha_clamped };
    let red = combine_channel(
        rgb_r(other_rgb), rgb_r(local_rgb), local_alpha, rgb_r(texture_rgb), texture_alpha,
        zero_other, sub_local, factor_sel, reverse_blend, add_mode, invert_rgb);
    let green = combine_channel(
        rgb_g(other_rgb), rgb_g(local_rgb), local_alpha, rgb_g(texture_rgb), texture_alpha,
        zero_other, sub_local, factor_sel, reverse_blend, add_mode, invert_rgb);
    let blue = combine_channel(
        rgb_b(other_rgb), rgb_b(local_rgb), local_alpha, rgb_b(texture_rgb), texture_alpha,
        zero_other, sub_local, factor_sel, reverse_blend, add_mode, invert_rgb);
    red ++ green ++ blue ++ alpha_out
}

pub fn data_swizzle(data: u32, word_swap: u1, byte_swizzle: u1) -> u32 {
    let words = if word_swap { data[0+:u16] ++ data[16+:u16] } else { data };
    if byte_swizzle {
        words[0+:u8] ++ words[8+:u8] ++ words[16+:u8] ++ words[24+:u8]
    } else {
        words
    }
}

pub fn pixel_tests
    (color: u24, chroma: u24, alpha: u8, alpha_reference: u8, alpha_function: u3) -> u2 {
    let chroma_match = color == chroma;
    let alpha_pass = depth_compare(alpha_function, alpha as u16, alpha_reference as u16);
    (chroma_match as u1) ++ alpha_pass
}

fn expand_5(value: u5) -> u8 { ((value as u8) << u8:3) | ((value as u8) >> u8:2) }

fn expand_6(value: u6) -> u8 { ((value as u8) << u8:2) | ((value as u8) >> u8:4) }

fn rgb_565(value: u16) -> u24 {
    pack_rgb(expand_5(value[11+:u5]), expand_6(value[5+:u6]), expand_5(value[0+:u5]))
}

fn rgb_555(value: u16) -> u24 {
    pack_rgb(expand_5(value[10+:u5]), expand_5(value[5+:u5]), expand_5(value[0+:u5]))
}

fn rgba_lanes(data: u32, lanes: u2) -> u32 {
    match lanes {
        u2:0 => data[16+:u8] ++ data[8+:u8] ++ data[0+:u8] ++ data[24+:u8],
        u2:1 => data[0+:u8] ++ data[8+:u8] ++ data[16+:u8] ++ data[24+:u8],
        u2:2 => data[24+:u8] ++ data[16+:u8] ++ data[8+:u8] ++ data[0+:u8],
        _ => data[8+:u8] ++ data[16+:u8] ++ data[24+:u8] ++ data[0+:u8],
    }
}

pub fn lfb_decode
    (data32: u32, data16: u16, format: u4, lanes: u2, default_depth: u16, alpha_planes: u1) -> u50 {
    let swizzled = rgba_lanes(data32, lanes);
    let rgb = match format {
        u4:0 | u4:1 | u4:2 => if format == u4:0 { rgb_565(data16) } else { rgb_555(data16) },
        u4:4 | u4:5 => swizzled[8+:u24],
        u4:12 => rgb_565(data32[0+:u16]),
        u4:13 | u4:14 => rgb_555(data32[0+:u16]),
        _ => u24:0,
    };
    let depth = match format {
        u4:12 | u4:13 | u4:14 => data32[16+:u16],
        u4:15 => data16,
        _ => default_depth,
    };
    let alpha = match format {
        u4:5 => swizzled[0+:u8],
        u4:2 => if data16[15+:u1] { u8:255 } else { u8:0 },
        u4:14 => if data32[15+:u1] { u8:255 } else { u8:0 },
        _ => u8:255,
    };
    let rgb_write = format != u4:15;
    let aux_write = match format {
        u4:0 | u4:1 | u4:2 | u4:4 | u4:5 => alpha_planes,
        u4:12 | u4:13 | u4:14 | u4:15 => u1:1,
        _ => u1:0,
    };
    rgb ++ depth ++ alpha ++ (rgb_write as u1) ++ aux_write
}

#[test]
fn write_pack_test() {
    let packed = write_pack(u26:0x1000000, u16:11, u16:5, u11:640, u16:0xabcd);
    assert_eq((packed >> u62:36) as u26, u26:0x1001914);
    assert_eq(packed[0+:u4], u4:0xc);
}

#[test]
fn depth_test_test() {
    assert_eq(depth_compare(u3:1, u16:3, u16:4), u1:1);
    assert_eq(depth_compare(u3:1, u16:4, u16:3), u1:0);
    assert_eq(depth_compare(u3:7, u16:4, u16:3), u1:1);
}

#[test]
fn fog_w_depth_test() {
    assert_eq(fog_w_depth(s32:-1), u16:0);
    assert_eq(fog_w_depth(s32:0), u16:0xf001);
}

#[test]
fn framebuffer_address_test() {
    assert_eq(mul_u16_by_u4(u16:65535, u4:15), u20:983025);
    assert_eq(mul_u16_by_u4(u16:65535, u4:0), u20:0);
    assert_eq(framebuffer_address(u26:0x1000000, u16:11, u16:5, u11:640), u26:0x1001916);
    assert_eq(framebuffer_address(u26:0, u16:0, u16:0, u11:2047), u26:0);
}

#[test]
fn fog_arithmetic_test() {
    assert_eq(fog_table_factor(u16:0x200, u16:0x1064), u8:108);
    assert_eq(fog_blend(u24:0x0a141e, u24:0x6e7882, u8:127, u1:1, u1:0, u1:0, u1:0), u24:0x3c4650);
    assert_eq(fog_blend(u24:0xf00a14, u24:0x2060f0, u8:0, u1:1, u1:0, u1:0, u1:1), u24:0xff6aff);
}

#[test]
fn alpha_and_color_combine_test() {
    assert_eq(alpha_blend(u24:0x123456, u24:0, u24:0, u8:255, u4:0, u4:0, u1:0), u24:0x123456);
    assert_eq(
        color_combine(
            u24:0x112233, u8:0x44, u8:0, u24:0, u8:0, u24:0, u8:0, u24:0, u8:0, u2:0, u1:0, u1:0,
            u1:0, u1:0, u3:0, u1:0, u2:0, u1:0, u2:0, u2:0, u1:0, u1:0, u3:0, u1:0, u2:0, u1:0),
        u32:0x11223344);
}

#[test]
fn lfb_and_swizzle_test() {
    let decoded = lfb_decode(u32:0x80402010, u16:0, u4:5, u2:0, u16:0x1234, u1:1);
    assert_eq(decoded[26+:u24], u24:0x402010);
    assert_eq(decoded[2+:u8], u8:0x80);
    assert_eq(decoded[0+:u2], u2:3);
    assert_eq(data_swizzle(u32:0x11223344, u1:1, u1:0), u32:0x33441122);
    assert_eq(data_swizzle(u32:0x11223344, u1:0, u1:1), u32:0x44332211);
    assert_eq(pixel_tests(u24:0x123456, u24:0x123456, u8:7, u8:8, u3:1), u2:3);
}
