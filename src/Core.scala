package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo.utils.{PciFifo, TexWritePayload}

object Core {
  val cpuBmbParams = BmbParameter(
    addressWidth = 24,
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )

  def fbMemBmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1 + log2Up(5), // 5 arbiter inputs
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  def texMemBmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 4 + log2Up(3), // max(srcW=1,4) + 2 route bits for 3 inputs = 6
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  // Internal CPU texture write bus params
  val cpuTexBmbParams = BmbParameter(
    addressWidth = 26,
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 6,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}

case class Core(c: Config) extends Component {
  val io = new Bundle {
    // Unified CPU bus (24-bit address covers 16MB PCI BAR)
    val cpuBus = slave(Bmb(Core.cpuBmbParams))

    // Framebuffer memory bus (R/W, internal 5-port arbitration)
    val fbMem = master(Bmb(Core.fbMemBmbParams(c)))

    // Texture memory bus (R/W, internal 2-port arbitration with texBaseAddr relocation)
    val texMem = master(Bmb(Core.texMemBmbParams(c)))

    // Status inputs (hardware state)
    val statusInputs = in(new Bundle {
      val vRetrace = Bool()
      val memFifoFree = UInt(16 bits)
      val pciInterrupt = Bool()
    })

    // SwapBuffer outputs (active buffer index and pending swap count)
    val swapDisplayedBuffer = out UInt (2 bits)
    val swapsPending = out UInt (3 bits)

    // Framebuffer base address
    val fbBaseAddr = in UInt (c.addressWidth)

    // Simulation-only framebuffer cache flush request
    val flushFbCaches = in Bool ()

  }

  // Instantiate components
  val addressRemapper =
    AddressRemapper(RegisterBank.externalBmbParams(c), RegisterBank.bmbParams(c))
  val regBank = RegisterBank(c)

  // PciFifo: extracted FIFO component between AddressRemapper and RegisterBank
  val pciFifo = PciFifo(
    busParams = RegisterBank.bmbParams(c),
    categories = regBank.busif.getCategories,
    floatAliases = regBank.busif.getFloatAliases,
    commandAddresses = regBank.busif.getCommandStreamReady.keys.toSeq.sorted
  )

  val triangleSetup = TriangleSetup(c)
  val rasterizer = Rasterizer(c)
  val tmu = Tmu(c) // Single TMU (Voodoo 1 level)
  val colorCombine = ColorCombine(c)
  val writeColor = Write(c)
  val writeAux = Write(c)

  val floatShadowStartS = Reg(SInt(60 bits)) init (0)
  val floatShadowStartT = Reg(SInt(60 bits)) init (0)
  val floatShadowStartW = Reg(SInt(60 bits)) init (0)
  val floatShadowDSdX = Reg(SInt(60 bits)) init (0)
  val floatShadowDTdX = Reg(SInt(60 bits)) init (0)
  val floatShadowDWdX = Reg(SInt(60 bits)) init (0)
  val floatShadowDSdY = Reg(SInt(60 bits)) init (0)
  val floatShadowDTdY = Reg(SInt(60 bits)) init (0)
  val floatShadowDWdY = Reg(SInt(60 bits)) init (0)
  val floatShadowStartA = Reg(SInt(60 bits)) init (0)
  val floatShadowDAdX = Reg(SInt(60 bits)) init (0)
  val floatShadowDAdY = Reg(SInt(60 bits)) init (0)

  // Make triangle command streams accessible for simulation monitoring
  regBank.commands.triangleCmd.simPublic()
  regBank.commands.ftriangleCmd.simPublic()

  // ========================================================================
  // CPU bus address decode
  // ========================================================================
  // Decode cpuBus address bits [23:22]:
  //   00 = registers (0x000000-0x3FFFFF)
  //   01 = LFB       (0x400000-0x7FFFFF)
  //   10 = texture    (0x800000-0xBFFFFF)
  //   11 = texture    (0xC00000-0xFFFFFF)

  val cpuCmd = io.cpuBus.cmd
  val cpuRsp = io.cpuBus.rsp

  val target = cpuCmd.address(23 downto 22)

  // Register which target was selected (latched on cmd.fire for response routing)
  val rspTarget = RegNextWhen(target, cpuCmd.fire) init (0)

  val isReg = target === 0
  val isLfb = target === 1
  val isTex = target >= 2
  val isTexWrite = isTex && cpuCmd.opcode(0)
  val isTexRead = isTex && !cpuCmd.opcode(0)

  // Internal register bus (22-bit address, includes bit 21 for remap detection)
  // Strip Voodoo chip-select bits [13:10] from PCI register address.
  // 86Box uses (addr >> 10) & 0xF as chip select (FBI=1, TREX0=2, TREX1=4).
  // Our unified RegisterBank handles all chips, so only the register offset matters.
  val internalRegBus = Bmb(RegisterBank.externalBmbParams(c))
  internalRegBus.cmd.valid := cpuCmd.valid && isReg
  internalRegBus.cmd.opcode := cpuCmd.opcode
  internalRegBus.cmd.address := (cpuCmd.address(21 downto 14) ## U"4'0" ## cpuCmd.address(
    9 downto 0
  )).asUInt
  internalRegBus.cmd.data := cpuCmd.data
  internalRegBus.cmd.mask := cpuCmd.mask
  internalRegBus.cmd.length := cpuCmd.length
  internalRegBus.cmd.last := cpuCmd.last
  internalRegBus.cmd.source := cpuCmd.source.resize(
    RegisterBank.externalBmbParams(c).access.sourceWidth bits
  )

  // Internal LFB bus (22-bit address)
  val internalLfbBus = Bmb(Lfb.bmbParams(c))
  internalLfbBus.cmd.valid := cpuCmd.valid && isLfb
  internalLfbBus.cmd.opcode := cpuCmd.opcode
  internalLfbBus.cmd.address := cpuCmd.address.resize(22 bits)
  internalLfbBus.cmd.data := cpuCmd.data
  internalLfbBus.cmd.mask := cpuCmd.mask
  internalLfbBus.cmd.length := cpuCmd.length
  internalLfbBus.cmd.last := cpuCmd.last
  internalLfbBus.cmd.source := cpuCmd.source.resize(Lfb.bmbParams(c).access.sourceWidth bits)

  // ========================================================================
  // Texture write path: through PciFifo
  // ========================================================================
  // Texture writes go into PciFifo alongside register writes
  pciFifo.io.texWrite.valid := cpuCmd.valid && isTexWrite
  pciFifo.io.texWrite.pciAddr := cpuCmd.address(22 downto 0)
  pciFifo.io.texWrite.data := cpuCmd.data
  pciFifo.io.texWrite.mask := cpuCmd.mask

  // CPU texture read bus (direct to SRAM, no FIFO needed)
  // For reads, use flat addressing with texBaseAddr from post-FIFO register bank
  val cpuTexBus = Bmb(Core.cpuTexBmbParams)
  val texReadPciOffset = cpuCmd.address(22 downto 0)
  val texReadBaseAddr = regBank.tmuConfig.texBaseAddr(18 downto 0)
  val texReadSramAddr = ((texReadBaseAddr << 3) +^ texReadPciOffset).resize(26 bits)
  cpuTexBus.cmd.valid := cpuCmd.valid && isTexRead
  cpuTexBus.cmd.address := texReadSramAddr
  cpuTexBus.cmd.opcode := cpuCmd.opcode
  cpuTexBus.cmd.data := cpuCmd.data
  cpuTexBus.cmd.mask := cpuCmd.mask
  cpuTexBus.cmd.length := cpuCmd.length.resize(Core.cpuTexBmbParams.access.lengthWidth bits)
  cpuTexBus.cmd.last := cpuCmd.last
  cpuTexBus.cmd.source := cpuCmd.source

  // Texture write response: generate immediate response when FIFO accepts
  val texWriteRspPending = RegInit(False)
  val texWriteRspSource = Reg(cpuCmd.source)
  when(cpuCmd.valid && isTexWrite && pciFifo.io.texWrite.ready && cpuCmd.ready) {
    texWriteRspPending := True
    texWriteRspSource := cpuCmd.source
  }
  tmu.io.invalidate := cpuCmd.valid && isTexWrite && pciFifo.io.texWrite.ready && cpuCmd.ready
  when(texWriteRspPending) {
    texWriteRspPending := False
  }

  // Mux ready back from selected target
  cpuCmd.ready := (isReg && internalRegBus.cmd.ready) ||
    (isLfb && internalLfbBus.cmd.ready) ||
    (isTexWrite && pciFifo.io.texWrite.ready) ||
    (isTexRead && cpuTexBus.cmd.ready)

  // Response mux: route response from selected target
  cpuRsp.valid := False
  cpuRsp.data := 0
  cpuRsp.last := True
  cpuRsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  cpuRsp.source := 0

