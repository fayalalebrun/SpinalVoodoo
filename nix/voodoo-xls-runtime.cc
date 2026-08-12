#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <string_view>
#include <utility>
#include <vector>

#include <cerrno>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "xls-bridge-protocol.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/status/status_macros.h"
#include "xls/interpreter/channel_queue.h"
#include "xls/interpreter/serial_proc_runtime.h"
#include "xls/ir/bits.h"
#include "xls/ir/package.h"
#include "xls/ir/value.h"
#include "xls/jit/aot_entrypoint.pb.h"
#include "xls/jit/function_base_jit.h"
#include "xls/jit/jit_channel_queue.h"
#include "xls/jit/jit_proc_runtime.h"
#include "xls/jit/type_layout.h"
#include "xls/public/ir_parser.h"

namespace {

struct Command {
  uint8_t write;
  uint32_t address;
  uint32_t data;
  uint8_t mask;
};

struct Operation {
  bool burst;
  std::array<Command, 32> commands;
};

struct Trace {
  std::vector<Command> commands;
  uint32_t framebuffer_words;
  uint32_t texture_words;
};

struct StateSnapshot {
  std::vector<Command> registers;
  std::vector<uint8_t> framebuffer;
  std::vector<uint8_t> texture;
  uint32_t swap_count;
  uint32_t width;
  uint32_t height;
};

struct FramebufferConfig {
  uint32_t base;
  uint32_t stride;
};

using PackedBank = std::array<std::array<uint8_t, 64 * sizeof(uint32_t)>, 64>;

struct Options {
  std::string ir;
  std::string trace;
  std::string state;
  std::string aot_proto;
  std::string aot_library;
  std::string framebuffer_output;
  std::string server_socket;
  uint32_t width = 640;
  uint32_t height = 480;
  uint64_t max_ticks = 2000000;
  std::optional<uint32_t> expected_hash;
  std::optional<double> minimum_cache_hit_rate;
  bool de10 = false;
};

uint32_t ReadU32(const uint8_t* bytes) {
  return static_cast<uint32_t>(bytes[0]) |
         (static_cast<uint32_t>(bytes[1]) << 8) |
         (static_cast<uint32_t>(bytes[2]) << 16) |
         (static_cast<uint32_t>(bytes[3]) << 24);
}

uint32_t RegisterAddress(bool de10, uint32_t address) {
  uint32_t mapped = address & 0x3fffffu;
  if (de10 && (mapped & 0x200000u) != 0) {
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

uint32_t ByteMask(uint8_t mask) {
  uint32_t expanded = 0;
  for (uint32_t byte = 0; byte < 4; ++byte) {
    if ((mask & (1u << byte)) != 0) {
      expanded |= 0xffu << (byte * 8);
    }
  }
  return expanded;
}

void Apply(uint32_t* value, uint32_t data, uint8_t mask) {
  const uint32_t expanded = ByteMask(mask);
  *value = (*value & ~expanded) | (data & expanded);
}

absl::StatusOr<std::vector<uint8_t>> ReadBytes(const std::string& path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream) {
    return absl::NotFoundError("cannot open " + path);
  }
  const std::streamsize size = stream.tellg();
  if (size < 0) {
    return absl::InvalidArgumentError("cannot determine size of " + path);
  }
  std::vector<uint8_t> bytes(static_cast<size_t>(size));
  stream.seekg(0);
  if (!bytes.empty() && !stream.read(reinterpret_cast<char*>(bytes.data()), size)) {
    return absl::DataLossError("cannot read " + path);
  }
  return bytes;
}

absl::StatusOr<StateSnapshot> LoadState(const std::string& path, bool de10) {
  XLS_ASSIGN_OR_RETURN(std::vector<uint8_t> bytes, ReadBytes(path));
  if (bytes.size() < 64 || ReadU32(bytes.data()) != 0x41545356u ||
      ReadU32(bytes.data() + 4) != 2) {
    return absl::InvalidArgumentError("invalid Voodoo state: " + path);
  }
  const uint32_t framebuffer_size = ReadU32(bytes.data() + 16);
  const uint32_t texture_size = ReadU32(bytes.data() + 20);
  const uint32_t tmu_count = ReadU32(bytes.data() + 24);
  const uint32_t register_count = ReadU32(bytes.data() + 28);
  const uint64_t expected_size = 64ull + static_cast<uint64_t>(register_count) * 8ull +
      framebuffer_size + static_cast<uint64_t>(texture_size) * tmu_count;
  if (tmu_count != 1 || bytes.size() != expected_size ||
      (framebuffer_size & 3u) != 0 || (texture_size & 3u) != 0 ||
      framebuffer_size > 16u * 1024u * 1024u || texture_size > 16u * 1024u * 1024u) {
    return absl::InvalidArgumentError("unsupported or truncated Voodoo state: " + path);
  }
  StateSnapshot state;
  uint32_t fbi_init2 = 0;
  const std::array<uint32_t, 5> skipped = {0x080, 0x100, 0x120, 0x124, 0x128};
  for (uint32_t index = 0; index < register_count; ++index) {
    const uint8_t* entry = bytes.data() + 64 + index * 8;
    const uint32_t raw_address = ReadU32(entry);
    const uint32_t register_address = raw_address & 0x3fcu;
    const uint32_t value = ReadU32(entry + 4);
    if (register_address == 0x218) {
      fbi_init2 = value;
    }
    if (std::find(skipped.begin(), skipped.end(), register_address) == skipped.end()) {
      state.registers.push_back(Command{1, RegisterAddress(de10, raw_address), value, 0xf});
    }
  }
  const size_t framebuffer_offset = 64 + static_cast<size_t>(register_count) * 8;
  const size_t texture_offset = framebuffer_offset + framebuffer_size;
  state.framebuffer.assign(bytes.begin() + framebuffer_offset, bytes.begin() + texture_offset);
  state.texture.assign(bytes.begin() + texture_offset, bytes.end());
  const uint32_t buffer_offset = ((fbi_init2 >> 11) & 0x1ffu) << 12;
  const uint32_t draw_offset = ReadU32(bytes.data() + 40);
  state.swap_count = buffer_offset == 0 ? 0 : 1 - ((draw_offset / buffer_offset) & 1u);
  state.width = ReadU32(bytes.data() + 48);
  state.height = ReadU32(bytes.data() + 52);
  return state;
}

absl::StatusOr<Trace> LoadTrace(const std::string& path, bool de10) {
  XLS_ASSIGN_OR_RETURN(std::vector<uint8_t> bytes, ReadBytes(path));
  if (bytes.size() < 64 || ReadU32(bytes.data()) != 0x564f4f44u) {
    return absl::InvalidArgumentError("invalid Voodoo trace: " + path);
  }
  if ((bytes.size() - 64) % 24 != 0) {
    return absl::InvalidArgumentError("truncated Voodoo trace: " + path);
  }
  const uint32_t entry_count = static_cast<uint32_t>((bytes.size() - 64) / 24);
  const uint32_t declared_count = ReadU32(bytes.data() + 44);
  if (declared_count != 0 && declared_count != entry_count) {
    return absl::InvalidArgumentError("Voodoo trace entry count mismatch");
  }
  Trace trace;
  const uint32_t framebuffer_mb = ReadU32(bytes.data() + 12);
  const uint32_t texture_mb = ReadU32(bytes.data() + 16);
  if (framebuffer_mb > 16 || texture_mb > 16) {
    return absl::InvalidArgumentError("Voodoo trace memory size exceeds the CoreSim address space");
  }
  trace.framebuffer_words = std::max(framebuffer_mb, 1u) * 1024u * 1024u / 4u;
  trace.texture_words = std::max(texture_mb, 1u) * 1024u * 1024u / 4u;
  for (uint32_t index = 0; index < entry_count; ++index) {
    const uint8_t* entry = bytes.data() + 64 + index * 24;
    const uint8_t kind = entry[8];
    const uint32_t address = ReadU32(entry + 12);
    const uint32_t data = ReadU32(entry + 16);
    const uint32_t count = std::max(ReadU32(entry + 20), 1u);
    if (count > 10000000u - std::min<size_t>(trace.commands.size(), 10000000u)) {
      return absl::ResourceExhaustedError("Voodoo trace expands beyond 10 million commands");
    }
    for (uint32_t repeat = 0; repeat < count; ++repeat) {
      switch (kind) {
        case 0:
          trace.commands.push_back(Command{1, RegisterAddress(de10, address), data, 0xf});
          break;
        case 1: {
          const bool high = (address & 2u) != 0;
          trace.commands.push_back(Command{
               1, RegisterAddress(de10, address), high ? data << 16 : data,
              static_cast<uint8_t>(high ? 0xc : 0x3)});
          break;
        }
        case 2:
          trace.commands.push_back(Command{1, 0x400000u | (address & 0x3fffffu), data, 0xf});
          break;
        case 3: {
          const uint32_t target = address & 0x3fffffu;
          const bool high = (target & 2u) != 0;
          trace.commands.push_back(Command{
              1, 0x400000u | (target & ~3u), high ? data << 16 : data,
              static_cast<uint8_t>(high ? 0xc : 0x3)});
          break;
        }
        case 4:
          trace.commands.push_back(Command{1, 0x800000u | (address & 0x7fffffu), data, 0xf});
          break;
        default:
          break;
      }
    }
  }
  return trace;
}

std::array<std::pair<uint32_t, uint32_t>, 9> TextureLayout(
    const std::array<uint32_t, 256>& registers) {
  const std::array<uint32_t, 4> bases = {
      (registers[0x30c / 4] & 0x7ffffu) << 3,
      (registers[0x310 / 4] & 0x7ffffu) << 3,
      (registers[0x314 / 4] & 0x7ffffu) << 3,
      (registers[0x318 / 4] & 0x7ffffu) << 3};
  const uint32_t texture_mode = registers[0x300 / 4];
  const uint32_t t_lod = registers[0x304 / 4];
  const uint32_t aspect = (t_lod >> 21) & 3u;
  const bool multibase = ((t_lod >> 24) & 1u) != 0;
  uint32_t offset = 0;
  uint32_t tail = 0;
  std::array<std::pair<uint32_t, uint32_t>, 9> result;
  for (uint32_t lod = 0; lod < 9; ++lod) {
    const uint32_t wide = lod < 8 ? 8 - lod : 0;
    const uint32_t narrow = lod < 8 - aspect ? 8 - aspect - lod : 0;
    uint32_t base = bases[0] + offset;
    if (multibase && lod == 1) {
      base = bases[1];
    } else if (multibase && lod == 2) {
      base = bases[2];
    } else if (multibase && lod >= 3) {
      base = bases[3] + tail;
    }
    const uint32_t size = 1u << (wide + narrow + (((texture_mode >> 8) & 15u) >= 8));
    result[lod] = {base & 0x3fffffu, ((t_lod >> 20) & 1u) != 0 ? wide : narrow};
    offset = (offset + size) & 0x3fffffu;
    if (lod >= 3) {
      tail = (tail + size) & 0x3fffffu;
    }
  }
  return result;
}

std::optional<uint32_t> TextureAddress(
    const std::array<uint32_t, 256>& registers, uint32_t pci_address) {
  const uint32_t lod = (pci_address >> 17) & 15u;
  if (((pci_address >> 21) & 3u) != 0 || lod > 8) {
    return std::nullopt;
  }
  const uint32_t texture_mode = registers[0x300 / 4];
  const bool is_16_bit = ((texture_mode >> 8) & 15u) >= 8;
  const bool sequential = ((texture_mode >> 31) & 1u) != 0;
  const auto [base, shift] = TextureLayout(registers)[lod];
  const uint32_t t = (pci_address >> 9) & 0xffu;
  uint32_t s;
  if (is_16_bit) {
    s = (pci_address >> 1) & 0xfeu;
  } else if (sequential) {
    s = pci_address & 0xfcu;
  } else {
    s = (pci_address >> 1) & 0xfcu;
  }
  return (base + (s << is_16_bit) + (t << (shift + is_16_bit))) & 0x3ffffffu;
}

std::vector<Operation> Coalesce(const std::vector<Command>& commands,
                                bool enable_bursts) {
  std::array<uint32_t, 256> registers = {};
  std::vector<Operation> operations;
  size_t index = 0;
  while (index < commands.size()) {
    bool burst = enable_bursts && index + 32 <= commands.size();
    std::array<Command, 32> chunk = {};
    std::array<uint32_t, 32> mapped = {};
    for (size_t offset = 0; offset < 32 && burst; ++offset) {
      chunk[offset] = commands[index + offset];
      const Command& command = chunk[offset];
      const std::optional<uint32_t> address =
          command.write != 0 && command.address >= 0x800000u
              ? TextureAddress(registers, command.address & 0x7fffffu)
              : std::nullopt;
      if (!address.has_value()) {
        burst = false;
      } else {
        mapped[offset] = *address;
      }
    }
    if (burst) {
      burst = mapped[0] % 128 == 0;
      for (size_t offset = 1; offset < 32 && burst; ++offset) {
        burst = mapped[offset] == mapped[0] + offset * 4;
      }
    }
    if (burst) {
      operations.push_back(Operation{true, chunk});
      index += 32;
      continue;
    }
    Operation operation = {};
    operation.commands[0] = commands[index];
    operations.push_back(operation);
    const Command& command = commands[index];
    if (command.write != 0 && command.address < 0x400) {
      Apply(&registers[(command.address & 0x3fcu) / 4], command.data, command.mask);
    }
    ++index;
  }
  return operations;
}

FramebufferConfig GetFramebufferConfig(const std::vector<Command>& commands,
                                       size_t first_trace_command,
                                       uint32_t displayed) {
  std::array<uint32_t, 256> registers = {};
  std::optional<uint32_t> presented;
  for (size_t index = 0; index < commands.size(); ++index) {
    const Command& command = commands[index];
    if (command.write == 0 || command.address >= 0x400) {
      continue;
    }
    if (index >= first_trace_command && command.address == 0x128) {
      const uint32_t offset = ((registers[0x218 / 4] >> 11) & 0x1ffu) << 12;
      const uint32_t front = displayed != 0 ? offset : 0;
      const uint32_t back = displayed != 0 ? 0 : offset;
      presented = ((registers[0x110 / 4] >> 14) & 1u) != 0 ? back : front;
      displayed ^= 1;
    }
    Apply(&registers[(command.address & 0x3fcu) / 4], command.data, command.mask);
  }
  const uint32_t offset = ((registers[0x218 / 4] >> 11) & 0x1ffu) << 12;
  const uint32_t front = displayed != 0 ? offset : 0;
  const uint32_t back = displayed != 0 ? 0 : offset;
  const uint32_t base = ((registers[0x110 / 4] >> 14) & 1u) != 0 ? back : front;
  const uint32_t tiles = (registers[0x214 / 4] >> 4) & 0xfu;
  return FramebufferConfig{presented.value_or(base), (tiles != 0 ? tiles : 10u) << 6};
}

xls::Value BitsValue(uint64_t value, int64_t width) {
  return xls::Value(xls::UBits(value, width));
}

xls::Value CommandValue(const Command& command) {
  return xls::Value::TupleOwned({
      BitsValue(command.write, 1), BitsValue(command.address, 24),
      BitsValue(command.data, 32), BitsValue(command.mask, 4)});
}

xls::Value De10RequestValue(const Command& command) {
  return xls::Value::TupleOwned({
      BitsValue(command.address >> 2, 22), BitsValue(command.write == 0, 1),
      BitsValue(command.write != 0, 1), BitsValue(command.mask, 4),
      BitsValue(command.data, 32)});
}

xls::Value BurstValue(const std::array<Command, 32>& commands) {
  std::vector<xls::Value> values;
  values.reserve(commands.size());
  for (const Command& command : commands) {
    values.push_back(CommandValue(command));
  }
  return xls::Value::TupleOwned({xls::Value::ArrayOwned(std::move(values))});
}

uint64_t AsUint64(const xls::Value& value) {
  return value.bits().ToUint64().value();
}

uint64_t ReadNative(const uint8_t* buffer, const xls::ElementLayout& layout) {
  uint64_t value = 0;
  std::memcpy(&value, buffer + layout.offset, layout.data_size);
  return value;
}

void WriteNative(uint8_t* buffer, const xls::ElementLayout& layout,
                 uint64_t value) {
  std::memcpy(buffer + layout.offset, &value, layout.data_size);
}

absl::Status AttachMemoryGenerator(
    xls::JitChannelQueue* requests, xls::JitChannelQueue* responses,
    std::vector<uint32_t>* memory, xls::JitRuntime* runtime) {
  xls::TypeLayout request_layout = runtime->GetTypeLayout(requests->channel()->type());
  xls::TypeLayout response_layout = runtime->GetTypeLayout(responses->channel()->type());
  if (request_layout.elements().size() != 4 ||
      response_layout.elements().size() != 2) {
    return absl::InvalidArgumentError("unexpected native memory channel layout");
  }
  std::vector<uint8_t> request_buffer(request_layout.size());
  return responses->AttachRawGenerator(
       [requests, memory, request_layout = std::move(request_layout),
        response_layout = std::move(response_layout),
       request_buffer = std::move(request_buffer)](uint8_t* output) mutable {
        if (!requests->ReadRaw(request_buffer.data())) {
          return false;
        }
        const auto fields = request_layout.elements();
        const uint32_t write = ReadNative(request_buffer.data(), fields[0]);
         const uint32_t address = ReadNative(request_buffer.data(), fields[1]);
         const uint32_t data = ReadNative(request_buffer.data(), fields[2]);
         const uint32_t mask = ReadNative(request_buffer.data(), fields[3]);
         std::memset(output, 0, response_layout.size());
         if (address >= memory->size()) {
           return true;
         }
         if (write != 0) {
           Apply(&(*memory)[address], data, mask);
         }
         const auto result = response_layout.elements();
         WriteNative(output, result[0], (*memory)[address]);
        WriteNative(output, result[1], 1);
        return true;
      });
}

absl::Status AttachMemoryPairGenerator(
    xls::JitChannelQueue* requests, xls::JitChannelQueue* responses,
    std::vector<uint32_t>* memory, xls::JitRuntime* runtime) {
  xls::TypeLayout request_layout = runtime->GetTypeLayout(requests->channel()->type());
  xls::TypeLayout response_layout = runtime->GetTypeLayout(responses->channel()->type());
  if (request_layout.elements().size() != 9 ||
      response_layout.elements().size() != 4) {
    return absl::InvalidArgumentError("unexpected native memory-pair channel layout");
  }
  std::vector<uint8_t> request_buffer(request_layout.size());
  return responses->AttachRawGenerator(
      [requests, memory, request_layout = std::move(request_layout),
       response_layout = std::move(response_layout),
       request_buffer = std::move(request_buffer)](uint8_t* output) mutable {
        if (!requests->ReadRaw(request_buffer.data())) {
          return false;
        }
        const auto request = request_layout.elements();
        const auto response = response_layout.elements();
        const uint32_t valid = ReadNative(request_buffer.data(), request[0]);
        std::memset(output, 0, response_layout.size());
        for (uint32_t index = 0; index < 2; ++index) {
          if ((valid & (1u << index)) == 0) {
            continue;
          }
          const uint32_t field = 1 + index * 4;
          const uint32_t address = ReadNative(request_buffer.data(), request[field + 1]);
          if (address >= memory->size()) {
            continue;
          }
          if (ReadNative(request_buffer.data(), request[field]) != 0) {
            Apply(&(*memory)[address], ReadNative(request_buffer.data(), request[field + 2]),
                  ReadNative(request_buffer.data(), request[field + 3]));
          }
          WriteNative(output, response[index * 2], (*memory)[address]);
          WriteNative(output, response[index * 2 + 1], 1);
        }
        return true;
      });
}

absl::Status AttachBlockReadGenerator(
    xls::JitChannelQueue* requests, xls::JitChannelQueue* responses,
    std::vector<uint32_t>* memory, uint64_t* read_count, xls::JitRuntime* runtime) {
  xls::TypeLayout request_layout = runtime->GetTypeLayout(requests->channel()->type());
  xls::TypeLayout response_layout = runtime->GetTypeLayout(responses->channel()->type());
  if (request_layout.elements().size() != 1 ||
      response_layout.elements().size() != 2 ||
      response_layout.elements()[0].data_size != 64 * sizeof(uint32_t)) {
    return absl::InvalidArgumentError("unexpected native memory block-read channel layout");
  }
  std::vector<uint8_t> request_buffer(request_layout.size());
  return responses->AttachRawGenerator(
      [requests, memory, read_count, request_layout = std::move(request_layout),
       response_layout = std::move(response_layout),
       request_buffer = std::move(request_buffer)](uint8_t* output) mutable {
        if (!requests->ReadRaw(request_buffer.data())) {
          return false;
        }
        const uint32_t address =
            ReadNative(request_buffer.data(), request_layout.elements()[0]);
        const bool success =
            address <= memory->size() && memory->size() - address >= 64;
        std::memset(output, 0, response_layout.size());
        const auto result = response_layout.elements();
        if (success) {
          std::memcpy(output + result[0].offset, memory->data() + address,
                      64 * sizeof(uint32_t));
          ++*read_count;
        }
        WriteNative(output, result[1], success);
        return true;
      });
}

absl::Status AttachPackedBankGenerator(
    xls::JitChannelQueue* requests, xls::JitChannelQueue* responses,
    PackedBank* bank, xls::JitRuntime* runtime) {
  xls::TypeLayout request_layout = runtime->GetTypeLayout(requests->channel()->type());
  xls::TypeLayout response_layout = runtime->GetTypeLayout(responses->channel()->type());
  if (request_layout.elements().size() != 4 || response_layout.elements().size() != 2) {
    return absl::InvalidArgumentError("unexpected native packed-bank channel layout");
  }
  std::vector<uint8_t> request_buffer(request_layout.size());
  return responses->AttachRawGenerator(
      [requests, bank, request_layout = std::move(request_layout),
       response_layout = std::move(response_layout),
       request_buffer = std::move(request_buffer)](uint8_t* output) mutable {
        if (!requests->ReadRaw(request_buffer.data())) {
          return false;
        }
        const auto fields = request_layout.elements();
        const bool write = ReadNative(request_buffer.data(), fields[0]) != 0;
        const uint32_t address = ReadNative(request_buffer.data(), fields[1]);
        const uint32_t slot = (address >> 6) & 63u;
        const uint32_t word = address & 63u;
        uint32_t data = 0;
        std::memcpy(&data, (*bank)[slot].data() + word * sizeof(uint32_t), sizeof(data));
        if (write) {
          Apply(&data, ReadNative(request_buffer.data(), fields[2]),
                ReadNative(request_buffer.data(), fields[3]));
          std::memcpy((*bank)[slot].data() + word * sizeof(uint32_t), &data, sizeof(data));
        }
        std::memset(output, 0, response_layout.size());
        const auto result = response_layout.elements();
        WriteNative(output, result[0], data);
        WriteNative(output, result[1], 1);
        return true;
      });
}

absl::Status AttachBlockGenerator(
    xls::JitChannelQueue* requests, xls::JitChannelQueue* responses,
    std::vector<uint32_t>* memory, xls::JitRuntime* runtime) {
  xls::TypeLayout request_layout = runtime->GetTypeLayout(requests->channel()->type());
  xls::TypeLayout response_layout = runtime->GetTypeLayout(responses->channel()->type());
  if (request_layout.elements().size() != 65 ||
      response_layout.elements().size() != 1) {
    return absl::InvalidArgumentError("unexpected native memory block channel layout");
  }
  std::vector<uint8_t> request_buffer(request_layout.size());
  return responses->AttachRawGenerator(
      [requests, memory, request_layout = std::move(request_layout),
       response_layout = std::move(response_layout),
       request_buffer = std::move(request_buffer)](uint8_t* output) mutable {
        if (!requests->ReadRaw(request_buffer.data())) {
          return false;
        }
        const auto fields = request_layout.elements();
         const uint32_t address = ReadNative(request_buffer.data(), fields[0]);
         std::memset(output, 0, response_layout.size());
         if (address > memory->size() || memory->size() - address < 32) {
           return true;
         }
         for (uint32_t index = 0; index < 32; ++index) {
           Apply(&(*memory)[address + index],
                ReadNative(request_buffer.data(), fields[1 + index]),
                ReadNative(request_buffer.data(), fields[33 + index]));
        }
        WriteNative(output, response_layout.elements()[0], 1);
        return true;
      });
}


absl::StatusOr<std::unique_ptr<xls::SerialProcRuntime>> CreateRuntime(
    xls::Package* package, const Options& options) {
  XLS_ASSIGN_OR_RETURN(xls::Proc * top, package->GetTopAsProc());
  if (options.aot_library.empty()) {
    return xls::CreateJitSerialProcRuntime(top);
  }
  XLS_ASSIGN_OR_RETURN(std::string proto_data, xls::GetFileContents(options.aot_proto));
  xls::AotPackageEntrypointsProto proto;
  if (!proto.ParseFromString(proto_data)) {
    return absl::InvalidArgumentError("cannot parse AOT metadata " + options.aot_proto);
  }
  void* library = dlopen(options.aot_library.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (library == nullptr) {
    return absl::InvalidArgumentError("cannot load AOT library: " + std::string(dlerror()));
  }
  std::vector<xls::ProcAotEntrypoints> implementations;
  implementations.reserve(proto.entrypoint_size());
  for (const xls::AotEntrypointProto& entrypoint : proto.entrypoint()) {
    if (!entrypoint.has_proc_metadata() || !entrypoint.has_function_symbol()) {
      return absl::InvalidArgumentError("AOT metadata contains a non-proc entrypoint");
    }
    void* symbol = dlsym(library, entrypoint.function_symbol().c_str());
    if (symbol == nullptr) {
      return absl::InvalidArgumentError(
          "missing AOT symbol " + entrypoint.function_symbol());
    }
    xls::JitFunctionType function;
    static_assert(sizeof(function) == sizeof(symbol));
    std::memcpy(&function, &symbol, sizeof(function));
    implementations.push_back(xls::ProcAotEntrypoints{
        .proc_interface_proto = entrypoint.proc_metadata().proc_interface(),
        .unpacked = function});
  }
  return xls::CreateAotSerialProcRuntime(top, proto, implementations);
}

absl::StatusOr<xls::ChannelQueue*> Queue(
    xls::ChannelQueueManager* manager, std::string_view name) {
  return manager->GetBoundaryQueueByName(name);
}

absl::Status RunOperation(
    xls::SerialProcRuntime* runtime, xls::ChannelQueue* input,
    xls::ChannelQueue* responses, const xls::Value& value,
    uint64_t max_ticks, uint64_t* ticks, uint32_t* response_data = nullptr) {
  XLS_RETURN_IF_ERROR(input->Write(value));
  while (*ticks < max_ticks) {
    XLS_RETURN_IF_ERROR(runtime->Tick());
    ++*ticks;
    std::optional<xls::Value> response = responses->Read();
    if (response.has_value()) {
      if (AsUint64(response->element(1)) == 0) {
        return absl::InternalError("CoreSim rejected a replay operation");
      }
      if (response_data != nullptr) {
        *response_data = static_cast<uint32_t>(AsUint64(response->element(0)));
      }
      return absl::OkStatus();
    }
  }
  return absl::DeadlineExceededError("maximum CoreSim tick count exceeded");
}

absl::StatusOr<uint32_t> FramebufferHash(
    const std::vector<uint32_t>& memory, const FramebufferConfig& config,
    uint32_t width, uint32_t height) {
  uint32_t hash = 0x811c9dc5u;
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      const uint64_t address = config.base +
                               static_cast<uint64_t>(y) * config.stride * 2 +
                               static_cast<uint64_t>(x) * 2;
      if ((address >> 2) >= memory.size()) {
        return absl::OutOfRangeError("framebuffer dimensions exceed native memory");
      }
      const uint32_t word = memory[address >> 2];
      const uint16_t pixel = (address & 2u) != 0 ? word >> 16 : word;
      hash = (hash ^ (pixel & 0xffu)) * 0x01000193u;
      hash = (hash ^ (pixel >> 8)) * 0x01000193u;
    }
  }
  return hash;
}

absl::Status WriteFramebuffer(
    const std::string& path, const std::vector<uint32_t>& memory,
    const FramebufferConfig& config, uint32_t width, uint32_t height) {
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    return absl::InvalidArgumentError("cannot open framebuffer output " + path);
  }
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      const uint64_t address = config.base +
                               static_cast<uint64_t>(y) * config.stride * 2 +
                               static_cast<uint64_t>(x) * 2;
      if ((address >> 2) >= memory.size()) {
        return absl::OutOfRangeError("framebuffer dimensions exceed native memory");
      }
      const uint32_t word = memory[address >> 2];
      const uint16_t pixel = (address & 2u) != 0 ? word >> 16 : word;
      const char bytes[2] = {
          static_cast<char>(pixel), static_cast<char>(pixel >> 8)};
      output.write(bytes, sizeof(bytes));
    }
  }
  if (!output) {
    return absl::DataLossError("cannot write framebuffer output " + path);
  }
  return absl::OkStatus();
}

