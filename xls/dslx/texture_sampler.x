import layout;
import texture;
import tmu;
import tmu_lookup;

pub struct SampleRequest {
    enabled: u1,
    bilinear: u1,
    is_16_bit: u1,
    format: u4,
    addresses: uN[104],
    ds: u4,
    dt: u4,
}

pub fn sample_request
    (s: s48, t: s48, w: s48, ds_dx: s32, dt_dx: s32, ds_dy: s32, dt_dy: s32, texture_mode: u32,
     t_lod: u32, texture_layout: bits[432], base0: u24, base1: u24, base2: u24, base38: u24)
    -> SampleRequest {
    let meta = tmu::tmu_recip_meta(w, texture_mode[3+:u1]);
    let index = meta[17+:u8];
    let base = tmu_lookup::reciprocal(index as u9);
    let next = tmu_lookup::reciprocal((index as u9) + u9:1);
    let reciprocal = tmu::tmu_recip(
        base, base - next, meta[9+:u8], tmu_lookup::log_fraction(index), meta[2+:u7], meta[1+:u1]);
    let perspective =
        tmu::tmu_perspective_coords(s, t, reciprocal[16+:u17], meta[2+:u7], texture_mode[0+:u1]);
    let gradient = tmu::tmu_gradient(ds_dx, dt_dx, ds_dy, dt_dy);
    let lod_index = tmu::tmu_lod_index(gradient);
    let base_lod =
        tmu::tmu_base_lod(gradient, lod_index[8+:u7], tmu_lookup::log_fraction(lod_index[0+:u8]));
    let lod = tmu::tmu_lod_select(base_lod, reciprocal[0+:u16] as s16, t_lod[0+:u27]);
    let level = lod[8+:u4];
    let coordinates = tmu::tmu_coordinates(
        perspective[18+:u18] as s18, perspective[0+:u18] as s18, level, meta[0+:u1]);
    let segment = (texture_layout >> (((u4:8 - level) as u10) * u10:48)) as u48;
    let addresses = tmu::tmu_addresses(
        coordinates[50+:u14] as s14, coordinates[36+:u14] as s14, coordinates[22+:u14] as s14,
        coordinates[8+:u14] as s14, lod[4+:u4], lod[0+:u4], texture_mode[6+:u1],
        texture_mode[7+:u1], (texture_mode[8+:u4] >= u4:8) as u1, u1:1, segment[26+:u22],
        segment[0+:u4], base0, base1, base2, base38, t_lod[24+:u1], level);
    let bilinear = if level == u4:0 { texture_mode[2+:u1] } else { texture_mode[1+:u1] };
    let selected = if bilinear == u1:1 {
        addresses[78+:u26] ++ addresses[52+:u26] ++ addresses[26+:u26] ++ addresses[0+:u26]
    } else {
        addresses[104+:u26] ++ addresses[104+:u26] ++ addresses[104+:u26] ++ addresses[104+:u26]
    };
    SampleRequest {
        enabled: texture_mode[0+:u32] != u32:0,
        bilinear,
        is_16_bit: (texture_mode[8+:u4] >= u4:8) as u1,
        format: texture_mode[8+:u4],
        addresses: selected,
        ds: coordinates[4+:u4],
        dt: coordinates[0+:u4],
    }
}

pub fn read_mask(addresses: uN[104]) -> u4 {
    let a0 = addresses[78+:u26] >> u8:2;
    let a1 = addresses[52+:u26] >> u8:2;
    let a2 = addresses[26+:u26] >> u8:2;
    let a3 = addresses[0+:u26] >> u8:2;
    u4:1 |
    (((a1 != a0) as u4) << u8:1) |
    (((a2 != a0 && a2 != a1) as u4) << u8:2) |
    (((a3 != a0 && a3 != a1 && a3 != a2) as u4) << u8:3)
}

pub fn texel(word: u32, address: u26, is_16_bit: u1) -> u16 {
    if is_16_bit == u1:1 {
        if address[1+:u1] == u1:1 { word[16+:u16] } else { word[0+:u16] }
    } else {
        let shift = (address[0+:u2] as u5) << u8:3;
        let byte = (word >> shift) as u8;
        byte ++ byte
    }
}

pub fn decode(request: SampleRequest, word0: u32, word1: u32, word2: u32, word3: u32) -> u32 {
    let address0 = request.addresses[78+:u26];
    let address1 = request.addresses[52+:u26];
    let address2 = request.addresses[26+:u26];
    let address3 = request.addresses[0+:u26];
    let texel0 = texture::texel_decode(
        texel(word0, address0, request.is_16_bit), request.format, u24:0, u8:0, u27:0, u27:0);
    let texel1 = texture::texel_decode(
        texel(word1, address1, request.is_16_bit), request.format, u24:0, u8:0, u27:0, u27:0);
    let texel2 = texture::texel_decode(
        texel(word2, address2, request.is_16_bit), request.format, u24:0, u8:0, u27:0, u27:0);
    let texel3 = texture::texel_decode(
        texel(word3, address3, request.is_16_bit), request.format, u24:0, u8:0, u27:0, u27:0);
    texture::texture_filter(
        texel0, texel1, texel2, texel3, request.ds, request.dt, request.bilinear)
}

#[test]
fn requests_console_texture_taps() {
    let texture_layout = layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:2, u2:1, u1:1, u1:0);
    let request = sample_request(
        s48:18 << u8:18, s48:24 << u8:18, s48:1 << u8:30, s32:0, s32:0, s32:0, s32:0,
        u32:0x000012cf, u32:0x00300104, texture_layout, u24:0, u24:0, u24:0, u24:0);
    assert_eq(request.bilinear, u1:1);
    assert_eq(request.format, u4:2);
    assert_eq(request.addresses[78+:u26], u26:0x8588);
    assert_eq((request.ds, request.dt), (u4:8, u4:8));
}

#[test]
fn deduplicates_texture_word_reads() {
    let addresses = u26:1 ++ u26:2 ++ u26:129 ++ u26:130;
    assert_eq(read_mask(addresses), u4:0x5);
    let addresses = u26:0 ++ u26:4 ++ u26:8 ++ u26:12;
    assert_eq(read_mask(addresses), u4:0xf);
}

#[test]
fn extracts_and_decodes_eight_bit_texels() {
    let request = SampleRequest {
        enabled: u1:1,
        bilinear: u1:0,
        is_16_bit: u1:0,
        format: u4:2,
        addresses: u26:1 ++ u26:1 ++ u26:1 ++ u26:1,
        ds: u4:0,
        dt: u4:0,
    };
    assert_eq(texel(u32:0x44332211, u26:1, u1:0), u16:0x2222);
    assert_eq(decode(request, u32:0x44332211, u32:0, u32:0, u32:0), u32:0x22222222);
}
