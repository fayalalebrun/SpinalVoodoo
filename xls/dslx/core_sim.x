#![feature(channel_attributes)]

import banked_memory;
import control;
import de10_adapter;
import dither;
import fastfill_state;
import float_alias;
import interpolator;
import layout;
import memory;
import pixel;
import raster;
import raster_state;
import tmu;
import texture_cache;
import texture_sampler;

pub struct CoreCommand { write: u1, address: u24, data: u32, mask: u4 }

pub struct De10MmioRequest { address: u22, read: u1, write: u1, byte_enable: u4, write_data: u32 }

pub struct TextureBurst { commands: CoreCommand[32] }

pub struct CoreResponse { data: u32, success: u1 }

pub struct ScanoutState { base: u26, pixel_stride: u11, displayed: u1, swap_count: u2 }

pub struct TextureRegisters {
    texture_mode: u32,
    t_lod: u32,
    tex_base: u32,
    tex_base1: u32,
    tex_base2: u32,
    tex_base38: u32,
}

type RegisterFile = u32[256];
type FloatShadowFile = s50[32];
type Palette = u24[256];
type PixelRegisterFile = u32[38];
type PixelFloatShadowFile = s50[12];

pub struct CoreState {
    registers: RegisterFile,
    float_shadows: FloatShadowFile,
    palette: Palette,
    raster_uses_float_shadows: u1,
    de10_mode: u1,
    texture: TextureRegisters,
    texture_layout: bits[432],
    commands: control::CommandState,
    raster: raster_state::RasterState,
    fastfill: fastfill_state::FastfillState,
    texture_cache_hits: u32,
    texture_cache_misses: u32,
    texture_cache_all_hit_requests: u32,
    texture_cache_missed_requests: u32,
}

struct PixelCacheResponse {
    cache: texture_cache::CacheResponse,
    palette_entries: u24[4],
}

struct HostedControllerState {
    core: CoreState,
    pixels_pending: u1,
    triangle_pending: u1,
    palette_cache: texture_cache::CacheResponse,
    palette_entries: u24[4],
    palette_index: u2,
    palette_active: u1,
}

struct PixelContext {
    registers: PixelRegisterFile,
    float_shadows: PixelFloatShadowFile,
    raster_uses_float_shadows: u1,
    texture: TextureRegisters,
    texture_layout: bits[432],
    drawing: u1,
}

struct PixelJob { core: PixelContext, candidate: raster_state::PixelCandidate, last: u1 }

struct TriangleSetupRequest {
    ax: s20,
    ay: s20,
    bx: s20,
    by: s20,
    cx: s20,
    cy: s20,
    reverse: u1,
}

struct TriangleEdges {
    request: TriangleSetupRequest,
    x0: s20,
    x1: s20,
    y0: s20,
    y1: s20,
    edge0: uN[120],
    edge1: uN[120],
    edge2: uN[120],
}

struct TriangleStartState {
    edges: TriangleEdges,
    phase: u2,
    start0: s40,
    start1: s40,
    start2: s40,
}

struct TriangleSetupState {
    request: TriangleSetupRequest,
    phase: u3,
    x0: s20,
    x1: s20,
    y0: s20,
    y1: s20,
    edge0: uN[120],
    edge1: uN[120],
    edge2: uN[120],
    start0: s40,
    start1: s40,
    start2: s40,
}

struct PixelInterpolationRequest {
    registers: PixelRegisterFile,
    float_shadows: PixelFloatShadowFile,
    raster_uses_float_shadows: u1,
    candidate: raster_state::PixelCandidate,
}

struct PixelInterpolatedValues {
    s: s48,
    t: s48,
    w: s48,
    red: s48,
    green: s48,
    blue: s48,
    alpha: s48,
    z: s48,
}

struct PixelInterpolatorState {
    request: PixelInterpolationRequest,
    values: PixelInterpolatedValues,
    phase: u4,
}

struct PixelResult {
    cache_valid: u1,
    texture_cache_hits: u32,
    texture_cache_misses: u32,
    texture_cache_all_hit_requests: u32,
    texture_cache_missed_requests: u32,
}

fn initial_triangle_edges() -> TriangleEdges {
    TriangleEdges {
        request: TriangleSetupRequest {
            ax: s20:0, ay: s20:0, bx: s20:0, by: s20:0, cx: s20:0, cy: s20:0, reverse: u1:0,
        },
        x0: s20:0,
        x1: s20:0,
        y0: s20:0,
        y1: s20:0,
        edge0: uN[120]:0,
        edge1: uN[120]:0,
        edge2: uN[120]:0,
    }
}

fn initial_triangle_start_state() -> TriangleStartState {
    TriangleStartState {
        edges: initial_triangle_edges(),
        phase: u2:0,
        start0: s40:0,
        start1: s40:0,
        start2: s40:0,
    }
}

fn initial_triangle_setup_state() -> TriangleSetupState {
    TriangleSetupState {
        request: TriangleSetupRequest {
            ax: s20:0, ay: s20:0, bx: s20:0, by: s20:0, cx: s20:0, cy: s20:0, reverse: u1:0,
        },
        phase: u3:0,
        x0: s20:0,
        x1: s20:0,
        y0: s20:0,
        y1: s20:0,
        edge0: uN[120]:0,
        edge1: uN[120]:0,
        edge2: uN[120]:0,
        start0: s40:0,
        start1: s40:0,
        start2: s40:0,
    }
}

fn initial_pixel_interpolator_state() -> PixelInterpolatorState {
    PixelInterpolatorState {
        request: PixelInterpolationRequest {
            registers: PixelRegisterFile:[u32:0, ...],
            float_shadows: PixelFloatShadowFile:[s50:0, ...],
            raster_uses_float_shadows: u1:0,
            candidate: raster_state::PixelCandidate { valid: u1:0, x: s16:0, y: s16:0 },
        },
        values: PixelInterpolatedValues {
            s: s48:0, t: s48:0, w: s48:0, red: s48:0, green: s48:0, blue: s48:0,
            alpha: s48:0, z: s48:0,
        },
        phase: u4:0,
    }
}

fn empty_cache_response() -> texture_cache::CacheResponse {
    texture_cache::CacheResponse {
        memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
        texels: u16[4]:[u16:0, ...],
        hits: u1[4]:[u1:0, ...],
        total_hits: u32:0,
        total_misses: u32:0,
        all_hit_requests: u32:0,
        missed_requests: u32:0,
    }
}

fn zero_texture_registers() -> TextureRegisters {
    TextureRegisters {
        texture_mode: u32:0,
        t_lod: u32:0,
        tex_base: u32:0,
        tex_base1: u32:0,
        tex_base2: u32:0,
        tex_base38: u32:0,
    }
}

pub fn initial_state() -> CoreState {
    let texture = zero_texture_registers();
    CoreState {
        registers: RegisterFile:[u32:0, ...],
        float_shadows: FloatShadowFile:[s50:0, ...],
        palette: Palette:[u24:0, ...],
        raster_uses_float_shadows: u1:0,
        de10_mode: u1:0,
        texture,
        texture_layout: layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:0, u2:0, u1:0, u1:0),
        commands: control::initial_commands(),
        raster: raster_state::initial_state(),
        fastfill: fastfill_state::initial_state(),
        texture_cache_hits: u32:0,
        texture_cache_misses: u32:0,
        texture_cache_all_hit_requests: u32:0,
        texture_cache_missed_requests: u32:0,
    }
}

fn pixel_registers(registers: RegisterFile) -> PixelRegisterFile {
    PixelRegisterFile:[
        registers[u8:2], registers[u8:3], registers[u8:8], registers[u8:9],
        registers[u8:10], registers[u8:11], registers[u8:12], registers[u8:13],
        registers[u8:14], registers[u8:15], registers[u8:16], registers[u8:17],
        registers[u8:18], registers[u8:19], registers[u8:20], registers[u8:21],
        registers[u8:22], registers[u8:23], registers[u8:24], registers[u8:25],
        registers[u8:26], registers[u8:27], registers[u8:28], registers[u8:29],
        registers[u8:30], registers[u8:31], registers[u8:65], registers[u8:67],
        registers[u8:68], registers[u8:70], registers[u8:71], registers[u8:76],
        registers[u8:77], registers[u8:81], registers[u8:82], registers[u8:133],
        registers[u8:134], registers[u8:135],
    ]
}

fn pixel_float_shadows(float_shadows: FloatShadowFile) -> PixelFloatShadowFile {
    PixelFloatShadowFile:[
        float_shadows[u8:12], float_shadows[u8:13], float_shadows[u8:14],
        float_shadows[u8:15], float_shadows[u8:20], float_shadows[u8:21],
        float_shadows[u8:22], float_shadows[u8:23], float_shadows[u8:28],
        float_shadows[u8:29], float_shadows[u8:30], float_shadows[u8:31],
    ]
}

fn pixel_register(registers: PixelRegisterFile, index: u8) -> u32 {
    if index >= u8:2 && index <= u8:3 {
        registers[index - u8:2]
    } else if index >= u8:8 && index <= u8:31 {
        registers[index - u8:6]
    } else {
        match index {
            u8:65 => registers[u32:26], u8:67 => registers[u32:27],
            u8:68 => registers[u32:28], u8:70 => registers[u32:29],
            u8:71 => registers[u32:30], u8:76 => registers[u32:31],
            u8:77 => registers[u32:32], u8:81 => registers[u32:33],
            u8:82 => registers[u32:34], u8:133 => registers[u32:35],
            u8:134 => registers[u32:36], u8:135 => registers[u32:37],
            _ => u32:0,
        }
    }
}

fn pixel_float_shadow(float_shadows: PixelFloatShadowFile, index: u8) -> s50 {
    if index >= u8:12 && index <= u8:15 {
        float_shadows[index - u8:12]
    } else if index >= u8:20 && index <= u8:23 {
        float_shadows[index - u8:16]
    } else if index >= u8:28 && index <= u8:31 {
        float_shadows[index - u8:20]
    } else {
        s50:0
    }
}

fn pixel_context(core: CoreState) -> PixelContext {
    PixelContext {
        registers: pixel_registers(core.registers),
        float_shadows: pixel_float_shadows(core.float_shadows),
        raster_uses_float_shadows: core.raster_uses_float_shadows,
        texture: core.texture,
        texture_layout: core.texture_layout,
        drawing: core.commands.swap.draw_count[0+:u1],
    }
}

pub fn de10_request_valid(request: De10MmioRequest) -> u1 { request.read ^ request.write }

pub fn de10_core_command(request: De10MmioRequest) -> CoreCommand {
    CoreCommand {
        write: request.write,
        address: (request.address as u24) << u8:2,
        data: request.write_data,
        mask: request.byte_enable,
    }
}

fn byte_mask(mask: u4) -> u32 {
    (if mask[0+:u1] == u1:1 { u32:0x000000ff } else { u32:0 }) |
    (if mask[1+:u1] == u1:1 { u32:0x0000ff00 } else { u32:0 }) |
    (if mask[2+:u1] == u1:1 { u32:0x00ff0000 } else { u32:0 }) |
    (if mask[3+:u1] == u1:1 { u32:0xff000000 } else { u32:0 })
}

fn masked_write(current: u32, data: u32, mask: u4) -> u32 {
    let expanded = byte_mask(mask);
    (current & !expanded) | (data & expanded)
}

fn read_texture_register(texture: TextureRegisters, address: u10) -> CoreResponse {
    match address {
        u10:0x300 => CoreResponse { data: texture.texture_mode, success: u1:1 },
        u10:0x304 => CoreResponse { data: texture.t_lod, success: u1:1 },
        u10:0x30c => CoreResponse { data: texture.tex_base, success: u1:1 },
        u10:0x310 => CoreResponse { data: texture.tex_base1, success: u1:1 },
        u10:0x314 => CoreResponse { data: texture.tex_base2, success: u1:1 },
        u10:0x318 => CoreResponse { data: texture.tex_base38, success: u1:1 },
        _ => CoreResponse { data: u32:0, success: u1:0 },
    }
}

fn write_texture_register
    (texture: TextureRegisters, address: u10, data: u32, mask: u4) -> (TextureRegisters, u1) {
    match address {
        u10:0x300 => (
            TextureRegisters {
                texture_mode: masked_write(texture.texture_mode, data, mask),
                ..texture
            }, u1:1,
        ),
        u10:0x304 => (
            TextureRegisters { t_lod: masked_write(texture.t_lod, data, mask), ..texture }, u1:1
        ),
        u10:0x30c => (
            TextureRegisters { tex_base: masked_write(texture.tex_base, data, mask), ..texture },
            u1:1,
        ),
        u10:0x310 => (
            TextureRegisters { tex_base1: masked_write(texture.tex_base1, data, mask), ..texture },
            u1:1,
        ),
        u10:0x314 => (
            TextureRegisters { tex_base2: masked_write(texture.tex_base2, data, mask), ..texture },
            u1:1,
        ),
        u10:0x318 => (
            TextureRegisters { tex_base38: masked_write(texture.tex_base38, data, mask), ..texture },
            u1:1,
        ),
        _ => (texture, u1:0),
    }
}

fn write_palette(palette: Palette, address: u10, data: u32) -> Palette {
    let hit = address == u10:0x334 || address == u10:0x338 || address == u10:0x33c ||
              address == u10:0x340 || address == u10:0x344 || address == u10:0x348 ||
              address == u10:0x34c || address == u10:0x350;
    let odd = address == u10:0x338 || address == u10:0x340 || address == u10:0x348 ||
              address == u10:0x350;
    let index = (data[23+:u8] & u8:0xfe) | (odd as u8);
    if hit && data[31+:u1] == u1:1 { update(palette, index, data[0+:u24]) } else { palette }
}

fn compute_texture_layout(texture: TextureRegisters) -> bits[432] {
    layout::texture_layout(
        texture.tex_base[0+:u19], texture.tex_base1[0+:u19], texture.tex_base2[0+:u19],
        texture.tex_base38[0+:u19], texture.texture_mode[8+:u4], texture.t_lod[21+:u2],
        texture.t_lod[20+:u1], texture.t_lod[24+:u1])
}

fn flush_narrow_texture_cache(raster_active: u1, t_lod: u32, texture_mode: u32) -> u1 {
    raster_active && t_lod[21+:u2] != u2:0 && texture_mode[8+:u4] == u4:5
}

fn status(state: CoreState) -> u32 {
    let displayed = state.commands.swap.swap_count[0+:u1] as u32;
    let pending = state.commands.swap.swaps_pending as u32;
    let retrace = state.commands.swap.retrace_previous as u32;
    u32:0x0ffff03f | (retrace << u32:6) | (displayed << u32:10) | (pending << u32:28)
}

