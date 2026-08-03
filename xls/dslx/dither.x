fn offset_4x4(x: u16, y: u16) -> u4 {
    let position = ((y[0+:u2] as u4) << u8:2) | (x[0+:u2] as u4);
    match position {
        u4:0 => u4:0,
        u4:1 => u4:8,
        u4:2 => u4:2,
        u4:3 => u4:10,
        u4:4 => u4:12,
        u4:5 => u4:4,
        u4:6 => u4:14,
        u4:7 => u4:6,
        u4:8 => u4:3,
        u4:9 => u4:11,
        u4:10 => u4:1,
        u4:11 => u4:9,
        u4:12 => u4:15,
        u4:13 => u4:7,
        u4:14 => u4:13,
        _ => u4:5,
    }
}

fn offset_2x2(x: u16, y: u16) -> u4 {
    let position = ((y[0+:u1] as u2) << u8:1) | (x[0+:u1] as u2);
    match position {
        u2:0 => u4:2,
        u2:1 => u4:10,
        u2:2 => u4:14,
        _ => u4:6,
    }
}

fn dither_5(value: u8, offset: u4) -> u5 {
    let expanded = ((value as u10) << u8:1) - ((value >> u8:4) as u10) + ((value >> u8:7) as u10) +
                   (offset as u10);
    let result = expanded >> u8:4;
    if result > u10:31 { u5:31 } else { result as u5 }
}

fn dither_6(value: u8, offset: u4) -> u6 {
    let expanded = ((value as u11) << u8:2) - ((value >> u8:4) as u11) + ((value >> u8:6) as u11) +
                   (offset as u11);
    let result = expanded >> u8:4;
    if result > u11:63 { u6:63 } else { result as u6 }
}

pub fn rgb565(red: u8, green: u8, blue: u8, x: u16, y: u16, enable: u1, use_2x2: u1) -> u16 {
    let offset = if use_2x2 == u1:1 { offset_2x2(x, y) } else { offset_4x4(x, y) };
    let r = if enable == u1:1 { dither_5(red, offset) } else { (red >> u8:3) as u5 };
    let g = if enable == u1:1 { dither_6(green, offset) } else { (green >> u8:2) as u6 };
    let b = if enable == u1:1 { dither_5(blue, offset) } else { (blue >> u8:3) as u5 };
    ((r as u16) << u8:11) | ((g as u16) << u8:5) | (b as u16)
}

fn clamp_s10(value: s10) -> u8 {
    if value < s10:0 {
        u8:0
    } else if value > s10:255 {
        u8:255
    } else {
        value as u8
    }
}

fn subtract_5(value: u8, offset: u4) -> u8 {
    if value < u8:4 {
        u8:0
    } else {
        clamp_s10((value as s10) + s10:5 - ((((offset as u5) + u5:1) >> u8:1) as s10))
    }
}

fn subtract_6(value: u8, offset: u4) -> u8 {
    if value < u8:2 {
        u8:0
    } else {
        clamp_s10((value as s10) + s10:3 - ((((offset as u5) + u5:3) >> u8:2) as s10))
    }
}

pub fn subtract_rgb(rgb: u24, x: u16, y: u16, use_2x2: u1) -> u24 {
    let offset = if use_2x2 == u1:1 { offset_2x2(x, y) } else { offset_4x4(x, y) };
    subtract_5(rgb[16+:u8], offset) ++ subtract_6(rgb[8+:u8], offset) ++
    subtract_5(rgb[0+:u8], offset)
}

#[test]
fn subtracts_destination_dither() {
    assert_eq(subtract_rgb(u24:0x040204, u16:0, u16:0, u1:0), u24:0x090509);
    assert_eq(subtract_rgb(u24:0xffffff, u16:2, u16:1, u1:0), u24:0xfdfefd);
}

#[test]
fn truncates_when_disabled() {
    assert_eq(rgb565(u8:255, u8:127, u8:31, u16:0, u16:0, u1:0, u1:0), u16:0xfbe3);
}

#[test]
fn applies_four_by_four_thresholds() {
    assert_eq(rgb565(u8:1, u8:1, u8:1, u16:0, u16:0, u1:1, u1:0), u16:0);
    assert_eq(rgb565(u8:1, u8:1, u8:1, u16:2, u16:1, u1:1, u1:0), u16:0x0821);
}

#[test]
fn applies_two_by_two_thresholds_and_clamps() {
    assert_eq(rgb565(u8:1, u8:1, u8:1, u16:0, u16:1, u1:1, u1:1), u16:0x0821);
    assert_eq(rgb565(u8:255, u8:255, u8:255, u16:0, u16:1, u1:1, u1:1), u16:0xffff);
}
