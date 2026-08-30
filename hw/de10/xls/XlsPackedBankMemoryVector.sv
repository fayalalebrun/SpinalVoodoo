module XlsPackedBankMemoryVector (
  input wire clk,
  input wire reset,
  input wire [261:0] slow_request,
  input wire slow_request_valid,
  output wire slow_request_ready,
  input wire [108:0] fast_request,
  input wire fast_request_valid,
  output wire fast_request_ready,
  output wire [280:0] slow_response,
  output wire slow_response_valid,
  input wire slow_response_ready,
  output wire [280:0] fast_response,
  output wire fast_response_valid,
  input wire fast_response_ready
);
  reg [216:0] slow_queued_request;
  reg [64:0] fast_queued_request;
  reg slow_queued_valid;
  reg fast_queued_valid;
  reg slow_pending_valid;
  reg fast_pending_valid;
  reg [3:0] slow_pending_mask;
  reg [3:0] fast_pending_mask;
  reg [20:0] slow_pending_read;
  reg [20:0] fast_pending_read;

  wire slow_available = slow_queued_valid || slow_request_valid;
  wire fast_available = fast_queued_valid || fast_request_valid;
  wire [3:0] fast_selected = fast_queued_valid ? fast_queued_request[4:1] : fast_request[4:1];
  wire [15:0] fast_locations = fast_queued_valid ? fast_queued_request[20:5] : fast_request[20:5];

  function automatic [3:0] taps_for_bank(
    input [3:0] selected,
    input [15:0] locations,
    input [1:0] bank
  );
    begin
      taps_for_bank = {
        selected[3] && locations[14:13] == bank,
        selected[2] && locations[10:9] == bank,
        selected[1] && locations[6:5] == bank,
        selected[0] && locations[2:1] == bank
      };
    end
  endfunction

  function automatic [1:0] first_tap(input [3:0] taps);
    begin
      if (taps[0]) first_tap = 0;
      else if (taps[1]) first_tap = 1;
      else if (taps[2]) first_tap = 2;
      else first_tap = 3;
    end
  endfunction

  wire [21:0] fast_entry0 = fast_queued_valid
      ? {11'b0, fast_queued_request[31:21]} : fast_request[42:21];
  wire [21:0] fast_entry1 = fast_queued_valid
      ? {11'b0, fast_queued_request[42:32]} : fast_request[64:43];
  wire [21:0] fast_entry2 = fast_queued_valid
      ? {11'b0, fast_queued_request[53:43]} : fast_request[86:65];
  wire [21:0] fast_entry3 = fast_queued_valid
      ? {11'b0, fast_queued_request[64:54]} : fast_request[108:87];
  wire [21:0] fast_entry [0:3];
  assign fast_entry[0] = fast_entry0;
  assign fast_entry[1] = fast_entry1;
  assign fast_entry[2] = fast_entry2;
  assign fast_entry[3] = fast_entry3;

  wire [3:0] fast_taps [0:3];
  wire [3:0] fast_victim [0:3];
  wire [1:0] fast_first [0:3];
  wire [3:0] fast_different [0:3];
  wire [1:0] fast_second [0:3];
  wire [58:0] fast_command [0:3];
  wire [3:0] fast_mask;
  genvar bank_index;
  generate
    for (bank_index = 0; bank_index < 4; bank_index = bank_index + 1) begin : fast_commands
      assign fast_taps[bank_index] = taps_for_bank(fast_selected, fast_locations, bank_index[1:0]);
      assign fast_victim[bank_index] = {
        fast_taps[bank_index][3] && fast_entry3[10],
        fast_taps[bank_index][2] && fast_entry2[10],
        fast_taps[bank_index][1] && fast_entry1[10],
        fast_taps[bank_index][0] && fast_entry0[10]
      };
      assign fast_first[bank_index] = |fast_victim[bank_index]
          ? first_tap(fast_victim[bank_index]) : first_tap(fast_taps[bank_index]);
      assign fast_different[bank_index] = {
        fast_taps[bank_index][3] && fast_entry3 != fast_entry[fast_first[bank_index]],
        fast_taps[bank_index][2] && fast_entry2 != fast_entry[fast_first[bank_index]],
        fast_taps[bank_index][1] && fast_entry1 != fast_entry[fast_first[bank_index]],
        fast_taps[bank_index][0] && fast_entry0 != fast_entry[fast_first[bank_index]]
      };
      assign fast_second[bank_index] = first_tap(fast_different[bank_index]);
      assign fast_command[bank_index] = {
        1'b0,
        fast_entry[fast_first[bank_index]],
        10'b0,
        fast_entry[fast_second[bank_index]],
        3'b100,
        |fast_different[bank_index]
      };
      assign fast_mask[bank_index] = |fast_taps[bank_index];
    end
  endgenerate

  wire [3:0] packed_location [0:3];
  genvar tap_index;
  generate
    for (tap_index = 0; tap_index < 4; tap_index = tap_index + 1) begin : packed_locations
      wire [3:0] location = fast_locations[tap_index * 4 +: 4];
      wire [1:0] logical_bank = location[2:1];
      wire different_entry = fast_entry[tap_index] != fast_entry[fast_first[logical_bank]];
      assign packed_location[tap_index] = {different_entry, location[2:1], location[0]};
    end
  endgenerate
  wire [20:0] fast_read = {
    packed_location[3], packed_location[2], packed_location[1], packed_location[0],
    fast_selected, fast_queued_valid ? fast_queued_request[0] : fast_request[0]
  };

  wire [3:0] slow_mask = slow_queued_valid
      ? slow_queued_request[216:213] : slow_request[260:257];
  wire [47:0] slow_compact_command [0:3];
  assign slow_compact_command[0] = slow_queued_valid
      ? slow_queued_request[68:21] : {slow_request[79], slow_request[67:21]};
  assign slow_compact_command[1] = slow_queued_valid
      ? slow_queued_request[116:69] : {slow_request[138], slow_request[126:80]};
  assign slow_compact_command[2] = slow_queued_valid
      ? slow_queued_request[164:117] : {slow_request[197], slow_request[185:139]};
  assign slow_compact_command[3] = slow_queued_valid
      ? slow_queued_request[212:165] : {slow_request[256], slow_request[244:198]};
  wire [58:0] slow_command [0:3];
  assign slow_command[0] = {slow_compact_command[0][47], 11'b0, slow_compact_command[0][46:0]};
  assign slow_command[1] = {slow_compact_command[1][47], 11'b0, slow_compact_command[1][46:0]};
  assign slow_command[2] = {slow_compact_command[2][47], 11'b0, slow_compact_command[2][46:0]};
  assign slow_command[3] = {slow_compact_command[3][47], 11'b0, slow_compact_command[3][46:0]};
  wire [20:0] slow_read = slow_queued_valid ? slow_queued_request[20:0] : slow_request[20:0];

  wire [3:0] slow_command_ready;
  wire [3:0] fast_command_ready;
  wire [64:0] slow_bank_response [0:3];
  wire [64:0] fast_bank_response [0:3];
  wire [3:0] slow_bank_response_valid;
  wire [3:0] fast_bank_response_valid;
  wire slow_responses_valid = slow_pending_valid &&
      ((slow_bank_response_valid & slow_pending_mask) == slow_pending_mask);
  wire fast_responses_valid = fast_pending_valid &&
      ((fast_bank_response_valid & fast_pending_mask) == fast_pending_mask);
  wire slow_response_accept = slow_responses_valid && slow_response_ready;
  wire fast_response_accept = fast_responses_valid && fast_response_ready;
  wire slow_slot_ready = !slow_pending_valid || slow_response_accept;
  wire fast_slot_ready = !fast_pending_valid || fast_response_accept;
  wire slow_all_writes =
      (!slow_mask[0] || slow_command[0][58]) &&
      (!slow_mask[1] || slow_command[1][58]) &&
      (!slow_mask[2] || slow_command[2][58]) &&
      (!slow_mask[3] || slow_command[3][58]);
  wire slow_try = slow_available && slow_slot_ready;
  wire pair_try = slow_try && fast_available && fast_slot_ready && slow_all_writes;
  wire fast_try = fast_available && fast_slot_ready && (!slow_try || pair_try);
  wire slow_banks_ready = ((slow_command_ready | ~slow_mask) == 4'hf);
  wire fast_banks_ready = ((fast_command_ready | ~fast_mask) == 4'hf);
  wire slow_dispatch = !reset && slow_try && slow_banks_ready &&
      (!pair_try || fast_banks_ready);
  wire fast_dispatch = !reset && fast_try && fast_banks_ready &&
      (!pair_try || slow_banks_ready);
  wire direct_slow_accept = slow_dispatch && !slow_queued_valid;
  wire direct_fast_accept = fast_dispatch && !fast_queued_valid;
  wire queued_slow_accept = slow_dispatch && slow_queued_valid;
  wire queued_fast_accept = fast_dispatch && fast_queued_valid;
  wire slow_queue_push = slow_request_valid && slow_request_ready && !direct_slow_accept;
  wire fast_queue_push = fast_request_valid && fast_request_ready && !direct_fast_accept;

  assign slow_request_ready = !reset && !slow_queued_valid;
  assign fast_request_ready = !reset && !fast_queued_valid;
  assign slow_response = {
    slow_pending_mask[3] ? slow_bank_response[3] : 65'b0,
    slow_pending_mask[2] ? slow_bank_response[2] : 65'b0,
    slow_pending_mask[1] ? slow_bank_response[1] : 65'b0,
    slow_pending_mask[0] ? slow_bank_response[0] : 65'b0,
    slow_pending_read
  };
  assign fast_response = {
    fast_pending_mask[3] ? fast_bank_response[3] : 65'b0,
    fast_pending_mask[2] ? fast_bank_response[2] : 65'b0,
    fast_pending_mask[1] ? fast_bank_response[1] : 65'b0,
    fast_pending_mask[0] ? fast_bank_response[0] : 65'b0,
    fast_pending_read
  };
  assign slow_response_valid = slow_responses_valid;
  assign fast_response_valid = fast_responses_valid;

  genvar memory_index;
  generate
    for (memory_index = 0; memory_index < 4; memory_index = memory_index + 1) begin : banks
      XlsConcurrentPackedBankMemory bank (
        .clk(clk),
        .reset(reset),
        .slow_command(slow_command[memory_index]),
        .slow_command_valid(slow_dispatch && slow_mask[memory_index]),
        .slow_command_ready(slow_command_ready[memory_index]),
        .slow_response(slow_bank_response[memory_index]),
        .slow_response_valid(slow_bank_response_valid[memory_index]),
        .slow_response_ready(slow_response_accept && slow_pending_mask[memory_index]),
        .fast_command(fast_command[memory_index]),
        .fast_command_valid(fast_dispatch && fast_mask[memory_index]),
        .fast_command_ready(fast_command_ready[memory_index]),
        .fast_response(fast_bank_response[memory_index]),
        .fast_response_valid(fast_bank_response_valid[memory_index]),
        .fast_response_ready(fast_response_accept && fast_pending_mask[memory_index])
      );
    end
  endgenerate

  always @(posedge clk) begin
    if (reset) begin
      slow_queued_request <= 0;
      fast_queued_request <= 0;
      slow_queued_valid <= 0;
      fast_queued_valid <= 0;
      slow_pending_valid <= 0;
      fast_pending_valid <= 0;
      slow_pending_mask <= 0;
      fast_pending_mask <= 0;
      slow_pending_read <= 0;
      fast_pending_read <= 0;
    end else begin
      if (queued_slow_accept) slow_queued_valid <= 0;
      if (queued_fast_accept) fast_queued_valid <= 0;
      if (slow_queue_push) begin
        slow_queued_request <= {
          slow_request[260:257], slow_request[256], slow_request[244:198],
          slow_request[197], slow_request[185:139], slow_request[138], slow_request[126:80],
          slow_request[79], slow_request[67:21], slow_request[20:0]
        };
        slow_queued_valid <= 1;
      end
      if (fast_queue_push) begin
        fast_queued_request <= {
          fast_request[97:87], fast_request[75:65], fast_request[53:43],
          fast_request[31:21], fast_request[20:0]
        };
        fast_queued_valid <= 1;
      end
      if (slow_response_accept) slow_pending_valid <= 0;
      if (fast_response_accept) fast_pending_valid <= 0;
      if (slow_dispatch) begin
        slow_pending_valid <= 1;
        slow_pending_mask <= slow_mask;
        slow_pending_read <= slow_read;
      end
      if (fast_dispatch) begin
        fast_pending_valid <= 1;
        fast_pending_mask <= fast_mask;
        fast_pending_read <= fast_read;
      end
    end
  end
endmodule
