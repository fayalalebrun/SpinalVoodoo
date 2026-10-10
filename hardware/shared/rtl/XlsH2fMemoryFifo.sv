module XlsH2fMemoryFifo #(
  parameter REQUEST_WIDTH = 60,
  parameter EGRESS_ADDRESS_WIDTH = 5,
  parameter RING_ADDRESS_WIDTH = 16,
  parameter [31:0] BASE_ADDRESS = 32'h3f000000
) (
  input  wire                     clk,
  input  wire                     reset,
  input  wire [REQUEST_WIDTH-1:0] ingress_data,
  input  wire                     ingress_empty,
  output wire                     ingress_pop,
  output wire [REQUEST_WIDTH-1:0] request_data,
  output wire                     request_empty,
  input  wire                     request_pop,
  output wire                     mem_read,
  output wire                     mem_write,
  input  wire                     mem_waitRequestn,
  output wire [10:0]              mem_burstCount,
  output wire [31:0]              mem_address,
  output wire [3:0]               mem_byteEnable,
  output wire [31:0]              mem_writeData,
  input  wire                     mem_readDataValid,
  input  wire [31:0]              mem_readData
);
  localparam RING_COUNT_WIDTH = RING_ADDRESS_WIDTH + 1;
  localparam EGRESS_COUNT_WIDTH = EGRESS_ADDRESS_WIDTH + 1;
  localparam [RING_COUNT_WIDTH-1:0] RING_CAPACITY =
      {1'b1, {RING_ADDRESS_WIDTH{1'b0}}};
  localparam [EGRESS_COUNT_WIDTH-1:0] EGRESS_CAPACITY =
      {1'b1, {EGRESS_ADDRESS_WIDTH{1'b0}}};
  localparam IDLE = 2'd0;
  localparam WRITE_DATA = 2'd1;
  localparam READ_REQUEST = 2'd2;
  localparam READ_DATA = 2'd3;

  reg [REQUEST_WIDTH-1:0] egress_memory [0:(1 << EGRESS_ADDRESS_WIDTH)-1];
  reg [EGRESS_ADDRESS_WIDTH-1:0] egress_write_pointer;
  reg [EGRESS_ADDRESS_WIDTH-1:0] egress_read_pointer;
  reg [EGRESS_COUNT_WIDTH-1:0] egress_count;
  reg [REQUEST_WIDTH-1:0] request_data_register;
  reg request_valid;
  reg [RING_ADDRESS_WIDTH-1:0] ring_write_pointer;
  reg [RING_ADDRESS_WIDTH-1:0] ring_read_pointer;
  reg [RING_COUNT_WIDTH-1:0] ring_count;
  reg [3:0] write_buffer_start;
  reg [4:0] write_count;
  reg [4:0] active_write_entries;
  reg [5:0] write_word;
  reg [31:0] read_low;
  reg read_high_phase;
  reg [7:0] read_words_remaining;
  reg [6:0] active_read_entries;
  reg [1:0] state;

  wire [RING_COUNT_WIDTH-1:0] read_entries_to_wrap =
      RING_CAPACITY - {1'b0, ring_read_pointer};
  wire [RING_COUNT_WIDTH-1:0] write_entries_to_wrap =
      RING_CAPACITY - {1'b0, ring_write_pointer};
  wire [6:0] ring_available = ring_count > 64 ? 7'd64 : ring_count[6:0];
  wire [6:0] wrap_available = read_entries_to_wrap > 64 ?
      7'd64 : read_entries_to_wrap[6:0];
  wire [6:0] egress_space = EGRESS_CAPACITY - egress_count;
  wire [4:0] write_wrap_available = write_entries_to_wrap > 16 ?
      5'd16 : write_entries_to_wrap[4:0];
  wire [4:0] write_entries = write_count < write_wrap_available ?
      write_count : write_wrap_available;
  wire [6:0] read_limit = ring_available < wrap_available ? ring_available : wrap_available;
  wire [6:0] read_entries = read_limit < egress_space ? read_limit : egress_space;
  wire start_read = state == IDLE && ring_count != 0 && read_entries != 0 &&
      egress_count <= (EGRESS_CAPACITY >> 1);
  wire write_fits = ring_count + write_count <= RING_CAPACITY;
  wire start_write = state == IDLE && !start_read && write_count != 0 && write_fits &&
      (write_count == 16 || ingress_empty);
  wire start_bypass = state == IDLE && !start_read && !start_write && ring_count == 0 &&
      write_count == 0 && !ingress_empty && egress_count != EGRESS_CAPACITY;
  wire stage_write = state == IDLE && !start_read && !start_write && !start_bypass &&
      !ingress_empty && write_count != 16;
  wire write_fire = state == WRITE_DATA && mem_waitRequestn;
  wire write_last = write_word == ({active_write_entries, 1'b0} - 1'b1);
  wire write_memory_read = start_write || (write_fire && write_word[0] && !write_last);
  wire [3:0] write_memory_read_address = start_write ? write_buffer_start :
      write_buffer_start + write_word[5:1] + 1'b1;
  wire [3:0] write_memory_write_address = write_buffer_start + write_count[3:0];
  wire [REQUEST_WIDTH-1:0] write_memory_data;
  wire read_request_fire = state == READ_REQUEST && mem_waitRequestn;
  wire egress_read_push = state == READ_DATA && mem_readDataValid && read_high_phase;
  wire egress_push = start_bypass || egress_read_push;
  wire request_advance = !request_valid || request_pop;
  wire egress_pop = request_advance && egress_count != 0;
  wire [REQUEST_WIDTH-1:0] egress_push_data = start_bypass ? ingress_data :
      {mem_readData[REQUEST_WIDTH-33:0], read_low};

  XlsH2fWriteMemory #(.REQUEST_WIDTH(REQUEST_WIDTH)) write_memory (
    .clk(clk),
    .read_enable(write_memory_read),
    .read_address(write_memory_read_address),
    .write_enable(stage_write),
    .write_address(write_memory_write_address),
    .write_data(ingress_data),
    .read_data(write_memory_data)
  );

  assign ingress_pop = start_bypass || stage_write;
  assign request_data = request_data_register;
  assign request_empty = !request_valid;
  assign mem_read = !reset && state == READ_REQUEST;
  assign mem_write = !reset && state == WRITE_DATA;
  assign mem_burstCount = state == READ_REQUEST ? {3'b0, active_read_entries, 1'b0} :
      state == WRITE_DATA ? {5'b0, active_write_entries, 1'b0} : 11'd1;
  assign mem_address = state == READ_REQUEST ?
      BASE_ADDRESS + ({16'b0, ring_read_pointer} << 3) :
      BASE_ADDRESS + ({16'b0, ring_write_pointer} << 3);
  assign mem_byteEnable = 4'hf;
  assign mem_writeData = write_word[0] ?
      {{(64-REQUEST_WIDTH){1'b0}}, write_memory_data[REQUEST_WIDTH-1:32]} :
      write_memory_data[31:0];

  always @(posedge clk) begin
    if (egress_pop) request_data_register <= egress_memory[egress_read_pointer];
  end

  always @(posedge clk or posedge reset) begin
    if (reset) begin
      egress_write_pointer <= 0;
      egress_read_pointer <= 0;
      egress_count <= 0;
      request_valid <= 0;
      ring_write_pointer <= 0;
      ring_read_pointer <= 0;
      ring_count <= 0;
      write_buffer_start <= 0;
      write_count <= 0;
      active_write_entries <= 0;
      write_word <= 0;
      read_low <= 0;
      read_high_phase <= 0;
      read_words_remaining <= 0;
      active_read_entries <= 0;
      state <= IDLE;
    end else begin
      if (request_advance) request_valid <= egress_count != 0;
      if (egress_push) begin
        egress_memory[egress_write_pointer] <= egress_push_data;
        egress_write_pointer <= egress_write_pointer + 1'b1;
      end
      if (egress_pop) egress_read_pointer <= egress_read_pointer + 1'b1;
      case ({egress_push, egress_pop})
        2'b10: egress_count <= egress_count + 1'b1;
        2'b01: egress_count <= egress_count - 1'b1;
        default: egress_count <= egress_count;
      endcase

      if (stage_write) begin
        write_count <= write_count + 1'b1;
      end else if (start_write) begin
        active_write_entries <= write_entries;
        write_word <= 0;
        state <= WRITE_DATA;
      end else if (write_fire) begin
        if (write_last) begin
          ring_write_pointer <= ring_write_pointer + active_write_entries;
          ring_count <= ring_count + active_write_entries;
          write_buffer_start <= write_buffer_start + active_write_entries[3:0];
          write_count <= write_count - active_write_entries;
          state <= IDLE;
        end else begin
          write_word <= write_word + 1'b1;
        end
      end else if (start_read) begin
        active_read_entries <= read_entries;
        state <= READ_REQUEST;
      end else if (read_request_fire) begin
        ring_read_pointer <= ring_read_pointer + active_read_entries;
        ring_count <= ring_count - active_read_entries;
        read_words_remaining <= {active_read_entries, 1'b0};
        read_high_phase <= 1'b0;
        state <= READ_DATA;
      end else if (state == READ_DATA && mem_readDataValid) begin
        read_words_remaining <= read_words_remaining - 1'b1;
        if (!read_high_phase) begin
          read_low <= mem_readData;
          read_high_phase <= 1'b1;
        end else begin
          read_high_phase <= 1'b0;
        end
        if (read_words_remaining == 1) state <= IDLE;
      end
    end
  end
endmodule

module XlsH2fWriteMemory #(
  parameter REQUEST_WIDTH = 60
) (
  input  wire                     clk,
  input  wire                     read_enable,
  input  wire [3:0]               read_address,
  input  wire                     write_enable,
  input  wire [3:0]               write_address,
  input  wire [REQUEST_WIDTH-1:0] write_data,
  output wire [REQUEST_WIDTH-1:0] read_data
);
  (* ramstyle = "M10K, no_rw_check" *) reg [REQUEST_WIDTH-1:0] memory [0:15];
  reg [REQUEST_WIDTH-1:0] read_data_register;

  assign read_data = read_data_register;

  always @(posedge clk) begin
    if (read_enable) read_data_register <= memory[read_address];
    if (write_enable) memory[write_address] <= write_data;
  end
endmodule
