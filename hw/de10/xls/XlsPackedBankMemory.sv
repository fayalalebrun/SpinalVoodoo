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
   wire [31:0] read_data;

  wire write = command[58];
  wire [21:0] address = command[57:36];
  wire [31:0] data = command[35:4];
  wire [3:0] mask = command[3:0];
  wire [5:0] slot = address[11:6];
  wire [5:0] word = address[5:0];
  wire [11:0] memory_address = {word, slot};
  wire [11:0] saved_memory_address = {saved_word, saved_slot};

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
   wire [31:0] write_response_data =
       (read_data & ~byte_mask(saved_mask)) | (saved_data & byte_mask(saved_mask));
   wire [31:0] response_data = saved_write ? write_response_data : read_data;

   assign command_ready = !reset && !write_pending && (!response_pending || response_ready);
   assign response = {response_data, 1'b1};
   assign response_valid = response_pending;

   XlsPackedBankMemoryArray storage (
     .clk(clk),
     .read_enable(accept),
     .read_address(memory_address),
     .write_enable(write_pending),
     .write_address(saved_memory_address),
     .write_data(saved_data),
     .write_mask(saved_mask),
     .read_data(read_data)
   );

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

module XlsPackedBankMemoryArray (
  input  wire        clk,
  input  wire        read_enable,
  input  wire [11:0] read_address,
  input  wire        write_enable,
  input  wire [11:0] write_address,
  input  wire [31:0] write_data,
  input  wire [3:0]  write_mask,
  output wire [31:0] read_data
);
  (* ramstyle = "M10K, no_rw_check" *) reg [7:0] byte0 [0:4095];
  (* ramstyle = "M10K, no_rw_check" *) reg [7:0] byte1 [0:4095];
  (* ramstyle = "M10K, no_rw_check" *) reg [7:0] byte2 [0:4095];
  (* ramstyle = "M10K, no_rw_check" *) reg [7:0] byte3 [0:4095];
  reg [31:0] read_data_register;

  assign read_data = read_data_register;

  always @(posedge clk) begin
    if (read_enable) begin
      read_data_register <= {
        byte3[read_address], byte2[read_address], byte1[read_address], byte0[read_address]
      };
    end
    if (write_enable) begin
      if (write_mask[0]) byte0[write_address] <= write_data[7:0];
      if (write_mask[1]) byte1[write_address] <= write_data[15:8];
      if (write_mask[2]) byte2[write_address] <= write_data[23:16];
      if (write_mask[3]) byte3[write_address] <= write_data[31:24];
    end
  end
endmodule