pub fn scanout_state(state: CoreState) -> ScanoutState {
    let configured_tiles = state.registers[u8:133][4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let buffer_offset = (state.registers[u8:134][11+:u9] as u26) << u26:12;
    let displayed = state.commands.swap.swap_count[0+:u1];
    ScanoutState {
        base: if displayed == u1:1 { buffer_offset } else { u26:0 },
        pixel_stride: (tiles as u11) << u11:6,
        displayed,
        swap_count: state.commands.swap.swap_count,
    }
}

fn vertex(registers: RegisterFile, index: u8) -> s20 { (registers[index] as u20) as s20 }

fn pixel_vertex(registers: PixelRegisterFile, index: u8) -> s20 {
    (pixel_register(registers, index) as u20) as s20
}

fn float_shadow_shift(index: u8) -> (u6, u1) {
    match index {
        u8:12 | u8:20 | u8:28 => (u6:18, u1:1),
        u8:13 | u8:14 | u8:21 | u8:22 | u8:29 | u8:30 => (u6:14, u1:1),
        u8:15 | u8:23 | u8:31 => (u6:2, u1:1),
        _ => (u6:0, u1:0),
    }
}

fn round_float_shadow(index: u8, value: s50, shift: u6) -> s48 {
    let rounded = match index {
        u8:13 | u8:14 | u8:21 | u8:22 | u8:29 | u8:30 | u8:15 | u8:23 | u8:31 =>
            value + (s50:1 << (shift - u6:1)),
        _ => value,
    };
    (rounded >> shift) as s48
}

fn tick_events<SKIP_INVISIBLE: bool = {true}>
    (commands: control::CommandState, raster: raster_state::RasterState,
     fastfill: fastfill_state::FastfillState, v_retrace: u1)
    -> (
        control::CommandState, raster_state::RasterState, fastfill_state::FastfillState,
        raster_state::PixelCandidate, fastfill_state::FastfillWord
    ) {
    let (next_fastfill, fill_pixel) = fastfill_state::tick(fastfill);
    let (next_raster, candidate) = if fastfill.active == u1:1 {
        (raster, raster_state::PixelCandidate { valid: u1:0, x: s16:0, y: s16:0 })
    } else if SKIP_INVISIBLE {
        raster_state::tick_visible(raster)
    } else {
        raster_state::tick(raster)
    };
    (control::command_tick(commands, v_retrace), next_raster, next_fastfill, candidate, fill_pixel)
}

fn tick_with_events
    (state: CoreState, v_retrace: u1)
    -> (CoreState, raster_state::PixelCandidate, fastfill_state::FastfillWord) {
    let (commands, raster, fastfill, candidate, fill_pixel) =
        tick_events(state.commands, state.raster, state.fastfill, v_retrace);
    (CoreState { commands, raster, fastfill, ..state }, candidate, fill_pixel)
}

pub fn tick_with_pixel
    (state: CoreState, v_retrace: u1) -> (CoreState, raster_state::PixelCandidate) {
    let (state, candidate, _) = tick_with_events(state, v_retrace);
    (state, candidate)
}

pub fn tick(state: CoreState, v_retrace: u1) -> CoreState {
    let (state, _) = tick_with_pixel(state, v_retrace);
    state
}

fn advance_burst_ticks(state: CoreState) -> CoreState {
    let (commands, raster, fastfill) =
        for (_, accumulator): (
            u6, (control::CommandState, raster_state::RasterState, fastfill_state::FastfillState)
        ) in u6:0..u6:31 {
            let (commands, raster, fastfill) = accumulator;
            let (fastfill, _) = fastfill_state::tick(fastfill);
            let (raster, _) = raster_state::tick(raster);
            (control::command_tick(commands, u1:0), raster, fastfill)
        }((state.commands, state.raster, state.fastfill));
    CoreState { commands, raster, fastfill, ..state }
}

fn begin_fastfill(state: CoreState) -> fastfill_state::FastfillState {
    let fbz_mode = state.registers[u8:68];
    let clip_x = state.registers[u8:70];
    let clip_y = state.registers[u8:71];
    let color = state.registers[u8:82];
    let init1 = state.registers[u8:133];
    let init2 = state.registers[u8:134];
    let configured_tiles = init1[4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let buffer_offset = (init2[11+:u9] as u26) << u26:12;
    let drawing = state.commands.swap.draw_count[0+:u1];
    let front = if drawing == u1:1 { buffer_offset } else { u26:0 };
    let back = if drawing == u1:1 { u26:0 } else { buffer_offset };
    let color_base = if fbz_mode[14+:u1] == u1:1 { back } else { front };
    fastfill_state::begin(
        clip_x[16+:u10], clip_x[0+:u10], clip_y[16+:u10], clip_y[0+:u10], color[16+:u8],
        color[8+:u8], color[0+:u8], state.registers[u8:76][0+:u16], color_base,
        buffer_offset << u26:1, (tiles as u11) << u11:6, fbz_mode[9+:u1], fbz_mode[10+:u1],
        fbz_mode[17+:u1], state.registers[u8:135][22+:u10], fbz_mode[8+:u1], fbz_mode[11+:u1])
}

fn is_triangle_command(command: CoreCommand) -> bool {
    let address = command.address[0+:u10];
    let (write_address, _, _) = float_alias::alias(address);
    command.write == u1:1 && command.address < u24:0x400 && command.address[0+:u2] == u2:0 &&
    (write_address == u10:0x080 || write_address == u10:0x100)
}

fn triangle_setup_request(state: CoreState, command: CoreCommand) -> TriangleSetupRequest {
    TriangleSetupRequest {
        ax: vertex(state.registers, u8:2),
        ay: vertex(state.registers, u8:3),
        bx: vertex(state.registers, u8:4),
        by: vertex(state.registers, u8:5),
        cx: vertex(state.registers, u8:6),
        cy: vertex(state.registers, u8:7),
        reverse: command.data[31+:u1],
    }
}

fn step_with_raster_setup
    (state: CoreState, command: CoreCommand, hosted_triangle: u1)
    -> (CoreState, CoreResponse) {
    let is_register = command.address < u24:0x400 && command.address[0+:u2] == u2:0;
    let address = command.address[0+:u10];
    let (write_address, fraction_bits, is_float) = float_alias::alias(address);
    let write_data = if is_float == u1:1 {
        float_alias::float32_to_fixed(command.data, fraction_bits) as u32
    } else {
        command.data
    };
    let write_mask = if is_float == u1:1 { u4:0xf } else { command.mask };
    let index = write_address[2+:u8];
    if !is_register {
        (state, CoreResponse { data: u32:0, success: u1:0 })
    } else if command.write == u1:1 {
        let value = masked_write(state.registers[index], write_data, write_mask);
        let registers = update(state.registers, index, value);
        let (shadow_shift, has_float_shadow) = float_shadow_shift(index);
        let shadow_value = if is_float == u1:1 {
            float_alias::float32_to_fixed(command.data, fraction_bits + shadow_shift)
        } else {
            ((value as s32) as s50) << shadow_shift
        };
        let float_shadows = if has_float_shadow == u1:1 {
            update(state.float_shadows, index, shadow_value)
        } else {
            state.float_shadows
        };
        let (texture, _) =
            write_texture_register(state.texture, write_address, write_data, write_mask);
        let palette = write_palette(state.palette, write_address, write_data);
        let commands = control::command_write(state.commands, write_address, write_data);
        let triangle_command = write_address == u10:0x080 || write_address == u10:0x100;
        let raster = if triangle_command && hosted_triangle == u1:0 {
            raster_state::begin_triangle(
                vertex(registers, u8:2), vertex(registers, u8:3), vertex(registers, u8:4),
                vertex(registers, u8:5), vertex(registers, u8:6), vertex(registers, u8:7),
                write_data[31+:u1])
        } else {
            state.raster
        };
        let next_state = CoreState {
            registers,
            float_shadows,
            raster_uses_float_shadows: if triangle_command {
                (write_address == u10:0x100) as u1
            } else {
                state.raster_uses_float_shadows
            },
            texture,
            palette,
            texture_layout: compute_texture_layout(texture),
            commands,
            raster,
            ..state
        };
        let fastfill =
            if write_address == u10:0x124 { begin_fastfill(next_state) } else { state.fastfill };
        let next_state = CoreState { fastfill, ..next_state };
        (next_state, CoreResponse { data: u32:0, success: u1:1 })
    } else {
        let data = if address == u10:0 { status(state) } else { state.registers[index] };
        (state, CoreResponse { data, success: u1:1 })
    }
}

pub fn step(state: CoreState, command: CoreCommand) -> (CoreState, CoreResponse) {
    step_with_raster_setup(state, command, u1:0)
}

fn color_channel(value: s48) -> u8 {
    let integer = value >> u8:12;
    if integer < s48:0 {
        u8:0
    } else if integer > s48:255 {
        u8:255
    } else {
        integer as u8
    }
}

fn interpolated_value
    (registers: PixelRegisterFile, float_shadows: PixelFloatShadowFile,
     raster_uses_float_shadows: u1, candidate: raster_state::PixelCandidate, start_index: u8,
     dx_index: u8, dy_index: u8) -> s48 {
    let (shadow_shift, has_float_shadow) = float_shadow_shift(start_index);
    let adjust = pixel_register(registers, u8:65)[26+:u1];
    if raster_uses_float_shadows == u1:1 && has_float_shadow == u1:1 {
        let high = interpolator::at_pixel_high(
            pixel_float_shadow(float_shadows, start_index),
            pixel_float_shadow(float_shadows, dx_index),
            pixel_float_shadow(float_shadows, dy_index), pixel_vertex(registers, u8:2),
            pixel_vertex(registers, u8:3), candidate.x, candidate.y, adjust);
        round_float_shadow(start_index, high, shadow_shift)
    } else {
        interpolator::at_pixel(
            (pixel_register(registers, start_index) as s32) as s48,
            (pixel_register(registers, dx_index) as s32) as s48,
            (pixel_register(registers, dy_index) as s32) as s48, pixel_vertex(registers, u8:2),
            pixel_vertex(registers, u8:3), candidate.x, candidate.y, adjust)
    }
}

fn interpolated_parameter
    (state: CoreState, candidate: raster_state::PixelCandidate, start_index: u8, dx_index: u8,
     dy_index: u8) -> s48 {
    interpolated_value(
        pixel_registers(state.registers), pixel_float_shadows(state.float_shadows),
        state.raster_uses_float_shadows, candidate, start_index, dx_index, dy_index)
}

fn interpolated_color_values
    (registers: PixelRegisterFile, float_shadows: PixelFloatShadowFile,
     raster_uses_float_shadows: u1,
     candidate: raster_state::PixelCandidate, start_index: u8, dx_index: u8, dy_index: u8) -> u8 {
    color_channel(
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, start_index, dx_index,
            dy_index))
}

fn interpolated_color
    (state: CoreState, candidate: raster_state::PixelCandidate, start_index: u8, dx_index: u8,
     dy_index: u8) -> u8 {
    interpolated_color_values(
        pixel_registers(state.registers), pixel_float_shadows(state.float_shadows),
        state.raster_uses_float_shadows, candidate, start_index, dx_index, dy_index)
}

fn texture_request_interpolated
    (registers: PixelRegisterFile, texture: TextureRegisters, texture_layout: bits[432],
     s: s48, t: s48, w: s48) -> texture_sampler::SampleRequest {
    texture_sampler::sample_request(
        s, t, w,
        pixel_register(registers, u8:21) as s32, pixel_register(registers, u8:22) as s32,
        pixel_register(registers, u8:29) as s32, pixel_register(registers, u8:30) as s32,
        texture.texture_mode, texture.t_lod, texture_layout,
        texture.tex_base as u24, texture.tex_base1 as u24, texture.tex_base2 as u24,
        texture.tex_base38 as u24)
}

fn texture_request_values
    (registers: PixelRegisterFile, float_shadows: PixelFloatShadowFile,
     raster_uses_float_shadows: u1,
     texture: TextureRegisters, texture_layout: bits[432], candidate: raster_state::PixelCandidate)
    -> texture_sampler::SampleRequest {
    texture_request_interpolated(
        registers, texture, texture_layout,
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:13, u8:21, u8:29),
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:14, u8:22, u8:30),
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:15, u8:23, u8:31))
}

fn texture_request
    (state: CoreState, candidate: raster_state::PixelCandidate) -> texture_sampler::SampleRequest {
    texture_request_values(
        pixel_registers(state.registers), pixel_float_shadows(state.float_shadows),
        state.raster_uses_float_shadows, state.texture, state.texture_layout, candidate)
}

fn candidate_visible
    (mode: u32, clip_x: u32, clip_y: u32, candidate: raster_state::PixelCandidate) -> bool {
    let inside_clip = candidate.x >= (clip_x[16+:u10] as s16) &&
                      candidate.x < (clip_x[0+:u10] as s16) &&
                      candidate.y >= (clip_y[16+:u10] as s16) &&
                      candidate.y < (clip_y[0+:u10] as s16);
    candidate.valid == u1:1 && candidate.x >= s16:0 && candidate.y >= s16:0 &&
    (mode[9+:u1] == u1:1 || mode[10+:u1] == u1:1) && (mode[0+:u1] == u1:0 || inside_clip)
}

fn raster_visible(state: CoreState, candidate: raster_state::PixelCandidate) -> bool {
    candidate_visible(
        state.registers[u8:68], state.registers[u8:70], state.registers[u8:71], candidate)
}

fn combined_color_interpolated
    (registers: PixelRegisterFile, texture_rgba: u32, red: s48, green: s48, blue: s48,
     alpha: s48, z: s48) -> u32 {
    let path = pixel_register(registers, u8:65);
    let iterated_rgb = color_channel(red) ++ color_channel(green) ++ color_channel(blue);
    let iterated_alpha = color_channel(alpha);
    let iterated_z = color_channel(z);
    let color0 = pixel_register(registers, u8:81);
    let color1 = pixel_register(registers, u8:82);
    pixel::color_combine(
        iterated_rgb, iterated_alpha, iterated_z, texture_rgba[8+:u24], texture_rgba[0+:u8],
        color0[0+:u24], color0[24+:u8], color1[0+:u24], color1[24+:u8], path[0+:u2], path[4+:u1],
        path[7+:u1], path[8+:u1], path[9+:u1], path[10+:u3], path[13+:u1], path[14+:u2],
        path[16+:u1], path[2+:u2], path[5+:u2], path[17+:u1], path[18+:u1], path[19+:u3],
        path[22+:u1], path[23+:u2], path[25+:u1])
}

fn combined_color_values
    (registers: PixelRegisterFile, float_shadows: PixelFloatShadowFile,
     raster_uses_float_shadows: u1,
     candidate: raster_state::PixelCandidate, texture_rgba: u32) -> u32 {
    combined_color_interpolated(
        registers, texture_rgba,
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:8, u8:16, u8:24),
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:9, u8:17, u8:25),
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:10, u8:18, u8:26),
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:12, u8:20, u8:28),
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:11, u8:19, u8:27))
}

fn combined_color
    (state: CoreState, candidate: raster_state::PixelCandidate, texture_rgba: u32) -> u32 {
    combined_color_values(
        pixel_registers(state.registers), pixel_float_shadows(state.float_shadows),
        state.raster_uses_float_shadows, candidate, texture_rgba)
}

fn decode_rgb565(value: u16) -> u24 {
    let red = ((value[11+:u5] as u8) << u8:3) | ((value[11+:u5] as u8) >> u8:2);
    let green = ((value[5+:u6] as u8) << u8:2) | ((value[5+:u6] as u8) >> u8:4);
    let blue = ((value[0+:u5] as u8) << u8:3) | ((value[0+:u5] as u8) >> u8:2);
    red ++ green ++ blue
}

fn framebuffer_y_values(mode: u32, y_origin: u10, candidate: raster_state::PixelCandidate) -> u16 {
    if mode[17+:u1] == u1:1 { (y_origin as u16) - (candidate.y as u16) } else { candidate.y as u16 }
}

fn framebuffer_y(state: CoreState, candidate: raster_state::PixelCandidate) -> u16 {
    framebuffer_y_values(state.registers[u8:68], state.registers[u8:135][22+:u10], candidate)
}

fn clamp_depth(value: s48) -> u16 {
    if value < s48:0 {
        u16:0
    } else if value > s48:65535 {
        u16:0xffff
    } else {
        value as u16
    }
}

fn new_depth_interpolated(registers: PixelRegisterFile, z: s48, w: s48) -> u16 {
    let mode = pixel_register(registers, u8:68);
    let z_depth = clamp_depth(z >> u8:12);
    let w_depth = pixel::fog_w_depth(w as s32);
    let base = if mode[3+:u1] == u1:1 { w_depth } else { z_depth };
    let selected = if mode[20+:u1] == u1:1 {
        pixel_register(registers, u8:76)[0+:u16]
    } else {
        base
    };
    let biased = clamp_depth(
        (selected as s48) + ((pixel_register(registers, u8:76)[0+:u16] as s16) as s48));
    if mode[16+:u1] == u1:1 { biased } else { selected }
}