#include "xls-bridge-server.inc"

absl::StatusOr<uint64_t> ParseNumber(std::string_view text) {
  try {
    size_t used = 0;
    const uint64_t value = std::stoull(std::string(text), &used, 0);
    if (used != text.size()) {
      return absl::InvalidArgumentError("invalid number " + std::string(text));
    }
    return value;
  } catch (...) {
    return absl::InvalidArgumentError("invalid number " + std::string(text));
  }
}

absl::StatusOr<Options> ParseOptions(int argc, char** argv) {
  if (argc < 3) {
    return absl::InvalidArgumentError(
        "usage: voodoo_xls_runtime IR TRACE|--server=SOCKET [--de10] "
        "[--state=FILE] [--aot_proto=FILE --aot_library=FILE] "
        "[--width=N --height=N --max_ticks=N --expected_hash=N] "
        "[--framebuffer_output=FILE --minimum_cache_hit_rate=RATE]");
  }
  Options options;
  options.ir = argv[1];
  const std::string source = argv[2];
  if (source.rfind("--server=", 0) == 0) {
    options.server_socket = source.substr(strlen("--server="));
  } else {
    options.trace = source;
  }
  for (int index = 3; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--de10") {
      options.de10 = true;
      continue;
    }
    const size_t separator = argument.find('=');
    if (separator == std::string::npos || argument.rfind("--", 0) != 0) {
      return absl::InvalidArgumentError("invalid option " + argument);
    }
    const std::string name = argument.substr(2, separator - 2);
    const std::string value = argument.substr(separator + 1);
    if (name == "state") {
      options.state = value;
    } else if (name == "aot_proto") {
      options.aot_proto = value;
    } else if (name == "aot_library") {
      options.aot_library = value;
    } else if (name == "framebuffer_output") {
      options.framebuffer_output = value;
    } else if (name == "server") {
      options.server_socket = value;
    } else if (name == "minimum_cache_hit_rate") {
      try {
        size_t used = 0;
        const double rate = std::stod(value, &used);
        if (used != value.size() || !std::isfinite(rate)) {
          return absl::InvalidArgumentError("invalid cache hit rate " + value);
        }
        options.minimum_cache_hit_rate = rate;
      } catch (const std::exception&) {
        return absl::InvalidArgumentError("invalid cache hit rate " + value);
      }
    } else {
      XLS_ASSIGN_OR_RETURN(uint64_t number, ParseNumber(value));
      if ((name == "width" || name == "height" || name == "expected_hash") &&
          number > std::numeric_limits<uint32_t>::max()) {
        return absl::InvalidArgumentError("option --" + name + " exceeds 32 bits");
      }
      if (name == "width") {
        options.width = static_cast<uint32_t>(number);
      } else if (name == "height") {
        options.height = static_cast<uint32_t>(number);
      } else if (name == "max_ticks") {
        options.max_ticks = number;
      } else if (name == "expected_hash") {
        options.expected_hash = static_cast<uint32_t>(number);
      } else {
        return absl::InvalidArgumentError("unknown option --" + name);
      }
    }
  }
  if (options.trace.empty() == options.server_socket.empty()) {
    return absl::InvalidArgumentError("specify exactly one trace or bridge server socket");
  }
  if (!options.server_socket.empty() && options.de10) {
    return absl::InvalidArgumentError("bridge server uses the CoreSim command interface");
  }
  if (options.aot_proto.empty() != options.aot_library.empty()) {
    return absl::InvalidArgumentError(
        "--aot_proto and --aot_library must be specified together");
  }
  if (options.width == 0 || options.height == 0 || options.max_ticks == 0) {
    return absl::InvalidArgumentError("width, height, and max_ticks must be nonzero");
  }
  if (options.minimum_cache_hit_rate.has_value() &&
      (*options.minimum_cache_hit_rate < 0.0 || *options.minimum_cache_hit_rate > 1.0)) {
    return absl::InvalidArgumentError("cache hit rate must be between zero and one");
  }
  return options;
}

