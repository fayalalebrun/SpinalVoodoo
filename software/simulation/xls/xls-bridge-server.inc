struct LiveFramebufferState {
  std::array<uint32_t, 256> registers = {};
  std::optional<uint32_t> presented;
  uint32_t displayed = 0;
};

FramebufferConfig ActiveFramebufferConfig(const LiveFramebufferState& state) {
  const uint32_t offset = ((state.registers[0x218 / 4] >> 11) & 0x1ffu) << 12;
  const uint32_t front = state.displayed != 0 ? offset : 0;
  const uint32_t back = state.displayed != 0 ? 0 : offset;
  const uint32_t base = ((state.registers[0x110 / 4] >> 14) & 1u) != 0 ? back : front;
  const uint32_t tiles = (state.registers[0x214 / 4] >> 4) & 0xfu;
  return FramebufferConfig{base, (tiles != 0 ? tiles : 10u) << 6};
}

FramebufferConfig CurrentFramebufferConfig(const LiveFramebufferState& state) {
  FramebufferConfig config = ActiveFramebufferConfig(state);
  config.base = state.presented.value_or(config.base);
  return config;
}

void TrackLiveCommand(LiveFramebufferState* state, const Command& command, bool track_swap) {
  if (command.write == 0 || command.address >= 0x400) return;
  if (track_swap && command.address == 0x128) {
    state->presented = ActiveFramebufferConfig(*state).base;
    state->displayed ^= 1;
  }
  Apply(&state->registers[(command.address & 0x3fcu) / 4], command.data, command.mask);
}

absl::Status WriteExact(int fd, const void* data, size_t size) {
  const uint8_t* bytes = static_cast<const uint8_t*>(data);
  while (size != 0) {
    const ssize_t written = write(fd, bytes, size);
    if (written < 0 && errno == EINTR) continue;
    if (written <= 0) return absl::ErrnoToStatus(errno, "bridge write failed");
    bytes += written;
    size -= static_cast<size_t>(written);
  }
  return absl::OkStatus();
}

absl::StatusOr<bool> ReadExact(int fd, void* data, size_t size, bool allow_eof = false) {
  uint8_t* bytes = static_cast<uint8_t*>(data);
  size_t received_total = 0;
  while (received_total != size) {
    const ssize_t received = read(fd, bytes + received_total, size - received_total);
    if (received < 0 && errno == EINTR) continue;
    if (received < 0) return absl::ErrnoToStatus(errno, "bridge read failed");
    if (received == 0) {
      if (allow_eof && received_total == 0) return false;
      return absl::DataLossError("truncated bridge request");
    }
    received_total += static_cast<size_t>(received);
  }
  return true;
}

absl::Status SendBridgeResponse(int fd, uint32_t status, uint32_t data,
                                uint32_t count, uint64_t ticks) {
  const xls_bridge_response_t response = {
      XLS_BRIDGE_MAGIC, status, data, count, ticks};
  return WriteExact(fd, &response, sizeof(response));
}

absl::Status RunBridgeCommand(xls::SerialProcRuntime* runtime,
                              xls::ChannelQueue* command,
                              xls::ChannelQueue* response,
                              const Command& value, uint64_t max_ticks,
                              uint64_t* total_ticks, uint32_t* data) {
  uint64_t operation_ticks = 0;
  XLS_RETURN_IF_ERROR(RunOperation(runtime, command, response, CommandValue(value),
                                   max_ticks, &operation_ticks, data));
  *total_ticks += operation_ticks;
  return absl::OkStatus();
}

