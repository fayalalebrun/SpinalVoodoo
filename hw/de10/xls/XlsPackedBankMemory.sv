module XlsPackedBankMemory (
  input  wire          clk,
  input  wire          reset,
  input  wire [58:0]   command,
  input  wire          command_valid,
  output wire          command_ready,
  output wire [32:0]   response,
  output wire          response_valid,
  input  wire          response_ready
);
   reg response_pending;
   reg write_pending;
   reg saved_write;
   reg [5:0] saved_slot;
   reg [5:0] saved_word;
   reg [31:0] saved_data;
   reg [3:0] saved_mask;
   wire [31:0] lane_read_data [0:63];

  wire write = command[58];
  wire [21:0] address = command[57:36];
  wire [31:0] data = command[35:4];
  wire [3:0] mask = command[3:0];
  wire [5:0] slot = address[11:6];
  wire [5:0] word = address[5:0];

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

   wire accept = command_valid && command_ready;
   wire [31:0] saved_word_data = lane_read_data[saved_word];
   wire [31:0] write_response_data =
       (saved_word_data & ~byte_mask(saved_mask)) | (saved_data & byte_mask(saved_mask));
   wire [31:0] response_data = saved_write ? write_response_data : saved_word_data;

   assign command_ready = !reset && !write_pending && (!response_pending || response_ready);
   assign response = {response_data, 1'b1};
   assign response_valid = response_pending;

   genvar lane;
   generate
     for (lane = 0; lane < 64; lane = lane + 1) begin : lane_memory
       wire [31:0] read_data;
       wire [31:0] scalar_data =
           (read_data & ~byte_mask(saved_mask)) | (saved_data & byte_mask(saved_mask));

       assign lane_read_data[lane] = read_data;

       XlsPackedBankLaneMemory storage (
         .clk(clk),
         .read_enable(accept),
         .read_address(slot),
         .write_enable(write_pending && saved_word == lane),
         .write_address(saved_slot),
         .write_data(scalar_data),
         .read_data(read_data)
       );
     end
   endgenerate

   always @(posedge clk) begin
     if (reset) begin
       response_pending <= 0;
       write_pending <= 0;
       saved_write <= 0;
       saved_slot <= 0;
       saved_word <= 0;
       saved_data <= 0;
       saved_mask <= 0;
     end else begin
       if (response_pending && response_ready) begin
         response_pending <= 0;
       end
       if (write_pending) begin
         write_pending <= 0;
         response_pending <= 1;
       end
       if (accept) begin
         saved_write <= write;
         saved_slot <= slot;
         saved_word <= word;
         saved_data <= data;
         saved_mask <= mask;
         if (write) begin
           write_pending <= 1;
         end else begin
           response_pending <= 1;
         end
       end
     end
   end
endmodule

module XlsPackedBankLaneMemory (
  input  wire        clk,
  input  wire        read_enable,
  input  wire [5:0]  read_address,
  input  wire        write_enable,
  input  wire [5:0]  write_address,
  input  wire [31:0] write_data,
  output wire [31:0] read_data
);
  (* ramstyle = "M10K, no_rw_check" *) reg [31:0] storage [0:63];
  reg [31:0] read_data_register;

  assign read_data = read_data_register;

  always @(posedge clk) begin
    if (read_enable) begin
      read_data_register <= storage[read_address];
    end
    if (write_enable) begin
      storage[write_address] <= write_data;
    end
  end
endmodule
