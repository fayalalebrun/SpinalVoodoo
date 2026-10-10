`timescale 1ns/1ps

module XlsPipelineProfilerTb;
  reg clk = 0;
  reg reset = 1;
  reg clear = 0;
  reg active = 0;
  reg [8:0] performance_event = 0;
  reg performance_event_valid = 0;
  reg interpolation_start = 0;
  reg pipeline_start = 0;
  reg cache_done = 0;
  reg read_done = 0;
  reg write_done = 0;
  reg [5:0] pending_pixels = 0;
  reg pending_pixels_valid = 0;
  reg [65:0] color_request = 0;
  reg color_request_valid = 0;
  reg color_request_ready = 1;
  reg color_response_valid = 0;
  reg color_response_ready = 1;
  reg color_mem_read = 0;
  reg color_mem_write = 0;
  reg color_mem_waitRequestn = 1;
  reg [10:0] color_mem_burstCount = 1;
  reg color_mem_readDataValid = 0;
  reg [65:0] color_write_request = 66'd0;
  reg color_write_request_valid = 0;
  reg color_write_request_ready = 1;
  reg color_write_response_valid = 0;
  reg color_write_response_ready = 1;
  reg color_write_mem_read = 0;
  reg color_write_mem_write = 0;
  reg color_write_mem_waitRequestn = 1;
  reg [10:0] color_write_mem_burstCount = 1;
  reg color_write_mem_readDataValid = 0;
  reg [5:0] read_index = 0;
  wire [31:0] read_data;

  XlsPipelineProfiler dut (
    .clk(clk), .reset(reset), .clear(clear), .active(active),
    .performance_event(performance_event), .performance_event_valid(performance_event_valid),
    .interpolation_start(interpolation_start), .pipeline_start(pipeline_start),
    .cache_done(cache_done), .read_done(read_done), .write_done(write_done),
    .pending_pixels(pending_pixels), .pending_pixels_valid(pending_pixels_valid),
    .color_read_request(color_request), .color_read_request_valid(color_request_valid),
    .color_read_request_ready(color_request_ready),
    .color_read_response_valid(color_response_valid),
    .color_read_response_ready(color_response_ready), .color_read_mem_read(color_mem_read),
    .color_read_mem_write(color_mem_write),
    .color_read_mem_waitRequestn(color_mem_waitRequestn),
     .color_read_mem_burstCount(color_mem_burstCount),
     .color_read_mem_readDataValid(color_mem_readDataValid),
     .color_read_data_stall(1'b0), .color_read_inflight_stall(1'b0),
     .color_read_unissued_stall(1'b0), .color_read_response_stall(1'b0),
     .color_write_request(color_write_request),
    .color_write_request_valid(color_write_request_valid),
    .color_write_request_ready(color_write_request_ready),
    .color_write_response_valid(color_write_response_valid),
    .color_write_response_ready(color_write_response_ready),
    .color_write_mem_read(color_write_mem_read), .color_write_mem_write(color_write_mem_write),
    .color_write_mem_waitRequestn(color_write_mem_waitRequestn),
    .color_write_mem_burstCount(color_write_mem_burstCount),
    .color_write_mem_readDataValid(color_write_mem_readDataValid),
    .aux_read_request(66'b0), .aux_read_request_valid(1'b0), .aux_read_request_ready(1'b1),
    .aux_read_response_valid(1'b0), .aux_read_response_ready(1'b1),
    .aux_read_mem_read(1'b0), .aux_read_mem_write(1'b0),
     .aux_read_mem_waitRequestn(1'b1), .aux_read_mem_burstCount(11'd1),
     .aux_read_mem_readDataValid(1'b0), .aux_read_data_stall(1'b0),
     .aux_read_inflight_stall(1'b0), .aux_read_unissued_stall(1'b0),
     .aux_read_response_stall(1'b0), .aux_write_request(66'b0),
    .aux_write_request_valid(1'b0), .aux_write_request_ready(1'b1),
    .aux_write_response_valid(1'b0), .aux_write_response_ready(1'b1),
    .aux_write_mem_read(1'b0), .aux_write_mem_write(1'b0),
    .aux_write_mem_waitRequestn(1'b1), .aux_write_mem_burstCount(11'd1),
    .aux_write_mem_readDataValid(1'b0), .texture_request(66'b0),
    .texture_request_valid(1'b0), .texture_request_ready(1'b1),
    .texture_response_valid(1'b0), .texture_response_ready(1'b1),
    .texture_mem_read(1'b0), .texture_mem_write(1'b0),
    .texture_mem_waitRequestn(1'b1), .texture_mem_burstCount(11'd1),
    .texture_mem_readDataValid(1'b0), .read_index(read_index), .read_data(read_data)
  );

  always #5 clk = ~clk;

  task step;
    begin
      @(posedge clk);
      #1;
    end
  endtask

  task pulse_stage;
    input integer stage;
    begin
      interpolation_start = stage == 0;
      pipeline_start = stage == 1;
      cache_done = stage == 2;
      read_done = stage == 3;
      write_done = stage == 4;
      step();
      interpolation_start = 0;
      pipeline_start = 0;
      cache_done = 0;
      read_done = 0;
      write_done = 0;
    end
  endtask

  task expect_counter;
    input [5:0] index;
    input [31:0] expected;
    begin
      read_index = index;
      #1;
      if (read_data !== expected) $fatal(1, "counter %0d got %0d expected %0d", index, read_data, expected);
    end
  endtask

  initial begin
    repeat (2) step();
    reset = 0;
    active = 1;
    pulse_stage(0);
    step();
    step();
    pulse_stage(1);
    step();
    pulse_stage(2);
    step();
    pulse_stage(3);
    step();
    pulse_stage(4);
    step();
    performance_event = 9'b111111111;
    performance_event_valid = 1;
    pending_pixels = 6'd32;
    pending_pixels_valid = 1;
    step();
    performance_event = 0;
    performance_event_valid = 0;
    pending_pixels = 0;
    pending_pixels_valid = 0;
    expect_counter(1, 3);
    expect_counter(2, 2);
    expect_counter(3, 2);
    expect_counter(4, 2);
    expect_counter(5, 2);
    expect_counter(6, 1);
    expect_counter(11, 1);
    expect_counter(16, 1);
    expect_counter(17, 1);
    expect_counter(18, 1);
    expect_counter(19, 1);
    expect_counter(20, 1);
    expect_counter(21, 1);
    expect_counter(22, 32);
    expect_counter(23, 32);
    expect_counter(62, 0);

    color_request_ready = 0;
    color_request_valid = 1;
    step();
    color_request_ready = 1;
    step();
    color_request_valid = 0;
    color_mem_read = 1;
    color_mem_waitRequestn = 0;
    step();
    color_mem_waitRequestn = 1;
    step();
    color_mem_read = 0;
    color_mem_readDataValid = 1;
    step();
    color_mem_readDataValid = 0;
    color_response_ready = 0;
    color_response_valid = 1;
    step();
    color_response_ready = 1;
    step();
    color_response_valid = 0;
    expect_counter(24, 1);
    expect_counter(25, 1);
    expect_counter(26, 0);
    expect_counter(27, 1);
    expect_counter(28, 0);
    expect_counter(29, 0);
    expect_counter(30, 0);
    expect_counter(31, 1);
    expect_counter(32, 1);
    expect_counter(34, 1);
    expect_counter(35, 0);

    color_write_request = 66'd1 << 58;
    color_write_request_valid = 1;
    step();
    color_write_request_valid = 0;
    color_write_mem_write = 1;
    step();
    color_write_mem_write = 0;
    color_write_response_valid = 1;
    step();
    color_write_response_valid = 0;
    expect_counter(24, 2);
    expect_counter(25, 1);
    expect_counter(26, 1);
    expect_counter(33, 1);

    clear = 1;
    step();
    clear = 0;
    expect_counter(1, 0);
    expect_counter(24, 0);
    cache_done = 1;
    step();
    cache_done = 0;
    step();
    expect_counter(2, 0);
    expect_counter(62, 1);
    $display("XlsPipelineProfilerTb PASS");
    $finish;
  end
endmodule