absl::Status ServeBridgeClient(
    int fd, xls::SerialProcRuntime* runtime, xls::ChannelQueue* command,
    xls::ChannelQueue* response, std::vector<uint32_t>* framebuffer,
    std::vector<uint32_t>* texture, uint64_t max_ticks,
    LiveFramebufferState* framebuffer_state) {
  uint64_t total_ticks = 0;
  uint32_t swap_count = framebuffer_state->displayed;
  while (true) {
    xls_bridge_request_t request;
    XLS_ASSIGN_OR_RETURN(bool present, ReadExact(fd, &request, sizeof(request), true));
    if (!present) return absl::OkStatus();
    if (request.magic != XLS_BRIDGE_MAGIC) {
      return absl::InvalidArgumentError("invalid bridge request magic");
    }
    uint32_t response_data = 0;
    absl::Status operation_status = absl::OkStatus();
    std::vector<uint32_t> words;
    if (request.operation == XLS_BRIDGE_WRITE || request.operation == XLS_BRIDGE_READ) {
      const Command value = {
          request.operation == XLS_BRIDGE_WRITE ? uint8_t{1} : uint8_t{0},
          request.address, request.data, static_cast<uint8_t>(request.mask)};
      operation_status = RunBridgeCommand(runtime, command, response, value, max_ticks,
                                          &total_ticks, &response_data);
      if (operation_status.ok() && value.write != 0) {
        TrackLiveCommand(framebuffer_state, value, true);
      }
    } else if (request.operation == XLS_BRIDGE_IDLE ||
               request.operation == XLS_BRIDGE_READ_FRONTBUFFER) {
      const Command fence = {0, 0, 0, 0};
      operation_status = RunBridgeCommand(runtime, command, response, fence, max_ticks,
                                          &total_ticks, &response_data);
    } else if (request.operation == XLS_BRIDGE_WRITE_BUS_BULK) {
      if (request.count > 1u << 22) {
        operation_status = absl::InvalidArgumentError("bridge bulk write is too large");
      } else {
        words.resize(request.count);
        XLS_ASSIGN_OR_RETURN(
            bool payload_present,
            ReadExact(fd, words.data(), words.size() * sizeof(uint32_t)));
        if (!payload_present) return absl::DataLossError("missing bridge bulk payload");
        for (uint32_t index = 0; index < request.count && operation_status.ok(); ++index) {
          const Command value = {
              1, request.address + index * 4, words[index], 0xf};
          operation_status = RunBridgeCommand(runtime, command, response, value, max_ticks,
                                              &total_ticks, nullptr);
          if (operation_status.ok()) TrackLiveCommand(framebuffer_state, value, true);
        }
      }
    } else if (request.operation == XLS_BRIDGE_READ_FB ||
               request.operation == XLS_BRIDGE_READ_TEX ||
               request.operation == XLS_BRIDGE_WRITE_FB ||
               request.operation == XLS_BRIDGE_WRITE_TEX) {
      std::vector<uint32_t>* memory =
          request.operation == XLS_BRIDGE_READ_FB || request.operation == XLS_BRIDGE_WRITE_FB
              ? framebuffer
              : texture;
      const uint64_t first = request.address / sizeof(uint32_t);
      const uint64_t end = first + request.count;
      if ((request.address & 3u) != 0 || end > memory->size()) {
        operation_status = absl::OutOfRangeError("bridge memory range is out of bounds");
      } else if (request.operation == XLS_BRIDGE_WRITE_FB ||
                 request.operation == XLS_BRIDGE_WRITE_TEX) {
        words.resize(request.count);
        XLS_ASSIGN_OR_RETURN(
            bool payload_present,
            ReadExact(fd, words.data(), words.size() * sizeof(uint32_t)));
        if (!payload_present) return absl::DataLossError("missing bridge memory payload");
        std::copy(words.begin(), words.end(), memory->begin() + first);
      }
    } else if (request.operation == XLS_BRIDGE_TICK) {
      for (uint32_t index = 0; index < request.count && operation_status.ok(); ++index) {
        operation_status = runtime->Tick();
        ++total_ticks;
      }
    } else if (request.operation == XLS_BRIDGE_SET_SWAP) {
      swap_count = request.data & 3u;
      framebuffer_state->displayed = swap_count & 1u;
      framebuffer_state->presented.reset();
    } else if (request.operation == XLS_BRIDGE_GET_SWAP) {
      response_data = swap_count;
    } else if (request.operation != XLS_BRIDGE_CLOSE) {
      operation_status = absl::InvalidArgumentError("unknown bridge operation");
    }
    if (!operation_status.ok()) {
      XLS_RETURN_IF_ERROR(SendBridgeResponse(fd, 1, 0, 0, total_ticks));
      return operation_status;
    }
    if (request.operation == XLS_BRIDGE_READ_FB || request.operation == XLS_BRIDGE_READ_TEX) {
      const std::vector<uint32_t>& memory =
          request.operation == XLS_BRIDGE_READ_FB ? *framebuffer : *texture;
      const uint32_t bytes = request.count * sizeof(uint32_t);
      XLS_RETURN_IF_ERROR(SendBridgeResponse(fd, 0, 0, bytes, total_ticks));
      XLS_RETURN_IF_ERROR(
          WriteExact(fd, memory.data() + request.address / sizeof(uint32_t), bytes));
    } else if (request.operation == XLS_BRIDGE_READ_FRONTBUFFER) {
      const uint32_t width = request.address;
      const uint32_t height = request.data;
      if (width == 0 || height == 0 ||
          static_cast<uint64_t>(width) * height > 1u << 22) {
        XLS_RETURN_IF_ERROR(SendBridgeResponse(fd, 1, 0, 0, total_ticks));
        return absl::InvalidArgumentError("invalid bridge framebuffer dimensions");
      }
      const FramebufferConfig config = CurrentFramebufferConfig(*framebuffer_state);
      std::vector<uint16_t> pixels(static_cast<size_t>(width) * height);
      for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
          const uint64_t address = config.base +
              static_cast<uint64_t>(y) * config.stride * 2 + x * 2;
          if (address / 4 >= framebuffer->size()) {
            XLS_RETURN_IF_ERROR(SendBridgeResponse(fd, 1, 0, 0, total_ticks));
            return absl::OutOfRangeError("bridge framebuffer is out of bounds");
          }
          const uint32_t word = (*framebuffer)[address / 4];
          pixels[static_cast<size_t>(y) * width + x] =
              (address & 2u) != 0 ? word >> 16 : word;
        }
      }
      const uint32_t bytes = pixels.size() * sizeof(uint16_t);
      XLS_RETURN_IF_ERROR(SendBridgeResponse(fd, 0, 0, bytes, total_ticks));
      XLS_RETURN_IF_ERROR(WriteExact(fd, pixels.data(), bytes));
    } else {
      XLS_RETURN_IF_ERROR(SendBridgeResponse(fd, 0, response_data, 0, total_ticks));
    }
    if (request.operation == XLS_BRIDGE_CLOSE) {
      std::printf("XLS bridge closed: ticks=%llu\n",
                  static_cast<unsigned long long>(total_ticks));
      return absl::OkStatus();
    }
  }
}

