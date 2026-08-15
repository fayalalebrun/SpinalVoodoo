import raster;

pub struct RasterState {
    active: u1,
    x0: s20,
    x1: s20,
    y1: s20,
    x: s20,
    y: s20,
    edge0: s40,
    edge1: s40,
    edge2: s40,
    row_edge0: s40,
    row_edge1: s40,
    row_edge2: s40,
    a0: s40,
    a1: s40,
    a2: s40,
    b0: s40,
    b1: s40,
    b2: s40,
}

pub struct PixelCandidate { valid: u1, x: s16, y: s16 }

pub fn initial_state() -> RasterState {
    RasterState {
        active: u1:0,
        x0: s20:0,
        x1: s20:0,
        y1: s20:0,
        x: s20:0,
        y: s20:0,
        edge0: s40:0,
        edge1: s40:0,
        edge2: s40:0,
        row_edge0: s40:0,
        row_edge1: s40:0,
        row_edge2: s40:0,
        a0: s40:0,
        a1: s40:0,
        a2: s40:0,
        b0: s40:0,
        b1: s40:0,
        b2: s40:0,
    }
}

pub fn from_triangle_parts(bounds: uN[80], edges: uN[360], starts: uN[120]) -> RasterState {
    let x0 = bounds[60+:u20] as s20;
    let x1 = bounds[40+:u20] as s20;
    let y0 = bounds[20+:u20] as s20;
    let y1 = bounds[0+:u20] as s20;
    let edge0 = starts[80+:u40] as s40;
    let edge1 = starts[40+:u40] as s40;
    let edge2 = starts[0+:u40] as s40;
    RasterState {
        active: (x0 < x1 && y0 < y1) as u1,
        x0,
        x1,
        y1,
        x: x0,
        y: y0,
        edge0,
        edge1,
        edge2,
        row_edge0: edge0,
        row_edge1: edge1,
        row_edge2: edge2,
        a0: edges[320+:u40] as s40,
        a1: edges[200+:u40] as s40,
        a2: edges[80+:u40] as s40,
        b0: edges[280+:u40] as s40,
        b1: edges[160+:u40] as s40,
        b2: edges[40+:u40] as s40,
    }
}

pub fn from_triangle_setup(setup: uN[560]) -> RasterState {
    from_triangle_parts(setup[480+:uN[80]], setup[120+:uN[360]], setup[0+:uN[120]])
}

pub fn begin_triangle
    (ax: s20, ay: s20, bx: s20, by: s20, cx: s20, cy: s20, reverse: u1) -> RasterState {
    from_triangle_setup(raster::triangle_setup(ax, ay, bx, by, cx, cy, reverse))
}

pub fn tick(state: RasterState) -> (RasterState, PixelCandidate) {
    let inside = state.edge0 >= s40:0 && state.edge1 >= s40:0 && state.edge2 >= s40:0;
    let candidate = PixelCandidate {
        valid: (state.active == u1:1 && inside) as u1,
        x: (state.x >> u8:4) as s16,
        y: (state.y >> u8:4) as s16,
    };
    let end_row = state.x + s20:16 >= state.x1;
    let end_triangle = end_row && state.y + s20:16 >= state.y1;
    let row_edge0 = if end_row { state.row_edge0 + state.b0 } else { state.row_edge0 };
    let row_edge1 = if end_row { state.row_edge1 + state.b1 } else { state.row_edge1 };
    let row_edge2 = if end_row { state.row_edge2 + state.b2 } else { state.row_edge2 };
    let next_state = RasterState {
        active: if end_triangle { u1:0 } else { state.active },
        x: if end_row { state.x0 } else { state.x + s20:16 },
        y: if end_row { state.y + s20:16 } else { state.y },
        edge0: if end_row { row_edge0 } else { state.edge0 + state.a0 },
        edge1: if end_row { row_edge1 } else { state.edge1 + state.a1 },
        edge2: if end_row { row_edge2 } else { state.edge2 + state.a2 },
        row_edge0,
        row_edge1,
        row_edge2,
        ..state
    };
    (next_state, candidate)
}

