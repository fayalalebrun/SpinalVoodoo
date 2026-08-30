module XlsFramebufferWriteBuffer #(
  parameter [31:0] BASE_ADDRESS = 32'h00000000,
  parameter integer SPAN_WORDS = 1048576,
  parameter integer RESPONSE_DEPTH = 128,
  parameter integer MAX_REQUESTS_PER_RUN = 64
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
  input  wire          flush,
  input  wire          block_line_valid,
  input  wire [15:0]   block_line_tag,
  output wire          busy,
  output wire          update_valid,
  output wire [21:0]   update_address,
  output wire [31:0]   update_data,
  output wire [3:0]    update_mask,
  output wire          pending_line0_valid,
  output wire [15:0]   pending_line0_tag,
  output wire          pending_line1_valid,
  output wire [15:0]   pending_line1_tag
);
  localparam [2:0] EMPTY = 3'd0;
  localparam [2:0] FILL = 3'd1;
  localparam [2:0] FINALIZE = 3'd2;
  localparam [2:0] READY = 3'd3;
  localparam [2:0] DRAIN = 3'd4;
   localparam integer RESPONSE_PTR_WIDTH = $clog2(RESPONSE_DEPTH);

  reg [2:0] slot0_state;
  reg [2:0] slot1_state;
  reg [21:0] slot0_start;
  reg [21:0] slot1_start;
  reg [6:0] slot0_count;
  reg [6:0] slot1_count;
  reg [6:0] slot0_requests;
  reg [6:0] slot1_requests;
  reg [31:0] slot0_current_data;
  reg [31:0] slot1_current_data;
  reg [3:0] slot0_current_mask;
  reg [3:0] slot1_current_mask;
  reg [6:0] slot0_current_acks;
  reg [6:0] slot1_current_acks;
  wire [42:0] slot0_read_data;
  wire [42:0] slot1_read_data;
  reg drain_active;
  reg drain_sel;
  reg drain_preference;
   reg drain_data_valid;
    reg [5:0] drain_index;

   wire [32:0] response_memory_read_data;
  reg response_head_valid;
  reg [RESPONSE_PTR_WIDTH-1:0] response_write_ptr;
  reg [RESPONSE_PTR_WIDTH-1:0] response_read_ptr;
  reg [RESPONSE_PTR_WIDTH:0] response_queued;
  reg [RESPONSE_PTR_WIDTH:0] response_completed;


  wire [6:0] request_count = request[65:59];
  wire request_write = request[58];
  wire [21:0] request_address = request[57:36];
  wire [31:0] request_data = request[35:4];
  wire [3:0] request_mask = request[3:0];
  wire request_legal = request_count == 7'd1 && request_write &&
      request_address < SPAN_WORDS;
  wire slot0_fill = slot0_state == FILL;
  wire slot1_fill = slot1_state == FILL;
  wire has_fill = slot0_fill || slot1_fill;
  wire fill_sel = slot1_fill;
  wire [21:0] fill_start = fill_sel ? slot1_start : slot0_start;
  wire [6:0] fill_count = fill_sel ? slot1_count : slot0_count;
  wire [6:0] fill_requests = fill_sel ? slot1_requests : slot0_requests;
  wire [21:0] fill_last = fill_start + fill_count - 1'b1;
  wire request_merge = has_fill && request_address == fill_last;
  wire request_append = has_fill && fill_count < 7'd64 &&
      request_address == fill_start + fill_count &&
      request_address[21:6] == fill_start[21:6];
  wire request_fits = request_merge || request_append;
  wire other_empty = fill_sel ? slot0_state == EMPTY : slot1_state == EMPTY;
  wire any_empty = slot0_state == EMPTY || slot1_state == EMPTY;
  wire slots_empty = slot0_state == EMPTY && slot1_state == EMPTY && !drain_active;
  wire response_space = response_queued < RESPONSE_DEPTH;
  wire request_blocked = block_line_valid && request_address[21:6] == block_line_tag;
  wire legal_ready = !flush && !request_blocked && response_space &&
      ((has_fill && (request_fits || other_empty)) || (!has_fill && any_empty));
  wire illegal_ready = !flush && response_space && slots_empty;
  wire request_fire = request_valid && request_ready;
  wire response_fire = response_valid && response_ready;
  wire drain_fire = mem_write && mem_waitRequestn;
  wire drain_last = drain_sel ? drain_index == slot1_count - 1'b1 :
      drain_index == slot0_count - 1'b1;
  wire start_slot0 = !drain_active && slot0_state == READY &&
      (!drain_preference || slot1_state != READY);
  wire start_slot1 = !drain_active && slot1_state == READY &&
      (drain_preference || slot0_state != READY);
  wire slot0_write_enable = slot0_state == FINALIZE ||
      (request_fire && request_legal && has_fill && !fill_sel && request_append);
  wire slot1_write_enable = slot1_state == FINALIZE ||
      (request_fire && request_legal && has_fill && fill_sel && request_append);
  wire [5:0] slot0_write_address = slot0_count[5:0] - 1'b1;
  wire [5:0] slot1_write_address = slot1_count[5:0] - 1'b1;
  wire slot0_read_enable = start_slot0 ||
      (drain_fire && !drain_last && !drain_sel);
  wire slot1_read_enable = start_slot1 ||
      (drain_fire && !drain_last && drain_sel);
  wire [5:0] slot_read_address = drain_active ? drain_index + 1'b1 : 6'b0;
  wire [42:0] drain_word = drain_sel ? slot1_read_data : slot0_read_data;
  wire [7:0] completion_add = request_fire ? 8'd1 : 8'd0;
    wire close_fill = has_fill && (flush || !response_space ||
        (request_valid && (!request_legal || (!request_fits && !other_empty))) ||
        (!request_valid && !drain_active));
  wire response_fetch = (!response_head_valid || response_fire) &&
      response_completed > (response_fire ? 1'b1 : 1'b0);

  assign request_ready = request_legal ? legal_ready : illegal_ready;
  assign response = response_memory_read_data;
  assign response_valid = response_head_valid;
  assign mem_read = 1'b0;
  assign mem_write = drain_active && drain_data_valid;
  assign mem_burstCount = drain_sel ? {4'b0, slot1_count} : {4'b0, slot0_count};
  assign mem_address = BASE_ADDRESS +
      ((drain_sel ? slot1_start : slot0_start) << 2);
  assign mem_byteEnable = drain_word[35:32];
  assign mem_writeData = drain_word[31:0];
  assign busy = !slots_empty;
  assign update_valid = request_fire && request_legal;
  assign update_address = request_address;
  assign update_data = request_data;
  assign update_mask = request_mask;
  assign pending_line0_valid = slot0_state != EMPTY;
  assign pending_line0_tag = slot0_start[21:6];
  assign pending_line1_valid = slot1_state != EMPTY;
  assign pending_line1_tag = slot1_start[21:6];

  XlsFramebufferRunMemory slot0_storage (
    .clk(clk), .read_enable(slot0_read_enable), .read_address(slot_read_address),
    .write_enable(slot0_write_enable), .write_address(slot0_write_address),
    .write_data({slot0_current_acks, slot0_current_mask, slot0_current_data}),
    .read_data(slot0_read_data)
  );

  XlsFramebufferRunMemory slot1_storage (
    .clk(clk), .read_enable(slot1_read_enable), .read_address(slot_read_address),
    .write_enable(slot1_write_enable), .write_address(slot1_write_address),
    .write_data({slot1_current_acks, slot1_current_mask, slot1_current_data}),
    .read_data(slot1_read_data)
  );

  XlsFramebufferResponseMemory #(.DEPTH(RESPONSE_DEPTH)) response_storage (
    .clk(clk), .read_enable(response_fetch),
    .read_address(response_read_ptr + (response_fire ? 1'b1 : 1'b0)),
    .write_enable(request_fire), .write_address(response_write_ptr),
    .write_data({request_data, request_legal}), .read_data(response_memory_read_data)
  );

  always @(posedge clk) begin
    if (reset) begin
      slot0_state <= EMPTY;
      slot1_state <= EMPTY;
      slot0_start <= 0;
      slot1_start <= 0;
      slot0_count <= 0;
      slot1_count <= 0;
      slot0_requests <= 0;
      slot1_requests <= 0;
      slot0_current_data <= 0;
      slot1_current_data <= 0;
      slot0_current_mask <= 0;
      slot1_current_mask <= 0;
      slot0_current_acks <= 0;
      slot1_current_acks <= 0;
      drain_active <= 0;
      drain_sel <= 0;
      drain_preference <= 0;
       drain_data_valid <= 0;
       drain_index <= 0;
      response_head_valid <= 0;
      response_write_ptr <= 0;
      response_read_ptr <= 0;
      response_queued <= 0;
      response_completed <= 0;
     end else begin
       response_queued <= response_queued + (request_fire ? 1'b1 : 1'b0) -
          (response_fire ? 1'b1 : 1'b0);
      response_completed <= response_completed + completion_add -
          (response_fire ? 1'b1 : 1'b0);

      if (request_fire) response_write_ptr <= response_write_ptr + 1'b1;
      if (response_fire) response_read_ptr <= response_read_ptr + 1'b1;
      if (response_fetch) response_head_valid <= 1'b1;
      else if (response_fire) response_head_valid <= 1'b0;

      if (slot0_state == FINALIZE) slot0_state <= READY;
      if (slot1_state == FINALIZE) slot1_state <= READY;
      if (close_fill) begin
        if (fill_sel) slot1_state <= FINALIZE;
        else slot0_state <= FINALIZE;
      end

       if (request_fire && request_legal) begin
         if (!has_fill) begin
           if (slot0_state == EMPTY) begin
             slot0_state <= MAX_REQUESTS_PER_RUN == 1 ? FINALIZE : FILL;
             slot0_start <= request_address;
             slot0_count <= 7'd1;
             slot0_requests <= 7'd1;
             slot0_current_data <= request_data;
             slot0_current_mask <= request_mask;
             slot0_current_acks <= 7'd1;
           end else begin
             slot1_state <= MAX_REQUESTS_PER_RUN == 1 ? FINALIZE : FILL;
             slot1_start <= request_address;
             slot1_count <= 7'd1;
             slot1_requests <= 7'd1;
             slot1_current_data <= request_data;
             slot1_current_mask <= request_mask;
             slot1_current_acks <= 7'd1;
           end
         end else if (request_fits) begin
          if (!fill_sel) begin
            slot0_requests <= slot0_requests + 1'b1;
             if (request_merge) begin
               if (request_mask[0]) slot0_current_data[7:0] <= request_data[7:0];
               if (request_mask[1]) slot0_current_data[15:8] <= request_data[15:8];
               if (request_mask[2]) slot0_current_data[23:16] <= request_data[23:16];
               if (request_mask[3]) slot0_current_data[31:24] <= request_data[31:24];
               slot0_current_mask <= slot0_current_mask | request_mask;
              slot0_current_acks <= slot0_current_acks + 1'b1;
            end else begin
               slot0_current_data <= request_data;
               slot0_current_mask <= request_mask;
              slot0_current_acks <= 7'd1;
              slot0_count <= slot0_count + 1'b1;
            end
            if (fill_requests == MAX_REQUESTS_PER_RUN - 1 ||
                 (request_append && fill_count == 7'd63)) slot0_state <= FINALIZE;
          end else begin
            slot1_requests <= slot1_requests + 1'b1;
             if (request_merge) begin
               if (request_mask[0]) slot1_current_data[7:0] <= request_data[7:0];
               if (request_mask[1]) slot1_current_data[15:8] <= request_data[15:8];
               if (request_mask[2]) slot1_current_data[23:16] <= request_data[23:16];
               if (request_mask[3]) slot1_current_data[31:24] <= request_data[31:24];
               slot1_current_mask <= slot1_current_mask | request_mask;
              slot1_current_acks <= slot1_current_acks + 1'b1;
            end else begin
               slot1_current_data <= request_data;
               slot1_current_mask <= request_mask;
              slot1_current_acks <= 7'd1;
              slot1_count <= slot1_count + 1'b1;
            end
            if (fill_requests == MAX_REQUESTS_PER_RUN - 1 ||
                 (request_append && fill_count == 7'd63)) slot1_state <= FINALIZE;
          end
        end else if (!fill_sel) begin
          slot0_state <= FINALIZE;
           slot1_state <= MAX_REQUESTS_PER_RUN == 1 ? FINALIZE : FILL;
           slot1_start <= request_address;
           slot1_count <= 7'd1;
           slot1_requests <= 7'd1;
           slot1_current_data <= request_data;
           slot1_current_mask <= request_mask;
          slot1_current_acks <= 7'd1;
        end else begin
          slot1_state <= FINALIZE;
           slot0_state <= MAX_REQUESTS_PER_RUN == 1 ? FINALIZE : FILL;
           slot0_start <= request_address;
           slot0_count <= 7'd1;
           slot0_requests <= 7'd1;
           slot0_current_data <= request_data;
           slot0_current_mask <= request_mask;
          slot0_current_acks <= 7'd1;
        end
      end

      if (start_slot0) begin
        drain_active <= 1'b1;
        drain_data_valid <= 1'b1;
        drain_sel <= 1'b0;
        drain_index <= 0;
        slot0_state <= DRAIN;
      end else if (start_slot1) begin
        drain_active <= 1'b1;
        drain_data_valid <= 1'b1;
        drain_sel <= 1'b1;
        drain_index <= 0;
        slot1_state <= DRAIN;
      end else if (drain_fire) begin
        if (drain_last) begin
          drain_active <= 1'b0;
          drain_data_valid <= 1'b0;
          drain_preference <= ~drain_sel;
          drain_index <= 0;
          if (drain_sel) slot1_state <= EMPTY;
          else slot0_state <= EMPTY;
        end else begin
          drain_index <= drain_index + 1'b1;
        end
      end
    end
  end

  wire unused = mem_readDataValid ^ ^mem_readData;
endmodule

module XlsFramebufferRunMemory (
  input  wire        clk,
  input  wire        read_enable,
  input  wire [5:0]  read_address,
  input  wire        write_enable,
  input  wire [5:0]  write_address,
  input  wire [42:0] write_data,
  output wire [42:0] read_data
);
  (* ramstyle = "M10K, no_rw_check" *) reg [42:0] memory [0:63];
  reg [42:0] read_data_register;

  assign read_data = read_data_register;

  always @(posedge clk) begin
    if (read_enable) read_data_register <= memory[read_address];
    if (write_enable) memory[write_address] <= write_data;
  end
endmodule

module XlsFramebufferResponseMemory #(
  parameter integer DEPTH = 128,
  parameter integer ADDRESS_WIDTH = $clog2(DEPTH)
) (
  input  wire                     clk,
  input  wire                     read_enable,
  input  wire [ADDRESS_WIDTH-1:0] read_address,
  input  wire                     write_enable,
  input  wire [ADDRESS_WIDTH-1:0] write_address,
  input  wire [32:0]              write_data,
  output wire [32:0]              read_data
);
  (* ramstyle = "M10K, no_rw_check" *) reg [32:0] memory [0:DEPTH-1];
  reg [32:0] read_data_register;

  assign read_data = read_data_register;

  always @(posedge clk) begin
    if (read_enable) read_data_register <= memory[read_address];
    if (write_enable) memory[write_address] <= write_data;
  end
endmodule
