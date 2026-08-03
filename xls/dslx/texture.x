fn clamp_s12_to_u8(value: s12) -> u8 {
    if value < s12:0 {
        u8:0
    } else if value > s12:255 {
        u8:255
    } else {
        value as u8
    }
}

fn expand_1(value: u1) -> u8 { if value { u8:255 } else { u8:0 } }

fn expand_2(value: u2) -> u8 { value ++ value ++ value ++ value }

fn expand_3(value: u3) -> u8 {
    ((value as u8) << u8:5) | ((value as u8) << u8:2) | ((value as u8) >> u8:1)
}

fn expand_4(value: u4) -> u8 { value ++ value }

fn expand_5(value: u5) -> u8 { ((value as u8) << u8:3) | ((value as u8) >> u8:2) }

fn expand_6(value: u6) -> u8 { ((value as u8) << u8:2) | ((value as u8) >> u8:4) }

fn pack_rgba(r: u8, g: u8, b: u8, a: u8) -> u32 { r ++ g ++ b ++ a }

fn ncc_channel(y: u8, i: s9, q: s9) -> u8 { clamp_s12_to_u8((y as s12) + (i as s12) + (q as s12)) }

pub fn texel_decode
    (texel: u16, format: u4, palette: u24, ncc_y: u8, ncc_i: u27, ncc_q: u27) -> u32 {
    let texel_byte = texel[0+:u8];
    let ir = ncc_i[18+:u9] as s9;
    let ig = ncc_i[9+:u9] as s9;
    let ib = ncc_i[0+:u9] as s9;
    let qr = ncc_q[18+:u9] as s9;
    let qg = ncc_q[9+:u9] as s9;
    let qb = ncc_q[0+:u9] as s9;
    let nr = ncc_channel(ncc_y, ir, qr);
    let ng = ncc_channel(ncc_y, ig, qg);
    let nb = ncc_channel(ncc_y, ib, qb);
    match format {
        u4:0 => pack_rgba(
            expand_3(texel_byte[5+:u3]), expand_3(texel_byte[2+:u3]), expand_2(texel_byte[0+:u2]),
            u8:255),
        u4:1 => pack_rgba(nr, ng, nb, u8:255),
        u4:2 => pack_rgba(texel_byte, texel_byte, texel_byte, texel_byte),
        u4:3 => pack_rgba(texel_byte, texel_byte, texel_byte, u8:255),
        u4:4 => {
            let intensity = expand_4(texel_byte[0+:u4]);
            pack_rgba(intensity, intensity, intensity, expand_4(texel_byte[4+:u4]))
        },
        u4:5 => pack_rgba(palette[16+:u8], palette[8+:u8], palette[0+:u8], u8:255),
        u4:8 => pack_rgba(
            expand_3(texel_byte[5+:u3]), expand_3(texel_byte[2+:u3]), expand_2(texel_byte[0+:u2]),
            texel[8+:u8]),
        u4:9 => pack_rgba(nr, ng, nb, texel[8+:u8]),
        u4:10 => pack_rgba(
            expand_5(texel[11+:u5]), expand_6(texel[5+:u6]), expand_5(texel[0+:u5]), u8:255),
        u4:11 => pack_rgba(
            expand_5(texel[10+:u5]), expand_5(texel[5+:u5]), expand_5(texel[0+:u5]),
            expand_1(texel[15+:u1])),
        u4:12 => pack_rgba(
            expand_4(texel[8+:u4]), expand_4(texel[4+:u4]), expand_4(texel[0+:u4]),
            expand_4(texel[12+:u4])),
        u4:13 => pack_rgba(texel_byte, texel_byte, texel_byte, texel[8+:u8]),
        u4:14 => pack_rgba(palette[16+:u8], palette[8+:u8], palette[0+:u8], texel[8+:u8]),
        _ => pack_rgba(
            expand_5(texel[11+:u5]), expand_6(texel[5+:u6]), expand_5(texel[0+:u5]), u8:255),
    }
}

fn rgba_r(rgba: u32) -> u8 { rgba[24+:u8] }

fn rgba_g(rgba: u32) -> u8 { rgba[16+:u8] }

fn rgba_b(rgba: u32) -> u8 { rgba[8+:u8] }

