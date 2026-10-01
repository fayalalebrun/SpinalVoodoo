#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "VCoreSimDe10.h"
#include "verilated.h"

struct Command {
  uint8_t write;
  uint32_t address;
  uint32_t data;
  uint8_t mask;
};


struct PairResponse {
  bool valid = false;
  std::array<uint32_t, 3> data = {};
};

struct PairRequest {
  std::array<uint32_t, 4> data = {};
};

struct PaletteMemoryResponse {
  bool valid = false;
  std::array<uint32_t, 3> data = {};
};

using Bank = std::array<std::array<uint32_t, 64>, 128>;
using BankCommand = uint64_t;
using BankVectorResponse = std::array<uint32_t, 9>;
using Palette = std::array<uint32_t, 256>;

struct LineCacheStats {
  static constexpr uint32_t kLineWords = 64;
  std::vector<uint32_t> tags;
  std::vector<bool> valid;
  uint64_t hits = 0;
  uint64_t misses = 0;

  explicit LineCacheStats(uint32_t lines) : tags(lines), valid(lines) {}

  void read(uint32_t address) {
    const uint32_t tag = address / kLineWords;
    const uint32_t slot = tag % tags.size();
    if (valid[slot] && tags[slot] == tag) {
      ++hits;
    } else {
      ++misses;
      valid[slot] = true;
      tags[slot] = tag;
    }
  }
};

uint64_t extract_bits(const uint32_t* words, uint32_t offset, uint32_t width) {
  uint64_t value = 0;
  for (uint32_t bit = 0; bit < width; ++bit) {
    const uint32_t source = offset + bit;
    value |= static_cast<uint64_t>((words[source / 32] >> (source % 32)) & 1u) << bit;
  }
  return value;
}

void insert_bits(uint32_t* words, uint32_t offset, uint32_t width, uint64_t value) {
  for (uint32_t bit = 0; bit < width; ++bit) {
    const uint32_t target = offset + bit;
    words[target / 32] |= ((value >> bit) & 1u) << (target % 32);
  }
}

uint32_t read_u32(const uint8_t* bytes) {
  return static_cast<uint32_t>(bytes[0]) |
         (static_cast<uint32_t>(bytes[1]) << 8) |
         (static_cast<uint32_t>(bytes[2]) << 16) |
         (static_cast<uint32_t>(bytes[3]) << 24);
}

uint32_t register_address(uint32_t address) {
  uint32_t mapped = address & 0x3fffffu;
  if ((mapped & 0x200000u) != 0) {
    switch (mapped & 0xfffu) {
      case 0x0a4: mapped = 0x0c0; break;
      case 0x0a8: mapped = 0x0e0; break;
      case 0x0ac: mapped = 0x0a4; break;
      case 0x0b0: mapped = 0x0c4; break;
      case 0x0b4: mapped = 0x0e4; break;
      case 0x0b8: mapped = 0x0a8; break;
      case 0x0bc: mapped = 0x0c8; break;
      case 0x0c0: mapped = 0x0e8; break;
      case 0x0c4: mapped = 0x0ac; break;
      case 0x0c8: mapped = 0x0cc; break;
      case 0x0cc: mapped = 0x0ec; break;
      case 0x0d0: mapped = 0x0b0; break;
      case 0x0d4: mapped = 0x0d0; break;
      case 0x0d8: mapped = 0x0f0; break;
      case 0x0dc: mapped = 0x0b4; break;
      case 0x0e0: mapped = 0x0d4; break;
      case 0x0e4: mapped = 0x0f4; break;
      case 0x0e8: mapped = 0x0b8; break;
      case 0x0ec: mapped = 0x0d8; break;
      case 0x0f0: mapped = 0x0f8; break;
      case 0x0f4: mapped = 0x0bc; break;
      case 0x0f8: mapped = 0x0dc; break;
      default: mapped &= ~0x200000u; break;
    }
  }
  return mapped & 0x3fcu;
}

uint32_t byte_mask(uint32_t mask) {
  uint32_t result = 0;
  for (uint32_t byte = 0; byte < 4; ++byte) {
    if ((mask & (1u << byte)) != 0) result |= 0xffu << (byte * 8);
  }
  return result;
}

void apply(uint32_t* target, uint32_t data, uint32_t mask) {
  const uint32_t expanded = byte_mask(mask);
  *target = (*target & ~expanded) | (data & expanded);
}

std::vector<uint8_t> read_file(const char* path) {
  std::ifstream stream(path, std::ios::binary);
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(stream), {});
}

std::vector<Command> load_trace(const char* path, uint32_t* framebuffer_words,
                                uint32_t* texture_words) {
  const std::vector<uint8_t> bytes = read_file(path);
  if (bytes.size() < 64 || read_u32(bytes.data()) != 0x564f4f44u ||
      (bytes.size() - 64) % 24 != 0) {
    std::fprintf(stderr, "invalid trace\n");
    std::exit(1);
  }
  *framebuffer_words = std::max(read_u32(bytes.data() + 12), 1u) * 1024u * 1024u / 4u;
  *texture_words = std::max(read_u32(bytes.data() + 16), 1u) * 1024u * 1024u / 4u;
  std::vector<Command> commands;
  for (size_t offset = 64; offset < bytes.size(); offset += 24) {
    const uint8_t kind = bytes[offset + 8];
    const uint32_t address = read_u32(bytes.data() + offset + 12);
    const uint32_t data = read_u32(bytes.data() + offset + 16);
    const uint32_t count = std::max(read_u32(bytes.data() + offset + 20), 1u);
    for (uint32_t repeat = 0; repeat < count; ++repeat) {
      if (kind == 0) {
        commands.push_back(Command{1, register_address(address), data, 0xf});
      } else if (kind == 1) {
        const bool high = (address & 2u) != 0;
        commands.push_back(Command{1, register_address(address), high ? data << 16 : data,
                                   static_cast<uint8_t>(high ? 0xc : 0x3)});
      } else if (kind == 2) {
        commands.push_back(Command{1, 0x400000u | (address & 0x3fffffu), data, 0xf});
      } else if (kind == 3) {
        const uint32_t target = address & 0x3fffffu;
        const bool high = (target & 2u) != 0;
        commands.push_back(Command{1, 0x400000u | (target & ~3u),
                                   high ? data << 16 : data,
                                   static_cast<uint8_t>(high ? 0xc : 0x3)});
      } else if (kind == 4) {
        commands.push_back(Command{1, 0x800000u | (address & 0x7fffffu), data, 0xf});
      }
    }
  }
  commands.push_back(Command{0, 0x400000u, 0, 0});
  commands.push_back(Command{0, 0x3f0u, 0, 0});
  commands.push_back(Command{0, 0x3f4u, 0, 0});
  commands.push_back(Command{0, 0x3f8u, 0, 0});
  commands.push_back(Command{0, 0x3fcu, 0, 0});
  return commands;
}

