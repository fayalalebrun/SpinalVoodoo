package voodoo.frontend

import voodoo._
import voodoo.core.FramebufferLayout
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
  * Dual-pixel formats (16-bit: 0-3, 15) produce two items per write. Single-pixel formats (32-bit:
  * 4-5, 12-14) produce one item per write.
  */
case class Lfb(c: Config) extends Component {
  val io = new Bundle {
    val bus = slave(Bmb(Lfb.bmbParams(c)))

    // Bypass mode output (pixelPipelineEnable=0)
    val writeOutput = master Stream (Write.PreDither(c))

    // Pipeline mode output (pixelPipelineEnable=1)
    val pipelineOutput = master Stream (ColorCombine.Output(c))

    val busy = out Bool ()

    val regs = in(Lfb.Regs(c))

    // Framebuffer read bus (for LFB reads)
    val fbReadBus = master(Bmb(Lfb.fbReadBmbParams(c)))

    // LFB reads stall until FIFO is empty and pipeline is idle (SST-1 spec)
    val pciFifoEmpty = in Bool ()
    val pipelineBusy = in Bool ()
  }

  def pixelCoords(x: UInt, y: UInt) = new Composite(x) {
    val coords = PixelCoords(c)
    coords.x := (False ## x).asSInt.resize(c.vertexFormat.nonFraction bits)
    coords.y := (False ## y).asSInt.resize(c.vertexFormat.nonFraction bits)
  }.coords

  def buildWritePreDither(x: UInt, y: UInt): Write.PreDither = {
    val out = Write.PreDither(c)
    out.r := decodedR
    out.g := decodedG
    out.b := decodedB
    out.coords := pixelCoords(x, y)
    out.enableDithering := io.regs.fbzMode.enableDithering
    out.ditherAlgorithm := io.regs.fbzMode.ditherAlgorithm
    out.depthAlpha := decodedDepth
    out.rgbWrite := decodedRgbWrite
    out.auxWrite := decodedAuxWrite
    out.routing := io.regs.writeRouting
    if (c.trace.enabled) out.trace := Trace.originPixelKey(Trace.Origin.lfb)
    out
  }

  def buildPipelineOutput(x: UInt, y: UInt): ColorCombine.Output = {
    val out = ColorCombine.Output(c)
    out.coords := pixelCoords(x, y)
    out.color.r := decodedR
    out.color.g := decodedG
    out.color.b := decodedB
    out.alpha := decodedAlpha
    out.depth.raw := (decodedDepth.asUInt.resize(32 bits) |<< 12).asBits
    out.iteratedAlpha := decodedAlpha
    out.rawW := S(0, 32 bits)
    out.fogColor := io.regs.fogColor
    out.chromaKey := B(0, 32 bits)
    out.zaColor := io.regs.zaColor
    out.alphaMode := io.regs.alphaMode
    out.fogMode := io.regs.fogMode
    out.fbzMode := io.regs.fbzMode
    out.routing := io.regs.writeRouting
    if (c.trace.enabled) out.trace := Trace.originPixelKey(Trace.Origin.lfb)
    out
  }

  // Capture BMB command data
  val capturedAddr = Reg(UInt(22 bits))
  val capturedData = Reg(Bits(32 bits))
  val capturedMask = Reg(Bits(4 bits)) init (0)
  val capturedIsRead = Reg(Bool()) init (False)
  val rspPending = Reg(Bool()) init (False)

  // Captured FB read results for read path
  val capturedRead1 = Reg(Bits(32 bits))
  val capturedRead2 = Reg(Bits(32 bits))
  val capturedRead1LaneHi = Reg(Bool()) init (False)
  val capturedRead2LaneHi = Reg(Bool()) init (False)
  val fbReadPlaneAddr = Reg(UInt(c.addressWidth.value bits)) init (0)

  // Is this a dual-pixel format?
  val isDualPixel =
    io.regs.lfbMode.writeFormat === 0 || io.regs.lfbMode.writeFormat === 1 || io.regs.lfbMode.writeFormat === 2 ||
      io.regs.lfbMode.writeFormat === 3 || io.regs.lfbMode.writeFormat === 15

  // State machine: 3 bits to hold 6 states
  val state = RegInit(U(0, 3 bits))
  val stateIdle = U(0, 3 bits)
  val statePixel1 = U(1, 3 bits)
  val statePixel2 = U(2, 3 bits)
  val stateRfetch1 = U(3, 3 bits)
  val stateRfetch2 = U(4, 3 bits)
  val stateRresp = U(5, 3 bits)
  val stateRwait = U(6, 3 bits)

  // Accept BMB commands only in IDLE and when no pending response
  io.bus.cmd.ready := (state === stateIdle) && !rspPending

  // Apply word swap and byte swizzle to incoming write data
  val afterByteSwizzle =
    wordSwapAndSwizzle(
      io.bus.cmd.data,
      io.regs.lfbMode.wordSwapWrites,
      io.regs.lfbMode.byteSwizzleWrites
    )

  // Capture on cmd.fire
  when(io.bus.cmd.fire) {
    capturedAddr := io.bus.cmd.address
    rspPending := True
    when(io.bus.cmd.opcode === Bmb.Cmd.Opcode.READ) {
      capturedIsRead := True
      state := stateRwait
    } otherwise {
      capturedIsRead := False
      capturedData := afterByteSwizzle
      capturedMask := io.bus.cmd.mask
      state := statePixel1
    }
  }

  // Address decode: depends on format stride (for writes)
  val is32bitStride = io.regs.lfbMode.writeFormat === 4 || io.regs.lfbMode.writeFormat === 5
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
  val isHalfWordWrite = capturedMask === B"4'b0011" || capturedMask === B"4'b1100"
  val useHiHalfForPixel1 = capturedMask === B"4'b1100"

  val pixelData16 = UInt(16 bits)
  pixelData16 := (state === statePixel2 || useHiHalfForPixel1) ? hi16 | lo16

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
  decodedDepth := io.regs.zaColor(15 downto 0) // Default depth from zaColor when no explicit depth
  decodedRgbWrite := True
  decodedAuxWrite := False

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
      is(2) { a := data(7 downto 0).asUInt } // RGBA: A=[7:0]
      is(3) { a := data(7 downto 0).asUInt } // BGRA: A=[7:0]
    }
    a
  }

  // Format decode logic
  switch(io.regs.lfbMode.writeFormat) {
    is(0) { // RGB565 - dual pixel
      val rgb = expandRgb565(pixelData16)
      decodedR := rgb.r; decodedG := rgb.g; decodedB := rgb.b
      decodedRgbWrite := True; decodedAuxWrite := io.regs.fbzMode.enableAlphaPlanes
    }
    is(1) { // RGB555 - dual pixel
      val rgb = expandRgb555(pixelData16)
      decodedR := rgb.r; decodedG := rgb.g; decodedB := rgb.b
      decodedRgbWrite := True; decodedAuxWrite := io.regs.fbzMode.enableAlphaPlanes
    }
    is(2) { // ARGB1555 - dual pixel
      val rgb = expandRgb555(pixelData16)
      decodedR := rgb.r; decodedG := rgb.g; decodedB := rgb.b
      decodedRgbWrite := True; decodedAuxWrite := io.regs.fbzMode.enableAlphaPlanes
    }
    is(4) { // XRGB8888 - single pixel
      val (r, g, b) = decodeArgb8888(pixelData32, io.regs.lfbMode.rgbaLanes)
      decodedR := r; decodedG := g; decodedB := b
      decodedRgbWrite := True; decodedAuxWrite := io.regs.fbzMode.enableAlphaPlanes
    }
    is(5) { // ARGB8888 - single pixel
      val (r, g, b) = decodeArgb8888(pixelData32, io.regs.lfbMode.rgbaLanes)
      decodedR := r; decodedG := g; decodedB := b
      decodedRgbWrite := True; decodedAuxWrite := io.regs.fbzMode.enableAlphaPlanes
    }
    is(12) { // Depth+RGB565 - single pixel (hi16=depth, lo16=RGB565)
      val rgb = expandRgb565(lo16)
      decodedR := rgb.r; decodedG := rgb.g; decodedB := rgb.b
      decodedDepth := hi16.asBits
      decodedRgbWrite := True; decodedAuxWrite := True
    }
    is(13) { // Depth+RGB555 - single pixel
      val rgb = expandRgb555(lo16)
      decodedR := rgb.r; decodedG := rgb.g; decodedB := rgb.b
      decodedDepth := hi16.asBits
      decodedRgbWrite := True; decodedAuxWrite := True
    }
    is(14) { // Depth+ARGB1555 - single pixel
      val rgb = expandRgb555(lo16)
      decodedR := rgb.r; decodedG := rgb.g; decodedB := rgb.b
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
  decodedAlpha := U(0xff, 8 bits) // Default: fully opaque
  switch(io.regs.lfbMode.writeFormat) {
    is(5) { // ARGB8888: extract alpha
      decodedAlpha := decodeAlpha8888(pixelData32, io.regs.lfbMode.rgbaLanes)
    }
    is(2) { // ARGB1555 dual pixel: bit 15
      decodedAlpha := pixelData16(15) ? U(0xff, 8 bits) | U(0, 8 bits)
    }
    is(14) { // Depth+ARGB1555: bit 15 of lo16
      decodedAlpha := lo16(15) ? U(0xff, 8 bits) | U(0, 8 bits)
    }
  }

  // ========================================================================
  // Bypass mode: output Write.PreDither (dithering done by shared instance in Core)
  // ========================================================================
  val writePixelActive = (state === statePixel1) || (state === statePixel2)
  val pixel1X = (isHalfWordWrite && useHiHalfForPixel1) ? pixel2X | pixelX
  val currentX = (state === statePixel2) ? pixel2X | pixel1X

  io.writeOutput.valid := writePixelActive && !io.regs.lfbMode.pixelPipelineEnable
  io.writeOutput.payload := buildWritePreDither(currentX, pixelY)

  // ========================================================================
  // Pipeline mode: ColorCombine.Output (no dither, no delay)
  // ========================================================================
  io.pipelineOutput.valid := writePixelActive && io.regs.lfbMode.pixelPipelineEnable
  io.pipelineOutput.payload := buildPipelineOutput(currentX, pixelY)

  // Active output fire: bypass fires via downstream Stream handshake, pipeline fires immediately
  val bypassFire = io.writeOutput.fire
  val pipelineFire =
    writePixelActive && io.regs.lfbMode.pixelPipelineEnable && io.pipelineOutput.ready
  val activeOutputFire = bypassFire || pipelineFire

  // State transitions on active output fire
  when(activeOutputFire) {
    when(state === statePixel1) {
      when(isDualPixel && !isHalfWordWrite) {
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
  io.fbReadBus.cmd.fragment.data := 0
  io.fbReadBus.cmd.fragment.mask := 0
  io.fbReadBus.cmd.last := True

  io.fbReadBus.rsp.ready := (state === stateRfetch1) || (state === stateRfetch2)

  // On accepting a read command, compute the first FB address but don't issue yet.
  // The read will wait in stateRwait until the FIFO drains and pipeline is idle.
  when(io.bus.cmd.fire && io.bus.cmd.opcode === Bmb.Cmd.Opcode.READ) {
    val cmdAddr = io.bus.cmd.address
    val rx = (cmdAddr >> 1).resize(10 bits)
    val ry = (cmdAddr >> 11).resize(10 bits)
    val readBase =
      (io.regs.lfbMode.readBufferSelect === 2) ? io.regs.readRouting.auxBaseAddr | io.regs.readRouting.colorBaseAddr
    val planeAddr1 =
      FramebufferAddressMath.planeAddress(readBase, rx, ry, io.regs.readRouting.pixelStride)
    capturedRead1LaneHi := planeAddr1(1)
    fbReadPlaneAddr := planeAddr1
    fbReadAddr := (planeAddr1(c.addressWidth.value - 1 downto 2) ## U"2'b00").asUInt
  }

  // Wait for FIFO empty + pipeline idle before issuing FB reads (SST-1 spec)
  when(state === stateRwait) {
    when(io.pciFifoEmpty && !io.pipelineBusy) {
      fbReadCmdPending := True
      state := stateRfetch1
    }
  }

  // Read state machine
  when(state === stateRfetch1) {
    when(io.fbReadBus.cmd.fire) {
      fbReadCmdPending := False
    }
    when(io.fbReadBus.rsp.fire) {
      capturedRead1 := io.fbReadBus.rsp.data
      state := stateRfetch2
      val planeAddr2 = (fbReadPlaneAddr + 2).resize(c.addressWidth.value bits)
      capturedRead2LaneHi := planeAddr2(1)
      fbReadPlaneAddr := planeAddr2
      fbReadAddr := (planeAddr2(c.addressWidth.value - 1 downto 2) ## U"2'b00").asUInt
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

  // Build read response data from selected plane (always low 16 bits)
  val readData1 = capturedRead1LaneHi ? capturedRead1(31 downto 16) | capturedRead1(15 downto 0)
  val readData2 = capturedRead2LaneHi ? capturedRead2(31 downto 16) | capturedRead2(15 downto 0)

  // Pack: lo16 = pixel(x), hi16 = pixel(x+1)
  val readResponseRaw = readData2 ## readData1

  // Apply word swap and byte swizzle to read result
  val readResponseData =
    wordSwapAndSwizzle(
      readResponseRaw,
      io.regs.lfbMode.wordSwapReads,
      io.regs.lfbMode.byteSwizzleReads
    )

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

  // Busy signal: only write-side activity contributes to pipeline busy.
  // Read states are excluded to avoid circular dependency (pipelineBusy includes lfb.io.busy,
  // and stateRwait needs pipelineBusy to go false before it can proceed).
  io.busy := (state === statePixel1) || (state === statePixel2)
}

object Lfb {
  case class Regs(c: Config) extends Bundle {
    val lfbMode = LfbMode()
    val fbzMode = FbzMode()
    val alphaMode = AlphaMode()
    val fogMode = FogMode()
    val fogColor = Bits(32 bits)
    val zaColor = Bits(32 bits)
    val writeRouting = FbRouting(c)
    val readRouting = FbRouting(c)
  }

  object Regs {
    def fromRegisterBank(c: Config, regBank: RegisterBank, layout: FramebufferLayout): Regs = {
      val regs = Regs(c)
      regs.lfbMode := regBank.renderConfig.lfbModeBundle
      regs.fbzMode := regBank.renderConfig.fbzModeBundle
      regs.alphaMode := regBank.renderConfig.alphaModeBundle
      regs.fogMode := regBank.renderConfig.fogModeBundle
      regs.fogColor := regBank.renderConfig.fogColor
      regs.zaColor := regBank.renderConfig.zaColor
      regs.writeRouting := layout.lfbWrite
      regs.readRouting := layout.lfbRead
      regs
    }
  }

  def fbReadBmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

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

}
