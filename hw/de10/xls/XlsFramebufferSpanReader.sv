module XlsFramebufferSpanReader #(
  parameter [31:0] BASE_ADDRESS = 32'h00000000,
  parameter integer ADDRESS_WORDS = 1048576,
  parameter integer SPAN_DEPTH = 4,
  parameter integer DATA_DEPTH = 128,
  parameter integer ROUTE_DEPTH = 4,
  parameter integer RESPONSE_DEPTH = 2,
  parameter integer MAX_BURST = 64
) (
  input  wire          clk,
  input  wire          reset,
  input  wire [45:0]   span,
  input  wire          span_valid,
  output wire          span_ready,
  input  wire [65:0]   request,
  input  wire          request_valid,
  output wire          request_ready,
  output wire [32:0]   response,
  output wire          response_valid,
  input  wire          response_ready,
  output wire          mem_read,
  output wire          mem_write,
  input  wire          mem_waitRequestn,
  output wire [10:0]   mem_burstCount,
  output wire [31:0]   mem_address,
  output wire [3:0]    mem_byteEnable,
  output wire [31:0]   mem_writeData,
  input  wire          mem_readDataValid,
  input  wire [31:0]   mem_readData,
  input  wire          invalidate,
  output wire          debug_data_stall,
  output wire          debug_inflight_stall,
  output wire          debug_unissued_stall,
  output wire          debug_response_stall
);
  localparam integer SPAN_PTR_BITS = $clog2(SPAN_DEPTH);
  localparam integer SPAN_COUNT_BITS = $clog2(SPAN_DEPTH + 1);
  localparam integer DATA_PTR_BITS = $clog2(DATA_DEPTH);
  localparam integer DATA_COUNT_BITS = $clog2(DATA_DEPTH + 1);
  localparam integer ROUTE_PTR_BITS = $clog2(ROUTE_DEPTH);
  localparam integer ROUTE_COUNT_BITS = $clog2(ROUTE_DEPTH + 1);
  localparam integer RESPONSE_PTR_BITS = $clog2(RESPONSE_DEPTH);
  localparam integer RESPONSE_COUNT_BITS = $clog2(RESPONSE_DEPTH + 1);

  wire [22:0] span_start_lane = span[45:23];
  wire [22:0] span_end_lane = span[22:0];
  wire span_legal = span_start_lane <= span_end_lane && span_end_lane[22:1] < ADDRESS_WORDS;

  reg [22:0] span_start [0:SPAN_DEPTH-1];
  reg [22:0] span_end [0:SPAN_DEPTH-1];

  reg [SPAN_PTR_BITS-1:0] span_write_pointer;
  reg [SPAN_PTR_BITS-1:0] issue_span_read_pointer;
  reg [SPAN_PTR_BITS-1:0] consume_span_read_pointer;
  reg [SPAN_COUNT_BITS-1:0] issue_span_count;
  reg [SPAN_COUNT_BITS-1:0] consume_span_count;

  reg issue_active;
  reg [21:0] issue_word_address;
  reg [9:0] issue_words_remaining;

  reg consume_active;
  reg [22:0] consume_lane;
  reg [22:0] consume_end_lane;

  reg [31:0] data_memory [0:DATA_DEPTH-1];
  reg [DATA_PTR_BITS-1:0] data_write_pointer;
  reg [DATA_PTR_BITS-1:0] data_read_pointer;
  reg [DATA_COUNT_BITS-1:0] data_count;
  reg [DATA_COUNT_BITS-1:0] reserved_words;
  wire [31:0] data_head = data_memory[data_read_pointer];

  reg route_direct [0:ROUTE_DEPTH-1];
  reg [6:0] route_beats [0:ROUTE_DEPTH-1];
  reg [ROUTE_PTR_BITS-1:0] route_write_pointer;
  reg [ROUTE_PTR_BITS-1:0] route_read_pointer;
  reg [ROUTE_COUNT_BITS-1:0] route_count;
  wire route_head_direct = route_direct[route_read_pointer];
  wire [6:0] route_head_beats = route_beats[route_read_pointer];

  reg [32:0] response_memory [0:RESPONSE_DEPTH-1];
  reg [RESPONSE_PTR_BITS-1:0] response_write_pointer;
  reg [RESPONSE_PTR_BITS-1:0] response_read_pointer;
  reg [RESPONSE_COUNT_BITS-1:0] response_count;
  reg [RESPONSE_COUNT_BITS-1:0] direct_outstanding;

  wire [6:0] request_count = request[65:59];
  wire request_write = request[58];
  wire [21:0] request_address = request[57:36];
  wire request_legal = request_count == 7'd1 && !request_write &&
      request_address < ADDRESS_WORDS;
  wire [21:0] expected_word_address = consume_lane[22:1];
  wire consume_matches = consume_active && request_address == expected_word_address;
  wire consume_ahead = consume_active && request_address > expected_word_address;
  wire consume_behind = consume_active && request_address < expected_word_address;

  wire response_full = response_count == RESPONSE_DEPTH;
  wire response_space_reserved = response_count + direct_outstanding < RESPONSE_DEPTH;
  wire route_full = route_count == ROUTE_DEPTH;
  wire cached_ready = request_legal && consume_matches && data_count != 0 &&
      direct_outstanding == 0 && !response_full;
  wire invalid_ready = !request_legal && direct_outstanding == 0 && !response_full;
  wire direct_needed = request_legal &&
      (consume_behind || (!consume_active && consume_span_count == 0));
  wire direct_command = request_valid && direct_needed && response_space_reserved && !route_full;

  wire [10:0] issue_burst_words = issue_words_remaining > MAX_BURST ?
      MAX_BURST : issue_words_remaining;
  wire prefetch_capacity = reserved_words + issue_burst_words <= DATA_DEPTH;
  wire prefetch_command = issue_active && prefetch_capacity && !route_full && !direct_command;

  assign mem_read = !reset && (direct_command || prefetch_command);
  assign mem_write = 1'b0;
  assign mem_burstCount = direct_command ? 11'd1 : issue_burst_words;
  assign mem_address = BASE_ADDRESS +
      ((direct_command ? request_address : issue_word_address) << 2);
  assign mem_byteEnable = 4'hf;
  assign mem_writeData = 32'b0;
  wire memory_command_fire = mem_read && mem_waitRequestn;
  wire direct_fire = direct_command && mem_waitRequestn;
  wire prefetch_fire = prefetch_command && mem_waitRequestn;

  assign request_ready = !reset &&
      (cached_ready || invalid_ready || (direct_needed && direct_fire));
  wire cached_fire = request_valid && request_ready && cached_ready;
  wire invalid_fire = request_valid && request_ready && invalid_ready;



  wire skip_word = request_valid && request_legal && consume_ahead && data_count != 0;
  wire cached_finishes_span = cached_fire && consume_lane == consume_end_lane;
  wire cached_pops_word = cached_fire && (consume_lane[0] || cached_finishes_span);
  wire skip_finishes_span = skip_word && expected_word_address == consume_end_lane[22:1];
  wire data_pop = cached_pops_word || skip_word;

  wire memory_response = mem_readDataValid && route_count != 0;
  wire direct_response = memory_response && route_head_direct;
  wire prefetch_response = memory_response && !route_head_direct;
  wire route_pop = memory_response && route_head_beats == 7'd1;
  wire response_push = cached_fire || invalid_fire || direct_response;
  wire [32:0] response_push_data = invalid_fire ? 33'b0 :
      direct_response ? {mem_readData, 1'b1} : {data_head, 1'b1};

  assign response_valid = response_count != 0;
  assign response = response_memory[response_read_pointer];
  wire response_pop = response_valid && response_ready;

  assign span_ready = !reset && issue_span_count != SPAN_DEPTH &&
      consume_span_count != SPAN_DEPTH;
  wire span_fire = span_valid && span_ready && span_legal;
  wire issue_span_load = !issue_active && issue_span_count != 0;
  wire consume_span_load = !consume_active && consume_span_count != 0;

  wire debug_request_stall = request_valid && !request_ready;
  assign debug_data_stall = debug_request_stall && request_legal && !direct_needed &&
      !response_full && direct_outstanding == 0 && data_count == 0;
  assign debug_inflight_stall = debug_data_stall && reserved_words != 0;
  assign debug_unissued_stall = debug_data_stall && reserved_words == 0;
  assign debug_response_stall = debug_request_stall && request_legal && response_full;

  integer i;
  always @(posedge clk) begin
    if (reset || invalidate) begin
      span_write_pointer <= 0;
      issue_span_read_pointer <= 0;
      consume_span_read_pointer <= 0;
      issue_span_count <= 0;
      consume_span_count <= 0;
      issue_active <= 0;
      issue_word_address <= 0;
      issue_words_remaining <= 0;
      consume_active <= 0;
      consume_lane <= 0;
      consume_end_lane <= 0;
      data_write_pointer <= 0;
      data_read_pointer <= 0;
      data_count <= 0;
      reserved_words <= 0;
      route_write_pointer <= 0;
      route_read_pointer <= 0;
      route_count <= 0;
      response_write_pointer <= 0;
      response_read_pointer <= 0;
      response_count <= 0;
      direct_outstanding <= 0;
    end else begin
      if (span_fire) begin
        span_start[span_write_pointer] <= span_start_lane;
        span_end[span_write_pointer] <= span_end_lane;
        span_write_pointer <= span_write_pointer + 1'b1;
      end

      if (issue_span_load) begin
        issue_word_address <= span_start[issue_span_read_pointer][22:1];
        issue_words_remaining <=
            span_end[issue_span_read_pointer][22:1] -
            span_start[issue_span_read_pointer][22:1] + 1'b1;
        issue_span_read_pointer <= issue_span_read_pointer + 1'b1;
        issue_active <= 1;
      end
      if (prefetch_fire) begin
        issue_word_address <= issue_word_address + issue_burst_words;
        issue_words_remaining <= issue_words_remaining - issue_burst_words;
        if (issue_words_remaining == issue_burst_words) issue_active <= 0;
      end

      if (consume_span_load) begin
        consume_lane <= span_start[consume_span_read_pointer];
        consume_end_lane <= span_end[consume_span_read_pointer];
        consume_span_read_pointer <= consume_span_read_pointer + 1'b1;
        consume_active <= 1;
      end
      if (cached_fire) begin
        if (cached_finishes_span) begin
          consume_active <= 0;
        end else begin
          consume_lane <= consume_lane + 1'b1;
        end
      end else if (skip_word) begin
        if (skip_finishes_span) begin
          consume_active <= 0;
        end else begin
          consume_lane <= {expected_word_address + 1'b1, 1'b0};
        end
      end

      case ({span_fire, issue_span_load})
        2'b10: issue_span_count <= issue_span_count + 1'b1;
        2'b01: issue_span_count <= issue_span_count - 1'b1;
        default: issue_span_count <= issue_span_count;
      endcase
      case ({span_fire, consume_span_load})
        2'b10: consume_span_count <= consume_span_count + 1'b1;
        2'b01: consume_span_count <= consume_span_count - 1'b1;
        default: consume_span_count <= consume_span_count;
      endcase

      if (prefetch_response) begin
        data_memory[data_write_pointer] <= mem_readData;
        data_write_pointer <= data_write_pointer + 1'b1;
      end
      if (data_pop) data_read_pointer <= data_read_pointer + 1'b1;
      case ({prefetch_response, data_pop})
        2'b10: data_count <= data_count + 1'b1;
        2'b01: data_count <= data_count - 1'b1;
        default: data_count <= data_count;
      endcase
      case ({prefetch_fire, data_pop})
        2'b10: reserved_words <= reserved_words + issue_burst_words;
        2'b01: reserved_words <= reserved_words - 1'b1;
        2'b11: reserved_words <= reserved_words + issue_burst_words - 1'b1;
        default: reserved_words <= reserved_words;
      endcase

      if (memory_command_fire) begin
        route_direct[route_write_pointer] <= direct_command;
        route_beats[route_write_pointer] <= direct_command ? 7'd1 : issue_burst_words[6:0];
        route_write_pointer <= route_write_pointer + 1'b1;
      end
      if (memory_response) begin
        if (route_pop) begin
          route_read_pointer <= route_read_pointer + 1'b1;
        end else begin
          route_beats[route_read_pointer] <= route_head_beats - 1'b1;
        end
      end
      case ({memory_command_fire, route_pop})
        2'b10: route_count <= route_count + 1'b1;
        2'b01: route_count <= route_count - 1'b1;
        default: route_count <= route_count;
      endcase

      if (response_push) begin
        response_memory[response_write_pointer] <= response_push_data;
        response_write_pointer <= response_write_pointer + 1'b1;
      end
      if (response_pop) response_read_pointer <= response_read_pointer + 1'b1;
      case ({response_push, response_pop})
        2'b10: response_count <= response_count + 1'b1;
        2'b01: response_count <= response_count - 1'b1;
        default: response_count <= response_count;
      endcase
      case ({direct_fire, direct_response})
        2'b10: direct_outstanding <= direct_outstanding + 1'b1;
        2'b01: direct_outstanding <= direct_outstanding - 1'b1;
        default: direct_outstanding <= direct_outstanding;
      endcase
    end
  end

  wire unused = ^request[35:0];
endmodule
