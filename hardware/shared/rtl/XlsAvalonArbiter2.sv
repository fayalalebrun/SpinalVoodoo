module XlsAvalonArbiter2 (
  input  wire        clk,
  input  wire        reset,
  input  wire        m0_read,
  input  wire        m0_write,
  output wire        m0_waitRequestn,
  input  wire [10:0] m0_burstCount,
  input  wire [31:0] m0_address,
  input  wire [3:0]  m0_byteEnable,
  input  wire [31:0] m0_writeData,
  output wire        m0_readDataValid,
  output wire [31:0] m0_readData,
  input  wire        m1_read,
  input  wire        m1_write,
  output wire        m1_waitRequestn,
  input  wire [10:0] m1_burstCount,
  input  wire [31:0] m1_address,
  input  wire [3:0]  m1_byteEnable,
  input  wire [31:0] m1_writeData,
  output wire        m1_readDataValid,
  output wire [31:0] m1_readData,
  output wire        s_read,
  output wire        s_write,
  input  wire        s_waitRequestn,
  output wire [10:0] s_burstCount,
  output wire [31:0] s_address,
  output wire [3:0]  s_byteEnable,
  output wire [31:0] s_writeData,
  input  wire        s_readDataValid,
  input  wire [31:0] s_readData
);
  reg read_pending;
  reg read_owner;
  reg [10:0] read_beats_remaining;
  reg write_pending;
  reg write_owner;
  reg [10:0] write_beats_remaining;
  wire m0_request = m0_read || m0_write;
  wire m1_request = m1_read || m1_write;
  wire m0_grant = !reset && !read_pending &&
      (write_pending ? !write_owner : m0_request);
  wire m1_grant = !reset && !read_pending &&
      (write_pending ? write_owner : (!m0_request && m1_request));
  wire read_fire = s_read && s_waitRequestn;
  wire write_fire = s_write && s_waitRequestn;

  assign m0_waitRequestn = m0_grant && s_waitRequestn;
  assign m1_waitRequestn = m1_grant && s_waitRequestn;
  assign m0_readDataValid = read_pending && !read_owner && s_readDataValid;
  assign m1_readDataValid = read_pending && read_owner && s_readDataValid;
  assign m0_readData = s_readData;
  assign m1_readData = s_readData;
  assign s_read = (m0_grant && m0_read) || (m1_grant && m1_read);
  assign s_write = (m0_grant && m0_write) || (m1_grant && m1_write);
  assign s_burstCount = m0_grant ? m0_burstCount : m1_burstCount;
  assign s_address = m0_grant ? m0_address : m1_address;
  assign s_byteEnable = m0_grant ? m0_byteEnable : m1_byteEnable;
  assign s_writeData = m0_grant ? m0_writeData : m1_writeData;

  always @(posedge clk or posedge reset) begin
    if (reset) begin
      read_pending <= 1'b0;
      read_owner <= 1'b0;
      read_beats_remaining <= 0;
      write_pending <= 1'b0;
      write_owner <= 1'b0;
      write_beats_remaining <= 0;
    end else begin
      if (write_fire) begin
        if (!write_pending && s_burstCount > 1) begin
          write_pending <= 1'b1;
          write_owner <= m1_grant;
          write_beats_remaining <= s_burstCount - 1'b1;
        end else if (write_pending && write_beats_remaining == 1) begin
          write_pending <= 1'b0;
          write_beats_remaining <= 0;
        end else if (write_pending) begin
          write_beats_remaining <= write_beats_remaining - 1'b1;
        end
      end
      if (read_fire) begin
        read_pending <= 1'b1;
        read_owner <= m1_grant;
        read_beats_remaining <= s_burstCount;
      end else if (read_pending && s_readDataValid) begin
        if (read_beats_remaining == 1) begin
          read_pending <= 1'b0;
          read_beats_remaining <= 0;
        end else begin
          read_beats_remaining <= read_beats_remaining - 1'b1;
        end
      end
    end
  end
endmodule