fn rgba_a(rgba: u32) -> u8 { rgba[0+:u8] }

fn filter_channel(t0: u8, t1: u8, t2: u8, t3: u8, ds: u4, dt: u4) -> u8 {
    let x = ds as u5;
    let y = dt as u5;
    let ix = u5:16 - x;
    let iy = u5:16 - y;
    let w0 = (ix as u10) * (iy as u10);
    let w1 = (x as u10) * (iy as u10);
    let w2 = (ix as u10) * (y as u10);
    let w3 = (x as u10) * (y as u10);
    let sum = ((t0 as u18) * (w0 as u18)) + ((t1 as u18) * (w1 as u18)) +
              ((t2 as u18) * (w2 as u18)) + ((t3 as u18) * (w3 as u18));
    (sum >> u8:8) as u8
}

pub fn texture_filter(t0: u32, t1: u32, t2: u32, t3: u32, ds: u4, dt: u4, bilinear: u1) -> u32 {
    if !bilinear {
        t0
    } else {
        pack_rgba(
            filter_channel(rgba_r(t0), rgba_r(t1), rgba_r(t2), rgba_r(t3), ds, dt),
            filter_channel(rgba_g(t0), rgba_g(t1), rgba_g(t2), rgba_g(t3), ds, dt),
            filter_channel(rgba_b(t0), rgba_b(t1), rgba_b(t2), rgba_b(t3), ds, dt),
            filter_channel(rgba_a(t0), rgba_a(t1), rgba_a(t2), rgba_a(t3), ds, dt))
    }
}

#[test]
fn rgb565_decode_test() {
    assert_eq(texel_decode(u16:0xf81f, u4:10, u24:0, u8:0, u27:0, u27:0), u32:0xff00ffff);
}

#[test]
fn point_filter_test() {
    assert_eq(texture_filter(u32:0x12345678, u32:0, u32:0, u32:0, u4:0, u4:0, u1:0), u32:0x12345678);
}

#[test]
fn all_texture_formats_test() {
    assert_eq(texel_decode(u16:0xff, u4:0, u24:0, u8:0, u27:0, u27:0), u32:0xffffffff);
    assert_eq(texel_decode(u16:0, u4:1, u24:0, u8:100, u27:0, u27:0), u32:0x646464ff);
    assert_eq(texel_decode(u16:0x80, u4:2, u24:0, u8:0, u27:0, u27:0), u32:0x80808080);
    assert_eq(texel_decode(u16:0x80, u4:3, u24:0, u8:0, u27:0, u27:0), u32:0x808080ff);
    assert_eq(texel_decode(u16:0xa5, u4:4, u24:0, u8:0, u27:0, u27:0), u32:0x555555aa);
    assert_eq(texel_decode(u16:0, u4:5, u24:0x112233, u8:0, u27:0, u27:0), u32:0x112233ff);
    assert_eq(texel_decode(u16:0x80ff, u4:8, u24:0, u8:0, u27:0, u27:0), u32:0xffffff80);
    assert_eq(texel_decode(u16:0x8000, u4:9, u24:0, u8:100, u27:0, u27:0), u32:0x64646480);
    assert_eq(texel_decode(u16:0xffff, u4:11, u24:0, u8:0, u27:0, u27:0), u32:0xffffffff);
    assert_eq(texel_decode(u16:0xabcd, u4:12, u24:0, u8:0, u27:0, u27:0), u32:0xbbccddaa);
    assert_eq(texel_decode(u16:0x8040, u4:13, u24:0, u8:0, u27:0, u27:0), u32:0x40404080);
    assert_eq(texel_decode(u16:0x8042, u4:14, u24:0x112233, u8:0, u27:0, u27:0), u32:0x11223380);
}

#[test]
fn bilinear_filter_test() {
    assert_eq(texture_filter(u32:0x10203040, u32:0, u32:0, u32:0, u4:0, u4:0, u1:1), u32:0x10203040);
    assert_eq(
        texture_filter(u32:0, u32:0, u32:0, u32:0xffffffff, u4:15, u4:15, u1:1), u32:0xe0e0e0e0);
    assert_eq(texture_filter(u32:0, u32:0, u32:0, u32:0xffffffff, u4:8, u4:8, u1:1), u32:0x3f3f3f3f);
}