void load_state(const char* trace_path, std::vector<Command>* commands,
                std::vector<uint32_t>* framebuffer, std::vector<uint32_t>* texture) {
  std::string path(trace_path);
  const size_t slash = path.find_last_of('/');
  path = (slash == std::string::npos ? std::string() : path.substr(0, slash + 1)) + "state.bin";
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return;
  const std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(stream), {});
  if (bytes.size() < 64 || read_u32(bytes.data()) != 0x41545356u ||
      read_u32(bytes.data() + 4) != 2) {
    std::fprintf(stderr, "invalid state\n");
    std::exit(1);
  }
  const uint32_t framebuffer_size = read_u32(bytes.data() + 16);
  const uint32_t texture_size = read_u32(bytes.data() + 20);
  const uint32_t tmu_count = read_u32(bytes.data() + 24);
  const uint32_t register_count = read_u32(bytes.data() + 28);
  const size_t framebuffer_offset = 64 + static_cast<size_t>(register_count) * 8;
  const size_t texture_offset = framebuffer_offset + framebuffer_size;
  const uint64_t expected_size = texture_offset + static_cast<uint64_t>(texture_size) * tmu_count;
  if (tmu_count != 1 || bytes.size() != expected_size ||
      (framebuffer_size & 3u) != 0 || (texture_size & 3u) != 0) {
    std::fprintf(stderr, "unsupported or truncated state\n");
    std::exit(1);
  }
  std::vector<Command> registers;
  uint32_t fbi_init2 = 0;
  const std::array<uint32_t, 5> skipped = {0x080, 0x100, 0x120, 0x124, 0x128};
  for (uint32_t index = 0; index < register_count; ++index) {
    const uint8_t* entry = bytes.data() + 64 + index * 8;
    const uint32_t raw_address = read_u32(entry);
    const uint32_t address = raw_address & 0x3fcu;
    const uint32_t value = read_u32(entry + 4);
    if (address == 0x218) fbi_init2 = value;
    if (std::find(skipped.begin(), skipped.end(), address) == skipped.end())
      registers.push_back(Command{1, register_address(raw_address), value, 0xf});
  }
  const uint32_t buffer_offset = ((fbi_init2 >> 11) & 0x1ffu) << 12;
  const uint32_t draw_offset = read_u32(bytes.data() + 40);
  if (buffer_offset != 0 && 1 - ((draw_offset / buffer_offset) & 1u) != 0)
    registers.push_back(Command{1, 0x128, 0, 0xf});
  commands->insert(commands->begin(), registers.begin(), registers.end());
  framebuffer->resize(std::max(framebuffer->size(), static_cast<size_t>(framebuffer_size / 4)));
  texture->resize(std::max(texture->size(), static_cast<size_t>(texture_size / 4)));
  for (uint32_t offset = 0; offset < framebuffer_size; offset += 4)
    (*framebuffer)[offset / 4] = read_u32(bytes.data() + framebuffer_offset + offset);
  for (uint32_t offset = 0; offset < texture_size; offset += 4)
    (*texture)[offset / 4] = read_u32(bytes.data() + texture_offset + offset);
}

uint64_t de10_request(const Command& command) {
  return static_cast<uint64_t>(command.data) |
         (static_cast<uint64_t>(command.mask) << 32) |
         (static_cast<uint64_t>(command.write != 0) << 36) |
         (static_cast<uint64_t>(command.write == 0) << 37) |
         (static_cast<uint64_t>(command.address >> 2) << 38);
}


uint64_t pair_command(const PairRequest& request, uint32_t index) {
  const uint32_t start = index * 59;
  uint64_t command = 0;
  for (uint32_t bit = 0; bit < 59; ++bit) {
    const uint32_t source = start + bit;
    command |= static_cast<uint64_t>((request.data[source / 32] >> (source % 32)) & 1u) << bit;
  }
  return command;
}

uint32_t pair_valid(const PairRequest& request) {
  return request.data[3] >> 22;
}

void set_pair_element(std::array<uint32_t, 3>* output, uint32_t index, uint64_t value) {
  const uint32_t start = index * 33;
  const uint32_t word = start / 32;
  const uint32_t shift = start % 32;
  (*output)[word] |= static_cast<uint32_t>(value << shift);
  (*output)[word + 1] |= static_cast<uint32_t>(value >> (32 - shift));
}

uint64_t memory_response(uint64_t command, const std::vector<uint32_t>& memory) {
  const bool write = (command >> 58) != 0;
  const uint32_t address = (command >> 36) & 0x3fffffu;
  const uint32_t value = (command >> 4) & 0xffffffffu;
  const uint32_t mask = command & 0xfu;
  if (address >= memory.size()) return 0;
  uint32_t data = memory[address];
  if (write) apply(&data, value, mask);
  return (static_cast<uint64_t>(data) << 1) | 1u;
}

PairResponse pair_response(const PairRequest& request, const std::vector<uint32_t>& memory) {
  PairResponse response;
  response.valid = true;
  const uint32_t valid = pair_valid(request);
  for (uint32_t i = 0; i < 2; ++i) {
    if ((valid & (1u << i)) != 0) {
      set_pair_element(&response.data, i, memory_response(pair_command(request, i), memory));
    }
  }
  return response;
}

void commit_memory(uint64_t command, std::vector<uint32_t>* memory) {
  if ((command >> 58) == 0) return;
  const uint32_t address = (command >> 36) & 0x3fffffu;
  if (address < memory->size()) {
    apply(&(*memory)[address], (command >> 4) & 0xffffffffu, command & 0xfu);
  }
}

uint64_t bank_response(BankCommand command, const Bank& bank) {
  const bool write = (command >> 58) != 0;
  const uint32_t address = (command >> 36) & 0x3fffffu;
  const uint32_t word = address & 63u;
  const uint32_t slot = (address >> 6) & 63u;
  const uint32_t physical_slot = (address & (1u << 12)) != 0 ? slot + 64u : slot;
  uint32_t data = bank[physical_slot][word];
  if (write) apply(&data, (command >> 4) & 0xffffffffu, command & 0xfu);
  return (static_cast<uint64_t>(data) << 1) | 1u;
}