fn first_nonnegative(edge: s40, step: s40, limit: u16) -> u16 {
    if edge >= s40:0 {
        u16:0
    } else if step <= s40:0 {
        limit
    } else {
        let distance = -(edge as s41);
        let count = ((distance + (step as s41) - s41:1) / (step as s41)) as u40;
        if count >= (limit as u40) { limit } else { count as u16 }
    }
}

pub fn tick_bounded(state: RasterState) -> (RasterState, PixelCandidate) {
    let inside = state.edge0 >= s40:0 && state.edge1 >= s40:0 && state.edge2 >= s40:0;
    let next_x = state.x + s20:16;
    let skipped_x = state.x + s20:32;
    let end_first = next_x >= state.x1;
    let end_second = skipped_x >= state.x1;
    let skip = state.active == u1:1 && !inside && !end_first;
    let candidate_edge0 = if skip { state.edge0 + state.a0 } else { state.edge0 };
    let candidate_edge1 = if skip { state.edge1 + state.a1 } else { state.edge1 };
    let candidate_edge2 = if skip { state.edge2 + state.a2 } else { state.edge2 };
    let candidate_inside = candidate_edge0 >= s40:0 && candidate_edge1 >= s40:0 &&
                           candidate_edge2 >= s40:0;
    let end_row = end_first || (skip && end_second);
    let end_triangle = end_row && state.y + s20:16 >= state.y1;
    let row_edge0 = if end_row { state.row_edge0 + state.b0 } else { state.row_edge0 };
    let row_edge1 = if end_row { state.row_edge1 + state.b1 } else { state.row_edge1 };
    let row_edge2 = if end_row { state.row_edge2 + state.b2 } else { state.row_edge2 };
    let step0 = if skip { state.a0 << u8:1 } else { state.a0 };
    let step1 = if skip { state.a1 << u8:1 } else { state.a1 };
    let step2 = if skip { state.a2 << u8:1 } else { state.a2 };
    let next_state = RasterState {
        active: if end_triangle { u1:0 } else { state.active },
        x: if end_row { state.x0 } else { if skip { skipped_x } else { next_x } },
        y: if end_row { state.y + s20:16 } else { state.y },
        edge0: if end_row { row_edge0 } else { state.edge0 + step0 },
        edge1: if end_row { row_edge1 } else { state.edge1 + step1 },
        edge2: if end_row { row_edge2 } else { state.edge2 + step2 },
        row_edge0,
        row_edge1,
        row_edge2,
        ..state
    };
    let candidate = PixelCandidate {
        valid: (state.active == u1:1 && candidate_inside) as u1,
        x: ((if skip { next_x } else { state.x }) >> u8:4) as s16,
        y: (state.y >> u8:4) as s16,
    };
    (next_state, candidate)
}

pub fn tick_visible(state: RasterState) -> (RasterState, PixelCandidate) {
    if state.active == u1:0 {
        tick(state)
    } else {
        let remaining = ((((state.x1 as s21) - (state.x as s21) + s21:15) >> u8:4) as u16);
        let offset0 = first_nonnegative(state.edge0, state.a0, remaining);
        let offset1 = first_nonnegative(state.edge1, state.a1, remaining);
        let offset2 = first_nonnegative(state.edge2, state.a2, remaining);
        let offset = if offset0 > offset1 { offset0 } else { offset1 };
        let offset = if offset > offset2 { offset } else { offset2 };
        let edge_offset = offset as s40;
        let edge0 = state.edge0 + state.a0 * edge_offset;
        let edge1 = state.edge1 + state.a1 * edge_offset;
        let edge2 = state.edge2 + state.a2 * edge_offset;
        let covered = offset < remaining && edge0 >= s40:0 && edge1 >= s40:0 && edge2 >= s40:0;
        let consumed = if covered { offset + u16:1 } else { remaining };
        let end_row = consumed >= remaining;
        let row_edge0 = if end_row { state.row_edge0 + state.b0 } else { state.row_edge0 };
        let row_edge1 = if end_row { state.row_edge1 + state.b1 } else { state.row_edge1 };
        let row_edge2 = if end_row { state.row_edge2 + state.b2 } else { state.row_edge2 };
        let end_triangle = end_row && state.y + s20:16 >= state.y1;
        let next_state = RasterState {
            active: if end_triangle { u1:0 } else { state.active },
            x: if end_row { state.x0 } else { state.x + ((consumed as s20) << u8:4) },
            y: if end_row { state.y + s20:16 } else { state.y },
            edge0: if end_row { row_edge0 } else { state.edge0 + state.a0 * (consumed as s40) },
            edge1: if end_row { row_edge1 } else { state.edge1 + state.a1 * (consumed as s40) },
            edge2: if end_row { row_edge2 } else { state.edge2 + state.a2 * (consumed as s40) },
            row_edge0,
            row_edge1,
            row_edge2,
            ..state
        };
        let candidate = PixelCandidate {
            valid: covered as u1,
            x: ((state.x + ((offset as s20) << u8:4)) >> u8:4) as s16,
            y: (state.y >> u8:4) as s16,
        };
        (next_state, candidate)
    }
}

