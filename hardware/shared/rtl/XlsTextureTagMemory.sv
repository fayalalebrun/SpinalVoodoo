module XlsTextureTagMemory (
  input wire clk,
  input wire reset,
  input wire [24:0] read_request,
  input wire read_valid,
  output wire read_ready,
  output wire [55:0] response,
  output wire response_valid,
  input wire response_ready,
  input wire [26:0] write_request,
  input wire write_valid,
  output wire write_ready
);
  reg ram_valid;
  reg skid_valid;
  reg [55:0] skid_data;
  wire [55:0] ram_response;
  wire reading = read_valid && read_ready;
  wire writing = write_valid && write_ready;
  wire [5:0] primary_slot = write_request[14:9] ^ write_request[20:15] ^ write_request[26:21];
  wire [3:0] shadow_slot = write_request[12:9] ^ write_request[16:13] ^
      write_request[20:17] ^ write_request[24:21] ^ {2'b0, write_request[26:25]};
  reg primary_response;
  assign read_ready = !reset && !skid_valid;
  assign write_ready = !reset;
  assign response_valid = ram_valid || skid_valid;
  assign response = skid_valid ? skid_data : ram_response;
  always @(posedge clk) begin
    if (reset) begin
      ram_valid <= 1'b0;
      skid_valid <= 1'b0;
    end else begin
      if (reading) ram_valid <= 1'b1;
      else if (!skid_valid && response_ready) ram_valid <= 1'b0;
      if (skid_valid && response_ready) skid_valid <= 1'b0;
      if (reading && ram_valid && !response_ready) begin
        skid_valid <= 1'b1;
        skid_data <= ram_response;
      end
    end
    if (reading) primary_response <= read_request[0];
  end
  genvar tap;
  generate for (tap = 0; tap < 4; tap = tap + 1) begin : tags
    (* ramstyle = "M10K" *) reg [11:0] primary [0:63];
    (* ramstyle = "M10K" *) reg [13:0] shadow [0:15];
    reg [11:0] primary_data;
    reg [13:0] shadow_data;
    reg forwarded;
    reg [13:0] forward_data;
    wire [5:0] slot = read_request[1 + 6*(3-tap) +: 6];
    always @(posedge clk) begin
      if (writing && write_request[0]) primary[primary_slot] <= write_request[26:15];
      if (writing) shadow[shadow_slot] <= write_request[26:13];
      if (reading) begin
        primary_data <= primary[slot];
        shadow_data <= shadow[slot[3:0]];
        forwarded <= writing && (read_request[0] ?
            write_request[0] && slot == primary_slot : slot[3:0] == shadow_slot);
        forward_data <= read_request[0] ? {2'b0, write_request[26:15]} : write_request[26:13];
      end
    end
    assign ram_response[14*(3-tap) +: 14] = forwarded ? forward_data :
        primary_response ? {2'b0, primary_data} : shadow_data;
  end endgenerate
endmodule
