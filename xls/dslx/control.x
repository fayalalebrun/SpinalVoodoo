pub struct QueuedWrite { address: u24, data: u32, mask: u4 }

type PciEntries = QueuedWrite[64];

pub struct PciFifoState { entries: PciEntries, read_pointer: u6, write_pointer: u6, occupancy: u7 }

fn zero_write() -> QueuedWrite { QueuedWrite { address: u24:0, data: u32:0, mask: u4:0 } }

pub fn initial_pci_fifo() -> PciFifoState {
    PciFifoState {
        entries: PciEntries:[zero_write(), ...],
        read_pointer: u6:0,
        write_pointer: u6:0,
        occupancy: u7:0,
    }
}

pub fn pci_fifo_enqueue(state: PciFifoState, entry: QueuedWrite) -> (PciFifoState, u1) {
    if state.occupancy == u7:64 {
        (state, u1:0)
    } else {
        let entries = update(state.entries, state.write_pointer, entry);
        (
            PciFifoState {
                entries,
                read_pointer: state.read_pointer,
                write_pointer: state.write_pointer + u6:1,
                occupancy: state.occupancy + u7:1,
            }, u1:1,
        )
    }
}

pub fn pci_fifo_dequeue(state: PciFifoState) -> (PciFifoState, QueuedWrite, u1) {
    if state.occupancy == u7:0 {
        (state, zero_write(), u1:0)
    } else {
        let entry = state.entries[state.read_pointer];
        (
            PciFifoState {
                entries: state.entries,
                read_pointer: state.read_pointer + u6:1,
                write_pointer: state.write_pointer,
                occupancy: state.occupancy - u7:1,
            }, entry, u1:1,
        )
    }
}

pub struct SwapState {
    phase: u2,
    retrace_counter: u8,
    swap_count: u2,
    draw_count: u2,
    swaps_pending: u3,
    interval: u8,
    vsync_enable: u1,
    retrace_previous: u1,
}

pub fn initial_swap() -> SwapState {
    SwapState {
        phase: u2:0,
        retrace_counter: u8:0,
        swap_count: u2:0,
        draw_count: u2:0,
        swaps_pending: u3:0,
        interval: u8:0,
        vsync_enable: u1:0,
        retrace_previous: u1:0,
    }
}

pub fn swap_enqueue(state: SwapState, command: u32) -> SwapState {
    let start = state.phase == u2:0;
    SwapState {
        phase: if start { u2:1 } else { state.phase },
        draw_count: state.draw_count + u2:1,
        swaps_pending: state.swaps_pending + u3:1,
        interval: if start { command[1+:u8] } else { state.interval },
        vsync_enable: if start { command[0+:u1] } else { state.vsync_enable },
        ..state
    }
}

pub fn swap_tick(state: SwapState, v_retrace: u1) -> SwapState {
    let rising = v_retrace == u1:1 && state.retrace_previous == u1:0;
    let sampling_complete = state.phase == u2:1 && state.vsync_enable == u1:0;
    let waiting_complete = state.phase == u2:2 && rising && state.retrace_counter >= state.interval;
    let completed = sampling_complete || waiting_complete;
    let phase = if completed {
        u2:0
    } else if state.phase == u2:1 {
        u2:2
    } else {
        state.phase
    };
    let retrace_counter = if state.phase == u2:1 {
        u8:0
    } else if state.phase == u2:2 && rising && !waiting_complete {
        state.retrace_counter + u8:1
    } else {
        state.retrace_counter
    };
    SwapState {
        phase,
        retrace_counter,
        swap_count: if completed { state.swap_count + u2:1 } else { state.swap_count },
        swaps_pending: if completed { state.swaps_pending - u3:1 } else { state.swaps_pending },
        retrace_previous: v_retrace,
        ..state
    }
}

pub struct CommandState { swap: SwapState, triangles: u32, fastfills: u32, nops: u32 }

pub fn initial_commands() -> CommandState {
    CommandState { swap: initial_swap(), triangles: u32:0, fastfills: u32:0, nops: u32:0 }
}

pub fn command_write(state: CommandState, address: u10, data: u32) -> CommandState {
    CommandState {
        swap: if address == u10:0x128 { swap_enqueue(state.swap, data) } else { state.swap },
        triangles: if address == u10:0x080 || address == u10:0x100 {
            state.triangles + u32:1
        } else {
            state.triangles
        },
        fastfills: if address == u10:0x124 { state.fastfills + u32:1 } else { state.fastfills },
        nops: if address == u10:0x120 { state.nops + u32:1 } else { state.nops },
    }
}

pub fn command_tick(state: CommandState, v_retrace: u1) -> CommandState {
    CommandState { swap: swap_tick(state.swap, v_retrace), ..state }
}

#[test]
fn pci_fifo_preserves_order_and_capacity() {
    let initial = initial_pci_fifo();
    let (full, accepted): (PciFifoState, u1) =
        for (i, accumulator): (u7, (PciFifoState, u1)) in u7:0..u7:64 {
            let (state, _) = accumulator;
            pci_fifo_enqueue(
                state, QueuedWrite { address: i as u24, data: i as u32, mask: u4:0xf })
        }((initial, u1:1));
    assert_eq(full.occupancy, u7:64);
    assert_eq(accepted, u1:1);
    let (still_full, rejected) = pci_fifo_enqueue(full, zero_write());
    assert_eq(still_full, full);
    assert_eq(rejected, u1:0);
    let (empty, ordered): (PciFifoState, u1) =
        for (i, accumulator): (u7, (PciFifoState, u1)) in u7:0..u7:64 {
            let (state, ordered) = accumulator;
            let (state, entry, valid) = pci_fifo_dequeue(state);
            (state, ordered && valid && entry.data == i as u32)
        }((full, u1:1));
    assert_eq(empty.occupancy, u7:0);
    assert_eq(ordered, u1:1);
}

#[test]
fn command_write_counts_work() {
    let state = initial_commands();
    let state = command_write(state, u10:0x080, u32:0);
    let state = command_write(state, u10:0x100, u32:0);
    let state = command_write(state, u10:0x120, u32:0);
    let state = command_write(state, u10:0x124, u32:0);
    assert_eq(state.triangles, u32:2);
    assert_eq(state.nops, u32:1);
    assert_eq(state.fastfills, u32:1);
}

#[test]
fn swap_without_vsync_completes_after_sampling() {
    let state = swap_enqueue(initial_swap(), u32:0);
    assert_eq(state.draw_count, u2:1);
    assert_eq(state.swaps_pending, u3:1);
    let state = swap_tick(state, u1:0);
    assert_eq(state.swap_count, u2:1);
    assert_eq(state.swaps_pending, u3:0);
    assert_eq(state.phase, u2:0);
}

#[test]
fn swap_waits_for_requested_retraces() {
    let state = swap_enqueue(initial_swap(), u32:0x00000005);
    let state = swap_tick(state, u1:0);
    assert_eq(state.phase, u2:2);
    let state = swap_tick(state, u1:1);
    let state = swap_tick(state, u1:0);
    let state = swap_tick(state, u1:1);
    let state = swap_tick(state, u1:0);
    let state = swap_tick(state, u1:1);
    assert_eq(state.swap_count, u2:1);
    assert_eq(state.phase, u2:0);
}
