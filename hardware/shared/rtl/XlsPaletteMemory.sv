module XlsPaletteMemory (
  input  wire        clk,
  input  wire        reset,
  input  wire [31:0] read_request,
  input  wire        read_request_valid,
  output wire        read_request_ready,
  input  wire [32:0] write,
  input  wire        write_valid,
  output wire        write_ready,
  output wire [95:0] response,
  output wire        response_valid,
  input  wire        response_ready
);
  reg [3:0] bank;
  wire [11:0] index0 = {bank, read_request[31:24]};
  wire [11:0] index1 = {bank, read_request[23:16]};
  wire [11:0] index2 = {bank, read_request[15:8]};
  wire [11:0] index3 = {bank, read_request[7:0]};
  wire [11:0] write_index = {bank, write[31:24]};
  wire [23:0] write_data = write[23:0];
  wire read_accept = read_request_valid && read_request_ready;
  wire write_accept = write_valid && write_ready;

  (* ramstyle = "M10K, no_rw_check" *) reg [23:0] memory0 [0:4095];
  (* ramstyle = "M10K, no_rw_check" *) reg [23:0] memory1 [0:4095];
  (* ramstyle = "M10K, no_rw_check" *) reg [23:0] memory2 [0:4095];
  (* ramstyle = "M10K, no_rw_check" *) reg [23:0] memory3 [0:4095];
  reg [23:0] read0;
  reg [23:0] read1;
  reg [23:0] read2;
  reg [23:0] read3;
  reg response_pending;

  assign read_request_ready = !reset && (!response_pending || response_ready);
  assign write_ready = !reset;
  assign response = {read0, read1, read2, read3};
  assign response_valid = response_pending;

  always @(posedge clk) begin
    if (reset) begin
      bank <= 4'b0;
      response_pending <= 1'b0;
    end else begin
      if (response_pending && response_ready) response_pending <= 1'b0;
      if (write_accept && write[32] && write[31:4] == 0) bank <= write[3:0];
      if (write_accept && !write[32]) begin
        memory0[write_index] <= write_data;
        memory1[write_index] <= write_data;
        memory2[write_index] <= write_data;
        memory3[write_index] <= write_data;
      end
      if (read_accept) begin
        read0 <= memory0[index0];
        read1 <= memory1[index1];
        read2 <= memory2[index2];
        read3 <= memory3[index3];
        response_pending <= 1'b1;
      end
    end
  end
endmodule
