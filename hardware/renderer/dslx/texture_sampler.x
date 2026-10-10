import layout;
import texture;
import tmu;
import tmu_lookup;

pub struct SampleRequest {
    enabled: u1,
    bilinear: u1,
    is_16_bit: u1,
    format: u4,
    large_cache_safe: u1,
    addresses: uN[104],
    locations: u4[4],
    ds: u4,
    dt: u4,
}


fn cache_location(segment: u48, is_16_bit: u1, address: u26) -> u4 {
    let offset = (address as u10) - (segment[26+:u22] as u10);
    let shift = segment[0+:u4];
    let bank_msb = if is_16_bit == u1:1 {
        offset[shift+:u1] | offset[(shift + u4:1) as u4+:u1]
    } else {
        offset[shift+:u1]
    };
    let bank_lsb = if is_16_bit == u1:1 { address[1+:u1] } else { address[0+:u1] };
    let pair = is_16_bit == u1:0 && address[1+:u1] == u1:1;
    u1:1 ++ bank_msb ++ bank_lsb ++ (pair as u1)
}

fn large_cache_safe(segment: u48, addresses: uN[104]) -> u1 {
    let base = segment[26+:u22];
    let end = segment[4+:u22];
    let address0 = addresses[78+:u26][0+:u22];
    let address1 = addresses[52+:u26][0+:u22];
    let address2 = addresses[26+:u26][0+:u22];
    let address3 = addresses[0+:u26][0+:u22];
    address0 >= base && address0 < end &&
    address1 >= base && address1 < end &&
    address2 >= base && address2 < end &&
    address3 >= base && address3 < end
}

fn cache_locations(segment: u48, is_16_bit: u1, addresses: uN[104]) -> u4[4] {
    u4[4]:[
        cache_location(segment, is_16_bit, addresses[78+:u26]),
        cache_location(segment, is_16_bit, addresses[52+:u26]),
        cache_location(segment, is_16_bit, addresses[26+:u26]),
        cache_location(segment, is_16_bit, addresses[0+:u26]),
    ]
}

pub fn base_lod(ds_dx: s32, dt_dx: s32, ds_dy: s32, dt_dy: s32) -> s16 {
    let gradient = tmu::tmu_gradient(ds_dx, dt_dx, ds_dy, dt_dy);
    let index = tmu::tmu_lod_index(gradient);
    tmu::tmu_base_lod(gradient, index[8+:u7], tmu_lookup::log_fraction(index[0+:u8]))
}


pub fn sample_request_with_base_lod
    (s: s48, t: s48, w: s48, base_lod: s16, texture_mode: u32, t_lod: u32,
     texture_layout: bits[432], base0: u24, base1: u24, base2: u24, base38: u24)
    -> SampleRequest {
    let meta = tmu::tmu_recip_meta(w, texture_mode[3+:u1]);
    let index = meta[17+:u8];
    let base = tmu_lookup::reciprocal(index as u9);
    let next = tmu_lookup::reciprocal((index as u9) + u9:1);
    let reciprocal = tmu::tmu_recip(
        base, base - next, meta[9+:u8], tmu_lookup::log_fraction(index), meta[2+:u7], meta[1+:u1]);
    let perspective =
        tmu::tmu_perspective_coords(s, t, reciprocal[16+:u17], meta[2+:u7], texture_mode[0+:u1]);

    let lod = tmu::tmu_lod_select(base_lod, reciprocal[0+:u16] as s16, t_lod[0+:u27]);
    let level = lod[8+:u4];
    let coordinates = tmu::tmu_coordinates(
        perspective[18+:u18] as s18, perspective[0+:u18] as s18, level, meta[0+:u1]);
    let segment = (texture_layout >> (((u4:8 - level) as u10) * u10:48)) as u48;
    let is_16_bit = (texture_mode[8+:u4] >= u4:8) as u1;
    let addresses = tmu::tmu_addresses(
        coordinates[50+:u14] as s14, coordinates[36+:u14] as s14, coordinates[22+:u14] as s14,
        coordinates[8+:u14] as s14, lod[4+:u4], lod[0+:u4], texture_mode[6+:u1],
        texture_mode[7+:u1], is_16_bit, u1:1, segment[26+:u22], segment[0+:u4], base0, base1,
        base2, base38, t_lod[24+:u1], level);
    let bilinear = texture_mode[1+:u1] || texture_mode[2+:u1];
    let selected = if bilinear == u1:1 {
        addresses[78+:u26] ++ addresses[52+:u26] ++ addresses[26+:u26] ++ addresses[0+:u26]
    } else {
        addresses[104+:u26] ++ addresses[104+:u26] ++ addresses[104+:u26] ++ addresses[104+:u26]
    };
    SampleRequest {
        enabled: texture_mode[0+:u32] != u32:0,
        bilinear,
        is_16_bit,
        format: texture_mode[8+:u4],
        large_cache_safe: large_cache_safe(segment, selected),
        addresses: selected,
        locations: cache_locations(segment, is_16_bit, selected),
        ds: coordinates[4+:u4],
        dt: coordinates[0+:u4],
    }
}