#[test]
fn bounded_tick_checks_two_candidates() {
    let state = RasterState {
        active: u1:1,
        x0: s20:0,
        x1: s20:64,
        y1: s20:16,
        x: s20:0,
        y: s20:0,
        edge0: s40:-2,
        edge1: s40:0,
        edge2: s40:0,
        row_edge0: s40:-2,
        row_edge1: s40:0,
        row_edge2: s40:0,
        a0: s40:1,
        a1: s40:0,
        a2: s40:0,
        b0: s40:0,
        b1: s40:0,
        b2: s40:0,
    };
    let (state, candidate) = tick_bounded(state);
    assert_eq(candidate.valid, u1:0);
    assert_eq(state.x, s20:32);
    let (state, candidate) = tick_bounded(state);
    assert_eq(candidate, PixelCandidate { valid: u1:1, x: s16:2, y: s16:0 });
    assert_eq(state.x, s20:48);
}

#[test]
fn visible_tick_skips_rejected_candidates() {
    let state = RasterState {
        active: u1:1,
        x0: s20:0,
        x1: s20:64,
        y1: s20:16,
        x: s20:0,
        y: s20:0,
        edge0: s40:-2,
        edge1: s40:0,
        edge2: s40:0,
        row_edge0: s40:-2,
        row_edge1: s40:0,
        row_edge2: s40:0,
        a0: s40:1,
        a1: s40:0,
        a2: s40:0,
        b0: s40:0,
        b1: s40:0,
        b2: s40:0,
    };
    let (state, candidate) = tick_visible(state);
    assert_eq(candidate, PixelCandidate { valid: u1:1, x: s16:2, y: s16:0 });
    assert_eq(state.x, s20:48);
}

#[test]
fn visible_tick_skips_empty_row() {
    let state = RasterState {
        active: u1:1,
        x0: s20:0,
        x1: s20:64,
        y1: s20:32,
        x: s20:0,
        y: s20:0,
        edge0: s40:-1,
        edge1: s40:0,
        edge2: s40:0,
        row_edge0: s40:-1,
        row_edge1: s40:0,
        row_edge2: s40:0,
        a0: s40:0,
        a1: s40:0,
        a2: s40:0,
        b0: s40:1,
        b1: s40:0,
        b2: s40:0,
    };
    let (state, candidate) = tick_visible(state);
    assert_eq(candidate.valid, u1:0);
    assert_eq(state.x, s20:0);
    assert_eq(state.y, s20:16);
    assert_eq(state.edge0, s40:0);
}

#[test]
fn raster_walks_triangle_bounds() {
    let state = begin_triangle(s20:0x08, s20:0x08, s20:0x38, s20:0x08, s20:0x08, s20:0x38, u1:0);
    let (state, p0) = tick(state);
    let (state, p1) = tick(state);
    let (state, p2) = tick(state);
    let (state, p3) = tick(state);
    assert_eq(p0.x, s16:0);
    assert_eq(p1.x, s16:1);
    assert_eq(p2.x, s16:2);
    assert_eq(p3.x, s16:3);
    assert_eq(state.y, s20:0x10);
}

#[test]
fn raster_eventually_becomes_idle() {
    let state = begin_triangle(s20:0x08, s20:0x08, s20:0x28, s20:0x08, s20:0x08, s20:0x28, u1:0);
    let state = for (_, state): (u6, RasterState) in u6:0..u6:16 {
        let (state, _) = tick(state);
        state
    }(state);
    assert_eq(state.active, u1:0);
}
