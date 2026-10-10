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

struct PixelTextureRegisters {
    texture_mode: u12,
    texture_enabled: u1,
    t_lod: u27,
    tex_base: u24,
    tex_base1: u24,
    tex_base2: u24,
    tex_base38: u24,
}

type RegisterFile = u32[44];
type FloatShadowFile = s50[12];
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

struct PixelCacheResponse { texels: u16[4], palette_entries: u24[4] }

pub struct FramebufferSpan { start_lane: u23, end_lane: u23 }

struct FramebufferSpanConfig {
    clip_enabled: u1,
    depth_read: u1,
    read_enabled: u1,
    back_buffer: u1,
    y_invert: u1,
    color_read: u1,
    clip_left: u10,
    clip_right: u10,
    clip_low_y: u10,
    clip_high_y: u10,
    configured_tiles: u4,
    buffer_offset: u9,
    y_origin: u10,
    drawing: u1,
}

pub struct PaletteReadRequest { index0: u8, index1: u8, index2: u8, index3: u8 }

pub struct PaletteWrite { select: u1, index: u8, data: u24 }

pub struct PaletteResponse { entry0: u24, entry1: u24, entry2: u24, entry3: u24 }

struct FloatShadowRequest { data: u32, fraction_bits: u6, shadow_shift: u6 }

struct FloatShadowResponse { fixed: u32, shadow: s50 }

fn convert_float_register(request: FloatShadowRequest) -> FloatShadowResponse {
    FloatShadowResponse {
        fixed: float_alias::float32_to_fixed32_fast(request.data, request.fraction_bits) as u32,
        shadow: float_alias::float32_to_fixed(
            request.data, request.fraction_bits + request.shadow_shift),
    }
}

#[quickcheck(test_count=1024)]
fn float_register_conversion_matches_reference
    (data: u32, fraction_bits: u6, shadow_shift: u6) -> bool {
    let converted =
        convert_float_register(FloatShadowRequest { data, fraction_bits, shadow_shift });
    converted.fixed == float_alias::float32_to_fixed(data, fraction_bits) as u32 &&
    converted.shadow == float_alias::float32_to_fixed(data, fraction_bits + shadow_shift)
}

struct PixelContext {
    base_lod: s16,
    registers: PixelRegisterFile,
    float_shadows: PixelFloatShadowFile,
    raster_uses_float_shadows: u1,
    texture: PixelTextureRegisters,
    texture_layout: bits[432],
    drawing: u1,
}

struct FramebufferSpanJob { raster: raster_state::RasterState, config: FramebufferSpanConfig }

struct HostedControllerState {
    core: CoreState,
    float_pending: bool,
    float_command: CoreCommand,
    base_lod: s16,
    pixels_pending: u6,
    triangle_pending: u1,
    triangle_install: u1,
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
    texture_derivatives: s32[4],
}

struct TriangleSetupResult { bounds: uN[80], edges: uN[360], starts: uN[120], base_lod: s16 }

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
    red: u8,
    green: u8,
    blue: u8,
    alpha: u8,
    z: s48,
}

struct ReadPixelContext {
    base_lod: s16,
    path: u28,
    alpha_mode: u32,
    fbz_mode: u21,
    clip_x_min: u10,
    clip_x_max: u10,
    clip_y_min: u10,
    clip_y_max: u10,
    za_color: u16,
    chroma_key: u24,
    color0: u32,
    color1: u32,
    framebuffer_tiles: u4,
    framebuffer_offset: u9,
    y_origin: u10,
    texture: PixelTextureRegisters,
    texture_layout: bits[432],
    drawing: u1,
    candidate: raster_state::PixelCandidate,
    last: u1,
}

struct InterpolatedPixelJob { context: ReadPixelContext, values: PixelInterpolatedValues }

struct TexturePixelContext {
    path: u28,
    alpha_mode: u32,
    fbz_mode: u21,
    za_color: u16,
    chroma_key: u24,
    color0: u32,
    color1: u32,
    framebuffer_tiles: u4,
    framebuffer_offset: u9,
    y_origin: u10,
    drawing: u1,
    candidate: raster_state::PixelCandidate,
    last: u1,
}

struct TexturePixelValues { red: u8, green: u8, blue: u8, alpha: u8, z: u8, z_depth: u16, w: s32 }

struct TexturePixelJob {
    context: TexturePixelContext,
    values: TexturePixelValues,
    sample: texture_sampler::SampleRequest,
    raster_valid: u1,
    textured: u1,
    cache_access: u1,
}

struct ReadPixelJob {
    candidate: raster_state::PixelCandidate,
    source_color: u32,
    depth: u16,
    aux: memory::MemoryCommand,
    color_high: u1,
    framebuffer_tiles: u4,
    framebuffer_offset: u9,
    fbz_mode: u20,
    alpha_mode: u16,
    drawing: u1,
    y_origin: u10,
    raster_valid: u1,
    tests_pass: u1,
    read_access: u1,
    depth_access: u1,
    blend_access: u1,
    textured: u1,
    last: u1,
}

pub struct PerformanceEvent {
    input_pixel: u1,
    output_pixel: u1,
    triangle_done: u1,
    pipeline_job: u1,
    textured: u1,
    depth_read: u1,
    blend_read: u1,
    color_write: u1,
    aux_write: u1,
}

struct PendingPixelResult { event: PerformanceEvent, write_access: u1 }

type PixelResult = PerformanceEvent;

fn empty_cache_response() -> texture_cache::CacheResponse {
    texture_cache::CacheResponse {
        memory_response: memory::MemoryResponse { data: u32:0, success: u1:1 },
        texels: u16[4]:[u16:0, ...],
        hits: u1[4]:[u1:0, ...],
        total_hits: u24:0,
        total_misses: u24:0,
        all_hit_requests: u24:0,
        missed_requests: u24:0,
    }
}

fn pixel_cache_response
    (cache: texture_cache::CacheResponse, palette: Palette) -> PixelCacheResponse {
    PixelCacheResponse {
        texels: cache.texels,
        palette_entries: u24[4]:[
            palette[cache.texels[0][0+:u8]], palette[cache.texels[1][0+:u8]],
            palette[cache.texels[2][0+:u8]], palette[cache.texels[3][0+:u8]],
        ],
    }
}

fn palette_read_request(texels: u16[4]) -> PaletteReadRequest {
    PaletteReadRequest {
        index0: texels[0][0+:u8],
        index1: texels[1][0+:u8],
        index2: texels[2][0+:u8],
        index3: texels[3][0+:u8],
    }
}

fn external_pixel_cache_response(texels: u16[4], palette: PaletteResponse) -> PixelCacheResponse {
    PixelCacheResponse {
        texels,
        palette_entries: u24[4]:[palette.entry0, palette.entry1, palette.entry2, palette.entry3],
    }
}