pub fn sample_request
    (s: s48, t: s48, w: s48, ds_dx: s32, dt_dx: s32, ds_dy: s32, dt_dy: s32, texture_mode: u32,
     t_lod: u32, texture_layout: bits[432], base0: u24, base1: u24, base2: u24, base38: u24)
    -> SampleRequest {
    sample_request_with_base_lod(
        s, t, w, base_lod(ds_dx, dt_dx, ds_dy, dt_dy), texture_mode, t_lod,
        texture_layout, base0, base1, base2, base38)
}

pub fn read_mask(addresses: uN[104]) -> u4 {
    let a0 = addresses[78+:u26] >> u8:2;
    let a1 = addresses[52+:u26] >> u8:2;
    let a2 = addresses[26+:u26] >> u8:2;
    let a3 = addresses[0+:u26] >> u8:2;
    u4:1 | (((a1 != a0) as u4) << u8:1) | (((a2 != a0 && a2 != a1) as u4) << u8:2) |
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

pub fn decode_texels(request: SampleRequest, raw0: u16, raw1: u16, raw2: u16, raw3: u16) -> u32 {
    let texel0 = texture::texel_decode(raw0, request.format, u24:0, u8:0, u27:0, u27:0);
    let texel1 = texture::texel_decode(raw1, request.format, u24:0, u8:0, u27:0, u27:0);
    let texel2 = texture::texel_decode(raw2, request.format, u24:0, u8:0, u27:0, u27:0);
    let texel3 = texture::texel_decode(raw3, request.format, u24:0, u8:0, u27:0, u27:0);
    texture::texture_filter(
        texel0, texel1, texel2, texel3, request.ds, request.dt, request.bilinear)
}

pub fn decode_texels_with_entries
    (request: SampleRequest, raw0: u16, raw1: u16, raw2: u16, raw3: u16, entries: u24[4]) -> u32 {
    let texel0 = texture::texel_decode(raw0, request.format, entries[u32:0], u8:0, u27:0, u27:0);
    let texel1 = texture::texel_decode(raw1, request.format, entries[u32:1], u8:0, u27:0, u27:0);
    let texel2 = texture::texel_decode(raw2, request.format, entries[u32:2], u8:0, u27:0, u27:0);
    let texel3 = texture::texel_decode(raw3, request.format, entries[u32:3], u8:0, u27:0, u27:0);
    texture::texture_filter(
        texel0, texel1, texel2, texel3, request.ds, request.dt, request.bilinear)
}

pub fn decode_texels_with_palette
    (request: SampleRequest, raw0: u16, raw1: u16, raw2: u16, raw3: u16, palette: u24[256]) -> u32 {
    decode_texels_with_entries(
        request, raw0, raw1, raw2, raw3,
        u24[4]:[
            palette[raw0[0+:u8]], palette[raw1[0+:u8]], palette[raw2[0+:u8]], palette[raw3[0+:u8]],
        ])
}

pub fn decode(request: SampleRequest, word0: u32, word1: u32, word2: u32, word3: u32) -> u32 {
    let address0 = request.addresses[78+:u26];
    let address1 = request.addresses[52+:u26];
    let address2 = request.addresses[26+:u26];
    let address3 = request.addresses[0+:u26];
    decode_texels(
        request, texel(word0, address0, request.is_16_bit),
        texel(word1, address1, request.is_16_bit), texel(word2, address2, request.is_16_bit),
        texel(word3, address3, request.is_16_bit))
}

pub fn decode_with_palette
    (request: SampleRequest, word0: u32, word1: u32, word2: u32, word3: u32, palette: u24[256])
    -> u32 {
    decode_texels_with_palette(
        request, texel(word0, request.addresses[78+:u26], request.is_16_bit),
        texel(word1, request.addresses[52+:u26], request.is_16_bit),
        texel(word2, request.addresses[26+:u26], request.is_16_bit),
        texel(word3, request.addresses[0+:u26], request.is_16_bit), palette)
}

#[quickcheck]
fn base_lod_matches_full_width_gradient(ds_dx: s32, dt_dx: s32, ds_dy: s32, dt_dy: s32) -> bool {
    let dx = ((ds_dx as s64) * (ds_dx as s64)) as u64 +
             ((dt_dx as s64) * (dt_dx as s64)) as u64;
    let dy = ((ds_dy as s64) * (ds_dy as s64)) as u64 +
             ((dt_dy as s64) * (dt_dy as s64)) as u64;
    let gradient = if dx > dy { dx } else { dy };
    let index = tmu::tmu_lod_index(gradient);
    base_lod(ds_dx, dt_dx, ds_dy, dt_dy) ==
        tmu::tmu_base_lod(gradient, index[8+:u7], tmu_lookup::log_fraction(index[0+:u8]))
}

#[test]
fn base_lod_preserves_zero_negative_and_extreme_derivatives() {
    assert_eq(base_lod(s32:0, s32:0, s32:0, s32:0), s16:0);
    assert_eq(base_lod(s32:1048576, s32:0, s32:0, s32:0), s16:256);
    assert_eq(base_lod(s32:0, s32:0, s32:-1048576, s32:0), s16:256);
    assert_eq(base_lod(s32:0x80000000, s32:0, s32:0, s32:0), s16:1664);
    assert_eq(base_lod(s32:0x80000000, s32:0x80000000, s32:0, s32:0), s16:1728);
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
fn either_filter_bit_enables_bilinear_sampling() {
    let texture_layout = layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:2, u2:1, u1:1, u1:0);
    let minified = sample_request(
        s48:18 << u8:18, s48:24 << u8:18, s48:1 << u8:30, s32:0, s32:0, s32:0, s32:0,
        u32:0x000012c2, u32:0x00300104, texture_layout, u24:0, u24:0, u24:0, u24:0);
    let magnified = sample_request(
        s48:18 << u8:18, s48:24 << u8:18, s48:1 << u8:30, s32:0, s32:0, s32:0, s32:0,
        u32:0x000012c4, u32:0x00300104, texture_layout, u24:0, u24:0, u24:0, u24:0);
    assert_eq(minified.bilinear, u1:1);
    assert_eq(magnified.bilinear, u1:1);
}

#[test]
fn rejects_large_cache_mapping_for_wrapped_addresses() {
    let segment = (u48:0x100 << u8:26) | (u48:0x120 << u8:4);
    let inside = u26:0x110 ++ u26:0x112 ++ u26:0x114 ++ u26:0x116;
    let wrapped = u26:0x130 ++ u26:0x132 ++ u26:0x134 ++ u26:0x136;
    assert_eq(large_cache_safe(segment, inside), u1:1);
    assert_eq(large_cache_safe(segment, wrapped), u1:0);
}

#[test]
fn deduplicates_texture_word_reads() {
    let addresses = u26:1 ++ u26:2 ++ u26:129 ++ u26:130;
    assert_eq(read_mask(addresses), u4:0x5);
    let addresses = u26:0 ++ u26:4 ++ u26:8 ++ u26:12;
    assert_eq(read_mask(addresses), u4:0xf);
}

#[test]
fn decodes_paletted_texels() {
    let request = SampleRequest {
        enabled: u1:1,
        bilinear: u1:0,
        is_16_bit: u1:0,
        format: u4:5,
        large_cache_safe: u1:0,
        addresses: uN[104]:0,
        locations: u4[4]:[u4:0, ...],
        ds: u4:0,
        dt: u4:0,
    };
    let palette = update(u24[256]:[u24:0, ...], u8:0x22, u24:0x123456);
    assert_eq(
        decode_texels_with_palette(request, u16:0x2222, u16:0, u16:0, u16:0, palette),
        u32:0x123456ff);
    assert_eq(
        decode_texels_with_entries(
            request, u16:0x2222, u16:0, u16:0, u16:0, u24[4]:[u24:0x123456, u24:0, u24:0, u24:0]),
        u32:0x123456ff);
}

#[test]
fn extracts_and_decodes_eight_bit_texels() {
    let request = SampleRequest {
        enabled: u1:1,
        bilinear: u1:0,
        is_16_bit: u1:0,
        format: u4:2,
        large_cache_safe: u1:0,
        addresses: u26:1 ++ u26:1 ++ u26:1 ++ u26:1,
        locations: u4[4]:[u4:0, ...],
        ds: u4:0,
        dt: u4:0,
    };
    assert_eq(texel(u32:0x44332211, u26:1, u1:0), u16:0x2222);
    assert_eq(decode(request, u32:0x44332211, u32:0, u32:0, u32:0), u32:0x22222222);
}
