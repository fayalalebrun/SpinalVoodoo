module XlsFramebufferReadPrefetch #(
  parameter [31:0] BASE_ADDRESS = 32'h00000000,
  parameter integer ADDRESS_WORDS = 1048576,
  parameter integer LINE_WORDS = 2,
  parameter integer LINE_COUNT = 4,
  parameter integer REQUEST_DEPTH = 32
) (
  input  wire          clk,
  input  wire          reset,
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
  input  wire          write_pending,
  output wire          miss_waiting,
  output wire          fill_active,
  output wire [15:0]   fill_line_tag,
  input  wire          update_valid,
  input  wire [21:0]   update_address,
  input  wire [31:0]   update_data,
  input  wire [3:0]    update_mask,
  input  wire          debug_clear,
  input  wire [3:0]    debug_read_index,
  output reg  [31:0]   debug_read_data
);
  localparam integer OFFSET_BITS = $clog2(LINE_WORDS);
  localparam integer SLOT_BITS = $clog2(LINE_COUNT);
  localparam integer REQUEST_PTR_BITS = $clog2(REQUEST_DEPTH);
  localparam integer REQUEST_COUNT_BITS = $clog2(REQUEST_DEPTH + 1);
  localparam integer FILL_COUNT_BITS = $clog2(LINE_COUNT + 1);
  localparam integer REF_BITS = $clog2(REQUEST_DEPTH + 1);

  wire [6:0] request_count = request[65:59];
  wire request_write = request[58];
  wire [21:0] request_address = request[57:36];
  wire request_legal = request_count == 7'd1 && !request_write &&
      request_address < ADDRESS_WORDS;
  wire [21:OFFSET_BITS] request_tag = request_address[21:OFFSET_BITS];
  wire [OFFSET_BITS-1:0] request_offset = request_address[OFFSET_BITS-1:0];

  reg [21:OFFSET_BITS] line_tag [0:LINE_COUNT-1];
  reg line_valid [0:LINE_COUNT-1];
  reg line_filling [0:LINE_COUNT-1];
  reg [REF_BITS-1:0] line_references [0:LINE_COUNT-1];
  reg [31:0] line_data [0:LINE_COUNT-1][0:LINE_WORDS-1];
  reg [SLOT_BITS-1:0] replacement_pointer;

  reg descriptor_failure [0:REQUEST_DEPTH-1];
  reg [SLOT_BITS-1:0] descriptor_slot [0:REQUEST_DEPTH-1];
  reg [OFFSET_BITS-1:0] descriptor_offset [0:REQUEST_DEPTH-1];
  reg [REQUEST_PTR_BITS-1:0] descriptor_write_pointer;
  reg [REQUEST_PTR_BITS-1:0] descriptor_read_pointer;
  reg [REQUEST_COUNT_BITS-1:0] descriptor_count;

  reg [SLOT_BITS-1:0] fill_slots [0:LINE_COUNT-1];
  reg [SLOT_BITS-1:0] fill_write_pointer;
  reg [SLOT_BITS-1:0] fill_read_pointer;
  reg [FILL_COUNT_BITS-1:0] fill_count;
  reg [OFFSET_BITS-1:0] fill_beat;

  reg [23:0] debug_request_stall_cycles;
  reg [23:0] debug_descriptor_stall_cycles;
  reg [23:0] debug_fill_stall_cycles;
  reg [23:0] debug_pinned_stall_cycles;
  reg [23:0] debug_avalon_wait_cycles;
  reg [23:0] debug_head_wait_cycles;
  reg [23:0] debug_response_stall_cycles;
  reg [23:0] debug_hits;
  reg [23:0] debug_misses;
  reg [23:0] debug_fill_busy_cycles;

  reg hit_found;
  reg [SLOT_BITS-1:0] hit_slot;
  reg replacement_found;
  reg [SLOT_BITS-1:0] replacement_slot;
  integer i;
  integer scan_index;

  always @(*) begin
    hit_found = 1'b0;
    hit_slot = 0;
    for (i = 0; i < LINE_COUNT; i = i + 1) begin
      if (!hit_found && (line_valid[i] || line_filling[i]) && line_tag[i] == request_tag) begin
        hit_found = 1'b1;
        hit_slot = i[SLOT_BITS-1:0];
      end
    end
  end

  always @(*) begin
    replacement_found = 1'b0;
    replacement_slot = 0;
    for (i = 0; i < LINE_COUNT; i = i + 1) begin
      scan_index = (replacement_pointer + i) & (LINE_COUNT - 1);
      if (!replacement_found && !line_filling[scan_index] &&
          line_references[scan_index] == 0) begin
        replacement_found = 1'b1;
        replacement_slot = scan_index[SLOT_BITS-1:0];
      end
    end
  end

  wire descriptor_full = descriptor_count == REQUEST_DEPTH;
  wire fill_full = fill_count == LINE_COUNT;
  wire miss = request_legal && !hit_found;
  wire request_can_complete = !request_legal || hit_found ||
      (replacement_found && !fill_full && mem_waitRequestn);
  assign request_ready = !reset && !descriptor_full && request_can_complete;
  wire request_fire = request_valid && request_ready;
  wire miss_fire = request_fire && miss;
  wire [SLOT_BITS-1:0] request_slot = hit_found ? hit_slot : replacement_slot;

  assign mem_read = !reset && request_valid && !descriptor_full && miss &&
      replacement_found && !fill_full;
  assign mem_write = 1'b0;
  assign mem_burstCount = LINE_WORDS;
  assign mem_address = BASE_ADDRESS +
      ({request_address[21:OFFSET_BITS], {OFFSET_BITS{1'b0}}} << 2);
  assign mem_byteEnable = 4'hf;
  assign mem_writeData = 32'b0;

  wire head_failure = descriptor_failure[descriptor_read_pointer];
  wire [SLOT_BITS-1:0] head_slot = descriptor_slot[descriptor_read_pointer];
  wire [OFFSET_BITS-1:0] head_offset = descriptor_offset[descriptor_read_pointer];
  wire head_ready = head_failure || line_valid[head_slot];
  assign response_valid = descriptor_count != 0 && head_ready;
  assign response = head_failure ? 33'b0 : {line_data[head_slot][head_offset], 1'b1};
  wire response_fire = response_valid && response_ready;

  wire fill_response = mem_readDataValid && fill_count != 0;
  wire fill_last = fill_beat == LINE_WORDS - 1;
  wire fill_pop = fill_response && fill_last;
  wire [SLOT_BITS-1:0] active_fill_slot = fill_slots[fill_read_pointer];

  assign miss_waiting = 1'b0;
  assign fill_active = 1'b0;
  assign fill_line_tag = 16'b0;

  always @(*) begin
    case (debug_read_index)
      0: debug_read_data = {8'b0, debug_request_stall_cycles};
      1: debug_read_data = {8'b0, debug_descriptor_stall_cycles};
      2: debug_read_data = {8'b0, debug_fill_stall_cycles};
      3: debug_read_data = {8'b0, debug_pinned_stall_cycles};
      4: debug_read_data = {8'b0, debug_avalon_wait_cycles};
      5: debug_read_data = {8'b0, debug_head_wait_cycles};
      6: debug_read_data = {8'b0, debug_response_stall_cycles};
      7: debug_read_data = {8'b0, debug_hits};
      8: debug_read_data = {8'b0, debug_misses};
      9: debug_read_data = {8'b0, debug_fill_busy_cycles};
      default: debug_read_data = 0;
    endcase
  end

  integer slot;
  always @(posedge clk) begin
    if (reset) begin
      replacement_pointer <= 0;
      descriptor_write_pointer <= 0;
      descriptor_read_pointer <= 0;
      descriptor_count <= 0;
      fill_write_pointer <= 0;
      fill_read_pointer <= 0;
      fill_count <= 0;
      fill_beat <= 0;
      debug_request_stall_cycles <= 0;
      debug_descriptor_stall_cycles <= 0;
      debug_fill_stall_cycles <= 0;
      debug_pinned_stall_cycles <= 0;
      debug_avalon_wait_cycles <= 0;
      debug_head_wait_cycles <= 0;
      debug_response_stall_cycles <= 0;
      debug_hits <= 0;
      debug_misses <= 0;
      debug_fill_busy_cycles <= 0;
      for (slot = 0; slot < LINE_COUNT; slot = slot + 1) begin
        line_tag[slot] <= 0;
        line_valid[slot] <= 0;
        line_filling[slot] <= 0;
        line_references[slot] <= 0;
      end
    end else begin
      if (debug_clear) begin
        debug_request_stall_cycles <= 0;
        debug_descriptor_stall_cycles <= 0;
        debug_fill_stall_cycles <= 0;
        debug_pinned_stall_cycles <= 0;
        debug_avalon_wait_cycles <= 0;
        debug_head_wait_cycles <= 0;
        debug_response_stall_cycles <= 0;
        debug_hits <= 0;
        debug_misses <= 0;
        debug_fill_busy_cycles <= 0;
      end else begin
        if (request_valid && !request_ready && !(&debug_request_stall_cycles))
          debug_request_stall_cycles <= debug_request_stall_cycles + 1'b1;
        if (request_valid && descriptor_full && !(&debug_descriptor_stall_cycles))
          debug_descriptor_stall_cycles <= debug_descriptor_stall_cycles + 1'b1;
        if (request_valid && !descriptor_full && request_legal && miss &&
            !replacement_found && fill_full && !(&debug_fill_stall_cycles))
          debug_fill_stall_cycles <= debug_fill_stall_cycles + 1'b1;
        if (request_valid && !descriptor_full && request_legal && miss &&
            !replacement_found && !fill_full && !(&debug_pinned_stall_cycles))
          debug_pinned_stall_cycles <= debug_pinned_stall_cycles + 1'b1;
        if (mem_read && !mem_waitRequestn && !(&debug_avalon_wait_cycles))
          debug_avalon_wait_cycles <= debug_avalon_wait_cycles + 1'b1;
        if (descriptor_count != 0 && !head_ready && !(&debug_head_wait_cycles))
          debug_head_wait_cycles <= debug_head_wait_cycles + 1'b1;
        if (response_valid && !response_ready && !(&debug_response_stall_cycles))
          debug_response_stall_cycles <= debug_response_stall_cycles + 1'b1;
        if (request_fire && request_legal && hit_found && !(&debug_hits))
          debug_hits <= debug_hits + 1'b1;
        if (miss_fire && !(&debug_misses)) debug_misses <= debug_misses + 1'b1;
        if (fill_count != 0 && !(&debug_fill_busy_cycles))
          debug_fill_busy_cycles <= debug_fill_busy_cycles + 1'b1;
      end
      if (invalidate) begin
        for (slot = 0; slot < LINE_COUNT; slot = slot + 1) begin
          if (line_references[slot] == 0 && !line_filling[slot]) line_valid[slot] <= 0;
        end
      end

      if (request_fire) begin
        descriptor_failure[descriptor_write_pointer] <= !request_legal;
        descriptor_slot[descriptor_write_pointer] <= request_slot;
        descriptor_offset[descriptor_write_pointer] <= request_offset;
        descriptor_write_pointer <= descriptor_write_pointer + 1'b1;
      end
      if (response_fire) descriptor_read_pointer <= descriptor_read_pointer + 1'b1;
      case ({request_fire, response_fire})
        2'b10: descriptor_count <= descriptor_count + 1'b1;
        2'b01: descriptor_count <= descriptor_count - 1'b1;
        default: descriptor_count <= descriptor_count;
      endcase

      if (miss_fire) begin
        line_tag[replacement_slot] <= request_tag;
        line_valid[replacement_slot] <= 0;
        line_filling[replacement_slot] <= 1;
        replacement_pointer <= replacement_slot + 1'b1;
        fill_slots[fill_write_pointer] <= replacement_slot;
        fill_write_pointer <= fill_write_pointer + 1'b1;
      end
      if (fill_pop) fill_read_pointer <= fill_read_pointer + 1'b1;
      case ({miss_fire, fill_pop})
        2'b10: fill_count <= fill_count + 1'b1;
        2'b01: fill_count <= fill_count - 1'b1;
        default: fill_count <= fill_count;
      endcase

      if (fill_response) begin
        line_data[active_fill_slot][fill_beat] <= mem_readData;
        if (fill_last) begin
          line_valid[active_fill_slot] <= 1;
          line_filling[active_fill_slot] <= 0;
          fill_beat <= 0;
        end else begin
          fill_beat <= fill_beat + 1'b1;
        end
      end

      for (slot = 0; slot < LINE_COUNT; slot = slot + 1) begin
        case ({request_fire && request_legal && request_slot == slot,
               response_fire && !head_failure && head_slot == slot})
          2'b10: line_references[slot] <= line_references[slot] + 1'b1;
          2'b01: line_references[slot] <= line_references[slot] - 1'b1;
          default: line_references[slot] <= line_references[slot];
        endcase
      end
    end
  end

  wire unused = write_pending ^ update_valid ^ ^update_address ^ ^update_data ^ ^update_mask;
endmodule
