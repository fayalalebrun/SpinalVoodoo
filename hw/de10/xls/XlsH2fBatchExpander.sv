module XlsH2fBatchExpander #(
  parameter [21:0] DOORBELL_ADDRESS = 22'h0ffc00,
  parameter [31:0] BATCH_BASE_ADDRESS = 32'h3f080000
) (
  input  wire        clk,
  input  wire        reset,
  input  wire [59:0] ingress_data,
  input  wire        ingress_empty,
  output wire        ingress_pop,
  output wire [59:0] request_data,
  output wire        request_empty,
  input  wire        request_pop,
  output wire        mem_read,
  output wire        mem_write,
  input  wire        mem_waitRequestn,
  output wire [10:0] mem_burstCount,
  output wire [31:0] mem_address,
  output wire [3:0]  mem_byteEnable,
  output wire [31:0] mem_writeData,
  input  wire        mem_readDataValid,
  input  wire [31:0] mem_readData
);
  reg read_active;
  reg read_issued;
  reg read_high_phase;
  reg [11:0] batch_slot;
  reg [6:0] batch_count;
  reg [7:0] read_words_remaining;
  reg [31:0] read_low;
  reg [59:0] entries [0:63];
  reg [5:0] write_pointer;
  reg [5:0] read_pointer;
  reg [6:0] occupancy;

  wire marker = !ingress_empty && ingress_data[36] &&
      ingress_data[59:38] == DOORBELL_ADDRESS;
  wire [6:0] marker_count = {1'b0, ingress_data[5:0]} + 1'b1;
  wire request_pop_valid = request_pop && occupancy != 0;
  wire start_batch = !reset && !read_active && marker &&
      {1'b0, occupancy} + {1'b0, marker_count} <= 8'd64;
  wire direct_request = !read_active && !marker && occupancy == 0;
  wire batch_push = read_active && mem_readDataValid && read_high_phase;
  wire [31:0] slot_offset = {11'b0, batch_slot, 9'b0};

  assign ingress_pop = start_batch || (direct_request && request_pop);
  assign request_data = occupancy == 0 ? ingress_data : entries[read_pointer];
  assign request_empty = occupancy == 0 ? ingress_empty || marker || read_active : 1'b0;
  assign mem_read = !reset && read_active && !read_issued;
  assign mem_write = 1'b0;
  assign mem_burstCount = {3'b0, batch_count, 1'b0};
  assign mem_address = BATCH_BASE_ADDRESS + slot_offset;
  assign mem_byteEnable = 4'hf;
  assign mem_writeData = 32'b0;

  always @(posedge clk or posedge reset) begin
    if (reset) begin
      read_active <= 1'b0;
      read_issued <= 1'b0;
      read_high_phase <= 1'b0;
      batch_slot <= 0;
      batch_count <= 0;
      read_words_remaining <= 0;
      read_low <= 0;
      write_pointer <= 0;
      read_pointer <= 0;
      occupancy <= 0;
    end else begin
      if (start_batch) begin
        read_active <= 1'b1;
        read_issued <= 1'b0;
        read_high_phase <= 1'b0;
        batch_slot <= ingress_data[31:20];
        batch_count <= marker_count;
        read_words_remaining <= {marker_count, 1'b0};
      end
      if (mem_read && mem_waitRequestn) read_issued <= 1'b1;
      if (read_active && mem_readDataValid) begin
        read_high_phase <= !read_high_phase;
        read_words_remaining <= read_words_remaining - 1'b1;
        if (!read_high_phase) begin
          read_low <= mem_readData;
        end else begin
          entries[write_pointer] <= {mem_readData[27:0], read_low};
          write_pointer <= write_pointer + 1'b1;
        end
        if (read_words_remaining == 1) begin
          read_active <= 1'b0;
          read_issued <= 1'b0;
        end
      end
      if (request_pop_valid) read_pointer <= read_pointer + 1'b1;
      case ({batch_push, request_pop_valid})
        2'b10: occupancy <= occupancy + 1'b1;
        2'b01: occupancy <= occupancy - 1'b1;
        default: occupancy <= occupancy;
      endcase
    end
  end
endmodule
