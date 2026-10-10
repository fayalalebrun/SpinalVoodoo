module XlsTransferProfilerLegacy (
  input  wire         clk,
  input  wire         reset,
  input  wire         clear,
  input  wire         active,
  input  wire [65:0]  request,
  input  wire         request_valid,
  input  wire         request_ready,
  input  wire         response_valid,
  input  wire         response_ready,
  input  wire         mem_read,
  input  wire         mem_write,
  input  wire         mem_waitRequestn,
  input  wire [10:0]  mem_burstCount,
  input  wire         mem_readDataValid,
  input  wire [3:0]   read_index,
  output reg  [31:0]  read_data
);
  reg [6:0] queued_requests;
  reg [6:0] outstanding_transfers;
  reg [12:0] pending_read_beats;
  reg [31:0] requests;
  reg [31:0] reads;
  reg [31:0] writes;
  reg [31:0] request_stall_cycles;
  reg [31:0] transfer_occupancy_cycles;
  reg [31:0] request_queue_cycles;
  reg [31:0] read_beat_cycles;
  reg [31:0] avalon_wait_cycles;
  reg [31:0] avalon_read_commands;
  reg [31:0] avalon_write_commands;
  reg [31:0] read_beats;
  reg [31:0] response_stall_cycles;
  wire request_fire = request_valid && request_ready;
  wire response_fire = response_valid && response_ready;
  wire mem_read_fire = mem_read && mem_waitRequestn;
  wire mem_write_fire = mem_write && mem_waitRequestn;
  wire mem_fire = mem_read_fire || mem_write_fire;

  always @(posedge clk) begin
    if (reset || clear) begin
      queued_requests <= 0;
      outstanding_transfers <= 0;
      pending_read_beats <= 0;
      requests <= 0;
      reads <= 0;
      writes <= 0;
      request_stall_cycles <= 0;
      transfer_occupancy_cycles <= 0;
      request_queue_cycles <= 0;
      read_beat_cycles <= 0;
      avalon_wait_cycles <= 0;
      avalon_read_commands <= 0;
      avalon_write_commands <= 0;
      read_beats <= 0;
      response_stall_cycles <= 0;
    end else begin
      queued_requests <= queued_requests + request_fire - mem_fire;
      outstanding_transfers <= outstanding_transfers + request_fire - response_fire;
      pending_read_beats <= pending_read_beats +
          (mem_read_fire ? mem_burstCount : 11'd0) - mem_readDataValid;
      if (active) begin
        requests <= requests + request_fire;
        reads <= reads + (request_fire && !request[58]);
        writes <= writes + (request_fire && request[58]);
        request_stall_cycles <= request_stall_cycles + (request_valid && !request_ready);
        transfer_occupancy_cycles <= transfer_occupancy_cycles + outstanding_transfers;
        request_queue_cycles <= request_queue_cycles + queued_requests;
        read_beat_cycles <= read_beat_cycles + pending_read_beats;
        avalon_wait_cycles <= avalon_wait_cycles +
            ((mem_read || mem_write) && !mem_waitRequestn);
        avalon_read_commands <= avalon_read_commands + mem_read_fire;
        avalon_write_commands <= avalon_write_commands + mem_write_fire;
        read_beats <= read_beats + mem_readDataValid;
        response_stall_cycles <= response_stall_cycles +
            (response_valid && !response_ready);
      end
    end
  end

  always @(*) begin
    case (read_index)
      0: read_data = requests;
      1: read_data = reads;
      2: read_data = writes;
      3: read_data = request_stall_cycles;
      4: read_data = transfer_occupancy_cycles;
      5: read_data = request_queue_cycles;
      6: read_data = read_beat_cycles;
      7: read_data = avalon_wait_cycles;
      8: read_data = avalon_read_commands;
      9: read_data = avalon_write_commands;
      10: read_data = read_beats;
      11: read_data = response_stall_cycles;
      default: read_data = 0;
    endcase
  end
endmodule

module XlsTransferProfiler (
  input  wire         clk,
  input  wire         reset,
  input  wire         clear,
  input  wire [65:0]  request_a,
  input  wire         request_valid_a,
  input  wire         request_ready_a,
  input  wire         mem_read_a,
  input  wire         mem_write_a,
  input  wire         mem_waitRequestn_a,
  input  wire         mem_readDataValid_a,
  input  wire [65:0]  request_b,
  input  wire         request_valid_b,
  input  wire         request_ready_b,
  input  wire         mem_read_b,
  input  wire         mem_write_b,
  input  wire         mem_waitRequestn_b,
  input  wire         mem_readDataValid_b,
  input  wire [3:0]   read_index,
  output reg  [31:0]  read_data
);
  reg [31:0] requests;
  reg [31:0] reads;
  reg [31:0] writes;
  reg [31:0] request_stall_cycles;
  reg [31:0] avalon_wait_cycles;
  reg [31:0] avalon_read_commands;
  reg [31:0] avalon_write_commands;
  reg [31:0] read_beats;
  wire request_fire_a = request_valid_a && request_ready_a;
  wire request_fire_b = request_valid_b && request_ready_b;
  wire mem_read_fire_a = mem_read_a && mem_waitRequestn_a;
  wire mem_read_fire_b = mem_read_b && mem_waitRequestn_b;
  wire mem_write_fire_a = mem_write_a && mem_waitRequestn_a;
  wire mem_write_fire_b = mem_write_b && mem_waitRequestn_b;

  always @(posedge clk) begin
    if (reset || clear) begin
      requests <= 0;
      reads <= 0;
      writes <= 0;
      request_stall_cycles <= 0;
      avalon_wait_cycles <= 0;
      avalon_read_commands <= 0;
      avalon_write_commands <= 0;
      read_beats <= 0;
    end else begin
      requests <= requests + request_fire_a + request_fire_b;
      reads <= reads + (request_fire_a && !request_a[58]) +
          (request_fire_b && !request_b[58]);
      writes <= writes + (request_fire_a && request_a[58]) +
          (request_fire_b && request_b[58]);
      request_stall_cycles <= request_stall_cycles +
          (request_valid_a && !request_ready_a) + (request_valid_b && !request_ready_b);
      avalon_wait_cycles <= avalon_wait_cycles +
          ((mem_read_a || mem_write_a) && !mem_waitRequestn_a) +
          ((mem_read_b || mem_write_b) && !mem_waitRequestn_b);
      avalon_read_commands <= avalon_read_commands + mem_read_fire_a + mem_read_fire_b;
      avalon_write_commands <= avalon_write_commands + mem_write_fire_a + mem_write_fire_b;
      read_beats <= read_beats + mem_readDataValid_a + mem_readDataValid_b;
    end
  end

  always @(*) begin
    case (read_index)
      0: read_data = requests;
      1: read_data = reads;
      2: read_data = writes;
      3: read_data = request_stall_cycles;
      7: read_data = avalon_wait_cycles;
      8: read_data = avalon_read_commands;
      9: read_data = avalon_write_commands;
      10: read_data = read_beats;
      default: read_data = 0;
    endcase
  end
endmodule

module XlsPipelineProfiler #(
  parameter DETAILED = 1
) (
  input  wire         clk,
  input  wire         reset,
  input  wire         clear,
  input  wire         active,
  input  wire [8:0]   performance_event,
  input  wire         performance_event_valid,
  input  wire         interpolation_start,
  input  wire         pipeline_start,
  input  wire         cache_done,
  input  wire         read_done,
  input  wire         write_done,
  input wire [5:0] pending_pixels,
  input  wire         pending_pixels_valid,
  input  wire [65:0]  color_read_request,
  input  wire         color_read_request_valid,
  input  wire         color_read_request_ready,
  input  wire         color_read_response_valid,
  input  wire         color_read_response_ready,
  input  wire         color_read_mem_read,
  input  wire         color_read_mem_write,
  input  wire         color_read_mem_waitRequestn,
  input  wire [10:0]  color_read_mem_burstCount,
  input  wire         color_read_mem_readDataValid,
  input  wire         color_read_data_stall,
  input  wire         color_read_inflight_stall,
  input  wire         color_read_unissued_stall,
  input  wire         color_read_response_stall,
  input  wire [65:0]  color_write_request,
  input  wire         color_write_request_valid,
  input  wire         color_write_request_ready,
  input  wire         color_write_response_valid,
  input  wire         color_write_response_ready,
  input  wire         color_write_mem_read,
  input  wire         color_write_mem_write,
  input  wire         color_write_mem_waitRequestn,
  input  wire [10:0]  color_write_mem_burstCount,
  input  wire         color_write_mem_readDataValid,
  input  wire [65:0]  aux_read_request,
  input  wire         aux_read_request_valid,
  input  wire         aux_read_request_ready,
  input  wire         aux_read_response_valid,
  input  wire         aux_read_response_ready,
  input  wire         aux_read_mem_read,
  input  wire         aux_read_mem_write,
  input  wire         aux_read_mem_waitRequestn,
  input  wire [10:0]  aux_read_mem_burstCount,
  input  wire         aux_read_mem_readDataValid,
  input  wire         aux_read_data_stall,
  input  wire         aux_read_inflight_stall,
  input  wire         aux_read_unissued_stall,
  input  wire         aux_read_response_stall,
  input  wire [65:0]  aux_write_request,
  input  wire         aux_write_request_valid,
  input  wire         aux_write_request_ready,
  input  wire         aux_write_response_valid,
  input  wire         aux_write_response_ready,
  input  wire         aux_write_mem_read,
  input  wire         aux_write_mem_write,
  input  wire         aux_write_mem_waitRequestn,
  input  wire [10:0]  aux_write_mem_burstCount,
  input  wire         aux_write_mem_readDataValid,
  input  wire [65:0]  texture_request,
  input  wire         texture_request_valid,
  input  wire         texture_request_ready,
  input  wire         texture_response_valid,
  input  wire         texture_response_ready,
  input  wire         texture_mem_read,
  input  wire         texture_mem_write,
  input  wire         texture_mem_waitRequestn,
  input  wire [10:0]  texture_mem_burstCount,
  input  wire         texture_mem_readDataValid,
  input  wire [5:0]   read_index,
  output reg  [31:0]  read_data
);
  reg [5:0] interpolation_occupancy;
  reg [5:0] cache_occupancy;
  reg [5:0] read_occupancy;
  reg [5:0] write_occupancy;
  reg [5:0] retire_occupancy;
   reg [21:0] interpolation_cycles;
   reg [21:0] cache_cycles;
   reg [21:0] framebuffer_read_cycles;
   reg [21:0] framebuffer_write_cycles;
  reg [31:0] retire_cycles;
  reg [31:0] interpolation_max;
  reg [31:0] cache_max;
  reg [31:0] read_max;
  reg [31:0] write_max;
  reg [31:0] retire_max;
  reg [31:0] interpolation_starts;
  reg [31:0] pipeline_starts;
  reg [31:0] cache_completions;
  reg [31:0] read_completions;
  reg [31:0] write_completions;
  reg [31:0] pipeline_completions;
  reg [31:0] textured_pixels;
  reg [31:0] depth_read_pixels;
  reg [31:0] blend_read_pixels;
  reg [31:0] color_write_pixels;
  reg [31:0] aux_write_pixels;
  reg [31:0] pending_pixel_sum;
  reg [31:0] pending_pixel_max;
  reg [31:0] active_empty_cycles;
  reg [31:0] active_busy_cycles;
  reg [31:0] event_errors;
  wire any_read_request_stall =
      (color_read_request_valid && !color_read_request_ready) ||
      (aux_read_request_valid && !aux_read_request_ready);
  wire any_read_data_stall = color_read_data_stall || aux_read_data_stall;
  wire any_read_inflight_stall = color_read_inflight_stall || aux_read_inflight_stall;
  wire any_read_unissued_stall = color_read_unissued_stall || aux_read_unissued_stall;
  wire any_read_response_stall = color_read_response_stall || aux_read_response_stall;
  wire pipeline_done = performance_event_valid && performance_event[5];
  wire input_pixel = performance_event_valid && performance_event[8];
  wire any_stage_busy = interpolation_occupancy != 0 || cache_occupancy != 0 ||
      read_occupancy != 0 || write_occupancy != 0 || retire_occupancy != 0;
   wire [31:0] color_profile_data;
   wire [31:0] aux_profile_data;
  wire [31:0] texture_profile_data;
  wire [3:0] color_read_index = read_index - 6'd24;
  wire [3:0] aux_read_index = read_index - 6'd36;
  wire [3:0] texture_read_index = read_index - 6'd48;

   XlsTransferProfiler color_profiler (
     .clk(clk), .reset(reset), .clear(clear),
     .request_a(color_read_request), .request_valid_a(color_read_request_valid),
     .request_ready_a(color_read_request_ready), .mem_read_a(color_read_mem_read),
     .mem_write_a(color_read_mem_write), .mem_waitRequestn_a(color_read_mem_waitRequestn),
     .mem_readDataValid_a(color_read_mem_readDataValid), .request_b(color_write_request),
     .request_valid_b(color_write_request_valid), .request_ready_b(color_write_request_ready),
     .mem_read_b(color_write_mem_read), .mem_write_b(color_write_mem_write),
     .mem_waitRequestn_b(color_write_mem_waitRequestn),
     .mem_readDataValid_b(color_write_mem_readDataValid), .read_index(color_read_index),
     .read_data(color_profile_data)
   );
   XlsTransferProfiler aux_profiler (
     .clk(clk), .reset(reset), .clear(clear),
     .request_a(aux_read_request), .request_valid_a(aux_read_request_valid),
     .request_ready_a(aux_read_request_ready), .mem_read_a(aux_read_mem_read),
     .mem_write_a(aux_read_mem_write), .mem_waitRequestn_a(aux_read_mem_waitRequestn),
     .mem_readDataValid_a(aux_read_mem_readDataValid), .request_b(aux_write_request),
     .request_valid_b(aux_write_request_valid), .request_ready_b(aux_write_request_ready),
     .mem_read_b(aux_write_mem_read), .mem_write_b(aux_write_mem_write),
     .mem_waitRequestn_b(aux_write_mem_waitRequestn),
     .mem_readDataValid_b(aux_write_mem_readDataValid), .read_index(aux_read_index),
     .read_data(aux_profile_data)
   );
   XlsTransferProfiler texture_profiler (
     .clk(clk), .reset(reset), .clear(clear),
     .request_a(texture_request), .request_valid_a(texture_request_valid),
     .request_ready_a(texture_request_ready), .mem_read_a(texture_mem_read),
     .mem_write_a(texture_mem_write), .mem_waitRequestn_a(texture_mem_waitRequestn),
     .mem_readDataValid_a(texture_mem_readDataValid), .request_b(66'b0),
     .request_valid_b(1'b0), .request_ready_b(1'b0), .mem_read_b(1'b0),
     .mem_write_b(1'b0), .mem_waitRequestn_b(1'b1), .mem_readDataValid_b(1'b0),
     .read_index(texture_read_index), .read_data(texture_profile_data)
   );

  always @(posedge clk) begin
    if (reset || clear) begin
      interpolation_occupancy <= 0;
      cache_occupancy <= 0;
      read_occupancy <= 0;
      write_occupancy <= 0;
      retire_occupancy <= 0;
      interpolation_cycles <= 0;
      cache_cycles <= 0;
      framebuffer_read_cycles <= 0;
      framebuffer_write_cycles <= 0;
      retire_cycles <= 0;
      interpolation_max <= 0;
      cache_max <= 0;
      read_max <= 0;
      write_max <= 0;
      retire_max <= 0;
      interpolation_starts <= 0;
      pipeline_starts <= 0;
      cache_completions <= 0;
      read_completions <= 0;
      write_completions <= 0;
      pipeline_completions <= 0;
      textured_pixels <= 0;
      depth_read_pixels <= 0;
      blend_read_pixels <= 0;
      color_write_pixels <= 0;
      aux_write_pixels <= 0;
      pending_pixel_sum <= 0;
      pending_pixel_max <= 0;
      active_empty_cycles <= 0;
      active_busy_cycles <= 0;
      event_errors <= 0;
    end else begin
      if (DETAILED) begin
        interpolation_cycles <= interpolation_cycles + interpolation_occupancy;
        cache_cycles <= cache_cycles + cache_occupancy;
        framebuffer_read_cycles <= framebuffer_read_cycles + read_occupancy;
        framebuffer_write_cycles <= framebuffer_write_cycles + write_occupancy;
        retire_cycles <= retire_cycles + retire_occupancy;
      end
      if (interpolation_start && !pipeline_start)
        interpolation_occupancy <= interpolation_occupancy + 1;
      else if (pipeline_start && !interpolation_start && interpolation_occupancy != 0)
        interpolation_occupancy <= interpolation_occupancy - 1;
      if (pipeline_start && !cache_done)
        cache_occupancy <= cache_occupancy + 1;
      else if (cache_done && !pipeline_start && cache_occupancy != 0)
        cache_occupancy <= cache_occupancy - 1;
      if (cache_done && !read_done)
        read_occupancy <= read_occupancy + 1;
      else if (read_done && !cache_done && read_occupancy != 0)
        read_occupancy <= read_occupancy - 1;
      if (read_done && !write_done)
        write_occupancy <= write_occupancy + 1;
      else if (write_done && !read_done && write_occupancy != 0)
        write_occupancy <= write_occupancy - 1;
      if (write_done && !pipeline_done)
        retire_occupancy <= retire_occupancy + 1;
      else if (pipeline_done && !write_done && retire_occupancy != 0)
        retire_occupancy <= retire_occupancy - 1;
      if (DETAILED) begin
        if (interpolation_start && !pipeline_start && interpolation_occupancy + 1 > interpolation_max)
          interpolation_max <= interpolation_occupancy + 1;
        if (pipeline_start && !cache_done && cache_occupancy + 1 > cache_max)
          cache_max <= cache_occupancy + 1;
        if (cache_done && !read_done && read_occupancy + 1 > read_max)
          read_max <= read_occupancy + 1;
        if (read_done && !write_done && write_occupancy + 1 > write_max)
          write_max <= write_occupancy + 1;
        if (write_done && !pipeline_done && retire_occupancy + 1 > retire_max)
          retire_max <= retire_occupancy + 1;
        interpolation_starts <= interpolation_starts + interpolation_start;
        pipeline_starts <= pipeline_starts + pipeline_start;
        cache_completions <= cache_completions + cache_done;
        read_completions <= read_completions + read_done;
        write_completions <= write_completions + write_done;
        pipeline_completions <= pipeline_completions + pipeline_done;
        if (pending_pixels_valid) begin
          pending_pixel_sum <= pending_pixel_sum + pending_pixels;
          if (pending_pixels > pending_pixel_max) pending_pixel_max <= pending_pixels;
        end
        if (input_pixel) begin
          textured_pixels <= textured_pixels + performance_event[4];
          depth_read_pixels <= depth_read_pixels + performance_event[3];
          blend_read_pixels <= blend_read_pixels + performance_event[2];
          color_write_pixels <= color_write_pixels + performance_event[1];
          aux_write_pixels <= aux_write_pixels + performance_event[0];
        end
        if (active && any_stage_busy) active_busy_cycles <= active_busy_cycles + 1;
        if (active && !any_stage_busy) active_empty_cycles <= active_empty_cycles + 1;
        event_errors <= event_errors +
            (pipeline_start && !interpolation_start && interpolation_occupancy == 0) +
            (cache_done && !pipeline_start && cache_occupancy == 0) +
            (read_done && !cache_done && read_occupancy == 0) +
            (write_done && !read_done && write_occupancy == 0) +
            (pipeline_done && !write_done && retire_occupancy == 0);
      end
    end
  end

  always @(*) begin
    if (!DETAILED) begin
      case (read_index)
        0: read_data = 32'h000f0000;
        default: read_data = 0;
      endcase
    end else begin
      case (read_index)
      0: read_data = 32'h00020000;
      1: read_data = interpolation_cycles;
      2: read_data = cache_cycles;
      3: read_data = framebuffer_read_cycles;
      4: read_data = framebuffer_write_cycles;
      5: read_data = retire_cycles;
      6: read_data = interpolation_max;
      7: read_data = cache_max;
      8: read_data = read_max;
      9: read_data = write_max;
      10: read_data = retire_max;
      11: read_data = interpolation_starts;
      12: read_data = pipeline_starts;
      13: read_data = cache_completions;
      14: read_data = read_completions;
      15: read_data = write_completions;
      16: read_data = pipeline_completions;
      17: read_data = textured_pixels;
      18: read_data = depth_read_pixels;
      19: read_data = blend_read_pixels;
      20: read_data = color_write_pixels;
      21: read_data = aux_write_pixels;
      22: read_data = pending_pixel_sum;
      23: read_data = pending_pixel_max;
      60: read_data = active_empty_cycles;
      61: read_data = active_busy_cycles;
      62: read_data = event_errors;
      default: begin
        if (read_index >= 24 && read_index < 36) read_data = color_profile_data;
        else if (read_index >= 36 && read_index < 48) read_data = aux_profile_data;
        else if (read_index >= 48 && read_index < 60) read_data = texture_profile_data;
        else read_data = 0;
      end
      endcase
    end
  end
endmodule
