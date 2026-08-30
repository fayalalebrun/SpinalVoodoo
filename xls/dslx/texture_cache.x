#![feature(channel_attributes)]

import layout;
import memory;
import texture_sampler;

pub struct CacheRequest {
    kind: u3,
    memory_command: memory::MemoryCommand,
    sample: texture_sampler::SampleRequest,
    texture_layout: bits[432],
}

pub struct CacheResponse {
    memory_response: memory::MemoryResponse,
    texels: u16[4],
    hits: u1[4],
    total_hits: u24,
    total_misses: u24,
    all_hit_requests: u24,
    missed_requests: u24,
}

pub struct CacheState {
    valid: u1[16],
    bases: u14[16],
    second_valid: u1,
    second_base: u14,
    replace_second: u1,
    third_valid: u1,
    third_base: u14,
    replace_third: u1,
    fourth_valid: u1,
    fourth_base: u14,
    replace_fourth: u1,
    fifth_valid: u1,
    fifth_base: u14,
    sixth_valid: u1,
    sixth_base: u14,
    hits: u24,
    misses: u24,
    all_hit_requests: u24,
    missed_requests: u24,
    phase: u3,
    request: CacheRequest,
    tap: u3,
    slot: u6,
    texels: u16[4],
    request_hits: u1[4],
    unresolved: u4,
    miss_mask: u4,
    fill_active: u1,
    fill_slot: u6,
    fill_tag: u14,
    fill_start: u6,
    fill_progress: u6,
    deferred_valid: u1,
    deferred_kind: u3,
    deferred_memory_command: memory::MemoryCommand,
    deferred_sample: texture_sampler::SampleRequest,
    deferred_set: u4,
}

struct ProbeResult { slot: u6, hit: u1 }

struct LookupResult { state: CacheState, slot: u6, hit: u1 }

pub struct ParallelLookup {
    addresses: u26[4],
    locations: u4[4],
    entries: u22[4],
    active: u4,
    hits: u4,
    conflict: u1,
}

struct CacheStats {
    total_hits: u24,
    total_misses: u24,
    all_hit_requests: u24,
    missed_requests: u24,
}

pub struct ParallelRead { entries: u22[4], locations: u4[4], active: u4 }

struct ParallelRequestState { active: u1, read: ParallelRead, unresolved: u4 }

struct BankWrites { data: u32[4], masks: u4[4] }

pub struct PackedBankCommand { command: memory::MemoryCommand }

pub struct ParallelReadResult { locations: u4[4], active: u4, last: u1 }

pub struct BankRequest {
    fast: u1,
    valid: u4,
    commands: PackedBankCommand[4],
    read: ParallelReadResult,
}

pub struct FastBankRequest {
    entries: u22[4],
    locations: u4[4],
    selected: u4,
    last: u1,
}

pub struct PackedBankResponse { data: u64, success: u1 }

pub struct BankResponse { responses: PackedBankResponse[4], read: ParallelReadResult }

struct ParallelResponseJob { read: ParallelReadResult, words: u64[4], texels: u16[4] }

struct FillRequest {
    texture_layout: bits[432],
    sample: texture_sampler::SampleRequest,
    line_base: u26,
    slot: u6,
    texels: u16[4],
    unresolved: u4,
}

struct FillResult { texels: u16[4], unresolved: u4, complete: u1, word: u6 }

struct FillStreamState {
    active: u1,
    prefetched: u1,
    request: FillRequest,
    word: u6,
    texels: u16[4],
    unresolved: u4,
}

struct FillPending { bank_valid: u1, report: u1, word: u6 }

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
        locations: u4[4]:[u4:0, ...],
        ds: u4:0,
        dt: u4:0,
    }
}

fn zero_parallel_read_result() -> ParallelReadResult {
    ParallelReadResult { locations: u4[4]:[u4:0, ...], active: u4:0, last: u1:0 }
}

fn zero_parallel_read() -> ParallelRead {
    ParallelRead { entries: u22[4]:[u22:0, ...], locations: u4[4]:[u4:0, ...], active: u4:0 }
}

fn initial_parallel_request_state() -> ParallelRequestState {
    ParallelRequestState { active: u1:0, read: zero_parallel_read(), unresolved: u4:0 }
}

fn zero_fill_request() -> FillRequest {
    FillRequest {
        texture_layout: bits[432]:0,
        sample: zero_sample(),
        line_base: u26:0,
        slot: u6:0,
        texels: u16[4]:[u16:0, ...],
        unresolved: u4:0,
    }
}

pub fn memory_request(command: memory::MemoryCommand) -> CacheRequest {
    CacheRequest {
        kind: u3:0,
        memory_command: command,
        sample: zero_sample(),
        texture_layout: bits[432]:0,
    }
}

pub fn invalidate_request() -> CacheRequest {
    CacheRequest {
        kind: u3:2,
        memory_command: zero_memory_command(),
        sample: zero_sample(),
        texture_layout: bits[432]:0,
    }
}

pub fn status_request() -> CacheRequest {
    CacheRequest {
        kind: u3:3,
        memory_command: zero_memory_command(),
        sample: zero_sample(),
        texture_layout: bits[432]:0,
    }
}

pub fn initial_state() -> CacheState {
    CacheState {
        valid: u1[16]:[u1:0, ...],
        bases: u14[16]:[u14:0, ...],
        second_valid: u1:0,
        second_base: u14:0,
        replace_second: u1:0,
        third_valid: u1:0,
        third_base: u14:0,
        replace_third: u1:0,
        fourth_valid: u1:0,
        fourth_base: u14:0,
        replace_fourth: u1:0,
        fifth_valid: u1:0,
        fifth_base: u14:0,
        sixth_valid: u1:0,
        sixth_base: u14:0,
        hits: u24:0,
        misses: u24:0,
        all_hit_requests: u24:0,
        missed_requests: u24:0,
        phase: u3:0,
        request: invalidate_request(),
        tap: u3:0,
        slot: u6:0,
        texels: u16[4]:[u16:0, ...],
        request_hits: u1[4]:[u1:0, ...],
        unresolved: u4:0,
        miss_mask: u4:0,
        fill_active: u1:0,
        fill_slot: u6:0,
        fill_tag: u14:0,
        fill_start: u6:0,
        fill_progress: u6:0,
        deferred_valid: u1:0,
        deferred_kind: u3:0,
        deferred_memory_command: zero_memory_command(),
        deferred_sample: zero_sample(),
        deferred_set: u4:0,
    }
}

fn invalidate(state: CacheState) -> CacheState {
    CacheState {
        valid: u1[16]:[u1:0, ...],
        second_valid: u1:0,
        third_valid: u1:0,
        fourth_valid: u1:0,
        fifth_valid: u1:0,
        sixth_valid: u1:0,
        ..state
    }
}

