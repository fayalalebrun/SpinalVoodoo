module XlsFramebufferReadCache #(
  parameter [31:0] BASE_ADDRESS = 32'h00000000,
  parameter integer ADDRESS_WORDS = 1048576
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
  input  wire [3:0]    update_mask
);
  localparam [1:0] IDLE = 2'd0;
  localparam [1:0] MISS_REQUEST = 2'd1;
  localparam [1:0] MISS_RESPONSE = 2'd2;
  localparam [1:0] HIT_RESPONSE = 2'd3;

  reg [1:0] state;
  reg line_valid;
  reg [15:0] line_tag;
  wire [31:0] line_read_data;
  reg [21:0] miss_address;
  reg [5:0] fill_index;
  reg fill_invalidated;
  reg response_pending;
  reg response_success;
  reg [31:0] response_data;
  reg saved_update_current;
  reg [31:0] saved_update_data;
  reg [3:0] saved_update_mask;

  wire [6:0] request_count = request[65:59];
  wire request_write = request[58];
  wire [21:0] request_address = request[57:36];
  wire request_legal = request_count == 7'd1 && !request_write &&
      request_address < ADDRESS_WORDS;
  wire request_hit = request_legal && line_valid && line_tag == request_address[21:6];
  wire update_hit = update_valid && line_valid && line_tag == update_address[21:6];
  wire update_miss_line = update_valid && miss_address[21:6] == update_address[21:6];
  wire update_current_word = update_hit && update_address == request_address;
  wire request_fire = request_valid && request_ready;
  wire line_read_enable = request_fire && request_hit;
  wire line_write_fill = state == MISS_RESPONSE && mem_readDataValid;
  wire line_write_update = state == IDLE && update_hit;
  wire line_write_enable = line_write_fill || line_write_update;
  wire [5:0] line_write_address = line_write_fill ? fill_index : update_address[5:0];
  wire [31:0] line_write_data = line_write_fill ? mem_readData : update_data;
  wire [3:0] line_write_mask = line_write_fill ? 4'hf : update_mask;

  assign miss_waiting = state == IDLE && request_valid && request_legal && !request_hit;
  assign fill_active = state == MISS_REQUEST || state == MISS_RESPONSE;
  assign fill_line_tag = miss_address[21:6];
  assign request_ready = state == IDLE && (!response_pending || response_ready) &&
      (!miss_waiting || !write_pending);
  assign response = {response_data, response_success};
  assign response_valid = response_pending;
  assign mem_read = state == MISS_REQUEST;
  assign mem_write = 1'b0;
  assign mem_burstCount = 11'd64;
  assign mem_address = BASE_ADDRESS + ({miss_address[21:6], 6'b0} << 2);
  assign mem_byteEnable = 4'hf;
  assign mem_writeData = 32'b0;

  XlsFramebufferLineMemory line_memory (
    .clk(clk), .read_enable(line_read_enable), .read_address(request_address[5:0]),
    .write_enable(line_write_enable), .write_address(line_write_address),
    .write_data(line_write_data), .write_mask(line_write_mask), .read_data(line_read_data)
  );

  always @(posedge clk) begin
    if (reset) begin
      state <= IDLE;
      line_valid <= 1'b0;
      line_tag <= 16'b0;
      miss_address <= 22'b0;
      fill_index <= 6'b0;
      fill_invalidated <= 1'b0;
      response_pending <= 1'b0;
      response_success <= 1'b0;
      response_data <= 32'b0;
      saved_update_current <= 1'b0;
      saved_update_data <= 32'b0;
      saved_update_mask <= 4'b0;
    end else begin
      if (response_pending && response_ready) response_pending <= 1'b0;
      if (invalidate) line_valid <= 1'b0;

      if ((state == MISS_REQUEST || state == MISS_RESPONSE) && (invalidate || update_miss_line))
        fill_invalidated <= 1'b1;

      case (state)
        IDLE: begin
          if (request_valid && request_ready) begin
            if (!request_legal) begin
              response_success <= 1'b0;
              response_data <= 32'b0;
              response_pending <= 1'b1;
            end else if (request_hit) begin
              saved_update_current <= update_current_word;
              saved_update_data <= update_data;
              saved_update_mask <= update_mask;
              state <= HIT_RESPONSE;
            end else begin
              miss_address <= request_address;
              fill_invalidated <= invalidate ||
                  (update_valid && request_address[21:6] == update_address[21:6]);
              state <= MISS_REQUEST;
            end
          end
        end
        MISS_REQUEST: begin
          if (mem_read && mem_waitRequestn) begin
            fill_index <= 0;
            state <= MISS_RESPONSE;
          end
        end
        MISS_RESPONSE: begin
          if (mem_readDataValid) begin
            if (fill_index == miss_address[5:0]) response_data <= mem_readData;
            if (fill_index == 6'd63) begin
              response_success <= 1'b1;
              response_pending <= 1'b1;
              if (!fill_invalidated && !invalidate && !update_miss_line) begin
                line_valid <= 1'b1;
                line_tag <= miss_address[21:6];
              end
              fill_index <= 0;
              state <= IDLE;
            end else begin
              fill_index <= fill_index + 1'b1;
            end
          end
        end
        HIT_RESPONSE: begin
          response_success <= 1'b1;
          response_data <= line_read_data;
          if (saved_update_current && saved_update_mask[0])
            response_data[7:0] <= saved_update_data[7:0];
          if (saved_update_current && saved_update_mask[1])
            response_data[15:8] <= saved_update_data[15:8];
          if (saved_update_current && saved_update_mask[2])
            response_data[23:16] <= saved_update_data[23:16];
          if (saved_update_current && saved_update_mask[3])
            response_data[31:24] <= saved_update_data[31:24];
          response_pending <= 1'b1;
          state <= IDLE;
        end
        default: state <= IDLE;
      endcase
    end
  end
endmodule

module XlsFramebufferLineMemory (
  input  wire        clk,
  input  wire        read_enable,
  input  wire [5:0]  read_address,
  input  wire        write_enable,
  input  wire [5:0]  write_address,
  input  wire [31:0] write_data,
  input  wire [3:0]  write_mask,
  output wire [31:0] read_data
);
  (* ramstyle = "M10K, no_rw_check" *) reg [7:0] byte0 [0:63];
  (* ramstyle = "M10K, no_rw_check" *) reg [7:0] byte1 [0:63];
  (* ramstyle = "M10K, no_rw_check" *) reg [7:0] byte2 [0:63];
  (* ramstyle = "M10K, no_rw_check" *) reg [7:0] byte3 [0:63];
  reg [31:0] read_data_register;

  assign read_data = read_data_register;

  always @(posedge clk) begin
    if (read_enable) begin
      read_data_register <= {
        byte3[read_address], byte2[read_address], byte1[read_address], byte0[read_address]
      };
    end
    if (write_enable) begin
      if (write_mask[0]) byte0[write_address] <= write_data[7:0];
      if (write_mask[1]) byte1[write_address] <= write_data[15:8];
      if (write_mask[2]) byte2[write_address] <= write_data[23:16];
      if (write_mask[3]) byte3[write_address] <= write_data[31:24];
    end
  end
endmodule
