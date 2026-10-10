fn parameter_delta(vertex: s20, pixel: s16, adjust: u1) -> s21 {
    let floor = vertex >> u8:4;
    let anchor = if (vertex as u4) > u4:8 { floor + s20:1 } else { floor };
    if adjust == u1:1 {
        ((((pixel as s20) << u8:4) + s20:8) as s21) - (vertex as s21)
    } else {
        ((pixel as s21) - (anchor as s21)) << u8:4
    }
}

pub fn at_pixel
    (start: s48, dx: s48, dy: s48, vertex_x: s20, vertex_y: s20, pixel_x: s16, pixel_y: s16,
     adjust: u1) -> s48 {
    let delta_x = parameter_delta(vertex_x, pixel_x, adjust);
    let delta_y = parameter_delta(vertex_y, pixel_y, adjust);
    let correction = (((dx as sN[69]) * (delta_x as sN[69])) +
                     ((dy as sN[69]) * (delta_y as sN[69]))) >> u8:4;
    ((start as sN[69]) + correction) as s48
}

pub fn at_pixel_high
    (start: s50, dx: s50, dy: s50, vertex_x: s20, vertex_y: s20, pixel_x: s16, pixel_y: s16,
     adjust: u1) -> s50 {
    let delta_x = parameter_delta(vertex_x, pixel_x, adjust);
    let delta_y = parameter_delta(vertex_y, pixel_y, adjust);
    let correction = (((dx as sN[71]) * (delta_x as sN[71])) +
                     ((dy as sN[71]) * (delta_y as sN[71]))) >> u8:4;
    ((start as sN[71]) + correction) as s50
}

#[test]
fn interpolates_at_pixel_centers() {
    assert_eq(
        at_pixel(s48:100, s48:16, s48:32, s20:8, s20:8, s16:0, s16:0, u1:1), s48:100);
    assert_eq(
        at_pixel(s48:100, s48:16, s48:32, s20:8, s20:8, s16:2, s16:3, u1:1), s48:228);
}

#[test]
fn interpolates_negative_vertex_offsets() {
    assert_eq(
        at_pixel(s48:100, s48:16, s48:32, s20:24, s20:24, s16:0, s16:0, u1:1), s48:52);
}

#[test]
fn combines_subpixel_terms_before_truncating() {
    assert_eq(
        at_pixel(s48:100, s48:1, s48:1, s20:9, s20:9, s16:0, s16:0, u1:1), s48:99);
}
