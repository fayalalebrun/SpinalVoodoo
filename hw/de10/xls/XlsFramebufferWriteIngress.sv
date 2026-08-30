module XlsFramebufferWriteIngress #(
  parameter [31:0] BASE_ADDRESS = 32'h00000000,
  parameter integer SPAN_WORDS = 1048576,
  parameter integer RESPONSE_DEPTH = 128,
  parameter integer INGRESS_DEPTH = 256,
  parameter integer START_WATERMARK = 8,
  parameter integer IDLE_TIMEOUT = 8
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
  output wire [15:0]   pending_line1_tag,
  output wire [8:0]    debug_count
);
  localparam integer INGRESS_PTR_WIDTH = $clog2(INGRESS_DEPTH);
  localparam integer INGRESS_COUNT_WIDTH = $clog2(INGRESS_DEPTH + 1);
  localparam integer RESPONSE_PTR_WIDTH = $clog2(RESPONSE_DEPTH);
  localparam integer RESPONSE_COUNT_WIDTH = $clog2(RESPONSE_DEPTH + 1);
  localparam integer IDLE_WIDTH = $clog2(IDLE_TIMEOUT + 1);

  (* ramstyle = "M10K, no_rw_check" *) reg [65:0] ingress_memory [0:INGRESS_DEPTH-1];
  reg [INGRESS_PTR_WIDTH-1:0] ingress_write_ptr;
  reg [INGRESS_PTR_WIDTH-1:0] ingress_read_ptr;
  reg [INGRESS_COUNT_WIDTH-1:0] ingress_count;
  reg [INGRESS_COUNT_WIDTH-1:0] ingress_memory_count;
  reg [65:0] ingress_head;
  reg ingress_head_valid;
  reg [IDLE_WIDTH-1:0] idle_count;
  reg draining;
  reg [32:0] response_memory [0:RESPONSE_DEPTH-1];
  reg [RESPONSE_PTR_WIDTH-1:0] response_write_ptr;
  reg [RESPONSE_PTR_WIDTH-1:0] response_read_ptr;
  reg [RESPONSE_COUNT_WIDTH-1:0] response_count;

  wire [6:0] request_count = request[65:59];
  wire request_write = request[58];
  wire [21:0] request_address = request[57:36];
  wire [31:0] request_data = request[35:4];
  wire [3:0] request_mask = request[3:0];
  wire request_legal = request_count == 7'd1 && request_write &&
      request_address < SPAN_WORDS;
  wire request_blocked = block_line_valid && request_address[21:6] == block_line_tag;
  wire response_space = response_count < RESPONSE_DEPTH;
  wire ingress_space = ingress_count < INGRESS_DEPTH;
  wire request_fire = request_valid && request_ready;
  wire ingress_push = request_fire && request_legal;
  wire response_pop = response_valid && response_ready;

  wire release_queue = draining || flush || ingress_count >= START_WATERMARK ||
      idle_count == IDLE_TIMEOUT;
  wire engine_request_valid = ingress_head_valid && release_queue;
  wire engine_request_ready;
  wire ingress_pop = engine_request_valid && engine_request_ready;
  wire ingress_replace_head = ingress_push &&
      ((!ingress_head_valid && ingress_memory_count == 0) ||
       (ingress_pop && ingress_memory_count == 0));
  wire ingress_store = ingress_push && !ingress_replace_head;
  wire ingress_fetch = ingress_pop && ingress_memory_count != 0;
  wire [32:0] engine_response;
  wire engine_response_valid;
  wire engine_busy;
  wire engine_flush = flush && ingress_count == 0;

  assign request_ready = !flush && response_space &&
      (!request_legal || (!request_blocked && ingress_space));
  assign response = response_memory[response_read_ptr];
  assign response_valid = response_count != 0;
  assign busy = ingress_count != 0 || engine_busy;
  assign update_valid = ingress_push;
  assign update_address = request_address;
  assign update_data = request_data;
  assign update_mask = request_mask;
  assign debug_count = ingress_count;

  XlsFramebufferWriteBuffer #(
    .BASE_ADDRESS(BASE_ADDRESS), .SPAN_WORDS(SPAN_WORDS),
    .RESPONSE_DEPTH(RESPONSE_DEPTH)
  ) engine (
    .clk(clk), .reset(reset), .request(ingress_head),
    .request_valid(engine_request_valid), .request_ready(engine_request_ready),
    .response(engine_response), .response_valid(engine_response_valid), .response_ready(1'b1),
    .mem_read(mem_read), .mem_write(mem_write), .mem_waitRequestn(mem_waitRequestn),
    .mem_burstCount(mem_burstCount), .mem_address(mem_address),
    .mem_byteEnable(mem_byteEnable), .mem_writeData(mem_writeData),
    .mem_readDataValid(mem_readDataValid), .mem_readData(mem_readData),
    .flush(engine_flush), .block_line_valid(block_line_valid), .block_line_tag(block_line_tag),
    .busy(engine_busy), .update_valid(), .update_address(), .update_data(), .update_mask(),
    .pending_line0_valid(pending_line0_valid), .pending_line0_tag(pending_line0_tag),
    .pending_line1_valid(pending_line1_valid), .pending_line1_tag(pending_line1_tag)
  );

  always @(posedge clk) begin
    if (reset) begin
      ingress_write_ptr <= 0;
      ingress_read_ptr <= 0;
      ingress_count <= 0;
      ingress_memory_count <= 0;
      ingress_head <= 0;
      ingress_head_valid <= 0;
      idle_count <= 0;
      draining <= 0;
      response_write_ptr <= 0;
      response_read_ptr <= 0;
      response_count <= 0;
    end else begin
      if (ingress_store) begin
        ingress_memory[ingress_write_ptr] <= request;
        ingress_write_ptr <= ingress_write_ptr + 1'b1;
      end
      if (ingress_replace_head) begin
        ingress_head <= request;
        ingress_head_valid <= 1;
      end else if (ingress_fetch) begin
        ingress_head <= ingress_memory[ingress_read_ptr];
        ingress_head_valid <= 1;
      end else if (ingress_pop) begin
        ingress_head_valid <= 0;
      end
      if (ingress_fetch) ingress_read_ptr <= ingress_read_ptr + 1'b1;
      case ({ingress_store, ingress_fetch})
        2'b10: ingress_memory_count <= ingress_memory_count + 1'b1;
        2'b01: ingress_memory_count <= ingress_memory_count - 1'b1;
        default: ingress_memory_count <= ingress_memory_count;
      endcase

      if (ingress_push) begin
        idle_count <= 0;
      end else if (ingress_count == 0) begin
        idle_count <= 0;
      end else if (idle_count != IDLE_TIMEOUT) begin
        idle_count <= idle_count + 1'b1;
      end
      case ({ingress_push, ingress_pop})
        2'b10: ingress_count <= ingress_count + 1'b1;
        2'b01: ingress_count <= ingress_count - 1'b1;
        default: ingress_count <= ingress_count;
      endcase

      if (ingress_count == 0) draining <= 0;
      else if (flush || ingress_count >= START_WATERMARK || idle_count == IDLE_TIMEOUT)
        draining <= 1;
      if (ingress_pop && ingress_count == 1 && !ingress_push) draining <= 0;

      if (request_fire) begin
        response_memory[response_write_ptr] <= {request_data, request_legal};
        response_write_ptr <= response_write_ptr + 1'b1;
      end
      if (response_pop) response_read_ptr <= response_read_ptr + 1'b1;
      case ({request_fire, response_pop})
        2'b10: response_count <= response_count + 1'b1;
        2'b01: response_count <= response_count - 1'b1;
        default: response_count <= response_count;
      endcase
    end
  end

  wire unused = ^engine_response ^ engine_response_valid;
endmodule
