#![feature(channel_attributes)]

import banked_memory;
import control;
import dither;
import fastfill_state;
import float_alias;
import interpolator;
import layout;
import memory;
import pixel;
import raster_state;
import tmu;
import texture_sampler;

pub struct CoreCommand { write: u1, address: u24, data: u32, mask: u4 }

pub struct TextureBurst { commands: CoreCommand[32] }

pub struct CoreResponse { data: u32, success: u1 }

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

pub struct CoreState {
    registers: RegisterFile,
    float_shadows: FloatShadowFile,
    raster_uses_float_shadows: u1,
    texture: TextureRegisters,
    texture_layout: bits[432],
    pci_fifo: control::PciFifoState,
    commands: control::CommandState,
    raster: raster_state::RasterState,
    fastfill: fastfill_state::FastfillState,
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
        raster_uses_float_shadows: u1:0,
        texture,
        texture_layout: layout::texture_layout(u19:0, u19:0, u19:0, u19:0, u4:0, u2:0, u1:0, u1:0),
        pci_fifo: control::initial_pci_fifo(),
        commands: control::initial_commands(),
        raster: raster_state::initial_state(),
        fastfill: fastfill_state::initial_state(),
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

fn compute_texture_layout(texture: TextureRegisters) -> bits[432] {
    layout::texture_layout(
        texture.tex_base[0+:u19], texture.tex_base1[0+:u19], texture.tex_base2[0+:u19],
        texture.tex_base38[0+:u19], texture.texture_mode[8+:u4], texture.t_lod[21+:u2],
        texture.t_lod[20+:u1], texture.t_lod[24+:u1])
}

fn status(state: CoreState) -> u32 {
    let displayed = state.commands.swap.swap_count[0+:u1] as u32;
    let pending = state.commands.swap.swaps_pending as u32;
    let retrace = state.commands.swap.retrace_previous as u32;
    u32:0x0ffff03f | (retrace << u32:6) | (displayed << u32:10) | (pending << u32:28)
}

fn vertex(registers: RegisterFile, index: u8) -> s20 { (registers[index] as u20) as s20 }

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
        u8:13 | u8:14 | u8:21 | u8:22 | u8:29 | u8:30 |
        u8:15 | u8:23 | u8:31 => value + (s50:1 << (shift - u6:1)),
        _ => value,
    };
    (rounded >> shift) as s48
}