fn empty_palette_response() -> PaletteResponse {
    PaletteResponse { entry0: u24:0, entry1: u24:0, entry2: u24:0, entry3: u24:0 }
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

fn register_index(index: u8) -> u8 {
    if index >= u8:2 && index <= u8:31 {
        index - u8:2
    } else {
        match index {
            u8:65 => u8:30,
            u8:67 => u8:31,
            u8:68 => u8:32,
            u8:70 => u8:33,
            u8:71 => u8:34,
            u8:76 => u8:35,
            u8:77 => u8:36,
            u8:81 => u8:37,
            u8:82 => u8:38,
            u8:133 => u8:39,
            u8:134 => u8:40,
            u8:135 => u8:41,
            u8:69 => u8:43,
            _ => u8:42,
        }
    }
}

fn register_read(registers: RegisterFile, index: u8) -> u32 { registers[register_index(index)] }

fn pixel_registers(registers: RegisterFile) -> PixelRegisterFile {
    PixelRegisterFile:[
        register_read(registers, u8:2), register_read(registers, u8:3),
        register_read(registers, u8:8), register_read(registers, u8:9),
        register_read(registers, u8:10), register_read(registers, u8:11),
        register_read(registers, u8:12), register_read(registers, u8:13),
        register_read(registers, u8:14), register_read(registers, u8:15),
        register_read(registers, u8:16), register_read(registers, u8:17),
        register_read(registers, u8:18), register_read(registers, u8:19),
        register_read(registers, u8:20), register_read(registers, u8:21),
        register_read(registers, u8:22), register_read(registers, u8:23),
        register_read(registers, u8:24), register_read(registers, u8:25),
        register_read(registers, u8:26), register_read(registers, u8:27),
        register_read(registers, u8:28), register_read(registers, u8:29),
        register_read(registers, u8:30), register_read(registers, u8:31),
        register_read(registers, u8:65), register_read(registers, u8:67),
        register_read(registers, u8:68), register_read(registers, u8:70),
        register_read(registers, u8:71), register_read(registers, u8:76),
        register_read(registers, u8:77), register_read(registers, u8:81),
        register_read(registers, u8:82), register_read(registers, u8:133),
        register_read(registers, u8:134), register_read(registers, u8:135),
    ]
}

fn pixel_float_shadows(float_shadows: FloatShadowFile) -> PixelFloatShadowFile { float_shadows }

fn float_shadow_index(index: u8) -> u8 {
    if index >= u8:12 && index <= u8:15 {
        index - u8:12
    } else if index >= u8:20 && index <= u8:23 {
        index - u8:16
    } else {
        index - u8:20
    }
}

fn pixel_register(registers: PixelRegisterFile, index: u8) -> u32 {
    if index >= u8:2 && index <= u8:3 {
        registers[index - u8:2]
    } else if index >= u8:8 && index <= u8:31 {
        registers[index - u8:6]
    } else {
        match index {
            u8:65 => registers[u32:26],
            u8:67 => registers[u32:27],
            u8:68 => registers[u32:28],
            u8:70 => registers[u32:29],
            u8:71 => registers[u32:30],
            u8:76 => registers[u32:31],
            u8:77 => registers[u32:32],
            u8:81 => registers[u32:33],
            u8:82 => registers[u32:34],
            u8:133 => registers[u32:35],
            u8:134 => registers[u32:36],
            u8:135 => registers[u32:37],
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

fn pixel_texture_registers(texture: TextureRegisters) -> PixelTextureRegisters {
    PixelTextureRegisters {
        texture_mode: texture.texture_mode[0+:u12],
        texture_enabled: (texture.texture_mode != u32:0) as u1,
        t_lod: texture.t_lod[0+:u27],
        tex_base: texture.tex_base[0+:u24],
        tex_base1: texture.tex_base1[0+:u24],
        tex_base2: texture.tex_base2[0+:u24],
        tex_base38: texture.tex_base38[0+:u24],
    }
}

fn pixel_texture_mode(texture: PixelTextureRegisters) -> u32 {
    (texture.texture_mode as u32) | ((texture.texture_enabled as u32) << u32:31)
}

fn pixel_context(core: CoreState, base_lod: s16) -> PixelContext {
    PixelContext {
        base_lod,
        registers: pixel_registers(core.registers),
        float_shadows: pixel_float_shadows(core.float_shadows),
        raster_uses_float_shadows: core.raster_uses_float_shadows,
        texture: pixel_texture_registers(core.texture),
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

fn write_register_file
    (registers: RegisterFile, compact_index: u8, data: u32, mask: u4) -> RegisterFile {
    for (i, result): (u32, RegisterFile) in u32:0..array_size(registers) {
        let value = if compact_index as u32 == i {
            masked_write(registers[i], data, mask)
        } else {
            registers[i]
        };
        update(result, i, value)
    }(registers)
}

fn write_addressed_register_file
    (registers: RegisterFile, index: u8, data: u32, mask: u4) -> RegisterFile {
    let high_indexes = u8[12]:[
        u8:65, u8:67, u8:68, u8:70, u8:71, u8:76, u8:77, u8:81, u8:82, u8:133, u8:134, u8:135,
    ];
    for (i, result): (u32, RegisterFile) in u32:0..array_size(registers) {
        let selected = if i < u32:30 {
            index == (i + u32:2) as u8
        } else if i < u32:42 {
            index == high_indexes[i - u32:30]
        } else if i == u32:42 {
            register_index(index) == u8:42
        } else {
            index == u8:69
        };
        update(
            result, i, if selected { masked_write(registers[i], data, mask) } else { registers[i] })
    }(registers)
}

#[test]
fn lfb_mode_survives_other_register_writes() {
    let registers =
        write_addressed_register_file(RegisterFile:[u32:0, ...], u8:69, u32:0x10, u4:15);
    let registers = write_addressed_register_file(registers, u8:72, u32:0, u4:15);
    let registers = write_addressed_register_file(registers, u8:192, u32:0, u4:15);
    assert_eq(register_read(registers, u8:69), u32:0x10);
}

#[quickcheck(test_count=1024)]
fn addressed_register_write_matches_compact
    (registers: RegisterFile, index: u8, data: u32, mask: u4) -> bool {
    write_addressed_register_file(registers, index, data, mask) ==
    write_register_file(registers, register_index(index), data, mask)
}

#[quickcheck(test_count=1024)]
fn register_write_matches_indexed_update
    (registers: RegisterFile, index: u8, data: u32, mask: u4) -> bool {
    let expected = if (index as u32) < array_size(registers) {
        update(registers, index, masked_write(registers[index], data, mask))
    } else {
        registers
    };
    write_register_file(registers, index, data, mask) == expected
}

#[test]
fn register_write_preserves_other_words_and_masked_bytes() {
    let registers =
        for (i, registers): (u32, RegisterFile) in u32:0..array_size(RegisterFile:[u32:0, ...]) {
            update(registers, i, u32:0x89abcdef ^ i)
        }(RegisterFile:[u32:0, ...]);
    for (index, _): (u8, ()) in u8:0..(array_size(registers) + u32:1) as u8 {
        for (mask, _): (u5, ()) in u5:0..u5:16 {
            let actual = write_register_file(registers, index, u32:0x12345678, mask as u4);
            let expected = if (index as u32) < array_size(registers) {
                update(
                    registers, index, masked_write(registers[index], u32:0x12345678, mask as u4))
            } else {
                registers
            };
            assert_eq(actual, expected)
        }(())
    }(());
    assert_eq(write_register_file(registers, u8:255, u32:0, u4:15), registers);
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

fn palette_write_request(address: u10, data: u32) -> (u1, PaletteWrite) {
    let hit = address == u10:0x334 || address == u10:0x338 || address == u10:0x33c ||
              address == u10:0x340 || address == u10:0x344 || address == u10:0x348 ||
              address == u10:0x34c || address == u10:0x350;
    let odd = address == u10:0x338 || address == u10:0x340 || address == u10:0x348 ||
              address == u10:0x350;
    let index = (data[23+:u8] & u8:0xfe) | (odd as u8);
    (hit && data[31+:u1] == u1:1, PaletteWrite { select: u1:0, index, data: data[0+:u24] })
}

fn palette_bank_select(command: CoreCommand) -> bool {
    command.address == u24:0x3ec && command.write == u1:1 &&
    command.mask == u4:15 && command.data < u32:16
}

#[test]
fn palette_bank_select_requires_exact_address_full_mask_and_valid_bank() {
    let command = CoreCommand { address: u24:0x3ec, write: u1:1, mask: u4:15, data: u32:15 };
    assert_eq(palette_bank_select(command), true);
    assert_eq(palette_bank_select(CoreCommand { data: u32:0, ..command }), true);
    assert_eq(palette_bank_select(CoreCommand { data: u32:16, ..command }), false);
    assert_eq(palette_bank_select(CoreCommand { data: u32:31, ..command }), false);
    assert_eq(palette_bank_select(CoreCommand { data: u32:32, ..command }), false);
    assert_eq(palette_bank_select(CoreCommand { data: u32:0xffffffff, ..command }), false);
    assert_eq(palette_bank_select(CoreCommand { mask: u4:7, ..command }), false);
    assert_eq(palette_bank_select(CoreCommand { write: u1:0, ..command }), false);
    assert_eq(palette_bank_select(CoreCommand { address: u24:0x7ec, ..command }), false);
}

fn write_palette(palette: Palette, address: u10, data: u32) -> Palette {
    let (valid, request) = palette_write_request(address, data);
    if valid { update(palette, request.index, request.data) } else { palette }
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
    let configured_tiles = register_read(state.registers, u8:133)[4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let buffer_offset = (register_read(state.registers, u8:134)[11+:u9] as u26) << u26:12;
    let displayed = state.commands.swap.swap_count[0+:u1];
    ScanoutState {
        base: if displayed == u1:1 { buffer_offset } else { u26:0 },
        pixel_stride: (tiles as u11) << u11:6,
        displayed,
        swap_count: state.commands.swap.swap_count,
    }
}

fn vertex(registers: RegisterFile, index: u8) -> s20 {
    (register_read(registers, index) as u20) as s20
}

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

fn tick_events<SKIP_INVISIBLE: bool = {true}, PREFETCHED_SPANS: bool = {false}>
    (commands: control::CommandState, raster: raster_state::RasterState,
     fastfill: fastfill_state::FastfillState, v_retrace: u1)
    -> (
        control::CommandState, raster_state::RasterState, fastfill_state::FastfillState,
        raster_state::PixelCandidate, fastfill_state::FastfillWord
    ) {
    let (next_fastfill, fill_pixel) = fastfill_state::tick(fastfill);
    let (next_raster, candidate) = if fastfill.active == u1:1 {
        (raster, raster_state::PixelCandidate { valid: u1:0, x: s16:0, y: s16:0 })
    } else if PREFETCHED_SPANS {
        raster_state::tick_prefetched(raster)
    } else if SKIP_INVISIBLE {
        raster_state::tick_bounded(raster)
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
    let fbz_mode = register_read(state.registers, u8:68);
    let clip_x = register_read(state.registers, u8:70);
    let clip_y = register_read(state.registers, u8:71);
    let color = register_read(state.registers, u8:82);
    let init1 = register_read(state.registers, u8:133);
    let init2 = register_read(state.registers, u8:134);
    let configured_tiles = init1[4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let buffer_offset = (init2[11+:u9] as u26) << u26:12;
    let drawing = state.commands.swap.draw_count[0+:u1];
    let front = if drawing == u1:1 { buffer_offset } else { u26:0 };
    let back = if drawing == u1:1 { u26:0 } else { buffer_offset };
    let color_base = if fbz_mode[14+:u1] == u1:1 { back } else { front };
    fastfill_state::begin(
        clip_x[16+:u10], clip_x[0+:u10], clip_y[16+:u10], clip_y[0+:u10], color[16+:u8],
        color[8+:u8], color[0+:u8], register_read(state.registers, u8:76)[0+:u16], color_base,
        buffer_offset << u26:1, (tiles as u11) << u11:6, fbz_mode[9+:u1], fbz_mode[10+:u1],
        fbz_mode[17+:u1], register_read(state.registers, u8:135)[22+:u10], fbz_mode[8+:u1],
        fbz_mode[11+:u1])
}

fn is_triangle_command(command: CoreCommand) -> bool {
    command.write == u1:1 && (command.address == u24:0x080 || command.address == u24:0x100)
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
        texture_derivatives: s32[4]:[
            register_read(state.registers, u8:21) as s32,
            register_read(state.registers, u8:22) as s32,
            register_read(state.registers, u8:29) as s32,
            register_read(state.registers, u8:30) as s32,
        ],
    }
}

fn step_with_raster_setup
    (state: CoreState, command: CoreCommand, hosted_triangle: u1, converted_shadow: s50,
     converted_fixed: u32) -> (CoreState, CoreResponse) {

    let is_register = (command.address < u24:0x400 ||
                      (command.address >= u24:0x800 && command.address < u24:0xc00)) &&
                      command.address[0+:u2] == u2:0;
    let address = command.address[0+:u10];
    let (write_address, _, is_float) = float_alias::alias(address);
    let write_data = if is_float == u1:1 { converted_fixed } else { command.data };
    let write_mask = if is_float == u1:1 { u4:0xf } else { command.mask };
    let index = write_address[2+:u8];
    if !is_register {
        (state, CoreResponse { data: u32:0, success: u1:0 })
    } else if command.write == u1:1 {
        let compact_index = register_index(index);
        let value = masked_write(state.registers[compact_index], write_data, write_mask);
        let registers =
            write_addressed_register_file(state.registers, index, write_data, write_mask);
        let (shadow_shift, has_float_shadow) = float_shadow_shift(index);
        let shadow_value = if is_float == u1:1 {
            converted_shadow
        } else {
            ((value as s32) as s50) << shadow_shift
        };
        let float_shadows = if has_float_shadow == u1:1 {
            update(state.float_shadows, float_shadow_index(index), shadow_value)
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
        let data =
            if address == u10:0 { status(state) } else { register_read(state.registers, index) };
        (state, CoreResponse { data, success: u1:1 })
    }
}

pub fn step(state: CoreState, command: CoreCommand) -> (CoreState, CoreResponse) {
    let (write_address, fraction_bits, is_float) = float_alias::alias(command.address[0+:u10]);
    let (shadow_shift, _) = float_shadow_shift(write_address[2+:u8]);
    let converted_shadow = if is_float == u1:1 {
        float_alias::float32_to_fixed(command.data, fraction_bits + shadow_shift)
    } else {
        s50:0
    };
    step_with_raster_setup(
        state, command, u1:0, converted_shadow,
        float_alias::float32_to_fixed32_fast(command.data, fraction_bits) as u32)
}

#[test]
fn float_parameters_are_idempotent_and_preserved_by_triangle_commands() {
    let state = for (i, state): (u32, CoreState) in u32:0..u32:30 {
        let command = CoreCommand {
            write: u1:1, address: (u32:0x088 + i * u32:4) as u24,
            data: u32:0x3f800000 + (i << u32:17), mask: u4:0xf,
        };
        let (written, _) = step(state, command);
        let (repeated, _) = step(written, command);
        assert_eq(repeated, written);
        written
    }(initial_state());
    let (fixed, _) = step(
        state, CoreCommand { write: u1:1, address: u24:0x080, data: u32:0x80000000, mask: u4:0xf });
    let (floating, _) = step(
        fixed, CoreCommand { write: u1:1, address: u24:0x100, data: u32:0x80000000, mask: u4:0xf });
    assert_eq(state.float_shadows, fixed.float_shadows);
    assert_eq(state.float_shadows, floating.float_shadows);
    for (i, _): (u32, ()) in u32:0..u32:30 {
        assert_eq(state.registers[i], fixed.registers[i]);
        assert_eq(state.registers[i], floating.registers[i]);
    }(())
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
    let use_float_shadow = raster_uses_float_shadows == u1:1 && has_float_shadow == u1:1;
    let start = if use_float_shadow {
        pixel_float_shadow(float_shadows, start_index)
    } else {
        (pixel_register(registers, start_index) as s32) as s50
    };
    let dx = if use_float_shadow {
        pixel_float_shadow(float_shadows, dx_index)
    } else {
        (pixel_register(registers, dx_index) as s32) as s50
    };
    let dy = if use_float_shadow {
        pixel_float_shadow(float_shadows, dy_index)
    } else {
        (pixel_register(registers, dy_index) as s32) as s50
    };
    let high = interpolator::at_pixel_high(
        start, dx, dy, pixel_vertex(registers, u8:2), pixel_vertex(registers, u8:3), candidate.x,
        candidate.y, pixel_register(registers, u8:65)[26+:u1]);
    if use_float_shadow { round_float_shadow(start_index, high, shadow_shift) } else { high as s48 }
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
     raster_uses_float_shadows: u1, candidate: raster_state::PixelCandidate, start_index: u8,
     dx_index: u8, dy_index: u8) -> u8 {
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

fn texture_request_interpolated_fields
    (d_s_dx: s32, d_t_dx: s32, d_s_dy: s32, d_t_dy: s32, texture: PixelTextureRegisters,
     texture_layout: bits[432], s: s48, t: s48, w: s48) -> texture_sampler::SampleRequest {
    texture_sampler::sample_request(
        s, t, w, d_s_dx, d_t_dx, d_s_dy, d_t_dy, pixel_texture_mode(texture), texture.t_lod as u32,
        texture_layout, texture.tex_base, texture.tex_base1, texture.tex_base2, texture.tex_base38)
}

fn texture_request_interpolated
    (registers: PixelRegisterFile, texture: PixelTextureRegisters, texture_layout: bits[432],
     s: s48, t: s48, w: s48) -> texture_sampler::SampleRequest {
    texture_request_interpolated_fields(
        pixel_register(registers, u8:21) as s32, pixel_register(registers, u8:22) as s32,
        pixel_register(registers, u8:29) as s32, pixel_register(registers, u8:30) as s32, texture,
        texture_layout, s, t, w)
}

fn texture_request_values
    (registers: PixelRegisterFile, float_shadows: PixelFloatShadowFile,
     raster_uses_float_shadows: u1, texture: TextureRegisters, texture_layout: bits[432],
     candidate: raster_state::PixelCandidate) -> texture_sampler::SampleRequest {
    texture_request_interpolated_fields(
        pixel_register(registers, u8:21) as s32, pixel_register(registers, u8:22) as s32,
        pixel_register(registers, u8:29) as s32, pixel_register(registers, u8:30) as s32,
        pixel_texture_registers(texture), texture_layout,
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

fn candidate_visible_fields
    (mode: u21, clip_x_min: u10, clip_x_max: u10, clip_y_min: u10, clip_y_max: u10,
     candidate: raster_state::PixelCandidate) -> bool {
    let inside_clip = candidate.x >= (clip_x_min as s16) && candidate.x < (clip_x_max as s16) &&
                      candidate.y >= (clip_y_min as s16) && candidate.y < (clip_y_max as s16);
    candidate.valid == u1:1 && candidate.x >= s16:0 && candidate.y >= s16:0 &&
    (mode[9+:u1] == u1:1 || mode[10+:u1] == u1:1) && (mode[0+:u1] == u1:0 || inside_clip)
}

fn candidate_visible
    (mode: u32, clip_x: u32, clip_y: u32, candidate: raster_state::PixelCandidate) -> bool {
    candidate_visible_fields(
        mode[0+:u21], clip_x[16+:u10], clip_x[0+:u10], clip_y[16+:u10], clip_y[0+:u10], candidate)
}

fn raster_visible(state: CoreState, candidate: raster_state::PixelCandidate) -> bool {
    candidate_visible(
        register_read(state.registers, u8:68), register_read(state.registers, u8:70),
        register_read(state.registers, u8:71), candidate)
}

fn combined_color_fields
    (path: u32, color0: u32, color1: u32, texture_rgba: u32, red: u8, green: u8, blue: u8,
     alpha: u8, z: u8) -> u32 {
    pixel::color_combine(
        red ++ green ++ blue, alpha, z, texture_rgba[8+:u24], texture_rgba[0+:u8], color0[0+:u24],
        color0[24+:u8], color1[0+:u24], color1[24+:u8], path[0+:u2], path[4+:u1], path[7+:u1],
        path[8+:u1], path[9+:u1], path[10+:u3], path[13+:u1], path[14+:u2], path[16+:u1],
        path[2+:u2], path[5+:u2], path[17+:u1], path[18+:u1], path[19+:u3], path[22+:u1],
        path[23+:u2], path[25+:u1])
}

fn combined_color_interpolated_fields
    (path: u32, color0: u32, color1: u32, texture_rgba: u32, red: s48, green: s48, blue: s48,
     alpha: s48, z: s48) -> u32 {
    combined_color_fields(
        path, color0, color1, texture_rgba, color_channel(red), color_channel(green),
        color_channel(blue), color_channel(alpha), color_channel(z))
}

fn combined_color_interpolated
    (registers: PixelRegisterFile, texture_rgba: u32, red: s48, green: s48, blue: s48, alpha: s48,
     z: s48) -> u32 {
    combined_color_interpolated_fields(
        pixel_register(registers, u8:65), pixel_register(registers, u8:81),
        pixel_register(registers, u8:82), texture_rgba, red, green, blue, alpha, z)
}

fn combined_color_values
    (registers: PixelRegisterFile, float_shadows: PixelFloatShadowFile,
     raster_uses_float_shadows: u1, candidate: raster_state::PixelCandidate, texture_rgba: u32)
    -> u32 {
    combined_color_interpolated_fields(
        pixel_register(registers, u8:65), pixel_register(registers, u8:81),
        pixel_register(registers, u8:82), texture_rgba,
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
    framebuffer_y_values(
        register_read(state.registers, u8:68), register_read(state.registers, u8:135)[22+:u10],
        candidate)
}

fn lfb_command(state: CoreState, command: CoreCommand) -> memory::MemoryCommand {
    let init1 = register_read(state.registers, u8:133);
    let init2 = register_read(state.registers, u8:134);
    let lfb_mode = register_read(state.registers, u8:69);
    let configured_tiles = init1[4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let stride = (tiles as u11) << u11:6;
    let buffer_offset = (init2[11+:u9] as u26) << u26:12;
    let drawing = state.commands.swap.draw_count[0+:u1];
    let front = if drawing == u1:1 { buffer_offset } else { u26:0 };
    let back = if drawing == u1:1 { u26:0 } else { buffer_offset };
    let buffer_select = if command.write == u1:0 {
        lfb_mode[6+:u2]
    } else if lfb_mode[0+:u4] == u4:15 {
        u2:2
    } else {
        lfb_mode[4+:u2]
    };
    let base = match buffer_select {
        u2:0 => front,
        u2:1 => back,
        u2:2 => buffer_offset << u26:1,
        _ => u26:0,
    };
    let pci_address = command.address - u24:0x400000;
    let x = pci_address[1+:u10] as u16;
    let y = pci_address[11+:u10];
    let y = if lfb_mode[13+:u1] == u1:1 {
        register_read(state.registers, u8:135)[22+:u10] - y
    } else {
        y
    };
    let address = pixel::framebuffer_address(base, x, y as u16, stride);
    memory::MemoryCommand {
        write: command.write,
        address: address[2+:u22],
        data: command.data,
        mask: command.mask,
    }
}

#[test]
fn lfb_aux_and_lower_origin_addressing() {
    let registers =
        write_addressed_register_file(RegisterFile:[u32:0, ...], u8:134, u32:150 << u32:11, u4:15);
    let registers = write_addressed_register_file(registers, u8:135, u32:479 << u32:22, u4:15);
    for (origin, _): (u2, ()) in u2:0..u2:2 {
        let registers = write_addressed_register_file(
            registers, u8:69, u32:0x8f | ((origin as u32) << u32:13), u4:15);
        let state = CoreState { registers, ..initial_state() };
        for (write, _): (u2, ()) in u2:0..u2:2 {
            let command = CoreCommand {
                write: write as u1,
                address: u24:0x400828,
                data: u32:0x12345678,
                mask: u4:3,
            };
            let result = lfb_command(state, command);
            let row = if origin == u2:0 { u32:1 } else { u32:478 };
            assert_eq(result.address as u32, (u32:1228800 + row * u32:1280 + u32:40) >> u32:2);
            assert_eq(result.data, command.data);
            assert_eq(result.mask, command.mask);
        }(())
    }(())
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

fn new_depth_fields(mode: u32, za_color: u32, z_depth: u16, w: s32) -> u16 {
    let w_depth = pixel::fog_w_depth(w);
    let base = if mode[3+:u1] == u1:1 { w_depth } else { z_depth };
    let selected = if mode[20+:u1] == u1:1 { za_color[0+:u16] } else { base };
    let biased = clamp_depth((selected as s48) + ((za_color[0+:u16] as s16) as s48));
    if mode[16+:u1] == u1:1 { biased } else { selected }
}

fn new_depth_interpolated_fields(mode: u32, za_color: u32, z: s48, w: s48) -> u16 {
    new_depth_fields(mode, za_color, clamp_depth(z >> u8:12), w as s32)
}

fn new_depth_interpolated(registers: PixelRegisterFile, z: s48, w: s48) -> u16 {
    new_depth_interpolated_fields(
        pixel_register(registers, u8:68), pixel_register(registers, u8:76), z, w)
}

fn new_depth_values
    (registers: PixelRegisterFile, float_shadows: PixelFloatShadowFile,
     raster_uses_float_shadows: u1, candidate: raster_state::PixelCandidate) -> u16 {
    new_depth_interpolated_fields(
        pixel_register(registers, u8:68), pixel_register(registers, u8:76),
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
        register_read(state.registers, u8:133), register_read(state.registers, u8:134),
        register_read(state.registers, u8:68), register_read(state.registers, u8:135)[22+:u10],
        candidate, depth, alpha)
}

fn raster_write_location
    (init1: u32, init2: u32, fbz_mode: u32, drawing: u1, y_origin: u10,
     candidate: raster_state::PixelCandidate) -> (u22, u1) {
    let configured_tiles = init1[4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let stride = (tiles as u11) << u11:6;
    let buffer_offset = (init2[11+:u9] as u26) << u26:12;
    let front = if drawing == u1:1 { buffer_offset } else { u26:0 };
    let back = if drawing == u1:1 { u26:0 } else { buffer_offset };
    let base = if fbz_mode[14+:u1] == u1:1 { back } else { front };
    let address = pixel::framebuffer_address(
        base, candidate.x as u16, framebuffer_y_values(fbz_mode, y_origin, candidate), stride);
    (address[2+:u22], address[1+:u1])
}

fn framebuffer_span_config(core: CoreState) -> FramebufferSpanConfig {
    let fbz_mode = register_read(core.registers, u8:68);
    let alpha_mode = register_read(core.registers, u8:67);
    let clip_x = register_read(core.registers, u8:70);
    let clip_y = register_read(core.registers, u8:71);
    FramebufferSpanConfig {
        clip_enabled: fbz_mode[0+:u1],
        depth_read: fbz_mode[4+:u1],
        read_enabled: fbz_mode[9+:u1] || fbz_mode[10+:u1],
        back_buffer: fbz_mode[14+:u1],
        y_invert: fbz_mode[17+:u1],
        color_read: alpha_mode[4+:u1],
        clip_left: clip_x[16+:u10],
        clip_right: clip_x[0+:u10],
        clip_low_y: clip_y[16+:u10],
        clip_high_y: clip_y[0+:u10],
        configured_tiles: register_read(core.registers, u8:133)[4+:u4],
        buffer_offset: register_read(core.registers, u8:134)[11+:u9],
        y_origin: register_read(core.registers, u8:135)[22+:u10],
        drawing: core.commands.swap.draw_count[0+:u1],
    }
}

fn framebuffer_read_spans
    (config: FramebufferSpanConfig, row: raster_state::RowSpan)
    -> (FramebufferSpan, u1, FramebufferSpan, u1) {
    let clip_enabled = config.clip_enabled == u1:1;
    let clip_left = if clip_enabled { config.clip_left as s16 } else { s16:0 };
    let clip_right = if clip_enabled { config.clip_right as s16 } else { s16:32767 };
    let clip_low_y = if clip_enabled { config.clip_low_y as s16 } else { s16:0 };
    let clip_high_y = if clip_enabled { config.clip_high_y as s16 } else { s16:32767 };
    let nonnegative_start = if row.x_start < s16:0 { s16:0 } else { row.x_start };
    let start_x = if nonnegative_start < clip_left { clip_left } else { nonnegative_start };
    let clipped_end = if row.x_end >= clip_right { clip_right - s16:1 } else { row.x_end };
    let visible = row.valid == u1:1 && start_x <= clipped_end && row.y >= clip_low_y &&
                  row.y < clip_high_y && row.y >= s16:0 && config.read_enabled == u1:1;
    let start = raster_state::PixelCandidate { valid: u1:1, x: start_x, y: row.y };
    let end = raster_state::PixelCandidate { valid: u1:1, x: clipped_end, y: row.y };
    let init1 = (config.configured_tiles as u32) << u32:4;
    let init2 = (config.buffer_offset as u32) << u32:11;
    let fbz_mode = (config.back_buffer as u32) << u32:14 | (config.y_invert as u32) << u32:17;
    let y_origin = config.y_origin;
    let drawing = config.drawing;
    let start_aux = aux_write_values(init1, init2, fbz_mode, y_origin, start, u16:0, u8:0);
    let end_aux = aux_write_values(init1, init2, fbz_mode, y_origin, end, u16:0, u8:0);
    let (start_color_address, start_color_high) =
        raster_write_location(init1, init2, fbz_mode, drawing, y_origin, start);
    let (end_color_address, end_color_high) =
        raster_write_location(init1, init2, fbz_mode, drawing, y_origin, end);
    let aux = FramebufferSpan {
        start_lane: start_aux.address ++ (start_aux.mask == u4:0xc) as u1,
        end_lane: end_aux.address ++ (end_aux.mask == u4:0xc) as u1,
    };
    let color = FramebufferSpan {
        start_lane: start_color_address ++ start_color_high,
        end_lane: end_color_address ++ end_color_high,
    };
    let aux_valid = visible && config.depth_read == u1:1;
    let color_valid = visible && config.color_read == u1:1;
    (aux, aux_valid as u1, color, color_valid as u1)
}

proc framebuffer_span_prefetch {
    job_r: chan<FramebufferSpanJob> in;
    aux_span_s: chan<FramebufferSpan> out;
    color_span_s: chan<FramebufferSpan> out;
    row_ready_s: chan<raster_state::RowSpan> out;

    config(job_r: chan<FramebufferSpanJob> in, aux_span_s: chan<FramebufferSpan> out,
           color_span_s: chan<FramebufferSpan> out, row_ready_s: chan<raster_state::RowSpan> out) {
        (job_r, aux_span_s, color_span_s, row_ready_s)
    }

    init { (raster_state::initial_state(), framebuffer_span_config(initial_state()), u3:0) }

    next(state: (raster_state::RasterState, FramebufferSpanConfig, u3)) {
        let (raster, config, phase) = state;
        if raster.active == u1:0 {
            let (_, job) = recv(join(), job_r);
            (job.raster, job.config, u3:0)
        } else {
            let (raster, phase, row_span) = raster_state::scan_row_coherent(raster, phase);
            let (aux_span, aux_valid, color_span, color_valid) =
                framebuffer_read_spans(config, row_span);
            let span_complete = row_span.complete == u1:1;
            let tok = send_if(join(), aux_span_s, span_complete && aux_valid == u1:1, aux_span);
            let tok = send_if(tok, color_span_s, span_complete && color_valid == u1:1, color_span);
            let tok = send_if(tok, row_ready_s, span_complete, row_span);
            (raster, config, phase)
        }
    }
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
        register_read(state.registers, u8:133), register_read(state.registers, u8:134),
        register_read(state.registers, u8:68), register_read(state.registers, u8:67),
        state.commands.swap.draw_count[0+:u1], register_read(state.registers, u8:135)[22+:u10],
        candidate, combined_color(state, candidate, texture_rgba), dest_rgb)
}

fn fastfill_write
    (base: u26, fill: fastfill_state::FastfillWord, data: u32) -> memory::MemoryCommand {
    let address = pixel::framebuffer_address(base, fill.x, fill.y, fill.stride);
    memory::MemoryCommand { write: u1:1, address: address[2+:u22], data, mask: fill.mask }
}

fn texture_layout_segment(texture_layout: bits[432], lod: u4) -> u48 {
    match lod {
        u4:0 => texture_layout[384+:u48],
        u4:1 => texture_layout[336+:u48],
        u4:2 => texture_layout[288+:u48],
        u4:3 => texture_layout[240+:u48],
        u4:4 => texture_layout[192+:u48],
        u4:5 => texture_layout[144+:u48],
        u4:6 => texture_layout[96+:u48],
        u4:7 => texture_layout[48+:u48],
        u4:8 => texture_layout[0+:u48],
        _ => u48:0,
    }
}

fn texture_download_address(state: CoreState, pci_address: u23) -> u27 {
    let lod = pci_address[17+:u4];
    let segment = texture_layout_segment(state.texture_layout, lod);
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

proc hosted_triangle_setup {
    request_r: chan<TriangleSetupRequest> in;
    response_s: chan<TriangleSetupResult> out;

    config(request_r: chan<TriangleSetupRequest> in, response_s: chan<TriangleSetupResult> out) {
        (request_r, response_s)
    }

    init { () }

    next(state: ()) {
        let (tok, request) = recv(join(), request_r);
        let setup = raster::triangle_setup(
            request.ax, request.ay, request.bx, request.by, request.cx, request.cy, request.reverse);
        let tok = send(
            tok, response_s,
            TriangleSetupResult {
                bounds: setup[480+:uN[80]],
                edges: setup[120+:uN[360]],
                starts: setup[0+:uN[120]],
                base_lod: texture_sampler::base_lod(
                    request.texture_derivatives[0], request.texture_derivatives[1],
                    request.texture_derivatives[2], request.texture_derivatives[3]),
            });
        state
    }
}

proc hosted_pixel_interpolation_stage<EXPORT_PROFILE: bool = {false}> {
    job_r: chan<PixelJob> in;
    work_s: chan<PixelInterpolatedValues> out;
    context_s: chan<ReadPixelContext> out;
    profile_start_s: chan<u1> out;

    config(job_r: chan<PixelJob> in, work_s: chan<PixelInterpolatedValues> out,
           context_s: chan<ReadPixelContext> out, profile_start_s: chan<u1> out) {
        (job_r, work_s, context_s, profile_start_s)
    }

    init { () }

    next(state: ()) {
        let (tok, job) = recv(join(), job_r);
        let tok = send_if(tok, profile_start_s, EXPORT_PROFILE, u1:1);
        let core = job.core;
        let values = PixelInterpolatedValues {
            s: interpolated_value(
                core.registers, core.float_shadows, core.raster_uses_float_shadows, job.candidate,
                u8:13, u8:21, u8:29),
            t: interpolated_value(
                core.registers, core.float_shadows, core.raster_uses_float_shadows, job.candidate,
                u8:14, u8:22, u8:30),
            w: interpolated_value(
                core.registers, core.float_shadows, core.raster_uses_float_shadows, job.candidate,
                u8:15, u8:23, u8:31),
            red: interpolated_color_values(
                core.registers, core.float_shadows, core.raster_uses_float_shadows, job.candidate,
                u8:8, u8:16, u8:24),
            green: interpolated_color_values(
                core.registers, core.float_shadows, core.raster_uses_float_shadows, job.candidate,
                u8:9, u8:17, u8:25),
            blue: interpolated_color_values(
                core.registers, core.float_shadows, core.raster_uses_float_shadows, job.candidate,
                u8:10, u8:18, u8:26),
            alpha: interpolated_color_values(
                core.registers, core.float_shadows, core.raster_uses_float_shadows, job.candidate,
                u8:12, u8:20, u8:28),
            z: interpolated_value(
                core.registers, core.float_shadows, core.raster_uses_float_shadows, job.candidate,
                u8:11, u8:19, u8:27),
        };
        let context = ReadPixelContext {
            base_lod: core.base_lod,
            path: pixel_register(core.registers, u8:65)[0+:u28],
            alpha_mode: pixel_register(core.registers, u8:67),
            fbz_mode: pixel_register(core.registers, u8:68)[0+:u21],
            clip_x_min: pixel_register(core.registers, u8:70)[16+:u10],
            clip_x_max: pixel_register(core.registers, u8:70)[0+:u10],
            clip_y_min: pixel_register(core.registers, u8:71)[16+:u10],
            clip_y_max: pixel_register(core.registers, u8:71)[0+:u10],
            za_color: pixel_register(core.registers, u8:76)[0+:u16],
            chroma_key: pixel_register(core.registers, u8:77)[0+:u24],
            color0: pixel_register(core.registers, u8:81),
            color1: pixel_register(core.registers, u8:82),
            framebuffer_tiles: pixel_register(core.registers, u8:133)[4+:u4],
            framebuffer_offset: pixel_register(core.registers, u8:134)[11+:u9],
            y_origin: pixel_register(core.registers, u8:135)[22+:u10],
            texture: core.texture,
            texture_layout: core.texture_layout,
            drawing: core.drawing,
            candidate: job.candidate,
            last: job.last,
        };
        let context_tok = send(tok, context_s, context);
        let tok = send(tok, work_s, values);
        state
    }
}

proc hosted_pixel_pipeline<EXPORT_PROFILE: bool = {false}> {
    job_r: chan<InterpolatedPixelJob> in;
    result_s: chan<PixelResult> out;
    cache_request_s: chan<texture_cache::CacheRequest> out;
    cache_response_r: chan<PixelCacheResponse> in;
    framebuffer_read_s: chan<memory::MemoryRequestPair> out;
    framebuffer_read_response_r: chan<memory::MemoryResponsePair> in;
    framebuffer_write_s: chan<memory::MemoryRequestPair> out;
    framebuffer_write_response_r: chan<memory::MemoryResponsePair> in;
    profile_pipeline_start_s: chan<u1> out;
    profile_cache_done_s: chan<u1> out;
    profile_read_done_s: chan<u1> out;
    profile_write_done_s: chan<u1> out;

    config(job_r: chan<InterpolatedPixelJob> in, result_s: chan<PixelResult> out,
           cache_request_s: chan<texture_cache::CacheRequest> out,
           cache_response_r: chan<PixelCacheResponse> in,
           framebuffer_read_s: chan<memory::MemoryRequestPair> out,
           framebuffer_read_response_r: chan<memory::MemoryResponsePair> in,
           framebuffer_write_s: chan<memory::MemoryRequestPair> out,
           framebuffer_write_response_r: chan<memory::MemoryResponsePair> in,
           profile_pipeline_start_s: chan<u1> out, profile_cache_done_s: chan<u1> out,
           profile_read_done_s: chan<u1> out, profile_write_done_s: chan<u1> out) {
        (
            job_r, result_s, cache_request_s, cache_response_r, framebuffer_read_s,
            framebuffer_read_response_r, framebuffer_write_s, framebuffer_write_response_r,
            profile_pipeline_start_s, profile_cache_done_s, profile_read_done_s,
            profile_write_done_s,
        )
    }

    init { () }

    next(state: ()) {
        let (tok, work) = recv(join(), job_r);
        let tok = send_if(tok, profile_pipeline_start_s, EXPORT_PROFILE, u1:1);
        let job = work.job;
        let core = job.core;
        let candidate = job.candidate;
        let interpolated = work.values;
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
        let sample = texture_request_interpolated(
            core.registers, core.texture, core.texture_layout, interpolated.s, interpolated.t,
            interpolated.w);
        let textured = raster_valid && path[27+:u1] == u1:1;
        let narrow = flush_narrow_texture_cache(
            u1:1, core.texture.t_lod as u32, core.texture.texture_mode as u32);
        let cache_access = textured || job.last;
        let cache_request = if textured {
            if narrow {
                texture_cache::narrow_sample_request(sample, core.texture_layout)
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
                texels: u16[4]:[u16:0, ...],
                palette_entries: u24[4]:[u24:0, ...],
            });
        let tok = send_if(tok, profile_cache_done_s, EXPORT_PROFILE, u1:1);
        let texture_rgba = if textured {
            texture_sampler::decode_texels_with_entries(
                sample, pixel_cache_response.texels[u32:0], pixel_cache_response.texels[u32:1],
                pixel_cache_response.texels[u32:2], pixel_cache_response.texels[u32:3],
                pixel_cache_response.palette_entries)
        } else {
            u32:0
        };
        let source_color = combined_color_fields(
            pixel_register(core.registers, u8:65), pixel_register(core.registers, u8:81),
            pixel_register(core.registers, u8:82), texture_rgba, interpolated.red,
            interpolated.green, interpolated.blue, interpolated.alpha,
            color_channel(interpolated.z));
        let depth = new_depth_interpolated(core.registers, interpolated.z, interpolated.w);
        let aux = aux_write_values(
            init1, init2, fbz_mode, y_origin, candidate, depth, source_color[0+:u8]);
        let depth_access = raster_valid && fbz_mode[4+:u1] == u1:1;
        let depth_read =
            memory::MemoryCommand { write: u1:0, address: aux.address, data: u32:0, mask: u4:0 };
        let (color_address, color_high) =
            raster_write_location(init1, init2, fbz_mode, core.drawing, y_origin, candidate);
        let blend_access = raster_valid && alpha_mode[4+:u1] == u1:1;
        let blend_read =
            memory::MemoryCommand { write: u1:0, address: color_address, data: u32:0, mask: u4:0 };
        let read_valid = (blend_access as u2) << u2:1 | (depth_access as u2);
        let framebuffer_reads = memory::MemoryRequestPair {
            valid: read_valid,
            commands: memory::MemoryCommand[2]:[depth_read, blend_read],
        };
        let tok = send_if(tok, framebuffer_read_s, read_valid != u2:0, framebuffer_reads);
        let (tok, read_responses) = recv_if(
            tok, framebuffer_read_response_r, read_valid != u2:0, memory::empty_response_pair());
        let tok = send_if(tok, profile_read_done_s, EXPORT_PROFILE, u1:1);
        let depth_response = read_responses.responses[u32:0];
        let blend_response = read_responses.responses[u32:1];
        let old_depth = if aux.mask == u4:0xc {
            depth_response.data[16+:u16]
        } else {
            depth_response.data[0+:u16]
        };
        let depth_pass = fbz_mode[4+:u1] == u1:0 ||
                         pixel::depth_compare(fbz_mode[5+:u3], depth, old_depth) == u1:1;
        let tests = pixel::pixel_tests(
            source_color[8+:u24], chroma_key[0+:u24], source_color[0+:u8], alpha_mode[24+:u8],
            alpha_mode[1+:u3]);
        let tests_pass = (fbz_mode[1+:u1] == u1:0 || tests[1+:u1] == u1:0) &&
                         (alpha_mode[0+:u1] == u1:0 || tests[0+:u1] == u1:1);
        let pixel_valid = raster_valid && depth_pass && tests_pass;
        let destination = if color_high == u1:1 {
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
        let tok = send_if(tok, profile_write_done_s, EXPORT_PROFILE, u1:1);
        let tok = send(
            tok, result_s,
            PerformanceEvent {
                input_pixel: candidate.valid,
                output_pixel: pixel_valid,
                triangle_done: job.last,
                pipeline_job: u1:1,
                textured,
                depth_read: depth_access,
                blend_read: blend_access,
                color_write: color_valid,
                aux_write: aux_valid,
            });
        state
    }
}

proc hosted_pixel_texture_requests<EXPORT_PROFILE: bool = {false}> {
    job_r: chan<PixelInterpolatedValues> in;
    context_r: chan<ReadPixelContext> in;
    job_s: chan<TexturePixelJob> out;
    cache_request_s: chan<texture_cache::CacheRequest> out;
    framebuffer_read_s: chan<memory::MemoryRequestPair> out;
    profile_pipeline_start_s: chan<u1> out;

    config(job_r: chan<PixelInterpolatedValues> in, context_r: chan<ReadPixelContext> in,
           job_s: chan<TexturePixelJob> out, cache_request_s: chan<texture_cache::CacheRequest> out,
           framebuffer_read_s: chan<memory::MemoryRequestPair> out,
           profile_pipeline_start_s: chan<u1> out) {
        (job_r, context_r, job_s, cache_request_s, framebuffer_read_s, profile_pipeline_start_s)
    }

    init { () }

    next(state: ()) {
        let (tok, values) = recv(join(), job_r);
        let (tok, context) = recv(tok, context_r);
        let work = InterpolatedPixelJob { context, values };
        let tok = send_if(tok, profile_pipeline_start_s, EXPORT_PROFILE, u1:1);
        let raster_valid = candidate_visible_fields(
            context.fbz_mode, context.clip_x_min, context.clip_x_max, context.clip_y_min,
            context.clip_y_max, context.candidate);
        let sample = texture_sampler::sample_request_with_base_lod(
            work.values.s, work.values.t, work.values.w, context.base_lod,
            pixel_texture_mode(context.texture), context.texture.t_lod as u32,
            context.texture_layout, context.texture.tex_base, context.texture.tex_base1,
            context.texture.tex_base2, context.texture.tex_base38);
        let textured = raster_valid && context.path[27+:u1] == u1:1;
        let narrow = flush_narrow_texture_cache(
            u1:1, context.texture.t_lod as u32, context.texture.texture_mode as u32);
        let cache_access = textured || context.last;
        let request = if textured {
            if narrow {
                texture_cache::narrow_sample_request(sample, context.texture_layout)
            } else {
                texture_cache::sample_request(sample, context.texture_layout)
            }
        } else {
            texture_cache::status_request()
        };
        let tok = send_if(tok, cache_request_s, cache_access, request);
        let init1 = (context.framebuffer_tiles as u32) << u32:4;
        let init2 = (context.framebuffer_offset as u32) << u32:11;
        let early_aux = aux_write_values(
            init1, init2, context.fbz_mode as u32, context.y_origin, context.candidate, u16:0, u8:0);
        let depth_access = raster_valid && context.fbz_mode[4+:u1] == u1:1;
        let depth_read = memory::MemoryCommand {
            write: u1:0,
            address: early_aux.address,
            data: u32:0,
            mask: u4:0,
        };
        let (color_address, _) = raster_write_location(
            init1, init2, context.fbz_mode as u32, context.drawing, context.y_origin,
            context.candidate);
        let blend_access = raster_valid && context.alpha_mode[4+:u1] == u1:1;
        let blend_read =
            memory::MemoryCommand { write: u1:0, address: color_address, data: u32:0, mask: u4:0 };
        let read_valid = (blend_access as u2) << u2:1 | (depth_access as u2);
        let framebuffer_reads = memory::MemoryRequestPair {
            valid: read_valid,
            commands: memory::MemoryCommand[2]:[depth_read, blend_read],
        };
        let tok = send_if(tok, framebuffer_read_s, read_valid != u2:0, framebuffer_reads);
        let read_context = TexturePixelContext {
            path: context.path,
            alpha_mode: context.alpha_mode,
            fbz_mode: context.fbz_mode,
            za_color: context.za_color,
            chroma_key: context.chroma_key,
            color0: context.color0,
            color1: context.color1,
            framebuffer_tiles: context.framebuffer_tiles,
            framebuffer_offset: context.framebuffer_offset,
            y_origin: context.y_origin,
            drawing: context.drawing,
            candidate: context.candidate,
            last: context.last,
        };
        let read_values = TexturePixelValues {
            red: work.values.red,
            green: work.values.green,
            blue: work.values.blue,
            alpha: work.values.alpha,
            z: color_channel(work.values.z),
            z_depth: clamp_depth(work.values.z >> u8:12),
            w: work.values.w as s32,
        };
        let tok = send(
            tok, job_s,
            TexturePixelJob {
                context: read_context,
                values: read_values,
                sample,
                raster_valid,
                textured,
                cache_access,
            });
        state
    }
}

proc hosted_pixel_read_pipeline<EXPORT_PROFILE: bool = {false}> {
    job_r: chan<TexturePixelJob> in;
    read_job_s: chan<ReadPixelJob> out;
    cache_response_r: chan<PixelCacheResponse> in;
    profile_cache_done_s: chan<u1> out;

    config(job_r: chan<TexturePixelJob> in, read_job_s: chan<ReadPixelJob> out,
           cache_response_r: chan<PixelCacheResponse> in, profile_cache_done_s: chan<u1> out) {
        (job_r, read_job_s, cache_response_r, profile_cache_done_s)
    }

    init { () }

    next(state: ()) {
        let (tok, texture_job) = recv(join(), job_r);
        let context = texture_job.context;
        let candidate = context.candidate;
        let interpolated = texture_job.values;
        let alpha_mode = context.alpha_mode;
        let fbz_mode = context.fbz_mode;
        let chroma_key = context.chroma_key;
        let framebuffer_tiles = context.framebuffer_tiles;
        let framebuffer_offset = context.framebuffer_offset;
        let init1 = (framebuffer_tiles as u32) << u32:4;
        let init2 = (framebuffer_offset as u32) << u32:11;
        let y_origin = context.y_origin;
        let raster_valid = texture_job.raster_valid;
        let sample = texture_job.sample;
        let textured = texture_job.textured;
        let cache_access = texture_job.cache_access;
        let (tok, pixel_cache_response) = recv_if(
            tok, cache_response_r, cache_access,
            PixelCacheResponse {
                texels: u16[4]:[u16:0, ...],
                palette_entries: u24[4]:[u24:0, ...],
            });
        let tok = send_if(tok, profile_cache_done_s, EXPORT_PROFILE, u1:1);
        let texture_rgba = if textured {
            texture_sampler::decode_texels_with_entries(
                sample, pixel_cache_response.texels[u32:0], pixel_cache_response.texels[u32:1],
                pixel_cache_response.texels[u32:2], pixel_cache_response.texels[u32:3],
                pixel_cache_response.palette_entries)
        } else {
            u32:0
        };
        let source_color = combined_color_fields(
            context.path as u32, context.color0, context.color1, texture_rgba, interpolated.red,
            interpolated.green, interpolated.blue, interpolated.alpha, interpolated.z);
        let depth = new_depth_fields(
            context.fbz_mode as u32, context.za_color as u32, interpolated.z_depth, interpolated.w);
        let aux = aux_write_values(
            init1, init2, fbz_mode as u32, y_origin, candidate, depth, source_color[0+:u8]);
        let depth_access = raster_valid && fbz_mode[4+:u1] == u1:1;
        let (_, color_high) = raster_write_location(
            init1, init2, fbz_mode as u32, context.drawing, y_origin, candidate);
        let blend_access = raster_valid && alpha_mode[4+:u1] == u1:1;
        let read_valid = (blend_access as u2) << u2:1 | (depth_access as u2);
        let tests = pixel::pixel_tests(
            source_color[8+:u24], chroma_key[0+:u24], source_color[0+:u8], alpha_mode[24+:u8],
            alpha_mode[1+:u3]);
        let tests_pass = (fbz_mode[1+:u1] == u1:0 || tests[1+:u1] == u1:0) &&
                         (alpha_mode[0+:u1] == u1:0 || tests[0+:u1] == u1:1);
        let tok = send(
            tok, read_job_s,
            ReadPixelJob {
                candidate,
                source_color,
                depth,
                aux,
                color_high,
                framebuffer_tiles,
                framebuffer_offset,
                fbz_mode: fbz_mode[0+:u20],
                alpha_mode: alpha_mode[0+:u16],
                drawing: context.drawing,
                y_origin,
                raster_valid,
                tests_pass,
                read_access: (read_valid != u2:0) as u1,
                depth_access,
                blend_access,
                textured,
                last: context.last,
            });
        state
    }
}

proc hosted_pixel_write_pipeline<EXPORT_PROFILE: bool = {false}> {
    job_r: chan<ReadPixelJob> in;
    pending_s: chan<PendingPixelResult> out;
    framebuffer_read_response_r: chan<memory::MemoryResponsePair> in;
    framebuffer_write_s: chan<memory::MemoryRequestPair> out;
    profile_read_done_s: chan<u1> out;

    config(job_r: chan<ReadPixelJob> in, pending_s: chan<PendingPixelResult> out,
           framebuffer_read_response_r: chan<memory::MemoryResponsePair> in,
           framebuffer_write_s: chan<memory::MemoryRequestPair> out,
           profile_read_done_s: chan<u1> out) {
        (job_r, pending_s, framebuffer_read_response_r, framebuffer_write_s, profile_read_done_s)
    }

    init { () }

    next(state: ()) {
        let (tok, job) = recv(join(), job_r);
        let (tok, read_responses) = recv_if(
            tok, framebuffer_read_response_r, job.read_access == u1:1,
            memory::empty_response_pair());
        let tok = send_if(tok, profile_read_done_s, EXPORT_PROFILE, u1:1);
        let depth_response = read_responses.responses[u32:0];
        let blend_response = read_responses.responses[u32:1];
        let old_depth = if job.aux.mask == u4:0xc {
            depth_response.data[16+:u16]
        } else {
            depth_response.data[0+:u16]
        };
        let depth_pass = job.fbz_mode[4+:u1] == u1:0 ||
                         pixel::depth_compare(job.fbz_mode[5+:u3], job.depth, old_depth) == u1:1;
        let pixel_valid = job.raster_valid && depth_pass && job.tests_pass;
        let destination = if job.color_high == u1:1 {
            blend_response.data[16+:u16]
        } else {
            blend_response.data[0+:u16]
        };
        let destination_rgb = decode_rgb565(destination);
        let destination_rgb = if job.fbz_mode[19+:u1] == u1:1 {
            dither::subtract_rgb(
                destination_rgb, job.candidate.x as u16,
                framebuffer_y_values(job.fbz_mode as u32, job.y_origin, job.candidate),
                job.fbz_mode[11+:u1])
        } else {
            destination_rgb
        };
        let color_valid = pixel_valid && job.fbz_mode[9+:u1] == u1:1;
        let init1 = (job.framebuffer_tiles as u32) << u32:4;
        let init2 = (job.framebuffer_offset as u32) << u32:11;
        let color_write = raster_write_color(
            init1, init2, job.fbz_mode as u32, job.alpha_mode as u32, job.drawing, job.y_origin,
            job.candidate, job.source_color, destination_rgb);
        let aux_valid = pixel_valid && job.fbz_mode[10+:u1] == u1:1;
        let write_valid = (aux_valid as u2) << u2:1 | (color_valid as u2);
        let framebuffer_writes = memory::MemoryRequestPair {
            valid: write_valid,
            commands: memory::MemoryCommand[2]:[color_write, job.aux],
        };
        let tok = send_if(tok, framebuffer_write_s, write_valid != u2:0, framebuffer_writes);
        let event = PerformanceEvent {
            input_pixel: job.candidate.valid,
            output_pixel: pixel_valid,
            triangle_done: job.last,
            pipeline_job: u1:1,
            textured: job.textured,
            depth_read: job.depth_access,
            blend_read: job.blend_access,
            color_write: color_valid,
            aux_write: aux_valid,
        };
        let tok = send(
            tok, pending_s, PendingPixelResult { event, write_access: (write_valid != u2:0) as u1 });
        state
    }
}

proc hosted_pixel_posted_retire_pipeline<EXPORT_PROFILE: bool = {false}> {
    pending_r: chan<PendingPixelResult> in;
    result_s: chan<PixelResult> out;
    profile_write_done_s: chan<u1> out;

    config(pending_r: chan<PendingPixelResult> in, result_s: chan<PixelResult> out,
           profile_write_done_s: chan<u1> out) {
        (pending_r, result_s, profile_write_done_s)
    }

    init { () }

    next(state: ()) {
        let (tok, pending) = recv(join(), pending_r);
        let tok = send_if(tok, profile_write_done_s, EXPORT_PROFILE, u1:1);
        let tok = send(tok, result_s, pending.event);
        state
    }
}

proc hosted_pixel_write_response_drain {
    framebuffer_write_response_r: chan<memory::MemoryResponsePair> in;

    config(framebuffer_write_response_r: chan<memory::MemoryResponsePair> in) {
        (framebuffer_write_response_r,)
    }

    init { () }

    next(state: ()) {
        let (tok, _) = recv(join(), framebuffer_write_response_r);
        state
    }
}

proc hosted_pixel_retire_pipeline<EXPORT_PROFILE: bool = {false}> {
    pending_r: chan<PendingPixelResult> in;
    result_s: chan<PixelResult> out;
    framebuffer_write_response_r: chan<memory::MemoryResponsePair> in;
    profile_write_done_s: chan<u1> out;

    config(pending_r: chan<PendingPixelResult> in, result_s: chan<PixelResult> out,
           framebuffer_write_response_r: chan<memory::MemoryResponsePair> in,
           profile_write_done_s: chan<u1> out) {
        (pending_r, result_s, framebuffer_write_response_r, profile_write_done_s)
    }

    init { () }

    next(state: ()) {
        let (tok, pending) = recv(join(), pending_r);
        let (tok, _) = recv_if(
            tok, framebuffer_write_response_r, pending.write_access == u1:1,
            memory::empty_response_pair());
        let tok = send_if(tok, profile_write_done_s, EXPORT_PROFILE, u1:1);
        let tok = send(tok, result_s, pending.event);
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
        let is_register = command_valid && command.address[22+:u2] == u2:0;
        let is_framebuffer = command_valid && command.address[22+:u2] == u2:1;
        let is_texture = command_valid && command.address[23+:u1] == u1:1;
        let framebuffer_command = lfb_command(state, command);
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
                total_hits: u24:0,
                total_misses: u24:0,
                all_hit_requests: u24:0,
                missed_requests: u24:0,
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
                total_hits: u24:0,
                total_misses: u24:0,
                all_hit_requests: u24:0,
                missed_requests: u24:0,
            });
        let (tok, texture_block_response) = if ENABLE_BURSTS {
            let tok = send_if(tok, texture_block_s, block_access, texture_block);
            recv_if(
                tok, texture_block_response_r, burst_valid && texture_block_valid,
                memory::MemoryBlockResponse { success: u1:0 })
        } else {
            (tok, memory::MemoryBlockResponse { success: u1:0 })
        };
        let (next_state, register_response) = if is_register {
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
            register_read(next_state.registers, u8:68), register_read(next_state.registers, u8:70),
            register_read(next_state.registers, u8:71), candidate);
        let sample = texture_request_values(
            pixel_registers(next_state.registers), pixel_float_shadows(next_state.float_shadows),
            next_state.raster_uses_float_shadows, next_state.texture, next_state.texture_layout,
            candidate);
        let textured = raster_valid && register_read(next_state.registers, u8:65)[27+:u1] == u1:1;
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
                total_hits: u24:0,
                total_misses: u24:0,
                all_hit_requests: u24:0,
                missed_requests: u24:0,
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
                total_hits: u24:0,
                total_misses: u24:0,
                all_hit_requests: u24:0,
                missed_requests: u24:0,
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
                total_hits: u24:0,
                total_misses: u24:0,
                all_hit_requests: u24:0,
                missed_requests: u24:0,
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
                total_hits: u24:0,
                total_misses: u24:0,
                all_hit_requests: u24:0,
                missed_requests: u24:0,
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
                total_hits: u24:0,
                total_misses: u24:0,
                all_hit_requests: u24:0,
                missed_requests: u24:0,
            });
        let next_state = if de10_sample {
            CoreState {
                texture_cache_hits: cached_sample.total_hits as u32,
                texture_cache_misses: cached_sample.total_misses as u32,
                texture_cache_all_hit_requests: cached_sample.all_hit_requests as u32,
                texture_cache_missed_requests: cached_sample.missed_requests as u32,
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
        let alpha_mode = register_read(next_state.registers, u8:67);
        let depth = new_depth_values(
            pixel_registers(next_state.registers), pixel_float_shadows(next_state.float_shadows),
            next_state.raster_uses_float_shadows, candidate);
        let aux = aux_write_values(
            register_read(next_state.registers, u8:133),
            register_read(next_state.registers, u8:134),
            register_read(next_state.registers, u8:68),
            register_read(next_state.registers, u8:135)[22+:u10], candidate, depth,
            source_color[0+:u8]);
        let depth_access =
            raster_valid && register_read(next_state.registers, u8:68)[4+:u1] == u1:1;
        let depth_read =
            memory::MemoryCommand { write: u1:0, address: aux.address, data: u32:0, mask: u4:0 };
        let (color_address, color_high) = raster_write_location(
            register_read(next_state.registers, u8:133),
            register_read(next_state.registers, u8:134),
            register_read(next_state.registers, u8:68), next_state.commands.swap.draw_count[0+:u1],
            register_read(next_state.registers, u8:135)[22+:u10], candidate);
        let blend_access = raster_valid && alpha_mode[4+:u1] == u1:1;
        let blend_read =
            memory::MemoryCommand { write: u1:0, address: color_address, data: u32:0, mask: u4:0 };
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
        let depth_pass = register_read(next_state.registers, u8:68)[4+:u1] == u1:0 ||
                         pixel::depth_compare(
                             register_read(next_state.registers, u8:68)[5+:u3], depth, old_depth) ==
                         u1:1;
        let tests = pixel::pixel_tests(
            source_color[8+:u24], register_read(next_state.registers, u8:77)[0+:u24],
            source_color[0+:u8], alpha_mode[24+:u8], alpha_mode[1+:u3]);
        let tests_pass = (register_read(next_state.registers, u8:68)[1+:u1] == u1:0 ||
                         tests[1+:u1] == u1:0) &&
                         (alpha_mode[0+:u1] == u1:0 || tests[0+:u1] == u1:1);
        let pixel_valid = raster_valid && depth_pass && tests_pass;
        let destination = if color_high == u1:1 {
            blend_response.data[16+:u16]
        } else {
            blend_response.data[0+:u16]
        };
        let destination_rgb = decode_rgb565(destination);
        let destination_rgb = if register_read(next_state.registers, u8:68)[19+:u1] == u1:1 {
            dither::subtract_rgb(
                destination_rgb, candidate.x as u16,
                framebuffer_y_values(
                    register_read(next_state.registers, u8:68),
                    register_read(next_state.registers, u8:135)[22+:u10], candidate),
                register_read(next_state.registers, u8:68)[11+:u1])
        } else {
            destination_rgb
        };
        let color_valid = pixel_valid && register_read(next_state.registers, u8:68)[9+:u1] == u1:1;
        let color_write = raster_write_color(
            register_read(next_state.registers, u8:133),
            register_read(next_state.registers, u8:134),
            register_read(next_state.registers, u8:68), register_read(next_state.registers, u8:67),
            next_state.commands.swap.draw_count[0+:u1],
            register_read(next_state.registers, u8:135)[22+:u10], candidate, source_color,
            destination_rgb);
        let aux_valid = pixel_valid && register_read(next_state.registers, u8:68)[10+:u1] == u1:1;
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

proc palette_lookup_requests {
    lookup_r: chan<PixelCacheResponse> in;
    pending_s: chan<bits[64]> out;
    palette_read_request_s: chan<PaletteReadRequest> out;

    config(lookup_r: chan<PixelCacheResponse> in, pending_s: chan<bits[64]> out,
           palette_read_request_s: chan<PaletteReadRequest> out) {
        (lookup_r, pending_s, palette_read_request_s)
    }

    init { () }

    next(state: ()) {
        let (tok, lookup) = recv(join(), lookup_r);
        let tok = send(tok, palette_read_request_s, palette_read_request(lookup.texels));
        let tok = send(
            tok, pending_s,
            lookup.texels[3] ++ lookup.texels[2] ++ lookup.texels[1] ++ lookup.texels[0]);
        state
    }
}

proc palette_lookup_responses {
    pending_r: chan<bits[64]> in;
    response_s: chan<PixelCacheResponse> out;
    palette_response_r: chan<PaletteResponse> in;

    config(pending_r: chan<bits[64]> in, response_s: chan<PixelCacheResponse> out,
           palette_response_r: chan<PaletteResponse> in) {
        (pending_r, response_s, palette_response_r)
    }

    init { () }

    next(state: ()) {
        let (tok, packed_texels) = recv(join(), pending_r);
        let texels = u16[4]:[
            packed_texels[0+:u16], packed_texels[16+:u16], packed_texels[32+:u16],
            packed_texels[48+:u16],
        ];
        let (tok, palette) = recv(tok, palette_response_r);
        let tok = send(tok, response_s, external_pixel_cache_response(texels, palette));
        state
    }
}

proc palette_lookup {
    config(lookup_r: chan<PixelCacheResponse> in, response_s: chan<PixelCacheResponse> out,
           palette_read_request_s: chan<PaletteReadRequest> out,
           palette_response_r: chan<PaletteResponse> in) {
        let (pending_s, pending_r) = #[channel(depth=8, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                     chan<bits[64]>("palette_pending");
        spawn palette_lookup_requests(lookup_r, pending_s, palette_read_request_s);
        spawn palette_lookup_responses(pending_r, response_s, palette_response_r);
        ()
    }

    init { () }

    next(state: ()) { state }
}

proc hosted_palette_memory {
    read_r: chan<PaletteReadRequest> in;
    response_s: chan<PaletteResponse> out;
    write_r: chan<PaletteWrite> in;

    config(read_r: chan<PaletteReadRequest> in, response_s: chan<PaletteResponse> out,
           write_r: chan<PaletteWrite> in) {
        (read_r, response_s, write_r)
    }

    init { Palette:[u24:0, ...] }

    next(palette: Palette) {
        let (tok, read, read_valid) = recv_non_blocking(
            join(), read_r,
            PaletteReadRequest { index0: u8:0, index1: u8:0, index2: u8:0, index3: u8:0 });
        let (tok, write, write_valid) =
            recv_non_blocking(tok, write_r, PaletteWrite { select: u1:0, index: u8:0, data: u24:0 });
        let tok = send_if(
            tok, response_s, read_valid,
            PaletteResponse {
                entry0: palette[read.index0],
                entry1: palette[read.index1],
                entry2: palette[read.index2],
                entry3: palette[read.index3],
            });
        if write_valid { update(palette, write.index, write.data) } else { palette }
    }
}

proc texture_cache_requests {
    scalar_request_r: chan<texture_cache::CacheRequest> in;
    pixel_request_r: chan<texture_cache::CacheRequest> in;
    request_s: chan<texture_cache::CacheRequest> out;
    route_s: chan<u1> out;

    config(scalar_request_r: chan<texture_cache::CacheRequest> in,
           pixel_request_r: chan<texture_cache::CacheRequest> in,
           request_s: chan<texture_cache::CacheRequest> out, route_s: chan<u1> out) {
        (scalar_request_r, pixel_request_r, request_s, route_s)
    }

    init { () }

    next(state: ()) {
        let empty = texture_cache::status_request();
        let (tok, scalar_request, scalar_valid) =
            recv_non_blocking(join(), scalar_request_r, empty);
        let (tok, pixel_request, pixel_valid) = if scalar_valid {
            (tok, empty, false)
        } else {
            recv_non_blocking(tok, pixel_request_r, empty)
        };
        let valid = scalar_valid || pixel_valid;
        let tok = send_if(
            tok, request_s, valid, if scalar_valid { scalar_request } else { pixel_request });
        let tok = send_if(tok, route_s, valid, scalar_valid);
        state
    }
}

proc texture_cache_responses {
    route_r: chan<u1> in;
    response_r: chan<texture_cache::CacheResponse> in;
    scalar_response_s: chan<texture_cache::CacheResponse> out;
    pixel_response_s: chan<PixelCacheResponse> out;

    config(route_r: chan<u1> in, response_r: chan<texture_cache::CacheResponse> in,
           scalar_response_s: chan<texture_cache::CacheResponse> out,
           pixel_response_s: chan<PixelCacheResponse> out) {
        (route_r, response_r, scalar_response_s, pixel_response_s)
    }

    init { () }

    next(state: ()) {
        let (tok, scalar) = recv(join(), route_r);
        let (tok, response) = recv(tok, response_r);
        let tok = send_if(tok, scalar_response_s, scalar == u1:1, response);
        let tok = send_if(
            tok, pixel_response_s, scalar == u1:0,
            PixelCacheResponse { texels: response.texels, palette_entries: u24[4]:[u24:0, ...] });
        state
    }
}

proc texture_cache_arbiter {
    config(scalar_request_r: chan<texture_cache::CacheRequest> in,
           scalar_response_s: chan<texture_cache::CacheResponse> out,
           pixel_request_r: chan<texture_cache::CacheRequest> in,
           pixel_response_s: chan<PixelCacheResponse> out,
           request_s: chan<texture_cache::CacheRequest> out,
           response_r: chan<texture_cache::CacheResponse> in) {
        let (route_s, route_r) = #[channel(depth=8, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                 chan<u1>("texture_cache_route");
        spawn texture_cache_requests(scalar_request_r, pixel_request_r, request_s, route_s);
        spawn texture_cache_responses(route_r, response_r, scalar_response_s, pixel_response_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

proc float_shadow_converter {
    request_r: chan<FloatShadowRequest> in;
    response_s: chan<FloatShadowResponse> out;

    config(request_r: chan<FloatShadowRequest> in, response_s: chan<FloatShadowResponse> out) {
        (request_r, response_s)
    }

    init { () }

    next(state: ()) {
        let (tok, request) = recv(join(), request_r);
        let tok = send(tok, response_s, convert_float_register(request));
        state
    }
}

proc core_sim_hosted_controller<EXPORT_SCANOUT: bool = {false}, EXTERNAL_PALETTE: bool = {false}, EXPORT_PERFORMANCE:
bool = {
    false}, SPAN_PREFETCH: bool = {false}, DE10_ONLY: bool = {false}>
{
    command_r: chan<CoreCommand> in;
    de10_request_r: chan<De10MmioRequest> in;
    response_s: chan<CoreResponse> out;
    framebuffer_command_s: chan<memory::MemoryCommand> out;
    framebuffer_response_r: chan<memory::MemoryResponse> in;
    framebuffer_fastfill_s: chan<memory::MemoryRequestPair> out;
    framebuffer_fastfill_response_r: chan<memory::MemoryResponsePair> in;
    span_job_s: chan<FramebufferSpanJob> out;
    span_row_ready_r: chan<raster_state::RowSpan> in;
    cache_request_s: chan<texture_cache::CacheRequest> out;
    cache_response_r: chan<texture_cache::CacheResponse> in;
    pixel_job_s: chan<PixelJob> out;
    pixel_result_r: chan<PixelResult> in;
    triangle_setup_request_s: chan<TriangleSetupRequest> out;
    triangle_setup_response_r: chan<TriangleSetupResult> in;
    v_retrace_r: chan<u1> in;
    scanout_s: chan<ScanoutState> out;
    float_shadow_request_s: chan<FloatShadowRequest> out;
    float_shadow_response_r: chan<FloatShadowResponse> in;
    palette_write_s: chan<PaletteWrite> out;
    performance_s: chan<PerformanceEvent> out;
    profile_pending_s: chan<u6> out;

    config(command_r: chan<CoreCommand> in, de10_request_r: chan<De10MmioRequest> in,
           response_s: chan<CoreResponse> out,
           framebuffer_command_s: chan<memory::MemoryCommand> out,
           framebuffer_response_r: chan<memory::MemoryResponse> in,
           framebuffer_fastfill_s: chan<memory::MemoryRequestPair> out,
           framebuffer_fastfill_response_r: chan<memory::MemoryResponsePair> in,
           span_job_s: chan<FramebufferSpanJob> out,
           span_row_ready_r: chan<raster_state::RowSpan> in,
           cache_request_s: chan<texture_cache::CacheRequest> out,
           cache_response_r: chan<texture_cache::CacheResponse> in, pixel_job_s: chan<PixelJob> out,
           pixel_result_r: chan<PixelResult> in,
           triangle_setup_request_s: chan<TriangleSetupRequest> out,
           triangle_setup_response_r: chan<TriangleSetupResult> in, v_retrace_r: chan<u1> in,
           scanout_s: chan<ScanoutState> out, float_shadow_request_s: chan<FloatShadowRequest> out,
           float_shadow_response_r: chan<FloatShadowResponse> in,
           palette_write_s: chan<PaletteWrite> out, performance_s: chan<PerformanceEvent> out,
           profile_pending_s: chan<u6> out) {
        (
            command_r, de10_request_r, response_s, framebuffer_command_s, framebuffer_response_r,
            framebuffer_fastfill_s, framebuffer_fastfill_response_r, span_job_s, span_row_ready_r,
            cache_request_s, cache_response_r, pixel_job_s, pixel_result_r,
            triangle_setup_request_s, triangle_setup_response_r, v_retrace_r, scanout_s,
            float_shadow_request_s, float_shadow_response_r, palette_write_s, performance_s,
            profile_pending_s,
        )
    }

    init {
        HostedControllerState {
            core: initial_state(),
            float_pending: false,
            float_command: CoreCommand { write: u1:0, address: u24:0, data: u32:0, mask: u4:0 },
            base_lod: s16:0,
            pixels_pending: u6:0,
            triangle_pending: u1:0,
            triangle_install: u1:0,
        }
    }

    next(state: HostedControllerState) {
        let (_, v_retrace, v_retrace_valid) = recv_non_blocking(join(), v_retrace_r, u1:0);
        let (result_tok, pixel_result, result_valid) = recv_non_blocking(
            join(), pixel_result_r,
            PerformanceEvent {
                input_pixel: u1:0,
                output_pixel: u1:0,
                triangle_done: u1:0,
                pipeline_job: u1:0,
                textured: u1:0,
                depth_read: u1:0,
                blend_read: u1:0,
                color_write: u1:0,
                aux_write: u1:0,
            });
        let core = if EXTERNAL_PALETTE {
            CoreState { palette: Palette:[u24:0, ...], ..state.core }
        } else {
            state.core
        };
        let pixels_pending = state.pixels_pending - (result_valid as u6);
        let triangle_pending = state.triangle_pending;
        let degenerate_triangle = state.triangle_install;
        let performance_event = if result_valid {
            pixel_result
        } else {
            PerformanceEvent {
                input_pixel: u1:0,
                output_pixel: u1:0,
                triangle_done: u1:1,
                pipeline_job: u1:0,
                textured: u1:0,
                depth_read: u1:0,
                blend_read: u1:0,
                color_write: u1:0,
                aux_write: u1:0,
            }
        };
        let pending_tok = send_if(
            result_tok, profile_pending_s, EXPORT_PERFORMANCE && result_valid, state.pixels_pending);
        let performance_tok = send_if(
            pending_tok, performance_s,
            EXPORT_PERFORMANCE && (result_valid || degenerate_triangle), performance_event);
        if state.triangle_install {
            HostedControllerState {
                core,
                pixels_pending,
                triangle_pending,
                triangle_install: u1:0,
                ..state
            }
        } else if triangle_pending && pixels_pending == u6:0 {
            let (triangle_tok, triangle_setup_result) = recv(join(), triangle_setup_response_r);
            let triangle_setup = raster_state::from_triangle_parts(
                triangle_setup_result.bounds, triangle_setup_result.edges,
                triangle_setup_result.starts);
            let span_tok = send_if(
                triangle_tok, span_job_s, SPAN_PREFETCH && triangle_setup.active == u1:1,
                FramebufferSpanJob {
                    raster: triangle_setup,
                    config: framebuffer_span_config(core),
                });
            HostedControllerState {
                core: CoreState { raster: triangle_setup, ..core },
                base_lod: triangle_setup_result.base_lod,
                pixels_pending,
                triangle_pending: u1:0,
                triangle_install: !triangle_setup.active,
                ..state
            }
        } else if SPAN_PREFETCH && core.raster.active == u1:1 && core.raster.row_prefetched == u1:0 &&
        core.fastfill.active == u1:0 {
            let (span_tok, span) = recv(join(), span_row_ready_r);
            HostedControllerState {
                core: CoreState { raster: raster_state::install_span(core.raster, span), ..core },
                base_lod: state.base_lod,
                pixels_pending,
                triangle_pending,
                triangle_install: u1:0,
                ..state
            }
        } else if (core.raster.active == u1:1 && pixels_pending < u6:63) ||
        core.fastfill.active == u1:1 {
            let (commands, raster, fastfill, candidate, fill) = tick_events<true, SPAN_PREFETCH>(
                core.commands, core.raster, core.fastfill,
                if v_retrace_valid { v_retrace } else { u1:0 });
            let ticked_core = CoreState { commands, raster, fastfill, ..core };
            let raster_tick = core.raster.active == u1:1 && pixels_pending < u6:63;
            let issue_pixel = raster_tick && (candidate.valid == u1:1 || raster.active == u1:0);
            let scanout_tok =
                send_if(join(), scanout_s, EXPORT_SCANOUT, scanout_state(ticked_core));
            let job = PixelJob {
                core: pixel_context(ticked_core, state.base_lod),
                candidate,
                last: raster_tick && raster.active == u1:0,
            };
            let job_tok = send_if(scanout_tok, pixel_job_s, issue_pixel, job);
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
                send_if(job_tok, framebuffer_fastfill_s, fill_valid != u2:0, framebuffer_fills);
            let (fill_tok, _) = recv_if(
                fill_tok, framebuffer_fastfill_response_r, fill_valid != u2:0,
                memory::empty_response_pair());
            HostedControllerState {
                core: ticked_core,
                base_lod: state.base_lod,
                pixels_pending: pixels_pending + (issue_pixel as u6),
                triangle_pending,
                triangle_install: u1:0,
                ..state
            }
        } else {
            let default_command =
                CoreCommand { write: u1:0, address: u24:0, data: u32:0, mask: u4:0 };
            let receive_command =
                triangle_pending == u1:0 && pixels_pending == u6:0 && !state.float_pending;
            let (command_tok, core_command, core_command_valid) = if receive_command && !DE10_ONLY {
                recv_non_blocking(join(), command_r, default_command)
            } else {
                (join(), default_command, false)
            };
            let default_request = De10MmioRequest {
                address: u22:0,
                read: u1:0,
                write: u1:0,
                byte_enable: u4:0,
                write_data: u32:0,
            };
            let (request_tok, request, request_valid) = if receive_command && !core_command_valid {
                recv_non_blocking(command_tok, de10_request_r, default_request)
            } else {
                (command_tok, default_request, false)
            };
            let request_legal = de10_request_valid(request);
            let command = if state.float_pending {
                state.float_command
            } else if request_valid {
                de10_core_command(request)
            } else {
                core_command
            };
            let received = core_command_valid || (request_valid && request_legal);
            let (write_address, fraction_bits, is_float) =
                float_alias::alias(command.address[0+:u10]);
            let defer_float = received && command.address[22+:u2] == u2:0 && command.write == u1:1 &&
                              is_float == u1:1;
            let command_valid = state.float_pending || (received && !defer_float);
            let response_valid =
                state.float_pending || ((core_command_valid || request_valid) && !defer_float);
            let is_register = command_valid && command.address[22+:u2] == u2:0;
            let is_framebuffer = command_valid && command.address[22+:u2] == u2:1;
            let is_texture = command_valid && command.address[23+:u1] == u1:1;
            let framebuffer_command = lfb_command(core, command);
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
            let texture_layout_access = is_register && command.address[5+:u5] == u5:0x18;
            let texture_layout_write = texture_layout_access && command.write == u1:1 &&
                                       (command.address == u24:0x300 ||
                                       command.address == u24:0x304 || command.address == u24:0x30c ||
                                       command.address == u24:0x310 || command.address == u24:0x314 ||
                                       command.address == u24:0x318);
            let cache_counter_access = is_register && command.write == u1:0 &&
                                       command.address >= u24:0x3f0 && command.address <= u24:0x3fc;
            let cache_access = is_texture || texture_layout_access || cache_counter_access;
            let cache_request = if texture_access {
                texture_cache::memory_request(texture_command)
            } else if texture_layout_write {
                texture_cache::invalidate_request()
            } else {
                texture_cache::status_request()
            };
            let cache_tok = send_if(join(), cache_request_s, cache_access, cache_request);
            let (cache_tok, cache_response) =
                recv_if(cache_tok, cache_response_r, cache_access, empty_cache_response());
            let triangle_command = is_register && is_triangle_command(command);
            let triangle_tok = send_if(
                join(), triangle_setup_request_s, triangle_command,
                triangle_setup_request(core, command));
            let (shadow_shift, _) = float_shadow_shift(write_address[2+:u8]);
            let shadow_tok = send_if(
                join(), float_shadow_request_s, defer_float,
                FloatShadowRequest { data: command.data, fraction_bits, shadow_shift });
            let (shadow_tok, converted_shadow) = recv_if(
                join(), float_shadow_response_r, state.float_pending,
                FloatShadowResponse { fixed: u32:0, shadow: s50:0 });
            let (palette_write, palette_request) =
                palette_write_request(write_address, command.data);
            let palette_control = EXTERNAL_PALETTE && is_register && command.address == u24:0x3ec;
            let palette_select = palette_control && palette_bank_select(command);
            let palette_request = if palette_select {
                PaletteWrite { select: u1:1, index: u8:0, data: command.data as u24 }
            } else { palette_request };
            let palette_tok = send_if(
                shadow_tok, palette_write_s,
                EXTERNAL_PALETTE && is_register && command.write == u1:1 &&
                (palette_write || palette_select), palette_request);
            let (next_core, register_response) = if is_register {
                step_with_raster_setup(
                    core, command, u1:1, converted_shadow.shadow, converted_shadow.fixed)
            } else {
                (core, CoreResponse { data: u32:0, success: u1:0 })
            };
            let fastfill_command =
                command_valid && command.write == u1:1 && command.address == u24:0x124;
            let next_core = CoreState {
                palette: if EXTERNAL_PALETTE { core.palette } else { next_core.palette },
                de10_mode: u1:1,
                raster: core.raster,
                fastfill: if fastfill_command {
                    begin_fastfill(core)
                } else {
                    fastfill_state::FastfillState { active: u1:0, ..core.fastfill }
                },
                ..next_core
            };
            let response =
                if palette_control {
                    CoreResponse {
                        data: if command.write == u1:0 { u32:0x50414c10 } else { u32:0 },
                        success: command.write == u1:0 || palette_select,
                    }
                } else if command_valid && command.write == u1:0 && command.address == u24:0x3f0 {
                    CoreResponse { data: cache_response.total_hits as u32, success: u1:1 }
                } else if command_valid && command.write == u1:0 && command.address == u24:0x3f4 {
                    CoreResponse { data: cache_response.total_misses as u32, success: u1:1 }
                } else if command_valid && command.write == u1:0 && command.address == u24:0x3f8 {
                    CoreResponse { data: cache_response.all_hit_requests as u32, success: u1:1 }
                } else if command_valid && command.write == u1:0 && command.address == u24:0x3fc {
                    CoreResponse { data: cache_response.missed_requests as u32, success: u1:1 }
                } else if is_framebuffer {
                    CoreResponse {
                        data: framebuffer_response.data,
                        success: framebuffer_response.success,
                    }
                } else if is_texture {
                    CoreResponse {
                        data: cache_response.memory_response.data,
                        success: cache_response.memory_response.success,
                    }
                } else {
                    register_response
                };
            let response_tok = send_if(palette_tok, response_s, response_valid, response);
            let raster_tick = pixels_pending == u6:0 && next_core.raster.active == u1:1 &&
                              next_core.fastfill.active == u1:0;
            let (commands, raster, fastfill, candidate, fill) = if pixels_pending != u6:0 {
                (
                    next_core.commands, next_core.raster, next_core.fastfill,
                    raster_state::PixelCandidate { valid: u1:0, x: s16:0, y: s16:0 },
                    fastfill_state::FastfillWord {
                        valid: u1:0,
                        x: u16:0,
                        y: u16:0,
                        color: u32:0,
                        depth: u32:0,
                        mask: u4:0,
                        color_base: u26:0,
                        aux_base: u26:0,
                        stride: u11:0,
                        rgb_write: u1:0,
                        aux_write: u1:0,
                    },
                )
            } else {
                tick_events<true, SPAN_PREFETCH>(
                    next_core.commands, next_core.raster, next_core.fastfill,
                    if v_retrace_valid { v_retrace } else { u1:0 })
            };
            let ticked_core = CoreState { commands, raster, fastfill, ..next_core };
            let issue_pixel = raster_tick && (candidate.valid == u1:1 || raster.active == u1:0);
            let scanout_tok =
                send_if(join(), scanout_s, EXPORT_SCANOUT, scanout_state(ticked_core));
            let job = PixelJob {
                core: pixel_context(ticked_core, state.base_lod),
                candidate,
                last: raster_tick && raster.active == u1:0,
            };
            let job_tok = send_if(join(), pixel_job_s, issue_pixel, job);
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
                base_lod: state.base_lod,
                pixels_pending: pixels_pending + (issue_pixel as u6),
                triangle_pending: triangle_pending || triangle_command,
                triangle_install: u1:0,
                float_pending: defer_float,
                float_command: if defer_float { command } else { state.float_command },
            }
        }
    }
}

proc block_read_response_streamer {
    response_r: chan<memory::MemoryBlockReadResponse> in;
    word_s: chan<memory::MemoryBlockReadWordResponse> out;

    config(response_r: chan<memory::MemoryBlockReadResponse> in,
           word_s: chan<memory::MemoryBlockReadWordResponse> out) {
        (response_r, word_s)
    }

    init { (bits[2048]:0, u6:0, u1:0, u1:0) }

    next(state: (bits[2048], u6, u1, u1)) {
        let (saved_data, saved_index, saved_success, active) = state;
        let (tok, response) = recv_if(
            join(), response_r, active == u1:0,
            memory::MemoryBlockReadResponse { data: bits[2048]:0, success: u1:0 });
        let data = if active == u1:1 { saved_data } else { response.data };
        let index = if active == u1:1 { saved_index } else { u6:0 };
        let success = if active == u1:1 { saved_success } else { response.success };
        let tok =
            send(tok, word_s, memory::MemoryBlockReadWordResponse { data: data as u32, success });
        if index == u6:63 {
            (bits[2048]:0, u6:0, u1:0, u1:0)
        } else {
            (data >> u8:32, index + u6:1, success, u1:1)
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
           bank0_response_r: chan<texture_cache::PackedBankResponse> in,
           bank1_command_s: chan<texture_cache::PackedBankCommand> out,
           bank1_response_r: chan<texture_cache::PackedBankResponse> in,
           bank2_command_s: chan<texture_cache::PackedBankCommand> out,
           bank2_response_r: chan<texture_cache::PackedBankResponse> in,
           bank3_command_s: chan<texture_cache::PackedBankCommand> out,
           bank3_response_r: chan<texture_cache::PackedBankResponse> in,
           texture_block_s: chan<memory::MemoryBlockWrite> out,
           texture_block_response_r: chan<memory::MemoryBlockResponse> in) {
        let (texture_cache_request_s, texture_cache_request_r) =
            chan<texture_cache::CacheRequest>("texture_cache_request");
        let (texture_cache_response_s, texture_cache_response_r) =
            chan<texture_cache::CacheResponse>("texture_cache_response");
        let (scalar_cache_request_s, scalar_cache_request_r) =
            chan<texture_cache::CacheRequest>("scalar_cache_request");
        let (scalar_cache_response_s, scalar_cache_response_r) =
            chan<texture_cache::CacheResponse>("scalar_cache_response");
        let (span_job_s, _) = chan<FramebufferSpanJob>("span_job_unused");
        let (_, span_row_ready_r) = chan<raster_state::RowSpan>("span_row_ready_unused");
        let (pixel_job_s, pixel_job_r) = chan<PixelJob>("pixel_job");
        let (pixel_work_s, pixel_work_r) = chan<PixelInterpolatedValues>("pixel_work");
        let (pixel_context_s, pixel_context_r) = #[channel(depth=8, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                                 chan<ReadPixelContext>("pixel_context");
        let (pixel_texture_s, pixel_texture_r) = #[channel(depth=32, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                                 chan<TexturePixelJob>("pixel_texture");
        let (pixel_read_job_s, pixel_read_job_r) = chan<ReadPixelJob>("pixel_read_job");
        let (pixel_pending_s, pixel_pending_r) = chan<PendingPixelResult>("pixel_pending");
        let (pixel_result_s, pixel_result_r) = chan<PixelResult>("pixel_result");
        let (pixel_cache_request_s, pixel_cache_request_r) =
            chan<texture_cache::CacheRequest>("pixel_cache_request");
        let (pixel_cache_response_s, pixel_cache_response_r) =
            chan<PixelCacheResponse>("pixel_cache_response");
        let (pixel_cache_lookup_s, pixel_cache_lookup_r) =
            chan<PixelCacheResponse>("pixel_cache_lookup");
        let (triangle_setup_request_s, triangle_setup_request_r) =
            chan<TriangleSetupRequest>("triangle_setup_request");
        let (triangle_setup_response_s, triangle_setup_response_r) =
            chan<TriangleSetupResult>("triangle_setup_response");
        let (float_shadow_request_s, float_shadow_request_r) =
            chan<FloatShadowRequest>("float_shadow_request");
        let (float_shadow_response_s, float_shadow_response_r) =
            chan<FloatShadowResponse>("float_shadow_response");
        let (palette_read_request_s, palette_read_request_r) =
            chan<PaletteReadRequest>("palette_read_request");
        let (palette_response_s, palette_response_r) = chan<PaletteResponse>("palette_response");
        let (palette_write_s, palette_write_r) = chan<PaletteWrite>("palette_write");
        let (performance_s, _) = chan<PerformanceEvent>("performance");
        let (profile_pending_s, _) = chan<u6>("profile_pending");
        let (profile_interpolation_start_s, _) = chan<u1>("profile_interpolation_start");
        let (profile_pipeline_start_s, _) = chan<u1>("profile_pipeline_start");
        let (profile_cache_done_s, _) = chan<u1>("profile_cache_done");
        let (profile_read_done_s, _) = chan<u1>("profile_read_done");
        let (profile_write_done_s, _) = chan<u1>("profile_write_done");
        let (_, v_retrace_r) = chan<u1>("v_retrace");
        let (scanout_s, _) = chan<ScanoutState>("scanout");
        let (texture_fill_word_s, texture_fill_word_r) =
            chan<memory::MemoryBlockReadWordResponse>("texture_fill_word");
        spawn block_read_response_streamer(texture_fill_response_r, texture_fill_word_s);
        spawn texture_cache_arbiter(
            scalar_cache_request_r, scalar_cache_response_s, pixel_cache_request_r,
            pixel_cache_lookup_s, texture_cache_request_s, texture_cache_response_r);
        spawn texture_cache::texture_cache(
            texture_cache_request_r, texture_cache_response_s, texture_command_s,
            texture_response_r, texture_fill_s, texture_fill_word_r, bank0_command_s,
            bank0_response_r, bank1_command_s, bank1_response_r, bank2_command_s, bank2_response_r,
            bank3_command_s, bank3_response_r);
        spawn hosted_triangle_setup(triangle_setup_request_r, triangle_setup_response_s);
        spawn float_shadow_converter(float_shadow_request_r, float_shadow_response_s);
        spawn hosted_pixel_interpolation_stage<false>(
            pixel_job_r, pixel_work_s, pixel_context_s, profile_interpolation_start_s);
        spawn hosted_pixel_texture_requests<false>(
            pixel_work_r, pixel_context_r, pixel_texture_s, pixel_cache_request_s,
            framebuffer_pixel_read_s, profile_pipeline_start_s);
        spawn hosted_pixel_read_pipeline<false>(
            pixel_texture_r, pixel_read_job_s, pixel_cache_response_r, profile_cache_done_s);
        spawn hosted_pixel_write_pipeline<false>(
            pixel_read_job_r, pixel_pending_s, framebuffer_pixel_read_response_r,
            framebuffer_pixel_write_s, profile_read_done_s);
        spawn hosted_pixel_retire_pipeline<false>(
            pixel_pending_r, pixel_result_s, framebuffer_pixel_write_response_r,
            profile_write_done_s);
        spawn palette_lookup(
            pixel_cache_lookup_r, pixel_cache_response_s, palette_read_request_s, palette_response_r);
        spawn hosted_palette_memory(palette_read_request_r, palette_response_s, palette_write_r);
        spawn core_sim_hosted_controller<false, true, false, false>(
            command_r, de10_request_r, response_s, framebuffer_command_s, framebuffer_response_r,
            framebuffer_fastfill_s, framebuffer_fastfill_response_r, span_job_s, span_row_ready_r,
            scalar_cache_request_s, scalar_cache_response_r, pixel_job_s, pixel_result_r,
            triangle_setup_request_s, triangle_setup_response_r, v_retrace_r, scanout_s,
            float_shadow_request_s, float_shadow_response_r, palette_write_s, performance_s,
            profile_pending_s);
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
           color_span_s: chan<FramebufferSpan> out, aux_span_s: chan<FramebufferSpan> out,
           color_read_transfer_s: chan<de10_adapter::MemoryTransfer> out,
           color_read_transfer_response_r: chan<de10_adapter::MemoryTransferResponse> in,
           color_write_transfer_s: chan<de10_adapter::MemoryTransfer> out,
           color_write_transfer_response_r: chan<de10_adapter::MemoryTransferResponse> in,
           aux_read_transfer_s: chan<de10_adapter::MemoryTransfer> out,
           aux_read_transfer_response_r: chan<de10_adapter::MemoryTransferResponse> in,
           aux_write_transfer_s: chan<de10_adapter::MemoryTransfer> out,
           aux_write_transfer_response_r: chan<de10_adapter::MemoryTransferResponse> in,
           texture_transfer_s: chan<de10_adapter::MemoryTransfer> out,
           texture_transfer_response_r: chan<de10_adapter::MemoryTransferResponse> in,
           slow_bank_request_s: chan<texture_cache::BankRequest> out,
           fast_bank_request_s: chan<texture_cache::FastBankRequest> out,
           slow_bank_response_r: chan<texture_cache::BankResponse> in,
           fast_bank_response_r: chan<texture_cache::BankResponse> in,
           tag_read_s: chan<texture_cache::TagRead> out, tag_response_r: chan<u14[4]> in,
           tag_write_s: chan<texture_cache::TagWrite> out,
           palette_read_request_s: chan<PaletteReadRequest> out,
           palette_response_r: chan<PaletteResponse> in, palette_write_s: chan<PaletteWrite> out,
           performance_s: chan<PerformanceEvent> out, profile_pending_s: chan<u6> out,
           profile_interpolation_start_s: chan<u1> out, profile_pipeline_start_s: chan<u1> out,
           profile_cache_done_s: chan<u1> out, profile_read_done_s: chan<u1> out,
           profile_write_done_s: chan<u1> out) {
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
            chan<memory::MemoryBlockReadWordResponse>("texture_fill_response");
        let (texture_cache_request_s, texture_cache_request_r) =
            chan<texture_cache::CacheRequest>("texture_cache_request");
        let (texture_cache_response_s, texture_cache_response_r) =
            chan<texture_cache::CacheResponse>("texture_cache_response");
        let (scalar_cache_request_s, scalar_cache_request_r) =
            chan<texture_cache::CacheRequest>("scalar_cache_request");
        let (scalar_cache_response_s, scalar_cache_response_r) =
            chan<texture_cache::CacheResponse>("scalar_cache_response");
        let (span_job_s, span_job_r) = chan<FramebufferSpanJob>("span_job");
        let (span_row_ready_s, span_row_ready_r) = #[channel(depth=3, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                                   chan<raster_state::RowSpan>("span_row_ready");
        let (pixel_job_s, pixel_job_r) = chan<PixelJob>("pixel_job");
        let (pixel_work_s, pixel_work_r) = chan<PixelInterpolatedValues>("pixel_work");
        let (pixel_context_s, pixel_context_r) = #[channel(depth=8, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                                 chan<ReadPixelContext>("pixel_context");
        let (pixel_texture_s, pixel_texture_r) = #[channel(depth=9, bypass=false, register_push_outputs=true, register_pop_outputs=true)]
                                                 chan<TexturePixelJob>("pixel_texture");
        let (pixel_read_job_s, pixel_read_job_r) = chan<ReadPixelJob>("pixel_read_job");
        let (pixel_pending_s, pixel_pending_r) = chan<PendingPixelResult>("pixel_pending");
        let (pixel_result_s, pixel_result_r) = chan<PixelResult>("pixel_result");
        let (pixel_cache_request_s, pixel_cache_request_r) =
            chan<texture_cache::CacheRequest>("pixel_cache_request");
        let (pixel_cache_response_s, pixel_cache_response_r) =
            chan<PixelCacheResponse>("pixel_cache_response");
        let (pixel_cache_lookup_s, pixel_cache_lookup_r) =
            chan<PixelCacheResponse>("pixel_cache_lookup");
        let (triangle_setup_request_s, triangle_setup_request_r) =
            chan<TriangleSetupRequest>("triangle_setup_request");
        let (triangle_setup_response_s, triangle_setup_response_r) =
            chan<TriangleSetupResult>("triangle_setup_response");
        let (float_shadow_request_s, float_shadow_request_r) =
            chan<FloatShadowRequest>("float_shadow_request");
        let (float_shadow_response_s, float_shadow_response_r) =
            chan<FloatShadowResponse>("float_shadow_response");

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

        let (color_write_command_s, color_write_command_r) =
            chan<memory::MemoryCommand>("color_write_command");
        let (color_write_response_s, color_write_response_r) =
            chan<memory::MemoryResponse>("color_write_response");

        let (aux_write_command_s, aux_write_command_r) =
            chan<memory::MemoryCommand>("aux_write_command");
        let (aux_write_response_s, aux_write_response_r) =
            chan<memory::MemoryResponse>("aux_write_response");

        let (texture_word_transfer_s, texture_word_transfer_r) =
            chan<de10_adapter::MemoryTransfer>("texture_word_transfer");
        let (texture_word_transfer_response_s, texture_word_transfer_response_r) =
            chan<de10_adapter::MemoryTransferResponse>("texture_word_transfer_response");
        let (texture_fill_transfer_s, texture_fill_transfer_r) =
            chan<de10_adapter::MemoryTransfer>("texture_fill_transfer");
        let (texture_fill_transfer_response_s, texture_fill_transfer_response_r) =
            chan<de10_adapter::MemoryTransferResponse>("texture_fill_transfer_response");

        spawn texture_cache_arbiter(
            scalar_cache_request_r, scalar_cache_response_s, pixel_cache_request_r,
            pixel_cache_lookup_s, texture_cache_request_s, texture_cache_response_r);
        spawn texture_cache::texture_cache_vector<true, true, true>(
            texture_cache_request_r, texture_cache_response_s, texture_command_s,
            texture_response_r, texture_fill_s, texture_fill_response_r, slow_bank_request_s,
            fast_bank_request_s, slow_bank_response_r, fast_bank_response_r, tag_read_s,
            tag_response_r, tag_write_s);
        spawn hosted_triangle_setup(triangle_setup_request_r, triangle_setup_response_s);
        spawn palette_lookup(
            pixel_cache_lookup_r, pixel_cache_response_s, palette_read_request_s, palette_response_r);
        spawn core_sim_hosted_controller<true, true, true, true, true>(
            command_r, de10_request_r, response_s, framebuffer_command_s, framebuffer_response_r,
            fastfill_s, fastfill_response_r, span_job_s, span_row_ready_r, scalar_cache_request_s,
            scalar_cache_response_r, pixel_job_s, pixel_result_r, triangle_setup_request_s,
            triangle_setup_response_r, v_retrace_r, scanout_s, float_shadow_request_s,
            float_shadow_response_r, palette_write_s, performance_s, profile_pending_s);

        spawn framebuffer_span_prefetch(span_job_r, aux_span_s, color_span_s, span_row_ready_s);
        spawn float_shadow_converter(float_shadow_request_r, float_shadow_response_s);
        spawn hosted_pixel_interpolation_stage<false>(
            pixel_job_r, pixel_work_s, pixel_context_s, profile_interpolation_start_s);
        spawn hosted_pixel_texture_requests<false>(
            pixel_work_r, pixel_context_r, pixel_texture_s, pixel_cache_request_s, pixel_read_s,
            profile_pipeline_start_s);
        spawn hosted_pixel_read_pipeline<false>(
            pixel_texture_r, pixel_read_job_s, pixel_cache_response_r, profile_cache_done_s);
        spawn hosted_pixel_write_pipeline<false>(
            pixel_read_job_r, pixel_pending_s, pixel_read_response_r, pixel_write_s,
            profile_read_done_s);
        spawn hosted_pixel_posted_retire_pipeline<false>(
            pixel_pending_r, pixel_result_s, profile_write_done_s);
        spawn hosted_pixel_write_response_drain(pixel_write_response_r);
        spawn de10_adapter::pair_to_planes(
            pixel_read_r, pixel_read_response_s, read_aux_s, read_aux_response_r, read_color_s,
            read_color_response_r);
        spawn de10_adapter::pair_to_planes(
            pixel_write_r, pixel_write_response_s, write_color_s, write_color_response_r,
            write_aux_s, write_aux_response_r);
        spawn de10_adapter::pair_to_planes(
            fastfill_r, fastfill_response_s, fill_color_s, fill_color_response_r, fill_aux_s,
            fill_aux_response_r);
        spawn de10_adapter::memory_arbiter2(
            write_color_r, write_color_response_s, fill_color_r, fill_color_response_s,
            color_write_command_s, color_write_response_r);
        spawn de10_adapter::memory_arbiter2(
            write_aux_r, write_aux_response_s, fill_aux_r, fill_aux_response_s, aux_write_command_s,
            aux_write_response_r);

        spawn de10_adapter::word_transfer_adapter(
            framebuffer_command_r, framebuffer_response_s, framebuffer_transfer_s,
            framebuffer_transfer_response_r);
        spawn de10_adapter::word_transfer_adapter(
            read_color_r, read_color_response_s, color_read_transfer_s,
            color_read_transfer_response_r);
        spawn de10_adapter::word_transfer_adapter(
            color_write_command_r, color_write_response_s, color_write_transfer_s,
            color_write_transfer_response_r);
        spawn de10_adapter::word_transfer_adapter(
            read_aux_r, read_aux_response_s, aux_read_transfer_s, aux_read_transfer_response_r);
        spawn de10_adapter::word_transfer_adapter(
            aux_write_command_r, aux_write_response_s, aux_write_transfer_s,
            aux_write_transfer_response_r);
        spawn de10_adapter::word_transfer_adapter(
            texture_command_r, texture_response_s, texture_word_transfer_s,
            texture_word_transfer_response_r);
        spawn de10_adapter::block_read_transfer_adapter<texture_cache::SECTOR_WORDS>(
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
        let (texture_fill_line_response_s, texture_fill_line_response_r) =
            chan<memory::MemoryBlockReadResponse>("texture_fill_line_response");
        let (texture_fill_response_s, texture_fill_response_r) =
            chan<memory::MemoryBlockReadWordResponse>("texture_fill_response");
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
        let (bank0_response_s, bank0_response_r) =
            chan<texture_cache::PackedBankResponse>("bank0_response");
        let (bank1_command_s, bank1_command_r) =
            chan<texture_cache::PackedBankCommand>("bank1_command");
        let (bank1_response_s, bank1_response_r) =
            chan<texture_cache::PackedBankResponse>("bank1_response");
        let (bank2_command_s, bank2_command_r) =
            chan<texture_cache::PackedBankCommand>("bank2_command");
        let (bank2_response_s, bank2_response_r) =
            chan<texture_cache::PackedBankResponse>("bank2_response");
        let (bank3_command_s, bank3_command_r) =
            chan<texture_cache::PackedBankCommand>("bank3_command");
        let (bank3_response_s, bank3_response_r) =
            chan<texture_cache::PackedBankResponse>("bank3_response");
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
            texture_fill_r, texture_fill_line_response_s);
        spawn block_read_response_streamer(texture_fill_line_response_r, texture_fill_response_s);
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
fn lfb_write_maps_pci_coordinates_to_selected_back_buffer() {
    let state = initial_state();
    let registers = update(
        update(
            update(state.registers, register_index(u8:133), u32:0x1251e0a8),
            register_index(u8:134), u32:0x1874b0e2), register_index(u8:69), u32:0x10);
    let state = CoreState { registers, ..state };
    let command = CoreCommand { write: u1:1, address: u24:0x4a73fa, data: u32:0, mask: u4:0xc };
    let mapped = lfb_command(state, command);
    assert_eq(mapped.address, u22:0x3fa7e);
    assert_eq(mapped.mask, u4:0xc);
    let swapped = CoreState {
        commands: control::CommandState {
            swap: control::SwapState { draw_count: u2:1, ..state.commands.swap },
            ..state.commands
        },
        ..state
    };
    assert_eq(lfb_command(swapped, command).address, u22:0x1a27e);
}

#[test]
fn scanout_tracks_displayed_buffer_and_stride() {
    let registers = update(
        update(initial_state().registers, register_index(u8:133), u32:0x000000a0),
        register_index(u8:134), u32:0x0004b000);
    let state = CoreState { registers, ..initial_state() };
    assert_eq(
        scanout_state(state),
        ScanoutState { base: u26:0, pixel_stride: u11:640, displayed: u1:0, swap_count: u2:0 });
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
fn external_palette_write_matches_internal_palette_addressing() {
    let (even_valid, even) = palette_write_request(u10:0x34c, u32:0x92010203);
    let (odd_valid, odd) = palette_write_request(u10:0x350, u32:0x92040506);
    let (disabled, _) = palette_write_request(u10:0x34c, u32:0x12010203);
    assert_eq(even_valid, u1:1);
    assert_eq(even.index, u8:0x24);
    assert_eq(even.data, u24:0x010203);
    assert_eq(odd_valid, u1:1);
    assert_eq(odd.index, u8:0x25);
    assert_eq(odd.data, u24:0x040506);
    assert_eq(disabled, u1:0);
}

#[test]
fn restored_draw_parity_switches_before_vsync() {
    let state = initial_state();
    let registers = update(state.registers, register_index(u8:68), u32:0x00004200);
    let registers = update(registers, register_index(u8:134), u32:0x0004b000);
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
fn pixel_texture_state_preserves_sampler_inputs() {
    let texture = pixel_texture_registers(
        TextureRegisters {
            texture_mode: u32:0x00100a05,
            t_lod: u32:0xf1234567,
            tex_base: u32:0xaa123456,
            tex_base1: u32:0xbb234567,
            tex_base2: u32:0xcc345678,
            tex_base38: u32:0xdd456789,
        });
    assert_eq(texture.texture_mode, u12:0xa05);
    assert_eq(pixel_texture_mode(texture), u32:0x80000a05);
    assert_eq(texture.t_lod, u27:0x1234567);
    assert_eq(
        (texture.tex_base, texture.tex_base1, texture.tex_base2, texture.tex_base38),
        (u24:0x123456, u24:0x234567, u24:0x345678, u24:0x456789));
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
fn step_maps_tmu_register_aliases() {
    let state = initial_state();
    let command =
        CoreCommand { write: u1:1, address: u24:0xb0c, data: u32:0x00012345, mask: u4:0xf };
    let (state, response) = step(state, command);
    assert_eq(response, CoreResponse { data: u32:0, success: u1:1 });
    assert_eq(state.texture.tex_base, u32:0x00012345);
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
    let registers = update(state.registers, register_index(u8:2), u32:8);
    let registers = update(registers, register_index(u8:3), u32:8);
    let registers = update(registers, register_index(u8:4), u32:56);
    let registers = update(registers, register_index(u8:5), u32:8);
    let registers = update(registers, register_index(u8:6), u32:8);
    let registers = update(registers, register_index(u8:7), u32:56);
    let registers = update(registers, register_index(u8:8), u32:0x0ff000);
    let registers = update(registers, register_index(u8:68), u32:0x200);
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
    let registers = update(state.registers, register_index(u8:68), u32:0x600);
    let registers = update(registers, register_index(u8:70), u32:0x00000002);
    let registers = update(registers, register_index(u8:71), u32:0x00000001);
    let registers = update(registers, register_index(u8:76), u32:0x1234);
    let registers = update(registers, register_index(u8:82), u32:0x00ff0000);
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
    let registers = update(state.registers, register_index(u8:11), u32:0x01234000);
    let registers = update(registers, register_index(u8:68), u32:0x00010410);
    let registers = update(registers, register_index(u8:76), u32:0x0000ffff);
    let registers = update(registers, register_index(u8:134), u32:0x00000800);
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
    let registers = update(state.registers, register_index(u8:2), u32:9);
    let registers = update(registers, register_index(u8:8), u32:100);
    let registers = update(registers, register_index(u8:16), u32:16);
    let state = CoreState { registers, ..state };
    let candidate = raster_state::PixelCandidate { valid: u1:1, x: s16:0, y: s16:0 };
    assert_eq(interpolated_parameter(state, candidate, u8:8, u8:16, u8:24), s48:84);
}

#[test]
fn ftriangle_texture_coordinates_round_to_eighteen_fraction_bits() {
    let state = initial_state();
    let registers = update(state.registers, register_index(u8:2), u32:1536);
    let registers = update(registers, register_index(u8:65), u32:0x04000000);
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
    let registers = update(state.registers, register_index(u8:65), u32:0x04000000);
    let state = CoreState { registers, ..state };
    let (state, _) = step(
        state, CoreCommand { write: u1:1, address: u24:0x0d4, data: u32:0x3dcccccd, mask: u4:0xf });
    assert_eq(register_read(state.registers, u8:21), u32:26214);
    assert_eq(state.float_shadows[u8:5], s50:429496736);
    let (state, _) =
        step(state, CoreCommand { write: u1:1, address: u24:0x100, data: u32:0, mask: u4:0xf });
    let candidate = raster_state::PixelCandidate { valid: u1:1, x: s16:100, y: s16:0 };
    assert_eq(interpolated_parameter(state, candidate, u8:13, u8:21, u8:29), s48:2634547);
    let (state, _) =
        step(state, CoreCommand { write: u1:1, address: u24:0x080, data: u32:0, mask: u4:0xf });
    assert_eq(interpolated_parameter(state, candidate, u8:13, u8:21, u8:29), s48:2634507);
}

#[test_proc]
proc triangle_setup_captures_lod_and_handles_degenerate_triangles {
    request_s: chan<TriangleSetupRequest> out;
    response_r: chan<TriangleSetupResult> in;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (request_s, request_r) = chan<TriangleSetupRequest>("request");
        let (response_s, response_r) = chan<TriangleSetupResult>("response");
        spawn hosted_triangle_setup(request_r, response_s);
        (request_s, response_r, terminator)
    }

    init { () }

    next(state: ()) {
        let registers = update(initial_state().registers, register_index(u8:21), u32:1048576);
        let core = CoreState { registers, ..initial_state() };
        let command = CoreCommand { write: u1:1, address: u24:0x100, data: u32:0, mask: u4:0xf };
        let first = triangle_setup_request(core, command);
        let next_core =
            CoreState { registers: update(registers, register_index(u8:21), u32:0), ..core };
        let second = triangle_setup_request(next_core, command);
        assert_eq(first.texture_derivatives, s32[4]:[s32:1048576, s32:0, s32:0, s32:0]);
        let tok = send(join(), request_s, first);
        let (tok, first_result) = recv(tok, response_r);
        assert_eq(first_result.base_lod, s16:256);
        let raster = raster_state::from_triangle_parts(
            first_result.bounds, first_result.edges, first_result.starts);
        assert_eq(raster.active, u1:0);
        let tok = send(tok, request_s, second);
        let (tok, second_result) = recv(tok, response_r);
        assert_eq(second_result.base_lod, s16:0);
        let tok = send(tok, terminator, true);
        state
    }
}

#[test]
fn vertex_preserves_twenty_bit_coordinates() {
    let registers = update(RegisterFile:[u32:0, ...], register_index(u8:2), u32:0x13880);
    assert_eq(vertex(registers, u8:2), s20:0x13880);
}

#[test_proc]
proc hosted_pixel_posted_retire_test {
    pending_s: chan<PendingPixelResult> out;
    result_r: chan<PixelResult> in;
    write_response_s: chan<memory::MemoryResponsePair> out;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (pending_s, pending_r) = chan<PendingPixelResult>("pending");
        let (result_s, result_r) = chan<PixelResult>("result");
        let (write_response_s, write_response_r) =
            chan<memory::MemoryResponsePair>("write_response");
        let (profile_s, _) = chan<u1>("profile");
        spawn hosted_pixel_posted_retire_pipeline<false>(pending_r, result_s, profile_s);
        spawn hosted_pixel_write_response_drain(write_response_r);
        (pending_s, result_r, write_response_s, terminator)
    }

    init { () }

    next(state: ()) {
        let tok = unroll_for! (i, tok): (u6, token) in u6:0..u6:39 {
            let writes = i[0+:u1];
            let event = PerformanceEvent {
                input_pixel: u1:1,
                output_pixel: writes,
                triangle_done: u1:0,
                pipeline_job: u1:1,
                textured: u1:0,
                depth_read: u1:0,
                blend_read: u1:0,
                color_write: writes,
                aux_write: u1:0,
            };
            let tok = send(tok, pending_s, PendingPixelResult { event, write_access: writes });
            let (tok, result) = recv(tok, result_r);
            assert_eq(result, event);
            tok
        }(join());
        let final_event = PerformanceEvent {
            input_pixel: u1:1,
            output_pixel: u1:1,
            triangle_done: u1:1,
            pipeline_job: u1:1,
            textured: u1:0,
            depth_read: u1:0,
            blend_read: u1:0,
            color_write: u1:1,
            aux_write: u1:0,
        };
        let tok =
            send(tok, pending_s, PendingPixelResult { event: final_event, write_access: u1:1 });
        let (tok, result) = recv(tok, result_r);
        assert_eq(result, final_event);
        let tok = unroll_for! (_, tok): (u5, token) in u5:0..u5:20 {
            send(tok, write_response_s, memory::empty_response_pair())
        }(tok);
        send(tok, terminator, true);
    }
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

#[test_proc]
proc hosted_pixel_interpolation_color_width_test {
    job_s: chan<PixelJob> out;
    work_r: chan<PixelInterpolatedValues> in;
    context_r: chan<ReadPixelContext> in;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (job_s, job_r) = chan<PixelJob>("job");
        let (work_s, work_r) = chan<PixelInterpolatedValues>("work");
        let (context_s, context_r) = chan<ReadPixelContext>("context");
        let (profile_s, _) = chan<u1>("profile");
        spawn hosted_pixel_interpolation_stage<false>(job_r, work_s, context_s, profile_s);
        (job_s, work_r, context_r, terminator)
    }

    init { () }

    next(state: ()) {
        let cases = s32[8]:[-1, 0, 4095, 4096, 1048575, 1048576, -2147483648, 2147483647];
        let tok = unroll_for! (i, tok): (u4, token) in u4:0..u4:8 {
            let value = cases[i];
            let registers = for (index, registers): (u8, RegisterFile) in u8:8..u8:16 {
                write_addressed_register_file(registers, index, value as u32, u4:15)
            }(initial_state().registers);
            let core = pixel_context(CoreState { registers, ..initial_state() }, i as s16);
            let candidate =
                raster_state::PixelCandidate { valid: u1:1, x: i as s16, y: -(i as s16) };
            let tok = send(tok, job_s, PixelJob { core, candidate, last: i as u1 });
            let (tok, values) = recv(tok, work_r);
            let (tok, context) = recv(tok, context_r);
            let work = InterpolatedPixelJob { context, values };
            let expected = color_channel(value as s48);
            assert_eq(work.context.base_lod, i as s16);
            assert_eq(work.context.candidate, candidate);
            assert_eq(work.context.last, i as u1);
            assert_eq(work.values.red, expected);
            assert_eq(work.values.green, expected);
            assert_eq(work.values.blue, expected);
            assert_eq(work.values.alpha, expected);
            assert_eq(work.values.z, value as s48);
            assert_eq(work.values.s, value as s48);
            assert_eq(work.values.t, value as s48);
            assert_eq(work.values.w, value as s48);
            tok
        }(join());
        send(tok, terminator, true);
    }
}

#[test_proc]
proc hosted_pixel_blend_read_access_test {
    work_s: chan<PixelInterpolatedValues> out;
    context_s: chan<ReadPixelContext> out;
    job_r: chan<ReadPixelJob> in;
    reads_r: chan<memory::MemoryRequestPair> in;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (work_s, work_r) = chan<PixelInterpolatedValues>("work");
        let (context_s, context_r) = chan<ReadPixelContext>("context");
        let (texture_s, texture_r) = chan<TexturePixelJob>("texture");
        let (job_s, job_r) = chan<ReadPixelJob>("job");
        let (reads_s, reads_r) = chan<memory::MemoryRequestPair>("reads");
        let (cache_s, _) = chan<texture_cache::CacheRequest>("cache");
        let (_, cache_r) = chan<PixelCacheResponse>("cache_response");
        let (start_s, _) = chan<u1>("start");
        let (done_s, _) = chan<u1>("done");
        spawn hosted_pixel_texture_requests<false>(
            work_r, context_r, texture_s, cache_s, reads_s, start_s);
        spawn hosted_pixel_read_pipeline<false>(texture_r, job_s, cache_r, done_s);
        (work_s, context_s, job_r, reads_r, terminator)
    }

    init { () }

    next(state: ()) {
        let tok = unroll_for! (flags, tok): (u5, token) in u5:0..u5:16 {
            let blend = flags[0+:u1];
            let depth = flags[1+:u1];
            let subtract = flags[2+:u1];
            let visible = flags[3+:u1];
            let context = ReadPixelContext {
                alpha_mode: (blend as u32) << u32:4,
                fbz_mode: u21:0x200 | ((depth as u21) << u21:4) | ((subtract as u21) << u21:19),
                candidate: raster_state::PixelCandidate {
                    valid: visible,
                    ..zero!<raster_state::PixelCandidate>()
                },
                ..zero!<ReadPixelContext>()
            };
            let tok = send(tok, context_s, context);
            let tok = send(tok, work_s, zero!<PixelInterpolatedValues>());
            let (tok, job) = recv(tok, job_r);
            let (tok, reads, received) =
                recv_non_blocking(tok, reads_r, zero!<memory::MemoryRequestPair>());
            let expected = if visible { ((blend as u2) << u2:1) | (depth as u2) } else { u2:0 };
            assert_eq(received, expected != u2:0);
            assert_eq(if received { reads.valid } else { u2:0 }, expected);
            assert_eq(job.read_access, expected != u2:0);
            assert_eq(job.blend_access, visible && blend);
            assert_eq(job.depth_access, visible && depth);
            tok
        }(join());
        send(tok, terminator, true);
    }
}

#[test]
fn framebuffer_span_reads_only_for_enabled_blending() {
    for (flags, _): (u4, ()) in u4:0..u4:8 {
        let blend = flags[0+:u1];
        let depth = flags[1+:u1];
        let subtract = flags[2+:u1];
        let core = initial_state();
        let registers =
            write_addressed_register_file(core.registers, u8:67, (blend as u32) << u32:4, u4:15);
        let registers = write_addressed_register_file(
            registers, u8:68,
            u32:0x200 | ((depth as u32) << u32:4) | ((subtract as u32) << u32:19), u4:15);
        let config = framebuffer_span_config(CoreState { registers, ..core });
        assert_eq(config.color_read, blend);
        assert_eq(config.depth_read, depth);
        assert_eq(config.read_enabled, u1:1);
    }(());
}

#[quickcheck(test_count=1024)]
fn opaque_output_does_not_depend_on_framebuffer
    (color: u32, dest0: u24, dest1: u24, fbz_mode: u32, alpha_mode: u32, x: s16, y: s16) -> bool {
    let candidate = raster_state::PixelCandidate { valid: u1:1, x, y };
    let alpha_mode = alpha_mode & !u32:0x10;
    raster_write_color(u32:0, u32:0, fbz_mode, alpha_mode, u1:0, u10:0, candidate, color, dest0) ==
    raster_write_color(u32:0, u32:0, fbz_mode, alpha_mode, u1:0, u10:0, candidate, color, dest1)
}
