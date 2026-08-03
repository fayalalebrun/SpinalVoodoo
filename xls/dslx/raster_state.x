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

pub fn begin_triangle
    (ax: s20, ay: s20, bx: s20, by: s20, cx: s20, cy: s20, reverse: u1) -> RasterState {
    let setup = raster::triangle_setup(ax, ay, bx, by, cx, cy, reverse);
    let x0 = setup[540+:u20] as s20;
    let x1 = setup[520+:u20] as s20;
    let y0 = setup[500+:u20] as s20;
    let y1 = setup[480+:u20] as s20;
    let edge0 = setup[80+:u40] as s40;
    let edge1 = setup[40+:u40] as s40;
    let edge2 = setup[0+:u40] as s40;
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
        a0: setup[440+:u40] as s40,
        a1: setup[320+:u40] as s40,
        a2: setup[200+:u40] as s40,
        b0: setup[400+:u40] as s40,
        b1: setup[280+:u40] as s40,
        b2: setup[160+:u40] as s40,
    }
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