fn tick_with_events
    (state: CoreState, v_retrace: u1)
    -> (CoreState, raster_state::PixelCandidate, fastfill_state::FastfillWord) {
    let (fastfill, fill_pixel) = fastfill_state::tick(state.fastfill);
    let (raster, candidate) = if state.fastfill.active == u1:1 {
        (state.raster, raster_state::PixelCandidate { valid: u1:0, x: s16:0, y: s16:0 })
    } else {
        raster_state::tick(state.raster)
    };
    (
        CoreState {
            commands: control::command_tick(state.commands, v_retrace),
            raster,
            fastfill,
            ..state
        }, candidate, fill_pixel,
    )
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
    let (commands, raster, fastfill) = for (_, accumulator):
        (u6, (control::CommandState, raster_state::RasterState, fastfill_state::FastfillState))
        in u6:0..u6:31 {
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
    let displayed = state.commands.swap.swap_count[0+:u1];
    let front = if displayed == u1:1 { buffer_offset } else { u26:0 };
    let back = if displayed == u1:1 { u26:0 } else { buffer_offset };
    let color_base = if fbz_mode[14+:u1] == u1:1 { back } else { front };
    fastfill_state::begin(
        clip_x[16+:u10], clip_x[0+:u10], clip_y[16+:u10], clip_y[0+:u10], color[16+:u8],
        color[8+:u8], color[0+:u8], state.registers[u8:76][0+:u16], color_base,
        buffer_offset << u26:1, (tiles as u11) << u11:6, fbz_mode[9+:u1], fbz_mode[10+:u1],
        fbz_mode[17+:u1], state.registers[u8:135][22+:u10], fbz_mode[8+:u1], fbz_mode[11+:u1])
}

pub fn step(state: CoreState, command: CoreCommand) -> (CoreState, CoreResponse) {
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
        let commands = control::command_write(state.commands, write_address, write_data);
        let triangle_command = write_address == u10:0x080 || write_address == u10:0x100;
        let raster = if triangle_command {
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

fn interpolated_parameter
    (state: CoreState, candidate: raster_state::PixelCandidate, start_index: u8, dx_index: u8,
     dy_index: u8) -> s48 {
    let (shadow_shift, has_float_shadow) = float_shadow_shift(start_index);
    let adjust = state.registers[u8:65][26+:u1];
    if state.raster_uses_float_shadows == u1:1 && has_float_shadow == u1:1 {
        let high = interpolator::at_pixel_high(
            state.float_shadows[start_index], state.float_shadows[dx_index],
            state.float_shadows[dy_index], vertex(state.registers, u8:2),
            vertex(state.registers, u8:3), candidate.x, candidate.y, adjust);
        round_float_shadow(start_index, high, shadow_shift)
    } else {
        interpolator::at_pixel(
            (state.registers[start_index] as s32) as s48,
            (state.registers[dx_index] as s32) as s48,
            (state.registers[dy_index] as s32) as s48, vertex(state.registers, u8:2),
            vertex(state.registers, u8:3), candidate.x, candidate.y, adjust)
    }
}

fn interpolated_color
    (state: CoreState, candidate: raster_state::PixelCandidate, start_index: u8, dx_index: u8,
     dy_index: u8) -> u8 {
    color_channel(interpolated_parameter(state, candidate, start_index, dx_index, dy_index))
}

fn texture_request
    (state: CoreState, candidate: raster_state::PixelCandidate) -> texture_sampler::SampleRequest {
    texture_sampler::sample_request(
        interpolated_parameter(state, candidate, u8:13, u8:21, u8:29),
        interpolated_parameter(state, candidate, u8:14, u8:22, u8:30),
        interpolated_parameter(state, candidate, u8:15, u8:23, u8:31),
        state.registers[u8:21] as s32, state.registers[u8:22] as s32, state.registers[u8:29] as s32,
        state.registers[u8:30] as s32, state.texture.texture_mode, state.texture.t_lod,
        state.texture_layout, state.texture.tex_base as u24, state.texture.tex_base1 as u24,
        state.texture.tex_base2 as u24, state.texture.tex_base38 as u24)
}

fn raster_visible(state: CoreState, candidate: raster_state::PixelCandidate) -> bool {
    let mode = state.registers[u8:68];
    let clip_x = state.registers[u8:70];
    let clip_y = state.registers[u8:71];
    let inside_clip = candidate.x >= (clip_x[16+:u10] as s16) &&
                      candidate.x < (clip_x[0+:u10] as s16) &&
                      candidate.y >= (clip_y[16+:u10] as s16) &&
                      candidate.y < (clip_y[0+:u10] as s16);
    candidate.valid == u1:1 && candidate.x >= s16:0 && candidate.y >= s16:0 &&
    (mode[9+:u1] == u1:1 || mode[10+:u1] == u1:1) && (mode[0+:u1] == u1:0 || inside_clip)
}

fn combined_color
    (state: CoreState, candidate: raster_state::PixelCandidate, texture_rgba: u32) -> u32 {
    let path = state.registers[u8:65];
    let iterated_rgb = interpolated_color(state, candidate, u8:8, u8:16, u8:24) ++
                       interpolated_color(state, candidate, u8:9, u8:17, u8:25) ++
                       interpolated_color(state, candidate, u8:10, u8:18, u8:26);
    let iterated_alpha = interpolated_color(state, candidate, u8:12, u8:20, u8:28);
    let iterated_z = interpolated_color(state, candidate, u8:11, u8:19, u8:27);
    let color0 = state.registers[u8:81];
    let color1 = state.registers[u8:82];
    pixel::color_combine(
        iterated_rgb, iterated_alpha, iterated_z, texture_rgba[8+:u24], texture_rgba[0+:u8],
        color0[0+:u24], color0[24+:u8], color1[0+:u24], color1[24+:u8], path[0+:u2], path[4+:u1],
        path[7+:u1], path[8+:u1], path[9+:u1], path[10+:u3], path[13+:u1], path[14+:u2],
        path[16+:u1], path[2+:u2], path[5+:u2], path[17+:u1], path[18+:u1], path[19+:u3],
        path[22+:u1], path[23+:u2], path[25+:u1])
}

fn decode_rgb565(value: u16) -> u24 {
    let red = ((value[11+:u5] as u8) << u8:3) | ((value[11+:u5] as u8) >> u8:2);
    let green = ((value[5+:u6] as u8) << u8:2) | ((value[5+:u6] as u8) >> u8:4);
    let blue = ((value[0+:u5] as u8) << u8:3) | ((value[0+:u5] as u8) >> u8:2);
    red ++ green ++ blue
}

fn framebuffer_y(state: CoreState, candidate: raster_state::PixelCandidate) -> u16 {
    if state.registers[u8:68][17+:u1] == u1:1 {
        (state.registers[u8:135][22+:u10] as u16) - (candidate.y as u16)
    } else {
        candidate.y as u16
    }
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

fn new_depth(state: CoreState, candidate: raster_state::PixelCandidate) -> u16 {
    let mode = state.registers[u8:68];
    let z_depth =
        clamp_depth(interpolated_parameter(state, candidate, u8:11, u8:19, u8:27) >> u8:12);
    let w_depth =
        pixel::fog_w_depth(interpolated_parameter(state, candidate, u8:15, u8:23, u8:31) as s32);
    let base = if mode[3+:u1] == u1:1 { w_depth } else { z_depth };
    let selected = if mode[20+:u1] == u1:1 { state.registers[u8:76][0+:u16] } else { base };
    let biased = clamp_depth((selected as s48) + ((state.registers[u8:76][0+:u16] as s16) as s48));
    if mode[16+:u1] == u1:1 { biased } else { selected }
}

fn aux_write
    (state: CoreState, candidate: raster_state::PixelCandidate, depth: u16, alpha: u8)
    -> memory::MemoryCommand {
    let configured_tiles = state.registers[u8:133][4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let offset = (state.registers[u8:134][11+:u9] as u26) << u26:12;
    let value = if state.registers[u8:68][18+:u1] == u1:1 { alpha ++ alpha } else { depth };
    let packed = pixel::write_pack(
        offset << u26:1, candidate.x as u16, framebuffer_y(state, candidate),
        (tiles as u11) << u11:6, value);
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
    let init1 = state.registers[u8:133];
    let init2 = state.registers[u8:134];
    let fbz_mode = state.registers[u8:68];
    let configured_tiles = init1[4+:u4];
    let tiles = if configured_tiles == u4:0 { u4:10 } else { configured_tiles };
    let stride = (tiles as u11) << u11:6;
    let buffer_offset = (init2[11+:u9] as u26) << u26:12;
    let displayed = state.commands.swap.swap_count[0+:u1];
    let front = if displayed == u1:1 { buffer_offset } else { u26:0 };
    let back = if displayed == u1:1 { u26:0 } else { buffer_offset };
    let base = if fbz_mode[14+:u1] == u1:1 { back } else { front };
    let color = combined_color(state, candidate, texture_rgba);
    let alpha_mode = state.registers[u8:67];
    let blended = pixel::alpha_blend(
        color[8+:u24], dest_rgb, color[8+:u24], color[0+:u8], alpha_mode[8+:u4],
        alpha_mode[12+:u4], alpha_mode[4+:u1]);
    let red = blended[16+:u8];
    let green = blended[8+:u8];
    let blue = blended[0+:u8];
    let output_y = framebuffer_y(state, candidate);
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
    let first = texture_download_address(
        state, (burst.commands[u32:0].address - u24:0x800000) as u23);
    let initial = memory::MemoryBlockWrite {
        address: first[2+:u22], data: u32[32]:[u32:0, ...], masks: u4[32]:[u4:0, ...],
    };
    for (i, accumulator): (u6, (memory::MemoryBlockWrite, u1)) in u6:0..u6:32 {
        let (block, valid) = accumulator;
        let command = burst.commands[i];
        let download = texture_download_address(
            state, (command.address - u24:0x800000) as u23);
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

pub fn queued_step(state: CoreState, command: CoreCommand) -> (CoreState, CoreResponse) {
    let entry =
        control::QueuedWrite { address: command.address, data: command.data, mask: command.mask };
    let (pci_fifo, accepted) = control::pci_fifo_enqueue(state.pci_fifo, entry);
    if accepted == u1:0 {
        (state, CoreResponse { data: u32:0, success: u1:0 })
    } else {
        let (pci_fifo, entry, valid) = control::pci_fifo_dequeue(pci_fifo);
        let queued_command = CoreCommand {
            write: command.write,
            address: entry.address,
            data: entry.data,
            mask: entry.mask,
        };
        let queued_state = CoreState { pci_fifo, ..state };
        if valid == u1:1 {
            step(queued_state, queued_command)
        } else {
            (queued_state, CoreResponse { data: u32:0, success: u1:0 })
        }
    }
}

pub proc core_sim_hosted {
    #[channel_strictness("total_order")]
    command_r: chan<CoreCommand> in;
    #[channel_strictness("total_order")]
    burst_r: chan<TextureBurst> in;
    #[channel_strictness("total_order")]
    response_s: chan<CoreResponse> out;
    #[channel_strictness("total_order")]
    framebuffer_command_s: chan<memory::MemoryCommand> out;
    #[channel_strictness("total_order")]
    framebuffer_response_r: chan<memory::MemoryResponse> in;
    #[channel_strictness("total_order")]
    texture_command_s: chan<memory::MemoryCommand> out;
    #[channel_strictness("total_order")]
    texture_response_r: chan<memory::MemoryResponse> in;
    #[channel_strictness("total_order")]
    texture_block_s: chan<memory::MemoryBlockWrite> out;
    #[channel_strictness("total_order")]
    texture_block_response_r: chan<memory::MemoryBlockResponse> in;

    config(
        command_r: chan<CoreCommand> in, burst_r: chan<TextureBurst> in,
        response_s: chan<CoreResponse> out,
        framebuffer_command_s: chan<memory::MemoryCommand> out,
        framebuffer_response_r: chan<memory::MemoryResponse> in,
        texture_command_s: chan<memory::MemoryCommand> out,
        texture_response_r: chan<memory::MemoryResponse> in,
        texture_block_s: chan<memory::MemoryBlockWrite> out,
        texture_block_response_r: chan<memory::MemoryBlockResponse> in,
    ) {
        (
            command_r, burst_r, response_s, framebuffer_command_s, framebuffer_response_r,
            texture_command_s, texture_response_r, texture_block_s, texture_block_response_r,
        )
    }

    init { initial_state() }

    next(state: CoreState) {
        let default_command = CoreCommand { write: u1:0, address: u24:0, data: u32:0, mask: u4:0 };
        let rendering_busy = state.raster.active == u1:1 || state.fastfill.active == u1:1;
        let (tok, command, command_valid) = if rendering_busy {
            (join(), default_command, false)
        } else {
            recv_non_blocking(join(), command_r, default_command)
        };
        let default_burst = TextureBurst { commands: CoreCommand[32]:[default_command, ...] };
        let (tok, burst, burst_valid) = if rendering_busy || command_valid {
            (tok, default_burst, false)
        } else {
            recv_non_blocking(tok, burst_r, default_burst)
        };
        let (texture_block, texture_block_valid) = texture_block_write(state, burst);
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
        let tok = send_if(tok, texture_command_s, texture_access, texture_command);
        let (tok, texture_response) = recv_if(
            tok, texture_response_r, texture_access,
            memory::MemoryResponse { data: u32:0, success: u1:1 });
        let tok = send_if(
            tok, texture_block_s, burst_valid && texture_block_valid, texture_block);
        let (tok, texture_block_response) = recv_if(
            tok, texture_block_response_r, burst_valid && texture_block_valid,
            memory::MemoryBlockResponse { success: u1:0 });
        let (next_state, register_response) = if command_valid && !is_framebuffer && !is_texture {
            queued_step(state, command)
        } else {
            (state, CoreResponse { data: u32:0, success: u1:0 })
        };
        let response = if is_framebuffer {
            CoreResponse { data: framebuffer_response.data, success: framebuffer_response.success }
        } else if is_texture {
            CoreResponse { data: texture_response.data, success: texture_response.success }
        } else {
            register_response
        };
        let tok = send_if(tok, response_s, command_valid, response);
        let burst_response = CoreResponse {
            data: u32:0,
            success: texture_block_valid && texture_block_response.success,
        };
        let tok = send_if(tok, response_s, burst_valid, burst_response);
        let (next_state, candidate, fill) = tick_with_events(next_state, u1:0);
        let fills = fastfill_state::FastfillWord[8]:[fill, ...];
        let (fastfill, fills) = for (i, accumulator):
            (u4, (fastfill_state::FastfillState, fastfill_state::FastfillWord[8])) in u4:1..u4:8 {
            let (fastfill, fills) = accumulator;
            let (fastfill, next_fill) = fastfill_state::tick(fastfill);
            (fastfill, update(fills, i, next_fill))
        }((next_state.fastfill, fills));
        let next_state = CoreState { fastfill, ..next_state };
        let raster_valid = raster_visible(next_state, candidate);
        let sample = texture_request(next_state, candidate);
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
        let read_mask = texture_sampler::read_mask(sample.addresses);
        let read0 = textured;
        let read1 = textured && read_mask[1+:u1] == u1:1;
        let read2 = textured && read_mask[2+:u1] == u1:1;
        let read3 = textured && read_mask[3+:u1] == u1:1;
        let default_sample = memory::MemoryResponse { data: u32:0, success: u1:0 };
        let tok = send_if(tok, texture_command_s, read0, sample_command0);
        let (tok, sample0) = recv_if(tok, texture_response_r, read0, default_sample);
        let tok = send_if(tok, texture_command_s, read1, sample_command1);
        let (tok, sample1_read) = recv_if(tok, texture_response_r, read1, default_sample);
        let sample1 = if read1 { sample1_read } else { sample0 };
        let tok = send_if(tok, texture_command_s, read2, sample_command2);
        let (tok, sample2_read) = recv_if(tok, texture_response_r, read2, default_sample);
        let sample2 = if read2 {
            sample2_read
        } else if sample_command2.address == sample_command0.address {
            sample0
        } else {
            sample1
        };
        let tok = send_if(tok, texture_command_s, read3, sample_command3);
        let (tok, sample3_read) = recv_if(tok, texture_response_r, read3, default_sample);
        let sample3 = if read3 {
            sample3_read
        } else if sample_command3.address == sample_command0.address {
            sample0
        } else if sample_command3.address == sample_command1.address {
            sample1
        } else {
            sample2
        };
        let texture_rgba = if textured {
            texture_sampler::decode(sample, sample0.data, sample1.data, sample2.data, sample3.data)
        } else {
            u32:0
        };
        let source_color = combined_color(next_state, candidate, texture_rgba);
        let alpha_mode = next_state.registers[u8:67];
        let depth = new_depth(next_state, candidate);
        let aux = aux_write(next_state, candidate, depth, source_color[0+:u8]);
        let depth_access = raster_valid && next_state.registers[u8:68][4+:u1] == u1:1;
        let depth_read =
            memory::MemoryCommand { write: u1:0, address: aux.address, data: u32:0, mask: u4:0 };
        let tok = send_if(tok, framebuffer_command_s, depth_access, depth_read);
        let (tok, depth_response) = recv_if(
            tok, framebuffer_response_r, depth_access,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
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
        let provisional = raster_write(next_state, candidate, texture_rgba, u24:0);
        let blend_access = pixel_valid &&
                           (alpha_mode[4+:u1] == u1:1 ||
                           next_state.registers[u8:68][19+:u1] == u1:1);
        let blend_read = memory::MemoryCommand {
            write: u1:0,
            address: provisional.address,
            data: u32:0,
            mask: u4:0,
        };
        let tok = send_if(tok, framebuffer_command_s, blend_access, blend_read);
        let (tok, blend_response) = recv_if(
            tok, framebuffer_response_r, blend_access,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
        let destination = if provisional.mask == u4:0xc {
            blend_response.data[16+:u16]
        } else {
            blend_response.data[0+:u16]
        };
        let destination_rgb = decode_rgb565(destination);
        let destination_rgb = if next_state.registers[u8:68][19+:u1] == u1:1 {
            dither::subtract_rgb(
                destination_rgb, candidate.x as u16, framebuffer_y(next_state, candidate),
                next_state.registers[u8:68][11+:u1])
        } else {
            destination_rgb
        };
        let color_valid = pixel_valid && next_state.registers[u8:68][9+:u1] == u1:1;
        let tok = send_if(
            tok, framebuffer_command_s, color_valid,
            raster_write(next_state, candidate, texture_rgba, destination_rgb));
        let (tok, _) = recv_if(
            tok, framebuffer_response_r, color_valid,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
        let aux_valid = pixel_valid && next_state.registers[u8:68][10+:u1] == u1:1;
        let tok = send_if(tok, framebuffer_command_s, aux_valid, aux);
        let (tok, _) = recv_if(
            tok, framebuffer_response_r, aux_valid,
            memory::MemoryResponse { data: u32:0, success: u1:0 });
        let tok = unroll_for! (fill, tok): (fastfill_state::FastfillWord, token) in fills {
            let color_fill = fill.valid == u1:1 && fill.rgb_write == u1:1;
            let tok = send_if(
                tok, framebuffer_command_s, color_fill,
                fastfill_write(fill.color_base, fill, fill.color));
            let (tok, _) = recv_if(
                tok, framebuffer_response_r, color_fill,
                memory::MemoryResponse { data: u32:0, success: u1:0 });
            let aux_fill = fill.valid == u1:1 && fill.aux_write == u1:1;
            let tok = send_if(
                tok, framebuffer_command_s, aux_fill,
                fastfill_write(fill.aux_base, fill, fill.depth));
            let (tok, _) = recv_if(
                tok, framebuffer_response_r, aux_fill,
                memory::MemoryResponse { data: u32:0, success: u1:0 });
            tok
        }(tok);
        if burst_valid { advance_burst_ticks(next_state) } else { next_state }
    }
}

pub proc core_sim<FB_WORDS: u32 = {u32:1048576}, TEX_WORDS: u32 = {u32:2097152}> {
    config(
        command_r: chan<CoreCommand> in, burst_r: chan<TextureBurst> in,
        response_s: chan<CoreResponse> out,
    ) {
        let (framebuffer_command_s, framebuffer_command_r) =
            chan<memory::MemoryCommand>("framebuffer_command");
        let (framebuffer_response_s, framebuffer_response_r) =
            chan<memory::MemoryResponse>("framebuffer_response");
        let (texture_command_s, texture_command_r) = chan<memory::MemoryCommand>("texture_command");
        let (texture_response_s, texture_response_r) =
            chan<memory::MemoryResponse>("texture_response");
        let (framebuffer_block_s, framebuffer_block_r) =
            chan<memory::MemoryBlockWrite>("framebuffer_block");
        let (framebuffer_block_response_s, framebuffer_block_response_r) =
            chan<memory::MemoryBlockResponse>("framebuffer_block_response");
        let (texture_block_s, texture_block_r) =
            chan<memory::MemoryBlockWrite>("texture_block");
        let (texture_block_response_s, texture_block_response_r) =
            chan<memory::MemoryBlockResponse>("texture_block_response");
        spawn banked_memory::banked_word_memory<FB_WORDS>(
            framebuffer_command_r, framebuffer_response_s, framebuffer_block_r,
            framebuffer_block_response_s);
        spawn banked_memory::banked_word_memory<TEX_WORDS>(
            texture_command_r, texture_response_s, texture_block_r, texture_block_response_s);
        spawn core_sim_hosted(
            command_r, burst_r, response_s, framebuffer_command_s, framebuffer_response_r,
            texture_command_s, texture_response_r, texture_block_s, texture_block_response_r);
        ()
    }

    init { () }

    next(state: ()) { state }
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
    }(CoreCommand[32]:[
        CoreCommand { write: u1:0, address: u24:0, data: u32:0, mask: u4:0 }, ...
    ]);
    let (block, valid) = texture_block_write(state, TextureBurst { commands });
    assert_eq(valid, u1:1);
    assert_eq(block.address, u22:0);
    assert_eq(block.data[u32:31], u32:31);
    assert_eq(block.masks[u32:0], u4:0x5);
}

#[test]
fn burst_tick_advance_matches_scalar_ticks() {
    let (state, _) = step(
        initial_state(),
        CoreCommand { write: u1:1, address: u24:0x128, data: u32:0, mask: u4:0xf });
    let expected = for (_, state): (u6, CoreState) in u6:0..u6:31 {
        tick(state, u1:0)
    }(state);
    assert_eq(advance_burst_ticks(state), expected);
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
    assert_eq(
        interpolated_parameter(state, candidate, u8:8, u8:16, u8:24),
        s48:84);
}

#[test]
fn ftriangle_texture_coordinates_round_to_eighteen_fraction_bits() {
    let state = initial_state();
    let registers = update(state.registers, u8:2, u32:1536);
    let registers = update(registers, u8:65, u32:0x04000000);
    let state = CoreState { registers, ..state };
    let (state, _) = step(
        state,
        CoreCommand { write: u1:1, address: u24:0x0b4, data: u32:0x43460000, mask: u4:0xf });
    let (state, _) = step(
        state,
        CoreCommand { write: u1:1, address: u24:0x0d4, data: u32:0x3fd80003, mask: u4:0xf });
    let (state, _) = step(
        state,
        CoreCommand { write: u1:1, address: u24:0x100, data: u32:0, mask: u4:0xf });
    let candidate = raster_state::PixelCandidate { valid: u1:1, x: s16:92, y: s16:0 };
    assert_eq(
        interpolated_parameter(state, candidate, u8:13, u8:21, u8:29),
        s48:50356224);
}

#[test]
fn ftriangle_interpolation_uses_float_shadow_precision() {
    let state = initial_state();
    let registers = update(state.registers, u8:65, u32:0x04000000);
    let state = CoreState { registers, ..state };
    let (state, _) = step(
        state,
        CoreCommand { write: u1:1, address: u24:0x0d4, data: u32:0x3dcccccd, mask: u4:0xf });
    assert_eq(state.registers[u8:21], u32:26214);
    assert_eq(state.float_shadows[u8:21], s50:429496736);
    let (state, _) = step(
        state,
        CoreCommand { write: u1:1, address: u24:0x100, data: u32:0, mask: u4:0xf });
    let candidate = raster_state::PixelCandidate { valid: u1:1, x: s16:100, y: s16:0 };
    assert_eq(
        interpolated_parameter(state, candidate, u8:13, u8:21, u8:29),
        s48:2634547);
    let (state, _) = step(
        state,
        CoreCommand { write: u1:1, address: u24:0x080, data: u32:0, mask: u4:0xf });
    assert_eq(
        interpolated_parameter(state, candidate, u8:13, u8:21, u8:29),
        s48:2634507);
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
