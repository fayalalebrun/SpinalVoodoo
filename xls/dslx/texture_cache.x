#![feature(channel_attributes)]

import layout;
import memory;
import texture_sampler;

pub struct CacheRequest {
    kind: u3,
    memory_command: memory::MemoryCommand,
    sample: texture_sampler::SampleRequest,
    locations: u4[4],
    texture_layout: bits[432],
}

pub struct CacheResponse {
    memory_response: memory::MemoryResponse,
    texels: u16[4],
    hits: u1[4],
    total_hits: u32,
    total_misses: u32,
    all_hit_requests: u32,
    missed_requests: u32,
}

pub struct CacheState {
    valid: u1[64],
    bases: u26[64],
    victims: u1[32],
    hits: u32,
    misses: u32,
    all_hit_requests: u32,
    missed_requests: u32,
    phase: u3,
    request: CacheRequest,
    tap: u3,
    slot: u6,
    texels: u16[4],
    request_hits: u1[4],
    unresolved: u4,
    raw_line: bits[2048],
    fill_word: u6,
    fill_byte: u2,
    allocation_address: u26,
    allocation_hit: u1,
    miss_mask: u4,
    prepared_address: u26,
    prepared_location: u4,
}

struct LookupResult { state: CacheState, slot: u6, hit: u1 }

struct ParallelLookup {
    addresses: u26[4],
    locations: u4[4],
    entries: u22[4],
    active: u4,
    hits: u4,
    conflict: u1,
}

struct BankWrites { data: u32[4], masks: u4[4] }

pub struct PackedBankCommand { command: memory::MemoryCommand }

fn zero_memory_command() -> memory::MemoryCommand {
    memory::MemoryCommand { write: u1:0, address: u22:0, data: u32:0, mask: u4:0 }
}

fn zero_sample() -> texture_sampler::SampleRequest {
    texture_sampler::SampleRequest {
        enabled: u1:0,
        bilinear: u1:0,
        is_16_bit: u1:0,
        format: u4:0,
        addresses: uN[104]:0,
        ds: u4:0,
        dt: u4:0,
    }
}

pub fn memory_request(command: memory::MemoryCommand) -> CacheRequest {
    CacheRequest {
        kind: u3:0,
        memory_command: command,
        sample: zero_sample(),
        locations: u4[4]:[u4:0, ...],
        texture_layout: bits[432]:0,
    }
}


pub fn invalidate_request() -> CacheRequest {
    CacheRequest {
        kind: u3:2,
        memory_command: zero_memory_command(),
        sample: zero_sample(),
        locations: u4[4]:[u4:0, ...],
        texture_layout: bits[432]:0,
    }
}

pub fn status_request() -> CacheRequest {
    CacheRequest {
        kind: u3:3,
        memory_command: zero_memory_command(),
        sample: zero_sample(),
        locations: u4[4]:[u4:0, ...],
        texture_layout: bits[432]:0,
    }
}


pub fn initial_state() -> CacheState {
    CacheState {
        valid: u1[64]:[u1:0, ...],
        bases: u26[64]:[u26:0, ...],
        victims: u1[32]:[u1:0, ...],
        hits: u32:0,
        misses: u32:0,
        all_hit_requests: u32:0,
        missed_requests: u32:0,
        phase: u3:0,
        request: invalidate_request(),
        tap: u3:0,
        slot: u6:0,
        texels: u16[4]:[u16:0, ...],
        request_hits: u1[4]:[u1:0, ...],
        unresolved: u4:0,
        raw_line: bits[2048]:0,
        fill_word: u6:0,
        fill_byte: u2:0,
        allocation_address: u26:0,
        allocation_hit: u1:0,
        miss_mask: u4:0,
        prepared_address: u26:0,
        prepared_location: u4:0,
    }
}

fn invalidate(state: CacheState) -> CacheState { CacheState { valid: u1[64]:[u1:0, ...], ..state } }

fn tap_address(addresses: uN[104], tap: u3) -> u26 {
    (addresses >> (((u3:3 - tap) as u8) * u8:26)) as u26
}

fn select_location_segment(selected: (u1, u22, u4), segment: u48, address: u22) -> (u1, u22, u4) {
    let lod_base = segment[26+:u22];
    let lod_end = segment[4+:u22];
    if address >= lod_base && address < lod_end {
        (u1:1, lod_base, segment[0+:u4])
    } else {
        selected
    }
}