void commit_pair(const PairRequest& request, std::vector<uint32_t>* memory) {
  const uint32_t valid = pair_valid(request);
  for (uint32_t i = 0; i < 2; ++i) {
    if ((valid & (1u << i)) != 0) commit_memory(pair_command(request, i), memory);
  }
}

void commit_bank(BankCommand command, Bank* bank) {
  if ((command >> 58) == 0) return;
  const uint32_t address = (command >> 36) & 0x3fffffu;
  const uint32_t word = address & 63u;
  const uint32_t slot = (address >> 6) & 63u;
  const bool duplicate = (address & (1u << 16)) != 0;
  if (duplicate) {
    apply(&(*bank)[slot + 64u][word],
          (command >> 4) & 0xffffffffu, command & 0xfu);
    const uint32_t shadow_slot = (address >> 12) & 15u;
    apply(&(*bank)[shadow_slot][word],
          (command >> 4) & 0xffffffffu, command & 0xfu);
  } else {
    apply(&(*bank)[slot][word],
          (command >> 4) & 0xffffffffu, command & 0xfu);
  }
}

void commit_palette(uint32_t write, Palette* palette) {
  (*palette)[write >> 24] = write & 0xffffffu;
}

PaletteMemoryResponse read_palette(uint32_t request, const Palette& palette) {
  const uint32_t entry0 = palette[request >> 24];
  const uint32_t entry1 = palette[(request >> 16) & 0xffu];
  const uint32_t entry2 = palette[(request >> 8) & 0xffu];
  const uint32_t entry3 = palette[request & 0xffu];
  return PaletteMemoryResponse{
      true,
      {entry3 | ((entry2 & 0xffu) << 24),
       (entry2 >> 8) | ((entry1 & 0xffffu) << 16),
       (entry1 >> 16) | (entry0 << 8)}};
}


uint32_t framebuffer_hash(const std::vector<uint32_t>& memory) {
  uint32_t hash = 0x811c9dc5u;
  for (uint32_t y = 0; y < 480; ++y) {
    for (uint32_t x = 0; x < 640; ++x) {
      const uint32_t address = 0x96000u + y * 640u * 2u + x * 2u;
      const uint32_t word = memory[address >> 2];
      const uint16_t pixel = (address & 2u) != 0 ? word >> 16 : word;
      hash = (hash ^ (pixel & 0xffu)) * 0x01000193u;
      hash = (hash ^ (pixel >> 8)) * 0x01000193u;
    }
  }
  return hash;
}

bool write_framebuffer(const char* path, const std::vector<uint32_t>& memory) {
  std::ofstream output(path, std::ios::binary);
  for (uint32_t y = 0; y < 480 && output; ++y) {
    for (uint32_t x = 0; x < 640; ++x) {
      const uint32_t address = 0x96000u + y * 640u * 2u + x * 2u;
      const uint32_t word = memory[address >> 2];
      const uint16_t pixel = (address & 2u) != 0 ? word >> 16 : word;
      output.put(static_cast<char>(pixel));
      output.put(static_cast<char>(pixel >> 8));
    }
  }
  return output.good();
}

