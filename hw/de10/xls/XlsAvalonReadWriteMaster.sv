module XlsAvalonReadWriteMaster #(
  parameter [31:0] BASE_ADDRESS = 32'h00000000,
  parameter integer SPAN_WORDS = 1,
  parameter integer ROUTE_DEPTH = 64
) (
  input wire clk,
  input wire reset,
  input wire [65:0] request,
  input wire request_valid,
  output wire request_ready,
  output wire [32:0] response,
  output wire response_valid,
  input wire response_ready,
  output wire read_mem_read,
  output wire read_mem_write,
  input wire read_mem_waitRequestn,
  output wire [10:0] read_mem_burstCount,
  output wire [31:0] read_mem_address,
  output wire [3:0] read_mem_byteEnable,
  output wire [31:0] read_mem_writeData,
  input wire read_mem_readDataValid,
  input wire [31:0] read_mem_readData,
  output wire write_mem_read,
  output wire write_mem_write,
  input wire write_mem_waitRequestn,
  output wire [10:0] write_mem_burstCount,
  output wire [31:0] write_mem_address,
  output wire [3:0] write_mem_byteEnable,
  output wire [31:0] write_mem_writeData,
  input wire write_mem_readDataValid,
  input wire [31:0] write_mem_readData
);
  localparam integer ROUTE_POINTER_WIDTH = $clog2(ROUTE_DEPTH);

  reg [7:0] route_memory [0:ROUTE_DEPTH-1];
  reg [ROUTE_POINTER_WIDTH-1:0] route_read_pointer;
  reg [ROUTE_POINTER_WIDTH-1:0] route_write_pointer;
  reg [ROUTE_POINTER_WIDTH:0] route_count;
  wire route_empty = route_count == 0;
  wire route_full = route_count == ROUTE_DEPTH;
  wire route_head = route_memory[route_read_pointer][7];
  wire [6:0] route_head_count = route_memory[route_read_pointer][6:0];
  wire request_write = request[58];
  wire [6:0] request_count = request[65:59];
  wire [6:0] request_response_count = request_write || request_count == 0 ? 7'd1 : request_count;
  wire read_request_ready;
  wire write_request_ready;
  wire [32:0] read_response;
  wire [32:0] write_response;
  wire read_response_valid;
  wire write_response_valid;
  wire read_response_ready;
  wire write_response_ready;
  wire route_push = request_valid && request_ready;
  wire response_fire = response_valid && response_ready;
  wire route_pop = response_fire && route_head_count == 7'd1;

  assign request_ready = !route_full && (request_write ? write_request_ready : read_request_ready);
  assign response = route_head ? write_response : read_response;
  assign response_valid = !route_empty && (route_head ? write_response_valid : read_response_valid);
  assign read_response_ready = !route_empty && !route_head && response_ready;
  assign write_response_ready = !route_empty && route_head && response_ready;

  always @(posedge clk) begin
    if (reset) begin
      route_read_pointer <= 0;
      route_write_pointer <= 0;
      route_count <= 0;
    end else begin
      if (route_push) begin
        route_memory[route_write_pointer] <= {request_write, request_response_count};
        route_write_pointer <= route_write_pointer + 1'b1;
      end
      if (response_fire && route_head_count > 7'd1)
        route_memory[route_read_pointer][6:0] <= route_head_count - 1'b1;
      if (route_pop) route_read_pointer <= route_read_pointer + 1'b1;
      case ({route_push, route_pop})
        2'b10: route_count <= route_count + 1'b1;
        2'b01: route_count <= route_count - 1'b1;
        default: route_count <= route_count;
      endcase
    end
  end

  XlsAvalonTransferMaster #(.BASE_ADDRESS(BASE_ADDRESS), .SPAN_WORDS(SPAN_WORDS)) read_master (
    .clk(clk), .reset(reset), .request(request),
    .request_valid(request_valid && !request_write && !route_full),
    .request_ready(read_request_ready), .response(read_response),
    .response_valid(read_response_valid), .response_ready(read_response_ready),
    .mem_read(read_mem_read), .mem_write(read_mem_write),
    .mem_waitRequestn(read_mem_waitRequestn), .mem_burstCount(read_mem_burstCount),
    .mem_address(read_mem_address), .mem_byteEnable(read_mem_byteEnable),
    .mem_writeData(read_mem_writeData), .mem_readDataValid(read_mem_readDataValid),
    .mem_readData(read_mem_readData)
  );

  XlsAvalonTransferMaster #(.BASE_ADDRESS(BASE_ADDRESS), .SPAN_WORDS(SPAN_WORDS)) write_master (
    .clk(clk), .reset(reset), .request(request),
    .request_valid(request_valid && request_write && !route_full),
    .request_ready(write_request_ready), .response(write_response),
    .response_valid(write_response_valid), .response_ready(write_response_ready),
    .mem_read(write_mem_read), .mem_write(write_mem_write),
    .mem_waitRequestn(write_mem_waitRequestn), .mem_burstCount(write_mem_burstCount),
    .mem_address(write_mem_address), .mem_byteEnable(write_mem_byteEnable),
    .mem_writeData(write_mem_writeData), .mem_readDataValid(write_mem_readDataValid),
    .mem_readData(write_mem_readData)
  );
endmodule