fn tap_address(addresses: uN[104], tap: u3) -> u26 {
    match tap {
        u3:0 => addresses[78+:u26],
        u3:1 => addresses[52+:u26],
        u3:2 => addresses[26+:u26],
        u3:3 => addresses[0+:u26],
        _ => u26:0,
    }
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

fn location_mapping(texture_layout: bits[432], address: u26) -> (u1, u22, u4) {
    let selected =
        select_location_segment((u1:0, u22:0, u4:0), texture_layout[384+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[336+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[288+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[240+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[192+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[144+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[96+:u48], address[0+:u22]);
    let selected = select_location_segment(selected, texture_layout[48+:u48], address[0+:u22]);
    select_location_segment(selected, texture_layout[0+:u48], address[0+:u22])
}

fn location(texture_layout: bits[432], is_16_bit: u1, address: u26) -> u4 {
    let (valid, base, shift) = location_mapping(texture_layout, address);
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
        tap_address(sample.addresses, u3:0), tap_address(sample.addresses, u3:1),
        tap_address(sample.addresses, u3:2), tap_address(sample.addresses, u3:3),
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
    CacheRequest { kind: u3:1, memory_command: zero_memory_command(), sample, texture_layout }
}

pub fn invalidating_sample_request
    (sample: texture_sampler::SampleRequest, texture_layout: bits[432]) -> CacheRequest {
    CacheRequest { kind: u3:4, memory_command: zero_memory_command(), sample, texture_layout }
}

fn line_base(address: u26) -> u26 { address & u26:0x3ffff00 }

fn cache_set(address: u26) -> u4 {
    let line = address >> u8:8;
    let folded = line[0+:u4] ^ line[4+:u4] ^ line[8+:u4] ^ line[12+:u4] ^
                 (line[16+:u2] as u4);
    folded
}

fn cache_tag(address: u26) -> u14 { address[12+:u14] }

fn second_way_set(set: u4) -> u1 { set == u4:0 || set == u4:2 || set == u4:3 }

fn probe_at(state: CacheState, set: u4, tag: u14, use_second: u1) -> ProbeResult {
    let index = set as u6;
    let first_hit = state.valid[index] && state.bases[index] == tag;
    let second_hit = use_second && set == u4:0 &&
                     state.second_valid && state.second_base == tag;
    let third_hit = use_second && set == u4:2 &&
                    state.third_valid && state.third_base == tag;
    let fourth_hit = use_second && set == u4:3 &&
                     state.fourth_valid && state.fourth_base == tag;
    let fifth_hit = use_second && set == u4:0 &&
                    state.fifth_valid && state.fifth_base == tag;
    let sixth_hit = use_second && set == u4:0 &&
                    state.sixth_valid && state.sixth_base == tag;
    let set0_slot = if second_hit { u6:16 } else if fifth_hit { u6:19 } else { u6:20 };
    let victim_slot = if set == u4:0 { set0_slot } else if set == u4:2 { u6:17 } else { u6:18 };
    ProbeResult {
        slot: if first_hit { index } else { victim_slot },
        hit: first_hit || second_hit || third_hit || fourth_hit || fifth_hit || sixth_hit,
    }
}

fn probe(state: CacheState, address: u26, use_second: u1) -> ProbeResult {
    probe_at(state, cache_set(address), cache_tag(address), use_second)
}

fn allocation_slot_at(state: CacheState, set: u4, use_second: u1) -> u6 {
    let index = set as u6;
    if !use_second || !second_way_set(set) || !state.valid[index] {
        index
    } else if set == u4:0 {
        if !state.second_valid {
            u6:16
        } else if !state.fifth_valid {
            u6:19
        } else if !state.sixth_valid {
            u6:20
        } else if state.replace_second {
            u6:16
        } else {
            index
        }
    } else if set == u4:2 {
        if !state.third_valid || state.replace_third { u6:17 } else { index }
    } else {
        if !state.fourth_valid || state.replace_fourth { u6:18 } else { index }
    }
}

fn allocation_slot(state: CacheState, address: u26, use_second: u1) -> u6 {
    allocation_slot_at(state, cache_set(address), use_second)
}

fn allocate_at
    (state: CacheState, set: u4, tag: u14, use_second: u1) -> (CacheState, u6) {
    let result = probe_at(state, set, tag, use_second);
    let index = set as u6;
    let new_slot = allocation_slot_at(state, set, use_second);
    let allocated = if new_slot == u6:16 {
        CacheState {
            second_valid: u1:1,
            second_base: tag,
            replace_second: u1:0,
            ..state
        }
    } else if new_slot == u6:17 {
        CacheState {
            third_valid: u1:1,
            third_base: tag,
            replace_third: u1:0,
            ..state
        }
    } else if new_slot == u6:18 {
        CacheState {
            fourth_valid: u1:1,
            fourth_base: tag,
            replace_fourth: u1:0,
            ..state
        }
    } else if new_slot == u6:19 {
        CacheState {
            fifth_valid: u1:1,
            fifth_base: tag,
            ..state
        }
    } else if new_slot == u6:20 {
        CacheState {
            sixth_valid: u1:1,
            sixth_base: tag,
            ..state
        }
    } else {
        CacheState {
            valid: update(state.valid, index, u1:1),
            bases: update(state.bases, index, tag),
            replace_second: if set == u4:0 { u1:1 } else { state.replace_second },
            replace_third: if set == u4:2 { u1:1 } else { state.replace_third },
            replace_fourth: if set == u4:3 { u1:1 } else { state.replace_fourth },
            ..state
        }
    };
    if result.hit { (state, result.slot) } else { (allocated, new_slot) }
}

fn allocate(state: CacheState, address: u26, use_second: u1) -> (CacheState, u6) {
    allocate_at(state, cache_set(address), cache_tag(address), use_second)
}

fn lookup(state: CacheState, address: u26) -> LookupResult {
    let result = probe(state, address, u1:1);
    let (allocated, slot) = allocate(state, address, u1:1);
    let next_state = if result.hit {
        CacheState { hits: state.hits + u24:1, ..state }
    } else {
        CacheState { misses: state.misses + u24:1, ..allocated }
    };
    LookupResult { state: next_state, slot: if result.hit { result.slot } else { slot }, hit: result.hit }
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

fn sample_uses_victim(sample: texture_sampler::SampleRequest) -> u1 { sample.format == u4:10 }

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

fn count_taps(mask: u4) -> u24 {
    (mask[0+:u1] as u24) + (mask[1+:u1] as u24) + (mask[2+:u1] as u24) +
    (mask[3+:u1] as u24)
}

fn bank_texel(word: u32, pair: u1) -> u16 {
    if pair == u1:1 { word[16+:u16] } else { word[0+:u16] }
}

fn physical_bank(location: u4, entry: u22) -> u3 {
    let victim = entry[6+:u5] >= u5:16;
    ((location[1+:u2] as u3) << u8:1) |
    (if victim { u3:0 } else {
        (entry[7+:u1] ^ entry[6+:u1] ^ entry[5+:u1]) as u3
    })
}

fn fill_address_safe(state: CacheState, address: u26) -> u1 {
    let result = probe(state, address, sample_uses_victim(state.request.sample));
    let same_line = result.slot == state.fill_slot && cache_tag(address) == state.fill_tag;
    !same_line || address[2+:u6] - state.fill_start <= state.fill_progress
}

fn background_lookup_safe(state: CacheState, lookup: ParallelLookup) -> u1 {
    (!lookup.active[0+:u1] || fill_address_safe(state, lookup.addresses[0])) &&
    (!lookup.active[1+:u1] || fill_address_safe(state, lookup.addresses[1])) &&
    (!lookup.active[2+:u1] || fill_address_safe(state, lookup.addresses[2])) &&
    (!lookup.active[3+:u1] || fill_address_safe(state, lookup.addresses[3]))
}

fn bank_entry(active: u4, locations: u4[4], entries: u22[4], bank: u2) -> u22 {
    if active[0+:u1] && locations[0][1+:u2] == bank {
        entries[0]
    } else if active[1+:u1] && locations[1][1+:u2] == bank {
        entries[1]
    } else if active[2+:u1] && locations[2][1+:u2] == bank {
        entries[2]
    } else {
        entries[3]
    }
}

fn parallel_lookup(state: CacheState, request: CacheRequest) -> ParallelLookup {
    let addresses = u26[4]:[
        tap_address(request.sample.addresses, u3:0), tap_address(request.sample.addresses, u3:1),
        tap_address(request.sample.addresses, u3:2), tap_address(request.sample.addresses, u3:3),
    ];
    let locations = request.sample.locations;
    let use_victim = sample_uses_victim(request.sample);
    let probe0 = probe(state, addresses[0], use_victim);
    let probe1 = probe(state, addresses[1], use_victim);
    let probe2 = probe(state, addresses[2], use_victim);
    let probe3 = probe(state, addresses[3], use_victim);
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
    let conflict = (active[0+:u1] && active[1+:u1] && locations[0][1+:u2] == locations[1][1+:u2] &&
                   entries[0] != entries[1]) ||
                   (active[0+:u1] && active[2+:u1] && locations[0][1+:u2] == locations[2][1+:u2] &&
                   entries[0] != entries[2]) ||
                   (active[0+:u1] && active[3+:u1] && locations[0][1+:u2] == locations[3][1+:u2] &&
                   entries[0] != entries[3]) ||
                   (active[1+:u1] && active[2+:u1] && locations[1][1+:u2] == locations[2][1+:u2] &&
                   entries[1] != entries[2]) ||
                   (active[1+:u1] && active[3+:u1] && locations[1][1+:u2] == locations[3][1+:u2] &&
                   entries[1] != entries[3]) ||
                   (active[2+:u1] && active[3+:u1] && locations[2][1+:u2] == locations[3][1+:u2] &&
                   entries[2] != entries[3]);
    ParallelLookup {
        addresses,
        locations,
        entries,
        active,
        hits,
        conflict: conflict as u1,
    }
}

fn request_bank_lookup(state: CacheState, request: CacheRequest) -> ParallelLookup {
    let addresses = u26[4]:[
        tap_address(request.sample.addresses, u3:0), tap_address(request.sample.addresses, u3:1),
        tap_address(request.sample.addresses, u3:2), tap_address(request.sample.addresses, u3:3),
    ];
    let locations = request.sample.locations;
    let active = active_taps(request.sample);
    let use_victim = sample_uses_victim(request.sample);
    let entries = u22[4]:[
        ((probe(
            state, addresses[0], use_victim).slot as u22) << u8:6) |
            (addresses[0][2+:u6] as u22),
        ((probe(
            state, addresses[1], use_victim).slot as u22) << u8:6) |
            (addresses[1][2+:u6] as u22),
        ((probe(
            state, addresses[2], use_victim).slot as u22) << u8:6) |
            (addresses[2][2+:u6] as u22),
        ((probe(
            state, addresses[3], use_victim).slot as u22) << u8:6) |
            (addresses[3][2+:u6] as u22),
    ];
    ParallelLookup {
        addresses,
        locations,
        entries,
        active,
        hits: u4:0,
        conflict: u1:0,
    }
}

fn bank_requested(lookup: ParallelLookup, bank: u2) -> u1 {
    ((lookup.active[0+:u1] && lookup.locations[0][1+:u2] == bank) ||
    (lookup.active[1+:u1] && lookup.locations[1][1+:u2] == bank) ||
    (lookup.active[2+:u1] && lookup.locations[2][1+:u2] == bank) ||
    (lookup.active[3+:u1] && lookup.locations[3][1+:u2] == bank)) as
    u1
}

fn raw_texel(word: u32, is_16_bit: u1, address: u26) -> u16 {
    if is_16_bit == u1:1 {
        if address[1+:u1] == u1:1 { word[16+:u16] } else { word[0+:u16] }
    } else {
        let byte = (word >> ((address[0+:u2] as u5) << u8:3)) as u8;
        byte ++ byte
    }
}

fn packed_bank_command(command: memory::MemoryCommand) -> PackedBankCommand {
    PackedBankCommand { command }
}

fn fill_bank_command
    (slot: u6, word: u6, writes: BankWrites, bank: u2) -> PackedBankCommand {
    PackedBankCommand {
        command: memory::MemoryCommand {
            write: u1:1,
            address: ((slot as u22) << u8:6) | (word as u22),
            data: writes.data[bank],
            mask: writes.masks[bank],
        },
    }
}

fn first_bank_group(active: u4, locations: u4[4], entries: u22[4]) -> u4 {
    let selected0 = active[0+:u1];
    let selected1 = active[1+:u1] &&
                    !(selected0 &&
                      physical_bank(locations[0], entries[0]) == physical_bank(locations[1], entries[1]) &&
                      entries[0] != entries[1]);
    let selected2 = active[2+:u1] &&
                    !(selected0 &&
                      physical_bank(locations[0], entries[0]) == physical_bank(locations[2], entries[2]) &&
                      entries[0] != entries[2]) &&
                    !(selected1 &&
                      physical_bank(locations[1], entries[1]) == physical_bank(locations[2], entries[2]) &&
                      entries[1] != entries[2]);
    let selected3 = active[3+:u1] &&
                    !(selected0 &&
                      physical_bank(locations[0], entries[0]) == physical_bank(locations[3], entries[3]) &&
                      entries[0] != entries[3]) &&
                    !(selected1 &&
                      physical_bank(locations[1], entries[1]) == physical_bank(locations[3], entries[3]) &&
                      entries[1] != entries[3]) &&
                    !(selected2 &&
                      physical_bank(locations[2], entries[2]) == physical_bank(locations[3], entries[3]) &&
                      entries[2] != entries[3]);
    (selected3 as u4) << u8:3 | (selected2 as u4) << u8:2 |
    (selected1 as u4) << u8:1 | (selected0 as u4)
}

fn selected_bank(selected: u4, locations: u4[4], bank: u2) -> u1 {
    ((selected[0+:u1] && locations[0][1+:u2] == bank) ||
     (selected[1+:u1] && locations[1][1+:u2] == bank) ||
     (selected[2+:u1] && locations[2][1+:u2] == bank) ||
     (selected[3+:u1] && locations[3][1+:u2] == bank)) as u1
}

fn grouped_bank_command(lookup: ParallelLookup, selected: u4, bank: u2) -> PackedBankCommand {
    packed_bank_command(
        memory::MemoryCommand {
            write: u1:0,
            address: bank_entry(selected, lookup.locations, lookup.entries, bank),
            data: u32:0,
            mask: u4:0,
        })
}

fn grouped_bank_request(lookup: ParallelLookup, selected: u4) -> BankRequest {
    let valid = (selected_bank(selected, lookup.locations, u2:3) as u4) << u8:3 |
                (selected_bank(selected, lookup.locations, u2:2) as u4) << u8:2 |
                (selected_bank(selected, lookup.locations, u2:1) as u4) << u8:1 |
                (selected_bank(selected, lookup.locations, u2:0) as u4);
    BankRequest {
        fast: u1:0,
        valid,
        commands: PackedBankCommand[4]:[
            grouped_bank_command(lookup, selected, u2:0),
            grouped_bank_command(lookup, selected, u2:1),
            grouped_bank_command(lookup, selected, u2:2),
            grouped_bank_command(lookup, selected, u2:3),
        ],
        read: zero_parallel_read_result(),
    }
}

fn bank_taps(selected: u4, locations: u4[4], bank: u2) -> u4 {
    ((selected[3+:u1] && locations[3][1+:u2] == bank) as u4) << u8:3 |
    ((selected[2+:u1] && locations[2][1+:u2] == bank) as u4) << u8:2 |
    ((selected[1+:u1] && locations[1][1+:u2] == bank) as u4) << u8:1 |
    ((selected[0+:u1] && locations[0][1+:u2] == bank) as u4)
}

fn parallel_group_command(read: ParallelRead, selected: u4, bank: u2) -> PackedBankCommand {
    let taps = bank_taps(selected, read.locations, bank);
    let first = first_tap(taps);
    let first_entry = read.entries[first];
    let different = for (tap, mask): (u3, u4) in u3:0..u3:4 {
        mask | (((taps[tap+:u1] && read.entries[tap] != first_entry) as u4) << tap)
    }(u4:0);
    let second = first_tap(different);
    packed_bank_command(memory::MemoryCommand {
        write: u1:0,
        address: first_entry,
        data: read.entries[second] as u32,
        mask: u4:0x8 | (different != u4:0) as u4,
    })
}

fn packed_locations(read: ParallelRead, selected: u4) -> u4[4] {
    for (tap, result): (u3, u4[4]) in u3:0..u3:4 {
        let location = read.locations[tap];
        let taps = bank_taps(selected, read.locations, location[1+:u2]);
        let first = first_tap(taps);
        let second_entry = read.entries[tap] != read.entries[first];
        let position = ((second_entry as u2) << u8:1) | (location[0+:u1] as u2);
        update(result, tap, (location & u4:0x6) | (position[1+:u1] as u4) << u8:3 |
                            (position[0+:u1] as u4))
    }(read.locations)
}

fn parallel_group_request(read: ParallelRead, selected: u4, last: u1) -> BankRequest {
    let locations = packed_locations(read, selected);
    BankRequest {
        fast: u1:1,
        valid: (selected_bank(selected, read.locations, u2:3) as u4) << u8:3 |
               (selected_bank(selected, read.locations, u2:2) as u4) << u8:2 |
               (selected_bank(selected, read.locations, u2:1) as u4) << u8:1 |
               (selected_bank(selected, read.locations, u2:0) as u4),
        commands: PackedBankCommand[4]:[
            parallel_group_command(read, selected, u2:0),
            parallel_group_command(read, selected, u2:1),
            parallel_group_command(read, selected, u2:2),
            parallel_group_command(read, selected, u2:3),
        ],
        read: ParallelReadResult { locations, active: selected, last },
    }
}

fn packed_texel(word: u64, position: u2) -> u16 {
    match position {
        u2:0 => word[0+:u16],
        u2:1 => word[16+:u16],
        u2:2 => word[32+:u16],
        _ => word[48+:u16],
    }
}

fn grouped_texels
    (texels: u16[4], selected: u4, locations: u4[4], words: u64[4]) -> u16[4] {
    for (tap, result): (u3, u16[4]) in u3:0..u3:4 {
        let location = locations[tap];
        let position = location[3+:u1] ++ location[0+:u1];
        let texel = packed_texel(words[location[1+:u2]], position);
        if selected[tap+:u1] { update(result, tap, texel) } else { result }
    }(texels)
}

fn raw_grouped_texels
    (texels: u16[4], selected: u4, locations: u4[4], words: u32[4]) -> u16[4] {
    for (tap, result): (u3, u16[4]) in u3:0..u3:4 {
        let location = locations[tap];
        if selected[tap+:u1] {
            update(result, tap, bank_texel(words[location[1+:u2]], location[0+:u1]))
        } else {
            result
        }
    }(texels)
}

fn taps_outside_line(sample: texture_sampler::SampleRequest, unresolved: u4,
                     current_line: u26) -> u4 {
    for (tap, result): (u3, u4) in u3:0..u3:4 {
        let address = tap_address(sample.addresses, tap);
        let outside = unresolved[tap+:u1] && line_base(address) != current_line;
        result | ((outside as u4) << tap)
    }(u4:0)
}

fn streamed_fill_result
    (request: FillRequest, word: u6, raw: u32, texels: u16[4], unresolved: u4) -> FillResult {
    let (texels, unresolved) = for (tap, accumulator): (u3, (u16[4], u4)) in u3:0..u3:4 {
        let (texels, unresolved) = accumulator;
        let address = tap_address(request.sample.addresses, tap);
        let capture = unresolved[tap+:u1] && line_base(address) == request.line_base &&
                      address[2+:u6] == word;
        let texels = if capture {
            update(texels, tap, raw_texel(raw, request.sample.is_16_bit, address))
        } else {
            texels
        };
        (texels, unresolved & !((capture as u4) << tap))
    }((texels, unresolved));
    FillResult { texels, unresolved, complete: word == u6:63, word }
}

fn streamed_fill_bank_request(request: FillRequest, word: u6, raw: u32) -> BankRequest {
    let writes = fill_word(
        request.texture_layout, request.sample.is_16_bit,
        request.line_base + ((word as u26) << u8:2), raw);
    let valid = ((writes.masks[3] != u4:0) as u4) << u8:3 |
                ((writes.masks[2] != u4:0) as u4) << u8:2 |
                ((writes.masks[1] != u4:0) as u4) << u8:1 |
                ((writes.masks[0] != u4:0) as u4);
    BankRequest {
        fast: u1:0,
        valid,
        commands: PackedBankCommand[4]:[
            fill_bank_command(request.slot, word, writes, u2:0),
            fill_bank_command(request.slot, word, writes, u2:1),
            fill_bank_command(request.slot, word, writes, u2:2),
            fill_bank_command(request.slot, word, writes, u2:3),
        ],
        read: zero_parallel_read_result(),
    }
}


fn initial_fill_stream_state() -> FillStreamState {
    FillStreamState {
        active: u1:0,
        prefetched: u1:0,
        request: zero_fill_request(),
        word: u6:0,
        texels: u16[4]:[u16:0, ...],
        unresolved: u4:0,
    }
}

proc streaming_fill_requests<CRITICAL_FIRST: bool = {false}> {
    fill_request_r: chan<FillRequest> in;
    prefetch_r: chan<memory::MemoryBlockRead> in;
    block_read_s: chan<memory::MemoryBlockRead> out;
    block_read_response_r: chan<memory::MemoryBlockReadWordResponse> in;
    bank_request_s: chan<BankRequest> out;
    pending_s: chan<FillPending> out;
    result_s: chan<FillResult> out;

    config(fill_request_r: chan<FillRequest> in, prefetch_r: chan<memory::MemoryBlockRead> in,
           block_read_s: chan<memory::MemoryBlockRead> out,
           block_read_response_r: chan<memory::MemoryBlockReadWordResponse> in,
           bank_request_s: chan<BankRequest> out, pending_s: chan<FillPending> out,
           result_s: chan<FillResult> out) {
        (
            fill_request_r, prefetch_r, block_read_s, block_read_response_r, bank_request_s,
            pending_s, result_s,
        )
    }

    init { initial_fill_stream_state() }

    next(state: FillStreamState) {
        let starting = state.active == u1:0;
        let (tok, incoming) = recv_if(join(), fill_request_r, starting, zero_fill_request());
        let request = if starting { incoming } else { state.request };
        let sequence = if starting { u6:0 } else { state.word };
        let critical_word =
            tap_address(request.sample.addresses, first_tap(request.unresolved))[2+:u6];
        let start_word = if CRITICAL_FIRST && critical_word > u6:48 {
            critical_word
        } else {
            u6:0
        };
        let word = start_word + sequence;
        let texels = if starting { incoming.texels } else { state.texels };
        let unresolved = if starting { incoming.unresolved } else { state.unresolved };
        let continuation_mask =
            taps_outside_line(request.sample, request.unresolved, request.line_base);
        let continuation_tap = first_tap(continuation_mask);
        let continuation_address = tap_address(request.sample.addresses, continuation_tap);
        let continuation_word = continuation_address[2+:u6];
        let continuation_start_word = if CRITICAL_FIRST && continuation_word > u6:48 {
            continuation_word
        } else {
            u6:0
        };
        let continuation = memory::MemoryBlockRead {
            address: ((line_base(continuation_address) >> u8:2) as u22) +
                     (continuation_start_word as u22),
        };
        let receive_prefetch = CRITICAL_FIRST && !state.prefetched &&
                               (starting || continuation_mask == u4:0);
        let (tok, deferred_prefetch, deferred_prefetch_valid) = if receive_prefetch {
            recv_non_blocking(tok, prefetch_r, memory::MemoryBlockRead { address: u22:0 })
        } else {
            (tok, memory::MemoryBlockRead { address: u22:0 }, false)
        };
        let current_prefetched = starting && state.prefetched;
        let issue_continuation = CRITICAL_FIRST && continuation_mask != u4:0 &&
                                 ((starting && current_prefetched) ||
                                  (!starting && !state.prefetched));
        let issue_current = starting && !current_prefetched && !deferred_prefetch_valid;
        let issue_block = issue_current || issue_continuation || deferred_prefetch_valid;
        let block = if issue_continuation { continuation } else if deferred_prefetch_valid {
            deferred_prefetch
        } else {
            memory::MemoryBlockRead {
                address: ((request.line_base >> u8:2) as u22) + (start_word as u22),
            }
        };
        let tok = send_if(tok, block_read_s, issue_block, block);
        let next_prefetched = if starting {
            issue_continuation
        } else {
            state.prefetched || issue_continuation || deferred_prefetch_valid
        };
        let (tok, block_read) = recv_if(
            tok, block_read_response_r, !starting,
            memory::MemoryBlockReadWordResponse { data: u32:0, success: u1:1 });
        let bank_request = streamed_fill_bank_request(request, word, block_read.data);
        let result = streamed_fill_result(request, word, block_read.data, texels, unresolved);
        let last = !starting && sequence == u6:63;
        let critical = !starting && unresolved != u4:0 && result.unresolved == u4:0;
        let report = critical || last;
        let tok =
            send_if(tok, bank_request_s, !starting && bank_request.valid != u4:0, bank_request);
        let tok = send_if(
            tok, pending_s, !starting,
            FillPending { bank_valid: bank_request.valid != u4:0, report, word: sequence });
        let tok = send_if(
            tok, result_s, report, FillResult { complete: last, word: sequence, ..result });
        if starting {
            FillStreamState {
                active: u1:1,
                prefetched: next_prefetched,
                request,
                word: u6:0,
                texels,
                unresolved,
            }
        } else if last {
            FillStreamState {
                active: u1:0,
                prefetched: next_prefetched,
                request: zero_fill_request(),
                word: u6:0,
                texels: u16[4]:[u16:0, ...],
                unresolved: u4:0,
            }
        } else {
            FillStreamState {
                active: u1:1,
                prefetched: next_prefetched,
                request,
                word: sequence + u6:1,
                texels: result.texels,
                unresolved: result.unresolved,
            }
        }
    }
}

proc streaming_fill_responses {
    pending_r: chan<FillPending> in;
    result_r: chan<FillResult> in;
    bank_response_r: chan<BankResponse> in;
    fill_response_s: chan<FillResult> out;
    progress_s: chan<u6> out;

    config(pending_r: chan<FillPending> in, result_r: chan<FillResult> in,
           bank_response_r: chan<BankResponse> in, fill_response_s: chan<FillResult> out,
           progress_s: chan<u6> out) {
        (pending_r, result_r, bank_response_r, fill_response_s, progress_s)
    }

    init { () }

    next(state: ()) {
        let (tok, pending) = recv(join(), pending_r);
        let (tok, _) = recv_if(
            tok, bank_response_r, pending.bank_valid,
            BankResponse {
                responses: PackedBankResponse[4]:[
                    PackedBankResponse { data: u64:0, success: u1:0 }, ...
                ],
                read: zero_parallel_read_result(),
            });
        let (tok, result) = recv_if(
            tok, result_r, pending.report,
            FillResult {
                texels: u16[4]:[u16:0, ...],
                unresolved: u4:0,
                complete: u1:0,
                word: u6:0,
            });
        let tok = send_if(tok, fill_response_s, pending.report, result);
        let tok = send_if(tok, progress_s, pending.word[0+:u1] == u1:1, pending.word);
        state
    }
}

proc streaming_fill<CRITICAL_FIRST: bool = {false}> {
    config(fill_request_r: chan<FillRequest> in, prefetch_r: chan<memory::MemoryBlockRead> in,
           fill_response_s: chan<FillResult> out, progress_s: chan<u6> out,
           block_read_s: chan<memory::MemoryBlockRead> out,
           block_read_response_r: chan<memory::MemoryBlockReadWordResponse> in,
           bank_request_s: chan<BankRequest> out, bank_response_r: chan<BankResponse> in) {
        let (pending_s, pending_r) = #[channel(depth=8, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                     chan<FillPending>("fill_pending");
        let (result_s, result_r) = #[channel(depth=1, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                   chan<FillResult>("fill_result");
        spawn streaming_fill_requests<CRITICAL_FIRST>(
            fill_request_r, prefetch_r, block_read_s, block_read_response_r, bank_request_s,
            pending_s, result_s);
        spawn streaming_fill_responses(
            pending_r, result_r, bank_response_r, fill_response_s, progress_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

pub proc packed_bank_memory {
    command_r: chan<PackedBankCommand> in;
    response_s: chan<PackedBankResponse> out;
    type Lines = bits[2048][21];

    config(command_r: chan<PackedBankCommand> in, response_s: chan<PackedBankResponse> out) {
        (command_r, response_s)
    }

    init { Lines:[bits[2048]:0, ...] }

    next(lines: Lines) {
        let (tok, request) = recv(join(), command_r);
        let command = request.command;
        let slot = command.address[6+:u5];
        let shift = (command.address[0+:u6] as u12) * u12:32;
        let line = lines[slot];
        let current = (line >> shift) as u32;
        let mask = byte_mask(command.mask);
        let data =
            if command.write == u1:1 { (current & !mask) | (command.data & mask) } else { current };
        let wide_mask = (mask as bits[2048]) << shift;
        let wide_data = (data as bits[2048]) << shift;
        let line = (line & !wide_mask) | (wide_data & wide_mask);
        let second_address = command.data as u22;
        let raw_second_slot = second_address[6+:u5];
        let second_slot = if raw_second_slot < u5:21 { raw_second_slot } else { u5:0 };
        let second_shift = (second_address[0+:u6] as u12) * u12:32;
        let second_data = (lines[second_slot] >> second_shift) as u32;
        let lines = if command.write == u1:1 { update(lines, slot, line) } else { lines };
        let packed = command.write == u1:0 && command.mask[3+:u1];
        let response_data = if packed { second_data ++ current } else { data as u64 };
        let tok = send(tok, response_s, PackedBankResponse { data: response_data, success: u1:1 });
        lines
    }
}

proc texture_cache_controller<PREFETCH: bool = {false}> {
    request_r: chan<CacheRequest> in;
    response_s: chan<CacheResponse> out;
    parallel_read_s: chan<ParallelRead> out;
    parallel_stats_s: chan<CacheStats> out;
    response_route_s: chan<u1> out;
    memory_command_s: chan<memory::MemoryCommand> out;
    memory_response_r: chan<memory::MemoryResponse> in;
    fill_request_s: chan<FillRequest> out;
    prefetch_s: chan<memory::MemoryBlockRead> out;
    fill_response_r: chan<FillResult> in;
    fill_progress_r: chan<u6> in;
    bank_request_s: chan<BankRequest> out;
    bank_response_r: chan<BankResponse> in;

    config(request_r: chan<CacheRequest> in, response_s: chan<CacheResponse> out,
           parallel_read_s: chan<ParallelRead> out, parallel_stats_s: chan<CacheStats> out,
           response_route_s: chan<u1> out, memory_command_s: chan<memory::MemoryCommand> out,
           memory_response_r: chan<memory::MemoryResponse> in,
            fill_request_s: chan<FillRequest> out, prefetch_s: chan<memory::MemoryBlockRead> out,
            fill_response_r: chan<FillResult> in,
            fill_progress_r: chan<u6> in, bank_request_s: chan<BankRequest> out,
            bank_response_r: chan<BankResponse> in) {
        (
            request_r, response_s, parallel_read_s, parallel_stats_s, response_route_s,
            memory_command_s, memory_response_r, fill_request_s, prefetch_s, fill_response_r,
            fill_progress_r, bank_request_s, bank_response_r,
        )
    }

    init { initial_state() }

    next(state: CacheState) {
        let idle = state.phase == u3:0;
        let lookup_active = state.phase == u3:1;
        let fill_starting = state.phase == u3:2;
        let fill_waiting = state.phase == u3:3;
        let conflict_active = state.phase == u3:4;
        let deferred_request = CacheRequest {
            kind: state.deferred_kind,
            memory_command: state.deferred_memory_command,
            sample: state.deferred_sample,
            texture_layout: state.request.texture_layout,
        };
        let deferred_parallel = parallel_lookup(state, deferred_request);
        let deferred_safe = deferred_request.kind == u3:1 &&
                            deferred_parallel.hits == deferred_parallel.active &&
                            background_lookup_safe(state, deferred_parallel);
        let use_deferred = idle && state.deferred_valid &&
                           (!state.fill_active || deferred_safe);
        let receive_request = idle && !state.deferred_valid;
        let (tok, received, received_valid) = if receive_request {
            recv_non_blocking(join(), request_r, invalidate_request())
        } else {
            (join(), invalidate_request(), false)
        };
        let incoming = if use_deferred { deferred_request } else { received };
        let incoming_valid = use_deferred || received_valid;
        let incoming_first_address = tap_address(incoming.sample.addresses, u3:0);
        let parallel = parallel_lookup(state, incoming);
        let background_safe = incoming.kind == u3:1 && parallel.hits == parallel.active &&
                              background_lookup_safe(state, parallel);
        let process_incoming = incoming_valid && (!state.fill_active || background_safe);
        let defer_incoming = incoming_valid && state.fill_active && !background_safe;
        let incoming_lookup = process_incoming &&
                              (incoming.kind == u3:1 || incoming.kind == u3:4);
        let parallel_done = incoming_lookup && incoming.kind == u3:1 &&
                            parallel.hits == parallel.active;
        let start_conflict = false;
        let direct_miss = incoming_lookup && incoming.kind == u3:1 &&
                          parallel.hits == u4:0;
        let memory_access = process_incoming && incoming.kind == u3:0;
        let start_lookup = incoming_lookup && !parallel_done && !start_conflict && !direct_miss;
        let immediate_response = process_incoming && !incoming_lookup;
        let invalidating = process_incoming &&
                           ((incoming.kind == u3:0 && incoming.memory_command.write == u1:1) ||
                           incoming.kind == u3:2 || incoming.kind == u3:4);
        let active = active_taps(state.request.sample);
        let lookup_address = tap_address(state.request.sample.addresses, state.tap);
        let lookup_probe = probe(
            state, lookup_address, sample_uses_victim(state.request.sample));
        let lookup_hit = lookup_active && active[state.tap+:u1] && lookup_probe.hit;
        let lookup_hits = if lookup_active {
            update(state.request_hits, state.tap, lookup_hit as u1)
        } else {
            state.request_hits
        };
        let lookup_remaining = state.unresolved & !((u4:1) << state.tap);
        let lookup_last = lookup_active && lookup_remaining == u4:0;
        let hits =
            lookup_hits[u32:3] ++ lookup_hits[u32:2] ++ lookup_hits[u32:1] ++ lookup_hits[u32:0];
        let misses = active & !hits;
        let deferred_misses = parallel.active & !parallel.hits;
        let deferred_miss_tap = first_tap(deferred_misses);
        let deferred_miss_address = tap_address(incoming.sample.addresses, deferred_miss_tap);
        let deferred_miss_word = deferred_miss_address[2+:u6];
        let deferred_start_word =
            if PREFETCH && deferred_miss_word > u6:48 { deferred_miss_word } else { u6:0 };
        let prefetch_tok = send_if(
            tok, prefetch_s,
            PREFETCH && defer_incoming && incoming.kind == u3:1 && deferred_misses != u4:0,
            memory::MemoryBlockRead {
                address: ((line_base(deferred_miss_address) >> u8:2) as u22) +
                         (deferred_start_word as u22),
            });

        let memory_tok = send_if(tok, memory_command_s, memory_access, incoming.memory_command);
        let (memory_tok, memory_response) = recv_if(
            memory_tok, memory_response_r, memory_access,
            memory::MemoryResponse { data: u32:0, success: u1:1 });

        let fill_event_enabled = fill_waiting || state.fill_active;
        let (fill_wait_tok, fill_result, fill_event_valid) = if fill_event_enabled {
            recv_non_blocking(
                tok, fill_response_r,
                FillResult {
                    texels: state.texels,
                    unresolved: state.unresolved,
                    complete: u1:0,
                    word: state.fill_progress,
                })
        } else {
            (
                tok,
                FillResult {
                    texels: state.texels,
                    unresolved: state.unresolved,
                    complete: u1:0,
                    word: state.fill_progress,
                },
                false,
            )
        };

        let (fill_wait_tok, fill_progress, fill_progress_valid) = if fill_event_enabled {
            recv_non_blocking(fill_wait_tok, fill_progress_r, state.fill_progress)
        } else {
            (fill_wait_tok, state.fill_progress, false)
        };

        let lookup_loc = state.request.sample.locations[state.tap];

        let conflict_lookup = request_bank_lookup(state, state.request);
        let conflict_selected =
            first_bank_group(state.unresolved, conflict_lookup.locations, conflict_lookup.entries);
        let serial_selected = if lookup_hit { (u4:1) << state.tap } else { u4:0 };
        let bank_selected = if conflict_active { conflict_selected } else { serial_selected };
        let bank_request = grouped_bank_request(conflict_lookup, bank_selected);
        let bank_read = zero_parallel_read_result();
        let bank_tok = send_if(tok, bank_request_s, bank_request.valid != u4:0, bank_request);
        let empty_bank_response = PackedBankResponse { data: u64:0, success: u1:0 };
        let (bank_tok, bank_response) = recv_if(
            bank_tok, bank_response_r, bank_request.valid != u4:0,
            BankResponse {
                responses: PackedBankResponse[4]:[empty_bank_response, ...],
                read: bank_read,
            });
        let bank_words = u32[4]:[
            bank_response.responses[0].data as u32, bank_response.responses[1].data as u32,
            bank_response.responses[2].data as u32, bank_response.responses[3].data as u32,
        ];
        let lookup_word = bank_words[lookup_loc[1+:u2]];
        let lookup_texels = if lookup_hit {
            update(state.texels, state.tap, bank_texel(lookup_word, lookup_loc[0+:u1]))
        } else {
            state.texels
        };
        let direct_fill = lookup_last && misses != u4:0;
        let continuation_fill = fill_waiting && fill_event_valid && fill_result.complete &&
                                fill_result.unresolved != u4:0;
        let launch_mask = if direct_miss { parallel.active } else if direct_fill {
            misses
        } else if continuation_fill {
            fill_result.unresolved
        } else {
            state.unresolved
        };
        let launch_tap = first_tap(launch_mask);
        let launch_sample = if direct_miss { incoming.sample } else { state.request.sample };
        let launch_layout = if direct_miss { incoming.texture_layout } else {
            state.request.texture_layout
        };
        let launch_address = tap_address(launch_sample.addresses, launch_tap);
        let launch_set = if direct_miss && use_deferred {
            state.deferred_set
        } else {
            cache_set(launch_address)
        };
        let fill_start_tok = send_if(
            tok, fill_request_s, fill_starting || direct_miss || direct_fill || continuation_fill,
            FillRequest {
                texture_layout: launch_layout,
                sample: launch_sample,
                line_base: line_base(launch_address),
                slot: if direct_miss || direct_fill || continuation_fill {
                    allocation_slot_at(
                        state, launch_set, sample_uses_victim(launch_sample))
                } else {
                    state.slot
                },
                texels: if direct_miss { u16[4]:[u16:0, ...] } else if direct_fill {
                    lookup_texels
                } else if continuation_fill {
                    fill_result.texels
                } else {
                    state.texels
                },
                unresolved: launch_mask,
            });
        let conflict_texels = raw_grouped_texels(
            state.texels, conflict_selected, conflict_lookup.locations, bank_words);
        let conflict_unresolved = state.unresolved & !conflict_selected;
        let conflict_done = conflict_active && conflict_unresolved == u4:0;

        let fill_texels = fill_result.texels;
        let next_unresolved = fill_result.unresolved;
        let fill_done = fill_waiting && fill_event_valid && next_unresolved == u4:0;
        let lookup_done = lookup_last && misses == u4:0;
        let parallel_total_hits = state.hits + count_taps(parallel.active);
        let parallel_all_hit_requests = state.all_hit_requests + u24:1;
        let lookup_total_hits = state.hits + count_taps(active & hits);
        let lookup_total_misses = state.misses + count_taps(misses);
        let lookup_all_hit_requests = state.all_hit_requests + (misses == u4:0) as u24;
        let lookup_missed_requests = state.missed_requests + (misses != u4:0) as u24;
        let conflict_total_hits = state.hits + count_taps(active);
        let conflict_all_hit_requests = state.all_hit_requests + u24:1;
        let route_tok = send_if(tok, response_route_s, process_incoming, parallel_done);
        let parallel_tok = send_if(
            tok, parallel_read_s, parallel_done,
            ParallelRead {
                entries: parallel.entries,
                locations: parallel.locations,
                active: parallel.active,
            });
        let parallel_stats_tok = send_if(
            tok, parallel_stats_s, parallel_done,
            CacheStats {
                total_hits: parallel_total_hits,
                total_misses: state.misses,
                all_hit_requests: parallel_all_hit_requests,
                missed_requests: state.missed_requests,
            });
        let response = if fill_done {
            CacheResponse {
                memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
                texels: fill_texels,
                hits: state.request_hits,
                total_hits: state.hits,
                total_misses: state.misses,
                all_hit_requests: state.all_hit_requests,
                missed_requests: state.missed_requests,
            }
        } else if conflict_done {
            CacheResponse {
                memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
                texels: conflict_texels,
                hits: u1[4]:[
                    active[0+:u1], active[1+:u1], active[2+:u1], active[3+:u1],
                ],
                total_hits: conflict_total_hits,
                total_misses: state.misses,
                all_hit_requests: conflict_all_hit_requests,
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
        let response_tok = send_if(
            tok, response_s, immediate_response || conflict_done || lookup_done || fill_done,
            response);

        let state = if invalidating { invalidate(state) } else { state };
        let state = if use_deferred {
            CacheState { deferred_valid: u1:0, ..state }
        } else {
            state
        };
        let state = if defer_incoming {
            CacheState {
                deferred_valid: u1:1,
                deferred_kind: incoming.kind,
                deferred_memory_command: incoming.memory_command,
                deferred_sample: incoming.sample,
                deferred_set: cache_set(incoming_first_address),
                ..state
            }
        } else {
            state
        };
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
                tap: first_tap(parallel.hits),
                texels: u16[4]:[u16:0, ...],
                request_hits: u1[4]:[
                    parallel.hits[0+:u1], parallel.hits[1+:u1],
                    parallel.hits[2+:u1], parallel.hits[3+:u1],
                ],
                unresolved: if start_conflict { parallel.active } else { parallel.hits },
                ..state
            }
        } else {
            state
        };
        let state = if direct_miss {
            CacheState {
                misses: state.misses + count_taps(parallel.active),
                missed_requests: state.missed_requests + u24:1,
                miss_mask: parallel.active,
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
                    tap: first_tap(lookup_remaining),
                    texels: lookup_texels,
                    request_hits: lookup_hits,
                    unresolved: lookup_remaining,
                    ..state
                }
            }
        } else {
            state
        };
        let state = if conflict_active {
            CacheState {
                hits: if conflict_done { conflict_total_hits } else { state.hits },
                all_hit_requests: if conflict_done {
                    conflict_all_hit_requests
                } else {
                    state.all_hit_requests
                },
                texels: conflict_texels,
                unresolved: conflict_unresolved,
                ..state
            }
        } else {
            state
        };
        let allocation_mask = if direct_miss { parallel.active } else if lookup_last {
            misses
        } else {
            next_unresolved
        };
        let allocating = direct_miss || (lookup_last && misses != u4:0) || continuation_fill;
        let state = if allocating {
            let miss_tap = first_tap(allocation_mask);
            let miss_address = tap_address(state.request.sample.addresses, miss_tap);
            let use_victim = sample_uses_victim(state.request.sample);
            let miss_set = if direct_miss && use_deferred {
                state.deferred_set
            } else {
                cache_set(miss_address)
            };
            let (allocated, miss_slot) = allocate_at(
                state, miss_set, cache_tag(miss_address), use_victim);
            CacheState {
                tap: miss_tap,
                slot: miss_slot,
                unresolved: allocation_mask,
                ..allocated
            }
        } else {
            state
        };
        let next_phase = match state.phase {
            u3:0 => if direct_miss { u3:3 } else if start_conflict {
                u3:4
            } else if start_lookup {
                u3:1
            } else {
                u3:0
            },
            u3:1 => if lookup_last {
                if misses != u4:0 { u3:3 } else { u3:0 }
            } else {
                u3:1
            },
            u3:2 => u3:3,
            u3:3 => if fill_done { u3:0 } else { u3:3 },
            u3:4 => if conflict_unresolved == u4:0 { u3:0 } else { u3:4 },
            u3:5 => u3:0,
            u3:6 => u3:0,
            u3:7 => u3:0,
        };
        let state = if fill_waiting && fill_event_valid {
            CacheState {
                miss_mask: next_unresolved,
                texels: fill_texels,
                unresolved: next_unresolved,
                ..state
            }
        } else {
            state
        };
        let fill_address = tap_address(state.request.sample.addresses, state.tap);
        let fill_critical_word = fill_address[2+:u6];
        let fill_start =
            if PREFETCH && fill_critical_word > u6:48 { fill_critical_word } else { u6:0 };
        let state = if fill_done && !fill_result.complete {
            CacheState {
                fill_active: u1:1,
                fill_slot: state.slot,
                fill_tag: cache_tag(fill_address),
                fill_start,
                fill_progress: fill_result.word,
                ..state
            }
        } else if state.fill_active && fill_event_valid {
            CacheState {
                fill_active: !fill_result.complete,
                fill_progress: fill_result.word,
                ..state
            }
        } else {
            state
        };
        let state = if state.fill_active && fill_progress_valid {
            CacheState { fill_progress, ..state }
        } else {
            state
        };
        CacheState { phase: next_phase, ..state }
    }
}

proc vector_parallel_requests {
    read_r: chan<ParallelRead> in;
    request_s: chan<FastBankRequest> out;

    config(read_r: chan<ParallelRead> in, request_s: chan<FastBankRequest> out) {
        (read_r, request_s)
    }

    init { initial_parallel_request_state() }

    next(state: ParallelRequestState) {
        let starting = state.active == u1:0;
        let (tok, incoming) = recv_if(join(), read_r, starting, zero_parallel_read());
        let read = if starting { incoming } else { state.read };
        let unresolved = if starting { incoming.active } else { state.unresolved };
        let selected = first_bank_group(unresolved, read.locations, read.entries);
        let remaining = unresolved & !selected;
        let tok = send(
            tok, request_s,
            FastBankRequest {
                entries: read.entries,
                locations: read.locations,
                selected,
                last: remaining == u4:0,
            });
        if remaining == u4:0 {
            initial_parallel_request_state()
        } else {
            ParallelRequestState { active: u1:1, read, unresolved: remaining }
        }
    }
}

proc vector_parallel_response_start {
    response_r: chan<BankResponse> in;
    job_s: chan<ParallelResponseJob> out;

    config(response_r: chan<BankResponse> in, job_s: chan<ParallelResponseJob> out) {
        (response_r, job_s)
    }

    init { () }

    next(state: ()) {
        let (tok, response) = recv(join(), response_r);
        let words = u64[4]:[
            response.responses[0].data, response.responses[1].data,
            response.responses[2].data, response.responses[3].data,
        ];
        let tok = send(
            tok, job_s,
            ParallelResponseJob { read: response.read, words, texels: u16[4]:[u16:0, ...] });
        state
    }
}

proc vector_parallel_decode<TAP: u32> {
    job_r: chan<ParallelResponseJob> in;
    job_s: chan<ParallelResponseJob> out;

    config(job_r: chan<ParallelResponseJob> in, job_s: chan<ParallelResponseJob> out) {
        (job_r, job_s)
    }

    init { () }

    next(state: ()) {
        let (tok, job) = recv(join(), job_r);
        let location = job.read.locations[TAP];
        let position = location[3+:u1] ++ location[0+:u1];
        let texel = if job.read.active[TAP+:u1] {
            packed_texel(job.words[location[1+:u2]], position)
        } else {
            u16:0
        };
        let tok =
            send(tok, job_s, ParallelResponseJob { texels: update(job.texels, TAP, texel), ..job });
        state
    }
}

proc vector_parallel_responses {
    response_r: chan<BankResponse> in;
    stats_r: chan<CacheStats> in;
    response_s: chan<CacheResponse> out;

    config(response_r: chan<BankResponse> in, stats_r: chan<CacheStats> in,
           response_s: chan<CacheResponse> out) {
        (response_r, stats_r, response_s)
    }

    init { (u16[4]:[u16:0, ...], u4:0) }

    next(state: (u16[4], u4)) {
        let (tok, response) = recv(join(), response_r);
        let words = u64[4]:[
            response.responses[0].data, response.responses[1].data,
            response.responses[2].data, response.responses[3].data,
        ];
        let texels = grouped_texels(
            state.0, response.read.active, response.read.locations, words);
        let active = state.1 | response.read.active;
        let last = response.read.last;
        let (tok, stats) = recv_if(
            tok, stats_r, last,
            CacheStats {
                total_hits: u24:0,
                total_misses: u24:0,
                all_hit_requests: u24:0,
                missed_requests: u24:0,
            });
        let tok = send_if(
            tok, response_s, last,
            CacheResponse {
                memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
                texels,
                hits: u1[4]:[
                    active[0+:u1], active[1+:u1], active[2+:u1], active[3+:u1],
                ],
                total_hits: stats.total_hits,
                total_misses: stats.total_misses,
                all_hit_requests: stats.all_hit_requests,
                missed_requests: stats.missed_requests,
            });
        if last { (u16[4]:[u16:0, ...], u4:0) } else { (texels, active) }
    }
}

proc bank_request_arbiter {
    slow_r: chan<BankRequest> in;
    fast_r: chan<BankRequest> in;
    request_s: chan<BankRequest> out;

    config(slow_r: chan<BankRequest> in, fast_r: chan<BankRequest> in,
           request_s: chan<BankRequest> out) {
        (slow_r, fast_r, request_s)
    }

    init { () }

    next(state: ()) {
        let empty_command = packed_bank_command(zero_memory_command());
        let empty_read = zero_parallel_read_result();
        let empty = BankRequest {
            fast: u1:0,
            valid: u4:0,
            commands: PackedBankCommand[4]:[empty_command, ...],
            read: empty_read,
        };
        let (tok, slow, slow_valid) = recv_non_blocking(join(), slow_r, empty);
        let (tok, fast, fast_valid) =
            if slow_valid { (tok, empty, false) } else { recv_non_blocking(tok, fast_r, empty) };
        let tok =
            send_if(tok, request_s, slow_valid || fast_valid, if slow_valid { slow } else { fast });
        state
    }
}

proc slow_bank_request_arbiter {
    controller_r: chan<BankRequest> in;
    fill_r: chan<BankRequest> in;
    request_s: chan<BankRequest> out;
    route_s: chan<u1> out;

    config(controller_r: chan<BankRequest> in, fill_r: chan<BankRequest> in,
           request_s: chan<BankRequest> out, route_s: chan<u1> out) {
        (controller_r, fill_r, request_s, route_s)
    }

    init { () }

    next(state: ()) {
        let empty_command = packed_bank_command(zero_memory_command());
        let empty = BankRequest {
            fast: u1:0,
            valid: u4:0,
            commands: PackedBankCommand[4]:[empty_command, ...],
            read: zero_parallel_read_result(),
        };
        let (tok, controller, controller_valid) = recv_non_blocking(join(), controller_r, empty);
        let (tok, fill, fill_valid) = if controller_valid {
            (tok, empty, false)
        } else {
            recv_non_blocking(tok, fill_r, empty)
        };
        let valid = controller_valid || fill_valid;
        let tok = send_if(tok, request_s, valid, if controller_valid { controller } else { fill });
        let tok = send_if(tok, route_s, valid, fill_valid as u1);
        state
    }
}

proc slow_bank_response_router {
    route_r: chan<u1> in;
    response_r: chan<BankResponse> in;
    controller_s: chan<BankResponse> out;
    fill_s: chan<BankResponse> out;

    config(route_r: chan<u1> in, response_r: chan<BankResponse> in,
           controller_s: chan<BankResponse> out, fill_s: chan<BankResponse> out) {
        (route_r, response_r, controller_s, fill_s)
    }

    init { () }

    next(state: ()) {
        let (tok, fill) = recv(join(), route_r);
        let (tok, response) = recv(tok, response_r);
        let tok = send_if(tok, controller_s, fill == u1:0, response);
        let tok = send_if(tok, fill_s, fill == u1:1, response);
        state
    }
}

proc cache_response_arbiter {
    route_r: chan<u1> in;
    slow_r: chan<CacheResponse> in;
    fast_r: chan<CacheResponse> in;
    response_s: chan<CacheResponse> out;

    config(route_r: chan<u1> in, slow_r: chan<CacheResponse> in, fast_r: chan<CacheResponse> in,
           response_s: chan<CacheResponse> out) {
        (route_r, slow_r, fast_r, response_s)
    }

    init { () }

    next(state: ()) {
        let (tok, fast) = recv(join(), route_r);
        let (tok, response) = if fast == u1:1 { recv(tok, fast_r) } else { recv(tok, slow_r) };
        let tok = send(tok, response_s, response);
        state
    }
}

proc bank_vector_requests {
    request_r: chan<BankRequest> in;
    pending_s: chan<BankRequest> out;
    bank0_s: chan<PackedBankCommand> out;
    bank1_s: chan<PackedBankCommand> out;
    bank2_s: chan<PackedBankCommand> out;
    bank3_s: chan<PackedBankCommand> out;

    config(request_r: chan<BankRequest> in, pending_s: chan<BankRequest> out,
           bank0_s: chan<PackedBankCommand> out, bank1_s: chan<PackedBankCommand> out,
           bank2_s: chan<PackedBankCommand> out, bank3_s: chan<PackedBankCommand> out) {
        (request_r, pending_s, bank0_s, bank1_s, bank2_s, bank3_s)
    }

    init { () }

    next(state: ()) {
        let (tok, request) = recv(join(), request_r);
        let tok0 = send_if(tok, bank0_s, request.valid[0+:u1], request.commands[0]);
        let tok1 = send_if(tok, bank1_s, request.valid[1+:u1], request.commands[1]);
        let tok2 = send_if(tok, bank2_s, request.valid[2+:u1], request.commands[2]);
        let tok3 = send_if(tok, bank3_s, request.valid[3+:u1], request.commands[3]);
        let tok = send(join(tok0, tok1, tok2, tok3), pending_s, request);
        state
    }
}

proc bank_vector_responses {
    pending_r: chan<BankRequest> in;
    bank0_r: chan<PackedBankResponse> in;
    bank1_r: chan<PackedBankResponse> in;
    bank2_r: chan<PackedBankResponse> in;
    bank3_r: chan<PackedBankResponse> in;
    slow_s: chan<BankResponse> out;
    fast_s: chan<BankResponse> out;

    config(pending_r: chan<BankRequest> in, bank0_r: chan<PackedBankResponse> in,
           bank1_r: chan<PackedBankResponse> in, bank2_r: chan<PackedBankResponse> in,
           bank3_r: chan<PackedBankResponse> in, slow_s: chan<BankResponse> out,
           fast_s: chan<BankResponse> out) {
        (pending_r, bank0_r, bank1_r, bank2_r, bank3_r, slow_s, fast_s)
    }

    init { () }

    next(state: ()) {
        let (tok, request) = recv(join(), pending_r);
        let empty = PackedBankResponse { data: u64:0, success: u1:0 };
        let (tok0, bank0) = recv_if(tok, bank0_r, request.valid[0+:u1], empty);
        let (tok1, bank1) = recv_if(tok, bank1_r, request.valid[1+:u1], empty);
        let (tok2, bank2) = recv_if(tok, bank2_r, request.valid[2+:u1], empty);
        let (tok3, bank3) = recv_if(tok, bank3_r, request.valid[3+:u1], empty);
        let response = BankResponse {
            responses: PackedBankResponse[4]:[bank0, bank1, bank2, bank3],
            read: request.read,
        };
        let tok = send_if(join(tok0, tok1, tok2, tok3), slow_s, request.fast == u1:0, response);
        let tok = send_if(tok, fast_s, request.fast == u1:1, response);
        state
    }
}

proc bank_vector_adapter {
    config(request_r: chan<BankRequest> in, slow_s: chan<BankResponse> out,
           fast_s: chan<BankResponse> out, bank0_s: chan<PackedBankCommand> out,
           bank0_r: chan<PackedBankResponse> in, bank1_s: chan<PackedBankCommand> out,
           bank1_r: chan<PackedBankResponse> in, bank2_s: chan<PackedBankCommand> out,
           bank2_r: chan<PackedBankResponse> in, bank3_s: chan<PackedBankCommand> out,
           bank3_r: chan<PackedBankResponse> in) {
        let (pending_s, pending_r) = #[channel(depth=32, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                     chan<BankRequest>("bank_pending");
        spawn bank_vector_requests(request_r, pending_s, bank0_s, bank1_s, bank2_s, bank3_s);
        spawn bank_vector_responses(pending_r, bank0_r, bank1_r, bank2_r, bank3_r, slow_s, fast_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

pub proc texture_cache_vector<CRITICAL_FIRST: bool = {false}> {
    config(request_r: chan<CacheRequest> in, response_s: chan<CacheResponse> out,
           memory_command_s: chan<memory::MemoryCommand> out,
           memory_response_r: chan<memory::MemoryResponse> in,
           block_read_s: chan<memory::MemoryBlockRead> out,
           block_read_response_r: chan<memory::MemoryBlockReadWordResponse> in,
           slow_bank_request_s: chan<BankRequest> out,
           fast_bank_request_s: chan<FastBankRequest> out,
           slow_bank_response_r: chan<BankResponse> in, fast_bank_response_r: chan<BankResponse> in) {
        let (slow_response_s, slow_response_r) = #[channel(depth=0)]
                                                 chan<CacheResponse>("slow_response");
        let (fast_response_s, fast_response_r) = #[channel(depth=0)]
                                                 chan<CacheResponse>("fast_response");
        let (parallel_read_s, parallel_read_r) = #[channel(depth=0)]
                                                 chan<ParallelRead>("parallel_read");
        let (parallel_stats_s, parallel_stats_r) = #[channel(depth=1, bypass=true, register_push_outputs=false, register_pop_outputs=false)]
                                                   chan<CacheStats>("parallel_stats");

        let (response_route_s, response_route_r) = #[channel(depth=8, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                                    chan<u1>("response_route");
        let (fill_request_s, fill_request_r) = #[channel(depth=0)]
                                               chan<FillRequest>("fill_request");
        let (prefetch_s, prefetch_r) = #[channel(depth=1, bypass=true, register_push_outputs=false, register_pop_outputs=false)]
                                       chan<memory::MemoryBlockRead>("fill_prefetch");
        let (fill_response_s, fill_response_r) = #[channel(depth=0)]
                                                 chan<FillResult>("fill_response");
        let (fill_progress_s, fill_progress_r) = #[channel(depth=8, bypass=true, register_push_outputs=false, register_pop_outputs=false)]
                                                 chan<u6>("fill_progress");

        let (controller_bank_request_s, controller_bank_request_r) = #[channel(depth=0)]
                                                                     chan<BankRequest>("controller_bank_request");
        let (controller_bank_response_s, controller_bank_response_r) = #[channel(depth=0)]
                                                                       chan<BankResponse>("controller_bank_response");
        let (fill_bank_request_s, fill_bank_request_r) = #[channel(depth=0)]
                                                         chan<BankRequest>("fill_bank_request");
        let (fill_bank_response_s, fill_bank_response_r) = #[channel(depth=0)]
                                                           chan<BankResponse>("fill_bank_response");
        let (slow_bank_route_s, slow_bank_route_r) = #[channel(depth=8, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                                     chan<u1>("slow_bank_route");

        spawn texture_cache_controller<CRITICAL_FIRST>(
            request_r, slow_response_s, parallel_read_s, parallel_stats_s, response_route_s,
            memory_command_s, memory_response_r, fill_request_s, prefetch_s, fill_response_r,
            fill_progress_r, controller_bank_request_s, controller_bank_response_r);
        spawn streaming_fill<CRITICAL_FIRST>(
            fill_request_r, prefetch_r, fill_response_s, fill_progress_s, block_read_s,
            block_read_response_r, fill_bank_request_s, fill_bank_response_r);
        spawn slow_bank_request_arbiter(
            controller_bank_request_r, fill_bank_request_r, slow_bank_request_s, slow_bank_route_s);
        spawn slow_bank_response_router(
            slow_bank_route_r, slow_bank_response_r, controller_bank_response_s,
            fill_bank_response_s);
        spawn vector_parallel_requests(parallel_read_r, fast_bank_request_s);
        spawn vector_parallel_responses(fast_bank_response_r, parallel_stats_r, fast_response_s);
        spawn cache_response_arbiter(response_route_r, slow_response_r, fast_response_r, response_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

proc fast_bank_request_adapter {
    request_r: chan<FastBankRequest> in;
    request_s: chan<BankRequest> out;

    config(request_r: chan<FastBankRequest> in, request_s: chan<BankRequest> out) {
        (request_r, request_s)
    }

    init { () }

    next(state: ()) {
        let (tok, request) = recv(join(), request_r);
        let read = ParallelRead {
            entries: request.entries,
            locations: request.locations,
            active: request.selected,
        };
        let tok = send(
            tok, request_s,
            parallel_group_request(read, request.selected, request.last));
        state
    }
}

pub proc texture_cache<CRITICAL_FIRST: bool = {false}> {
    config(request_r: chan<CacheRequest> in, response_s: chan<CacheResponse> out,
           memory_command_s: chan<memory::MemoryCommand> out,
           memory_response_r: chan<memory::MemoryResponse> in,
           block_read_s: chan<memory::MemoryBlockRead> out,
           block_read_response_r: chan<memory::MemoryBlockReadWordResponse> in,
           bank0_command_s: chan<PackedBankCommand> out,
           bank0_response_r: chan<PackedBankResponse> in,
           bank1_command_s: chan<PackedBankCommand> out,
           bank1_response_r: chan<PackedBankResponse> in,
           bank2_command_s: chan<PackedBankCommand> out,
           bank2_response_r: chan<PackedBankResponse> in,
           bank3_command_s: chan<PackedBankCommand> out,
           bank3_response_r: chan<PackedBankResponse> in) {
        let (slow_bank_request_s, slow_bank_request_r) = chan<BankRequest>("slow_bank_request");
        let (fast_bank_request_s, fast_bank_request_r) = chan<FastBankRequest>("fast_bank_request");
        let (converted_fast_s, converted_fast_r) = chan<BankRequest>("converted_fast_request");
        let (bank_request_s, bank_request_r) = chan<BankRequest>("bank_request");
        let (slow_bank_response_s, slow_bank_response_r) = chan<BankResponse>("slow_bank_response");
        let (fast_bank_response_s, fast_bank_response_r) = chan<BankResponse>("fast_bank_response");
        spawn texture_cache_vector<CRITICAL_FIRST>(
            request_r, response_s, memory_command_s, memory_response_r, block_read_s,
            block_read_response_r, slow_bank_request_s, fast_bank_request_s, slow_bank_response_r,
            fast_bank_response_r);
        spawn fast_bank_request_adapter(fast_bank_request_r, converted_fast_s);
        spawn bank_request_arbiter(slow_bank_request_r, converted_fast_r, bank_request_s);
        spawn bank_vector_adapter(
            bank_request_r, slow_bank_response_s, fast_bank_response_s, bank0_command_s,
            bank0_response_r, bank1_command_s, bank1_response_r, bank2_command_s, bank2_response_r,
            bank3_command_s, bank3_response_r);
        ()
    }

    init { () }

    next(state: ()) { state }
}

#[test]
fn lookup_uses_second_way_only_for_selected_set() {
    assert_eq(line_base(u26:0x0000ff), u26:0x000000);
    assert_eq(line_base(u26:0x000100), u26:0x000100);
    assert_eq(cache_set(u26:0x000000), u4:0);
    assert_eq(cache_set(u26:0x000f00), u4:0xf);
    assert_eq(cache_set(u26:0x001100), u4:0);
    let first = lookup(initial_state(), u26:0x000000);
    assert_eq((first.slot, first.hit), (u6:0, u1:0));
    let independent = lookup(first.state, u26:0x000100);
    assert_eq((independent.slot, independent.hit), (u6:1, u1:0));
    let direct_replace = lookup(independent.state, u26:0x001000);
    assert_eq((direct_replace.slot, direct_replace.hit), (u6:1, u1:0));
    let replace = lookup(direct_replace.state, u26:0x001100);
    assert_eq((replace.slot, replace.hit), (u6:16, u1:0));
    let hit = lookup(replace.state, u26:0x001100);
    assert_eq((hit.slot, hit.hit), (u6:16, u1:1));
    assert_eq(probe(replace.state, u26:0x001100, u1:0).hit, u1:0);
    let set0_third = lookup(hit.state, u26:0x002200);
    assert_eq((set0_third.slot, set0_third.hit), (u6:19, u1:0));
    let set0_fourth = lookup(set0_third.state, u26:0x003300);
    assert_eq((set0_fourth.slot, set0_fourth.hit), (u6:20, u1:0));
    let evicted = lookup(set0_fourth.state, u26:0x000000);
    assert_eq((evicted.slot, evicted.hit), (u6:0, u1:1));
    let set2_first = lookup(evicted.state, u26:0x000200);
    let set2_second = lookup(set2_first.state, u26:0x001300);
    assert_eq((set2_second.slot, set2_second.hit), (u6:17, u1:0));
    assert_eq(
        probe(set2_second.state, u26:0x001300, u1:1),
        ProbeResult { slot: u6:17, hit: u1:1 });
    assert_eq(probe(invalidate(set2_second.state), u26:0x001300, u1:1).hit, u1:0);
    let set3_first = lookup(set2_second.state, u26:0x000300);
    let set3_second = lookup(set3_first.state, u26:0x001200);
    assert_eq((set3_second.slot, set3_second.hit), (u6:18, u1:0));
    assert_eq(
        probe(set3_second.state, u26:0x001200, u1:1),
        ProbeResult { slot: u6:18, hit: u1:1 });
}

#[test]
fn generated_locations_match_layout_mapping() {
    let texture_layout = layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:2, u2:1, u1:1, u1:0);
    let sample = texture_sampler::sample_request(
        s48:18 << u8:18, s48:24 << u8:18, s48:1 << u8:30, s32:0, s32:0, s32:0, s32:0,
        u32:0x000012cf, u32:0x00300104, texture_layout, u24:0, u24:0, u24:0, u24:0);
    assert_eq(sample.locations, sample_locations(sample, texture_layout));
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

fn test_sample(addresses: uN[104], bilinear: u1) -> texture_sampler::SampleRequest {
    let texture_layout = layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:2, u2:0, u1:1, u1:0);
    let sample = texture_sampler::SampleRequest {
        enabled: u1:1,
        bilinear,
        is_16_bit: u1:0,
        format: u4:2,
        addresses,
        locations: u4[4]:[u4:0, ...],
        ds: u4:0,
        dt: u4:0,
    };
    texture_sampler::SampleRequest { locations: sample_locations(sample, texture_layout), ..sample }
}

#[test]
fn streamed_fill_maps_and_captures_word() {
    let texture_layout = layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:2, u2:0, u1:1, u1:0);
    let request = FillRequest {
        texture_layout,
        sample: test_sample(u26:0 ++ u26:1 ++ u26:2 ++ u26:3, u1:1),
        line_base: u26:0,
        slot: u6:5,
        texels: u16[4]:[u16:0, ...],
        unresolved: u4:0xf,
    };
    let untouched =
        streamed_fill_result(request, u6:1, u32:0x44332211, request.texels, request.unresolved);
    assert_eq(
        untouched,
        FillResult {
            texels: request.texels,
            unresolved: u4:0xf,
            complete: u1:0,
            word: u6:1,
        });
    let result =
        streamed_fill_result(request, u6:0, u32:0x44332211, request.texels, request.unresolved);
    assert_eq(
        result,
        FillResult {
            texels: u16[4]:[u16:0x1111, u16:0x2222, u16:0x3333, u16:0x4444],
            unresolved: u4:0,
            complete: u1:0,
            word: u6:0,
        });
    let bank_request = streamed_fill_bank_request(request, u6:0, u32:0x44332211);
    let writes = fill_word(texture_layout, u1:0, u26:0, u32:0x44332211);
    assert_eq(bank_request.valid, u4:0x3);
    assert_eq(
        bank_request.commands[0], fill_bank_command(u6:5, u6:0, writes, u2:0));
    assert_eq(
        bank_request.commands[1], fill_bank_command(u6:5, u6:0, writes, u2:1));
}

#[test]
fn same_bank_same_entry_is_parallel() {
    let texture_layout = layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:2, u2:0, u1:1, u1:0);
    let state = initial_state();
    let state = CacheState {
        valid: update(state.valid, u32:0, u1:1),
        bases: update(state.bases, u32:0, u14:0),
        ..state
    };
    let result =
        parallel_lookup(state, sample_request(test_sample(uN[104]:0, u1:1), texture_layout));
    assert_eq(result.hits, u4:0xf);
    assert_eq(result.conflict, u1:0);
}

#[test]
fn groups_conflicting_bank_reads_without_dropping_taps() {
    let locations = u4[4]:[u4:0, u4:0, u4:2, u4:2];
    let entries = u22[4]:[u22:0, u22:1, u22:2, u22:2];
    let first = first_bank_group(u4:0xf, locations, entries);
    assert_eq(first, u4:0xd);
    assert_eq(first_bank_group(u4:0xf & !first, locations, entries), u4:0x2);
    let victim_entries = u22[4]:[u22:0x400, u22:0x420, u22:0, u22:0];
    assert_eq(first_bank_group(u4:0x3, u4[4]:[u4:0, ...], victim_entries), u4:0x1);
}

#[test_proc]
proc cache_traffic_matches_de10_geometry {
    request_s: chan<CacheRequest> out;
    response_r: chan<CacheResponse> in;
    memory_command_r: chan<memory::MemoryCommand> in;
    memory_response_s: chan<memory::MemoryResponse> out;
    block_read_r: chan<memory::MemoryBlockRead> in;
    block_read_response_s: chan<memory::MemoryBlockReadWordResponse> out;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (request_s, request_r) = chan<CacheRequest>("request");
        let (response_s, response_r) = chan<CacheResponse>("response");
        let (memory_command_s, memory_command_r) = chan<memory::MemoryCommand>("memory_command");
        let (memory_response_s, memory_response_r) =
            chan<memory::MemoryResponse>("memory_response");
        let (block_read_s, block_read_r) = chan<memory::MemoryBlockRead>("block_read");
        let (block_read_response_s, block_read_response_r) =
            chan<memory::MemoryBlockReadWordResponse>("block_read_response");
        let (bank0_command_s, bank0_command_r) = chan<PackedBankCommand>("bank0_command");
        let (bank0_response_s, bank0_response_r) = chan<PackedBankResponse>("bank0_response");
        let (bank1_command_s, bank1_command_r) = chan<PackedBankCommand>("bank1_command");
        let (bank1_response_s, bank1_response_r) = chan<PackedBankResponse>("bank1_response");
        let (bank2_command_s, bank2_command_r) = chan<PackedBankCommand>("bank2_command");
        let (bank2_response_s, bank2_response_r) = chan<PackedBankResponse>("bank2_response");
        let (bank3_command_s, bank3_command_r) = chan<PackedBankCommand>("bank3_command");
        let (bank3_response_s, bank3_response_r) = chan<PackedBankResponse>("bank3_response");
        spawn packed_bank_memory(bank0_command_r, bank0_response_s);
        spawn packed_bank_memory(bank1_command_r, bank1_response_s);
        spawn packed_bank_memory(bank2_command_r, bank2_response_s);
        spawn packed_bank_memory(bank3_command_r, bank3_response_s);
        spawn texture_cache(
            request_r, response_s, memory_command_s, memory_response_r, block_read_s,
            block_read_response_r, bank0_command_s, bank0_response_r, bank1_command_s,
            bank1_response_r, bank2_command_s, bank2_response_r, bank3_command_s,
            bank3_response_r);
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
        let tok = unroll_for! (_, tok): (u7, token) in u7:0..u7:64 {
            send(
                tok, block_read_response_s,
                memory::MemoryBlockReadWordResponse { data: u32:0x44332211, success: u1:1 })
        }(tok);
        let (tok, cold_point) = recv(tok, response_r);
        assert_eq(cold_point.hits, u1[4]:[u1:0, ...]);
        assert_eq(cold_point.texels[u32:0], u16:0x1111);
        assert_eq(cold_point.all_hit_requests, u24:0);
        assert_eq(cold_point.missed_requests, u24:1);

        let tok = send(tok, request_s, sample_request(point, texture_layout));
        let (tok, hot_point) = recv(tok, response_r);
        assert_eq(hot_point.hits, u1[4]:[u1:1, u1:0, u1:0, u1:0]);
        assert_eq(hot_point.texels[u32:0], u16:0x1111);
        assert_eq(hot_point.all_hit_requests, u24:1);
        assert_eq(hot_point.missed_requests, u24:1);

        let tok = send(tok, request_s, sample_request(point, texture_layout));
        let tok = send(tok, request_s, sample_request(point, texture_layout));
        let (tok, hot_point0) = recv(tok, response_r);
        let (tok, hot_point1) = recv(tok, response_r);
        assert_eq(hot_point0.hits, u1[4]:[u1:1, u1:0, u1:0, u1:0]);
        assert_eq(hot_point1.hits, u1[4]:[u1:1, u1:0, u1:0, u1:0]);
        assert_eq(hot_point1.all_hit_requests, u24:3);
        assert_eq(hot_point1.missed_requests, u24:1);

        let tok = send(tok, request_s, status_request());
        let (tok, status_response) = recv(tok, response_r);
        assert_eq(status_response.total_hits, u24:3);
        assert_eq(status_response.total_misses, u24:1);
        assert_eq(status_response.all_hit_requests, u24:3);
        assert_eq(status_response.missed_requests, u24:1);

        let tok = send(tok, request_s, invalidating_sample_request(point, texture_layout));
        let (tok, command) = recv(tok, block_read_r);
        assert_eq(command, memory::MemoryBlockRead { address: u22:0 });
        let tok = unroll_for! (_, tok): (u7, token) in u7:0..u7:64 {
            send(
                tok, block_read_response_s,
                memory::MemoryBlockReadWordResponse { data: u32:0x44332211, success: u1:1 })
        }(tok);
        let (tok, invalidated_point) = recv(tok, response_r);
        assert_eq(invalidated_point.hits, u1[4]:[u1:0, ...]);
        assert_eq(invalidated_point.total_hits, u24:3);
        assert_eq(invalidated_point.total_misses, u24:2);

        let tok = send(tok, request_s, invalidate_request());
        let (tok, _) = recv(tok, response_r);
        let same_line = test_sample(u26:0 ++ u26:1 ++ u26:2 ++ u26:3, u1:1);
        let tok = send(tok, request_s, sample_request(same_line, texture_layout));
        let (tok, command) = recv(tok, block_read_r);
        assert_eq(command.address, u22:0);
        let tok = unroll_for! (_, tok): (u7, token) in u7:0..u7:64 {
            send(
                tok, block_read_response_s,
                memory::MemoryBlockReadWordResponse { data: u32:0x44332211, success: u1:1 })
        }(tok);
        let (tok, same_line_response) = recv(tok, response_r);
        assert_eq(same_line_response.hits, u1[4]:[u1:0, ...]);

        let tok = send(tok, request_s, invalidate_request());
        let (tok, _) = recv(tok, response_r);
        let four_lines = test_sample(u26:0 ++ u26:256 ++ u26:512 ++ u26:768, u1:1);
        let tok = send(tok, request_s, sample_request(four_lines, texture_layout));
        let tok = unroll_for! (line, tok): (u3, token) in u3:0..u3:4 {
            let (tok, command) = recv(tok, block_read_r);
            assert_eq(command.address, (line as u22) << u8:6);
            unroll_for! (_, tok): (u7, token) in u7:0..u7:64 {
                send(
                    tok, block_read_response_s,
                    memory::MemoryBlockReadWordResponse {
                        data: (line as u32) * u32:0x11111111 + u32:0x11111111,
                        success: u1:1,
                    })
            }(tok)
        }(tok);
        let (tok, four_line_response) = recv(tok, response_r);
        assert_eq(four_line_response.hits, u1[4]:[u1:0, ...]);
        assert_eq(four_line_response.texels, u16[4]:[u16:0x1111, u16:0x2222, u16:0x3333, u16:0x4444]);
        let tok = send(tok, request_s, sample_request(four_lines, texture_layout));
        let (tok, four_line_hot) = recv(tok, response_r);
        assert_eq(four_line_hot.texels, four_line_response.texels);
        let tok = send(tok, terminator, true);
    }
}