absl::Status ServeBridge(
    const std::string& path, xls::SerialProcRuntime* runtime,
    xls::ChannelQueue* command, xls::ChannelQueue* response,
    std::vector<uint32_t>* framebuffer, std::vector<uint32_t>* texture,
    uint64_t max_ticks, LiveFramebufferState framebuffer_state) {
  if (path.empty() || path.size() >= sizeof(sockaddr_un{}.sun_path)) {
    return absl::InvalidArgumentError("invalid bridge socket path");
  }
  const int server = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server < 0) return absl::ErrnoToStatus(errno, "cannot create bridge socket");
  sockaddr_un address = {};
  address.sun_family = AF_UNIX;
  std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
  unlink(path.c_str());
  if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
      listen(server, 1) != 0) {
    const absl::Status status = absl::ErrnoToStatus(errno, "cannot listen on bridge socket");
    close(server);
    unlink(path.c_str());
    return status;
  }
  std::printf("XLS bridge listening: %s\n", path.c_str());
  std::fflush(stdout);
  int client;
  do {
    client = accept(server, nullptr, nullptr);
  } while (client < 0 && errno == EINTR);
  absl::Status status = client < 0
      ? absl::ErrnoToStatus(errno, "cannot accept bridge client")
      : ServeBridgeClient(client, runtime, command, response, framebuffer, texture,
                          max_ticks, &framebuffer_state);
  if (client >= 0) close(client);
  close(server);
  unlink(path.c_str());
  return status;
}
