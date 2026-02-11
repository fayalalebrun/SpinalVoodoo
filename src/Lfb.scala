package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._

/** Linear Frame Buffer (LFB) support.
  *
  * Accepts CPU writes to the LFB address space and converts them into either:
  *   - Write.Input stream items (bypass mode, pixelPipelineEnable=0)
  *   - ColorCombine.Output stream items (pipeline mode, pixelPipelineEnable=1)
  *
  * Also supports CPU reads from the LFB address space (dual 16-bit pixel reads from FB).
  *
  * Dual-pixel formats (16-bit: 0-3, 15) produce two items per write.
  * Single-pixel formats (32-bit: 4-5, 12-14) produce one item per write.
  */
case class Lfb(c: Config) extends Component {
  val io = new Bundle {
    val bus = slave(Bmb(Lfb.bmbParams(c)))

    // Bypass mode output (pixelPipelineEnable=0)
    val writeOutput = master Stream (Write.Input(c))

    // Pipeline mode output (pixelPipelineEnable=1)
    val pipelineOutput = master Stream (ColorCombine.Output(c))

    val busy = out Bool ()

    // Config inputs from RegisterBank
    val pixelPipelineEnable = in Bool ()
    val writeFormat = in UInt (4 bits)
    val rgbaLanes = in UInt (2 bits)
    val wordSwapWrites = in Bool ()
    val byteSwizzleWrites = in Bool ()
    val enableDithering = in Bool ()
    val ditherAlgorithm = in Bool ()
    val zaColor = in Bits (32 bits)
    val enableAlphaPlanes = in Bool ()

    // Read config
    val readBufferSelect = in UInt (2 bits)
    val wordSwapReads = in Bool ()
    val byteSwizzleReads = in Bool ()

    // Framebuffer read bus (for LFB reads)
    val fbReadBus = master(Bmb(Lfb.fbReadBmbParams(c)))
    val fbWriteBaseAddr = in UInt (c.addressWidth)
    val fbReadBaseAddr = in UInt (c.addressWidth)
  }

  // Capture BMB command data
  val capturedAddr = Reg(UInt(22 bits))
  val capturedData = Reg(Bits(32 bits))
  val capturedIsRead = Reg(Bool()) init (False)
  val rspPending = Reg(Bool()) init (False)

  // Captured FB read results for read path
  val capturedRead1 = Reg(Bits(32 bits))
  val capturedRead2 = Reg(Bits(32 bits))

  // Is this a dual-pixel format?
  val isDualPixel = io.writeFormat === 0 || io.writeFormat === 1 || io.writeFormat === 2 ||
    io.writeFormat === 3 || io.writeFormat === 15

  // State machine: 3 bits to hold 6 states
  val state = RegInit(U(0, 3 bits))
  val stateIdle    = U(0, 3 bits)
  val statePixel1  = U(1, 3 bits)
  val statePixel2  = U(2, 3 bits)
  val stateRfetch1 = U(3, 3 bits)
  val stateRfetch2 = U(4, 3 bits)
  val stateRresp   = U(5, 3 bits)

  // Accept BMB commands only in IDLE and when no pending response
  io.bus.cmd.ready := (state === stateIdle) && !rspPending

  // Apply word swap and byte swizzle to incoming write data
  val rawData = io.bus.cmd.data
  val afterWordSwap = Bits(32 bits)
  when(io.wordSwapWrites) {
    afterWordSwap := rawData(15 downto 0) ## rawData(31 downto 16)
  } otherwise {
    afterWordSwap := rawData
  }
  val afterByteSwizzle = Bits(32 bits)
  when(io.byteSwizzleWrites) {
    afterByteSwizzle := afterWordSwap(7 downto 0) ## afterWordSwap(15 downto 8) ##
      afterWordSwap(23 downto 16) ## afterWordSwap(31 downto 24)
  } otherwise {
    afterByteSwizzle := afterWordSwap
  }

  // Capture on cmd.fire
  when(io.bus.cmd.fire) {
    capturedAddr := io.bus.cmd.address
    rspPending := True
    when(io.bus.cmd.opcode === Bmb.Cmd.Opcode.READ) {
      capturedIsRead := True
      state := stateRfetch1
    } otherwise {
      capturedIsRead := False
      capturedData := afterByteSwizzle
      state := statePixel1
    }
  }

  // Address decode: depends on format stride (for writes)
  val is32bitStride = io.writeFormat === 4 || io.writeFormat === 5
  val pixelX = UInt(10 bits)
  val pixelY = UInt(10 bits)
  when(is32bitStride) {
    pixelX := (capturedAddr >> 2).resize(10 bits)
    pixelY := (capturedAddr >> 12).resize(10 bits)
  } otherwise {
    pixelX := (capturedAddr >> 1).resize(10 bits)
    pixelY := (capturedAddr >> 11).resize(10 bits)
  }

  // Read address decode: always 16-bit stride
  val readPixelX = (capturedAddr >> 1).resize(10 bits)
  val readPixelY = (capturedAddr >> 11).resize(10 bits)

  // For PIXEL2: x+1
  val pixel2X = (pixelX + 1).resize(10 bits)

  // Select 16-bit pixel data: PIXEL1 = lo16, PIXEL2 = hi16
  val lo16 = capturedData(15 downto 0).asUInt
  val hi16 = capturedData(31 downto 16).asUInt

  val pixelData16 = UInt(16 bits)
  pixelData16 := (state === statePixel2) ? hi16 | lo16

  // Full 32-bit data for XRGB8888/ARGB8888
  val pixelData32 = capturedData

  // Format decode: expand to 8-bit RGB + depth + write masks
  val decodedR = UInt(8 bits)
  val decodedG = UInt(8 bits)
  val decodedB = UInt(8 bits)
  val decodedDepth = Bits(16 bits)
  val decodedRgbWrite = Bool()
  val decodedAuxWrite = Bool()

  // Default assignments
  decodedR := 0
  decodedG := 0
  decodedB := 0
  decodedDepth := io.zaColor(15 downto 0) // Default depth from zaColor when no explicit depth
  decodedRgbWrite := True
  decodedAuxWrite := False

  // RGB565 expansion matching 86Box: r = (rgb>>8)&0xf8; r |= r>>5; etc.
  def expandRgb565(v: UInt): (UInt, UInt, UInt) = {
    val r5 = v(15 downto 11)
    val g6 = v(10 downto 5)
    val b5 = v(4 downto 0)
    val r8 = (r5 @@ U(0, 3 bits)) | (r5 >> 2).resize(8 bits)
    val g8 = (g6 @@ U(0, 2 bits)) | (g6 >> 4).resize(8 bits)
    val b8 = (b5 @@ U(0, 3 bits)) | (b5 >> 2).resize(8 bits)
    (r8, g8, b8)
  }

  // RGB555 expansion
  def expandRgb555(v: UInt): (UInt, UInt, UInt) = {
    val r5 = v(14 downto 10)
    val g5 = v(9 downto 5)
    val b5 = v(4 downto 0)
    val r8 = (r5 @@ U(0, 3 bits)) | (r5 >> 2).resize(8 bits)
    val g8 = (g5 @@ U(0, 3 bits)) | (g5 >> 2).resize(8 bits)
    val b8 = (b5 @@ U(0, 3 bits)) | (b5 >> 2).resize(8 bits)
    (r8, g8, b8)
  }

  // ARGB8888 with rgbaLanes swizzle
  def decodeArgb8888(data: Bits, lanes: UInt): (UInt, UInt, UInt) = {
    val r = UInt(8 bits)
    val g = UInt(8 bits)
    val b = UInt(8 bits)
    r := data(23 downto 16).asUInt
    g := data(15 downto 8).asUInt
    b := data(7 downto 0).asUInt
    switch(lanes) {
      is(0) { // ARGB
        r := data(23 downto 16).asUInt
        g := data(15 downto 8).asUInt
        b := data(7 downto 0).asUInt
      }
      is(1) { // ABGR
        r := data(7 downto 0).asUInt
        g := data(15 downto 8).asUInt
        b := data(23 downto 16).asUInt
      }
      is(2) { // RGBA
        r := data(31 downto 24).asUInt
        g := data(23 downto 16).asUInt
        b := data(15 downto 8).asUInt
      }
      is(3) { // BGRA
        r := data(15 downto 8).asUInt
        g := data(23 downto 16).asUInt
        b := data(31 downto 24).asUInt
      }
    }
    (r, g, b)
  }

  // Decode alpha from ARGB8888 with rgbaLanes swizzle
  def decodeAlpha8888(data: Bits, lanes: UInt): UInt = {
    val a = UInt(8 bits)
    a := data(31 downto 24).asUInt
    switch(lanes) {
      is(0) { a := data(31 downto 24).asUInt } // ARGB: A=[31:24]
      is(1) { a := data(31 downto 24).asUInt } // ABGR: A=[31:24]
      is(2) { a := data(7 downto 0).asUInt }   // RGBA: A=[7:0]
      is(3) { a := data(7 downto 0).asUInt }   // BGRA: A=[7:0]
    }
    a
  }

  // Format decode logic
  switch(io.writeFormat) {
    is(0) { // RGB565 - dual pixel
      val (r, g, b) = expandRgb565(pixelData16)
      decodedR := r; decodedG := g; decodedB := b
      decodedRgbWrite := True; decodedAuxWrite := io.enableAlphaPlanes
    }
    is(1) { // RGB555 - dual pixel
      val (r, g, b) = expandRgb555(pixelData16)
      decodedR := r; decodedG := g; decodedB := b
      decodedRgbWrite := True; decodedAuxWrite := io.enableAlphaPlanes
    }
    is(2) { // ARGB1555 - dual pixel
      val (r, g, b) = expandRgb555(pixelData16)
      decodedR := r; decodedG := g; decodedB := b
      decodedRgbWrite := True; decodedAuxWrite := io.enableAlphaPlanes
    }
    is(4) { // XRGB8888 - single pixel
      val (r, g, b) = decodeArgb8888(pixelData32, io.rgbaLanes)
      decodedR := r; decodedG := g; decodedB := b
      decodedRgbWrite := True; decodedAuxWrite := io.enableAlphaPlanes
    }
    is(5) { // ARGB8888 - single pixel
      val (r, g, b) = decodeArgb8888(pixelData32, io.rgbaLanes)
      decodedR := r; decodedG := g; decodedB := b
      decodedRgbWrite := True; decodedAuxWrite := io.enableAlphaPlanes
    }
    is(12) { // Depth+RGB565 - single pixel (hi16=depth, lo16=RGB565)
      val (r, g, b) = expandRgb565(lo16)
      decodedR := r; decodedG := g; decodedB := b
      decodedDepth := hi16.asBits
      decodedRgbWrite := True; decodedAuxWrite := True
    }
    is(13) { // Depth+RGB555 - single pixel
      val (r, g, b) = expandRgb555(lo16)
      decodedR := r; decodedG := g; decodedB := b
      decodedDepth := hi16.asBits
      decodedRgbWrite := True; decodedAuxWrite := True
    }
    is(14) { // Depth+ARGB1555 - single pixel
      val (r, g, b) = expandRgb555(lo16)
      decodedR := r; decodedG := g; decodedB := b
      decodedDepth := hi16.asBits
      decodedRgbWrite := True; decodedAuxWrite := True
    }
    is(15) { // Depth-only - dual pixel
      decodedDepth := pixelData16.asBits
      decodedRgbWrite := False; decodedAuxWrite := True
    }
  }

  // Decode alpha for pipeline mode
  val decodedAlpha = UInt(8 bits)
  decodedAlpha := U(0xFF, 8 bits) // Default: fully opaque
  switch(io.writeFormat) {
    is(5) { // ARGB8888: extract alpha
      decodedAlpha := decodeAlpha8888(pixelData32, io.rgbaLanes)
    }
    is(2) { // ARGB1555 dual pixel: bit 15
      decodedAlpha := pixelData16(15) ? U(0xFF, 8 bits) | U(0, 8 bits)
    }
    is(14) { // Depth+ARGB1555: bit 15 of lo16
      decodedAlpha := lo16(15) ? U(0xFF, 8 bits) | U(0, 8 bits)
    }
  }

  // ========================================================================
  // Bypass mode: Dither and Write.Input
  // ========================================================================
  val dither = Dither()
  val currentX = (state === statePixel2) ? pixel2X | pixelX
  dither.io.r := decodedR
  dither.io.g := decodedG
  dither.io.b := decodedB
  dither.io.x := currentX.resize(2 bits)
  dither.io.y := pixelY.resize(2 bits)
  dither.io.enable := io.enableDithering
  dither.io.use2x2 := io.ditherAlgorithm

  // Build Write.Input (bypass mode)
  val writeInput = Write.Input(c)
  writeInput.coords(0) := (False ## currentX).asSInt.resize(c.vertexFormat.nonFraction bits)
  writeInput.coords(1) := (False ## pixelY).asSInt.resize(c.vertexFormat.nonFraction bits)
  writeInput.toFb.color.r := dither.io.ditR
  writeInput.toFb.color.g := dither.io.ditG
  writeInput.toFb.color.b := dither.io.ditB
  writeInput.toFb.depthAlpha := decodedDepth
  writeInput.rgbWrite := decodedRgbWrite
  writeInput.auxWrite := decodedAuxWrite
  writeInput.fbBaseAddr := io.fbWriteBaseAddr

  // ========================================================================
  // Pipeline mode: ColorCombine.Output
  // ========================================================================
  val pipelinePayload = ColorCombine.Output(c)
  pipelinePayload.coords(0) := (False ## currentX).asSInt.resize(c.vertexFormat.nonFraction bits)
  pipelinePayload.coords(1) := (False ## pixelY).asSInt.resize(c.vertexFormat.nonFraction bits)
  pipelinePayload.color.r := decodedR
  pipelinePayload.color.g := decodedG
  pipelinePayload.color.b := decodedB
  pipelinePayload.alpha := decodedAlpha
  // Depth: formats 12-14 have explicit hi16 depth, others use zaColor
  // Convert 16-bit depth to SQ(32,12) format: depth << 12
  // Use |<< for fixed-width shift (SpinalHDL << widens)
  val depthFor16 = decodedDepth.asUInt
  pipelinePayload.depth.raw := (depthFor16.resize(32 bits) |<< 12).asBits
  pipelinePayload.iteratedAlpha := decodedAlpha
  pipelinePayload.rawW := S(0, 32 bits) // LFB has no W data

  // Write pixel output valid signals — mux based on pixelPipelineEnable
  val writePixelActive = (state === statePixel1) || (state === statePixel2)

  io.writeOutput.valid := writePixelActive && !io.pixelPipelineEnable
  io.writeOutput.payload := writeInput

  io.pipelineOutput.valid := writePixelActive && io.pixelPipelineEnable
  io.pipelineOutput.payload := pipelinePayload

  // Active output ready: whichever mode is selected
  val activeReady = io.pixelPipelineEnable ? io.pipelineOutput.ready | io.writeOutput.ready

  // State transitions on active output fire
  val activeOutputFire = writePixelActive && activeReady
  when(activeOutputFire) {
    when(state === statePixel1) {
      when(isDualPixel) {
        state := statePixel2
      } otherwise {
        state := stateIdle
      }
    }
    when(state === statePixel2) {
      state := stateIdle
    }
  }

  // ========================================================================
  // Read path: FB memory reads
  // ========================================================================
  val fbReadCmdPending = Reg(Bool()) init (False) // True while waiting for cmd to be accepted
  val fbReadAddr = Reg(UInt(c.addressWidth.value bits))

  io.fbReadBus.cmd.valid := fbReadCmdPending
  io.fbReadBus.cmd.fragment.address := fbReadAddr
  io.fbReadBus.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
  io.fbReadBus.cmd.fragment.length := 3 // 4 bytes
  io.fbReadBus.cmd.fragment.source := 0
  io.fbReadBus.cmd.last := True

  io.fbReadBus.rsp.ready := (state === stateRfetch1) || (state === stateRfetch2)

  // On entering stateRfetch1 (from cmd.fire), set up address and issue first read
  when(io.bus.cmd.fire && io.bus.cmd.opcode === Bmb.Cmd.Opcode.READ) {
    // Compute first read address from the captured address (available on cmd cycle)
    val cmdAddr = io.bus.cmd.address
    val rx = (cmdAddr >> 1).resize(10 bits)
    val ry = (cmdAddr >> 11).resize(10 bits)
    val pixelFlat1 = (ry.resize(20 bits) * c.fbPixelStride + rx.resize(20 bits))
    fbReadAddr := (io.fbReadBaseAddr + (pixelFlat1 << 2).resize(c.addressWidth.value bits)).resized
    fbReadCmdPending := True
  }

  // Read state machine
  when(state === stateRfetch1) {
    when(io.fbReadBus.cmd.fire) {
      fbReadCmdPending := False
    }
    when(io.fbReadBus.rsp.fire) {
      capturedRead1 := io.fbReadBus.rsp.data
      state := stateRfetch2
      // Set up address for second read (x+1)
      val pixelFlat2 = (readPixelY.resize(20 bits) * c.fbPixelStride + (readPixelX + 1).resize(20 bits))
      fbReadAddr := (io.fbReadBaseAddr + (pixelFlat2 << 2).resize(c.addressWidth.value bits)).resized
      fbReadCmdPending := True
    }
  }

  when(state === stateRfetch2) {
    when(io.fbReadBus.cmd.fire) {
      fbReadCmdPending := False
    }
    when(io.fbReadBus.rsp.fire) {
      capturedRead2 := io.fbReadBus.rsp.data
      state := stateRresp
    }
  }

  // Build read response data
  // readBufferSelect: 0/1 → lo16 (RGB565), 2 → hi16 (depth/alpha)
  val readData1 = Bits(16 bits)
  val readData2 = Bits(16 bits)
  when(io.readBufferSelect === 2) {
    readData1 := capturedRead1(31 downto 16)
    readData2 := capturedRead2(31 downto 16)
  } otherwise {
    readData1 := capturedRead1(15 downto 0)
    readData2 := capturedRead2(15 downto 0)
  }

  // Pack: lo16 = pixel(x), hi16 = pixel(x+1)
  val readResponseRaw = readData2 ## readData1

  // Apply word swap and byte swizzle to read result
  val readAfterWordSwap = Bits(32 bits)
  when(io.wordSwapReads) {
    readAfterWordSwap := readResponseRaw(15 downto 0) ## readResponseRaw(31 downto 16)
  } otherwise {
    readAfterWordSwap := readResponseRaw
  }
  val readResponseData = Bits(32 bits)
  when(io.byteSwizzleReads) {
    readResponseData := readAfterWordSwap(7 downto 0) ## readAfterWordSwap(15 downto 8) ##
      readAfterWordSwap(23 downto 16) ## readAfterWordSwap(31 downto 24)
  } otherwise {
    readResponseData := readAfterWordSwap
  }

  // RRESP state: wait for bus response handshake
  when(state === stateRresp && io.bus.rsp.fire) {
    state := stateIdle
  }

  // ========================================================================
  // BMB response
  // ========================================================================
  val capturedSource = Reg(UInt(Lfb.bmbParams(c).access.sourceWidth bits))
  when(io.bus.cmd.fire) {
    capturedSource := io.bus.cmd.source
  }

  // For writes: respond immediately (rspPending set on cmd.fire)
  // For reads: respond only when data is ready (stateRresp)
  io.bus.rsp.valid := rspPending && (!capturedIsRead || state === stateRresp)
  io.bus.rsp.source := capturedSource
  io.bus.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  io.bus.rsp.last := True
  io.bus.rsp.data := capturedIsRead ? readResponseData | B(0, 32 bits)

  when(io.bus.rsp.fire) {
    rspPending := False
  }

  // Busy signal
  io.busy := (state =/= stateIdle) || rspPending
}

object Lfb {
  def bmbParams(c: Config) = BmbParameter(
    addressWidth = 22,
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )

  def fbReadBmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = false,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}
