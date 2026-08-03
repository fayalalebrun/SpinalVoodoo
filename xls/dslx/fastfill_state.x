import dither;

pub struct FastfillState {
    active: u1,
    x: u10,
    y: u10,
    left: u10,
    right: u10,
    high: u10,
    red: u8,
    green: u8,
    blue: u8,
    depth: u16,
    color_base: u26,
    aux_base: u26,
    stride: u11,
    rgb_write: u1,
    aux_write: u1,
    y_origin: u1,
    y_origin_swap: u10,
    enable_dithering: u1,
    use_2x2_dither: u1,
}

pub struct FastfillWord {
    valid: u1,
    x: u16,
    y: u16,
    color: u32,
    depth: u32,
    mask: u4,
    color_base: u26,
    aux_base: u26,
    stride: u11,
    rgb_write: u1,
    aux_write: u1,
}

pub fn initial_state() -> FastfillState {
    FastfillState {
        active: u1:0,
        x: u10:0,
        y: u10:0,
        left: u10:0,
        right: u10:0,
        high: u10:0,
        red: u8:0,
        green: u8:0,
        blue: u8:0,
        depth: u16:0,
        color_base: u26:0,
        aux_base: u26:0,
        stride: u11:0,
        rgb_write: u1:0,
        aux_write: u1:0,
        y_origin: u1:0,
        y_origin_swap: u10:0,
        enable_dithering: u1:0,
        use_2x2_dither: u1:0,
    }
}

pub fn begin
    (left: u10, right: u10, low: u10, high: u10, red: u8, green: u8, blue: u8, depth: u16,
     color_base: u26, aux_base: u26, stride: u11, rgb_write: u1, aux_write: u1, y_origin: u1,
     y_origin_swap: u10, enable_dithering: u1, use_2x2_dither: u1) -> FastfillState {
    FastfillState {
        active: (left < right && low < high && (rgb_write == u1:1 || aux_write == u1:1)) as u1,
        x: left & u10:0x3fe,
        y: low,
        left,
        right,
        high,
        red,
        green,
        blue,
        depth,
        color_base,
        aux_base,
        stride,
        rgb_write,
        aux_write,
        y_origin,
        y_origin_swap,
        enable_dithering,
        use_2x2_dither,
    }
}

pub fn tick(state: FastfillState) -> (FastfillState, FastfillWord) {
    let output_y = if state.y_origin == u1:1 {
        (state.y_origin_swap as u16) - (state.y as u16)
    } else {
        state.y as u16
    };
    let lane1_x = state.x + u10:1;
    let lane0 = state.x >= state.left && state.x < state.right;
    let lane1 = lane1_x >= state.left && lane1_x < state.right;
    let color0 = dither::rgb565(
        state.red, state.green, state.blue, state.x as u16, output_y, state.enable_dithering,
        state.use_2x2_dither);
    let color1 = dither::rgb565(
        state.red, state.green, state.blue, lane1_x as u16, output_y, state.enable_dithering,
        state.use_2x2_dither);
    let word = FastfillWord {
        valid: state.active,
        x: state.x as u16,
        y: output_y,
        color:
            (if lane0 { color0 as u32 } else { u32:0 }) |
            (if lane1 { (color1 as u32) << u8:16 } else { u32:0 }),
        depth:
            (if lane0 { state.depth as u32 } else { u32:0 }) |
            (if lane1 { (state.depth as u32) << u8:16 } else { u32:0 }),
        mask: (if lane0 { u4:0x3 } else { u4:0 }) | (if lane1 { u4:0xc } else { u4:0 }),
        color_base: state.color_base,
        aux_base: state.aux_base,
        stride: state.stride,
        rgb_write: state.rgb_write,
        aux_write: state.aux_write,
    };
    let end_row = (state.x as u11) + u11:2 >= (state.right as u11);
    let end_fill = end_row && state.y + u10:1 >= state.high;
    let next_state = FastfillState {
        active: if end_fill { u1:0 } else { state.active },
        x: if end_row { state.left & u10:0x3fe } else { state.x + u10:2 },
        y: if end_row { state.y + u10:1 } else { state.y },
        ..state
    };
    (next_state, word)
}

#[test]
fn walks_clip_rectangle() {
    let state = begin(
        u10:2, u10:4, u10:3, u10:5, u8:1, u8:2, u8:3, u16:0xabcd, u26:0x1000, u26:0x2000, u11:640,
        u1:1, u1:1, u1:0, u10:0, u1:0, u1:0);
    let (state, p0) = tick(state);
    let (state, p1) = tick(state);
    assert_eq((p0.x, p0.y, p0.mask), (u16:2, u16:3, u4:0xf));
    assert_eq((p1.x, p1.y, p1.mask), (u16:2, u16:4, u4:0xf));
    assert_eq(p0.depth, u32:0xabcdabcd);
    assert_eq(state.active, u1:0);
}

#[test]
fn masks_odd_clip_edges() {
    let state = begin(
        u10:1, u10:3, u10:0, u10:1, u8:255, u8:0, u8:0, u16:0, u26:0, u26:0, u11:640, u1:1, u1:0,
        u1:0, u10:0, u1:0, u1:0);
    let (state, p0) = tick(state);
    let (state, p1) = tick(state);
    assert_eq((p0.x, p0.mask, p0.color), (u16:0, u4:0xc, u32:0xf8000000));
    assert_eq((p1.x, p1.mask, p1.color), (u16:2, u4:0x3, u32:0x0000f800));
    assert_eq(state.active, u1:0);
}

#[test]
fn applies_y_origin() {
    let state = begin(
        u10:0, u10:1, u10:4, u10:5, u8:0, u8:0, u8:0, u16:0, u26:0, u26:0, u11:640, u1:1, u1:0,
        u1:1, u10:99, u1:0, u1:0);
    let (_, pixel) = tick(state);
    assert_eq(pixel.y, u16:95);
}

#[test]
fn rejects_empty_rectangle() {
    let state = begin(
        u10:4, u10:4, u10:1, u10:2, u8:0, u8:0, u8:0, u16:0, u26:0, u26:0, u11:640, u1:1, u1:1,
        u1:0, u10:0, u1:0, u1:0);
    let (_, pixel) = tick(state);
    assert_eq(pixel.valid, u1:0);
}
