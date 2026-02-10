package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._

/** Linear Frame Buffer (LFB) write support.
  *
  * Accepts CPU writes to the LFB address space and converts them into Write.Input stream items.
  * Supports all Voodoo1 LFB write formats in bypass mode (pixelPipelineEnable=0).
  *
  * Dual-pixel formats (16-bit: 0-3, 15) produce two Write.Input items per write.
  * Single-pixel formats (32-bit: 4-5, 12-14) produce one item per write.
  */
case class Lfb(c: Config) extends Component {
  val io = new Bundle {
    val bus = slave(Bmb(Lfb.bmbParams(c)))
    val writeOutput = master Stream (Write.Input(c))
    val busy = out Bool ()

    // Config inputs from RegisterBank
    val writeFormat = in UInt (4 bits)
    val rgbaLanes = in UInt (2 bits)
    val wordSwapWrites = in Bool ()
    val byteSwizzleWrites = in Bool ()
    val enableDithering = in Bool ()
    val ditherAlgorithm = in Bool ()
    val zaColor = in Bits (32 bits)
    val enableAlphaPlanes = in Bool ()
  }

  // Capture BMB command data
  val capturedAddr = Reg(UInt(22 bits))
  val capturedData = Reg(Bits(32 bits))
  val rspPending = Reg(Bool()) init (False)

  // Is this a dual-pixel format?
  val isDualPixel = io.writeFormat === 0 || io.writeFormat === 1 || io.writeFormat === 2 ||
    io.writeFormat === 3 || io.writeFormat === 15

  // State machine: 0=IDLE, 1=PIXEL1, 2=PIXEL2
  val state = RegInit(U(0, 2 bits))
  val stateIdle = U(0, 2 bits)
  val statePixel1 = U(1, 2 bits)
  val statePixel2 = U(2, 2 bits)

  // Accept BMB commands only in IDLE
  io.bus.cmd.ready := (state === stateIdle)

  // Apply word swap and byte swizzle to incoming data
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
    capturedData := afterByteSwizzle
    rspPending := True
    state := statePixel1
  }

  // Address decode: depends on format stride
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
    // Default: ARGB (lanes=0): bits [23:16]=R, [15:8]=G, [7:0]=B, [31:24]=A
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
    is(2) { // ARGB1555 - dual pixel (alpha bit ignored in bypass mode)
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

  // Dither: convert 8-bit RGB to RGB565
  val dither = Dither()
  val currentX = (state === statePixel2) ? pixel2X | pixelX
  dither.io.r := decodedR
  dither.io.g := decodedG
  dither.io.b := decodedB
  dither.io.x := currentX.resize(2 bits)
  dither.io.y := pixelY.resize(2 bits)
  dither.io.enable := io.enableDithering
  dither.io.use2x2 := io.ditherAlgorithm

  // Build Write.Input
  val writeInput = Write.Input(c)
  writeInput.coords(0) := (False ## currentX).asSInt.resize(c.vertexFormat.nonFraction bits)
  writeInput.coords(1) := (False ## pixelY).asSInt.resize(c.vertexFormat.nonFraction bits)
  writeInput.toFb.color.r := dither.io.ditR
  writeInput.toFb.color.g := dither.io.ditG
  writeInput.toFb.color.b := dither.io.ditB
  writeInput.toFb.depthAlpha := decodedDepth
  writeInput.rgbWrite := decodedRgbWrite
  writeInput.auxWrite := decodedAuxWrite

  // Output stream: valid in PIXEL1 and PIXEL2 states
  io.writeOutput.valid := (state === statePixel1) || (state === statePixel2)
  io.writeOutput.payload := writeInput

  // State transitions on output.fire
  when(io.writeOutput.fire) {
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

  // BMB response: need to capture source from cmd since cmd may not be valid when rsp fires
  val capturedSource = Reg(UInt(Lfb.bmbParams(c).access.sourceWidth bits))
  when(io.bus.cmd.fire) {
    capturedSource := io.bus.cmd.source
  }

  io.bus.rsp.valid := rspPending
  io.bus.rsp.source := capturedSource
  io.bus.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  io.bus.rsp.last := True

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
    canRead = false,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )
}
