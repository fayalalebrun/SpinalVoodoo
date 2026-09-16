fn max_sub(base: u4, amount: u4) -> u4 { if amount > base { u4:0 } else { base - amount } }

fn layout_step
    (lod: u4, offset: u22, tail_offset: u22, base0: u22, base1: u22, base2: u22, base38: u22,
     aspect: u2, s_wider: u1, multibase: u1, is_16_bit: u1) -> (u48, u22, u22) {
    let wide_bits = max_sub(u4:8, lod);
    let narrow_base = u4:8 - (aspect as u4);
    let narrow_bits = max_sub(narrow_base, lod);
    let normal_base = base0 + offset;
    let lod_base = if multibase && lod == u4:1 {
        base1
    } else if multibase && lod == u4:2 {
        base2
    } else if multibase && lod >= u4:3 {
        base38 + tail_offset
    } else {
        normal_base
    };
    let wide_size = u22:1 << (wide_bits as u8);
    let area = wide_size << (narrow_bits as u8);
    let lod_size = if is_16_bit { area << u8:1 } else { area };
    let lod_end = lod_base + lod_size;
    let row_shift = if s_wider { wide_bits } else { narrow_bits };
    let segment = lod_base ++ lod_end ++ row_shift;
    let next_tail = if lod >= u4:3 { tail_offset + lod_size } else { tail_offset };
    (segment, offset + lod_size, next_tail)
}

pub fn texture_layout
    (tex_base: u19, tex_base1: u19, tex_base2: u19, tex_base38: u19, format: u4, aspect: u2,
     s_wider: u1, multibase: u1) -> bits[432] {
    let base0 = (tex_base as u22) << u8:3;
    let base1 = (tex_base1 as u22) << u8:3;
    let base2 = (tex_base2 as u22) << u8:3;
    let base38 = (tex_base38 as u22) << u8:3;
    let is_16_bit = format >= u4:8;
    let step0 = layout_step(
        u4:0, u22:0, u22:0, base0, base1, base2, base38, aspect, s_wider, multibase, is_16_bit);
    let step1 = layout_step(
        u4:1, step0.1, step0.2, base0, base1, base2, base38, aspect, s_wider, multibase, is_16_bit);
    let step2 = layout_step(
        u4:2, step1.1, step1.2, base0, base1, base2, base38, aspect, s_wider, multibase, is_16_bit);
    let step3 = layout_step(
        u4:3, step2.1, step2.2, base0, base1, base2, base38, aspect, s_wider, multibase, is_16_bit);
    let step4 = layout_step(
        u4:4, step3.1, step3.2, base0, base1, base2, base38, aspect, s_wider, multibase, is_16_bit);
    let step5 = layout_step(
        u4:5, step4.1, step4.2, base0, base1, base2, base38, aspect, s_wider, multibase, is_16_bit);
    let step6 = layout_step(
        u4:6, step5.1, step5.2, base0, base1, base2, base38, aspect, s_wider, multibase, is_16_bit);
    let step7 = layout_step(
        u4:7, step6.1, step6.2, base0, base1, base2, base38, aspect, s_wider, multibase, is_16_bit);
    let step8 = layout_step(
        u4:8, step7.1, step7.2, base0, base1, base2, base38, aspect, s_wider, multibase, is_16_bit);
    step0.0 ++ step1.0 ++ step2.0 ++ step3.0 ++ step4.0 ++ step5.0 ++ step6.0 ++ step7.0 ++ step8.0
}

#[test]
fn texture_layout_test() {
    let layout = texture_layout(u19:0, u19:0, u19:0, u19:0, u4:10, u2:0, u1:0, u1:0);
    assert_eq(layout[410+:u22], u22:0);
    assert_eq(layout[388+:u22], u22:0x20000);
    assert_eq(layout[384+:u4], u4:8);
    let narrow = texture_layout(
        u19:0x12260, u19:0, u19:0, u19:0, u4:10, u2:2, u1:1, u1:0);
    assert_eq(narrow[314+:u22], u22:0x9b300);
    assert_eq(narrow[292+:u22], u22:0x9bb00);
    assert_eq(narrow[288+:u4], u4:6);
}

#[test]
fn texture_multibase_layout_test() {
    let layout = texture_layout(u19:0x10, u19:0x20, u19:0x30, u19:0x40, u4:10, u2:0, u1:1, u1:1);
    assert_eq(layout[410+:u22], u22:0x80);
    assert_eq(layout[362+:u22], u22:0x100);
    assert_eq(layout[314+:u22], u22:0x180);
    assert_eq(layout[266+:u22], u22:0x200);
    assert_eq(layout[218+:u22], u22:0xa00);
    assert_eq(layout[192+:u4], u4:4);
}