fn new_depth_values
    (registers: PixelRegisterFile, float_shadows: PixelFloatShadowFile,
     raster_uses_float_shadows: u1, candidate: raster_state::PixelCandidate) -> u16 {
    new_depth_interpolated(
        registers,
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:11, u8:19, u8:27),
        interpolated_value(
            registers, float_shadows, raster_uses_float_shadows, candidate, u8:15, u8:23, u8:31))
}

fn new_depth(state: CoreState, candidate: raster_state::PixelCandidate) -> u16 {
    new_depth_values(
        pixel_registers(state.registers), pixel_float_shadows(state.float_shadows),
        state.raster_uses_float_shadows, candidate)
}

fn aux_write_values
    (init1: u32, init2: u32, mode: u32, y_origin: u10, candidate: raster_state::PixelCandidate,
     depth: u16, alpha: u8) -> memory::MemoryCommand {
    let configured_tiles = init1[4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let offset = (init2[11+:u9] as u26) << u26:12;
    let value = if mode[18+:u1] == u1:1 { alpha ++ alpha } else { depth };
    let packed = pixel::write_pack(
        offset << u26:1, candidate.x as u16, framebuffer_y_values(mode, y_origin, candidate),
        (tiles as u11) << u11:6, value);
    memory::MemoryCommand {
        write: u1:1,
        address: packed[36+:u26][2+:u22],
        data: packed[4+:u32],
        mask: packed[0+:u4],
    }
}

fn aux_write
    (state: CoreState, candidate: raster_state::PixelCandidate, depth: u16, alpha: u8)
    -> memory::MemoryCommand {
    aux_write_values(
        state.registers[u8:133], state.registers[u8:134], state.registers[u8:68],
        state.registers[u8:135][22+:u10], candidate, depth, alpha)
}

fn raster_write_color
    (init1: u32, init2: u32, fbz_mode: u32, alpha_mode: u32, drawing: u1, y_origin: u10,
     candidate: raster_state::PixelCandidate, color: u32, dest_rgb: u24) -> memory::MemoryCommand {
    let configured_tiles = init1[4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let stride = (tiles as u11) << u11:6;
    let buffer_offset = (init2[11+:u9] as u26) << u26:12;
    let front = if drawing == u1:1 { buffer_offset } else { u26:0 };
    let back = if drawing == u1:1 { u26:0 } else { buffer_offset };
    let base = if fbz_mode[14+:u1] == u1:1 { back } else { front };
    let blended = pixel::alpha_blend(
        color[8+:u24], dest_rgb, color[8+:u24], color[0+:u8], alpha_mode[8+:u4],
        alpha_mode[12+:u4], alpha_mode[4+:u1]);
    let red = blended[16+:u8];
    let green = blended[8+:u8];
    let blue = blended[0+:u8];
    let output_y = framebuffer_y_values(fbz_mode, y_origin, candidate);
    let rgb565 = dither::rgb565(
        red, green, blue, candidate.x as u16, output_y, fbz_mode[8+:u1], fbz_mode[11+:u1]);
    let packed = pixel::write_pack(base, candidate.x as u16, output_y, stride, rgb565);
    memory::MemoryCommand {
        write: u1:1,
        address: packed[36+:u26][2+:u22],
        data: packed[4+:u32],
        mask: packed[0+:u4],
    }
}

fn raster_write
    (state: CoreState, candidate: raster_state::PixelCandidate, texture_rgba: u32, dest_rgb: u24)
    -> memory::MemoryCommand {
    raster_write_color(
        state.registers[u8:133], state.registers[u8:134], state.registers[u8:68],
        state.registers[u8:67], state.commands.swap.draw_count[0+:u1],
        state.registers[u8:135][22+:u10], candidate, combined_color(state, candidate, texture_rgba),
        dest_rgb)
}

fn fastfill_write
    (base: u26, fill: fastfill_state::FastfillWord, data: u32) -> memory::MemoryCommand {
    let address = pixel::framebuffer_address(base, fill.x, fill.y, fill.stride);
    memory::MemoryCommand { write: u1:1, address: address[2+:u22], data, mask: fill.mask }
}

fn texture_download_address(state: CoreState, pci_address: u23) -> u27 {
    let lod = pci_address[17+:u4];
    let segment = (state.texture_layout >> (((u4:8 - lod) as u10) * u10:48)) as u48;
    tmu::texture_write_address(
        pci_address, (state.texture.texture_mode[8+:u4] >= u4:8) as u1,
        state.texture.texture_mode[31+:u1], u1:1, segment[26+:u22], segment[0+:u4],
        state.texture.tex_base as u24, state.texture.tex_base1 as u24,
        state.texture.tex_base2 as u24, state.texture.tex_base38 as u24, state.texture.t_lod[24+:u1])
}

fn texture_block_write(state: CoreState, burst: TextureBurst) -> (memory::MemoryBlockWrite, u1) {
    let first =
        texture_download_address(state, (burst.commands[u32:0].address - u24:0x800000) as u23);
    let initial = memory::MemoryBlockWrite {
        address: first[2+:u22],
        data: u32[32]:[u32:0, ...],
        masks: u4[32]:[u4:0, ...],
    };
    for (i, accumulator): (u6, (memory::MemoryBlockWrite, u1)) in u6:0..u6:32 {
        let (block, valid) = accumulator;
        let command = burst.commands[i];
        let download = texture_download_address(state, (command.address - u24:0x800000) as u23);
        let expected = first[0+:u26] + ((i as u26) << u8:2);
        let command_valid = command.write == u1:1 && command.address >= u24:0x800000 &&
                            download[26+:u1] == u1:1 && download[0+:u26] == expected;
        let block = memory::MemoryBlockWrite {
            data: update(block.data, i, command.data),
            masks: update(block.masks, i, command.mask),
            ..block
        };
        (block, valid && command_valid)
    }((initial, first[26+:u1] && first[0+:u7] == u7:0))
}

proc framebuffer_pair_adapter {
    command_s: chan<memory::MemoryCommand> out;
    response_r: chan<memory::MemoryResponse> in;
    scalar_command_r: chan<memory::MemoryCommand> in;
    scalar_response_s: chan<memory::MemoryResponse> out;
    pixel_read_r: chan<memory::MemoryRequestPair> in;
    pixel_read_response_s: chan<memory::MemoryResponsePair> out;
    pixel_write_r: chan<memory::MemoryRequestPair> in;
    pixel_write_response_s: chan<memory::MemoryResponsePair> out;
    fastfill_r: chan<memory::MemoryRequestPair> in;
    fastfill_response_s: chan<memory::MemoryResponsePair> out;

    config(command_s: chan<memory::MemoryCommand> out, response_r: chan<memory::MemoryResponse> in,
           scalar_command_r: chan<memory::MemoryCommand> in,
           scalar_response_s: chan<memory::MemoryResponse> out,
           pixel_read_r: chan<memory::MemoryRequestPair> in,
           pixel_read_response_s: chan<memory::MemoryResponsePair> out,
           pixel_write_r: chan<memory::MemoryRequestPair> in,
           pixel_write_response_s: chan<memory::MemoryResponsePair> out,
           fastfill_r: chan<memory::MemoryRequestPair> in,
           fastfill_response_s: chan<memory::MemoryResponsePair> out) {
        (
            command_s, response_r, scalar_command_r, scalar_response_s, pixel_read_r,
            pixel_read_response_s, pixel_write_r, pixel_write_response_s, fastfill_r,
            fastfill_response_s,
        )
    }

    init { () }

    next(state: ()) {
        let default_command =
            memory::MemoryCommand { write: u1:0, address: u22:0, data: u32:0, mask: u4:0 };
        let default_response = memory::MemoryResponse { data: u32:0, success: u1:0 };
        let empty_responses = memory::MemoryResponse[2]:[default_response, ...];
        let (tok, scalar, scalar_valid) =
            recv_non_blocking(join(), scalar_command_r, default_command);
        let (tok, reads, reads_valid) =
            recv_non_blocking(tok, pixel_read_r, memory::empty_request_pair());
        let (tok, writes, writes_valid) =
            recv_non_blocking(tok, pixel_write_r, memory::empty_request_pair());
        let (tok, fills, fills_valid) =
            recv_non_blocking(tok, fastfill_r, memory::empty_request_pair());
        let tok = send_if(tok, command_s, scalar_valid, scalar);
        let (tok, scalar_response) = recv_if(tok, response_r, scalar_valid, default_response);
        let tok = send_if(tok, scalar_response_s, scalar_valid, scalar_response);
        let (tok, read_responses) =
            unroll_for! (i, accumulator): (u2, (token, memory::MemoryResponse[2])) in u2:0..u2:2 {
                let (tok, responses) = accumulator;
                let active = reads_valid && ((reads.valid >> i) as u1) == u1:1;
                let tok = send_if(tok, command_s, active, reads.commands[i]);
                let (tok, response) = recv_if(tok, response_r, active, default_response);
                (tok, update(responses, i, response))
            }((tok, empty_responses));
        let tok = send_if(
            tok, pixel_read_response_s, reads_valid,
            memory::MemoryResponsePair { responses: read_responses });
        let (tok, write_responses) =
            unroll_for! (i, accumulator): (u2, (token, memory::MemoryResponse[2])) in u2:0..u2:2 {
                let (tok, responses) = accumulator;
                let active = writes_valid && ((writes.valid >> i) as u1) == u1:1;
                let tok = send_if(tok, command_s, active, writes.commands[i]);
                let (tok, response) = recv_if(tok, response_r, active, default_response);
                (tok, update(responses, i, response))
            }((tok, empty_responses));
        let tok = send_if(
            tok, pixel_write_response_s, writes_valid,
            memory::MemoryResponsePair { responses: write_responses });
        let (tok, fill_responses) =
            unroll_for! (i, accumulator): (u2, (token, memory::MemoryResponse[2])) in u2:0..u2:2 {
                let (tok, responses) = accumulator;
                let active = fills_valid && ((fills.valid >> i) as u1) == u1:1;
                let tok = send_if(tok, command_s, active, fills.commands[i]);
                let (tok, response) = recv_if(tok, response_r, active, default_response);
                (tok, update(responses, i, response))
            }((tok, empty_responses));
        let tok = send_if(
            tok, fastfill_response_s, fills_valid,
            memory::MemoryResponsePair { responses: fill_responses });
        state
    }
}

proc hosted_triangle_edge_setup {
    request_r: chan<TriangleSetupRequest> in;
    response_s: chan<TriangleEdges> out;

    config(request_r: chan<TriangleSetupRequest> in, response_s: chan<TriangleEdges> out) {
        (request_r, response_s)
    }

    init { initial_triangle_setup_state() }

    next(state: TriangleSetupState) {
        let idle = state.phase == u3:0;
        let (tok, incoming) = recv_if(join(), request_r, idle, state.request);
        let request = if idle { incoming } else { state.request };
        let minimum_x =
            if request.ax < request.bx {
                if request.ax < request.cx { request.ax } else { request.cx }
            } else {
                if request.bx < request.cx { request.bx } else { request.cx }
            };
        let maximum_x =
            if request.ax > request.bx {
                if request.ax > request.cx { request.ax } else { request.cx }
            } else {
                if request.bx > request.cx { request.bx } else { request.cx }
            };
        let floor_x0 = (minimum_x >> u8:4) << u8:4;
        let floor_x1 = (maximum_x >> u8:4) << u8:4;
        let x0 = if idle { floor_x0 } else { state.x0 };
        let x1 = if idle {
            if (maximum_x as u4) == u4:0 { floor_x1 } else { floor_x1 + s20:16 }
        } else {
            state.x1
        };
        let floor_y0 = (request.ay >> u8:4) << u8:4;
        let floor_y1 = (request.cy >> u8:4) << u8:4;
        let y0 = if idle {
            if (request.ay as u4) > u4:8 { floor_y0 + s20:16 } else { floor_y0 }
        } else {
            state.y0
        };
        let y1 = if idle {
            if (request.cy as u4) > u4:8 { floor_y1 + s20:16 } else { floor_y1 }
        } else {
            state.y1
        };
        let edge = match state.phase {
            u3:1 => raster::edge_coefficients20(
                request.ax, request.ay, request.bx, request.by, request.reverse),
            u3:2 => raster::edge_coefficients20(
                request.bx, request.by, request.cx, request.cy, request.reverse),
            _ => raster::edge_coefficients20(
                request.cx, request.cy, request.ax, request.ay, request.reverse),
        };
        let edge0 = if state.phase == u3:1 { edge } else { state.edge0 };
        let edge1 = if state.phase == u3:2 { edge } else { state.edge1 };
        let edge2 = if state.phase == u3:3 { edge } else { state.edge2 };
        let done = state.phase == u3:3;
        let edges = TriangleEdges {
            request,
            x0,
            x1,
            y0,
            y1,
            edge0,
            edge1,
            edge2,
        };
        let tok = send_if(tok, response_s, done, edges);
        TriangleSetupState {
            request,
            phase: if idle { u3:1 } else if done { u3:0 } else { state.phase + u3:1 },
            x0,
            x1,
            y0,
            y1,
            edge0,
            edge1,
            edge2,
            start0: state.start0,
            start1: state.start1,
            start2: state.start2,
        }
    }
}

proc hosted_triangle_start_setup {
    request_r: chan<TriangleEdges> in;
    response_s: chan<uN[560]> out;

    config(request_r: chan<TriangleEdges> in, response_s: chan<uN[560]> out) {
        (request_r, response_s)
    }

    init { initial_triangle_start_state() }

    next(state: TriangleStartState) {
        let idle = state.phase == u2:0;
        let (tok, incoming) = recv_if(join(), request_r, idle, state.edges);
        let edges = if idle { incoming } else { state.edges };
        let edge = match state.phase {
            u2:1 => edges.edge0,
            u2:2 => edges.edge1,
            _ => edges.edge2,
        };
        let start = raster::edge_start20(edge, edges.x0 + s20:9, edges.y0 + s20:8);
        let start0 = if state.phase == u2:1 { start } else { state.start0 };
        let start1 = if state.phase == u2:2 { start } else { state.start1 };
        let start2 = if state.phase == u2:3 { start } else { state.start2 };
        let done = state.phase == u2:3;
        let setup = (edges.x0 as u20) ++ (edges.x1 as u20) ++ (edges.y0 as u20) ++
                    (edges.y1 as u20) ++ edges.edge0 ++ edges.edge1 ++ edges.edge2 ++
                    (start0 as u40) ++ (start1 as u40) ++ (start2 as u40);
        let tok = send_if(tok, response_s, done, setup);
        TriangleStartState {
            edges,
            phase: if idle { u2:1 } else if done { u2:0 } else { state.phase + u2:1 },
            start0,
            start1,
            start2,
        }
    }
}

proc hosted_pixel_interpolator {
    request_r: chan<PixelInterpolationRequest> in;
    response_s: chan<PixelInterpolatedValues> out;

    config(request_r: chan<PixelInterpolationRequest> in,
           response_s: chan<PixelInterpolatedValues> out) {
        (request_r, response_s)
    }

    init { initial_pixel_interpolator_state() }

    next(state: PixelInterpolatorState) {
        let idle = state.phase == u4:0;
        let (tok, incoming) = recv_if(join(), request_r, idle, state.request);
        let request = if idle { incoming } else { state.request };
        let index = if idle { u4:0 } else { state.phase - u4:1 };
        let (start_index, dx_index, dy_index) = match index {
            u4:0 => (u8:13, u8:21, u8:29),
            u4:1 => (u8:14, u8:22, u8:30),
            u4:2 => (u8:15, u8:23, u8:31),
            u4:3 => (u8:8, u8:16, u8:24),
            u4:4 => (u8:9, u8:17, u8:25),
            u4:5 => (u8:10, u8:18, u8:26),
            u4:6 => (u8:12, u8:20, u8:28),
            _ => (u8:11, u8:19, u8:27),
        };
        let value = interpolated_value(
            request.registers, request.float_shadows, request.raster_uses_float_shadows,
            request.candidate, start_index, dx_index, dy_index);
        let values = if idle {
            state.values
        } else {
            match index {
                u4:0 => PixelInterpolatedValues { s: value, ..state.values },
                u4:1 => PixelInterpolatedValues { t: value, ..state.values },
                u4:2 => PixelInterpolatedValues { w: value, ..state.values },
                u4:3 => PixelInterpolatedValues { red: value, ..state.values },
                u4:4 => PixelInterpolatedValues { green: value, ..state.values },
                u4:5 => PixelInterpolatedValues { blue: value, ..state.values },
                u4:6 => PixelInterpolatedValues { alpha: value, ..state.values },
                _ => PixelInterpolatedValues { z: value, ..state.values },
            }
        };
        let done = state.phase == u4:8;
        let tok = send_if(tok, response_s, done, values);
        PixelInterpolatorState {
            request,
            values,
            phase: if idle { u4:1 } else if done { u4:0 } else { state.phase + u4:1 },
        }
    }
}

proc hosted_pixel_pipeline {
    job_r: chan<PixelJob> in;
    result_s: chan<PixelResult> out;
    interpolation_request_s: chan<PixelInterpolationRequest> out;
    interpolation_response_r: chan<PixelInterpolatedValues> in;
    cache_request_s: chan<texture_cache::CacheRequest> out;
    cache_response_r: chan<PixelCacheResponse> in;
    framebuffer_read_s: chan<memory::MemoryRequestPair> out;
    framebuffer_read_response_r: chan<memory::MemoryResponsePair> in;
    framebuffer_write_s: chan<memory::MemoryRequestPair> out;
    framebuffer_write_response_r: chan<memory::MemoryResponsePair> in;

    config(job_r: chan<PixelJob> in, result_s: chan<PixelResult> out,
           interpolation_request_s: chan<PixelInterpolationRequest> out,
           interpolation_response_r: chan<PixelInterpolatedValues> in,
           cache_request_s: chan<texture_cache::CacheRequest> out,
           cache_response_r: chan<PixelCacheResponse> in,
           framebuffer_read_s: chan<memory::MemoryRequestPair> out,
           framebuffer_read_response_r: chan<memory::MemoryResponsePair> in,
           framebuffer_write_s: chan<memory::MemoryRequestPair> out,
           framebuffer_write_response_r: chan<memory::MemoryResponsePair> in) {
        (
            job_r, result_s, interpolation_request_s, interpolation_response_r, cache_request_s,
            cache_response_r, framebuffer_read_s, framebuffer_read_response_r,
            framebuffer_write_s, framebuffer_write_response_r,
        )
    }

    init { () }

    next(state: ()) {
        let (tok, job) = recv(join(), job_r);
        let core = job.core;
        let candidate = job.candidate;
        let path = pixel_register(core.registers, u8:65);
        let alpha_mode = pixel_register(core.registers, u8:67);
        let fbz_mode = pixel_register(core.registers, u8:68);
        let clip_x = pixel_register(core.registers, u8:70);
        let clip_y = pixel_register(core.registers, u8:71);
        let chroma_key = pixel_register(core.registers, u8:77);
        let init1 = pixel_register(core.registers, u8:133);
        let init2 = pixel_register(core.registers, u8:134);
        let y_origin = pixel_register(core.registers, u8:135)[22+:u10];
        let raster_valid = candidate_visible(fbz_mode, clip_x, clip_y, candidate);
        let interpolation_request = PixelInterpolationRequest {
            registers: core.registers,
            float_shadows: core.float_shadows,
            raster_uses_float_shadows: core.raster_uses_float_shadows,
            candidate,
        };
        let tok = send(tok, interpolation_request_s, interpolation_request);
        let (tok, interpolated) = recv(tok, interpolation_response_r);
        let sample = texture_request_interpolated(
            core.registers, core.texture, core.texture_layout, interpolated.s, interpolated.t,
            interpolated.w);
        let textured = raster_valid && path[27+:u1] == u1:1;
        let narrow =
            flush_narrow_texture_cache(u1:1, core.texture.t_lod, core.texture.texture_mode);
        let cache_access = textured || job.last;
        let cache_request = if textured {
            if narrow {
                texture_cache::invalidating_sample_request(sample, core.texture_layout)
            } else {
                texture_cache::sample_request(sample, core.texture_layout)
            }
        } else {
            texture_cache::status_request()
        };
        let tok = send_if(tok, cache_request_s, cache_access, cache_request);
        let (tok, pixel_cache_response) = recv_if(
            tok, cache_response_r, cache_access,
            PixelCacheResponse {
                cache: empty_cache_response(), palette_entries: u24[4]:[u24:0, ...]
            });
        let cached_sample = pixel_cache_response.cache;
        let texture_rgba = if textured {
            texture_sampler::decode_texels_with_entries(
                sample, cached_sample.texels[u32:0], cached_sample.texels[u32:1],
                cached_sample.texels[u32:2], cached_sample.texels[u32:3],
                pixel_cache_response.palette_entries)
        } else {
            u32:0
        };
        let source_color = combined_color_interpolated(
            core.registers, texture_rgba, interpolated.red, interpolated.green, interpolated.blue,
            interpolated.alpha, interpolated.z);
        let depth = new_depth_interpolated(core.registers, interpolated.z, interpolated.w);
        let aux = aux_write_values(
            init1, init2, fbz_mode, y_origin, candidate, depth, source_color[0+:u8]);
        let depth_access = raster_valid && fbz_mode[4+:u1] == u1:1;
        let depth_read =
            memory::MemoryCommand { write: u1:0, address: aux.address, data: u32:0, mask: u4:0 };
        let provisional = raster_write_color(
            init1, init2, fbz_mode, alpha_mode, core.drawing, y_origin, candidate, source_color,
            u24:0);
        let blend_access =
            raster_valid && (alpha_mode[4+:u1] == u1:1 || fbz_mode[19+:u1] == u1:1);
        let blend_read = memory::MemoryCommand {
            write: u1:0,
            address: provisional.address,
            data: u32:0,
            mask: u4:0,
        };
        let read_valid = (blend_access as u2) << u2:1 | (depth_access as u2);
        let framebuffer_reads = memory::MemoryRequestPair {
            valid: read_valid,
            commands: memory::MemoryCommand[2]:[depth_read, blend_read],
        };
        let tok = send_if(tok, framebuffer_read_s, read_valid != u2:0, framebuffer_reads);
        let (tok, read_responses) = recv_if(
            tok, framebuffer_read_response_r, read_valid != u2:0, memory::empty_response_pair());
        let depth_response = read_responses.responses[u32:0];
        let blend_response = read_responses.responses[u32:1];
        let old_depth = if aux.mask == u4:0xc {
            depth_response.data[16+:u16]
        } else {
            depth_response.data[0+:u16]
        };
        let depth_pass = fbz_mode[4+:u1] == u1:0 ||
                         pixel::depth_compare(fbz_mode[5+:u3], depth, old_depth) ==
                         u1:1;
        let tests = pixel::pixel_tests(
            source_color[8+:u24], chroma_key[0+:u24], source_color[0+:u8], alpha_mode[24+:u8],
            alpha_mode[1+:u3]);
        let tests_pass = (fbz_mode[1+:u1] == u1:0 || tests[1+:u1] == u1:0) &&
                         (alpha_mode[0+:u1] == u1:0 || tests[0+:u1] == u1:1);
        let pixel_valid = raster_valid && depth_pass && tests_pass;
        let destination = if provisional.mask == u4:0xc {
            blend_response.data[16+:u16]
        } else {
            blend_response.data[0+:u16]
        };
        let destination_rgb = decode_rgb565(destination);
        let destination_rgb = if fbz_mode[19+:u1] == u1:1 {
            dither::subtract_rgb(
                destination_rgb, candidate.x as u16,
                framebuffer_y_values(fbz_mode, y_origin, candidate), fbz_mode[11+:u1])
        } else {
            destination_rgb
        };
        let color_valid = pixel_valid && fbz_mode[9+:u1] == u1:1;
        let color_write = raster_write_color(
            init1, init2, fbz_mode, alpha_mode, core.drawing, y_origin, candidate, source_color,
            destination_rgb);
        let aux_valid = pixel_valid && fbz_mode[10+:u1] == u1:1;
        let write_valid = (aux_valid as u2) << u2:1 | (color_valid as u2);
        let framebuffer_writes = memory::MemoryRequestPair {
            valid: write_valid,
            commands: memory::MemoryCommand[2]:[color_write, aux],
        };
        let tok = send_if(tok, framebuffer_write_s, write_valid != u2:0, framebuffer_writes);
        let (tok, _) = recv_if(
            tok, framebuffer_write_response_r, write_valid != u2:0, memory::empty_response_pair());
        let result = PixelResult {
            cache_valid: cache_access as u1,
            texture_cache_hits: cached_sample.total_hits,
            texture_cache_misses: cached_sample.total_misses,
            texture_cache_all_hit_requests: cached_sample.all_hit_requests,
            texture_cache_missed_requests: cached_sample.missed_requests,
        };
        let tok = send(tok, result_s, result);
        state
    }
}

pub proc core_sim_impl<ENABLE_BURSTS: bool = {false}, HOSTED_DE10: bool = {false}> {
    command_r: chan<CoreCommand> in;
    de10_request_r: chan<De10MmioRequest> in;
    burst_r: chan<TextureBurst> in;
    response_s: chan<CoreResponse> out;
    framebuffer_command_s: chan<memory::MemoryCommand> out;
    framebuffer_response_r: chan<memory::MemoryResponse> in;
    framebuffer_pixel_read_s: chan<memory::MemoryRequestPair> out;
    framebuffer_pixel_read_response_r: chan<memory::MemoryResponsePair> in;
    framebuffer_pixel_write_s: chan<memory::MemoryRequestPair> out;
    framebuffer_pixel_write_response_r: chan<memory::MemoryResponsePair> in;
    framebuffer_fastfill_s: chan<memory::MemoryRequestPair> out;
    framebuffer_fastfill_response_r: chan<memory::MemoryResponsePair> in;
    texture_cache_request_s: chan<texture_cache::CacheRequest> out;
    texture_cache_response_r: chan<texture_cache::CacheResponse> in;
    texture_block_s: chan<memory::MemoryBlockWrite> out;
    texture_block_response_r: chan<memory::MemoryBlockResponse> in;

    config(command_r: chan<CoreCommand> in, de10_request_r: chan<De10MmioRequest> in,
           burst_r: chan<TextureBurst> in, response_s: chan<CoreResponse> out,
           framebuffer_command_s: chan<memory::MemoryCommand> out,
           framebuffer_response_r: chan<memory::MemoryResponse> in,
           framebuffer_pixel_read_s: chan<memory::MemoryRequestPair> out,
           framebuffer_pixel_read_response_r: chan<memory::MemoryResponsePair> in,
           framebuffer_pixel_write_s: chan<memory::MemoryRequestPair> out,
           framebuffer_pixel_write_response_r: chan<memory::MemoryResponsePair> in,
           framebuffer_fastfill_s: chan<memory::MemoryRequestPair> out,
           framebuffer_fastfill_response_r: chan<memory::MemoryResponsePair> in,
           texture_cache_request_s: chan<texture_cache::CacheRequest> out,
           texture_cache_response_r: chan<texture_cache::CacheResponse> in,
           texture_block_s: chan<memory::MemoryBlockWrite> out,
           texture_block_response_r: chan<memory::MemoryBlockResponse> in) {
        (
            command_r, de10_request_r, burst_r, response_s, framebuffer_command_s,
            framebuffer_response_r, framebuffer_pixel_read_s, framebuffer_pixel_read_response_r,
            framebuffer_pixel_write_s, framebuffer_pixel_write_response_r, framebuffer_fastfill_s,
            framebuffer_fastfill_response_r, texture_cache_request_s, texture_cache_response_r,
            texture_block_s, texture_block_response_r,
        )
    }

    init { initial_state() }

    next(state: CoreState) {
        let default_command = CoreCommand { write: u1:0, address: u24:0, data: u32:0, mask: u4:0 };
        let rendering_busy = state.raster.active == u1:1 || state.fastfill.active == u1:1;
        let (tok, core_command, core_command_valid) = if rendering_busy {
            (join(), default_command, false)
        } else {
            recv_non_blocking(join(), command_r, default_command)
        };
        let default_request = De10MmioRequest {
            address: u22:0,
            read: u1:0,
            write: u1:0,
            byte_enable: u4:0,
            write_data: u32:0,
        };
        let (tok, request, request_valid) = if rendering_busy || core_command_valid {
            (tok, default_request, false)
        } else {
            recv_non_blocking(tok, de10_request_r, default_request)
        };
        let request_legal = de10_request_valid(request);
        let command = if request_valid { de10_core_command(request) } else { core_command };
        let command_valid = core_command_valid || (request_valid && request_legal);
        let response_valid = core_command_valid || request_valid;
        let default_burst = TextureBurst { commands: CoreCommand[32]:[default_command, ...] };
        let (tok, burst, burst_valid) = if ENABLE_BURSTS {
            if rendering_busy || core_command_valid || request_valid {
                (tok, default_burst, false)
            } else {
                recv_non_blocking(tok, burst_r, default_burst)
            }
        } else {
            (tok, default_burst, false)
        };
        let (texture_block, texture_block_valid) = if ENABLE_BURSTS {
            texture_block_write(state, burst)
        } else {
            (
                memory::MemoryBlockWrite {
                    address: u22:0,
                    data: u32[32]:[u32:0, ...],
                    masks: u4[32]:[u4:0, ...],
                }, false,
            )
        };
        let is_framebuffer =
            command_valid && command.address >= u24:0x400000 && command.address < u24:0x800000;
        let is_texture = command_valid && command.address >= u24:0x800000;
        let framebuffer_command = memory::MemoryCommand {
            write: command.write,
            address: (command.address - u24:0x400000)[2+:u22],
            data: command.data,
            mask: command.mask,
        };
        let texture_pci_address = (command.address - u24:0x800000) as u23;
        let texture_download = texture_download_address(state, texture_pci_address);
        let texture_write = command.write == u1:1;
        let texture_access = is_texture && (!texture_write || texture_download[26+:u1] == u1:1);
        let texture_address =
            if texture_write { texture_download[0+:u26] } else { texture_pci_address as u26 };
        let texture_command = memory::MemoryCommand {
            write: command.write,
            address: texture_address[2+:u22],
            data: command.data,
            mask: command.mask,
        };
        let tok = send_if(tok, framebuffer_command_s, is_framebuffer, framebuffer_command);
        let (tok, framebuffer_response) = recv_if(
            tok, framebuffer_response_r, is_framebuffer,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
        let tok = send_if(
            tok, texture_cache_request_s, texture_access,
            texture_cache::memory_request(texture_command));
        let (tok, texture_cache_response) = recv_if(
            tok, texture_cache_response_r, texture_access,
            texture_cache::CacheResponse {
                memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
                texels: u16[4]:[u16:0, ...],
                hits: u1[4]:[u1:0, ...],
                total_hits: u32:0,
                total_misses: u32:0,
                all_hit_requests: u32:0,
                missed_requests: u32:0,
            });
        let texture_response = texture_cache_response.memory_response;
        let block_access = burst_valid && texture_block_valid;
        let texture_layout_write = command_valid && command.write == u1:1 &&
                                   (command.address == u24:0x300 || command.address == u24:0x304 ||
                                   command.address == u24:0x30c || command.address == u24:0x310 ||
                                   command.address == u24:0x314 || command.address == u24:0x318);
        let triangle_start = command_valid && command.write == u1:1 &&
                             (command.address == u24:0x080 || command.address == u24:0x100);
        let narrow_texture = flush_narrow_texture_cache(
            state.raster.active || triangle_start, state.texture.t_lod, state.texture.texture_mode);
        let cache_invalidate = (block_access || texture_layout_write || narrow_texture) &&
                               (HOSTED_DE10 || state.de10_mode || request_valid);
        let tok = send_if(
            tok, texture_cache_request_s, cache_invalidate, texture_cache::invalidate_request());
        let (tok, _) = recv_if(
            tok, texture_cache_response_r, cache_invalidate,
            texture_cache::CacheResponse {
                memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
                texels: u16[4]:[u16:0, ...],
                hits: u1[4]:[u1:0, ...],
                total_hits: u32:0,
                total_misses: u32:0,
                all_hit_requests: u32:0,
                missed_requests: u32:0,
            });
        let (tok, texture_block_response) = if ENABLE_BURSTS {
            let tok = send_if(tok, texture_block_s, block_access, texture_block);
            recv_if(
                tok, texture_block_response_r, burst_valid && texture_block_valid,
                memory::MemoryBlockResponse { success: u1:0 })
        } else {
            (tok, memory::MemoryBlockResponse { success: u1:0 })
        };
        let (next_state, register_response) = if command_valid && !is_framebuffer && !is_texture {
            step(state, command)
        } else {
            (state, CoreResponse { data: u32:0, success: u1:0 })
        };
        let next_state =
            CoreState { de10_mode: next_state.de10_mode || request_valid, ..next_state };
        let response = if command_valid && command.write == u1:0 && command.address == u24:0x3f0 {
            CoreResponse { data: state.texture_cache_hits, success: u1:1 }
        } else if command_valid && command.write == u1:0 && command.address == u24:0x3f4 {
            CoreResponse { data: state.texture_cache_misses, success: u1:1 }
        } else if command_valid && command.write == u1:0 && command.address == u24:0x3f8 {
            CoreResponse { data: state.texture_cache_all_hit_requests, success: u1:1 }
        } else if command_valid && command.write == u1:0 && command.address == u24:0x3fc {
            CoreResponse { data: state.texture_cache_missed_requests, success: u1:1 }
        } else if is_framebuffer {
            CoreResponse { data: framebuffer_response.data, success: framebuffer_response.success }
        } else if is_texture {
            CoreResponse { data: texture_response.data, success: texture_response.success }
        } else {
            register_response
        };
        let tok = send_if(tok, response_s, response_valid, response);
        let burst_response = CoreResponse {
            data: u32:0,
            success: texture_block_valid && texture_block_response.success,
        };
        let tok =
            if ENABLE_BURSTS { send_if(tok, response_s, burst_valid, burst_response) } else { tok };
        let (commands, raster, fastfill, candidate, fill) =
            tick_events(next_state.commands, next_state.raster, next_state.fastfill, u1:0);
        let next_state = CoreState { commands, raster, fastfill, ..next_state };
        let raster_valid = candidate_visible(
            next_state.registers[u8:68], next_state.registers[u8:70], next_state.registers[u8:71],
            candidate);
        let sample = texture_request_values(
            pixel_registers(next_state.registers), pixel_float_shadows(next_state.float_shadows),
            next_state.raster_uses_float_shadows, next_state.texture, next_state.texture_layout,
            candidate);
        let textured = raster_valid && next_state.registers[u8:65][27+:u1] == u1:1;
        let address0 = sample.addresses[78+:u26];
        let address1 = sample.addresses[52+:u26];
        let address2 = sample.addresses[26+:u26];
        let address3 = sample.addresses[0+:u26];
        let sample_command0 = memory::MemoryCommand {
            write: u1:0,
            address: address0[2+:u22],
            data: u32:0,
            mask: u4:0,
        };
        let sample_command1 = memory::MemoryCommand {
            write: u1:0,
            address: address1[2+:u22],
            data: u32:0,
            mask: u4:0,
        };
        let sample_command2 = memory::MemoryCommand {
            write: u1:0,
            address: address2[2+:u22],
            data: u32:0,
            mask: u4:0,
        };
        let sample_command3 = memory::MemoryCommand {
            write: u1:0,
            address: address3[2+:u22],
            data: u32:0,
            mask: u4:0,
        };
        let de10_sample = textured && (HOSTED_DE10 || next_state.de10_mode == u1:1);
        let direct_sample = textured && !HOSTED_DE10 && next_state.de10_mode == u1:0;
        let read_mask = texture_sampler::read_mask(sample.addresses);
        let read0 = direct_sample;
        let read1 = direct_sample && read_mask[1+:u1] == u1:1;
        let read2 = direct_sample && read_mask[2+:u1] == u1:1;
        let read3 = direct_sample && read_mask[3+:u1] == u1:1;
        let default_sample = memory::MemoryResponse { data: u32:0, success: u1:0 };
        let tok = send_if(
            tok, texture_cache_request_s, read0, texture_cache::memory_request(sample_command0));
        let (tok, sample0_cache) = recv_if(
            tok, texture_cache_response_r, read0,
            texture_cache::CacheResponse {
                memory_response: default_sample,
                texels: u16[4]:[u16:0, ...],
                hits: u1[4]:[u1:0, ...],
                total_hits: u32:0,
                total_misses: u32:0,
                all_hit_requests: u32:0,
                missed_requests: u32:0,
            });
        let sample0 = sample0_cache.memory_response;
        let tok = send_if(
            tok, texture_cache_request_s, read1, texture_cache::memory_request(sample_command1));
        let (tok, sample1_cache) = recv_if(
            tok, texture_cache_response_r, read1,
            texture_cache::CacheResponse {
                memory_response: default_sample,
                texels: u16[4]:[u16:0, ...],
                hits: u1[4]:[u1:0, ...],
                total_hits: u32:0,
                total_misses: u32:0,
                all_hit_requests: u32:0,
                missed_requests: u32:0,
            });
        let sample1_read = sample1_cache.memory_response;
        let sample1 = if read1 { sample1_read } else { sample0 };
        let tok = send_if(
            tok, texture_cache_request_s, read2, texture_cache::memory_request(sample_command2));
        let (tok, sample2_cache) = recv_if(
            tok, texture_cache_response_r, read2,
            texture_cache::CacheResponse {
                memory_response: default_sample,
                texels: u16[4]:[u16:0, ...],
                hits: u1[4]:[u1:0, ...],
                total_hits: u32:0,
                total_misses: u32:0,
                all_hit_requests: u32:0,
                missed_requests: u32:0,
            });
        let sample2_read = sample2_cache.memory_response;
        let sample2 = if read2 {
            sample2_read
        } else if sample_command2.address == sample_command0.address {
            sample0
        } else {
            sample1
        };
        let tok = send_if(
            tok, texture_cache_request_s, read3, texture_cache::memory_request(sample_command3));
        let (tok, sample3_cache) = recv_if(
            tok, texture_cache_response_r, read3,
            texture_cache::CacheResponse {
                memory_response: default_sample,
                texels: u16[4]:[u16:0, ...],
                hits: u1[4]:[u1:0, ...],
                total_hits: u32:0,
                total_misses: u32:0,
                all_hit_requests: u32:0,
                missed_requests: u32:0,
            });
        let sample3_read = sample3_cache.memory_response;
        let sample3 = if read3 {
            sample3_read
        } else if sample_command3.address == sample_command0.address {
            sample0
        } else if sample_command3.address == sample_command1.address {
            sample1
        } else {
            sample2
        };
        let tok = send_if(
            tok, texture_cache_request_s, de10_sample,
            texture_cache::sample_request(sample, next_state.texture_layout));
        let (tok, cached_sample) = recv_if(
            tok, texture_cache_response_r, de10_sample,
            texture_cache::CacheResponse {
                memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
                texels: u16[4]:[u16:0, ...],
                hits: u1[4]:[u1:0, ...],
                total_hits: u32:0,
                total_misses: u32:0,
                all_hit_requests: u32:0,
                missed_requests: u32:0,
            });
        let next_state = if de10_sample {
            CoreState {
                texture_cache_hits: cached_sample.total_hits,
                texture_cache_misses: cached_sample.total_misses,
                texture_cache_all_hit_requests: cached_sample.all_hit_requests,
                texture_cache_missed_requests: cached_sample.missed_requests,
                ..next_state
            }
        } else {
            next_state
        };
        let texture_rgba = if de10_sample {
            texture_sampler::decode_texels_with_palette(
                sample, cached_sample.texels[u32:0], cached_sample.texels[u32:1],
                cached_sample.texels[u32:2], cached_sample.texels[u32:3], next_state.palette)
        } else if direct_sample {
            texture_sampler::decode_with_palette(
                sample, sample0.data, sample1.data, sample2.data, sample3.data, next_state.palette)
        } else {
            u32:0
        };
        let source_color = combined_color_values(
            pixel_registers(next_state.registers), pixel_float_shadows(next_state.float_shadows),
            next_state.raster_uses_float_shadows, candidate, texture_rgba);
        let alpha_mode = next_state.registers[u8:67];
        let depth = new_depth_values(
            pixel_registers(next_state.registers), pixel_float_shadows(next_state.float_shadows),
            next_state.raster_uses_float_shadows, candidate);
        let aux = aux_write_values(
            next_state.registers[u8:133], next_state.registers[u8:134],
            next_state.registers[u8:68], next_state.registers[u8:135][22+:u10], candidate, depth,
            source_color[0+:u8]);
        let depth_access = raster_valid && next_state.registers[u8:68][4+:u1] == u1:1;
        let depth_read =
            memory::MemoryCommand { write: u1:0, address: aux.address, data: u32:0, mask: u4:0 };
        let provisional = raster_write_color(
            next_state.registers[u8:133], next_state.registers[u8:134],
            next_state.registers[u8:68], next_state.registers[u8:67],
            next_state.commands.swap.draw_count[0+:u1], next_state.registers[u8:135][22+:u10],
            candidate, source_color, u24:0);
        let blend_access = raster_valid &&
                           (alpha_mode[4+:u1] == u1:1 ||
                           next_state.registers[u8:68][19+:u1] == u1:1);
        let blend_read = memory::MemoryCommand {
            write: u1:0,
            address: provisional.address,
            data: u32:0,
            mask: u4:0,
        };
        let read_valid = (blend_access as u2) << u2:1 | (depth_access as u2);
        let framebuffer_reads = memory::MemoryRequestPair {
            valid: read_valid,
            commands: memory::MemoryCommand[2]:[depth_read, blend_read],
        };
        let tok = send_if(tok, framebuffer_pixel_read_s, read_valid != u2:0, framebuffer_reads);
        let (tok, framebuffer_read_responses) = recv_if(
            tok, framebuffer_pixel_read_response_r, read_valid != u2:0,
            memory::empty_response_pair());
        let depth_response = framebuffer_read_responses.responses[u32:0];
        let blend_response = framebuffer_read_responses.responses[u32:1];
        let old_depth = if aux.mask == u4:0xc {
            depth_response.data[16+:u16]
        } else {
            depth_response.data[0+:u16]
        };
        let depth_pass = next_state.registers[u8:68][4+:u1] == u1:0 ||
                         pixel::depth_compare(next_state.registers[u8:68][5+:u3], depth, old_depth) ==
                         u1:1;
        let tests = pixel::pixel_tests(
            source_color[8+:u24], next_state.registers[u8:77][0+:u24], source_color[0+:u8],
            alpha_mode[24+:u8], alpha_mode[1+:u3]);
        let tests_pass = (next_state.registers[u8:68][1+:u1] == u1:0 || tests[1+:u1] == u1:0) &&
                         (alpha_mode[0+:u1] == u1:0 || tests[0+:u1] == u1:1);
        let pixel_valid = raster_valid && depth_pass && tests_pass;
        let destination = if provisional.mask == u4:0xc {
            blend_response.data[16+:u16]
        } else {
            blend_response.data[0+:u16]
        };
        let destination_rgb = decode_rgb565(destination);
        let destination_rgb = if next_state.registers[u8:68][19+:u1] == u1:1 {
            dither::subtract_rgb(
                destination_rgb, candidate.x as u16,
                framebuffer_y_values(
                    next_state.registers[u8:68], next_state.registers[u8:135][22+:u10], candidate),
                next_state.registers[u8:68][11+:u1])
        } else {
            destination_rgb
        };
        let color_valid = pixel_valid && next_state.registers[u8:68][9+:u1] == u1:1;
        let color_write = raster_write_color(
            next_state.registers[u8:133], next_state.registers[u8:134],
            next_state.registers[u8:68], next_state.registers[u8:67],
            next_state.commands.swap.draw_count[0+:u1], next_state.registers[u8:135][22+:u10],
            candidate, source_color, destination_rgb);
        let aux_valid = pixel_valid && next_state.registers[u8:68][10+:u1] == u1:1;
        let write_valid = (aux_valid as u2) << u2:1 | (color_valid as u2);
        let framebuffer_writes = memory::MemoryRequestPair {
            valid: write_valid,
            commands: memory::MemoryCommand[2]:[color_write, aux],
        };
        let tok = send_if(tok, framebuffer_pixel_write_s, write_valid != u2:0, framebuffer_writes);
        let (tok, _) = recv_if(
            tok, framebuffer_pixel_write_response_r, write_valid != u2:0,
            memory::empty_response_pair());
        let color_fill = fill.valid == u1:1 && fill.rgb_write == u1:1;
        let aux_fill = fill.valid == u1:1 && fill.aux_write == u1:1;
        let fill_valid = (aux_fill as u2) << u2:1 | (color_fill as u2);
        let framebuffer_fills = memory::MemoryRequestPair {
            valid: fill_valid,
            commands: memory::MemoryCommand[2]:[
                fastfill_write(fill.color_base, fill, fill.color),
                fastfill_write(fill.aux_base, fill, fill.depth),
            ],
        };
        let tok = send_if(tok, framebuffer_fastfill_s, fill_valid != u2:0, framebuffer_fills);
        let (tok, _) = recv_if(
            tok, framebuffer_fastfill_response_r, fill_valid != u2:0, memory::empty_response_pair());
        if ENABLE_BURSTS {
            if burst_valid { advance_burst_ticks(next_state) } else { next_state }
        } else {
            next_state
        }
    }
}

proc core_sim_hosted_controller<EXPORT_SCANOUT: bool = {false}> {
    command_r: chan<CoreCommand> in;
    de10_request_r: chan<De10MmioRequest> in;
    response_s: chan<CoreResponse> out;
    framebuffer_command_s: chan<memory::MemoryCommand> out;
    framebuffer_response_r: chan<memory::MemoryResponse> in;
    framebuffer_fastfill_s: chan<memory::MemoryRequestPair> out;
    framebuffer_fastfill_response_r: chan<memory::MemoryResponsePair> in;
    cache_request_s: chan<texture_cache::CacheRequest> out;
    cache_response_r: chan<texture_cache::CacheResponse> in;
    pixel_job_s: chan<PixelJob> out;
    pixel_result_r: chan<PixelResult> in;
    pixel_cache_request_r: chan<texture_cache::CacheRequest> in;
    pixel_cache_response_s: chan<PixelCacheResponse> out;
    triangle_setup_request_s: chan<TriangleSetupRequest> out;
    triangle_setup_response_r: chan<uN[560]> in;
    v_retrace_r: chan<u1> in;
    scanout_s: chan<ScanoutState> out;

    config(command_r: chan<CoreCommand> in, de10_request_r: chan<De10MmioRequest> in,
           response_s: chan<CoreResponse> out,
           framebuffer_command_s: chan<memory::MemoryCommand> out,
           framebuffer_response_r: chan<memory::MemoryResponse> in,
           framebuffer_fastfill_s: chan<memory::MemoryRequestPair> out,
           framebuffer_fastfill_response_r: chan<memory::MemoryResponsePair> in,
           cache_request_s: chan<texture_cache::CacheRequest> out,
           cache_response_r: chan<texture_cache::CacheResponse> in, pixel_job_s: chan<PixelJob> out,
           pixel_result_r: chan<PixelResult> in,
           pixel_cache_request_r: chan<texture_cache::CacheRequest> in,
           pixel_cache_response_s: chan<PixelCacheResponse> out,
           triangle_setup_request_s: chan<TriangleSetupRequest> out,
           triangle_setup_response_r: chan<uN[560]> in,
           v_retrace_r: chan<u1> in, scanout_s: chan<ScanoutState> out) {
        (
            command_r, de10_request_r, response_s, framebuffer_command_s, framebuffer_response_r,
            framebuffer_fastfill_s, framebuffer_fastfill_response_r, cache_request_s,
            cache_response_r, pixel_job_s, pixel_result_r, pixel_cache_request_r,
            pixel_cache_response_s, triangle_setup_request_s, triangle_setup_response_r,
            v_retrace_r, scanout_s,
        )
    }

    init {
        HostedControllerState {
            core: initial_state(),
            pixels_pending: u1:0,
            triangle_pending: u1:0,
            palette_cache: empty_cache_response(),
            palette_entries: u24[4]:[u24:0, ...],
            palette_index: u2:0,
            palette_active: u1:0,
        }
    }

    next(state: HostedControllerState) {
        let (_, v_retrace, v_retrace_valid) = recv_non_blocking(join(), v_retrace_r, u1:0);
        let default_result = PixelResult {
            cache_valid: u1:0,
            texture_cache_hits: u32:0,
            texture_cache_misses: u32:0,
            texture_cache_all_hit_requests: u32:0,
            texture_cache_missed_requests: u32:0,
        };
        let (result_tok, pixel_result, result_valid) =
            recv_non_blocking(join(), pixel_result_r, default_result);
        let core = if result_valid && pixel_result.cache_valid == u1:1 {
            CoreState {
                texture_cache_hits: pixel_result.texture_cache_hits,
                texture_cache_misses: pixel_result.texture_cache_misses,
                texture_cache_all_hit_requests: pixel_result.texture_cache_all_hit_requests,
                texture_cache_missed_requests: pixel_result.texture_cache_missed_requests,
                ..state.core
            }
        } else {
            state.core
        };
        let pixels_pending = if result_valid { u1:0 } else { state.pixels_pending };
        let (triangle_tok, triangle_setup, triangle_valid) = recv_non_blocking(
            join(), triangle_setup_response_r, uN[560]:0);
        let core = if triangle_valid {
            CoreState { raster: raster_state::from_triangle_setup(triangle_setup), ..core }
        } else {
            core
        };
        let triangle_pending = state.triangle_pending && !triangle_valid;
        let rendering_busy = core.raster.active == u1:1 || core.fastfill.active == u1:1 ||
                             pixels_pending == u1:1 || triangle_pending == u1:1;
        let default_command = CoreCommand { write: u1:0, address: u24:0, data: u32:0, mask: u4:0 };
        let (command_tok, core_command, core_command_valid) = if rendering_busy {
            (join(), default_command, false)
        } else {
            recv_non_blocking(join(), command_r, default_command)
        };
        let default_request = De10MmioRequest {
            address: u22:0,
            read: u1:0,
            write: u1:0,
            byte_enable: u4:0,
            write_data: u32:0,
        };
        let (request_tok, request, request_valid) = if rendering_busy || core_command_valid {
            (join(), default_request, false)
        } else {
            recv_non_blocking(join(), de10_request_r, default_request)
        };
        let request_legal = de10_request_valid(request);
        let command = if request_valid { de10_core_command(request) } else { core_command };
        let command_valid = core_command_valid || (request_valid && request_legal);
        let response_valid = core_command_valid || request_valid;
        let is_framebuffer =
            command_valid && command.address >= u24:0x400000 && command.address < u24:0x800000;
        let is_texture = command_valid && command.address >= u24:0x800000;
        let framebuffer_command = memory::MemoryCommand {
            write: command.write,
            address: (command.address - u24:0x400000)[2+:u22],
            data: command.data,
            mask: command.mask,
        };
        let framebuffer_tok =
            send_if(join(), framebuffer_command_s, is_framebuffer, framebuffer_command);
        let (framebuffer_tok, framebuffer_response) = recv_if(
            framebuffer_tok, framebuffer_response_r, is_framebuffer,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
        let texture_pci_address = (command.address - u24:0x800000) as u23;
        let texture_download = texture_download_address(core, texture_pci_address);
        let texture_write = command.write == u1:1;
        let texture_access = is_texture && (!texture_write || texture_download[26+:u1] == u1:1);
        let texture_address =
            if texture_write { texture_download[0+:u26] } else { texture_pci_address as u26 };
        let texture_command = memory::MemoryCommand {
            write: command.write,
            address: texture_address[2+:u22],
            data: command.data,
            mask: command.mask,
        };
        let texture_layout_write = command_valid && command.write == u1:1 &&
                                   (command.address == u24:0x300 || command.address == u24:0x304 ||
                                   command.address == u24:0x30c || command.address == u24:0x310 ||
                                   command.address == u24:0x314 || command.address == u24:0x318);
        let cache_control = texture_access || texture_layout_write;
        let (pixel_cache_tok, pixel_cache_request, pixel_cache_valid) =
            if cache_control || state.palette_active == u1:1 {
            (join(), texture_cache::invalidate_request(), false)
        } else {
            recv_non_blocking(join(), pixel_cache_request_r, texture_cache::invalidate_request())
        };
        let cache_access = cache_control || pixel_cache_valid;
        let cache_request = if texture_access {
            texture_cache::memory_request(texture_command)
        } else if texture_layout_write {
            texture_cache::invalidate_request()
        } else {
            pixel_cache_request
        };
        let cache_tok = send_if(join(), cache_request_s, cache_access, cache_request);
        let (cache_tok, cache_response) =
            recv_if(cache_tok, cache_response_r, cache_access, empty_cache_response());
        let palette_active = state.palette_active || pixel_cache_valid;
        let palette_cache = if pixel_cache_valid { cache_response } else { state.palette_cache };
        let palette_index = if state.palette_active == u1:1 { state.palette_index } else { u2:0 };
        let palette_entries = if palette_active == u1:1 {
            update(
                state.palette_entries, palette_index,
                core.palette[palette_cache.texels[palette_index][0+:u8]])
        } else {
            state.palette_entries
        };
        let palette_done = palette_active == u1:1 && palette_index == u2:3;
        let cache_tok = send_if(
            cache_tok, pixel_cache_response_s, palette_done,
            PixelCacheResponse { cache: palette_cache, palette_entries });
        let triangle_command = command_valid && !is_framebuffer && !is_texture &&
                               is_triangle_command(command);
        let triangle_tok = send_if(
            triangle_tok, triangle_setup_request_s, triangle_command,
            triangle_setup_request(core, command));
        let (next_core, register_response) = if command_valid && !is_framebuffer && !is_texture {
            step_with_raster_setup(core, command, u1:1)
        } else {
            (core, CoreResponse { data: u32:0, success: u1:0 })
        };
        let next_core = CoreState { de10_mode: u1:1, ..next_core };
        let response = if command_valid && command.write == u1:0 && command.address == u24:0x3f0 {
            CoreResponse { data: core.texture_cache_hits, success: u1:1 }
        } else if command_valid && command.write == u1:0 && command.address == u24:0x3f4 {
            CoreResponse { data: core.texture_cache_misses, success: u1:1 }
        } else if command_valid && command.write == u1:0 && command.address == u24:0x3f8 {
            CoreResponse { data: core.texture_cache_all_hit_requests, success: u1:1 }
        } else if command_valid && command.write == u1:0 && command.address == u24:0x3fc {
            CoreResponse { data: core.texture_cache_missed_requests, success: u1:1 }
        } else if is_framebuffer {
            CoreResponse { data: framebuffer_response.data, success: framebuffer_response.success }
        } else if is_texture {
            CoreResponse {
                data: cache_response.memory_response.data,
                success: cache_response.memory_response.success,
            }
        } else {
            register_response
        };
        let response_tok = send_if(join(), response_s, response_valid, response);
        let raster_tick = palette_active == u1:0 && pixels_pending == u1:0 &&
                          next_core.raster.active == u1:1 && next_core.fastfill.active == u1:0;
        let (commands, raster, fastfill, candidate, fill) =
            if palette_active == u1:1 || pixels_pending == u1:1 {
            (
                next_core.commands, next_core.raster, next_core.fastfill,
                raster_state::PixelCandidate { valid: u1:0, x: s16:0, y: s16:0 },
                fastfill_state::FastfillWord {
                    valid: u1:0, x: u16:0, y: u16:0, color: u32:0, depth: u32:0, mask: u4:0,
                    color_base: u26:0, aux_base: u26:0, stride: u11:0, rgb_write: u1:0,
                    aux_write: u1:0,
                },
            )
        } else {
            tick_events<false>(
                next_core.commands, next_core.raster, next_core.fastfill,
                if v_retrace_valid { v_retrace } else { u1:0 })
        };
        let ticked_core = CoreState { commands, raster, fastfill, ..next_core };
        let scanout_tok = send_if(join(), scanout_s, EXPORT_SCANOUT, scanout_state(ticked_core));
        let job = PixelJob {
            core: pixel_context(ticked_core),
            candidate,
            last: raster_tick && raster.active == u1:0,
        };
        let job_tok = send_if(join(), pixel_job_s, raster_tick, job);
        let color_fill = fill.valid == u1:1 && fill.rgb_write == u1:1;
        let aux_fill = fill.valid == u1:1 && fill.aux_write == u1:1;
        let fill_valid = (aux_fill as u2) << u2:1 | (color_fill as u2);
        let framebuffer_fills = memory::MemoryRequestPair {
            valid: fill_valid,
            commands: memory::MemoryCommand[2]:[
                fastfill_write(fill.color_base, fill, fill.color),
                fastfill_write(fill.aux_base, fill, fill.depth),
            ],
        };
        let fill_tok =
            send_if(join(), framebuffer_fastfill_s, fill_valid != u2:0, framebuffer_fills);
        let (fill_tok, _) = recv_if(
            fill_tok, framebuffer_fastfill_response_r, fill_valid != u2:0,
            memory::empty_response_pair());
        HostedControllerState {
            core: ticked_core,
            pixels_pending: if raster_tick { u1:1 } else { pixels_pending },
            triangle_pending: triangle_pending || triangle_command,
            palette_cache,
            palette_entries,
            palette_index: if palette_active && !palette_done { palette_index + u2:1 } else { u2:0 },
            palette_active: palette_active && !palette_done,
        }
    }
}

pub proc core_sim_hosted {
    config(command_r: chan<CoreCommand> in, de10_request_r: chan<De10MmioRequest> in,
           burst_r: chan<TextureBurst> in, response_s: chan<CoreResponse> out,
           framebuffer_command_s: chan<memory::MemoryCommand> out,
           framebuffer_response_r: chan<memory::MemoryResponse> in,
           framebuffer_pixel_read_s: chan<memory::MemoryRequestPair> out,
           framebuffer_pixel_read_response_r: chan<memory::MemoryResponsePair> in,
           framebuffer_pixel_write_s: chan<memory::MemoryRequestPair> out,
           framebuffer_pixel_write_response_r: chan<memory::MemoryResponsePair> in,
           framebuffer_fastfill_s: chan<memory::MemoryRequestPair> out,
           framebuffer_fastfill_response_r: chan<memory::MemoryResponsePair> in,
           texture_command_s: chan<memory::MemoryCommand> out,
           texture_response_r: chan<memory::MemoryResponse> in,
           texture_fill_s: chan<memory::MemoryBlockRead> out,
           texture_fill_response_r: chan<memory::MemoryBlockReadResponse> in,
           bank0_command_s: chan<texture_cache::PackedBankCommand> out,
           bank0_response_r: chan<memory::MemoryResponse> in,
           bank1_command_s: chan<texture_cache::PackedBankCommand> out,
           bank1_response_r: chan<memory::MemoryResponse> in,
           bank2_command_s: chan<texture_cache::PackedBankCommand> out,
           bank2_response_r: chan<memory::MemoryResponse> in,
           bank3_command_s: chan<texture_cache::PackedBankCommand> out,
           bank3_response_r: chan<memory::MemoryResponse> in,
           texture_block_s: chan<memory::MemoryBlockWrite> out,
           texture_block_response_r: chan<memory::MemoryBlockResponse> in) {
        let (texture_cache_request_s, texture_cache_request_r) =
            chan<texture_cache::CacheRequest>("texture_cache_request");
        let (texture_cache_response_s, texture_cache_response_r) =
            chan<texture_cache::CacheResponse>("texture_cache_response");
        let (pixel_job_s, pixel_job_r) = chan<PixelJob>("pixel_job");
        let (pixel_result_s, pixel_result_r) = chan<PixelResult>("pixel_result");
        let (pixel_interpolation_request_s, pixel_interpolation_request_r) =
            chan<PixelInterpolationRequest>("pixel_interpolation_request");
        let (pixel_interpolation_response_s, pixel_interpolation_response_r) =
            chan<PixelInterpolatedValues>("pixel_interpolation_response");
        let (pixel_cache_request_s, pixel_cache_request_r) =
            chan<texture_cache::CacheRequest>("pixel_cache_request");
        let (pixel_cache_response_s, pixel_cache_response_r) =
            chan<PixelCacheResponse>("pixel_cache_response");
        let (triangle_setup_request_s, triangle_setup_request_r) =
            chan<TriangleSetupRequest>("triangle_setup_request");
        let (triangle_edges_s, triangle_edges_r) = chan<TriangleEdges>("triangle_edges");
        let (triangle_setup_response_s, triangle_setup_response_r) =
            chan<uN[560]>("triangle_setup_response");
        let (_, v_retrace_r) = chan<u1>("v_retrace");
        let (scanout_s, _) = chan<ScanoutState>("scanout");
        spawn texture_cache::texture_cache(
            texture_cache_request_r, texture_cache_response_s, texture_command_s,
            texture_response_r, texture_fill_s, texture_fill_response_r, bank0_command_s,
            bank0_response_r, bank1_command_s, bank1_response_r, bank2_command_s, bank2_response_r,
            bank3_command_s, bank3_response_r);
        spawn hosted_triangle_edge_setup(triangle_setup_request_r, triangle_edges_s);
        spawn hosted_triangle_start_setup(triangle_edges_r, triangle_setup_response_s);
        spawn hosted_pixel_interpolator(
            pixel_interpolation_request_r, pixel_interpolation_response_s);
        spawn hosted_pixel_pipeline(
            pixel_job_r, pixel_result_s, pixel_interpolation_request_s,
            pixel_interpolation_response_r, pixel_cache_request_s, pixel_cache_response_r,
            framebuffer_pixel_read_s, framebuffer_pixel_read_response_r, framebuffer_pixel_write_s,
            framebuffer_pixel_write_response_r);
        spawn core_sim_hosted_controller<false>(
            command_r, de10_request_r, response_s, framebuffer_command_s, framebuffer_response_r,
            framebuffer_fastfill_s, framebuffer_fastfill_response_r, texture_cache_request_s,
            texture_cache_response_r, pixel_job_s, pixel_result_r, pixel_cache_request_r,
            pixel_cache_response_s, triangle_setup_request_s, triangle_setup_response_r,
            v_retrace_r, scanout_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

pub proc core_sim_de10 {
    config(de10_request_r: chan<De10MmioRequest> in, v_retrace_r: chan<u1> in,
           response_s: chan<CoreResponse> out, scanout_s: chan<ScanoutState> out,
           framebuffer_transfer_s: chan<de10_adapter::MemoryTransfer> out,
           framebuffer_transfer_response_r: chan<de10_adapter::MemoryTransferResponse> in,
           color_transfer_s: chan<de10_adapter::MemoryTransfer> out,
           color_transfer_response_r: chan<de10_adapter::MemoryTransferResponse> in,
           aux_transfer_s: chan<de10_adapter::MemoryTransfer> out,
           aux_transfer_response_r: chan<de10_adapter::MemoryTransferResponse> in,
           texture_transfer_s: chan<de10_adapter::MemoryTransfer> out,
           texture_transfer_response_r: chan<de10_adapter::MemoryTransferResponse> in,
           bank0_command_s: chan<texture_cache::PackedBankCommand> out,
           bank0_response_r: chan<memory::MemoryResponse> in,
           bank1_command_s: chan<texture_cache::PackedBankCommand> out,
           bank1_response_r: chan<memory::MemoryResponse> in,
           bank2_command_s: chan<texture_cache::PackedBankCommand> out,
           bank2_response_r: chan<memory::MemoryResponse> in,
           bank3_command_s: chan<texture_cache::PackedBankCommand> out,
           bank3_response_r: chan<memory::MemoryResponse> in) {
        let (_, command_r) = chan<CoreCommand>("command");
        let (framebuffer_command_s, framebuffer_command_r) =
            chan<memory::MemoryCommand>("framebuffer_command");
        let (framebuffer_response_s, framebuffer_response_r) =
            chan<memory::MemoryResponse>("framebuffer_response");
        let (pixel_read_s, pixel_read_r) =
            chan<memory::MemoryRequestPair>("framebuffer_pixel_read");
        let (pixel_read_response_s, pixel_read_response_r) =
            chan<memory::MemoryResponsePair>("framebuffer_pixel_read_response");
        let (pixel_write_s, pixel_write_r) =
            chan<memory::MemoryRequestPair>("framebuffer_pixel_write");
        let (pixel_write_response_s, pixel_write_response_r) =
            chan<memory::MemoryResponsePair>("framebuffer_pixel_write_response");
        let (fastfill_s, fastfill_r) = chan<memory::MemoryRequestPair>("framebuffer_fastfill");
        let (fastfill_response_s, fastfill_response_r) =
            chan<memory::MemoryResponsePair>("framebuffer_fastfill_response");
        let (texture_command_s, texture_command_r) = chan<memory::MemoryCommand>("texture_command");
        let (texture_response_s, texture_response_r) =
            chan<memory::MemoryResponse>("texture_response");
        let (texture_fill_s, texture_fill_r) = chan<memory::MemoryBlockRead>("texture_fill");
        let (texture_fill_response_s, texture_fill_response_r) =
            chan<memory::MemoryBlockReadResponse>("texture_fill_response");
        let (texture_cache_request_s, texture_cache_request_r) =
            chan<texture_cache::CacheRequest>("texture_cache_request");
        let (texture_cache_response_s, texture_cache_response_r) =
            chan<texture_cache::CacheResponse>("texture_cache_response");
        let (pixel_job_s, pixel_job_r) = chan<PixelJob>("pixel_job");
        let (pixel_result_s, pixel_result_r) = chan<PixelResult>("pixel_result");
        let (pixel_interpolation_request_s, pixel_interpolation_request_r) =
            chan<PixelInterpolationRequest>("pixel_interpolation_request");
        let (pixel_interpolation_response_s, pixel_interpolation_response_r) =
            chan<PixelInterpolatedValues>("pixel_interpolation_response");
        let (pixel_cache_request_s, pixel_cache_request_r) =
            chan<texture_cache::CacheRequest>("pixel_cache_request");
        let (pixel_cache_response_s, pixel_cache_response_r) =
            chan<PixelCacheResponse>("pixel_cache_response");
        let (triangle_setup_request_s, triangle_setup_request_r) =
            chan<TriangleSetupRequest>("triangle_setup_request");
        let (triangle_edges_s, triangle_edges_r) = chan<TriangleEdges>("triangle_edges");
        let (triangle_setup_response_s, triangle_setup_response_r) =
            chan<uN[560]>("triangle_setup_response");

        let (read_color_s, read_color_r) = chan<memory::MemoryCommand>("read_color");
        let (read_color_response_s, read_color_response_r) =
            chan<memory::MemoryResponse>("read_color_response");
        let (read_aux_s, read_aux_r) = chan<memory::MemoryCommand>("read_aux");
        let (read_aux_response_s, read_aux_response_r) =
            chan<memory::MemoryResponse>("read_aux_response");
        let (write_color_s, write_color_r) = chan<memory::MemoryCommand>("write_color");
        let (write_color_response_s, write_color_response_r) =
            chan<memory::MemoryResponse>("write_color_response");
        let (write_aux_s, write_aux_r) = chan<memory::MemoryCommand>("write_aux");
        let (write_aux_response_s, write_aux_response_r) =
            chan<memory::MemoryResponse>("write_aux_response");
        let (fill_color_s, fill_color_r) = chan<memory::MemoryCommand>("fill_color");
        let (fill_color_response_s, fill_color_response_r) =
            chan<memory::MemoryResponse>("fill_color_response");
        let (fill_aux_s, fill_aux_r) = chan<memory::MemoryCommand>("fill_aux");
        let (fill_aux_response_s, fill_aux_response_r) =
            chan<memory::MemoryResponse>("fill_aux_response");

        let (color_command_s, color_command_r) = chan<memory::MemoryCommand>("color_command");
        let (color_response_s, color_response_r) = chan<memory::MemoryResponse>("color_response");
        let (aux_command_s, aux_command_r) = chan<memory::MemoryCommand>("aux_command");
        let (aux_response_s, aux_response_r) = chan<memory::MemoryResponse>("aux_response");
        let (texture_word_transfer_s, texture_word_transfer_r) =
            chan<de10_adapter::MemoryTransfer>("texture_word_transfer");
        let (texture_word_transfer_response_s, texture_word_transfer_response_r) =
            chan<de10_adapter::MemoryTransferResponse>("texture_word_transfer_response");
        let (texture_fill_transfer_s, texture_fill_transfer_r) =
            chan<de10_adapter::MemoryTransfer>("texture_fill_transfer");
        let (texture_fill_transfer_response_s, texture_fill_transfer_response_r) =
            chan<de10_adapter::MemoryTransferResponse>("texture_fill_transfer_response");

        spawn texture_cache::texture_cache(
            texture_cache_request_r, texture_cache_response_s, texture_command_s,
            texture_response_r, texture_fill_s, texture_fill_response_r, bank0_command_s,
            bank0_response_r, bank1_command_s, bank1_response_r, bank2_command_s, bank2_response_r,
            bank3_command_s, bank3_response_r);
        spawn hosted_triangle_edge_setup(triangle_setup_request_r, triangle_edges_s);
        spawn hosted_triangle_start_setup(triangle_edges_r, triangle_setup_response_s);
        spawn hosted_pixel_interpolator(
            pixel_interpolation_request_r, pixel_interpolation_response_s);
        spawn hosted_pixel_pipeline(
            pixel_job_r, pixel_result_s, pixel_interpolation_request_s,
            pixel_interpolation_response_r, pixel_cache_request_s, pixel_cache_response_r,
            pixel_read_s, pixel_read_response_r, pixel_write_s, pixel_write_response_r);
        spawn core_sim_hosted_controller<true>(
            command_r, de10_request_r, response_s, framebuffer_command_s, framebuffer_response_r,
            fastfill_s, fastfill_response_r, texture_cache_request_s, texture_cache_response_r,
            pixel_job_s, pixel_result_r, pixel_cache_request_r, pixel_cache_response_s,
            triangle_setup_request_s, triangle_setup_response_r, v_retrace_r, scanout_s);

        spawn de10_adapter::pair_to_planes(
            pixel_read_r, pixel_read_response_s, read_aux_s, read_aux_response_r, read_color_s,
            read_color_response_r);
        spawn de10_adapter::pair_to_planes(
            pixel_write_r, pixel_write_response_s, write_color_s, write_color_response_r,
            write_aux_s, write_aux_response_r);
        spawn de10_adapter::pair_to_planes(
            fastfill_r, fastfill_response_s, fill_color_s, fill_color_response_r, fill_aux_s,
            fill_aux_response_r);
        spawn de10_adapter::memory_arbiter3(
            read_color_r, read_color_response_s, write_color_r, write_color_response_s,
            fill_color_r, fill_color_response_s, color_command_s, color_response_r);
        spawn de10_adapter::memory_arbiter3(
            read_aux_r, read_aux_response_s, write_aux_r, write_aux_response_s, fill_aux_r,
            fill_aux_response_s, aux_command_s, aux_response_r);

        spawn de10_adapter::word_transfer_adapter(
            framebuffer_command_r, framebuffer_response_s, framebuffer_transfer_s,
            framebuffer_transfer_response_r);
        spawn de10_adapter::word_transfer_adapter(
            color_command_r, color_response_s, color_transfer_s, color_transfer_response_r);
        spawn de10_adapter::word_transfer_adapter(
            aux_command_r, aux_response_s, aux_transfer_s, aux_transfer_response_r);
        spawn de10_adapter::word_transfer_adapter(
            texture_command_r, texture_response_s, texture_word_transfer_s,
            texture_word_transfer_response_r);
        spawn de10_adapter::block_read_transfer_adapter(
            texture_fill_r, texture_fill_response_s, texture_fill_transfer_s,
            texture_fill_transfer_response_r);
        spawn de10_adapter::transfer_arbiter2(
            texture_word_transfer_r, texture_word_transfer_response_s, texture_fill_transfer_r,
            texture_fill_transfer_response_s, texture_transfer_s, texture_transfer_response_r);
        ()
    }

    init { () }

    next(state: ()) { state }
}

pub proc core_sim<FB_WORDS: u32 = {u32:1048576}, TEX_WORDS: u32 = {u32:2097152}> {
    config(command_r: chan<CoreCommand> in, burst_r: chan<TextureBurst> in,
           response_s: chan<CoreResponse> out) {
        let (_, de10_request_r) = chan<De10MmioRequest>("de10_request");
        let (framebuffer_command_s, framebuffer_command_r) =
            chan<memory::MemoryCommand>("framebuffer_command");
        let (framebuffer_response_s, framebuffer_response_r) =
            chan<memory::MemoryResponse>("framebuffer_response");
        let (framebuffer_memory_command_s, framebuffer_memory_command_r) =
            chan<memory::MemoryCommand>("framebuffer_memory_command");
        let (framebuffer_memory_response_s, framebuffer_memory_response_r) =
            chan<memory::MemoryResponse>("framebuffer_memory_response");
        let (framebuffer_pixel_read_s, framebuffer_pixel_read_r) =
            chan<memory::MemoryRequestPair>("framebuffer_pixel_read");
        let (framebuffer_pixel_read_response_s, framebuffer_pixel_read_response_r) =
            chan<memory::MemoryResponsePair>("framebuffer_pixel_read_response");
        let (framebuffer_pixel_write_s, framebuffer_pixel_write_r) =
            chan<memory::MemoryRequestPair>("framebuffer_pixel_write");
        let (framebuffer_pixel_write_response_s, framebuffer_pixel_write_response_r) =
            chan<memory::MemoryResponsePair>("framebuffer_pixel_write_response");
        let (framebuffer_fastfill_s, framebuffer_fastfill_r) =
            chan<memory::MemoryRequestPair>("framebuffer_fastfill");
        let (framebuffer_fastfill_response_s, framebuffer_fastfill_response_r) =
            chan<memory::MemoryResponsePair>("framebuffer_fastfill_response");
        let (_, framebuffer_fill_r) = chan<memory::MemoryBlockRead>("framebuffer_fill");
        let (framebuffer_fill_response_s, _) =
            chan<memory::MemoryBlockReadResponse>("framebuffer_fill_response");
        let (texture_command_s, texture_command_r) = chan<memory::MemoryCommand>("texture_command");
        let (texture_response_s, texture_response_r) =
            chan<memory::MemoryResponse>("texture_response");
        let (texture_fill_s, texture_fill_r) = chan<memory::MemoryBlockRead>("texture_fill");
        let (texture_fill_response_s, texture_fill_response_r) =
            chan<memory::MemoryBlockReadResponse>("texture_fill_response");
        let (texture_cache_request_s, texture_cache_request_r) =
            chan<texture_cache::CacheRequest>("texture_cache_request");
        let (texture_cache_response_s, texture_cache_response_r) =
            chan<texture_cache::CacheResponse>("texture_cache_response");

        let (framebuffer_block_s, framebuffer_block_r) =
            chan<memory::MemoryBlockWrite>("framebuffer_block");
        let (framebuffer_block_response_s, framebuffer_block_response_r) =
            chan<memory::MemoryBlockResponse>("framebuffer_block_response");
        let (texture_block_s, texture_block_r) = chan<memory::MemoryBlockWrite>("texture_block");
        let (texture_block_response_s, texture_block_response_r) =
            chan<memory::MemoryBlockResponse>("texture_block_response");
        let (bank0_command_s, bank0_command_r) =
            chan<texture_cache::PackedBankCommand>("bank0_command");
        let (bank0_response_s, bank0_response_r) = chan<memory::MemoryResponse>("bank0_response");
        let (bank1_command_s, bank1_command_r) =
            chan<texture_cache::PackedBankCommand>("bank1_command");
        let (bank1_response_s, bank1_response_r) = chan<memory::MemoryResponse>("bank1_response");
        let (bank2_command_s, bank2_command_r) =
            chan<texture_cache::PackedBankCommand>("bank2_command");
        let (bank2_response_s, bank2_response_r) = chan<memory::MemoryResponse>("bank2_response");
        let (bank3_command_s, bank3_command_r) =
            chan<texture_cache::PackedBankCommand>("bank3_command");
        let (bank3_response_s, bank3_response_r) = chan<memory::MemoryResponse>("bank3_response");
        spawn banked_memory::banked_word_memory<FB_WORDS>(
            framebuffer_memory_command_r, framebuffer_memory_response_s, framebuffer_block_r,
            framebuffer_block_response_s, framebuffer_fill_r, framebuffer_fill_response_s);
        spawn framebuffer_pair_adapter(
            framebuffer_memory_command_s, framebuffer_memory_response_r, framebuffer_command_r,
            framebuffer_response_s, framebuffer_pixel_read_r, framebuffer_pixel_read_response_s,
            framebuffer_pixel_write_r, framebuffer_pixel_write_response_s, framebuffer_fastfill_r,
            framebuffer_fastfill_response_s);
        spawn banked_memory::banked_word_memory<TEX_WORDS>(
            texture_command_r, texture_response_s, texture_block_r, texture_block_response_s,
            texture_fill_r, texture_fill_response_s);
        spawn texture_cache::packed_bank_memory(bank0_command_r, bank0_response_s);
        spawn texture_cache::packed_bank_memory(bank1_command_r, bank1_response_s);
        spawn texture_cache::packed_bank_memory(bank2_command_r, bank2_response_s);
        spawn texture_cache::packed_bank_memory(bank3_command_r, bank3_response_s);
        spawn texture_cache::texture_cache(
            texture_cache_request_r, texture_cache_response_s, texture_command_s,
            texture_response_r, texture_fill_s, texture_fill_response_r, bank0_command_s,
            bank0_response_r, bank1_command_s, bank1_response_r, bank2_command_s, bank2_response_r,
            bank3_command_s, bank3_response_r);
        spawn core_sim_impl<true, false>(
            command_r, de10_request_r, burst_r, response_s, framebuffer_command_s,
            framebuffer_response_r, framebuffer_pixel_read_s, framebuffer_pixel_read_response_r,
            framebuffer_pixel_write_s, framebuffer_pixel_write_response_r, framebuffer_fastfill_s,
            framebuffer_fastfill_response_r, texture_cache_request_s, texture_cache_response_r,
            texture_block_s, texture_block_response_r);
        ()
    }

    init { () }

    next(state: ()) { state }
}

#[test]
fn scanout_tracks_displayed_buffer_and_stride() {
    let registers = update(
        update(initial_state().registers, u8:133, u32:0x000000a0), u8:134, u32:0x0004b000);
    let state = CoreState { registers, ..initial_state() };
    assert_eq(
        scanout_state(state),
        ScanoutState {
            base: u26:0,
            pixel_stride: u11:640,
            displayed: u1:0,
            swap_count: u2:0,
        });
    let swapped = CoreState {
        commands: control::CommandState {
            swap: control::SwapState { swap_count: u2:1, ..state.commands.swap },
            ..state.commands
        },
        ..state
    };
    assert_eq(scanout_state(swapped).base, u26:0x96000);
    assert_eq(scanout_state(swapped).displayed, u1:1);
}

#[test]
fn de10_mmio_maps_word_addresses_and_byte_enables() {
    let request = De10MmioRequest {
        address: u22:0x200001,
        read: u1:0,
        write: u1:1,
        byte_enable: u4:0xc,
        write_data: u32:0x12345678,
    };
    assert_eq(de10_request_valid(request), u1:1);
    assert_eq(
        de10_core_command(request),
        CoreCommand { write: u1:1, address: u24:0x800004, data: u32:0x12345678, mask: u4:0xc });
    assert_eq(de10_request_valid(De10MmioRequest { read: u1:1, write: u1:1, ..request }), u1:0);
}

#[test]
fn palette_registers_update_even_and_odd_entries() {
    let state = initial_state();
    let (state, _) = step(
        state, CoreCommand { write: u1:1, address: u24:0x34c, data: u32:0x92010203, mask: u4:0xf });
    let (state, _) = step(
        state, CoreCommand { write: u1:1, address: u24:0x350, data: u32:0x92040506, mask: u4:0xf });
    assert_eq(state.palette[u8:0x24], u24:0x010203);
    assert_eq(state.palette[u8:0x25], u24:0x040506);
}

#[test]
fn restored_draw_parity_switches_before_vsync() {
    let state = initial_state();
    let registers = update(state.registers, u8:68, u32:0x00004200);
    let registers = update(registers, u8:134, u32:0x0004b000);
    let state = CoreState { registers, ..state };
    let (restored, _) =
        step(state, CoreCommand { write: u1:1, address: u24:0x128, data: u32:0, mask: u4:0xf });
    let restored = tick(restored, u1:0);
    let candidate = raster_state::PixelCandidate { valid: u1:1, x: s16:0, y: s16:0 };
    assert_eq(restored.commands.swap.draw_count, u2:1);
    assert_eq(raster_write(restored, candidate, u32:0, u24:0).address, u22:0);
    let (swapped, _) =
        step(restored, CoreCommand { write: u1:1, address: u24:0x128, data: u32:1, mask: u4:0xf });
    let waiting = tick(swapped, u1:0);
    assert_eq(waiting.commands.swap.draw_count, u2:2);
    assert_eq(waiting.commands.swap.swap_count, u2:1);
    assert_eq(raster_write(waiting, candidate, u32:0, u24:0).address, u22:0x25800);
    let completed = tick(waiting, u1:1);
    assert_eq(completed.commands.swap.swap_count, u2:2);
    assert_eq(completed.commands.swap.swaps_pending, u3:0);
}

#[test]
fn texture_burst_maps_contiguous_download() {
    let (state, _) = step(
        initial_state(),
        CoreCommand { write: u1:1, address: u24:0x300, data: u32:0x00000a00, mask: u4:0xf });
    let commands = for (i, commands): (u6, CoreCommand[32]) in u6:0..u6:32 {
        update(
            commands, i,
            CoreCommand {
                write: u1:1,
                address: u24:0x800000 + ((i as u24) << u8:2),
                data: i as u32,
                mask: u4:0x5,
            })
    }(CoreCommand[32]:[CoreCommand { write: u1:0, address: u24:0, data: u32:0, mask: u4:0 }, ...]);
    let (block, valid) = texture_block_write(state, TextureBurst { commands });
    assert_eq(valid, u1:1);
    assert_eq(block.address, u22:0);
    assert_eq(block.data[u32:31], u32:31);
    assert_eq(block.masks[u32:0], u4:0x5);
}

#[test]
fn burst_tick_advance_matches_scalar_ticks() {
    let (state, _) = step(
        initial_state(), CoreCommand { write: u1:1, address: u24:0x128, data: u32:0, mask: u4:0xf });
    let expected = for (_, state): (u6, CoreState) in u6:0..u6:31 {
        tick(state, u1:0)
    }(state);
    assert_eq(advance_burst_ticks(state), expected);
}

#[test]
fn narrow_paletted_textures_flush_banked_cache_while_rendering() {
    assert_eq(flush_narrow_texture_cache(u1:1, u32:0x00200000, u32:0x500), u1:1);
    assert_eq(flush_narrow_texture_cache(u1:1, u32:0, u32:0x500), u1:0);
    assert_eq(flush_narrow_texture_cache(u1:1, u32:0x00600000, u32:0x300), u1:0);
    assert_eq(flush_narrow_texture_cache(u1:0, u32:0x00600000, u32:0x500), u1:0);
}

#[test]
fn step_updates_texture_state_and_layout() {
    let state = initial_state();
    let command =
        CoreCommand { write: u1:1, address: u24:0x30c, data: u32:0x00012345, mask: u4:0xf };
    let (state, response) = step(state, command);
    assert_eq(response, CoreResponse { data: u32:0, success: u1:1 });
    assert_eq(state.texture.tex_base, u32:0x00012345);
    assert_eq(state.texture_layout[410+:u22], u22:0x91a28);
}

#[test]
fn step_rejects_unknown_register() {
    let state = initial_state();
    let command =
        CoreCommand { write: u1:1, address: u24:0x400, data: u32:0xffffffff, mask: u4:0xf };
    let (next_state, response) = step(state, command);
    assert_eq(next_state, state);
    assert_eq(response, CoreResponse { data: u32:0, success: u1:0 });
}

#[test]
fn raster_write_interpolates_rgb565() {
    let state = initial_state();
    let registers = update(state.registers, u8:2, u32:8);
    let registers = update(registers, u8:3, u32:8);
    let registers = update(registers, u8:4, u32:56);
    let registers = update(registers, u8:5, u32:8);
    let registers = update(registers, u8:6, u32:8);
    let registers = update(registers, u8:7, u32:56);
    let registers = update(registers, u8:8, u32:0x0ff000);
    let registers = update(registers, u8:68, u32:0x200);
    let state = CoreState { registers, ..state };
    let (state, _) =
        step(state, CoreCommand { write: u1:1, address: u24:0x080, data: u32:0, mask: u4:0xf });
    let (state, candidate) = tick_with_pixel(state, u1:0);
    assert_eq(candidate, raster_state::PixelCandidate { valid: u1:1, x: s16:0, y: s16:0 });
    assert_eq(
        raster_write(state, candidate, u32:0, u24:0),
        memory::MemoryCommand { write: u1:1, address: u22:0, data: u32:0x0000f800, mask: u4:0x3 });
}

#[test]
fn fastfill_command_captures_and_packs_state() {
    let state = initial_state();
    let registers = update(state.registers, u8:68, u32:0x600);
    let registers = update(registers, u8:70, u32:0x00000002);
    let registers = update(registers, u8:71, u32:0x00000001);
    let registers = update(registers, u8:76, u32:0x1234);
    let registers = update(registers, u8:82, u32:0x00ff0000);
    let state = CoreState { registers, ..state };
    let (state, _) =
        step(state, CoreCommand { write: u1:1, address: u24:0x124, data: u32:0, mask: u4:0xf });
    let (state, _, fill) = tick_with_events(state, u1:0);
    assert_eq((fill.valid, fill.x, fill.y), (u1:1, u16:0, u16:0));
    assert_eq(
        fastfill_write(fill.color_base, fill, fill.color),
        memory::MemoryCommand { write: u1:1, address: u22:0, data: u32:0xf800f800, mask: u4:0xf });
    assert_eq(
        fastfill_write(fill.aux_base, fill, fill.depth),
        memory::MemoryCommand { write: u1:1, address: u22:0, data: u32:0x12341234, mask: u4:0xf });
    assert_eq(state.fastfill.active, u1:0);
}

#[test]
fn computes_and_packs_aux_depth() {
    let state = initial_state();
    let registers = update(state.registers, u8:11, u32:0x01234000);
    let registers = update(registers, u8:68, u32:0x00010410);
    let registers = update(registers, u8:76, u32:0x0000ffff);
    let registers = update(registers, u8:134, u32:0x00000800);
    let state = CoreState { registers, ..state };
    let candidate = raster_state::PixelCandidate { valid: u1:1, x: s16:1, y: s16:0 };
    assert_eq(new_depth(state, candidate), u16:0x1233);
    assert_eq(
        aux_write(state, candidate, new_depth(state, candidate), u8:0x80),
        memory::MemoryCommand {
            write: u1:1,
            address: u22:0x800,
            data: u32:0x12330000,
            mask: u4:0xc,
        });
}

#[test]
fn interpolation_honors_disabled_param_adjust() {
    let state = initial_state();
    let registers = update(state.registers, u8:2, u32:9);
    let registers = update(registers, u8:8, u32:100);
    let registers = update(registers, u8:16, u32:16);
    let state = CoreState { registers, ..state };
    let candidate = raster_state::PixelCandidate { valid: u1:1, x: s16:0, y: s16:0 };
    assert_eq(interpolated_parameter(state, candidate, u8:8, u8:16, u8:24), s48:84);
}

#[test]
fn ftriangle_texture_coordinates_round_to_eighteen_fraction_bits() {
    let state = initial_state();
    let registers = update(state.registers, u8:2, u32:1536);
    let registers = update(registers, u8:65, u32:0x04000000);
    let state = CoreState { registers, ..state };
    let (state, _) = step(
        state, CoreCommand { write: u1:1, address: u24:0x0b4, data: u32:0x43460000, mask: u4:0xf });
    let (state, _) = step(
        state, CoreCommand { write: u1:1, address: u24:0x0d4, data: u32:0x3fd80003, mask: u4:0xf });
    let (state, _) =
        step(state, CoreCommand { write: u1:1, address: u24:0x100, data: u32:0, mask: u4:0xf });
    let candidate = raster_state::PixelCandidate { valid: u1:1, x: s16:92, y: s16:0 };
    assert_eq(interpolated_parameter(state, candidate, u8:13, u8:21, u8:29), s48:50356224);
}

#[test]
fn ftriangle_interpolation_uses_float_shadow_precision() {
    let state = initial_state();
    let registers = update(state.registers, u8:65, u32:0x04000000);
    let state = CoreState { registers, ..state };
    let (state, _) = step(
        state, CoreCommand { write: u1:1, address: u24:0x0d4, data: u32:0x3dcccccd, mask: u4:0xf });
    assert_eq(state.registers[u8:21], u32:26214);
    assert_eq(state.float_shadows[u8:21], s50:429496736);
    let (state, _) =
        step(state, CoreCommand { write: u1:1, address: u24:0x100, data: u32:0, mask: u4:0xf });
    let candidate = raster_state::PixelCandidate { valid: u1:1, x: s16:100, y: s16:0 };
    assert_eq(interpolated_parameter(state, candidate, u8:13, u8:21, u8:29), s48:2634547);
    let (state, _) =
        step(state, CoreCommand { write: u1:1, address: u24:0x080, data: u32:0, mask: u4:0xf });
    assert_eq(interpolated_parameter(state, candidate, u8:13, u8:21, u8:29), s48:2634507);
}

#[test]
fn vertex_preserves_twenty_bit_coordinates() {
    let registers = update(RegisterFile:[u32:0, ...], u8:2, u32:0x13880);
    assert_eq(vertex(registers, u8:2), s20:0x13880);
}

#[test_proc]
proc core_sim_proc_test {
    command_s: chan<CoreCommand> out;
    burst_s: chan<TextureBurst> out;
    response_r: chan<CoreResponse> in;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (command_s, command_r) = chan<CoreCommand>("command");
        let (burst_s, burst_r) = chan<TextureBurst>("burst");
        let (response_s, response_r) = chan<CoreResponse>("response");
        spawn core_sim<u32:2048, u32:16>(command_r, burst_r, response_s);
        (command_s, burst_s, response_r, terminator)
    }

    init { () }

    next(state: ()) {
        let write =
            CoreCommand { write: u1:1, address: u24:0x300, data: u32:0x12345678, mask: u4:0x5 };
        let tok = send(join(), command_s, write);
        let (tok, write_response) = recv(tok, response_r);
        assert_eq(write_response.success, u1:1);
        let read = CoreCommand { write: u1:0, address: u24:0x300, data: u32:0, mask: u4:0 };
        let tok = send(tok, command_s, read);
        let (tok, read_response) = recv(tok, response_r);
        assert_eq(read_response, CoreResponse { data: u32:0x00340078, success: u1:1 });
        let framebuffer_write =
            CoreCommand { write: u1:1, address: u24:0x40000c, data: u32:0x89abcdef, mask: u4:0xf };
        let tok = send(tok, command_s, framebuffer_write);
        let (tok, framebuffer_response) = recv(tok, response_r);
        assert_eq(framebuffer_response.data, u32:0x89abcdef);
        let framebuffer_read =
            CoreCommand { write: u1:0, address: u24:0x40000c, data: u32:0, mask: u4:0 };
        let tok = send(tok, command_s, framebuffer_read);
        let (tok, framebuffer_response) = recv(tok, response_r);
        assert_eq(framebuffer_response.data, u32:0x89abcdef);
        let texture_write =
            CoreCommand { write: u1:1, address: u24:0x800008, data: u32:0x01234567, mask: u4:0xf };
        let tok = send(tok, command_s, texture_write);
        let (tok, texture_response) = recv(tok, response_r);
        assert_eq(texture_response.data, u32:0x01234567);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x118, data: u32:2, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x11c, data: u32:1, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x148, data: u32:0xff, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x110, data: u32:0x200, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x124, data: u32:0, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:0, address: u24:0x400000, data: u32:0, mask: u4:0 });
        let (tok, fastfill_response) = recv(tok, response_r);
        assert_eq(fastfill_response.data, u32:0x001f001f);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x008, data: u32:8, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x00c, data: u32:8, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x010, data: u32:56, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x014, data: u32:8, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x018, data: u32:8, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x01c, data: u32:56, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x020, data: u32:0x0ff000, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x110, data: u32:0x200, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x080, data: u32:0, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let read_raster =
            CoreCommand { write: u1:0, address: u24:0x400000, data: u32:0, mask: u4:0 };
        let tok = send(tok, command_s, read_raster);
        let (tok, raster_response) = recv(tok, response_r);
        assert_eq(raster_response.data[0+:u16], u16:0xf800);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x300, data: u32:0x00000a00, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x800000, data: u32:0x001f001f, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x104, data: u32:0x08000001, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:1, address: u24:0x080, data: u32:0, mask: u4:0xf });
        let (tok, _) = recv(tok, response_r);
        let tok = send(
            tok, command_s,
            CoreCommand { write: u1:0, address: u24:0x400000, data: u32:0, mask: u4:0 });
        let (tok, textured_response) = recv(tok, response_r);
        assert_eq(textured_response.data[0+:u16], u16:0x001f);
        send(tok, terminator, true);
    }
}
