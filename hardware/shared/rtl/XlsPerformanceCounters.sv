module XlsPerformanceCounters (
  input  wire        clk,
  input  wire        reset,
  input  wire        clear,
  input  wire        stats_clear,
  input  wire        triangle_start,
  input  wire [8:0]  event_data,
  input  wire        event_valid,
  output reg  [63:0] render_cycles,
  output reg  [63:0] pixels_in,
  output reg  [63:0] pixels_out,
  output reg  [23:0] fbi_pixels_in,
  output reg  [23:0] fbi_pixels_out,
  output reg         triangle_active
);
  wire triangle_done = event_valid && event_data[6];

  always @(posedge clk) begin
    if (reset || clear) begin
      render_cycles <= 64'b0;
      pixels_in <= 64'b0;
      pixels_out <= 64'b0;
      fbi_pixels_in <= 24'b0;
      fbi_pixels_out <= 24'b0;
      triangle_active <= 1'b0;
    end else begin
      if (triangle_active) begin
        render_cycles[31:0] <= render_cycles[31:0] + 32'd1;
      end
      if (event_valid && event_data[8]) begin
        pixels_in[31:0] <= pixels_in[31:0] + 32'd1;
      end
      if (event_valid && event_data[7]) begin
        pixels_out[31:0] <= pixels_out[31:0] + 32'd1;
      end
      if (stats_clear) begin
        fbi_pixels_in <= 24'b0;
        fbi_pixels_out <= 24'b0;
      end else begin
        if (event_valid && event_data[8]) begin
          fbi_pixels_in <= fbi_pixels_in + 24'd1;
        end
        if (event_valid && event_data[7]) begin
          fbi_pixels_out <= fbi_pixels_out + 24'd1;
        end
      end
      if (triangle_start) begin
        triangle_active <= 1'b1;
      end
      if (triangle_done && !triangle_start) begin
        triangle_active <= 1'b0;
      end
    end
  end
endmodule

module XlsRenderCycleProfiler (
  input  wire        clk,
  input  wire        reset,
  input  wire        clear,
  input  wire        active,
  input  wire        fast_bank_request_blocked,
  input  wire        fast_bank_response_blocked,
  input  wire        texture_request_blocked,
  input  wire        texture_mem_read,
  input  wire        texture_mem_wait,
  input  wire [10:0] texture_mem_burst_count,
  input  wire        texture_mem_read_data_valid,
  input  wire        color_request_blocked,
  input  wire        color_mem_read,
  input  wire        color_mem_wait,
  input  wire [10:0] color_mem_burst_count,
  input  wire        color_mem_read_data_valid,
  input  wire        write_request_blocked,
  input  wire        write_mem_wait,
  input  wire [5:0]  read_index,
  output reg  [31:0] read_data
);
  reg [23:0] counters [0:13];
  reg [12:0] texture_pending_beats;
  reg [12:0] color_pending_beats;
  wire texture_read_fire = texture_mem_read && !texture_mem_wait;
  wire color_read_fire = color_mem_read && !color_mem_wait;
  integer i;

  always @(posedge clk) begin
    if (reset || clear) begin
      for (i = 0; i < 14; i = i + 1) counters[i] <= 0;
      texture_pending_beats <= 0;
      color_pending_beats <= 0;
    end else begin
      texture_pending_beats <= texture_pending_beats +
          (texture_read_fire ? texture_mem_burst_count : 11'd0) - texture_mem_read_data_valid;
      color_pending_beats <= color_pending_beats +
          (color_read_fire ? color_mem_burst_count : 11'd0) - color_mem_read_data_valid;
      if (active) begin
        counters[0] <= counters[0] + fast_bank_request_blocked;
        counters[1] <= counters[1] + fast_bank_response_blocked;
        counters[2] <= counters[2] + texture_request_blocked;
        counters[3] <= counters[3] + (texture_mem_read && texture_mem_wait);
        counters[4] <= counters[4] + (texture_pending_beats != 0 && !texture_mem_read_data_valid);
        counters[5] <= counters[5] + texture_read_fire;
        counters[6] <= counters[6] + texture_mem_read_data_valid;
        counters[7] <= counters[7] + color_request_blocked;
        counters[8] <= counters[8] + (color_mem_read && color_mem_wait);
        counters[9] <= counters[9] + (color_pending_beats != 0 && !color_mem_read_data_valid);
        counters[10] <= counters[10] + color_read_fire;
        counters[11] <= counters[11] + color_mem_read_data_valid;
        counters[12] <= counters[12] + write_request_blocked;
        counters[13] <= counters[13] + write_mem_wait;
      end
    end
  end

  always @(*) begin
    if (read_index >= 1 && read_index <= 14) begin
      read_data = {8'b0, counters[read_index - 1]};
    end else begin
      read_data = 0;
    end
  end
endmodule