  // Always accept responses from sub-buses
  internalRegBus.rsp.ready := (rspTarget === 0) && cpuRsp.ready
  internalLfbBus.rsp.ready := (rspTarget === 1) && cpuRsp.ready
  cpuTexBus.rsp.ready := (rspTarget >= 2) && cpuRsp.ready

  when(texWriteRspPending) {
    cpuRsp.valid := True
    cpuRsp.source := texWriteRspSource.resize(cpuRsp.p.access.sourceWidth bits)
  } elsewhen (rspTarget === 0) {
    cpuRsp.valid := internalRegBus.rsp.valid
    cpuRsp.data := internalRegBus.rsp.data
    cpuRsp.opcode := internalRegBus.rsp.opcode
    cpuRsp.source := internalRegBus.rsp.source.resize(cpuRsp.p.access.sourceWidth bits)
  } elsewhen (rspTarget === 1) {
    cpuRsp.valid := internalLfbBus.rsp.valid
    cpuRsp.data := internalLfbBus.rsp.data
    cpuRsp.opcode := internalLfbBus.rsp.opcode
    cpuRsp.source := internalLfbBus.rsp.source.resize(cpuRsp.p.access.sourceWidth bits)
  } otherwise {
    cpuRsp.valid := cpuTexBus.rsp.valid
    cpuRsp.data := cpuTexBus.rsp.data
    cpuRsp.opcode := cpuTexBus.rsp.opcode
    cpuRsp.source := cpuTexBus.rsp.source.resize(cpuRsp.p.access.sourceWidth bits)
  }

  // ========================================================================
  // Wire PciFifo into the bus path
  // ========================================================================
  // Before: addressRemapper.io.output <> regBank.io.bus
  // After:  addressRemapper -> PciFifo -> RegisterBank
  addressRemapper.io.input <> internalRegBus
  addressRemapper.io.output <> pciFifo.io.cpuSide
  pciFifo.io.regSide <> regBank.io.bus