fn location(texture_layout: bits[432], is_16_bit: u1, address: u26) -> u4 {
    let selected =
        select_location_segment((u1:0, u22:0, u4:0), texture_layout[384+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[336+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[288+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[240+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[192+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[144+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[96+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[48+:u48], address[0+:u22]);
    let (valid, base, shift) =
        select_location_segment(selected, texture_layout[0+:u48], address[0+:u22]);
    let offset = (address as u10) - (base as u10);
    let bank_msb = if is_16_bit == u1:1 {
        offset[shift+:u1] | offset[(shift + u4:1) as u4+:u1]
    } else {
        offset[shift+:u1]
    };
    let bank_lsb = if is_16_bit == u1:1 { offset[0+:u1] | offset[1+:u1] } else { offset[0+:u1] };
    let pair = is_16_bit == u1:0 && offset[1+:u1] == u1:1;
    valid ++ bank_msb ++ bank_lsb ++ (pair as u1)
}

fn sample_locations(sample: texture_sampler::SampleRequest, texture_layout: bits[432]) -> u4[4] {
    let addresses = u26[4]:[
        tap_address(sample.addresses, u3:0),
        tap_address(sample.addresses, u3:1),
        tap_address(sample.addresses, u3:2),
        tap_address(sample.addresses, u3:3),
    ];
    u4[4]:[
        location(texture_layout, sample.is_16_bit, addresses[0]),
        location(texture_layout, sample.is_16_bit, addresses[1]),
        location(texture_layout, sample.is_16_bit, addresses[2]),
        location(texture_layout, sample.is_16_bit, addresses[3]),
    ]
}

pub fn sample_request
    (sample: texture_sampler::SampleRequest, texture_layout: bits[432]) -> CacheRequest {
    CacheRequest {
        kind: u3:1,
        memory_command: zero_memory_command(),
        sample,
        locations: sample_locations(sample, texture_layout),
        texture_layout,
    }
}

pub fn invalidating_sample_request
    (sample: texture_sampler::SampleRequest, texture_layout: bits[432]) -> CacheRequest {
    CacheRequest {
        kind: u3:4,
        memory_command: zero_memory_command(),
        sample,
        locations: sample_locations(sample, texture_layout),
        texture_layout,
    }
}

fn line_base(address: u26) -> u26 { address & u26:0x3ffff00 }

fn cache_set(base: u26) -> u5 {
    let line = base >> u8:8;
    line[0+:u5] ^ line[5+:u5] ^ line[10+:u5]
}

fn probe(state: CacheState, address: u26) -> LookupResult {
    let line_base = line_base(address);
    let set = cache_set(line_base);
    let slot0 = (set as u6) << u8:1;
    let slot1 = slot0 | u6:1;
    let hit0 = state.valid[slot0] && state.bases[slot0] == line_base;
    let hit1 = state.valid[slot1] && state.bases[slot1] == line_base;
    let hit = hit0 || hit1;
    let way = if hit0 {
        u1:0
    } else if hit1 {
        u1:1
    } else if state.valid[slot0] == u1:0 {
        u1:0
    } else if state.valid[slot1] == u1:0 {
        u1:1
    } else {
        state.victims[set]
    };
    LookupResult { state, slot: slot0 | (way as u6), hit }
}

fn lookup(state: CacheState, address: u26) -> LookupResult {
    let result = probe(state, address);
    let line_base = line_base(address);
    let set = cache_set(line_base);
    let way = result.slot[0+:u1];
    let next_state = if result.hit {
        CacheState { hits: state.hits + u32:1, ..state }
    } else {
        CacheState {
            valid: update(state.valid, result.slot, u1:1),
            bases: update(state.bases, result.slot, line_base),
            victims: update(state.victims, set, !way),
            misses: state.misses + u32:1,
            ..state
        }
    };
    LookupResult { state: next_state, slot: result.slot, hit: result.hit }
}

fn add_fill_byte_at(is_16_bit: u1, raw: u32, byte: u2, loc: u4, writes: BankWrites) -> BankWrites {
    let mapped = byte[0+:u1] == u1:0 || is_16_bit == u1:0;
    if mapped && loc[3+:u1] == u1:1 {
        let bank = loc[1+:u2];
        let pair = loc[0+:u1];
        let raw_byte = (raw >> ((byte as u5) << u8:3)) as u8;
        let texel = if is_16_bit == u1:1 {
            if byte == u2:0 { raw[0+:u16] } else { raw[16+:u16] }
        } else {
            raw_byte ++ raw_byte
        };
        let data = if pair == u1:1 {
            texel ++ writes.data[bank][0+:u16]
        } else {
            writes.data[bank][16+:u16] ++ texel
        };
        BankWrites {
            data: update(writes.data, bank, data),
            masks: update(
                writes.masks, bank, writes.masks[bank] | if pair == u1:1 { u4:0xc } else { u4:0x3 }),
        }
    } else {
        writes
    }
}

fn add_fill_byte
    (texture_layout: bits[432], is_16_bit: u1, word_address: u26, raw: u32, byte: u2,
     writes: BankWrites) -> BankWrites {
    add_fill_byte_at(
        is_16_bit, raw, byte, location(texture_layout, is_16_bit, word_address + (byte as u26)),
        writes)
}

fn fill_word(texture_layout: bits[432], is_16_bit: u1, word_address: u26, raw: u32) -> BankWrites {
    for (byte, writes): (u3, BankWrites) in u3:0..u3:4 {
        add_fill_byte(texture_layout, is_16_bit, word_address, raw, byte as u2, writes)
    }(BankWrites { data: u32[4]:[u32:0, ...], masks: u4[4]:[u4:0, ...] })
}

fn byte_mask(mask: u4) -> u32 {
    (if mask[0+:u1] == u1:1 { u32:0x000000ff } else { u32:0 }) |
    (if mask[1+:u1] == u1:1 { u32:0x0000ff00 } else { u32:0 }) |
    (if mask[2+:u1] == u1:1 { u32:0x00ff0000 } else { u32:0 }) |
    (if mask[3+:u1] == u1:1 { u32:0xff000000 } else { u32:0 })
}

fn active_taps(sample: texture_sampler::SampleRequest) -> u4 {
    if sample.bilinear == u1:1 { u4:0xf } else { u4:0x1 }
}

fn first_tap(mask: u4) -> u3 {
    if mask[0+:u1] == u1:1 {
        u3:0
    } else if mask[1+:u1] == u1:1 {
        u3:1
    } else if mask[2+:u1] == u1:1 {
        u3:2
    } else {
        u3:3
    }
}

fn count_taps(mask: u4) -> u32 {
    (mask[0+:u1] as u32) + (mask[1+:u1] as u32) + (mask[2+:u1] as u32) + (mask[3+:u1] as u32)
}

fn bank_texel(word: u32, pair: u1) -> u16 {
    if pair == u1:1 { word[16+:u16] } else { word[0+:u16] }
}

fn parallel_lookup(state: CacheState, request: CacheRequest) -> ParallelLookup {
    let addresses = u26[4]:[
        tap_address(request.sample.addresses, u3:0),
        tap_address(request.sample.addresses, u3:1),
        tap_address(request.sample.addresses, u3:2),
        tap_address(request.sample.addresses, u3:3),
    ];
    let locations = request.locations;
    let probe0 = probe(state, addresses[0]);
    let probe1 = probe(state, addresses[1]);
    let probe2 = probe(state, addresses[2]);
    let probe3 = probe(state, addresses[3]);
    let active = active_taps(request.sample);
    let hits = ((active[3+:u1] && probe3.hit) as u4) << u8:3 |
               ((active[2+:u1] && probe2.hit) as u4) << u8:2 |
               ((active[1+:u1] && probe1.hit) as u4) << u8:1 |
               ((active[0+:u1] && probe0.hit) as u4);
    let entries = u22[4]:[
        ((probe0.slot as u22) << u8:6) | (addresses[0][2+:u6] as u22),
        ((probe1.slot as u22) << u8:6) | (addresses[1][2+:u6] as u22),
        ((probe2.slot as u22) << u8:6) | (addresses[2][2+:u6] as u22),
        ((probe3.slot as u22) << u8:6) | (addresses[3][2+:u6] as u22),
    ];
    let conflict =
        (active[0+:u1] && active[1+:u1] && locations[0][1+:u2] == locations[1][1+:u2]) ||
        (active[0+:u1] && active[2+:u1] && locations[0][1+:u2] == locations[2][1+:u2]) ||
        (active[0+:u1] && active[3+:u1] && locations[0][1+:u2] == locations[3][1+:u2]) ||
        (active[1+:u1] && active[2+:u1] && locations[1][1+:u2] == locations[2][1+:u2]) ||
        (active[1+:u1] && active[3+:u1] && locations[1][1+:u2] == locations[3][1+:u2]) ||
        (active[2+:u1] && active[3+:u1] && locations[2][1+:u2] == locations[3][1+:u2]);
    ParallelLookup { addresses, locations, entries, active, hits, conflict: conflict as u1 }
}

fn bank_tap(lookup: ParallelLookup, bank: u2) -> u2 {
    if lookup.active[0+:u1] && lookup.locations[0][1+:u2] == bank {
        u2:0
    } else if lookup.active[1+:u1] && lookup.locations[1][1+:u2] == bank {
        u2:1
    } else if lookup.active[2+:u1] && lookup.locations[2][1+:u2] == bank {
        u2:2
    } else {
        u2:3
    }
}

fn bank_requested(lookup: ParallelLookup, bank: u2) -> u1 {
    ((lookup.active[0+:u1] && lookup.locations[0][1+:u2] == bank) ||
     (lookup.active[1+:u1] && lookup.locations[1][1+:u2] == bank) ||
     (lookup.active[2+:u1] && lookup.locations[2][1+:u2] == bank) ||
     (lookup.active[3+:u1] && lookup.locations[3][1+:u2] == bank)) as u1
}

fn raw_texel(word: u32, is_16_bit: u1, address: u26) -> u16 {
    if is_16_bit == u1:1 {
        if address[1+:u1] == u1:1 { word[16+:u16] } else { word[0+:u16] }
    } else {
        let byte = (word >> ((address[0+:u2] as u5) << u8:3)) as u8;
        byte ++ byte
    }
}

fn raw_line_texel(line: bits[2048], is_16_bit: u1, address: u26) -> u16 {
    let shift = (address[2+:u6] as u12) * u12:32;
    raw_texel((line >> shift) as u32, is_16_bit, address)
}

fn packed_bank_command(command: memory::MemoryCommand) -> PackedBankCommand {
    PackedBankCommand { command }
}

fn fill_bank_command(slot: u6, word: u6, writes: BankWrites, bank: u3) -> PackedBankCommand {
    PackedBankCommand {
        command: memory::MemoryCommand {
            write: u1:1,
            address: ((slot as u22) << u8:6) | (word as u22),
            data: writes.data[bank],
            mask: writes.masks[bank],
        },
    }
}

pub proc packed_bank_memory {
    command_r: chan<PackedBankCommand> in;
    response_s: chan<memory::MemoryResponse> out;
    type Lines = bits[2048][64];

    config(command_r: chan<PackedBankCommand> in, response_s: chan<memory::MemoryResponse> out) {
        (command_r, response_s)
    }

    init { Lines:[bits[2048]:0, ...] }

    next(lines: Lines) {
        let (tok, request) = recv(join(), command_r);
        let command = request.command;
        let slot = command.address[6+:u6];
        let shift = (command.address[0+:u6] as u12) * u12:32;
        let line = lines[slot];
        let current = (line >> shift) as u32;
        let mask = byte_mask(command.mask);
        let data =
            if command.write == u1:1 { (current & !mask) | (command.data & mask) } else { current };
        let wide_mask = (mask as bits[2048]) << shift;
        let wide_data = (data as bits[2048]) << shift;
        let line = (line & !wide_mask) | (wide_data & wide_mask);
        let lines = if command.write == u1:1 { update(lines, slot, line) } else { lines };
        let tok = send(tok, response_s, memory::MemoryResponse { data, success: u1:1 });
        lines
    }
}

pub proc texture_cache {
    request_r: chan<CacheRequest> in;
    response_s: chan<CacheResponse> out;
    memory_command_s: chan<memory::MemoryCommand> out;
    memory_response_r: chan<memory::MemoryResponse> in;
    block_read_s: chan<memory::MemoryBlockRead> out;
    block_read_response_r: chan<memory::MemoryBlockReadResponse> in;
    bank0_command_s: chan<PackedBankCommand> out;
    bank0_response_r: chan<memory::MemoryResponse> in;
    bank1_command_s: chan<PackedBankCommand> out;
    bank1_response_r: chan<memory::MemoryResponse> in;
    bank2_command_s: chan<PackedBankCommand> out;
    bank2_response_r: chan<memory::MemoryResponse> in;
    bank3_command_s: chan<PackedBankCommand> out;
    bank3_response_r: chan<memory::MemoryResponse> in;

    config(request_r: chan<CacheRequest> in, response_s: chan<CacheResponse> out,
           memory_command_s: chan<memory::MemoryCommand> out,
           memory_response_r: chan<memory::MemoryResponse> in,
           block_read_s: chan<memory::MemoryBlockRead> out,
           block_read_response_r: chan<memory::MemoryBlockReadResponse> in,
           bank0_command_s: chan<PackedBankCommand> out,
           bank0_response_r: chan<memory::MemoryResponse> in,
           bank1_command_s: chan<PackedBankCommand> out,
           bank1_response_r: chan<memory::MemoryResponse> in,
           bank2_command_s: chan<PackedBankCommand> out,
           bank2_response_r: chan<memory::MemoryResponse> in,
           bank3_command_s: chan<PackedBankCommand> out,
           bank3_response_r: chan<memory::MemoryResponse> in) {
        (
            request_r, response_s, memory_command_s, memory_response_r, block_read_s,
            block_read_response_r, bank0_command_s, bank0_response_r, bank1_command_s,
            bank1_response_r, bank2_command_s, bank2_response_r, bank3_command_s, bank3_response_r,
        )
    }

    init { initial_state() }

    next(state: CacheState) {
        let idle = state.phase == u3:0;
        let lookup_active = state.phase == u3:1;
        let lookup_prepare_active = state.phase == u3:7;
        let fetching = state.phase == u3:2;
        let writing = state.phase == u3:3;
        let lookup_complete_active = state.phase == u3:6;
        let allocation_probe_active = state.phase == u3:4;
        let allocation_commit_active = state.phase == u3:5;
        let (tok, incoming) = recv_if(join(), request_r, idle, invalidate_request());
        let incoming_lookup = idle && (incoming.kind == u3:1 || incoming.kind == u3:4);
        let parallel = parallel_lookup(state, incoming);
        let parallel_done = incoming_lookup && incoming.kind == u3:1 &&
                            parallel.hits == parallel.active && !parallel.conflict;
        let memory_access = idle && incoming.kind == u3:0;
        let start_lookup = incoming_lookup && !parallel_done;
        let immediate_response = idle && !incoming_lookup;
        let invalidating = idle &&
                           ((incoming.kind == u3:0 && incoming.memory_command.write == u1:1) ||
                           incoming.kind == u3:2 || incoming.kind == u3:4);
        let active = active_taps(state.request.sample);
        let lookup_address = state.prepared_address;
        let lookup_probe = probe(state, lookup_address);
        let lookup_hit = lookup_active && active[state.tap+:u1] && lookup_probe.hit;
        let lookup_hits = if lookup_active {
            update(state.request_hits, state.tap, lookup_hit as u1)
        } else {
            state.request_hits
        };
        let lookup_last =
            lookup_active && (state.request.sample.bilinear == u1:0 || state.tap == u3:3);
        let hits =
            lookup_hits[u32:3] ++ lookup_hits[u32:2] ++ lookup_hits[u32:1] ++ lookup_hits[u32:0];
        let misses = active & !hits;

        let fill_address = tap_address(state.request.sample.addresses, state.tap);
        let fill_line_base = line_base(fill_address);
        let tok = send_if(tok, memory_command_s, memory_access, incoming.memory_command);
        let (tok, memory_response) = recv_if(
            tok, memory_response_r, memory_access,
            memory::MemoryResponse { data: u32:0, success: u1:1 });
        let tok = send_if(
            tok, block_read_s, fetching,
            memory::MemoryBlockRead { address: (fill_line_base >> u8:2) as u22 });
        let (tok, block_read) = recv_if(
            tok, block_read_response_r, fetching,
            memory::MemoryBlockReadResponse { data: bits[2048]:0, success: u1:1 });
        let fill_writes = fill_word(
            state.request.texture_layout, state.request.sample.is_16_bit,
            fill_line_base + ((state.fill_word as u26) << u8:2), state.raw_line as u32);

        let lookup_loc = state.prepared_location;
        let lookup_entry = ((lookup_probe.slot as u22) << u8:6) | (lookup_address[2+:u6] as u22);

        let bank0_write = fill_bank_command(state.slot, state.fill_word, fill_writes, u3:0);
        let bank1_write = fill_bank_command(state.slot, state.fill_word, fill_writes, u3:1);
        let bank2_write = fill_bank_command(state.slot, state.fill_word, fill_writes, u3:2);
        let bank3_write = fill_bank_command(state.slot, state.fill_word, fill_writes, u3:3);
        let bank0_parallel = parallel_done && bank_requested(parallel, u2:0);
        let bank1_parallel = parallel_done && bank_requested(parallel, u2:1);
        let bank2_parallel = parallel_done && bank_requested(parallel, u2:2);
        let bank3_parallel = parallel_done && bank_requested(parallel, u2:3);
        let bank0_read = (lookup_hit && lookup_loc[1+:u2] == u2:0) || bank0_parallel;
        let bank1_read = (lookup_hit && lookup_loc[1+:u2] == u2:1) || bank1_parallel;
        let bank2_read = (lookup_hit && lookup_loc[1+:u2] == u2:2) || bank2_parallel;
        let bank3_read = (lookup_hit && lookup_loc[1+:u2] == u2:3) || bank3_parallel;
        let bank0_read_address = if bank0_parallel {
            parallel.entries[bank_tap(parallel, u2:0)]
        } else {
            lookup_entry
        };
        let bank1_read_address = if bank1_parallel {
            parallel.entries[bank_tap(parallel, u2:1)]
        } else {
            lookup_entry
        };
        let bank2_read_address = if bank2_parallel {
            parallel.entries[bank_tap(parallel, u2:2)]
        } else {
            lookup_entry
        };
        let bank3_read_address = if bank3_parallel {
            parallel.entries[bank_tap(parallel, u2:3)]
        } else {
            lookup_entry
        };
        let bank0_read_command = memory::MemoryCommand {
            write: u1:0,
            address: bank0_read_address,
            data: u32:0,
            mask: u4:0,
        };
        let bank1_read_command = memory::MemoryCommand {
            write: u1:0,
            address: bank1_read_address,
            data: u32:0,
            mask: u4:0,
        };
        let bank2_read_command = memory::MemoryCommand {
            write: u1:0,
            address: bank2_read_address,
            data: u32:0,
            mask: u4:0,
        };
        let bank3_read_command = memory::MemoryCommand {
            write: u1:0,
            address: bank3_read_address,
            data: u32:0,
            mask: u4:0,
        };
        let bank0_active = (writing && fill_writes.masks[0] != u4:0) || bank0_read;
        let bank1_active = (writing && fill_writes.masks[1] != u4:0) || bank1_read;
        let bank2_active = (writing && fill_writes.masks[2] != u4:0) || bank2_read;
        let bank3_active = (writing && fill_writes.masks[3] != u4:0) || bank3_read;
        let bank0_command =
            if writing { bank0_write } else { packed_bank_command(bank0_read_command) };
        let bank1_command =
            if writing { bank1_write } else { packed_bank_command(bank1_read_command) };
        let bank2_command =
            if writing { bank2_write } else { packed_bank_command(bank2_read_command) };
        let bank3_command =
            if writing { bank3_write } else { packed_bank_command(bank3_read_command) };
        let bank0_tok = send_if(tok, bank0_command_s, bank0_active, bank0_command);
        let (bank0_tok, bank0) = recv_if(
            bank0_tok, bank0_response_r, bank0_active,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
        let bank1_tok = send_if(tok, bank1_command_s, bank1_active, bank1_command);
        let (bank1_tok, bank1) = recv_if(
            bank1_tok, bank1_response_r, bank1_active,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
        let bank2_tok = send_if(tok, bank2_command_s, bank2_active, bank2_command);
        let (bank2_tok, bank2) = recv_if(
            bank2_tok, bank2_response_r, bank2_active,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
        let bank3_tok = send_if(tok, bank3_command_s, bank3_active, bank3_command);
        let (bank3_tok, bank3) = recv_if(
            bank3_tok, bank3_response_r, bank3_active,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
        let tok = join(bank0_tok, bank1_tok, bank2_tok, bank3_tok);

        let lookup_word = match lookup_loc[1+:u2] {
            u2:0 => bank0.data,
            u2:1 => bank1.data,
            u2:2 => bank2.data,
            _ => bank3.data,
        };
        let lookup_texels = if lookup_hit {
            update(state.texels, state.tap, bank_texel(lookup_word, lookup_loc[0+:u1]))
        } else {
            state.texels
        };
        let bank_words = u32[4]:[bank0.data, bank1.data, bank2.data, bank3.data];
        let parallel_texels = u16[4]:[
            if parallel.active[0+:u1] {
                bank_texel(bank_words[parallel.locations[0][1+:u2]], parallel.locations[0][0+:u1])
            } else {
                u16:0
            },
            if parallel.active[1+:u1] {
                bank_texel(bank_words[parallel.locations[1][1+:u2]], parallel.locations[1][0+:u1])
            } else {
                u16:0
            },
            if parallel.active[2+:u1] {
                bank_texel(bank_words[parallel.locations[2][1+:u2]], parallel.locations[2][0+:u1])
            } else {
                u16:0
            },
            if parallel.active[3+:u1] {
                bank_texel(bank_words[parallel.locations[3][1+:u2]], parallel.locations[3][0+:u1])
            } else {
                u16:0
            },
        ];

        let (fill_texels, next_unresolved) =
            for (tap, accumulator): (u3, (u16[4], u4)) in u3:0..u3:4 {
                let (texels, unresolved) = accumulator;
                let capture_address = tap_address(state.request.sample.addresses, tap);
                let capture = writing && unresolved[tap+:u1] &&
                              line_base(capture_address) == fill_line_base &&
                              capture_address[2+:u6] == state.fill_word;
                let texels = if capture {
                    update(
                        texels, tap,
                        raw_texel(
                            state.raw_line as u32, state.request.sample.is_16_bit,
                            capture_address))
                } else {
                    texels
                };
                (texels, unresolved & !((capture as u4) << tap))
            }((state.texels, state.unresolved));
        let last_fill_word = writing && state.fill_word == u6:63;
        let fill_done = last_fill_word && next_unresolved == u4:0;
        let allocation_probe = probe(state, state.allocation_address);
        let allocation_base = line_base(state.allocation_address);
        let allocation_set = cache_set(allocation_base);
        let allocation_way = state.slot[0+:u1];
        let allocation_valid =
            if state.allocation_hit { state.valid } else { update(state.valid, state.slot, u1:1) };
        let allocation_bases = if state.allocation_hit {
            state.bases
        } else {
            update(state.bases, state.slot, allocation_base)
        };
        let allocation_victims = if state.allocation_hit {
            state.victims
        } else {
            update(state.victims, allocation_set, !allocation_way)
        };
        let lookup_done = lookup_last && misses == u4:0;
        let parallel_total_hits = state.hits + count_taps(parallel.active);
        let parallel_all_hit_requests = state.all_hit_requests + u32:1;
        let lookup_total_hits = state.hits + count_taps(active & hits);
        let lookup_total_misses = state.misses + count_taps(misses);
        let lookup_all_hit_requests = state.all_hit_requests + (misses == u4:0) as u32;
        let lookup_missed_requests = state.missed_requests + (misses != u4:0) as u32;
        let response = if parallel_done {
            CacheResponse {
                memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
                texels: parallel_texels,
                hits: u1[4]:[
                    parallel.active[0+:u1], parallel.active[1+:u1],
                    parallel.active[2+:u1], parallel.active[3+:u1],
                ],
                total_hits: parallel_total_hits,
                total_misses: state.misses,
                all_hit_requests: parallel_all_hit_requests,
                missed_requests: state.missed_requests,
            }
        } else if lookup_done {
            CacheResponse {
                memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
                texels: lookup_texels,
                hits: u1[4]:[hits[0+:u1], hits[1+:u1], hits[2+:u1], hits[3+:u1]],
                total_hits: lookup_total_hits,
                total_misses: lookup_total_misses,
                all_hit_requests: lookup_all_hit_requests,
                missed_requests: lookup_missed_requests,
            }
        } else if fill_done {
            CacheResponse {
                memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
                texels: fill_texels,
                hits: state.request_hits,
                total_hits: state.hits,
                total_misses: state.misses,
                all_hit_requests: state.all_hit_requests,
                missed_requests: state.missed_requests,
            }
        } else {
            CacheResponse {
                memory_response,
                texels: u16[4]:[u16:0, ...],
                hits: u1[4]:[u1:0, ...],
                total_hits: state.hits,
                total_misses: state.misses,
                all_hit_requests: state.all_hit_requests,
                missed_requests: state.missed_requests,
            }
        };
        let tok = send_if(
            tok, response_s, immediate_response || parallel_done || lookup_done || fill_done,
            response);

        let state = if invalidating { invalidate(state) } else { state };
        let state = if parallel_done {
            CacheState {
                hits: parallel_total_hits,
                all_hit_requests: parallel_all_hit_requests,
                ..state
            }
        } else {
            state
        };
        let state = if incoming_lookup {
            CacheState {
                request: incoming,
                tap: u3:0,
                texels: u16[4]:[u16:0, ...],
                request_hits: u1[4]:[u1:0, ...],
                unresolved: u4:0,
                ..state
            }
        } else {
            state
        };
        let state = if lookup_active {
            if lookup_last {
                CacheState {
                    hits: lookup_total_hits,
                    misses: lookup_total_misses,
                    all_hit_requests: lookup_all_hit_requests,
                    missed_requests: lookup_missed_requests,
                    miss_mask: misses,
                    texels: lookup_texels,
                    request_hits: lookup_hits,
                    ..state
                }
            } else {
                CacheState {
                    tap: state.tap + u3:1,
                    texels: lookup_texels,
                    request_hits: lookup_hits,
                    ..state
                }
            }
        } else {
            state
        };
        let state = if lookup_prepare_active {
            let prepared_address = tap_address(state.request.sample.addresses, state.tap);
            CacheState {
                prepared_address,
                prepared_location: state.request.locations[state.tap],
                ..state
            }
        } else {
            state
        };
        let state = if lookup_complete_active {
            let miss_tap = first_tap(state.miss_mask);
            let miss_address = tap_address(state.request.sample.addresses, miss_tap);
            CacheState {
                tap: miss_tap,
                allocation_address: miss_address,
                unresolved: state.miss_mask,
                ..state
            }
        } else {
            state
        };
        let state = if allocation_probe_active {
            CacheState {
                slot: allocation_probe.slot,
                allocation_hit: allocation_probe.hit,
                ..state
            }
        } else {
            state
        };
        let state = if allocation_commit_active {
            CacheState {
                valid: allocation_valid,
                bases: allocation_bases,
                victims: allocation_victims,
                ..state
            }
        } else {
            state
        };
        let state = if fetching {
            CacheState {
                raw_line: block_read.data,
                fill_word: u6:0,
                fill_byte: u2:0,
                ..state
            }
        } else {
            state
        };
        let next_phase = match state.phase {
            u3:0 => if start_lookup { u3:7 } else { u3:0 },
            u3:1 => if lookup_last { u3:6 } else { u3:7 },
            u3:2 => u3:3,
            u3:3 => if last_fill_word {
                if next_unresolved != u4:0 { u3:6 } else { u3:0 }
            } else { u3:3 },
            u3:4 => u3:5,
            u3:5 => u3:2,
            u3:6 => if state.miss_mask == u4:0 { u3:0 } else { u3:4 },
            u3:7 => u3:1,
        };
        let state = if last_fill_word {
            if next_unresolved != u4:0 {
                CacheState {
                    miss_mask: next_unresolved,
                    texels: fill_texels,
                    unresolved: next_unresolved,
                    fill_byte: u2:0,
                    ..state
                }
            } else {
                CacheState {
                    texels: fill_texels,
                    unresolved: u4:0,
                    fill_byte: u2:0,
                    ..state
                }
            }
        } else if writing {
            CacheState {
                raw_line: state.raw_line >> u8:32,
                fill_word: state.fill_word + u6:1,
                fill_byte: u2:0,
                texels: fill_texels,
                unresolved: next_unresolved,
                ..state
            }
        } else {
            state
        };
        CacheState { phase: next_phase, ..state }
    }
}

#[test]
fn lookup_uses_production_geometry_and_rotating_victim() {
    assert_eq(line_base(u26:0x0000ff), u26:0x000000);
    assert_eq(line_base(u26:0x000100), u26:0x000100);
    assert_eq(cache_set(u26:0x000000), u5:0);
    assert_eq(cache_set(u26:0x002100), u5:0);
    let first = lookup(initial_state(), u26:0x000000);
    assert_eq((first.slot, first.hit), (u6:0, u1:0));
    let second = lookup(first.state, u26:0x002100);
    assert_eq((second.slot, second.hit), (u6:1, u1:0));
    let hit = lookup(second.state, u26:0x000000);
    assert_eq((hit.slot, hit.hit), (u6:0, u1:1));
    let replace = lookup(hit.state, u26:0x004200);
    assert_eq((replace.slot, replace.hit), (u6:0, u1:0));
}

#[test]
fn fill_word_preserves_unwritten_banks() {
    let texture_layout = layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:2, u2:0, u1:1, u1:0);
    let writes = fill_word(texture_layout, u1:0, u26:0, u32:0x44332211);
    assert_eq(
        writes.masks[u32:0] | writes.masks[u32:1] | writes.masks[u32:2] | writes.masks[u32:3],
        u4:0xf);
    assert_eq(raw_texel(u32:0x44332211, u1:0, u26:2), u16:0x3333);
    assert_eq(raw_texel(u32:0x44332211, u1:1, u26:2), u16:0x4433);
}

fn repeated_line(word: u32) -> bits[2048] {
    for (index, line): (u7, bits[2048]) in u7:0..u7:64 {
        line | ((word as bits[2048]) << ((index as u12) * u12:32))
    }(bits[2048]:0)
}

fn test_sample(addresses: uN[104], bilinear: u1) -> texture_sampler::SampleRequest {
    texture_sampler::SampleRequest {
        enabled: u1:1,
        bilinear,
        is_16_bit: u1:0,
        format: u4:2,
        addresses,
        ds: u4:0,
        dt: u4:0,
    }
}

#[test_proc]
proc cache_traffic_matches_de10_geometry {
    request_s: chan<CacheRequest> out;
    response_r: chan<CacheResponse> in;
    memory_command_r: chan<memory::MemoryCommand> in;
    memory_response_s: chan<memory::MemoryResponse> out;
    block_read_r: chan<memory::MemoryBlockRead> in;
    block_read_response_s: chan<memory::MemoryBlockReadResponse> out;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (request_s, request_r) = chan<CacheRequest>("request");
        let (response_s, response_r) = chan<CacheResponse>("response");
        let (memory_command_s, memory_command_r) = chan<memory::MemoryCommand>("memory_command");
        let (memory_response_s, memory_response_r) =
            chan<memory::MemoryResponse>("memory_response");
        let (block_read_s, block_read_r) = chan<memory::MemoryBlockRead>("block_read");
        let (block_read_response_s, block_read_response_r) =
            chan<memory::MemoryBlockReadResponse>("block_read_response");
        let (bank0_command_s, bank0_command_r) = chan<PackedBankCommand>("bank0_command");
        let (bank0_response_s, bank0_response_r) = chan<memory::MemoryResponse>("bank0_response");
        let (bank1_command_s, bank1_command_r) = chan<PackedBankCommand>("bank1_command");
        let (bank1_response_s, bank1_response_r) = chan<memory::MemoryResponse>("bank1_response");
        let (bank2_command_s, bank2_command_r) = chan<PackedBankCommand>("bank2_command");
        let (bank2_response_s, bank2_response_r) = chan<memory::MemoryResponse>("bank2_response");
        let (bank3_command_s, bank3_command_r) = chan<PackedBankCommand>("bank3_command");
        let (bank3_response_s, bank3_response_r) = chan<memory::MemoryResponse>("bank3_response");
        spawn packed_bank_memory(bank0_command_r, bank0_response_s);
        spawn packed_bank_memory(bank1_command_r, bank1_response_s);
        spawn packed_bank_memory(bank2_command_r, bank2_response_s);
        spawn packed_bank_memory(bank3_command_r, bank3_response_s);
        spawn texture_cache(
            request_r, response_s, memory_command_s, memory_response_r, block_read_s,
            block_read_response_r, bank0_command_s, bank0_response_r, bank1_command_s,
            bank1_response_r, bank2_command_s, bank2_response_r, bank3_command_s, bank3_response_r);
        (
            request_s, response_r, memory_command_r, memory_response_s, block_read_r,
            block_read_response_s, terminator,
        )
    }

    init { () }

    next(state: ()) {
        let texture_layout =
            layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:2, u2:0, u1:1, u1:0);
        let point = test_sample(uN[104]:0, u1:0);
        let tok = send(join(), request_s, sample_request(point, texture_layout));
        let (tok, command) = recv(tok, block_read_r);
        assert_eq(command, memory::MemoryBlockRead { address: u22:0 });
        let tok = send(
            tok, block_read_response_s,
            memory::MemoryBlockReadResponse { data: repeated_line(u32:0x44332211), success: u1:1 });
        let (tok, cold_point) = recv(tok, response_r);
        assert_eq(cold_point.hits, u1[4]:[u1:0, ...]);
        assert_eq(cold_point.texels[u32:0], u16:0x1111);
        assert_eq(cold_point.all_hit_requests, u32:0);
        assert_eq(cold_point.missed_requests, u32:1);

        let tok = send(tok, request_s, sample_request(point, texture_layout));
        let (tok, hot_point) = recv(tok, response_r);
        assert_eq(hot_point.hits, u1[4]:[u1:1, u1:0, u1:0, u1:0]);
        assert_eq(hot_point.texels[u32:0], u16:0x1111);
        assert_eq(hot_point.all_hit_requests, u32:1);
        assert_eq(hot_point.missed_requests, u32:1);

        let tok = send(tok, request_s, sample_request(point, texture_layout));
        let tok = send(tok, request_s, sample_request(point, texture_layout));
        let (tok, hot_point0) = recv(tok, response_r);
        let (tok, hot_point1) = recv(tok, response_r);
        assert_eq(hot_point0.hits, u1[4]:[u1:1, u1:0, u1:0, u1:0]);
        assert_eq(hot_point1.hits, u1[4]:[u1:1, u1:0, u1:0, u1:0]);
        assert_eq(hot_point1.all_hit_requests, u32:3);
        assert_eq(hot_point1.missed_requests, u32:1);

        let tok = send(tok, request_s, status_request());
        let (tok, status_response) = recv(tok, response_r);
        assert_eq(status_response.total_hits, u32:3);
        assert_eq(status_response.total_misses, u32:1);
        assert_eq(status_response.all_hit_requests, u32:3);
        assert_eq(status_response.missed_requests, u32:1);

        let tok = send(tok, request_s, invalidating_sample_request(point, texture_layout));
        let (tok, command) = recv(tok, block_read_r);
        assert_eq(command, memory::MemoryBlockRead { address: u22:0 });
        let tok = send(
            tok, block_read_response_s,
            memory::MemoryBlockReadResponse { data: repeated_line(u32:0x44332211), success: u1:1 });
        let (tok, invalidated_point) = recv(tok, response_r);
        assert_eq(invalidated_point.hits, u1[4]:[u1:0, ...]);
        assert_eq(invalidated_point.total_hits, u32:3);
        assert_eq(invalidated_point.total_misses, u32:2);

        let tok = send(tok, request_s, invalidate_request());
        let (tok, _) = recv(tok, response_r);
        let same_line = test_sample(u26:0 ++ u26:1 ++ u26:2 ++ u26:3, u1:1);
        let tok = send(tok, request_s, sample_request(same_line, texture_layout));
        let (tok, command) = recv(tok, block_read_r);
        assert_eq(command.address, u22:0);
        let tok = send(
            tok, block_read_response_s,
            memory::MemoryBlockReadResponse { data: repeated_line(u32:0x44332211), success: u1:1 });
        let (tok, same_line_response) = recv(tok, response_r);
        assert_eq(same_line_response.hits, u1[4]:[u1:0, ...]);

        let tok = send(tok, request_s, invalidate_request());
        let (tok, _) = recv(tok, response_r);
        let four_lines = test_sample(u26:0 ++ u26:256 ++ u26:512 ++ u26:768, u1:1);
        let tok = send(tok, request_s, sample_request(four_lines, texture_layout));
        let tok = unroll_for! (line, tok): (u3, token) in u3:0..u3:4 {
            let (tok, command) = recv(tok, block_read_r);
            assert_eq(command.address, (line as u22) << u8:6);
            send(
                tok, block_read_response_s,
                memory::MemoryBlockReadResponse {
                    data: repeated_line(u32:0x44332211),
                    success: u1:1,
                })
        }(tok);
        let (tok, four_line_response) = recv(tok, response_r);
        assert_eq(four_line_response.hits, u1[4]:[u1:0, ...]);
        let tok = send(tok, terminator, true);
    }
}
