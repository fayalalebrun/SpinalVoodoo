`ifdef __ICARUS__
`define XLS_PACKED_BANK_BEHAVIORAL
`endif
`ifdef VERILATOR
`define XLS_PACKED_BANK_BEHAVIORAL
`endif

module XlsPackedBankMemory (
  input  wire          clk,
  input  wire          reset,
  input  wire [58:0]   command,
  input  wire          command_valid,
  output wire          command_ready,
  output wire [64:0]   response,
  output wire          response_valid,
  input  wire          response_ready
);
  reg pending_valid;
  reg saved_write;
  reg saved_packed;
  reg saved_forward;
  reg [10:0] saved_first_address;
  reg [31:0] saved_data;
  reg [3:0] saved_mask;
  reg [31:0] forwarded_data;

  wire write = command[58];
  wire [21:0] address = command[57:36];
  wire [31:0] data = command[35:4];
  wire [3:0] mask = command[3:0];
  wire packed_mode = !write && mask[3];
  wire [21:0] second_address = data[21:0];
  wire [10:0] first_memory_address = address[10:0];
  wire [10:0] second_memory_address = second_address[10:0];

  function automatic [31:0] byte_mask(input [3:0] value);
    begin
      byte_mask = {
        {8{value[3]}},
        {8{value[2]}},
        {8{value[1]}},
        {8{value[0]}}
      };
    end
  endfunction

  wire pending_accept = pending_valid && response_ready;
  wire accept = command_valid && command_ready;
  wire write_commit = pending_accept && saved_write;
  wire read0_enable = accept;
  wire read1_enable = accept && packed_mode && mask[0];
  wire [31:0] first_read_data;
  wire [31:0] second_read_data;
  wire [31:0] first_base_data = saved_forward ? forwarded_data : first_read_data;
  wire [31:0] write_response_data =
      (first_base_data & ~byte_mask(saved_mask)) | (saved_data & byte_mask(saved_mask));
  wire [31:0] first_response_data = saved_write ? write_response_data : first_base_data;
  wire [63:0] response_data =
      saved_packed ? {second_read_data, first_response_data} : {32'b0, first_response_data};
  wire forward = write_commit && accept && first_memory_address == saved_first_address;
  wire write_port_available = !(pending_valid && saved_write && response_ready);

  assign command_ready = !reset && (!pending_valid || response_ready) &&
                         (!packed_mode || write_port_available);
  assign response = {response_data, 1'b1};
  assign response_valid = pending_valid;

  XlsPackedBankMemoryArray #(.NUMWORDS(2048), .ADDRESS_WIDTH(11)) storage (
    .clk(clk),
    .read0_enable(read0_enable),
    .read0_address(first_memory_address),
    .read1_enable(read1_enable),
    .read1_address(second_memory_address),
    .write_enable(write_commit),
    .write_address(saved_first_address),
    .write_data(saved_data),
    .write_mask(saved_mask),
    .read0_data(first_read_data),
    .read1_data(second_read_data)
  );

  always @(posedge clk) begin
    if (reset) begin
      pending_valid <= 0;
      saved_write <= 0;
      saved_packed <= 0;
      saved_forward <= 0;
      saved_first_address <= 0;
      saved_data <= 0;
      saved_mask <= 0;
      forwarded_data <= 0;
    end else begin
      if (pending_accept) pending_valid <= 0;
      if (accept) begin
        pending_valid <= 1;
        saved_write <= write;
        saved_packed <= packed_mode;
        saved_forward <= forward;
        saved_first_address <= first_memory_address;
        saved_data <= data;
        saved_mask <= mask;
        if (forward) forwarded_data <= first_response_data;
      end
    end
  end
endmodule

module XlsPackedBankMemoryArray #(
  parameter integer NUMWORDS = 1024,
  parameter integer ADDRESS_WIDTH = 10
) (
  input  wire                     clk,
  input  wire                     read0_enable,
  input  wire [ADDRESS_WIDTH-1:0] read0_address,
  input  wire                     read1_enable,
  input  wire [ADDRESS_WIDTH-1:0] read1_address,
  input  wire                     write_enable,
  input  wire [ADDRESS_WIDTH-1:0] write_address,
  input  wire [31:0] write_data,
  input  wire [3:0]  write_mask,
  output wire [31:0] read0_data,
  output wire [31:0] read1_data
);
`ifndef XLS_PACKED_BANK_BEHAVIORAL
  wire [ADDRESS_WIDTH-1:0] port_a_address = write_enable ? write_address : read1_address;
  altsyncram #(
    .address_reg_b("CLOCK1"),
    .byte_size(8),
    .byteena_reg_a("CLOCK0"),
    .clock_enable_input_a("BYPASS"),
    .clock_enable_input_b("BYPASS"),
    .clock_enable_output_a("BYPASS"),
    .clock_enable_output_b("BYPASS"),
    .intended_device_family("Cyclone V"),
    .lpm_type("altsyncram"),
    .numwords_a(NUMWORDS),
    .numwords_b(NUMWORDS),
    .operation_mode("BIDIR_DUAL_PORT"),
    .outdata_aclr_a("NONE"),
    .outdata_aclr_b("NONE"),
    .outdata_reg_a("CLOCK0"),
    .outdata_reg_b("UNREGISTERED"),
    .power_up_uninitialized("FALSE"),
    .read_during_write_mode_mixed_ports("DONT_CARE"),
    .width_a(32),
    .width_b(32),
    .width_byteena_a(4),
    .widthad_a(ADDRESS_WIDTH),
    .widthad_b(ADDRESS_WIDTH)
  ) storage (
    .address_a(port_a_address),
    .address_b(read0_address),
    .byteena_a(write_mask),
    .byteena_b(4'hf),
    .clock0(clk),
    .clock1(clk),
    .clocken0(1'b1),
    .clocken1(1'b1),
    .data_a(write_data),
    .data_b(32'b0),
    .q_a(read1_data),
    .q_b(read0_data),
    .rden_a(read1_enable && !write_enable),
    .rden_b(read0_enable),
    .wren_a(write_enable),
    .wren_b(1'b0)
  );
`else
  reg [31:0] memory [0:NUMWORDS-1];
  reg [31:0] read0_register;
  reg [31:0] read1_register;

  assign read0_data = read0_register;
  assign read1_data = read1_register;

  always @(posedge clk) begin
    if (read0_enable) read0_register <= memory[read0_address];
    if (read1_enable && !write_enable) read1_register <= memory[read1_address];
    if (write_enable) begin
      if (write_mask[0]) memory[write_address][7:0] <= write_data[7:0];
      if (write_mask[1]) memory[write_address][15:8] <= write_data[15:8];
      if (write_mask[2]) memory[write_address][23:16] <= write_data[23:16];
      if (write_mask[3]) memory[write_address][31:24] <= write_data[31:24];
    end
  end