std::vector<uint64_t> transfer_response(const std::array<uint32_t, 3>& request,
                                         std::vector<uint32_t>* memory,
                                         bool wrap_burst64 = false) {
  const uint32_t word_count = (request[1] >> 27) | ((request[2] & 3u) << 5);
  const uint64_t command = request[0] |
      (static_cast<uint64_t>(request[1] & 0x07ffffffu) << 32);
  const bool write = (command >> 58) != 0;
  const uint32_t address = (command >> 36) & 0x3fffffu;
  const uint32_t response_count = write || word_count == 0 ? 1 : word_count;
  const bool wrap = wrap_burst64 && !write && word_count == 64 && (address & 63u) != 0;
  const uint32_t response_base = wrap ? address & ~63u : address;
  std::vector<uint64_t> response(response_count);
  const bool success = word_count != 0 && word_count <= 64 &&
      response_base <= memory->size() && memory->size() - response_base >= word_count &&
      (!write || word_count == 1);
  if (success && write) commit_memory(command, memory);
  if (success) {
    for (uint32_t i = 0; i < response_count; ++i) {
      const uint32_t response_address = wrap ?
          response_base + ((address + i) & 63u) : address + i;
      response[i] = (static_cast<uint64_t>((*memory)[response_address]) << 1) | 1u;
    }
  }
  return response;
}

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  const char* path = argc > 1 ? argv[1] : "traces/test28.bin";
  uint32_t framebuffer_words = 0;
  uint32_t texture_words = 0;
  std::vector<Command> commands = load_trace(path, &framebuffer_words, &texture_words);
  std::vector<uint32_t> framebuffer(framebuffer_words);
  std::vector<uint32_t> texture(texture_words);
  load_state(path, &commands, &framebuffer, &texture);
  std::array<std::array<LineCacheStats, 8>, 2> read_caches = {{
      {LineCacheStats(4), LineCacheStats(8), LineCacheStats(16), LineCacheStats(32),
       LineCacheStats(64), LineCacheStats(128), LineCacheStats(256), LineCacheStats(512)},
      {LineCacheStats(4), LineCacheStats(8), LineCacheStats(16), LineCacheStats(32),
       LineCacheStats(64), LineCacheStats(128), LineCacheStats(256), LineCacheStats(512)}}};
  std::array<Bank, 4> banks = {};
  std::deque<BankVectorResponse> slow_bank_responses;
  std::deque<BankVectorResponse> fast_bank_responses;
  std::array<uint16_t, 64> primary_tags = {};
  std::array<uint16_t, 16> shadow_tags = {};
  std::deque<uint64_t> tag_responses;
  std::array<Palette, 16> palettes = {};
  uint32_t palette_bank = 0;
  std::deque<PaletteMemoryResponse> palette_responses;
  std::array<std::deque<uint64_t>, 6> transfer_responses;
  std::deque<uint64_t> texture_response_cycles;
  const uint64_t texture_first_beat_delay =
      argc > 3 ? std::strtoull(argv[3], nullptr, 0) : 0;
  const uint64_t texture_wrap_delay =
      argc > 4 ? std::strtoull(argv[4], nullptr, 0) : 0;
  uint64_t fills = 0;
  std::array<uint64_t, 2> tag_reads = {};
  std::array<uint64_t, 2> tag_writes = {};
  uint64_t framebuffer_commands = 0;
  std::array<uint64_t, 6> transfer_commands = {};
  uint64_t cycles = 0;
  uint64_t render_cycles = 0;
  uint64_t pixels_in = 0;
  uint64_t pixels_out = 0;
  uint64_t texture_beats = 0;
  uint64_t texture_gap1 = 0;
  uint64_t texture_gap2 = 0;
  uint64_t texture_gap_more = 0;
  uint64_t texture_last_cycle = 0;
  uint64_t texture_scheduled_gap = 0;
  uint64_t texture_scheduled_gap_commands = 0;
  uint64_t fast_reads = 0;
  uint64_t slow_fast_same_address = 0;
  uint64_t slow_writes = 0;
  uint64_t slow_write_gap1 = 0;
  uint64_t slow_write_gap2 = 0;
  uint64_t slow_write_gap_more = 0;
  uint64_t slow_write_last_cycle = 0;
  uint32_t max_pending = 0;
  std::array<uint64_t, 5> profile_events = {};
  std::array<uint64_t, 5> profile_gap1 = {};
  std::array<uint64_t, 5> profile_gap2 = {};
  std::array<uint64_t, 5> profile_gap_more = {};
  std::array<uint64_t, 5> profile_extra_cycles = {};
  std::array<uint64_t, 5> profile_last_cycle = {};
  std::array<uint64_t, 64> pending_histogram = {};
  std::array<uint64_t, 9> performance_counts = {};
  std::array<std::deque<uint64_t>, 4> latency_starts;
  std::array<uint64_t, 4> latency_sum = {};
  std::array<uint64_t, 4> latency_max = {};
  std::array<uint64_t, 4> latency_count = {};
  uint64_t pipeline_inflight = 0;
  uint64_t render_issue_cycles = 0;
  uint64_t render_no_issue_high = 0;
  uint64_t render_no_issue_low = 0;
  uint64_t render_no_issue_empty = 0;
  uint64_t render_no_issue_fill = 0;
  uint32_t texture_fill_remaining = 0;
  uint32_t texture_fill_words = 0;
  bool triangle_active = false;
  size_t command_index = 0;
  bool request_accepted = false;
  bool wait_response_clear = false;
  std::array<uint32_t, 4> counters = {};
  VCoreSimDe10 dut;
  dut._v_retrace_r = 0;
  dut._v_retrace_r_vld = 0;
  dut._response_s_rdy = 1;
  dut._scanout_s_rdy = 1;
  dut._performance_s_rdy = 1;
  dut._profile_pending_s_rdy = 1;
  dut._profile_interpolation_start_s_rdy = 1;
  dut._profile_pipeline_start_s_rdy = 1;
  dut._profile_cache_done_s_rdy = 1;
  dut._profile_read_done_s_rdy = 1;
  dut._profile_write_done_s_rdy = 1;
  for (int cycle = 0; cycle < 5; ++cycle) {
    dut.rst = 1;
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
  }
  dut.rst = 0;
  const uint64_t max_cycles = argc > 2 ? std::strtoull(argv[2], nullptr, 0) : 20000000;
  while (command_index < commands.size() && cycles < max_cycles) {
    const bool drive_request = !request_accepted && !wait_response_clear;
    dut._de10_request_r = de10_request(commands[command_index]);
    dut._de10_request_r_vld = drive_request;
    dut._response_s_rdy = 1;
    dut._framebuffer_transfer_s_rdy = 1;
    dut._color_span_s_rdy = 1;
    dut._aux_span_s_rdy = 1;
    dut._color_read_transfer_s_rdy = 1;
    dut._color_write_transfer_s_rdy = 1;
    dut._aux_read_transfer_s_rdy = 1;
    dut._aux_write_transfer_s_rdy = 1;
    dut._texture_transfer_s_rdy = 1;
    dut._performance_s_rdy = 1;
    dut._profile_pending_s_rdy = 1;
    dut._profile_interpolation_start_s_rdy = 1;
    dut._profile_pipeline_start_s_rdy = 1;
    dut._profile_cache_done_s_rdy = 1;
    dut._profile_read_done_s_rdy = 1;
    dut._profile_write_done_s_rdy = 1;
    dut._framebuffer_transfer_response_r =
        transfer_responses[0].empty() ? 0 : transfer_responses[0].front();
    dut._color_read_transfer_response_r =
        transfer_responses[1].empty() ? 0 : transfer_responses[1].front();
    dut._color_write_transfer_response_r =
        transfer_responses[2].empty() ? 0 : transfer_responses[2].front();
    dut._aux_read_transfer_response_r =
        transfer_responses[3].empty() ? 0 : transfer_responses[3].front();
    dut._aux_write_transfer_response_r =
        transfer_responses[4].empty() ? 0 : transfer_responses[4].front();
    const bool texture_response_available = !transfer_responses[5].empty() &&
        texture_response_cycles.front() <= cycles;
    dut._texture_transfer_response_r =
        texture_response_available ? transfer_responses[5].front() : 0;
    dut._framebuffer_transfer_response_r_vld = !transfer_responses[0].empty();
    dut._color_read_transfer_response_r_vld = !transfer_responses[1].empty();
    dut._color_write_transfer_response_r_vld = !transfer_responses[2].empty();
    dut._aux_read_transfer_response_r_vld = !transfer_responses[3].empty();
    dut._aux_write_transfer_response_r_vld = !transfer_responses[4].empty();
    dut._texture_transfer_response_r_vld = texture_response_available;
    dut._fast_bank_request_s_rdy = 1;
    dut._slow_bank_request_s_rdy = 0;
    for (size_t word = 0; word < BankVectorResponse{}.size(); ++word) {
      dut._slow_bank_response_r[word] =
          slow_bank_responses.empty() ? 0 : slow_bank_responses.front()[word];
      dut._fast_bank_response_r[word] =
          fast_bank_responses.empty() ? 0 : fast_bank_responses.front()[word];
    }
    dut._slow_bank_response_r_vld = !slow_bank_responses.empty();
    dut._fast_bank_response_r_vld = !fast_bank_responses.empty();
    dut._tag_read_s_rdy = tag_responses.size() < 2;
    dut._tag_write_s_rdy = 1;
    dut._tag_response_r = tag_responses.empty() ? 0 : tag_responses.front();
    dut._tag_response_r_vld = !tag_responses.empty();
    dut._palette_read_request_s_rdy = 1;
    dut._palette_write_s_rdy = 1;
    dut._palette_response_r[0] = palette_responses.empty() ? 0 : palette_responses.front().data[0];
    dut._palette_response_r[1] = palette_responses.empty() ? 0 : palette_responses.front().data[1];
    dut._palette_response_r[2] = palette_responses.empty() ? 0 : palette_responses.front().data[2];
    dut._palette_response_r_vld = !palette_responses.empty();
    dut.clk = 0;
    dut.eval();
    dut._slow_bank_request_s_rdy = 1;
    dut.eval();

    if (dut._tag_response_r_vld && dut._tag_response_r_rdy) tag_responses.pop_front();
    if (dut._tag_write_s_vld && dut._tag_write_s_rdy) {
      ++tag_writes[dut._tag_write_s & 1];
      const uint32_t address = dut._tag_write_s >> 1;
      const uint32_t line = address >> 8;
      const uint32_t pslot = (line ^ (line >> 6) ^ (line >> 12)) & 63;
      const uint32_t sslot = (line ^ (line >> 4) ^ (line >> 8) ^ (line >> 12) ^ (line >> 16)) & 15;
      if (dut._tag_write_s & 1) primary_tags[pslot] = address >> 14;
      shadow_tags[sslot] = address >> 12;
    }
    if (dut._tag_read_s_vld && dut._tag_read_s_rdy) {
      ++tag_reads[dut._tag_read_s & 1];
      uint64_t tags = 0;
      for (uint32_t tap = 0; tap < 4; ++tap) {
        const uint32_t slot = (dut._tag_read_s >> (1 + 6*tap)) & 63;
        const uint64_t tag = (dut._tag_read_s & 1) ? primary_tags[slot] : shadow_tags[slot & 15];
        tags |= tag << (14*tap);
      }
      tag_responses.push_back(tags);
    }
    const bool request_handshake = drive_request && dut._de10_request_r_rdy;
    const bool triangle_start = request_handshake && commands[command_index].write != 0 &&
        (commands[command_index].address == 0x080u || commands[command_index].address == 0x100u);
    const bool performance_handshake = dut._performance_s_vld && dut._performance_s_rdy;
    const uint32_t performance_event = performance_handshake ? dut._performance_s : 0;
    const std::array<bool, 5> profile_valid = {
        static_cast<bool>(dut._profile_interpolation_start_s_vld),
        static_cast<bool>(dut._profile_pipeline_start_s_vld),
        static_cast<bool>(dut._profile_cache_done_s_vld),
        static_cast<bool>(dut._profile_read_done_s_vld),
        static_cast<bool>(dut._profile_write_done_s_vld)};
    for (size_t stage = 0; stage < profile_valid.size(); ++stage) {
      if (profile_valid[stage]) {
        if (profile_events[stage] != 0) {
          const uint64_t gap = cycles - profile_last_cycle[stage];
          if (gap == 1) ++profile_gap1[stage];
          else if (gap == 2) ++profile_gap2[stage];
          else ++profile_gap_more[stage];
          profile_extra_cycles[stage] += gap - 1;
        }
        ++profile_events[stage];
        profile_last_cycle[stage] = cycles;
      }
    }
    if (profile_valid[0]) latency_starts[0].push_back(cycles);
    if (profile_valid[1]) latency_starts[1].push_back(cycles);
    if (profile_valid[2]) latency_starts[2].push_back(cycles);
    if (profile_valid[3]) latency_starts[3].push_back(cycles);
    const std::array<bool, 4> latency_ends = {
        profile_valid[4], profile_valid[2], profile_valid[3], profile_valid[4]};
    for (size_t path_index = 0; path_index < latency_ends.size(); ++path_index) {
      if (latency_ends[path_index] && !latency_starts[path_index].empty()) {
        const uint64_t latency = cycles - latency_starts[path_index].front();
        latency_starts[path_index].pop_front();
        latency_sum[path_index] += latency;
        latency_max[path_index] = std::max(latency_max[path_index], latency);
        ++latency_count[path_index];
      }
    }
    if (dut._profile_pending_s_vld) {
      const uint32_t pending = dut._profile_pending_s;
      max_pending = std::max(max_pending, pending);
      if (pending < pending_histogram.size()) ++pending_histogram[pending];
    }
    const bool response_valid = dut._response_s_vld;
    const uint64_t core_response = dut._response_s;
    if (response_valid && !request_accepted && !request_handshake) {
      std::fprintf(stderr, "unsolicited response at cycle %llu\n",
                   static_cast<unsigned long long>(cycles));
      return 1;
    }
    const bool response_handshake = (request_accepted || request_handshake) && response_valid;

    const std::array<bool, 6> transfer_command_handshake = {
        static_cast<bool>(dut._framebuffer_transfer_s_vld && dut._framebuffer_transfer_s_rdy),
        static_cast<bool>(dut._color_read_transfer_s_vld && dut._color_read_transfer_s_rdy),
        static_cast<bool>(dut._color_write_transfer_s_vld && dut._color_write_transfer_s_rdy),
        static_cast<bool>(dut._aux_read_transfer_s_vld && dut._aux_read_transfer_s_rdy),
        static_cast<bool>(dut._aux_write_transfer_s_vld && dut._aux_write_transfer_s_rdy),
        static_cast<bool>(dut._texture_transfer_s_vld && dut._texture_transfer_s_rdy)};
    const std::array<bool, 6> transfer_response_handshake = {
        static_cast<bool>(!transfer_responses[0].empty() &&
                          dut._framebuffer_transfer_response_r_rdy),
        static_cast<bool>(!transfer_responses[1].empty() &&
                          dut._color_read_transfer_response_r_rdy),
        static_cast<bool>(!transfer_responses[2].empty() &&
                          dut._color_write_transfer_response_r_rdy),
        static_cast<bool>(!transfer_responses[3].empty() &&
                          dut._aux_read_transfer_response_r_rdy),
        static_cast<bool>(!transfer_responses[4].empty() &&
                          dut._aux_write_transfer_response_r_rdy),
        static_cast<bool>(texture_response_available &&
                          dut._texture_transfer_response_r_rdy)};

    std::array<std::array<uint32_t, 3>, 6> transfer_requests = {};
    if (transfer_command_handshake[0])
      std::copy_n(&dut._framebuffer_transfer_s[0], 3, transfer_requests[0].begin());
    if (transfer_command_handshake[1])
      std::copy_n(&dut._color_read_transfer_s[0], 3, transfer_requests[1].begin());
    if (transfer_command_handshake[2])
      std::copy_n(&dut._color_write_transfer_s[0], 3, transfer_requests[2].begin());
    if (transfer_command_handshake[3])
      std::copy_n(&dut._aux_read_transfer_s[0], 3, transfer_requests[3].begin());
    if (transfer_command_handshake[4])
      std::copy_n(&dut._aux_write_transfer_s[0], 3, transfer_requests[4].begin());
    if (transfer_command_handshake[5])
      std::copy_n(&dut._texture_transfer_s[0], 3, transfer_requests[5].begin());
    const bool fast_bank_request_handshake =
        dut._fast_bank_request_s_vld && dut._fast_bank_request_s_rdy;
    const bool slow_bank_request_handshake =
        dut._slow_bank_request_s_vld && dut._slow_bank_request_s_rdy;

    const bool slow_bank_response_handshake =
        !slow_bank_responses.empty() && dut._slow_bank_response_r_rdy;
    const bool fast_bank_response_handshake =
        !fast_bank_responses.empty() && dut._fast_bank_response_r_rdy;
    std::array<uint32_t, 4> fast_bank_request = {};
    std::array<uint32_t, 9> slow_bank_request = {};
    if (fast_bank_request_handshake)
      std::copy_n(&dut._fast_bank_request_s[0], fast_bank_request.size(), fast_bank_request.begin());
    if (slow_bank_request_handshake)
      std::copy_n(&dut._slow_bank_request_s[0], slow_bank_request.size(), slow_bank_request.begin());
    const bool palette_read_handshake =
        dut._palette_read_request_s_vld && dut._palette_read_request_s_rdy;
    const bool palette_write_handshake =
        dut._palette_write_s_vld && dut._palette_write_s_rdy;
    const bool palette_response_handshake =
        !palette_responses.empty() && dut._palette_response_r_rdy;
    const uint32_t palette_read_request =
        palette_read_handshake ? dut._palette_read_request_s : 0;
    const uint64_t palette_write = palette_write_handshake ? dut._palette_write_s : 0;
    dut.clk = 1;
    dut.eval();
    ++cycles;

    if (triangle_active) {
      ++render_cycles;
      if (profile_valid[0]) ++render_issue_cycles;
      else if (pipeline_inflight >= 40) ++render_no_issue_high;
      else if (pipeline_inflight != 0) ++render_no_issue_low;
      else ++render_no_issue_empty;
      if (!profile_valid[0] && texture_fill_remaining != 0) ++render_no_issue_fill;
    }
    if (profile_valid[0]) ++pipeline_inflight;
    if (performance_handshake && (performance_event & 0x020u) != 0) --pipeline_inflight;
    if (transfer_response_handshake[5]) {
      const uint64_t gap = cycles - texture_last_cycle;
      ++texture_beats;
      texture_gap1 += gap == 1;
      texture_gap2 += gap == 2;
      texture_gap_more += gap > 2;
      texture_last_cycle = cycles;
      if (texture_fill_remaining != 0) --texture_fill_remaining;
    }
    if (performance_handshake) {
      for (uint32_t bit = 0; bit < performance_counts.size(); ++bit)
        performance_counts[bit] += (performance_event >> bit) & 1u;
    }
    if (performance_handshake && (performance_event & 0x100u) != 0) ++pixels_in;
    if (performance_handshake && (performance_event & 0x080u) != 0) ++pixels_out;
    if (triangle_start) triangle_active = true;
    if (performance_handshake && (performance_event & 0x040u) != 0 && !triangle_start) {
      triangle_active = false;
    }

    if (request_handshake) request_accepted = true;
    if (wait_response_clear && !response_valid) wait_response_clear = false;
    if (palette_response_handshake) palette_responses.pop_front();
    if (palette_read_handshake)
      palette_responses.push_back(read_palette(palette_read_request, palettes[palette_bank]));
    if (palette_write_handshake) {
      if (palette_write >> 32) {
        if (static_cast<uint32_t>(palette_write) >= palettes.size()) {
          std::fprintf(stderr, "invalid palette bank selector\n");
          return 1;
        }
        palette_bank = static_cast<uint32_t>(palette_write);
      }
      else commit_palette(static_cast<uint32_t>(palette_write), &palettes[palette_bank]);
    }
    for (uint32_t port = 0; port < 6; ++port) {
      if (transfer_response_handshake[port]) {
        transfer_responses[port].pop_front();
        if (port == 5) texture_response_cycles.pop_front();
      }
      if (transfer_command_handshake[port]) {
        ++transfer_commands[port];
        std::vector<uint32_t>* memory = port == 5 ? &texture : &framebuffer;
        uint64_t texture_response_cycle = cycles + texture_first_beat_delay;
        if (port == 5) {
          const uint64_t previous_cycle = texture_response_cycles.empty() ?
              cycles : texture_response_cycles.back() + 1;
          if (texture_response_cycle > previous_cycle) {
            texture_scheduled_gap += texture_response_cycle - previous_cycle;
            ++texture_scheduled_gap_commands;
          }
          texture_response_cycle = std::max(texture_response_cycle, previous_cycle);
        }
        const uint32_t word_count = (transfer_requests[port][1] >> 27) |
            ((transfer_requests[port][2] & 3u) << 5);
        const uint64_t transfer_command = transfer_requests[port][0] |
            (static_cast<uint64_t>(transfer_requests[port][1] & 0x07ffffffu) << 32);
        const uint32_t transfer_address = (transfer_command >> 36) & 0x3fffffu;
        const uint32_t wrap_offset = port == 5 && word_count == 64 ? transfer_address & 63u : 0;
        uint32_t beat_index = 0;
        for (uint64_t beat : transfer_response(transfer_requests[port], memory, port == 5)) {
          if (wrap_offset != 0 && beat_index == 64 - wrap_offset)
            texture_response_cycle += texture_wrap_delay;
          transfer_responses[port].push_back(beat);
          if (port == 5) texture_response_cycles.push_back(texture_response_cycle++);
          ++beat_index;
        }
        if (port == 1 || port == 3) {
          for (LineCacheStats& cache : read_caches[port == 1 ? 0 : 1])
            cache.read(transfer_address);
        }
        if (port == 5 && ((transfer_command >> 58) & 1u) == 0 && word_count > 1) {
          if (texture_fill_words != 0 && texture_fill_words != word_count) {
            std::fprintf(stderr, "inconsistent texture refill sizes\n");
            return 1;
          }
          ++fills;
          texture_fill_words = word_count;
          texture_fill_remaining = word_count;
        }
        if (port != 5) ++framebuffer_commands;
      }
    }
    if (slow_bank_response_handshake) slow_bank_responses.pop_front();
    if (fast_bank_response_handshake) fast_bank_responses.pop_front();
    if (fast_bank_request_handshake) ++fast_reads;
    const auto process_slow_bank_request = [&](const std::array<uint32_t, 9>& request) {
      BankVectorResponse response = {};
      response[0] = request[0] & 0x001fffffu;
       const uint32_t valid = extract_bits(request.data(), 257, 4);
       bool writes = false;
       for (uint32_t bank = 0; bank < 4; ++bank) {
        if ((valid & (1u << bank)) != 0) {
          const BankCommand command = extract_bits(request.data(), 21 + bank * 59, 59);
          writes |= (command >> 58) != 0;
          commit_bank(command, &banks[bank]);
          const uint64_t first = bank_response(command, banks[bank]) >> 1;
          uint64_t data = first;

          insert_bits(response.data(), 21 + bank * 65, 1, 1);
          insert_bits(response.data(), 22 + bank * 65, 64, data);
        }
      }
      slow_bank_responses.push_back(response);
      if (writes) {
        const uint64_t gap = cycles - slow_write_last_cycle;
        ++slow_writes;
        slow_write_gap1 += gap == 1;
        slow_write_gap2 += gap == 2;
        slow_write_gap_more += gap > 2;
        slow_write_last_cycle = cycles;
      }
    };
    if (slow_bank_request_handshake && fast_bank_request_handshake) {
      const uint32_t slow_valid = extract_bits(slow_bank_request.data(), 257, 4);
      const uint32_t fast_selected = extract_bits(fast_bank_request.data(), 1, 4);
      for (uint32_t bank = 0; bank < 4; ++bank) {
        if ((slow_valid & (1u << bank)) == 0) continue;
        const BankCommand command = extract_bits(slow_bank_request.data(), 21 + bank * 59, 59);
        if ((command >> 58) == 0) continue;
        const uint32_t slow_address = (command >> 36) & 0xfffu;
        bool collision = false;
        for (uint32_t tap = 0; tap < 4; ++tap) {
          const uint32_t location = extract_bits(fast_bank_request.data(), 5 + tap * 4, 4);
          const uint32_t entry = extract_bits(fast_bank_request.data(), 21 + tap * 22, 22);
          collision |= (fast_selected & (1u << tap)) != 0 &&
                       ((location >> 1) & 3u) == bank && (entry & 0xfffu) == slow_address;
        }
        slow_fast_same_address += collision;
      }
    }
    if (slow_bank_request_handshake) process_slow_bank_request(slow_bank_request);
    if (fast_bank_request_handshake) {
      BankVectorResponse response = {};
      const uint32_t selected = extract_bits(fast_bank_request.data(), 1, 4);
      response[0] = (fast_bank_request[0] & 1u) | (selected << 1);
      std::array<uint32_t, 4> entries = {};
      std::array<uint32_t, 4> locations = {};
      for (uint32_t tap = 0; tap < 4; ++tap) {
        locations[tap] = extract_bits(fast_bank_request.data(), 5 + tap * 4, 4);
        entries[tap] = extract_bits(fast_bank_request.data(), 21 + tap * 22, 22);
      }
      for (uint32_t bank = 0; bank < 4; ++bank) {
        uint32_t first = 4;
        uint32_t second = 4;
        for (uint32_t tap = 0; tap < 4; ++tap) {
          if ((selected & (1u << tap)) == 0 || ((locations[tap] >> 1) & 3u) != bank) continue;
          if (first == 4) first = tap;
          else if (entries[tap] != entries[first] && second == 4) second = tap;
        }
        if (first == 4) continue;
        const BankCommand first_command = static_cast<uint64_t>(entries[first]) << 36;
        const uint64_t first_word = bank_response(first_command, banks[bank]) >> 1;
        uint64_t data = first_word;
        if (second != 4) {
          const BankCommand second_command = static_cast<uint64_t>(entries[second]) << 36;
          data |= (bank_response(second_command, banks[bank]) >> 1) << 32;
        }
        insert_bits(response.data(), 21 + bank * 65, 1, 1);
        insert_bits(response.data(), 22 + bank * 65, 64, data);
        for (uint32_t tap = 0; tap < 4; ++tap) {
          if ((selected & (1u << tap)) == 0 || ((locations[tap] >> 1) & 3u) != bank) continue;
          const uint32_t position = (entries[tap] != entries[first] ? 2u : 0u) |
                                    (locations[tap] & 1u);
          const uint32_t packed_location = (locations[tap] & 6u) |
                                           ((position & 2u) << 2) | (position & 1u);
          insert_bits(response.data(), 5 + tap * 4, 4, packed_location);
        }
      }
      fast_bank_responses.push_back(response);
    }
    if (response_handshake) {
      if ((core_response & 1u) == 0) {
        std::fprintf(stderr, "operation %zu failed at cycle %llu\n", command_index,
                     static_cast<unsigned long long>(cycles));
        return 1;
      }
      if (command_index + 4 >= commands.size()) {
        counters[command_index + 4 - commands.size()] =
            static_cast<uint32_t>(core_response >> 1);
      }
      ++command_index;
      request_accepted = false;
      wait_response_clear = true;
    }
  }
  dut.final();
  const uint32_t hash = framebuffer_hash(framebuffer);
  std::printf("commands: %zu completed=%zu request_accepted=%u next=0x%06x\n",
              commands.size() - 5, command_index, request_accepted,
              command_index < commands.size() ? commands[command_index].address : 0);
  std::printf("rtl cycles: %llu render cycles=%llu pixels in=%llu pixels out=%llu c/pixel=%.3f\n",
              static_cast<unsigned long long>(cycles),
              static_cast<unsigned long long>(render_cycles),
              static_cast<unsigned long long>(pixels_in),
              static_cast<unsigned long long>(pixels_out),
              pixels_in == 0 ? 0.0 : static_cast<double>(render_cycles) / pixels_in);
  std::printf("framebuffer commands: %llu max pending: %u\n",
              static_cast<unsigned long long>(framebuffer_commands), max_pending);
  std::printf("transfer commands: fb=%llu cr=%llu cw=%llu ar=%llu aw=%llu tex=%llu\n",
              static_cast<unsigned long long>(transfer_commands[0]),
              static_cast<unsigned long long>(transfer_commands[1]),
              static_cast<unsigned long long>(transfer_commands[2]),
              static_cast<unsigned long long>(transfer_commands[3]),
              static_cast<unsigned long long>(transfer_commands[4]),
              static_cast<unsigned long long>(transfer_commands[5]));

  std::printf("framebuffer hash: 0x%08x\n", hash);
  if (argc > 5 && !write_framebuffer(argv[5], framebuffer)) {
    std::fprintf(stderr, "cannot write framebuffer %s\n", argv[5]);
    return 1;
  }
  if (argc > 6) {
    std::ofstream output(argv[6], std::ios::binary);
    output.write(reinterpret_cast<const char*>(framebuffer.data()),
                 framebuffer.size() * sizeof(framebuffer[0]));
    output.close();
    if (!output) {
      std::fprintf(stderr, "cannot write framebuffer memory %s\n", argv[6]);
      return 1;
    }
  }
  for (size_t plane = 0; plane < read_caches.size(); ++plane) {
    std::printf("%s read line misses:", plane == 0 ? "color" : "aux");
    for (const LineCacheStats& cache : read_caches[plane])
      std::printf(" %zu=%llu", cache.tags.size(), static_cast<unsigned long long>(cache.misses));
    std::printf("\n");
  }
  std::printf("texture cache taps: hits=%u misses=%u\n", counters[0], counters[1]);
  std::printf("texture cache requests: all-hit=%u missed=%u\n", counters[2], counters[3]);
  std::printf("texture cache line fills: %llu\n", static_cast<unsigned long long>(fills));
  std::printf("texture beats: %llu gap1=%llu gap2=%llu gap>2=%llu scheduled-gap=%llu/%llu\n",
              static_cast<unsigned long long>(texture_beats),
              static_cast<unsigned long long>(texture_gap1),
              static_cast<unsigned long long>(texture_gap2),
              static_cast<unsigned long long>(texture_gap_more),
              static_cast<unsigned long long>(texture_scheduled_gap),
              static_cast<unsigned long long>(texture_scheduled_gap_commands));
  std::printf("fast reads: %llu same-address slow/fast: %llu\n",
              static_cast<unsigned long long>(fast_reads),
              static_cast<unsigned long long>(slow_fast_same_address));
  std::printf("slow writes: %llu gap1=%llu gap2=%llu gap>2=%llu\n",
              static_cast<unsigned long long>(slow_writes),
              static_cast<unsigned long long>(slow_write_gap1),
              static_cast<unsigned long long>(slow_write_gap2),
              static_cast<unsigned long long>(slow_write_gap_more));

  for (size_t stage = 0; stage < profile_events.size(); ++stage) {
    std::printf("profile stage %zu: events=%llu gap1=%llu gap2=%llu gap>2=%llu extra=%llu\n", stage,
                static_cast<unsigned long long>(profile_events[stage]),
                static_cast<unsigned long long>(profile_gap1[stage]),
                static_cast<unsigned long long>(profile_gap2[stage]),
                static_cast<unsigned long long>(profile_gap_more[stage]),
                static_cast<unsigned long long>(profile_extra_cycles[stage]));
  }
  std::printf("pending retirements: p47=%llu p40-46=%llu p32-39=%llu p0-31=%llu\n",
              static_cast<unsigned long long>(pending_histogram[47]),
              static_cast<unsigned long long>(std::accumulate(
                  pending_histogram.begin() + 40, pending_histogram.begin() + 47, uint64_t{0})),
              static_cast<unsigned long long>(std::accumulate(
                  pending_histogram.begin() + 32, pending_histogram.begin() + 40, uint64_t{0})),
              static_cast<unsigned long long>(std::accumulate(
                  pending_histogram.begin(), pending_histogram.begin() + 32, uint64_t{0})));
  std::printf("render cycles: issue=%llu no-issue-high=%llu low=%llu empty=%llu fill=%llu\n",
              static_cast<unsigned long long>(render_issue_cycles),
              static_cast<unsigned long long>(render_no_issue_high),
              static_cast<unsigned long long>(render_no_issue_low),
              static_cast<unsigned long long>(render_no_issue_empty),
              static_cast<unsigned long long>(render_no_issue_fill));
  std::printf("latency avg/max: total=%.1f/%llu cache=%.1f/%llu read=%.1f/%llu write=%.1f/%llu\n",
              static_cast<double>(latency_sum[0]) / latency_count[0],
              static_cast<unsigned long long>(latency_max[0]),
              static_cast<double>(latency_sum[1]) / latency_count[1],
              static_cast<unsigned long long>(latency_max[1]),
              static_cast<double>(latency_sum[2]) / latency_count[2],
              static_cast<unsigned long long>(latency_max[2]),
              static_cast<double>(latency_sum[3]) / latency_count[3],
              static_cast<unsigned long long>(latency_max[3]));
  std::printf("events: triangles=%llu jobs=%llu textured=%llu depth-read=%llu blend-read=%llu color-write=%llu aux-write=%llu\n",
              static_cast<unsigned long long>(performance_counts[6]),
              static_cast<unsigned long long>(performance_counts[5]),
              static_cast<unsigned long long>(performance_counts[4]),
              static_cast<unsigned long long>(performance_counts[3]),
              static_cast<unsigned long long>(performance_counts[2]),
              static_cast<unsigned long long>(performance_counts[1]),
              static_cast<unsigned long long>(performance_counts[0]));
  std::printf("cache tag traffic: small reads=%llu fills=%llu large reads=%llu fills=%llu\n",
              static_cast<unsigned long long>(tag_reads[0]),
              static_cast<unsigned long long>(tag_writes[0]),
              static_cast<unsigned long long>(tag_reads[1]),
              static_cast<unsigned long long>(tag_writes[1]));
  const bool tomb = std::string(path).find("tomb/trace.bin") != std::string::npos;
  const bool full_line_counters_match = tomb
      ? counters == std::array<uint32_t, 4>{1571516, 2428, 393606, 840}
      : counters == std::array<uint32_t, 4>{10208, 32, 2546, 14};
  const bool sector_counters_match = tomb
      ? counters == std::array<uint32_t, 4>{1570692, 3252, 393224, 1222}
      : counters == std::array<uint32_t, 4>{10186, 54, 2535, 25};
  const bool counters_match = (texture_fill_words == 64 && full_line_counters_match) ||
      (texture_fill_words == 32 && sector_counters_match);
  const bool performance_valid = tomb
      ? pixels_in == 404863 && pixels_out == 347726
      : pixels_in == 48298 && pixels_out == 48298;
  const bool fill_stream_valid = texture_wrap_delay != 0 ||
      (slow_write_gap1 + fills == slow_writes && slow_write_gap2 == 0 &&
       slow_write_gap_more == fills);
  std::printf("texture fill words: %u\n", texture_fill_words);
  std::printf("final drain: complete=%d triangle-active=%d fill-stream-valid=%d\n",
              command_index == commands.size(), triangle_active, fill_stream_valid);
  const uint32_t expected_hash = tomb ? 0x78bb4ef2u : 0xad4f2b82u;
  return command_index == commands.size() && hash == expected_hash && counters_match &&
      performance_valid && fill_stream_valid && !triangle_active ? 0 : 1;
}