  when(pciFifo.io.floatShadow.valid) {
    switch(pciFifo.io.floatShadow.address) {
      is(U(0x030, 12 bits)) { floatShadowStartA := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x034, 12 bits)) { floatShadowStartS := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x038, 12 bits)) { floatShadowStartT := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x03c, 12 bits)) { floatShadowStartW := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x050, 12 bits)) { floatShadowDAdX := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x054, 12 bits)) { floatShadowDSdX := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x058, 12 bits)) { floatShadowDTdX := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x05c, 12 bits)) { floatShadowDWdX := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x070, 12 bits)) { floatShadowDAdY := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x074, 12 bits)) { floatShadowDSdY := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x078, 12 bits)) { floatShadowDTdY := pciFifo.io.floatShadow.raw.resize(60 bits) }
      is(U(0x07c, 12 bits)) { floatShadowDWdY := pciFifo.io.floatShadow.raw.resize(60 bits) }
    }
  }.elsewhen(regBank.io.bus.cmd.fire && regBank.io.bus.cmd.opcode === Bmb.Cmd.Opcode.WRITE) {
    switch(regBank.io.bus.cmd.address) {
      is(U(0x030, 12 bits)) {
        floatShadowStartA := (regBank.io.bus.cmd.data.asSInt.resize(60 bits) |<< 18)
      }
      is(U(0x034, 12 bits)) {
        floatShadowStartS := (regBank.io.bus.cmd.data.asSInt.resize(60 bits) |<< 12)
      }
      is(U(0x038, 12 bits)) {
        floatShadowStartT := (regBank.io.bus.cmd.data.asSInt.resize(60 bits) |<< 12)
      }
      is(U(0x03c, 12 bits)) { floatShadowStartW := regBank.io.bus.cmd.data.asSInt.resize(60 bits) }
      is(U(0x050, 12 bits)) {
        floatShadowDAdX := (regBank.io.bus.cmd.data.asSInt.resize(60 bits) |<< 18)
      }
      is(U(0x054, 12 bits)) {
        floatShadowDSdX := (regBank.io.bus.cmd.data.asSInt.resize(60 bits) |<< 12)
      }
      is(U(0x058, 12 bits)) {
        floatShadowDTdX := (regBank.io.bus.cmd.data.asSInt.resize(60 bits) |<< 12)
      }
      is(U(0x05c, 12 bits)) { floatShadowDWdX := regBank.io.bus.cmd.data.asSInt.resize(60 bits) }
      is(U(0x070, 12 bits)) {
        floatShadowDAdY := (regBank.io.bus.cmd.data.asSInt.resize(60 bits) |<< 18)
      }
      is(U(0x074, 12 bits)) {
        floatShadowDSdY := (regBank.io.bus.cmd.data.asSInt.resize(60 bits) |<< 12)
      }
      is(U(0x078, 12 bits)) {
        floatShadowDTdY := (regBank.io.bus.cmd.data.asSInt.resize(60 bits) |<< 12)
      }
      is(U(0x07c, 12 bits)) { floatShadowDWdY := regBank.io.bus.cmd.data.asSInt.resize(60 bits) }
    }
  }
  regBank.io.statusInputs <> io.statusInputs

  // Pipeline busy signal: assigned after all pipeline stages are instantiated (see below)

  // TODO: Wire unused command streams to always ready
  regBank.commands.nopCmd.ready := True

  // ========================================================================
  // SwapBuffer command handler
  // ========================================================================
  val swapBuffer = SwapBuffer()
  swapBuffer.io.cmd << regBank.commands.swapbufferCmd
  swapBuffer.io.vRetrace := io.statusInputs.vRetrace
  swapBuffer.io.vsyncEnable := regBank.commands.swapVsyncEnable
  swapBuffer.io.swapInterval := regBank.commands.swapInterval
  swapBuffer.io.swapCmdEnqueued := pciFifo.io.wasEnqueued

  regBank.io.swapDisplayedBuffer := swapBuffer.io.swapCount(0).asUInt.resize(2 bits)
  regBank.io.swapsPending := swapBuffer.io.swapsPending
  io.swapDisplayedBuffer := swapBuffer.io.swapCount(0).asUInt.resize(2 bits)
  io.swapsPending := swapBuffer.io.swapsPending

  // ========================================================================
  // Framebuffer pixel stride (runtime, from fbiInit1[7:4])
  // ========================================================================
  // fbiInit1[7:4] = video tiles in X / 2; row_width_bytes = val * 128
  // stride_pixels = row_width_bytes / 2 = val * 64
  val fbPixelStride = (regBank.init.fbiInit1_videoTilesX << 6).resize(11 bits)

  // ========================================================================
  // Draw buffer selection (double-buffering)
  // ========================================================================
  // fbiInit2[20:11] = buffer offset in 4KB units
  // Shift by 12 to convert 4KB units to bytes
  val bufferOffsetBytes =
    regBank.init.fbiInit2_bufferOffset.resize(c.addressWidth.value bits) |<< 12
  val buffer0Base = io.fbBaseAddr
  val buffer1Base = io.fbBaseAddr + bufferOffsetBytes
  val auxBufferBase = io.fbBaseAddr + (bufferOffsetBytes << 1).resized
  val frontBufferBase = swapBuffer.io.swapCount(0) ? buffer1Base | buffer0Base
  val backBufferBase = swapBuffer.io.swapCount(0) ? buffer0Base | buffer1Base

  // Triangle/fastfill draw target: fbzMode[15:14] drawBuffer (0=front, 1=back)
  val drawColorBufferBase =
    regBank.renderConfig.fbzMode.drawBuffer(0) ? backBufferBase | frontBufferBase
  val drawAuxBufferBase = auxBufferBase

  regBank.io.triangleCapture.floatShadowStartS := floatShadowStartS
  regBank.io.triangleCapture.floatShadowStartT := floatShadowStartT
  regBank.io.triangleCapture.floatShadowStartW := floatShadowStartW
  regBank.io.triangleCapture.floatShadowDSdX := floatShadowDSdX
  regBank.io.triangleCapture.floatShadowDTdX := floatShadowDTdX
  regBank.io.triangleCapture.floatShadowDWdX := floatShadowDWdX
  regBank.io.triangleCapture.floatShadowDSdY := floatShadowDSdY
  regBank.io.triangleCapture.floatShadowDTdY := floatShadowDTdY
  regBank.io.triangleCapture.floatShadowDWdY := floatShadowDWdY
  regBank.io.triangleCapture.floatShadowStartA := floatShadowStartA
  regBank.io.triangleCapture.floatShadowDAdX := floatShadowDAdX
  regBank.io.triangleCapture.floatShadowDAdY := floatShadowDAdY
  regBank.io.triangleCapture.drawColorBufferBase := drawColorBufferBase
  regBank.io.triangleCapture.drawAuxBufferBase := drawAuxBufferBase
  regBank.io.triangleCapture.fbPixelStride := fbPixelStride

  // LFB color buffer bases: lfbMode[5:4] writeBufferSelect, lfbMode[7:6] readBufferSelect
  val lfbWriteColorBufferBase =
    regBank.renderConfig.lfbMode.writeBufferSelect(0) ? backBufferBase | frontBufferBase
  val lfbReadColorBufferBase =
    regBank.renderConfig.lfbMode.readBufferSelect(0) ? backBufferBase | frontBufferBase
  val lfbWriteAuxBufferBase = auxBufferBase
  val lfbReadAuxBufferBase = auxBufferBase

  // ========================================================================
  // Triangle Command Handling
  // ========================================================================
  // Both triangleCMD and FtriangleCMD read from the same integer registers.
  // Float addresses (0x088-0x0FC) are converted to fixed-point at FIFO input time
  // by PciFifo, so both commands see identical fixed-point data.

  // Data-driven gradient capture: zips (start, dX, dY) Bits triplets with GradientBundle.all
  def captureGradientsFrom(
      sources: Seq[(Bits, Bits, Bits)]
  ): Rasterizer.GradientBundle[Rasterizer.InputGradient] = {
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)
    grads.all.zip(sources).foreach { case (grad, (start, dx, dy)) =>
      grad.start.raw := start.asSInt.resize(grad.start.raw.getWidth bits).asBits
      grad.d(0).raw := dx.asSInt.resize(grad.d(0).raw.getWidth bits).asBits
      grad.d(1).raw := dy.asSInt.resize(grad.d(1).raw.getWidth bits).asBits
    }
    grads
  }

  def captureHiTexCoords(useFloatShadow: Bool): TriangleSetup.HiTexCoords = {
    val hi = TriangleSetup.HiTexCoords(c)
    val startSInt = regBank.triangleGeometry.startS.asSInt.resize(60 bits) |<< 12
    val startTInt = regBank.triangleGeometry.startT.asSInt.resize(60 bits) |<< 12
    val dSdXInt = regBank.triangleGeometry.dSdX.asSInt.resize(60 bits) |<< 12
    val dTdXInt = regBank.triangleGeometry.dTdX.asSInt.resize(60 bits) |<< 12
    val dSdYInt = regBank.triangleGeometry.dSdY.asSInt.resize(60 bits) |<< 12
    val dTdYInt = regBank.triangleGeometry.dTdY.asSInt.resize(60 bits) |<< 12

    hi.sStart.raw := Mux(useFloatShadow, floatShadowStartS.asBits, startSInt.asBits)
    hi.tStart.raw := Mux(useFloatShadow, floatShadowStartT.asBits, startTInt.asBits)
    hi.dSdX.raw := Mux(useFloatShadow, floatShadowDSdX.asBits, dSdXInt.asBits)
    hi.dTdX.raw := Mux(useFloatShadow, floatShadowDTdX.asBits, dTdXInt.asBits)
    hi.dSdY.raw := Mux(useFloatShadow, floatShadowDSdY.asBits, dSdYInt.asBits)
    hi.dTdY.raw := Mux(useFloatShadow, floatShadowDTdY.asBits, dTdYInt.asBits)
    hi
  }

  def captureHiAlpha(useFloatShadow: Bool): TriangleSetup.HiAlpha = {
    val hi = TriangleSetup.HiAlpha(c)
    val startAInt = regBank.triangleGeometry.startA.asSInt.resize(60 bits) |<< 18
    val dAdXInt = regBank.triangleGeometry.dAdX.asSInt.resize(60 bits) |<< 18
    val dAdYInt = regBank.triangleGeometry.dAdY.asSInt.resize(60 bits) |<< 18

    hi.start.raw := Mux(useFloatShadow, floatShadowStartA.asBits, startAInt.asBits)
    hi.dAdX.raw := Mux(useFloatShadow, floatShadowDAdX.asBits, dAdXInt.asBits)
    hi.dAdY.raw := Mux(useFloatShadow, floatShadowDAdY.asBits, dAdYInt.asBits)
    hi
  }

  // Capture gradients from integer registers at command time
  def captureGradients(
      useFloatShadow: Bool
  ): Rasterizer.GradientBundle[Rasterizer.InputGradient] = {
    val g = regBank.triangleGeometry
    captureGradientsFrom(
      Seq(
        (g.startR.asBits, g.dRdX.asBits, g.dRdY.asBits), // red   (12.12)
        (g.startG.asBits, g.dGdX.asBits, g.dGdY.asBits), // green (12.12)
        (g.startB.asBits, g.dBdX.asBits, g.dBdY.asBits), // blue  (12.12)
        (g.startZ.asBits, g.dZdX.asBits, g.dZdY.asBits), // depth (20.12)
        (g.startA.asBits, g.dAdX.asBits, g.dAdY.asBits), // alpha (12.12)
        (
          Mux(useFloatShadow, floatShadowStartW.asBits, g.startW.asSInt.resize(60 bits).asBits),
          Mux(useFloatShadow, floatShadowDWdX.asBits, g.dWdX.asSInt.resize(60 bits).asBits),
          Mux(useFloatShadow, floatShadowDWdY.asBits, g.dWdY.asSInt.resize(60 bits).asBits)
        ), // W     (2.30 / float shadow)
        (g.startS.asBits, g.dSdX.asBits, g.dSdY.asBits), // S     (14.18)
        (g.startT.asBits, g.dTdX.asBits, g.dTdY.asBits) // T     (14.18)
      )
    )
  }

  // Capture per-triangle config from registers at command time
  def capturePerTriangleConfig(): TriangleSetup.PerTriangleConfig = {
    val g = regBank.triangleGeometry
    val cfg = TriangleSetup.PerTriangleConfig(c)

    // FBI registers - use bundle accessors from RegisterBank
    cfg.fbzColorPath := regBank.renderConfig.fbzColorPathBundle
    cfg.fogMode := regBank.renderConfig.fogModeBundle
    cfg.alphaMode := regBank.renderConfig.alphaModeBundle
    cfg.fbzMode := regBank.renderConfig.fbzModeBundle

    // TMU registers (single TMU - Voodoo 1 level)
    cfg.tmuTextureMode := regBank.tmuConfig.textureMode
    cfg.tmuTexBaseAddr := regBank.tmuConfig.texBaseAddr
    cfg.tmuTLOD := regBank.tmuConfig.tLOD.resized
    cfg.tmuSendConfig := regBank.tmuConfig.trexInit1(18)
    cfg.tmudSdX.raw := g.dSdX.asBits
    cfg.tmudTdX.raw := g.dTdX.asBits
    cfg.tmudSdY.raw := g.dSdY.asBits
    cfg.tmudTdY.raw := g.dTdY.asBits

    // Constant colors (captured per-triangle to avoid pipelineBusy gap)
    cfg.color0 := regBank.renderConfig.color0
    cfg.color1 := regBank.renderConfig.color1
    cfg.fogColor := regBank.renderConfig.fogColor
    cfg.chromaKey := regBank.renderConfig.chromaKey
    cfg.zaColor := regBank.renderConfig.zaColor
    cfg.drawColorBufferBase := drawColorBufferBase
    cfg.drawAuxBufferBase := drawAuxBufferBase
    cfg.fbPixelStride := fbPixelStride

    // Packed texture layout tables (computed from post-FIFO register values)
    if (c.packedTexLayout) {
      val triCfg = TexLayoutTables.TexConfig()
      triCfg.texBaseAddr := regBank.tmuConfig.texBaseAddr(18 downto 0)
      triCfg.tformat := regBank.tmuConfig.textureMode(11 downto 8).asUInt
      triCfg.tLOD_aspect := regBank.tmuConfig.tLOD(22 downto 21).asUInt
      triCfg.tLOD_sIsWider := regBank.tmuConfig.tLOD(20)
      cfg.texTables := TexLayoutTables.compute(triCfg)
    }

    // NCC table: select table 0 or 1 based on nccSelect (textureMode bit 5)
    val nccSel = regBank.tmuConfig.textureMode(5)
    for (r <- 0 until 4; b <- 0 until 4) {
      cfg.ncc.y(r * 4 + b) := Mux(
        nccSel,
        regBank.nccTable.table1Y(r)((b + 1) * 8 - 1 downto b * 8).asUInt,
        regBank.nccTable.table0Y(r)((b + 1) * 8 - 1 downto b * 8).asUInt
      )
    }
    for (i <- 0 until 4) {
      cfg.ncc.i(i) := Mux(
        nccSel,
        regBank.nccTable.table1I(i)(26 downto 0),
        regBank.nccTable.table0I(i)(26 downto 0)
      )
      cfg.ncc.q(i) := Mux(
        nccSel,
        regBank.nccTable.table1Q(i)(26 downto 0),
        regBank.nccTable.table0Q(i)(26 downto 0)
      )
    }

    cfg
  }

  // Shared helper: build TriangleSetup.Input from integer registers
  def buildTriangleInput(signBit: Bool, useFloatShadow: Bool): TriangleSetup.Input = {
    val out = TriangleSetup.Input(c)
    out.triWithSign.tri(0)(0).raw := regBank.triangleGeometry.vertexAx.asBits
    out.triWithSign.tri(0)(1).raw := regBank.triangleGeometry.vertexAy.asBits
    out.triWithSign.tri(1)(0).raw := regBank.triangleGeometry.vertexBx.asBits
    out.triWithSign.tri(1)(1).raw := regBank.triangleGeometry.vertexBy.asBits
    out.triWithSign.tri(2)(0).raw := regBank.triangleGeometry.vertexCx.asBits
    out.triWithSign.tri(2)(1).raw := regBank.triangleGeometry.vertexCy.asBits
    out.triWithSign.signBit := signBit
    out.grads := captureGradients(useFloatShadow)
    out.hiAlpha := captureHiAlpha(useFloatShadow)
    out.texHi := captureHiTexCoords(useFloatShadow)
    out.config := capturePerTriangleConfig()
    if (c.trace.enabled) {
      out.trace.valid := True
      out.trace.origin := Trace.Origin.triangle
      out.trace.drawId := 0
      out.trace.primitiveId := 0
    }
    out
  }

  val triangleSetupInput = StreamArbiterFactory.assumeOhInput
    .on(Seq(regBank.commands.triangleCmd, regBank.commands.ftriangleCmd))
    .queue(16)

  // Merge integer and float triangle paths
  // Use assumeOhInput since only one path is active at a time (no arbitration needed)
  triangleSetup.i << triangleSetupInput

  // Make triangle streams accessible for simulation monitoring
  regBank.commands.triangleCmd.simPublic()
  triangleSetup.i.simPublic()
  triangleSetup.o.simPublic()
  rasterizer.i.simPublic()
  rasterizer.o.simPublic()
  writeColor.o.fbWrite.simPublic()
  writeAux.o.fbWrite.simPublic()

  // Make TMU and ColorCombine streams accessible for stall monitoring
  tmu.io.input.simPublic()
  tmu.io.output.simPublic()
  colorCombine.io.input.simPublic()
  colorCombine.io.output.simPublic()
  writeColor.i.fromPipeline.simPublic()
  writeAux.i.fromPipeline.simPublic()

  // Connect triangle setup directly to rasterizer
  // Gradients are now captured at command time and flow through TriangleSetup
  rasterizer.i << triangleSetup.o

  // Wire scissor clip bounds from register bank to rasterizer
  rasterizer.enableClipping := regBank.renderConfig.fbzMode.enableClipping
  rasterizer.clipLeft := regBank.renderConfig.clipLeftX
  rasterizer.clipRight := regBank.renderConfig.clipRightX
  rasterizer.clipLowY := regBank.renderConfig.clipLowY
  rasterizer.clipHighY := regBank.renderConfig.clipHighY

  // ========================================================================
  // Fastfill (screen clear) - bypasses TMU/CC/Fog/AlphaTest/FBAccess chain
  // ========================================================================
  val fastfill = Fastfill(c)
  fastfill.i << regBank.commands.fastfillCmd
  fastfill.clipLeft := regBank.renderConfig.clipLeftX
  fastfill.clipRight := regBank.renderConfig.clipRightX
  fastfill.clipLowY := regBank.renderConfig.clipLowY
  fastfill.clipHighY := regBank.renderConfig.clipHighY

  // ========================================================================
  // Linear Frame Buffer (LFB) writes
  // ========================================================================
  val lfb = Lfb(c)
  lfb.io.bus <> internalLfbBus
  lfb.io.lfbMode := regBank.renderConfig.lfbModeBundle
  lfb.io.fbzMode := regBank.renderConfig.fbzModeBundle
  lfb.io.alphaMode := regBank.renderConfig.alphaModeBundle
  lfb.io.fogMode := regBank.renderConfig.fogModeBundle
  lfb.io.fogColor := regBank.renderConfig.fogColor
  lfb.io.zaColor := regBank.renderConfig.zaColor
  lfb.io.fbWriteColorBaseAddr := lfbWriteColorBufferBase
  lfb.io.fbWriteAuxBaseAddr := lfbWriteAuxBufferBase
  lfb.io.fbReadColorBaseAddr := lfbReadColorBufferBase
  lfb.io.fbReadAuxBaseAddr := lfbReadAuxBufferBase
  lfb.io.fbPixelStride := fbPixelStride

  // ========================================================================
  // Color Combine Unit
  // ========================================================================
  // Decode FbzColorPath into ColorCombine.Config (all fields share names and types)
  def decodeColorCombineConfig(fcp: FbzColorPath): ColorCombine.Config = {
    val cfg = ColorCombine.Config()
    cfg.assignSomeByName(fcp)
    cfg
  }

  // ========================================================================
  // Texture Mapping Unit (single TMU - Voodoo 1 level)
  // ========================================================================
  // Architecture:
  //   Rasterizer output is forked:
  //     - One path goes to TMU for texture fetch
  //     - Other path is queued to preserve gradients during TMU's latency
  //   TMU's texture output is joined with the queued gradients
  //   This joined result goes to ColorCombine
  //
  // TMU configuration is now captured per-triangle and flows through the stream

  // Texture memory bus: connected through texture arbiter (see below)

  // ========================================================================
  // Palette writes via NCC table 0 I/Q registers (bit 31 = palette mode)
  // ========================================================================
  // When a NCC table 0 I/Q register is written with bit 31 set, the write
  // programs the 256-entry palette RAM instead of the NCC table.
  // Even registers (I0, I2, Q0, Q2): palette_index = (val >> 23) & 0xFE
  // Odd registers (I1, I3, Q1, Q3): palette_index = ((val >> 23) & 0xFE) | 0x01
  // Color: val(23:0) (RGB888)
  val paletteWriteFlow = Flow(Tmu.PaletteWrite())
  paletteWriteFlow.valid := False
  paletteWriteFlow.payload.address := 0
  paletteWriteFlow.payload.data := 0

  // NCC table 0 I/Q registers: (register accessor, isOdd)
  val nccPaletteRegs = Seq(
    (regBank.nccTable.table0I0, false), // I0 - even (0x334)
    (regBank.nccTable.table0I1, true), // I1 - odd  (0x338)
    (regBank.nccTable.table0I2, false), // I2 - even (0x33c)
    (regBank.nccTable.table0I3, true), // I3 - odd  (0x340)
    (regBank.nccTable.table0Q0, false), // Q0 - even (0x344)
    (regBank.nccTable.table0Q1, true), // Q1 - odd  (0x348)
    (regBank.nccTable.table0Q2, false), // Q2 - even (0x34c)
    (regBank.nccTable.table0Q3, true) // Q3 - odd  (0x350)
  )
  for ((reg, isOdd) <- nccPaletteRegs) {
    val prev = RegNext(reg)
    when(reg =/= prev && reg(31)) {
      paletteWriteFlow.valid := True
      val idx = reg(30 downto 23).asUInt & 0xfe
      paletteWriteFlow.payload.address := (if (isOdd) (idx | 1).resized else idx.resized)
      paletteWriteFlow.payload.data := reg(23 downto 0)
    }
  }

  tmu.io.paletteWrite << paletteWriteFlow

  // Y-origin transform: when fbzMode bit 17 is set, flip Y for bottom-up rendering
  // yOriginSwapValue comes from fbiInit3[31:22]; fb_y = yOriginSwapValue - raster_y
  val yOriginSwapValue = regBank.init.fbiInit3_yOriginSwap
  val yOriginEnable = regBank.renderConfig.fbzMode.yOrigin

  val rasterYTransformed = rasterizer.o.map { out =>
    val result = cloneOf(out)
    result := out
    when(yOriginEnable) {
      result.coords(1) := yOriginSwapValue.resize(c.vertexFormat.nonFraction bits).asSInt - out
        .coords(1)
    }
    result
  }
  rasterYTransformed.simPublic()

  // Fork rasterizer output: one to TMU, one to a queue for synchronization
  val rasterYPipe = rasterYTransformed.stage()
  val rasterFork = StreamFork2(rasterYPipe, synchronous = true)

  // Queue to hold rasterizer data while TMU processes
  // Depth must be >= TMU's maximum in-flight transactions
  val tmuGradQueue = rasterFork._2.queue(4).stage().stage()

  // Connect fork path 1 to TMU
  val tmuInput = Stream(Tmu.Input(c))
  tmuInput.translateFrom(rasterFork._1.stage()) { (out, in) =>
    out.s := in.grads.sGrad // TMU's S/W coordinate (interpolated value from rasterizer)
    out.t := in.grads.tGrad // TMU's T/W coordinate (interpolated value from rasterizer)
    out.w := in.grads.wGrad // Shared 1/W (interpolated value from rasterizer)
    out.cOther.r := 0 // No upstream texture for single TMU
    out.cOther.g := 0
    out.cOther.b := 0
    out.aOther := 0
    // TMU config from captured per-triangle state
    out.config.textureMode := in.config.tmuTextureMode
    out.config.texBaseAddr := in.config.tmuTexBaseAddr
    out.config.tLOD := in.config.tmuTLOD
    out.config.sendConfig := in.config.tmuSendConfig
    out.config.ncc := in.config.ncc
    if (c.packedTexLayout) {
      out.config.texTables := in.config.texTables
    }
    // Texture coordinate gradients for LOD calculation (from config, captured at command time)
    out.dSdX := in.config.tmudSdX
    out.dTdX := in.config.tmudTdX
    out.dSdY := in.config.tmudSdY
    out.dTdY := in.config.tmudTdY
    if (c.trace.enabled) {
      out.trace := in.trace
    }
  }
  tmuInput >/-> tmu.io.input

  // Join TMU output with queued gradients
  val tmuJoined = StreamJoin(tmu.io.output.stage(), tmuGradQueue)
  if (c.trace.enabled) {
    tmuGradQueue.simPublic()
    tmuJoined.simPublic()
    when(tmuJoined.valid) {
      assert(tmuJoined.payload._1.trace.asBits === tmuJoined.payload._2.trace.asBits)
    }
  }

  // Connect TMU joined output to ColorCombine
  val colorCombineInput = Stream(ColorCombine.Input(c))
  colorCombineInput.translateFrom(tmuJoined.stage()) { (out, payload) =>
    val tmuOut = payload._1
    val rasterOut = payload._2 // Original rasterizer output

    out.coords := rasterOut.coords

    // Convert interpolated color values from 12.12 fixed-point to 8-bit unsigned
    // Use interpolated values from rasterizer output (properly synchronized)
    // sat() saturates to [0,255] with exp=0, giving 8 integer bits + 12 fractional bits
    // Take the upper 8 bits (integer part) by right-shifting by 12 (the fractional bits)
    val redSat = rasterOut.grads.redGrad.sat(satMax = 255, satMin = 0, exp = 0 exp)
    val greenSat = rasterOut.grads.greenGrad.sat(satMax = 255, satMin = 0, exp = 0 exp)
    val blueSat = rasterOut.grads.blueGrad.sat(satMax = 255, satMin = 0, exp = 0 exp)
    val alphaSat = rasterOut.alphaGradHi.sat(satMax = 255, satMin = 0, exp = 0 exp)

    // Extract integer part (upper 8 bits) from saturated values
    // Color format after sat is UQ(8,12) - 8 integer bits, 12 fractional bits = 20 total bits
    out.iterated.r := (redSat.asBits >> 12).asUInt.resize(8 bits)
    out.iterated.g := (greenSat.asBits >> 12).asUInt.resize(8 bits)
    out.iterated.b := (blueSat.asBits >> 12).asUInt.resize(8 bits)
    out.iteratedAlpha := (alphaSat.asBits >> 30).asUInt.resize(8 bits)

    // Upper 8 bits of Z for alpha local select option (Z is 20.12)
    // Saturate to [0,255] at exp=12 (so bits [19:12] become [7:0])
    // Then extract the integer part
    val depthSat = rasterOut.grads.depthGrad.sat(satMax = 255, satMin = 0, exp = 12 exp)
    out.iteratedZ := (depthSat.asBits >> 12).asUInt.resize(8 bits)

    // Pass through depth for later stages
    out.depth := rasterOut.grads.depthGrad

    // Pass through raw W value for fog wDepth calculation (SQ(32,30) → raw 32-bit SInt)
    out.rawW := rasterOut.grads.wGrad.asSInt.resize(32 bits)

    // Use texture from TMU (single TMU output)
    out.texture := tmuOut.texture
    out.textureAlpha := tmuOut.textureAlpha

    // Constant colors from per-triangle captured config (avoids pipelineBusy gap)
    val color0Bits = rasterOut.config.color0
    val color1Bits = rasterOut.config.color1
    out.color0.r := color0Bits(23 downto 16).asUInt
    out.color0.g := color0Bits(15 downto 8).asUInt
    out.color0.b := color0Bits(7 downto 0).asUInt
    out.color0Alpha := color0Bits(31 downto 24).asUInt
    out.color1.r := color1Bits(23 downto 16).asUInt
    out.color1.g := color1Bits(15 downto 8).asUInt
    out.color1.b := color1Bits(7 downto 0).asUInt
    out.color1Alpha := color1Bits(31 downto 24).asUInt

    // Fog color from per-triangle captured config (pass-through to Fog stage)
    out.fogColor := rasterOut.config.fogColor
    out.chromaKey := rasterOut.config.chromaKey
    out.zaColor := rasterOut.config.zaColor

    // Decode configuration from captured per-triangle fbzColorPath
    out.config := decodeColorCombineConfig(rasterOut.config.fbzColorPath)

    // Per-triangle FIFO registers for downstream stages (alpha test, fog, framebuffer access)
    out.alphaMode := rasterOut.config.alphaMode
    out.fogMode := rasterOut.config.fogMode
    out.fbzMode := rasterOut.config.fbzMode
    out.drawColorBufferBase := rasterOut.config.drawColorBufferBase
    out.drawAuxBufferBase := rasterOut.config.drawAuxBufferBase
    out.fbPixelStride := rasterOut.config.fbPixelStride
    if (c.trace.enabled) {
      out.trace := rasterOut.trace
    }
  }
  colorCombineInput >/-> colorCombine.io.input

  // ========================================================================
  // Fog Stage
  // ========================================================================
  // Build fog table Vec from register bank entries
  val fogTableVec = Vec(regBank.fogTable.fogTable.map { case (dfog, fog) =>
    val entry = Bits(16 bits)
    entry(15 downto 8) := dfog // signed delta
    entry(7 downto 0) := fog // unsigned value
    entry
  })

  // Chroma key: discard pixels where color combine output matches chromaKey register
  val ckBits = colorCombine.io.output.payload.chromaKey
  val ckR = ckBits(23 downto 16).asUInt
  val ckG = ckBits(15 downto 8).asUInt
  val ckB = ckBits(7 downto 0).asUInt
  val ccColor = colorCombine.io.output.payload.color
  val chromaKill = colorCombine.io.output.payload.fbzMode.enableChromaKey &&
    ccColor.r === ckR && ccColor.g === ckG && ccColor.b === ckB
  val afterChromaKey = colorCombine.io.output.throwWhen(chromaKill).stage()

  val fog = Fog(c)
  fog.io.fogTable := fogTableVec
  fog.io.input.simPublic()
  fog.io.output.simPublic()
  // Arbiter: CC output (after chromakey, priority) and LFB pipeline output feed into Fog
  val fogArbiterInput = StreamArbiterFactory.lowerFirst.on(
    Seq(afterChromaKey, lfb.io.pipelineOutput)
  )
  fogArbiterInput >/-> fog.io.input

  // Alpha test: discard pixels that fail alpha comparison
  // Use per-triangle captured alphaMode (not live register) to avoid FIFO sync issues
  val alphaBits = fog.io.output.payload.alphaMode
  val alphaTestEnable = alphaBits.alphaTestEnable
  val alphaFunc = alphaBits.alphaFunc
  val alphaRef = alphaBits.alphaRef
  val srcAlpha = fog.io.output.payload.alpha

  val alphaPassed = alphaFunc.mux(
    0 -> False, // NEVER
    1 -> (srcAlpha < alphaRef), // LESS
    2 -> (srcAlpha === alphaRef), // EQUAL
    3 -> (srcAlpha <= alphaRef), // LEQUAL
    4 -> (srcAlpha > alphaRef), // GREATER
    5 -> (srcAlpha =/= alphaRef), // NOTEQUAL
    6 -> (srcAlpha >= alphaRef), // GEQUAL
    7 -> True // ALWAYS
  )

  val alphaKill = alphaTestEnable && !alphaPassed
  val afterAlphaTest = fog.io.output.throwWhen(alphaKill).stage()

  // ========================================================================
  // Framebuffer Access (depth test + alpha blend)
  // ========================================================================
  val fbAccess = FramebufferAccess(c)
  afterAlphaTest >/-> fbAccess.io.input
  fbAccess.io.input.simPublic()
  fbAccess.io.output.simPublic()

  // fbzMode fields are now per-pixel (carried through pipeline in Fog.Output.fbzMode)
  // alphaMode blend fields are now per-pixel (carried through pipeline in Fog.Output.alphaMode)
  val pixelsInCounter = Reg(UInt(24 bits)) init (0)
  val chromaFailCounter = Reg(UInt(24 bits)) init (0)
  val zFuncFailCounter = Reg(UInt(24 bits)) init (0)
  val aFuncFailCounter = Reg(UInt(24 bits)) init (0)
  val pixelsOutCounter = Reg(UInt(24 bits)) init (0)

  when(colorCombine.io.output.fire && chromaKill) {
    chromaFailCounter := chromaFailCounter + 1
  }
  when(fog.io.output.fire && alphaKill) {
    aFuncFailCounter := aFuncFailCounter + 1
  }
  when(fbAccess.io.zFuncFail) {
    zFuncFailCounter := zFuncFailCounter + 1
  }

  // ========================================================================
  // Triangle → Write.PreDither (from FramebufferAccess output)
  // ========================================================================
  val trianglePreDither = fbAccess.io.output
    .translateWith {
      val fbIn = fbAccess.io.output.payload
      val out = Write.PreDither(c)
      out.r := fbIn.color.r
      out.g := fbIn.color.g
      out.b := fbIn.color.b
      out.coords := fbIn.coords
      out.enableDithering := fbIn.enableDithering
      out.ditherAlgorithm := fbIn.ditherAlgorithm
      out.depthAlpha := (fbIn.enableAlphaPlanes ? fbIn.alpha.resize(16 bits) | fbIn.newDepth).asBits
      out.rgbWrite := fbIn.rgbWrite
      out.auxWrite := fbIn.auxWrite
      out.fbBaseAddr := fbIn.fbBaseAddr
      out.auxBaseAddr := fbIn.auxBaseAddr
      out.fbPixelStride := fbIn.fbPixelStride
      if (c.trace.enabled) {
        out.trace := fbIn.trace
      }
      out
    }
    .m2sPipe()

  // ========================================================================
  // Fastfill → Write.PreDither (direct color1/zaColor)
  // ========================================================================
  val fastfillWrite = FastfillWrite(c)
  fastfillWrite.io.pixels << fastfill.o
  fastfillWrite.io.cmdFire := regBank.commands.fastfillCmd.fire
  fastfillWrite.io.running := fastfill.running
  fastfillWrite.io.regs.color1 := regBank.renderConfig.color1
  fastfillWrite.io.regs.zaColor := regBank.renderConfig.zaColor
  fastfillWrite.io.regs.fbzMode := regBank.renderConfig.fbzModeBundle
  fastfillWrite.io.regs.drawColorBufferBase := drawColorBufferBase
  fastfillWrite.io.regs.drawAuxBufferBase := drawAuxBufferBase
  fastfillWrite.io.regs.yOriginSwapValue := yOriginSwapValue
  fastfillWrite.io.regs.fbPixelStride := fbPixelStride

  val pixelsInDelta =
    colorCombine.io.output.fire.asUInt.resize(3 bits) +
      lfb.io.pipelineOutput.fire.asUInt.resize(3 bits) +
      lfb.io.writeOutput.fire.asUInt.resize(3 bits) +
      fastfillWrite.io.output.fire.asUInt.resize(3 bits)

  pixelsInCounter := pixelsInCounter + pixelsInDelta.resize(24 bits)

  // ========================================================================
  // Shared Dither: merge all three PreDither paths, dither, produce plane writes
  // ========================================================================
  // lowerFirst gives fastfill priority (index 0), then triangle, then LFB bypass
  val preDitherMerged = StreamArbiterFactory.lowerFirst
    .on(
      Seq(fastfillWrite.io.output, trianglePreDither, lfb.io.writeOutput)
    )
    .stage()

  val dither = Dither()
  val (forDither, forPipe) = StreamFork2(preDitherMerged, synchronous = true)

  dither.io.input.translateFrom(forDither) { (ditIn, pd) =>
    ditIn.r := pd.r
    ditIn.g := pd.g
    ditIn.b := pd.b
    ditIn.x := pd.coords(0).asUInt.resize(2 bits)
    ditIn.y := pd.coords(1).asUInt.resize(2 bits)
    ditIn.enable := pd.enableDithering
    ditIn.use2x2 := pd.ditherAlgorithm
  }

  val preDitherPiped = forPipe.m2sPipe()
  val ditherJoined = StreamJoin(dither.io.output, preDitherPiped)

  val (forColorWrite, forAuxWrite) = StreamFork2(ditherJoined, synchronous = true)

  val colorWriteInput = forColorWrite
    .throwWhen(!forColorWrite.payload._2.rgbWrite)
    .translateWith {
      val ditOut = forColorWrite.payload._1
      val pd = forColorWrite.payload._2
      val out = Write.Input(c)
      out.coords := pd.coords
      out.data := (ditOut.ditR ## ditOut.ditG ## ditOut.ditB).asBits
      out.fbBaseAddr := pd.fbBaseAddr
      out.fbPixelStride := pd.fbPixelStride
      if (c.trace.enabled) {
        out.trace := pd.trace
      }
      out
    }
  colorWriteInput >/-> writeColor.i.fromPipeline

  val auxWriteInput = forAuxWrite
    .throwWhen(!forAuxWrite.payload._2.auxWrite)
    .translateWith {
      val pd = forAuxWrite.payload._2
      val out = Write.Input(c)
      out.coords := pd.coords
      out.data := pd.depthAlpha
      out.fbBaseAddr := pd.auxBaseAddr
      out.fbPixelStride := pd.fbPixelStride
      if (c.trace.enabled) {
        out.trace := pd.trace
      }
      out
    }
  auxWriteInput >/-> writeAux.i.fromPipeline

  when(writeColor.i.fromPipeline.fire) {
    pixelsOutCounter := pixelsOutCounter + 1
  }

  regBank.io.statisticsIn.pixelsIn := pixelsInCounter
  regBank.io.statisticsIn.chromaFail := chromaFailCounter
  regBank.io.statisticsIn.zFuncFail := zFuncFailCounter
  regBank.io.statisticsIn.aFuncFail := aFuncFailCounter
  regBank.io.statisticsIn.pixelsOut := pixelsOutCounter

  val fbColorBusy = Bool()
  val fbAuxBusy = Bool()
  val fbFillHits = UInt(32 bits)
  val fbFillMisses = UInt(32 bits)
  val fbFillBurstCount = UInt(32 bits)
  val fbFillBurstBeats = UInt(32 bits)
  val fbFillStallCycles = UInt(32 bits)

  if (c.useFbFillCache) {
    val fbColorCache = FramebufferPlaneCache(c)
    val fbAuxCache = FramebufferPlaneCache(c)
    fbColorCache.io.flush := io.flushFbCaches
    fbAuxCache.io.flush := io.flushFbCaches

    fbAccess.io.fbReadColorReq.s2mPipe() >> fbColorCache.io.readReq
    fbAccess.io.fbReadColorRsp << fbColorCache.io.readRsp
    fbAccess.io.fbReadAuxReq.s2mPipe() >> fbAuxCache.io.readReq
    fbAccess.io.fbReadAuxRsp << fbAuxCache.io.readRsp

    fbColorCache.io.writeReq << writeColor.o.fbWrite.s2mPipe()
    fbAuxCache.io.writeReq << writeAux.o.fbWrite.s2mPipe()

    val fbArbiter = BmbArbiter(
      inputsParameter = Seq(
        FramebufferPlaneCache.bmbParams(c),
        FramebufferPlaneCache.bmbParams(c),
        Lfb.fbReadBmbParams(c)
      ),
      outputParameter = Core.fbMemBmbParams(c),
      lowerFirstPriority = true
    )

    fbArbiter.io.inputs(0).cmd << fbColorCache.io.mem.cmd.s2mPipe()
    fbColorCache.io.mem.rsp << fbArbiter.io.inputs(0).rsp.s2mPipe()

    fbArbiter.io.inputs(1).cmd << fbAuxCache.io.mem.cmd.s2mPipe()
    fbAuxCache.io.mem.rsp << fbArbiter.io.inputs(1).rsp.s2mPipe()

    fbArbiter.io.inputs(2).cmd << lfb.io.fbReadBus.cmd
    lfb.io.fbReadBus.rsp << fbArbiter.io.inputs(2).rsp.s2mPipe()
    fbArbiter.io.output <> io.fbMem

    fbColorBusy := fbColorCache.io.busy
    fbAuxBusy := fbAuxCache.io.busy
    fbFillHits := (fbColorCache.io.fillHits + fbAuxCache.io.fillHits).resized
    fbFillMisses := (fbColorCache.io.fillMisses + fbAuxCache.io.fillMisses).resized
    fbFillBurstCount := (fbColorCache.io.fillBurstCount + fbAuxCache.io.fillBurstCount).resized
    fbFillBurstBeats := (fbColorCache.io.fillBurstBeats + fbAuxCache.io.fillBurstBeats).resized
    fbFillStallCycles := (fbColorCache.io.fillStallCycles + fbAuxCache.io.fillStallCycles).resized
  } else {
    val fbColorReadPort = FramebufferPlaneCache(c)
    val fbColorWritePort = FramebufferPlaneCache(c)
    val fbAuxReadPort = FramebufferPlaneCache(c)
    val fbAuxWritePort = FramebufferPlaneCache(c)

    fbColorReadPort.io.flush := False
    fbColorWritePort.io.flush := False
    fbAuxReadPort.io.flush := False
    fbAuxWritePort.io.flush := False

    fbAccess.io.fbReadColorReq.s2mPipe() >> fbColorReadPort.io.readReq
    fbAccess.io.fbReadColorRsp << fbColorReadPort.io.readRsp
    fbAccess.io.fbReadAuxReq.s2mPipe() >> fbAuxReadPort.io.readReq
    fbAccess.io.fbReadAuxRsp << fbAuxReadPort.io.readRsp

    fbColorReadPort.io.writeReq.valid := False
    fbColorReadPort.io.writeReq.address := 0
    fbColorReadPort.io.writeReq.data := 0
    fbColorReadPort.io.writeReq.mask := 0
    fbAuxReadPort.io.writeReq.valid := False
    fbAuxReadPort.io.writeReq.address := 0
    fbAuxReadPort.io.writeReq.data := 0
    fbAuxReadPort.io.writeReq.mask := 0

    fbColorWritePort.io.readReq.valid := False
    fbColorWritePort.io.readReq.address := 0
    fbColorWritePort.io.readRsp.ready := True
    fbAuxWritePort.io.readReq.valid := False
    fbAuxWritePort.io.readReq.address := 0
    fbAuxWritePort.io.readRsp.ready := True

    fbColorWritePort.io.writeReq << writeColor.o.fbWrite.s2mPipe()
    fbAuxWritePort.io.writeReq << writeAux.o.fbWrite.s2mPipe()

    val fbArbiter = BmbArbiter(
      inputsParameter = Seq(
        FramebufferPlaneCache.bmbParams(c),
        FramebufferPlaneCache.bmbParams(c),
        FramebufferPlaneCache.bmbParams(c),
        FramebufferPlaneCache.bmbParams(c),
        Lfb.fbReadBmbParams(c)
      ),
      outputParameter = Core.fbMemBmbParams(c),
      lowerFirstPriority = true
    )

    fbArbiter.io.inputs(0).cmd << fbColorWritePort.io.mem.cmd.s2mPipe()
    fbColorWritePort.io.mem.rsp << fbArbiter.io.inputs(0).rsp.s2mPipe()

    fbArbiter.io.inputs(1).cmd << fbAuxWritePort.io.mem.cmd.s2mPipe()
    fbAuxWritePort.io.mem.rsp << fbArbiter.io.inputs(1).rsp.s2mPipe()

    fbArbiter.io.inputs(2).cmd << fbColorReadPort.io.mem.cmd.s2mPipe()
    fbColorReadPort.io.mem.rsp << fbArbiter.io.inputs(2).rsp.s2mPipe()

    fbArbiter.io.inputs(3).cmd << fbAuxReadPort.io.mem.cmd.s2mPipe()
    fbAuxReadPort.io.mem.rsp << fbArbiter.io.inputs(3).rsp.s2mPipe()

    fbArbiter.io.inputs(4).cmd << lfb.io.fbReadBus.cmd
    lfb.io.fbReadBus.rsp << fbArbiter.io.inputs(4).rsp.s2mPipe()
    fbArbiter.io.output <> io.fbMem

    fbColorBusy := fbColorReadPort.io.busy || fbColorWritePort.io.busy
    fbAuxBusy := fbAuxReadPort.io.busy || fbAuxWritePort.io.busy
    fbFillHits := 0
    fbFillMisses := 0
    fbFillBurstCount := 0
    fbFillBurstBeats := 0
    fbFillStallCycles := 0
  }

  // ========================================================================
  // Packed texture address translation at FIFO drain
  // ========================================================================
  // When PciFifo drains a texture entry, translate PCI address to packed SRAM address
  val cpuTexWriteBus = Bmb(Core.cpuTexBmbParams)

  case class PackedCpuTexWritePrep() extends Bundle {
    val validWrite = Bool()
    val lodBase = UInt(22 bits)
    val lodShift = UInt(4 bits)
    val s = UInt(8 bits)
    val t = UInt(8 bits)
    val is16bit = Bool()
    val data = Bits(32 bits)
    val mask = Bits(4 bits)
  }

  case class LinearCpuTexWritePrep() extends Bundle {
    val validWrite = Bool()
    val address = UInt(c.addressWidth.value bits)
    val data = Bits(32 bits)
    val mask = Bits(4 bits)
  }

  // Discard texture writes targeting TMUs that don't exist.
  // PCI texture address space: [21] = TMU1 select, [22] = TMU2 select.
  // Voodoo1 has only TMU0, so writes with either bit set are discarded.
  val pciAddr = pciFifo.io.texDrain.pciAddr
  val tmuValid = pciAddr(22 downto 21) === 0 // Only TMU0 exists

  val cpuTexWriteCmd = if (c.packedTexLayout) {
    // Compute tex layout tables from post-FIFO register bank values
    val texCfg = TexLayoutTables.TexConfig()
    texCfg.texBaseAddr := regBank.tmuConfig.texBaseAddr(18 downto 0)
    texCfg.tformat := regBank.tmuConfig.textureMode(11 downto 8).asUInt
    texCfg.tLOD_aspect := regBank.tmuConfig.tLOD(22 downto 21).asUInt
    texCfg.tLOD_sIsWider := regBank.tmuConfig.tLOD(20)
    val writeTables = TexLayoutTables.compute(texCfg)

    val is16bit = texCfg.tformat >= Tmu.TextureFormat.ARGB8332
    val seq8 = regBank.tmuConfig.textureMode(31) // textureMode bit 31: SST_SEQ_8_DOWNLD

    // Extract LOD, T, S from PCI address (matching 86Box voodoo_tex_writel)
    val lod = pciAddr(20 downto 17)
    val t = pciAddr(16 downto 9)

    // S extraction depends on format
    val s = UInt(8 bits)
    when(is16bit) {
      s := ((pciAddr >> 1) & 0xfe).resize(8 bits)
    }.elsewhen(seq8) {
      s := (pciAddr & 0xfc).resize(8 bits)
    }.otherwise {
      s := ((pciAddr >> 1) & 0xfc).resize(8 bits) // interleaved 8-bit
    }

    // Guard: discard writes with lod > 8 or targeting non-existent TMU
    val drainValid = lod <= 8 && tmuValid

    val texDrainPrep = pciFifo.io.texDrain
      .translateWith {
        val prep = PackedCpuTexWritePrep()
        prep.validWrite := drainValid
        prep.lodBase := writeTables.texBase(lod.resize(4 bits))
        prep.lodShift := writeTables.texShift(lod.resize(4 bits))
        prep.s := s
        prep.t := t
        prep.is16bit := is16bit
        prep.data := pciFifo.io.texDrain.data
        prep.mask := pciFifo.io.texDrain.mask
        prep
      }
      .m2sPipe()

    texDrainPrep
      .throwWhen(!texDrainPrep.payload.validWrite)
      .translateWith {
        val prep = texDrainPrep.payload
        val sramAddr = UInt(c.addressWidth.value bits)
        when(prep.is16bit) {
          sramAddr := (prep.lodBase + (prep.s << 1).resize(22 bits) +
            (prep.t.resize(22 bits) << (prep.lodShift +^ U(1)).resize(5 bits)).resize(22 bits))
            .resize(c.addressWidth.value bits)
        }.otherwise {
          sramAddr :=
            (prep.lodBase + prep.s.resize(22 bits) + (prep.t.resize(22 bits) << prep.lodShift)
              .resize(22 bits)).resize(c.addressWidth.value bits)
        }

        val cmd = Fragment(BmbCmd(Core.cpuTexBmbParams))
        cmd.fragment.opcode := Bmb.Cmd.Opcode.WRITE
        cmd.fragment.address := sramAddr
        cmd.fragment.data := prep.data
        cmd.fragment.mask := prep.mask
        cmd.fragment.length := 3 // 4 bytes
        cmd.fragment.source := 0
        cmd.last := True
        cmd
      }
      .m2sPipe()
  } else {
    // Fallback: linear texture write mapping (legacy mode)
    val texBaseAddr = regBank.tmuConfig.texBaseAddr(18 downto 0)
    val flatSramAddr = ((texBaseAddr << 3) +^ pciAddr).resize(26 bits)

    val texDrainPrep = pciFifo.io.texDrain
      .translateWith {
        val prep = LinearCpuTexWritePrep()
        prep.validWrite := tmuValid
        prep.address := flatSramAddr
        prep.data := pciFifo.io.texDrain.data
        prep.mask := pciFifo.io.texDrain.mask
        prep
      }
      .m2sPipe()

    texDrainPrep
      .throwWhen(!texDrainPrep.payload.validWrite)
      .translateWith {
        val prep = texDrainPrep.payload
        val cmd = Fragment(BmbCmd(Core.cpuTexBmbParams))
        cmd.fragment.opcode := Bmb.Cmd.Opcode.WRITE
        cmd.fragment.address := prep.address
        cmd.fragment.data := prep.data
        cmd.fragment.mask := prep.mask
        cmd.fragment.length := 3 // 4 bytes
        cmd.fragment.source := 0
        cmd.last := True
        cmd
      }
      .m2sPipe()
  }

  cpuTexWriteBus.cmd << cpuTexWriteCmd
  cpuTexWriteBus.rsp.ready := True

  // ========================================================================
  // Texture arbiter (3 inputs → texMem: TMU read, CPU write drain, CPU read)
  // ========================================================================
  val texArbiter = BmbArbiter(
    inputsParameter = Seq(
      Tmu.bmbParams(c),
      Core.cpuTexBmbParams,
      Core.cpuTexBmbParams
    ),
    outputParameter = Core.texMemBmbParams(c),
    lowerFirstPriority = true // texRead gets priority over CPU writes
  )

  // Pipe texRead cmd.ready to break combinational loop: TMU's StreamFork has
  // a combinational path from texRead.cmd.ready -> texRead.cmd.valid, which
  // loops through BmbArbiter's StreamArbiter mask logic (valid -> ready).
  texArbiter.io.inputs(0).cmd << tmu.io.texRead.cmd.s2mPipe()
  texArbiter.io.inputs(0).rsp >> tmu.io.texRead.rsp

  texArbiter.io.inputs(1) <> cpuTexWriteBus
  texArbiter.io.inputs(2) <> cpuTexBus
  texArbiter.io.output <> io.texMem

  // Pipeline busy signal: any stage has valid data (placed after all stages instantiated)
  regBank.io.pipelineBusy.simPublic()
  triangleSetup.o.valid.simPublic()
  rasterizer.o.valid.simPublic()
  tmu.io.input.valid.simPublic()
  tmu.io.busy.simPublic()
  fbAccess.io.busy.simPublic()
  fbColorBusy.simPublic()
  fbAuxBusy.simPublic()
  colorCombine.io.input.valid.simPublic()
  fog.io.busy.simPublic()
  fbAccess.io.input.valid.simPublic()
  writeColor.i.fromPipeline.valid.simPublic()
  writeAux.i.fromPipeline.valid.simPublic()
  fastfill.running.simPublic()
  swapBuffer.io.waiting.simPublic()
  lfb.io.busy.simPublic()
  regBank.commands.nopCmd.valid.simPublic()
  regBank.commands.fastfillCmd.valid.simPublic()
  regBank.commands.swapbufferCmd.valid.simPublic()
  // Pipeline busy: any stage active.
  // rasterizer.running covers the gap between accepting a triangle and producing
  // first output pixel (rasterizer.o.valid only asserts once iteration begins).
  // This matches SST-1 hardware where SST_BUSY reflects actual pipeline activity.
  rasterizer.running.simPublic()
  fbFillHits.simPublic()
  fbFillMisses.simPublic()
  fbFillBurstCount.simPublic()
  fbFillBurstBeats.simPublic()
  fbFillStallCycles.simPublic()
  val pipelineBusySignal =
    triangleSetup.o.valid || rasterizer.running || tmu.io.input.valid ||
      tmu.io.busy || fbAccess.io.busy || fbColorBusy || fbAuxBusy ||
      colorCombine.io.input.valid || fog.io.busy || fbAccess.io.input.valid ||
      writeColor.i.fromPipeline.valid || writeAux.i.fromPipeline.valid ||
      writeColor.o.fbWrite.valid || writeAux.o.fbWrite.valid ||
      fastfill.running || fastfill.o.valid || fastfillWrite.io.output.valid ||
      preDitherMerged.valid || preDitherPiped.valid ||
      dither.io.output.valid || ditherJoined.valid ||
      forColorWrite.valid || forAuxWrite.valid ||
      colorWriteInput.valid || auxWriteInput.valid ||
      regBank.commands.nopCmd.valid || regBank.commands.fastfillCmd.valid ||
      regBank.commands.swapbufferCmd.valid ||
      swapBuffer.io.waiting || lfb.io.busy
  val busyDebugSignal = Bits(32 bits)
  busyDebugSignal := 0
  busyDebugSignal(0) := triangleSetup.o.valid
  busyDebugSignal(1) := rasterizer.running
  busyDebugSignal(2) := tmu.io.input.valid
  busyDebugSignal(3) := tmu.io.busy
  busyDebugSignal(4) := fbAccess.io.busy
  busyDebugSignal(5) := colorCombine.io.input.valid
  busyDebugSignal(6) := fog.io.busy
  busyDebugSignal(7) := fbAccess.io.input.valid
  busyDebugSignal(8) := writeColor.i.fromPipeline.valid
  busyDebugSignal(9) := writeAux.i.fromPipeline.valid
  busyDebugSignal(10) := fastfill.running
  busyDebugSignal(11) := swapBuffer.io.waiting
  busyDebugSignal(12) := lfb.io.busy
  busyDebugSignal(13) := fastfill.o.valid
  busyDebugSignal(14) := fastfill.o.ready
  busyDebugSignal(15) := fastfillWrite.io.output.valid
  busyDebugSignal(16) := fastfillWrite.io.output.ready
  busyDebugSignal(17) := preDitherMerged.valid
  busyDebugSignal(18) := preDitherMerged.ready
  busyDebugSignal(19) := preDitherPiped.valid
  busyDebugSignal(20) := preDitherPiped.ready
  busyDebugSignal(21) := dither.io.output.valid
  busyDebugSignal(22) := dither.io.output.ready
  busyDebugSignal(23) := ditherJoined.valid
  busyDebugSignal(24) := ditherJoined.ready
  busyDebugSignal(25) := forColorWrite.valid
  busyDebugSignal(26) := forColorWrite.ready
  busyDebugSignal(27) := forAuxWrite.valid
  busyDebugSignal(28) := forAuxWrite.ready
  busyDebugSignal(29) := writeColor.i.fromPipeline.ready
  busyDebugSignal(30) := writeAux.i.fromPipeline.ready
  busyDebugSignal(31) := fastfillWrite.io.output.payload.auxWrite
  val writePathDebugSignal = Bits(32 bits)
  writePathDebugSignal := 0
  writePathDebugSignal(0) := fastfill.running
  writePathDebugSignal(1) := fastfill.o.valid
  writePathDebugSignal(2) := fastfill.o.ready
  writePathDebugSignal(3) := fastfillWrite.io.output.valid
  writePathDebugSignal(4) := fastfillWrite.io.output.ready
  writePathDebugSignal(5) := preDitherMerged.valid
  writePathDebugSignal(6) := preDitherMerged.ready
  writePathDebugSignal(7) := ditherJoined.valid
  writePathDebugSignal(8) := ditherJoined.ready
  writePathDebugSignal(9) := forColorWrite.valid
  writePathDebugSignal(10) := forColorWrite.ready
  writePathDebugSignal(11) := colorWriteInput.valid
  writePathDebugSignal(12) := colorWriteInput.ready
  writePathDebugSignal(13) := writeColor.o.fbWrite.valid
  writePathDebugSignal(14) := writeColor.o.fbWrite.ready
  writePathDebugSignal(15) := fbColorBusy
  writePathDebugSignal(16) := fbAuxBusy
  writePathDebugSignal(17) := forAuxWrite.valid
  writePathDebugSignal(18) := forAuxWrite.ready
  writePathDebugSignal(19) := auxWriteInput.valid
  writePathDebugSignal(20) := auxWriteInput.ready
  writePathDebugSignal(21) := writeAux.o.fbWrite.valid
  writePathDebugSignal(22) := writeAux.o.fbWrite.ready
  writePathDebugSignal(23) := fbFillHits.orR
  writePathDebugSignal(24) := fbFillMisses.orR
  writePathDebugSignal(25) := fbFillBurstCount.orR
  writePathDebugSignal(26) := fbFillBurstBeats.orR
  writePathDebugSignal(27) := fbFillStallCycles.orR
  writePathDebugSignal(28) := pixelsInCounter.orR
  writePathDebugSignal(29) := pixelsOutCounter.orR
  writePathDebugSignal(30) := zFuncFailCounter.orR
  writePathDebugSignal(31) := aFuncFailCounter.orR
  regBank.io.pipelineBusy := pipelineBusySignal
  regBank.io.busyDebug := busyDebugSignal
  regBank.io.writePathDebug := writePathDebugSignal
  lfb.io.pciFifoEmpty := pciFifo.io.fifoEmpty
  lfb.io.pipelineBusy := pipelineBusySignal

  // ========================================================================
  // PciFifo control signal wiring
  // ========================================================================
  pciFifo.io.pipelineBusy := pipelineBusySignal
  pciFifo.io.commandReady := regBank.io.commandReady
  pciFifo.io.wasEnqueuedAddr := U(0x128, pciFifo.busAddrWidth bits) // swapbufferCMD address

  // Feed PciFifo status into RegisterBank
  regBank.io.pciFifoEmpty := pciFifo.io.fifoEmpty
  regBank.io.pciFifoFree := pciFifo.io.pciFifoFree
  regBank.io.swapCmdEnqueued := pciFifo.io.wasEnqueued
  regBank.io.syncDrained := pciFifo.io.syncDrained
}
