`timescale 1ns/1ps

module XlsPerformanceCountersTb;
  reg clk = 0;
  reg reset = 1;
  reg clear = 0;
  reg stats_clear = 0;
  reg triangle_start = 0;
  reg [8:0] event_data = 0;
  reg event_valid = 0;
  wire [63:0] render_cycles;
  wire [63:0] pixels_in;
  wire [63:0] pixels_out;
  wire [23:0] fbi_pixels_in;
  wire [23:0] fbi_pixels_out;
  wire triangle_active;

  XlsPerformanceCounters dut (
    .clk(clk), .reset(reset), .clear(clear), .stats_clear(stats_clear),
    .triangle_start(triangle_start), .event_data(event_data), .event_valid(event_valid),
    .render_cycles(render_cycles), .pixels_in(pixels_in), .pixels_out(pixels_out),
    .fbi_pixels_in(fbi_pixels_in), .fbi_pixels_out(fbi_pixels_out),
    .triangle_active(triangle_active)
  );

  always #5 clk = ~clk;

  task step;
    begin
      @(negedge clk);
      @(posedge clk);
      #1;
    end
  endtask

  initial begin
    repeat (2) step();
    reset = 0;
    triangle_start = 1;
    step();
    triangle_start = 0;
    repeat (3) step();
    event_data = 9'b100000000;
    event_valid = 1;
    step();
    event_data = 9'b110000000;
    step();
    event_data = 9'b001000000;
    step();
    event_valid = 0;
    event_data = 0;
    if (render_cycles !== 64'd6 || pixels_in !== 64'd2 || pixels_out !== 64'd1 ||
        fbi_pixels_in !== 24'd2 || fbi_pixels_out !== 24'd1 ||
        triangle_active !== 1'b0) $fatal(1, "counter mismatch");
    stats_clear = 1;
    step();
    stats_clear = 0;
    if (pixels_in !== 64'd2 || pixels_out !== 64'd1 || fbi_pixels_in !== 0 ||
        fbi_pixels_out !== 0) $fatal(1, "stats clear failed");
    clear = 1;
    step();
    clear = 0;
    if (render_cycles !== 0 || pixels_in !== 0 || pixels_out !== 0) $fatal(1, "clear failed");
    triangle_start = 1;
    event_data = 9'b001000000;
    event_valid = 1;
    step();
    triangle_start = 0;
    event_valid = 0;
    event_data = 0;
    if (triangle_active !== 1'b1) $fatal(1, "back-to-back triangle stopped");
    event_data = 9'b001000000;
    event_valid = 1;
    step();
    if (triangle_active !== 1'b0) $fatal(1, "triangle did not stop");
    $display("XlsPerformanceCountersTb PASS");
    $finish;
  end
endmodule
