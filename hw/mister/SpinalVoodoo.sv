module emu (
  `include "sys/emu_ports.vh"
);

localparam CONF_STR = {"SpinalVoodoo;;", "T[0],Reset;", "R[0],Reset and close OSD;", "V,XLS mailbox v1;"};
wire [127:0] status;
wire [1:0] buttons;
wire [10:0] ps2_key;
wire [35:0] extension;
wire [21:0] gamma_bus;
assign extension[32] = 0;
assign extension[15:0] = 0;
assign gamma_bus[21] = 0;
wire [31:0] sd_lba[1];
wire [5:0] sd_blk_cnt[1];
wire [7:0] sd_buff_din[1];
assign sd_lba[0] = 0;
assign sd_blk_cnt[0] = 0;
assign sd_buff_din[0] = 0;

hps_io #(.CONF_STR(CONF_STR), .PS2DIV(0)) hps_io (
  .clk_sys(CLK_50M), .HPS_BUS(HPS_BUS), .EXT_BUS(extension), .gamma_bus(gamma_bus),
  .buttons(buttons), .status(status), .ps2_key(ps2_key),
  .joystick_0(), .joystick_1(), .joystick_2(), .joystick_3(), .joystick_4(), .joystick_5(),
  .joystick_l_analog_0(), .joystick_l_analog_1(), .joystick_l_analog_2(),
  .joystick_l_analog_3(), .joystick_l_analog_4(), .joystick_l_analog_5(),
  .joystick_r_analog_0(), .joystick_r_analog_1(), .joystick_r_analog_2(),
  .joystick_r_analog_3(), .joystick_r_analog_4(), .joystick_r_analog_5(),
  .paddle_0(), .paddle_1(), .paddle_2(), .paddle_3(), .paddle_4(), .paddle_5(),
  .spinner_0(), .spinner_1(), .spinner_2(), .spinner_3(), .spinner_4(), .spinner_5(),
  .ps2_kbd_clk_out(), .ps2_kbd_data_out(), .ps2_mouse_clk_out(), .ps2_mouse_data_out(),
  .ps2_mouse(), .ps2_mouse_ext(), .forced_scandoubler(), .direct_video(),
  .img_mounted(), .img_readonly(), .img_size(), .sd_ack(), .sd_buff_addr(),
  .sd_buff_dout(), .sd_buff_wr(), .ioctl_download(), .ioctl_index(), .ioctl_wr(),
  .ioctl_addr(), .ioctl_dout(), .ioctl_upload(), .ioctl_rd(), .ioctl_file_ext(),
  .sdram_sz(), .RTC(), .TIMESTAMP(), .uart_mode(), .uart_speed(),
  .status_in(128'd0), .status_set(1'b0), .status_menumask(16'd0),
  .video_rotated(1'b0), .new_vmode(1'b0), .info_req(1'b0), .info(8'd0),
  .ps2_kbd_clk_in(1'b1), .ps2_kbd_data_in(1'b1),
  .ps2_mouse_clk_in(1'b1), .ps2_mouse_data_in(1'b1),
  .ps2_kbd_led_status(3'd0), .ps2_kbd_led_use(3'd0),
  .joystick_0_rumble(16'd0), .joystick_1_rumble(16'd0), .joystick_2_rumble(16'd0),
  .joystick_3_rumble(16'd0), .joystick_4_rumble(16'd0), .joystick_5_rumble(16'd0),
  .sd_lba(sd_lba), .sd_blk_cnt(sd_blk_cnt), .sd_rd(1'b0), .sd_wr(1'b0), .sd_buff_din(sd_buff_din),
  .ioctl_upload_req(1'b0), .ioctl_upload_index(8'd0), .ioctl_din(8'd0), .ioctl_wait(1'b0)
);

wire pixel_clock;
wire pixel_locked;
wire resetting;
wire underflow;
de10_hdmi_pll pixel_pll (.refclk(CLK_50M), .rst(1'b0), .outclk_0(pixel_clock), .locked(pixel_locked));
MisterCore core (
  .clk(CLK_50M), .io_pixelClock(pixel_clock),
  .io_resetRequest(RESET | status[0] | buttons[1] | ~pixel_locked),
  .io_ps2Key(ps2_key), .io_osdOpen(OSD_STATUS), .io_vblank(FB_VBL),
  .io_ddr_busy(DDRAM_BUSY), .io_ddr_address(DDRAM_ADDR), .io_ddr_burstCount(DDRAM_BURSTCNT),
  .io_ddr_read(DDRAM_RD), .io_ddr_write(DDRAM_WE), .io_ddr_writeData(DDRAM_DIN),
  .io_ddr_byteEnable(DDRAM_BE), .io_ddr_readData(DDRAM_DOUT), .io_ddr_readDataValid(DDRAM_DOUT_READY),
  .io_frame_sequence(), .io_frame_base(), .io_frame_width(), .io_frame_height(),
  .io_frame_stride(), .io_frame_format(), .io_frame_enable(),
  .io_video_rgb_r(VGA_R), .io_video_rgb_g(VGA_G), .io_video_rgb_b(VGA_B),
  .io_video_de(VGA_DE), .io_video_hSync(VGA_HS), .io_video_vSync(VGA_VS),
  .io_resetting(resetting), .io_underflow(underflow)
);

assign CLK_VIDEO = pixel_clock;
assign CE_PIXEL = 1'b1;
assign DDRAM_CLK = CLK_50M;
assign VIDEO_ARX = 13'd4;
assign VIDEO_ARY = 13'd3;
assign FB_FORCE_BLANK = resetting;
assign FB_EN = 0;
assign FB_BASE = 0;
assign FB_WIDTH = 0;
assign FB_HEIGHT = 0;
assign FB_STRIDE = 0;
assign FB_FORMAT = 0;
assign VGA_F1 = 0;
assign VGA_SL = 0;
assign VGA_SCALER = FB_EN;
assign VGA_DISABLE = 0;
assign HDMI_FREEZE = 0;
assign HDMI_BLACKOUT = 0;
assign HDMI_BOB_DEINT = 0;
assign LED_USER = underflow;
assign LED_POWER = 0;
assign LED_DISK = 0;
assign BUTTONS = 0;
assign AUDIO_L = 0;
assign AUDIO_R = 0;
assign AUDIO_S = 0;
assign AUDIO_MIX = 0;
assign ADC_BUS = 'z;
assign USER_OUT = '1;
assign {UART_RTS, UART_TXD, UART_DTR} = 0;
assign {SD_SCK, SD_MOSI, SD_CS} = 'z;
assign {SDRAM_DQ, SDRAM_A, SDRAM_BA, SDRAM_CLK, SDRAM_CKE, SDRAM_DQML, SDRAM_DQMH,
        SDRAM_nWE, SDRAM_nCAS, SDRAM_nRAS, SDRAM_nCS} = 'z;

endmodule