absl::Status RealMain(int argc, char** argv) {
  XLS_ASSIGN_OR_RETURN(Options options, ParseOptions(argc, argv));
  std::filesystem::path trace_path(options.trace);
  std::error_code filesystem_error;
  Trace trace;
  if (options.server_socket.empty()) {
    if (std::filesystem::is_directory(trace_path, filesystem_error)) {
      trace_path /= "trace.bin";
      options.trace = trace_path.string();
    }
    if (filesystem_error) {
      return absl::InvalidArgumentError("cannot inspect trace path " + options.trace);
    }
    XLS_ASSIGN_OR_RETURN(trace, LoadTrace(options.trace, options.de10));
  } else {
    trace.framebuffer_words = 4 * 1024 * 1024 / sizeof(uint32_t);
    trace.texture_words = 8 * 1024 * 1024 / sizeof(uint32_t);
  }
  std::optional<StateSnapshot> state;
  size_t first_trace_command = 0;
  uint32_t initial_displayed = 0;
  std::filesystem::path state_path = options.state.empty()
      ? options.server_socket.empty() ? trace_path.parent_path() / "state.bin"
                                      : std::filesystem::path()
      : std::filesystem::path(options.state);
  const bool state_exists = !state_path.empty() &&
      std::filesystem::is_regular_file(state_path, filesystem_error);
  if (filesystem_error == std::errc::no_such_file_or_directory) {
    filesystem_error.clear();
  }
  if (filesystem_error) {
    return absl::InvalidArgumentError("cannot inspect state path " + state_path.string());
  }
  if (!options.state.empty() && !state_exists) {
    return absl::NotFoundError("cannot open " + state_path.string());
  }
  if (state_exists) {
    XLS_ASSIGN_OR_RETURN(StateSnapshot loaded, LoadState(state_path.string(), options.de10));
    state = std::move(loaded);
    initial_displayed = state->swap_count;
    if (state->swap_count != 0) {
      state->registers.push_back(Command{1, 0x128, 0, 0xf});
    }
    first_trace_command = state->registers.size();
    trace.commands.insert(trace.commands.begin(), state->registers.begin(), state->registers.end());
    trace.framebuffer_words = std::max(
        trace.framebuffer_words, static_cast<uint32_t>(state->framebuffer.size() / 4));
    trace.texture_words = std::max(
        trace.texture_words, static_cast<uint32_t>(state->texture.size() / 4));
    options.width = state->width;
    options.height = state->height;
  }
  const FramebufferConfig framebuffer_config =
      GetFramebufferConfig(trace.commands, first_trace_command, initial_displayed);
  const std::vector<Operation> operations = Coalesce(trace.commands, false);
  XLS_ASSIGN_OR_RETURN(std::string ir, xls::GetFileContents(options.ir));
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<xls::Package> package,
      xls::ParsePackage(ir, std::string_view(options.ir)));
  const auto compile_start = std::chrono::steady_clock::now();
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<xls::SerialProcRuntime> runtime,
      CreateRuntime(package.get(), options));
  const auto replay_start = std::chrono::steady_clock::now();
  XLS_ASSIGN_OR_RETURN(xls::JitChannelQueueManager * jit_manager,
                       runtime->GetJitChannelQueueManager());
  xls::ChannelQueueManager* manager = jit_manager;
  XLS_ASSIGN_OR_RETURN(
      xls::ChannelQueue * command,
      Queue(manager, options.de10 ? "_de10_request_r" : "_command_r"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * burst, Queue(manager, "_burst_r"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * response, Queue(manager, "_response_s"));
  XLS_ASSIGN_OR_RETURN(
      xls::ChannelQueue * framebuffer_command,
      Queue(manager, "_framebuffer_command_s"));
  XLS_ASSIGN_OR_RETURN(
      xls::ChannelQueue * framebuffer_response,
      Queue(manager, "_framebuffer_response_r"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * framebuffer_pixel_read,
                       Queue(manager, "_framebuffer_pixel_read_s"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * framebuffer_pixel_read_response,
                       Queue(manager, "_framebuffer_pixel_read_response_r"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * framebuffer_pixel_write,
                       Queue(manager, "_framebuffer_pixel_write_s"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * framebuffer_pixel_write_response,
                       Queue(manager, "_framebuffer_pixel_write_response_r"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * framebuffer_fastfill,
                       Queue(manager, "_framebuffer_fastfill_s"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * framebuffer_fastfill_response,
                       Queue(manager, "_framebuffer_fastfill_response_r"));
  XLS_ASSIGN_OR_RETURN(
      xls::ChannelQueue * texture_command,
      Queue(manager, "_texture_command_s"));
  XLS_ASSIGN_OR_RETURN(
      xls::ChannelQueue * texture_response,
      Queue(manager, "_texture_response_r"));
  XLS_ASSIGN_OR_RETURN(
      xls::ChannelQueue * texture_fill,
      Queue(manager, "_texture_fill_s"));
  XLS_ASSIGN_OR_RETURN(
      xls::ChannelQueue * texture_fill_response,
      Queue(manager, "_texture_fill_response_r"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * bank0_command,
                       Queue(manager, "_bank0_command_s"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * bank0_response,
                       Queue(manager, "_bank0_response_r"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * bank1_command,
                       Queue(manager, "_bank1_command_s"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * bank1_response,
                       Queue(manager, "_bank1_response_r"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * bank2_command,
                       Queue(manager, "_bank2_command_s"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * bank2_response,
                       Queue(manager, "_bank2_response_r"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * bank3_command,
                       Queue(manager, "_bank3_command_s"));
  XLS_ASSIGN_OR_RETURN(xls::ChannelQueue * bank3_response,
                       Queue(manager, "_bank3_response_r"));

  XLS_ASSIGN_OR_RETURN(
      xls::ChannelQueue * texture_block,
      Queue(manager, "_texture_block_s"));
  XLS_ASSIGN_OR_RETURN(
      xls::ChannelQueue * texture_block_response,
      Queue(manager, "_texture_block_response_r"));
  std::vector<uint32_t> framebuffer(trace.framebuffer_words);
  std::vector<uint32_t> texture(trace.texture_words);
  if (state.has_value()) {
    std::memcpy(framebuffer.data(), state->framebuffer.data(), state->framebuffer.size());
    std::memcpy(texture.data(), state->texture.data(), state->texture.size());
  }
  XLS_RETURN_IF_ERROR(AttachMemoryGenerator(
      &jit_manager->GetJitQueue(framebuffer_command->channel_instance()),
      &jit_manager->GetJitQueue(framebuffer_response->channel_instance()), &framebuffer,
      &jit_manager->runtime()));
  XLS_RETURN_IF_ERROR(AttachMemoryPairGenerator(
      &jit_manager->GetJitQueue(framebuffer_pixel_read->channel_instance()),
      &jit_manager->GetJitQueue(framebuffer_pixel_read_response->channel_instance()),
      &framebuffer, &jit_manager->runtime()));
  XLS_RETURN_IF_ERROR(AttachMemoryPairGenerator(
      &jit_manager->GetJitQueue(framebuffer_pixel_write->channel_instance()),
      &jit_manager->GetJitQueue(framebuffer_pixel_write_response->channel_instance()),
      &framebuffer, &jit_manager->runtime()));
  XLS_RETURN_IF_ERROR(AttachMemoryPairGenerator(
      &jit_manager->GetJitQueue(framebuffer_fastfill->channel_instance()),
      &jit_manager->GetJitQueue(framebuffer_fastfill_response->channel_instance()),
      &framebuffer, &jit_manager->runtime()));
  XLS_RETURN_IF_ERROR(AttachMemoryGenerator(
      &jit_manager->GetJitQueue(texture_command->channel_instance()),
      &jit_manager->GetJitQueue(texture_response->channel_instance()), &texture,
      &jit_manager->runtime()));
  uint64_t texture_fill_reads = 0;
  XLS_RETURN_IF_ERROR(AttachBlockReadGenerator(
      &jit_manager->GetJitQueue(texture_fill->channel_instance()),
      &jit_manager->GetJitQueue(texture_fill_response->channel_instance()), &texture,
      &texture_fill_reads, &jit_manager->runtime()));
  std::array<PackedBank, 4> banks = {};
  XLS_RETURN_IF_ERROR(AttachPackedBankGenerator(
      &jit_manager->GetJitQueue(bank0_command->channel_instance()),
      &jit_manager->GetJitQueue(bank0_response->channel_instance()), &banks[0],
      &jit_manager->runtime()));
  XLS_RETURN_IF_ERROR(AttachPackedBankGenerator(
      &jit_manager->GetJitQueue(bank1_command->channel_instance()),
      &jit_manager->GetJitQueue(bank1_response->channel_instance()), &banks[1],
      &jit_manager->runtime()));
  XLS_RETURN_IF_ERROR(AttachPackedBankGenerator(
      &jit_manager->GetJitQueue(bank2_command->channel_instance()),
      &jit_manager->GetJitQueue(bank2_response->channel_instance()), &banks[2],
      &jit_manager->runtime()));
  XLS_RETURN_IF_ERROR(AttachPackedBankGenerator(
      &jit_manager->GetJitQueue(bank3_command->channel_instance()),
      &jit_manager->GetJitQueue(bank3_response->channel_instance()), &banks[3],
      &jit_manager->runtime()));
  XLS_RETURN_IF_ERROR(AttachBlockGenerator(
      &jit_manager->GetJitQueue(texture_block->channel_instance()),
      &jit_manager->GetJitQueue(texture_block_response->channel_instance()), &texture,
      &jit_manager->runtime()));

  LiveFramebufferState live_framebuffer;
  live_framebuffer.displayed = initial_displayed & 1u;
  for (size_t index = 0; index < trace.commands.size(); ++index) {
    TrackLiveCommand(&live_framebuffer, trace.commands[index], index >= first_trace_command);
  }
  uint64_t ticks = 0;
  for (const Operation& operation : operations) {
    const xls::Value value = operation.burst
                                 ? BurstValue(operation.commands)
                                 : options.de10
                                       ? De10RequestValue(operation.commands[0])
                                       : CommandValue(operation.commands[0]);
    XLS_RETURN_IF_ERROR(RunOperation(
        runtime.get(), operation.burst ? burst : command, response, value,
        options.max_ticks, &ticks));
  }
  if (!options.server_socket.empty()) {
    return ServeBridge(options.server_socket, runtime.get(), command, response,
                       &framebuffer, &texture, options.max_ticks,
                       std::move(live_framebuffer));
  }
  const Command fence = {0, 0x400000u, 0, 0};
  XLS_RETURN_IF_ERROR(RunOperation(
      runtime.get(), command, response,
      options.de10 ? De10RequestValue(fence) : CommandValue(fence),
      options.max_ticks, &ticks));
  uint32_t texture_cache_hits = 0;
  uint32_t texture_cache_misses = 0;
  uint32_t texture_cache_all_hit_requests = 0;
  uint32_t texture_cache_missed_requests = 0;
  const Command cache_hits = {0, 0x3f0u, 0, 0};
  const Command cache_misses = {0, 0x3f4u, 0, 0};
  const Command cache_all_hit_requests = {0, 0x3f8u, 0, 0};
  const Command cache_missed_requests = {0, 0x3fcu, 0, 0};
  XLS_RETURN_IF_ERROR(RunOperation(
      runtime.get(), command, response,
      options.de10 ? De10RequestValue(cache_hits) : CommandValue(cache_hits),
      options.max_ticks, &ticks, &texture_cache_hits));
  XLS_RETURN_IF_ERROR(RunOperation(
      runtime.get(), command, response,
      options.de10 ? De10RequestValue(cache_misses) : CommandValue(cache_misses),
      options.max_ticks, &ticks, &texture_cache_misses));
  XLS_RETURN_IF_ERROR(RunOperation(
      runtime.get(), command, response,
      options.de10 ? De10RequestValue(cache_all_hit_requests)
                   : CommandValue(cache_all_hit_requests),
      options.max_ticks, &ticks, &texture_cache_all_hit_requests));
  XLS_RETURN_IF_ERROR(RunOperation(
      runtime.get(), command, response,
      options.de10 ? De10RequestValue(cache_missed_requests)
                   : CommandValue(cache_missed_requests),
      options.max_ticks, &ticks, &texture_cache_missed_requests));
  XLS_ASSIGN_OR_RETURN(
      uint32_t hash,
      FramebufferHash(framebuffer, framebuffer_config, options.width, options.height));
  if (!options.framebuffer_output.empty()) {
    XLS_RETURN_IF_ERROR(WriteFramebuffer(
        options.framebuffer_output, framebuffer, framebuffer_config,
        options.width, options.height));
  }
  const auto end = std::chrono::steady_clock::now();
  const double compile_seconds =
      std::chrono::duration<double>(replay_start - compile_start).count();
  const double replay_seconds =
      std::chrono::duration<double>(end - replay_start).count();
  std::printf("interface: %s\ncommands: %zu\noperations: %zu\nticks: %llu\n",
              options.de10 ? "de10" : "core", trace.commands.size(), operations.size(),
              static_cast<unsigned long long>(ticks));
  std::printf("compile wall: %.6f s\nreplay wall: %.6f s\n", compile_seconds,
              replay_seconds);
  std::printf("framebuffer: %ux%u base=0x%x stride=%u\n", options.width,
              options.height, framebuffer_config.base, framebuffer_config.stride);
  std::printf("framebuffer hash: 0x%08x\n", hash);
  if (!options.framebuffer_output.empty()) {
    std::printf("framebuffer output: %s\n", options.framebuffer_output.c_str());
  }
  const uint64_t texture_cache_lookups =
      static_cast<uint64_t>(texture_cache_hits) + texture_cache_misses;
  const double texture_cache_hit_rate = texture_cache_lookups == 0
      ? 0.0
      : static_cast<double>(texture_cache_hits) / texture_cache_lookups;
  const uint64_t texture_cache_requests =
      static_cast<uint64_t>(texture_cache_all_hit_requests) + texture_cache_missed_requests;
  const double texture_cache_request_hit_rate = texture_cache_requests == 0
      ? 0.0
      : static_cast<double>(texture_cache_all_hit_requests) / texture_cache_requests;
  std::printf("texture cache taps: hits=%u misses=%u hit rate=%.4f\n",
              texture_cache_hits, texture_cache_misses, texture_cache_hit_rate);
  std::printf("texture cache requests: all-hit=%u missed=%u hit rate=%.4f\n",
              texture_cache_all_hit_requests, texture_cache_missed_requests,
              texture_cache_request_hit_rate);
  std::printf("texture cache line fills: %llu\n",
              static_cast<unsigned long long>(texture_fill_reads));
  if (options.minimum_cache_hit_rate.has_value() &&
      (texture_cache_hit_rate < *options.minimum_cache_hit_rate ||
       texture_cache_request_hit_rate < *options.minimum_cache_hit_rate)) {
    return absl::FailedPreconditionError("texture cache hit rate below required minimum");
  }
  if (options.expected_hash.has_value() && hash != *options.expected_hash) {
    return absl::FailedPreconditionError("framebuffer hash mismatch");
  }
  return absl::OkStatus();
}

}

int main(int argc, char** argv) {
  const absl::Status status = RealMain(argc, argv);
  if (!status.ok()) {
    std::fprintf(stderr, "%s\n", status.ToString().c_str());
    return 1;
  }
  return 0;
}
