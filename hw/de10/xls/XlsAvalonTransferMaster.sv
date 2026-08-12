module XlsAvalonTransferMaster #(
  parameter [31:0] BASE_ADDRESS = 32'h00000000,
  parameter integer SPAN_WORDS = 1
) (
  input  wire          clk,
  input  wire          reset,
  input  wire [65:0]   request,
  input  wire          request_valid,
  output wire          request_ready,
  output wire [2048:0] response,
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
  input  wire [31:0]   mem_readData
);
  localparam [1:0] IDLE = 2'd0;
  localparam [1:0] READ = 2'd1;
  localparam [1:0] RESPOND = 2'd2;

  reg [1:0] state;
  reg [6:0] words_left;
  reg [5:0] read_index;
  reg [2047:0] read_buffer;
  reg response_success;

  wire [6:0] word_count = request[65:59];
  wire command_write = request[58];
  wire [21:0] command_address = request[57:36];
  wire [31:0] command_data = request[35:4];
  wire [3:0] command_mask = request[3:0];
  wire [22:0] range_end = {1'b0, command_address} + word_count;
  wire range_valid = word_count != 0 && word_count <= 7'd64 &&
      command_address < SPAN_WORDS && range_end <= SPAN_WORDS &&
      (!command_write || word_count == 7'd1);
  wire launch = state == IDLE && request_valid && range_valid;

  assign request_ready = state == IDLE && (!range_valid || mem_waitRequestn);
  assign response = {read_buffer, response_success};
  assign response_valid = state == RESPOND;
  assign mem_read = launch && !command_write;
  assign mem_write = launch && command_write;
  assign mem_burstCount = {4'b0000, word_count};
  assign mem_address = BASE_ADDRESS + {8'b00000000, command_address, 2'b00};
  assign mem_byteEnable = command_write ? command_mask : 4'hf;
  assign mem_writeData = command_data;

  always @(posedge clk) begin
    if (reset) begin
      state <= IDLE;
      words_left <= 0;
      read_index <= 0;
      read_buffer <= 0;
      response_success <= 0;
    end else begin
      case (state)
        IDLE: begin
          if (request_valid && request_ready) begin
            read_buffer <= 0;
            response_success <= range_valid;
            if (!range_valid) begin
              state <= RESPOND;
            end else if (command_write) begin
              read_buffer[31:0] <= command_data;
              state <= RESPOND;
            end else if (mem_readDataValid) begin
              read_buffer[31:0] <= mem_readData;
              if (word_count == 7'd1) begin
                state <= RESPOND;
              end else begin
                words_left <= word_count - 1'b1;
                read_index <= 6'd1;
                state <= READ;
              end
            end else begin
              words_left <= word_count;
              read_index <= 0;
              state <= READ;
            end
          end
        end
        READ: begin
          if (mem_readDataValid) begin
            read_buffer[read_index * 32 +: 32] <= mem_readData;
            if (words_left == 7'd1) begin
              state <= RESPOND;
            end else begin
              words_left <= words_left - 1'b1;
              read_index <= read_index + 1'b1;
            end
          end
        end
        RESPOND: begin
          if (response_ready) begin
            state <= IDLE;
          end
        end
        default: state <= IDLE;
      endcase
    end
  end
endmodule
