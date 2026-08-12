#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
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

struct Response33 {
  bool valid = false;
  uint64_t data = 0;
};

struct FillResponse {
  bool valid = false;
  std::array<uint32_t, 65> data = {};
};

struct PairResponse {
  bool valid = false;
  std::array<uint32_t, 3> data = {};
};

struct PairRequest {
  std::array<uint32_t, 4> data = {};
};

using Bank = std::array<std::array<uint32_t, 64>, 64>;
using BankCommand = uint64_t;

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
  uint32_t data = bank[(address >> 6) & 63u][address & 63u];
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
  apply(&(*bank)[(address >> 6) & 63u][address & 63u],
        (command >> 4) & 0xffffffffu, command & 0xfu);
}

FillResponse fill_response(uint32_t address, const std::vector<uint32_t>& memory) {
  FillResponse response;
  response.valid = true;
  const bool success = address <= memory.size() && memory.size() - address >= 64;
  response.data[0] = success ? 1u | (memory[address] << 1) : 0;
  if (success) {
    for (uint32_t i = 1; i < 64; ++i) {
      response.data[i] = (memory[address + i - 1] >> 31) | (memory[address + i] << 1);
    }
    response.data[64] = memory[address + 63] >> 31;
  }
  return response;
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

FillResponse transfer_response(const std::array<uint32_t, 3>& request,
                               std::vector<uint32_t>* memory) {
  const uint32_t word_count = (request[1] >> 27) | ((request[2] & 3u) << 5);
  const uint64_t command = request[0] |
      (static_cast<uint64_t>(request[1] & 0x07ffffffu) << 32);
  const bool write = (command >> 58) != 0;
  const uint32_t address = (command >> 36) & 0x3fffffu;
  FillResponse response;
  response.valid = true;
  const bool success = word_count != 0 && word_count <= 64 &&
      address <= memory->size() && memory->size() - address >= word_count &&
      (!write || word_count == 1);
  if (success && write) commit_memory(command, memory);
  response.data[0] = success ? 1u | ((*memory)[address] << 1) : 0;
  if (success) {
    for (uint32_t i = 1; i < word_count; ++i) {
      response.data[i] = ((*memory)[address + i - 1] >> 31) |
                         ((*memory)[address + i] << 1);
    }
    response.data[word_count] = (*memory)[address + word_count - 1] >> 31;
  }
  return response;
}

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  const char* path = argc > 1 ? argv[1] : "traces/test28.bin";
  uint32_t framebuffer_words = 0;
  uint32_t texture_words = 0;
  const std::vector<Command> commands =
      load_trace(path, &framebuffer_words, &texture_words);
  std::vector<uint32_t> framebuffer(framebuffer_words);
  std::vector<uint32_t> texture(texture_words);
  std::array<Bank, 4> banks = {};
  std::array<Response33, 4> bank_responses = {};
  std::array<FillResponse, 4> transfer_responses = {};
  uint64_t fills = 0;
  uint64_t framebuffer_commands = 0;
  uint64_t cycles = 0;
  size_t command_index = 0;
  bool request_accepted = false;
  bool wait_response_clear = false;
  std::array<uint32_t, 4> counters = {};
  VCoreSimDe10 dut;
  dut._v_retrace_r = 0;
  dut._v_retrace_r_vld = 0;
  dut._response_s_rdy = 1;
  dut._scanout_s_rdy = 1;
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
    dut._color_transfer_s_rdy = 1;
    dut._aux_transfer_s_rdy = 1;
    dut._texture_transfer_s_rdy = 1;
    for (uint32_t i = 0; i < 65; ++i) {
      dut._framebuffer_transfer_response_r[i] = transfer_responses[0].data[i];
      dut._color_transfer_response_r[i] = transfer_responses[1].data[i];
      dut._aux_transfer_response_r[i] = transfer_responses[2].data[i];
      dut._texture_transfer_response_r[i] = transfer_responses[3].data[i];
    }
    dut._framebuffer_transfer_response_r_vld = transfer_responses[0].valid;
    dut._color_transfer_response_r_vld = transfer_responses[1].valid;
    dut._aux_transfer_response_r_vld = transfer_responses[2].valid;
    dut._texture_transfer_response_r_vld = transfer_responses[3].valid;
    dut._bank0_command_s_rdy = 1;
    dut._bank1_command_s_rdy = 1;
    dut._bank2_command_s_rdy = 1;
    dut._bank3_command_s_rdy = 1;
    dut._bank0_response_r = bank_responses[0].data;
    dut._bank1_response_r = bank_responses[1].data;
    dut._bank2_response_r = bank_responses[2].data;
    dut._bank3_response_r = bank_responses[3].data;
    dut._bank0_response_r_vld = bank_responses[0].valid;
    dut._bank1_response_r_vld = bank_responses[1].valid;
    dut._bank2_response_r_vld = bank_responses[2].valid;
    dut._bank3_response_r_vld = bank_responses[3].valid;
    dut.clk = 0;
    dut.eval();

    const bool request_handshake = drive_request && dut._de10_request_r_rdy;
    const bool register_read = commands[command_index].write == 0 &&
                               commands[command_index].address < 0x400000u;
    const bool response_valid = dut._response_s_vld;
    const uint64_t core_response = dut._response_s;
    const bool response_handshake = request_accepted && response_valid &&
        (!register_read || core_response != 0x3fffe07full);
    const std::array<bool, 4> transfer_command_handshake = {
        static_cast<bool>(dut._framebuffer_transfer_s_vld && dut._framebuffer_transfer_s_rdy),
        static_cast<bool>(dut._color_transfer_s_vld && dut._color_transfer_s_rdy),
        static_cast<bool>(dut._aux_transfer_s_vld && dut._aux_transfer_s_rdy),
        static_cast<bool>(dut._texture_transfer_s_vld && dut._texture_transfer_s_rdy)};
    const std::array<bool, 4> transfer_response_handshake = {
        static_cast<bool>(transfer_responses[0].valid && dut._framebuffer_transfer_response_r_rdy),
        static_cast<bool>(transfer_responses[1].valid && dut._color_transfer_response_r_rdy),
        static_cast<bool>(transfer_responses[2].valid && dut._aux_transfer_response_r_rdy),
        static_cast<bool>(transfer_responses[3].valid && dut._texture_transfer_response_r_rdy)};
    std::array<std::array<uint32_t, 3>, 4> transfer_requests = {};
    if (transfer_command_handshake[0]) {
      std::copy_n(&dut._framebuffer_transfer_s[0], 3, transfer_requests[0].begin());
    }
    if (transfer_command_handshake[1]) {
      std::copy_n(&dut._color_transfer_s[0], 3, transfer_requests[1].begin());
    }
    if (transfer_command_handshake[2]) {
      std::copy_n(&dut._aux_transfer_s[0], 3, transfer_requests[2].begin());
    }
    if (transfer_command_handshake[3]) {
      std::copy_n(&dut._texture_transfer_s[0], 3, transfer_requests[3].begin());
    }
    std::array<bool, 4> bank_command_handshake = {
        static_cast<bool>(dut._bank0_command_s_vld && dut._bank0_command_s_rdy),
        static_cast<bool>(dut._bank1_command_s_vld && dut._bank1_command_s_rdy),
        static_cast<bool>(dut._bank2_command_s_vld && dut._bank2_command_s_rdy),
        static_cast<bool>(dut._bank3_command_s_vld && dut._bank3_command_s_rdy)};
    const std::array<bool, 4> bank_response_handshake = {
        static_cast<bool>(bank_responses[0].valid && dut._bank0_response_r_rdy),
        static_cast<bool>(bank_responses[1].valid && dut._bank1_response_r_rdy),
        static_cast<bool>(bank_responses[2].valid && dut._bank2_response_r_rdy),
        static_cast<bool>(bank_responses[3].valid && dut._bank3_response_r_rdy)};
    std::array<BankCommand, 4> bank_commands = {};
    if (bank_command_handshake[0]) bank_commands[0] = dut._bank0_command_s;
    if (bank_command_handshake[1]) bank_commands[1] = dut._bank1_command_s;
    if (bank_command_handshake[2]) bank_commands[2] = dut._bank2_command_s;
    if (bank_command_handshake[3]) bank_commands[3] = dut._bank3_command_s;
    dut.clk = 1;
    dut.eval();
    ++cycles;

    if (request_handshake) request_accepted = true;
    if (wait_response_clear && !response_valid) wait_response_clear = false;
    for (uint32_t port = 0; port < 4; ++port) {
      if (transfer_response_handshake[port]) transfer_responses[port].valid = false;
      if (transfer_command_handshake[port]) {
        std::vector<uint32_t>* memory = port == 3 ? &texture : &framebuffer;
        transfer_responses[port] = transfer_response(transfer_requests[port], memory);
        const uint32_t word_count = (transfer_requests[port][1] >> 27) |
            ((transfer_requests[port][2] & 3u) << 5);
        if (port == 3 && word_count == 64) ++fills;
        if (port != 3) ++framebuffer_commands;
      }
      if (bank_response_handshake[port]) bank_responses[port].valid = false;
      if (bank_command_handshake[port]) {
        commit_bank(bank_commands[port], &banks[port]);
        bank_responses[port] = Response33{
            true, bank_response(bank_commands[port], banks[port])};
      }
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
  std::printf("rtl cycles: %llu framebuffer commands=%llu\n",
              static_cast<unsigned long long>(cycles),
              static_cast<unsigned long long>(framebuffer_commands));
  std::printf("framebuffer hash: 0x%08x\n", hash);
  std::printf("texture cache taps: hits=%u misses=%u\n", counters[0], counters[1]);
  std::printf("texture cache requests: all-hit=%u missed=%u\n", counters[2], counters[3]);
  std::printf("texture cache line fills: %llu\n", static_cast<unsigned long long>(fills));
  const bool counters_match = counters == std::array<uint32_t, 4>{10208, 32, 2546, 14};
  return command_index == commands.size() && hash == 0xad4f2b82u && counters_match ? 0 : 1;
}
