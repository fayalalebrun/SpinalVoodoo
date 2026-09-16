import raster;

pub struct RasterState {
    active: u1,
    row_prefetched: u1,
    scan_found: u1,
    scan_start: s16,
    scan_end: s16,
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

pub struct RowSpan { complete: u1, valid: u1, x_start: s16, x_end: s16, y: s16 }

pub struct SpanScanState {
    active: u1,
    scan_found: u1,
    scan_start: s16,
    scan_end: s16,
    x: s20,
    y: s20,
    edge0: s40,
    edge1: s40,
    edge2: s40,
    row_edge0: s40,
    row_edge1: s40,
    row_edge2: s40,
}

pub fn initial_state() -> RasterState {
    RasterState {
        active: u1:0,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
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
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
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

pub fn install_span(state: RasterState, span: RowSpan) -> RasterState {
    RasterState {
        row_prefetched: span.complete,
        scan_found: span.valid,
        scan_start: span.x_start,
        scan_end: span.x_end,
        x: (span.x_start as s20) << u8:4,
        y: (span.y as s20) << u8:4,
        ..state
    }
}

pub fn tick_prefetched(state: RasterState) -> (RasterState, PixelCandidate) {
    let ready = state.active == u1:1 && state.row_prefetched == u1:1;
    let candidate = PixelCandidate {
        valid: ready && state.scan_found == u1:1,
        x: (state.x >> u8:4) as s16,
        y: (state.y >> u8:4) as s16,
    };
    let end_row = state.scan_found == u1:0 || candidate.x == state.scan_end;
    let next_state = if ready {
        RasterState {
            active: !(end_row && state.y + s20:16 >= state.y1),
            row_prefetched: !end_row,
            scan_found: state.scan_found && !end_row,
            x: state.x + s20:16,
            y: if end_row { state.y + s20:16 } else { state.y },
            ..state
        }
    } else {
        state
    };
    (next_state, candidate)
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
        row_prefetched: if end_row { u1:0 } else { state.row_prefetched },
        scan_found: if end_row { u1:0 } else { state.scan_found },
        scan_start: if end_row { s16:0 } else { state.scan_start },
        scan_end: if end_row { s16:0 } else { state.scan_end },
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

fn bounded_step(step: s40, count: u5) -> s40 {
    match count {
        u5:0 => s40:0,
        u5:1 => step,
        u5:2 => step << u8:1,
        u5:3 => (step << u8:1) + step,
        u5:4 => step << u8:2,
        u5:5 => (step << u8:2) + step,
        u5:6 => (step << u8:2) + (step << u8:1),
        u5:7 => (step << u8:3) - step,
        u5:8 => step << u8:3,
        u5:9 => (step << u8:3) + step,
        u5:10 => (step << u8:3) + (step << u8:1),
        u5:11 => (step << u8:3) + (step << u8:1) + step,
        u5:12 => (step << u8:3) + (step << u8:2),
        u5:13 => (step << u8:3) + (step << u8:2) + step,
        u5:14 => (step << u8:4) - (step << u8:1),
        u5:15 => (step << u8:4) - step,
        _ => step << u8:4,
    }
}

pub fn span_scan_state(raster: RasterState) -> SpanScanState {
    SpanScanState {
        active: raster.active,
        scan_found: raster.scan_found,
        scan_start: raster.scan_start,
        scan_end: raster.scan_end,
        x: raster.x,
        y: raster.y,
        edge0: raster.edge0,
        edge1: raster.edge1,
        edge2: raster.edge2,
        row_edge0: raster.row_edge0,
        row_edge1: raster.row_edge1,
        row_edge2: raster.row_edge2,
    }
}

fn span_scan_raster(state: SpanScanState, geometry: RasterState) -> RasterState {
    RasterState {
        active: state.active,
        row_prefetched: u1:0,
        scan_found: state.scan_found,
        scan_start: state.scan_start,
        scan_end: state.scan_end,
        x: state.x,
        y: state.y,
        edge0: state.edge0,
        edge1: state.edge1,
        edge2: state.edge2,
        row_edge0: state.row_edge0,
        row_edge1: state.row_edge1,
        row_edge2: state.row_edge2,
        ..geometry
    }
}

pub fn scan_row_block(state: RasterState) -> (RasterState, RowSpan) {
    let remaining = ((((state.x1 as s21) - (state.x as s21) + s21:15) >> u8:4) as u16);
    let block_count = if remaining == u16:0 { u5:0 } else { u5:1 };
    let block_found =
        block_count == u5:1 && state.edge0 >= s40:0 && state.edge1 >= s40:0 && state.edge2 >= s40:0;
    let block_start = (state.x >> u8:4) as s16;
    let block_end = block_start;
    let found = state.scan_found == u1:1 || block_found;
    let span_start = if state.scan_found == u1:1 { state.scan_start } else { block_start };
    let span_end = if block_found { block_end } else { state.scan_end };
    let complete = block_count as u16 >= remaining;
    let advanced_edge0 = if block_count == u5:0 { state.edge0 } else { state.edge0 + state.a0 };
    let advanced_edge1 = if block_count == u5:0 { state.edge1 } else { state.edge1 + state.a1 };
    let advanced_edge2 = if block_count == u5:0 { state.edge2 } else { state.edge2 + state.a2 };
    let next_state = RasterState {
        row_prefetched: complete as u1,
        scan_found: if complete { u1:0 } else { found as u1 },
        scan_start: if complete { s16:0 } else { span_start },
        scan_end: if complete { s16:0 } else { span_end },
        x: if complete { state.x0 } else { state.x + ((block_count as s20) << u8:4) },
        edge0: if complete { state.row_edge0 } else { advanced_edge0 },
        edge1: if complete { state.row_edge1 } else { advanced_edge1 },
        edge2: if complete { state.row_edge2 } else { advanced_edge2 },
        ..state
    };
    let span = RowSpan {
        complete: complete as u1,
        valid: (complete && found) as u1,
        x_start: span_start,
        x_end: span_end,
        y: (state.y >> u8:4) as s16,
    };
    (next_state, span)
}

pub fn advance_row(state: RasterState) -> RasterState {
    let row_edge0 = state.row_edge0 + state.b0;
    let row_edge1 = state.row_edge1 + state.b1;
    let row_edge2 = state.row_edge2 + state.b2;
    RasterState {
        active: (state.active == u1:1 && state.y + s20:16 < state.y1) as u1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x: state.x0,
        y: state.y + s20:16,
        edge0: row_edge0,
        edge1: row_edge1,
        edge2: row_edge2,
        row_edge0,
        row_edge1,
        row_edge2,
        ..state
    }
}

fn finish_coherent_span_row(state: RasterState, valid: u1) -> (RasterState, u3, RowSpan) {
    let base_x = if valid == u1:1 { (state.scan_start as s20) << u8:4 } else { state.x };
    let base_edge0 = if valid == u1:1 { state.row_edge0 } else { state.edge0 };
    let base_edge1 = if valid == u1:1 { state.row_edge1 } else { state.edge1 };
    let base_edge2 = if valid == u1:1 { state.row_edge2 } else { state.edge2 };
    let next_edge0 = base_edge0 + state.b0;
    let next_edge1 = base_edge1 + state.b1;
    let next_edge2 = base_edge2 + state.b2;
    let next_state = RasterState {
        active: (state.active == u1:1 && state.y + s20:16 < state.y1) as u1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x: base_x,
        y: state.y + s20:16,
        edge0: next_edge0,
        edge1: next_edge1,
        edge2: next_edge2,
        row_edge0: next_edge0,
        row_edge1: next_edge1,
        row_edge2: next_edge2,
        ..state
    };
    (
        next_state, u3:0,
        RowSpan {
            complete: u1:1,
            valid,
            x_start: state.scan_start,
            x_end: state.scan_end,
            y: (state.y >> u8:4) as s16,
        },
    )
}

pub fn scan_row_coherent(state: RasterState, phase: u3) -> (RasterState, u3, RowSpan) {
    let inside = state.edge0 >= s40:0 && state.edge1 >= s40:0 && state.edge2 >= s40:0;
    let at_left = state.x <= state.x0;
    let at_right = state.x + s20:16 >= state.x1;
    let empty = RowSpan {
        complete: u1:0,
        valid: u1:0,
        x_start: s16:0,
        x_end: s16:0,
        y: (state.y >> u8:4) as s16,
    };
    if phase == u3:0 {
        let impossible = (state.edge0 < s40:0 && state.a0 == s40:0) ||
                         (state.edge1 < s40:0 && state.a1 == s40:0) ||
                         (state.edge2 < s40:0 && state.a2 == s40:0);
        let need_left = (state.edge0 < s40:0 && state.a0 < s40:0) ||
                        (state.edge1 < s40:0 && state.a1 < s40:0) ||
                        (state.edge2 < s40:0 && state.a2 < s40:0);
        let need_right = (state.edge0 < s40:0 && state.a0 > s40:0) ||
                         (state.edge1 < s40:0 && state.a1 > s40:0) ||
                         (state.edge2 < s40:0 && state.a2 > s40:0);
        if impossible || (need_left && need_right) {
            finish_coherent_span_row(state, u1:0)
        } else if inside || need_left {
            (state, u3:1, empty)
        } else {
            (state, u3:2, empty)
        }
    } else if phase == u3:1 {
        if inside {
            let state = RasterState {
                scan_found: u1:1,
                scan_start: (state.x >> u8:4) as s16,
                row_edge0: state.edge0,
                row_edge1: state.edge1,
                row_edge2: state.edge2,
                ..state
            };
            if at_left {
                (state, u3:3, empty)
            } else {
                (
                    RasterState {
                        x: state.x - s20:16,
                        edge0: state.edge0 - state.a0,
                        edge1: state.edge1 - state.a1,
                        edge2: state.edge2 - state.a2,
                        ..state
                    }, u3:1, empty,
                )
            }
        } else if state.scan_found == u1:1 {
            (
                RasterState {
                    x: (state.scan_start as s20) << u8:4,
                    edge0: state.row_edge0,
                    edge1: state.row_edge1,
                    edge2: state.row_edge2,
                    ..state
                }, u3:3, empty,
            )
        } else if at_left {
            finish_coherent_span_row(state, u1:0)
        } else {
            (
                RasterState {
                    x: state.x - s20:16,
                    edge0: state.edge0 - state.a0,
                    edge1: state.edge1 - state.a1,
                    edge2: state.edge2 - state.a2,
                    ..state
                }, u3:1, empty,
            )
        }
    } else if phase == u3:2 {
        if inside {
            (
                RasterState {
                    scan_found: u1:1,
                    scan_start: (state.x >> u8:4) as s16,
                    row_edge0: state.edge0,
                    row_edge1: state.edge1,
                    row_edge2: state.edge2,
                    ..state
                }, u3:3, empty,
            )
        } else if at_right {
            finish_coherent_span_row(state, u1:0)
        } else {
            (
                RasterState {
                    x: state.x + s20:16,
                    edge0: state.edge0 + state.a0,
                    edge1: state.edge1 + state.a1,
                    edge2: state.edge2 + state.a2,
                    ..state
                }, u3:2, empty,
            )
        }
    } else if inside {
        let state = RasterState { scan_end: (state.x >> u8:4) as s16, ..state };
        if at_right {
            finish_coherent_span_row(state, u1:1)
        } else {
            (
                RasterState {
                    x: state.x + s20:16,
                    edge0: state.edge0 + state.a0,
                    edge1: state.edge1 + state.a1,
                    edge2: state.edge2 + state.a2,
                    ..state
                }, u3:3, empty,
            )
        }
    } else {
        finish_coherent_span_row(state, u1:1)
    }
}

pub fn scan_span_row_block
    (state: SpanScanState, geometry: RasterState) -> (SpanScanState, RowSpan) {
    let (raster, span) = scan_row_block(span_scan_raster(state, geometry));
    (span_scan_state(raster), span)
}

pub fn advance_span_row(state: SpanScanState, geometry: RasterState) -> SpanScanState {
    span_scan_state(advance_row(span_scan_raster(state, geometry)))
}

pub fn tick_bounded(state: RasterState) -> (RasterState, PixelCandidate) {
    if state.active == u1:0 {
        tick(state)
    } else {
        let remaining = ((((state.x1 as s21) - (state.x as s21) + s21:15) >> u8:4) as u16);
        let candidates = for (i, candidates): (u5, u16) in u5:0..u5:16 {
            let edge0 = state.edge0 + bounded_step(state.a0, i);
            let edge1 = state.edge1 + bounded_step(state.a1, i);
            let edge2 = state.edge2 + bounded_step(state.a2, i);
            let inside =
                (i as u16) < remaining && edge0 >= s40:0 && edge1 >= s40:0 && edge2 >= s40:0;
            candidates | ((inside as u16) << i)
        }(u16:0);
        let coarse0 = state.edge0 < s40:0 && state.a0 > s40:0 &&
                      (state.edge0 as s41) + ((state.a0 as s41) << u8:6) < s41:0;
        let coarse1 = state.edge1 < s40:0 && state.a1 > s40:0 &&
                      (state.edge1 as s41) + ((state.a1 as s41) << u8:6) < s41:0;
        let coarse2 = state.edge2 < s40:0 && state.a2 > s40:0 &&
                      (state.edge2 as s41) + ((state.a2 as s41) << u8:6) < s41:0;
        let coarse_skip = remaining >= u16:64 && (coarse0 || coarse1 || coarse2);
        let covered = candidates != u16:0 && !coarse_skip;
        let cannot_recover = (state.edge0 < s40:0 && state.a0 <= s40:0) ||
                             (state.edge1 < s40:0 && state.a1 <= s40:0) ||
                             (state.edge2 < s40:0 && state.a2 <= s40:0);
        let offset = if covered { ctz(candidates) as u5 } else { u5:0 };
        let scanned = if covered {
            offset + u5:1
        } else if remaining < u16:16 {
            remaining as u5
        } else {
            u5:16
        };
        let consumed = if coarse_skip { u16:64 } else { scanned as u16 };
        let consumed_edge0 = if coarse_skip {
            ((state.edge0 as s41) + ((state.a0 as s41) << u8:6)) as s40
        } else {
            state.edge0 + bounded_step(state.a0, scanned)
        };
        let consumed_edge1 = if coarse_skip {
            ((state.edge1 as s41) + ((state.a1 as s41) << u8:6)) as s40
        } else {
            state.edge1 + bounded_step(state.a1, scanned)
        };
        let consumed_edge2 = if coarse_skip {
            ((state.edge2 as s41) + ((state.a2 as s41) << u8:6)) as s40
        } else {
            state.edge2 + bounded_step(state.a2, scanned)
        };
        let cannot_recover_after = (consumed_edge0 < s40:0 && state.a0 <= s40:0) ||
                                   (consumed_edge1 < s40:0 && state.a1 <= s40:0) ||
                                   (consumed_edge2 < s40:0 && state.a2 <= s40:0);
        let end_row = cannot_recover || cannot_recover_after || consumed >= remaining;
        let end_triangle = end_row && state.y + s20:16 >= state.y1;
        let row_edge0 = if end_row { state.row_edge0 + state.b0 } else { state.row_edge0 };
        let row_edge1 = if end_row { state.row_edge1 + state.b1 } else { state.row_edge1 };
        let row_edge2 = if end_row { state.row_edge2 + state.b2 } else { state.row_edge2 };
        let next_state = RasterState {
            active: if end_triangle { u1:0 } else { state.active },
            row_prefetched: if end_row { u1:0 } else { state.row_prefetched },
            scan_found: if end_row { u1:0 } else { state.scan_found },
            scan_start: if end_row { s16:0 } else { state.scan_start },
            scan_end: if end_row { s16:0 } else { state.scan_end },
            x: if end_row { state.x0 } else { state.x + ((consumed as s20) << u8:4) },
            y: if end_row { state.y + s20:16 } else { state.y },
            edge0: if end_row { row_edge0 } else { consumed_edge0 },
            edge1: if end_row { row_edge1 } else { consumed_edge1 },
            edge2: if end_row { row_edge2 } else { consumed_edge2 },
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
            row_prefetched: if end_row { u1:0 } else { state.row_prefetched },
            scan_found: if end_row { u1:0 } else { state.scan_found },
            scan_start: if end_row { s16:0 } else { state.scan_start },
            scan_end: if end_row { s16:0 } else { state.scan_end },
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
fn prefetched_tick_waits_for_completed_row() {
    let state = RasterState { active: u1:1, x1: s20:64, y1: s20:32, ..initial_state() };
    let (next, candidate) = tick_prefetched(state);
    assert_eq(next, state);
    assert_eq(candidate.valid, u1:0);
    let span = RowSpan { complete: u1:0, valid: u1:1, x_start: s16:0, x_end: s16:3, y: s16:0 };
    let state = install_span(state, span);
    let (next, candidate) = tick_prefetched(state);
    assert_eq(next, state);
    assert_eq(candidate.valid, u1:0);
    let state = install_span(initial_state(), RowSpan { complete: u1:1, ..span });
    let (next, candidate) = tick_prefetched(state);
    assert_eq(next, state);
    assert_eq(candidate.valid, u1:0);
}

#[test]
fn prefetched_tick_completes_empty_and_single_pixel_rows() {
    let state = RasterState { active: u1:1, y1: s20:32, ..initial_state() };
    let empty = RowSpan { complete: u1:1, valid: u1:0, x_start: s16:7, x_end: s16:-3, y: s16:0 };
    let (state, candidate) = tick_prefetched(install_span(state, empty));
    assert_eq(candidate.valid, u1:0);
    assert_eq((state.active, state.row_prefetched, state.y), (u1:1, u1:0, s20:16));
    let last = RowSpan { complete: u1:1, valid: u1:1, x_start: s16:-3, x_end: s16:-3, y: s16:1 };
    let (state, candidate) = tick_prefetched(install_span(state, last));
    assert_eq(candidate, PixelCandidate { valid: u1:1, x: s16:-3, y: s16:1 });
    assert_eq((state.active, state.row_prefetched), (u1:0, u1:0));
    let (next, candidate) = tick_prefetched(state);
    assert_eq(next, state);
    assert_eq(candidate.valid, u1:0);
    let state = RasterState { active: u1:1, y1: s20:16, ..initial_state() };
    let (state, candidate) = tick_prefetched(install_span(state, empty));
    assert_eq((state.active, candidate.valid), (u1:0, u1:0));
}

#[quickcheck(test_count=128)]
fn prefetched_pixels_match_triangle_walk(vertices: s8[6], reverse: u1) -> bool {
    let initial = begin_triangle(
        vertices[u32:0] as s20, vertices[u32:1] as s20, vertices[u32:2] as s20,
        vertices[u32:3] as s20, vertices[u32:4] as s20, vertices[u32:5] as s20, reverse);
    let (_, _, _, matches) =
        for (_, accum): (u5, (RasterState, RasterState, u3, bool)) in u5:0..u5:18 {
            let (reference, scanner, phase, matches) = accum;
            if reference.active == u1:0 {
                (reference, scanner, phase, matches && scanner.active == u1:0)
            } else {
                let empty = RowSpan {
                    complete: u1:0,
                    valid: u1:0,
                    x_start: s16:0,
                    x_end: s16:0,
                    y: s16:0,
                };
                let (_, expected) = for (_, accum): (u5, (RasterState, RowSpan)) in u5:0..u5:18 {
                    if accum.1.complete == u1:1 { accum } else { scan_row_block(accum.0) }
                }((reference, empty));
                let (scanner, phase, span) =
                    for (_, accum): (u6, (RasterState, u3, RowSpan)) in u6:0..u6:48 {
                        if accum.2.complete == u1:1 {
                            accum
                        } else {
                            scan_row_coherent(accum.0, accum.1)
                        }
                    }((scanner, phase, empty));
                let matches = matches && span.complete == u1:1 && expected.complete == u1:1 &&
                              span.valid == expected.valid && span.y == expected.y &&
                              (span.valid == u1:0 ||
                              (span.x_start == expected.x_start && span.x_end == expected.x_end));
                let count = if expected.valid == u1:1 {
                    (expected.x_end as s17 - expected.x_start as s17 + s17:1) as u5
                } else {
                    u5:0
                };
                let (walk, matches) = for (i, accum): (u5, (RasterState, bool)) in u5:0..u5:18 {
                    let (walk, matches) = accum;
                    let (walk, candidate) = tick_prefetched(walk);
                    let valid = i < count;
                    let matches = matches && candidate.valid == valid &&
                                  (!valid ||
                                  (candidate.x == expected.x_start + (i as s16) &&
                                  candidate.y == expected.y));
                    (walk, matches)
                }((install_span(reference, span), matches));
                let next = advance_row(reference);
                (
                    next, scanner, phase,
                    matches && walk.active == next.active && walk.row_prefetched == u1:0,
                )
            }
        }((initial, initial, u3:0, true));
    matches
}

#[test]
fn bounded_tick_checks_sixteen_candidates() {
    let state = RasterState {
        active: u1:1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x0: s20:0,
        x1: s20:272,
        y1: s20:16,
        x: s20:0,
        y: s20:0,
        edge0: s40:-15,
        edge1: s40:0,
        edge2: s40:0,
        row_edge0: s40:-15,
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
    assert_eq(candidate, PixelCandidate { valid: u1:1, x: s16:15, y: s16:0 });
    assert_eq(state.x, s20:256);
}

#[test]
fn bounded_tick_coarsely_skips_distant_coverage() {
    let state = RasterState {
        active: u1:1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x0: s20:0,
        x1: s20:2048,
        y1: s20:16,
        x: s20:0,
        y: s20:0,
        edge0: s40:-100,
        edge1: s40:0,
        edge2: s40:0,
        row_edge0: s40:-100,
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
    assert_eq((state.x, state.edge0), (s20:1024, s40:-36));
}

#[test]
fn bounded_tick_finishes_row_with_last_covered_pixel() {
    let state = RasterState {
        active: u1:1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x0: s20:0,
        x1: s20:64,
        y1: s20:32,
        x: s20:0,
        y: s20:0,
        edge0: s40:0,
        edge1: s40:0,
        edge2: s40:0,
        row_edge0: s40:0,
        row_edge1: s40:0,
        row_edge2: s40:0,
        a0: s40:-1,
        a1: s40:0,
        a2: s40:0,
        b0: s40:0,
        b1: s40:0,
        b2: s40:0,
    };
    let (state, candidate) = tick_bounded(state);
    assert_eq(candidate, PixelCandidate { valid: u1:1, x: s16:0, y: s16:0 });
    assert_eq((state.x, state.y, state.active), (s20:0, s20:16, u1:1));
}

#[test]
fn bounded_tick_skips_unrecoverable_row() {
    let state = RasterState {
        active: u1:1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x0: s20:0,
        x1: s20:1024,
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
        b0: s40:0,
        b1: s40:0,
        b2: s40:0,
    };
    let (state, candidate) = tick_bounded(state);
    assert_eq(candidate.valid, u1:0);
    assert_eq((state.x, state.y, state.active), (s20:0, s20:16, u1:1));
}

#[test]
fn visible_tick_skips_rejected_candidates() {
    let state = RasterState {
        active: u1:1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
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
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
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
fn span_scan_completes_before_pixel_walk() {
    let state = RasterState {
        active: u1:1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x0: s20:0,
        x1: s20:64,
        y1: s20:16,
        x: s20:0,
        y: s20:0,
        edge0: s40:-1,
        edge1: s40:0,
        edge2: s40:0,
        row_edge0: s40:-1,
        row_edge1: s40:0,
        row_edge2: s40:0,
        a0: s40:1,
        a1: s40:0,
        a2: s40:0,
        b0: s40:0,
        b1: s40:0,
        b2: s40:0,
    };
    let (state, span) = for (_, accum): (u3, (RasterState, RowSpan)) in u3:0..u3:4 {
        let (state, span) = accum;
        if span.complete == u1:1 { accum } else { scan_row_block(state) }
    }((state, RowSpan { complete: u1:0, valid: u1:0, x_start: s16:0, x_end: s16:0, y: s16:0 }));
    assert_eq(span, RowSpan { complete: u1:1, valid: u1:1, x_start: s16:1, x_end: s16:3, y: s16:0 });
    assert_eq((state.row_prefetched, state.x, state.edge0), (u1:1, s20:0, s40:-1));
}

#[test]
fn span_scan_accumulates_multiple_blocks() {
    let state = RasterState {
        active: u1:1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x0: s20:0,
        x1: s20:320,
        y1: s20:16,
        x: s20:0,
        y: s20:0,
        edge0: s40:0,
        edge1: s40:19,
        edge2: s40:0,
        row_edge0: s40:0,
        row_edge1: s40:19,
        row_edge2: s40:0,
        a0: s40:0,
        a1: s40:-1,
        a2: s40:0,
        b0: s40:0,
        b1: s40:0,
        b2: s40:0,
    };
    let (_, span) = for (_, accum): (u5, (RasterState, RowSpan)) in u5:0..u5:20 {
        let (state, span) = accum;
        if span.complete == u1:1 { accum } else { scan_row_block(state) }
    }((state, RowSpan { complete: u1:0, valid: u1:0, x_start: s16:0, x_end: s16:0, y: s16:0 }));
    assert_eq(
        span, RowSpan { complete: u1:1, valid: u1:1, x_start: s16:0, x_end: s16:19, y: s16:0 });
}

#[test]
fn coherent_span_scan_reuses_previous_left_edge() {
    let state = RasterState {
        active: u1:1,
        row_prefetched: u1:0,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x0: s20:0,
        x1: s20:80,
        y1: s20:32,
        x: s20:0,
        y: s20:0,
        edge0: s40:-1,
        edge1: s40:3,
        edge2: s40:0,
        row_edge0: s40:-1,
        row_edge1: s40:3,
        row_edge2: s40:0,
        a0: s40:1,
        a1: s40:-1,
        a2: s40:0,
        b0: s40:0,
        b1: s40:0,
        b2: s40:0,
    };
    let (state, phase, span) = for (_, accum): (u4, (RasterState, u3, RowSpan)) in u4:0..u4:10 {
        let (state, phase, span) = accum;
        if span.complete == u1:1 { accum } else { scan_row_coherent(state, phase) }
    }((
        state, u3:0,
        RowSpan { complete: u1:0, valid: u1:0, x_start: s16:0, x_end: s16:0, y: s16:0 },
    ));
    assert_eq(span, RowSpan { complete: u1:1, valid: u1:1, x_start: s16:1, x_end: s16:3, y: s16:0 });
    let (_, _, next_span) = for (_, accum): (u4, (RasterState, u3, RowSpan)) in u4:0..u4:10 {
        let (state, phase, span) = accum;
        if span.complete == u1:1 { accum } else { scan_row_coherent(state, phase) }
    }((
        state, phase,
        RowSpan { complete: u1:0, valid: u1:0, x_start: s16:0, x_end: s16:0, y: s16:0 },
    ));
    assert_eq(
        next_span, RowSpan { complete: u1:1, valid: u1:1, x_start: s16:1, x_end: s16:3, y: s16:1 });
}

#[test]
fn coherent_span_scan_completes_empty_row() {
    let state = RasterState {
        edge0: s40:-1,
        a0: s40:0,
        active: u1:1,
        x1: s20:16,
        y1: s20:16,
        ..initial_state()
    };
    let (state, phase, span) = scan_row_coherent(state, u3:0);
    assert_eq((state.active, phase), (u1:0, u3:0));
    assert_eq(span, RowSpan { complete: u1:1, valid: u1:0, x_start: s16:0, x_end: s16:0, y: s16:0 });
}

#[test]
fn span_scan_advances_to_next_row() {
    let state = RasterState {
        active: u1:1,
        row_prefetched: u1:1,
        scan_found: u1:0,
        scan_start: s16:0,
        scan_end: s16:0,
        x0: s20:0,
        x1: s20:64,
        y1: s20:32,
        x: s20:0,
        y: s20:0,
        edge0: s40:3,
        edge1: s40:4,
        edge2: s40:5,
        row_edge0: s40:3,
        row_edge1: s40:4,
        row_edge2: s40:5,
        a0: s40:1,
        a1: s40:2,
        a2: s40:3,
        b0: s40:10,
        b1: s40:20,
        b2: s40:30,
    };
    let state = advance_row(state);
    assert_eq((state.active, state.row_prefetched, state.x, state.y), (u1:1, u1:0, s20:0, s20:16));
    assert_eq((state.edge0, state.edge1, state.edge2), (s40:13, s40:24, s40:35));
    assert_eq(advance_row(state).active, u1:0);
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