`endif
endmodule

module XlsConcurrentPackedBankMemory (
  input  wire          clk,
  input  wire          reset,
  input  wire [58:0]   slow_command,
  input  wire          slow_command_valid,
  output wire          slow_command_ready,
  output wire [64:0]   slow_response,
  output wire          slow_response_valid,
  input  wire          slow_response_ready,
  input  wire [58:0]   fast_command,
  input  wire          fast_command_valid,
  output wire          fast_command_ready,
  output wire [64:0]   fast_response,
  output wire          fast_response_valid,
  input  wire          fast_response_ready
);
  reg slow_pending;
  reg fast_pending;
  reg fast_packed;
  reg slow_victim;
  reg fast_victim;
  wire slow_write = slow_command[58];
  wire [10:0] slow_address = slow_command[46:36];
  wire [31:0] slow_data = slow_command[35:4];
  wire [3:0] slow_mask = slow_command[3:0];
  wire [10:0] fast_address0 = fast_command[46:36];
  wire [10:0] fast_address1 = fast_command[14:4];
  wire fast_second = fast_command[0];
  wire slow_accept = slow_command_valid && slow_command_ready;
  wire fast_accept = fast_command_valid && fast_command_ready;
  wire slow_read = slow_accept && !slow_write;
  wire read0_victim = slow_read ? slow_address[10] : fast_address0[10];
  wire read0_enable = slow_read || fast_accept;
  wire [31:0] primary_read0_data;
  wire [31:0] victim_read_data;
  wire [31:0] slow_read_data = slow_victim ? victim_read_data : primary_read0_data;
  wire [31:0] fast_read0_data = fast_victim ? victim_read_data : primary_read0_data;
  wire [31:0] read1_data;

  assign slow_command_ready = !reset && (!slow_pending || slow_response_ready);
  assign fast_command_ready = !reset && (!fast_pending || fast_response_ready);
  assign slow_response = {32'b0, slow_read_data, 1'b1};
  assign fast_response = {fast_packed ? read1_data : 32'b0, fast_read0_data, 1'b1};
  assign slow_response_valid = slow_pending;
  assign fast_response_valid = fast_pending;

  XlsPackedBankMemoryArray mirror0 (
    .clk(clk),
    .read0_enable(read0_enable && !read0_victim),
    .read0_address(slow_read ? slow_address[9:0] : fast_address0[9:0]),
    .read1_enable(1'b0),
    .read1_address(10'b0),
    .write_enable(slow_accept && slow_write && !slow_address[10]),
    .write_address(slow_address[9:0]),
    .write_data(slow_data),
    .write_mask(slow_mask),
    .read0_data(primary_read0_data),
    .read1_data()
  );

  XlsPackedBankMemoryArray mirror1 (
    .clk(clk),
    .read0_enable(fast_accept && fast_second),
    .read0_address(fast_address1[9:0]),
    .read1_enable(1'b0),
    .read1_address(10'b0),
    .write_enable(slow_accept && slow_write && !slow_address[10]),
    .write_address(slow_address[9:0]),
    .write_data(slow_data),
    .write_mask(slow_mask),
    .read0_data(read1_data),
    .read1_data()
  );

  XlsPackedBankMemoryArray #(.NUMWORDS(320), .ADDRESS_WIDTH(9)) victim (
    .clk(clk),
    .read0_enable(read0_enable && read0_victim),
    .read0_address(slow_read ? slow_address[8:0] : fast_address0[8:0]),
    .read1_enable(1'b0),
    .read1_address(9'b0),
    .write_enable(slow_accept && slow_write && slow_address[10]),
    .write_address(slow_address[8:0]),
    .write_data(slow_data),
    .write_mask(slow_mask),
    .read0_data(victim_read_data),
    .read1_data()
  );

  always @(posedge clk) begin
    if (reset) begin
      slow_pending <= 0;
      fast_pending <= 0;
      fast_packed <= 0;
      slow_victim <= 0;
      fast_victim <= 0;
    end else begin
      if (slow_pending && slow_response_ready) slow_pending <= 0;
      if (fast_pending && fast_response_ready) fast_pending <= 0;
      if (slow_accept) begin
        slow_pending <= 1;
        slow_victim <= slow_address[10];
      end
      if (fast_accept) begin
        fast_pending <= 1;
        fast_packed <= fast_second;
        fast_victim <= fast_address0[10];
      end
    end
  end
endmodule

`ifdef XLS_PACKED_BANK_BEHAVIORAL
`undef XLS_PACKED_BANK_BEHAVIORAL
`endif
