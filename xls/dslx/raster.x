fn choose_signed(value: s48, negate: u1) -> s48 { if negate { -value } else { value } }

pub fn triangle_iter_load
    (start: s48, dx: s48, dy: s48, dx_subpixel: u4, dy_subpixel: u4, adjust: u1, dx_negative: u1,
     dy_negative: u1) -> uN[198] {
    let correction = (((dx as sN[54]) * (dx_subpixel as sN[54])) +
                     ((dy as sN[54]) * (dy_subpixel as sN[54]))) >> u8:4;
    let accumulator =
        if adjust { (start as sN[66]) + (correction as sN[66]) } else { start as sN[66] };
    let dx_term = choose_signed(dx, dx_negative) as sN[66];
    let dy_term = choose_signed(dy, dy_negative) as sN[66];
    (accumulator as uN[66]) ++ (dx_term as uN[66]) ++ (dy_term as uN[66])
}

fn scale_chunk(value: sN[66], chunk: u2) -> sN[68] {
    match chunk {
        u2:1 => value as sN[68],
        u2:2 => (value as sN[68]) << u8:1,
        u2:3 => (value as sN[68]) + ((value as sN[68]) << u8:1),
        _ => sN[68]:0,
    }
}

pub fn triangle_iter_step
    (accumulator: sN[66], dx_term: sN[66], dy_term: sN[66], dx_chunk: u2, dy_chunk: u2) -> uN[198] {
    let next = ((accumulator as sN[68]) + scale_chunk(dx_term, dx_chunk) +
               scale_chunk(dy_term, dy_chunk)) as
               sN[66];
    let next_dx = (dx_term << u8:2) as sN[66];
    let next_dy = (dy_term << u8:2) as sN[66];
    (next as uN[66]) ++ (next_dx as uN[66]) ++ (next_dy as uN[66])
}

pub fn signed_magnitude_16(value: s16) -> u16 {
    if value < s16:0 { (-value) as u16 } else { value as u16 }
}

fn floor_vertex(value: s20) -> s20 { (value >> u8:4) << u8:4 }

fn ceil_vertex(value: s20) -> s20 {
    let floor = floor_vertex(value);
    if (value as u4) == u4:0 { floor } else { floor + s20:16 }
}

fn round_half_down_vertex(value: s20) -> s20 {
    let floor = floor_vertex(value);
    if (value as u4) > u4:8 { floor + s20:16 } else { floor }
}

fn edge_coefficients(v0x: s20, v0y: s20, v1x: s20, v1y: s20, reverse: u1) -> uN[120] {
    let a_unadjusted = (((v0y as s21) - (v1y as s21)) as s40) << u8:4;
    let b_unadjusted = (((v1x as s21) - (v0x as s21)) as s40) << u8:4;
    let c_unadjusted = ((v0x as s40) * (v1y as s40) - (v1x as s40) * (v0y as s40)) as s40;
    let a = if reverse { -a_unadjusted } else { a_unadjusted };
    let b = if reverse { -b_unadjusted } else { b_unadjusted };
    let c = if reverse { -c_unadjusted } else { c_unadjusted };
    let top_left = a > s40:0 || (a == s40:0 && b > s40:0);
    let biased_c = if top_left { c } else { c - s40:1 };
    (a as u40) ++ (b as u40) ++ (biased_c as u40)
}

fn edge_start(coefficients: uN[120], x_center: s20, y_center: s20) -> s40 {
    let a = coefficients[80+:u40] as s40;
    let b = coefficients[40+:u40] as s40;
    let c = coefficients[0+:u40] as s40;
    let products = ((a as sN[60]) * (x_center as sN[60])) + ((b as sN[60]) * (y_center as sN[60]));
    ((products >> u8:4) + (c as sN[60])) as s40
}

pub fn triangle_setup
    (ax: s20, ay: s20, bx: s20, by: s20, cx: s20, cy: s20, reverse: u1) -> uN[560] {
    let minimum_x =
        if ax < bx { if ax < cx { ax } else { cx } } else { if bx < cx { bx } else { cx } };
    let maximum_x =
        if ax > bx { if ax > cx { ax } else { cx } } else { if bx > cx { bx } else { cx } };
    let x0 = floor_vertex(minimum_x);
    let x1 = ceil_vertex(maximum_x);
    let y0 = round_half_down_vertex(ay);
    let y1 = round_half_down_vertex(cy);
    let edge0 = edge_coefficients(ax, ay, bx, by, reverse);
    let edge1 = edge_coefficients(bx, by, cx, cy, reverse);
    let edge2 = edge_coefficients(cx, cy, ax, ay, reverse);
    let x_center = x0 + s20:9;
    let y_center = y0 + s20:8;
    let start0 = edge_start(edge0, x_center, y_center);
    let start1 = edge_start(edge1, x_center, y_center);
    let start2 = edge_start(edge2, x_center, y_center);
    (x0 as u20) ++ (x1 as u20) ++ (y0 as u20) ++ (y1 as u20) ++ edge0 ++ edge1 ++ edge2 ++
    (start0 as u40) ++ (start1 as u40) ++ (start2 as u40)
}

pub fn span_step(current: s48, dx: s48, dy: s48, mode: u2) -> s48 {
    match mode {
        u2:0 => current + dx,
        u2:1 => current - dx,
        u2:2 => current + dy,
        _ => current + dy - dx,
    }
}

#[test]
fn triangle_iter_test() {
    let loaded = triangle_iter_load(s48:100, s48:16, s48:-8, u4:8, u4:8, u1:1, u1:0, u1:1);
    assert_eq(loaded[132+:uN[66]] as sN[66], sN[66]:104);
    assert_eq(loaded[66+:uN[66]] as sN[66], sN[66]:16);
    assert_eq(loaded[0+:uN[66]] as sN[66], sN[66]:8);
}

#[test]
fn triangle_bounds_test() {
    let setup = triangle_setup(s20:0x18, s20:0x18, s20:0x58, s20:0x18, s20:0x18, s20:0x58, u1:0);
    assert_eq(setup[540+:u20] as s20, s20:0x10);
    assert_eq(setup[520+:u20] as s20, s20:0x60);
}

#[test]
fn triangle_coverage_uses_voodoo_horizontal_sample_offset() {
    let setup = triangle_setup(s20:3072, s20:2304, s20:8192, s20:3072, s20:5120, s20:6144, u1:0);
    assert_eq(setup[0+:u40] as s40, s40:18176);
}
