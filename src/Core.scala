package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

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
    sourceWidth = 1 + log2Up(3), // 3 arbiter inputs
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  def texMemBmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 4 + log2Up(2), // max(srcW=1,4) + 1 route bit = 5
    contextWidth = 0,
    lengthWidth = 2,
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
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}

case class Core(c: Config) extends Component {
  val io = new Bundle {
    // Unified CPU bus (24-bit address covers 16MB PCI BAR)
    val cpuBus = slave(Bmb(Core.cpuBmbParams))

    // Framebuffer memory bus (R/W, internal 3-port arbitration)
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

    // Statistics inputs
    val statisticsIn = in(new Bundle {
      val pixelsIn = UInt(24 bits)
      val chromaFail = UInt(24 bits)
      val zFuncFail = UInt(24 bits)
      val aFuncFail = UInt(24 bits)
      val pixelsOut = UInt(24 bits)
    })

    // Framebuffer base address
    val fbBaseAddr = in UInt (c.addressWidth)

  }

  // Instantiate components
  val addressRemapper =
    AddressRemapper(RegisterBank.externalBmbParams(c), RegisterBank.bmbParams(c))
  val regBank = RegisterBank(c)
  val triangleSetup = TriangleSetup(c)
  val rasterizer = Rasterizer(c)
  val tmu = Tmu(c) // Single TMU (Voodoo 1 level)
  val colorCombine = ColorCombine(c)
  val write = Write(c)

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

  // texBaseAddr snooping for CPU texture write relocation (SST-1 spec 5.53)
  // Snoop before the FIFO — texBaseAddr is not fifoBypass, so reading the
  // register bank would give a stale value. Real TREX intercepts PCI writes.
  val texBaseAddr = Reg(UInt(19 bits)) init (0)
  when(
    cpuCmd.fire && target === 0 &&
      cpuCmd.address(9 downto 0) === U(0x30c, 10 bits) && cpuCmd.opcode(0)
  ) {
    texBaseAddr := cpuCmd.data(18 downto 0).asUInt
  }

  // Register which target was selected (latched on cmd.fire for response routing)
  val rspTarget = RegNextWhen(target, cpuCmd.fire) init (0)

  val isReg = target === 0
  val isLfb = target === 1
  val isTex = target >= 2

  // Internal register bus (22-bit address, includes bit 21 for remap detection)
  val internalRegBus = Bmb(RegisterBank.externalBmbParams(c))
  internalRegBus.cmd.valid := cpuCmd.valid && isReg
  internalRegBus.cmd.opcode := cpuCmd.opcode
  internalRegBus.cmd.address := cpuCmd.address.resize(22 bits)
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

  // CPU texture bus with texBaseAddr relocation
  // SRAM addr = (texBaseAddr << 3) + PCI_offset (matches real TREX)
  val cpuTexBus = Bmb(Core.cpuTexBmbParams)
  val texPciOffset = cpuCmd.address(22 downto 0) // 23-bit PCI offset within texture space
  val texSramAddr = ((texBaseAddr << 3) +^ texPciOffset).resize(26 bits)
  cpuTexBus.cmd.valid := cpuCmd.valid && isTex
  cpuTexBus.cmd.address := texSramAddr
  cpuTexBus.cmd.opcode := cpuCmd.opcode
  cpuTexBus.cmd.data := cpuCmd.data
  cpuTexBus.cmd.mask := cpuCmd.mask
  cpuTexBus.cmd.length := cpuCmd.length
  cpuTexBus.cmd.last := cpuCmd.last
  cpuTexBus.cmd.source := cpuCmd.source

  // Mux ready back from selected target
  cpuCmd.ready := (isReg && internalRegBus.cmd.ready) ||
    (isLfb && internalLfbBus.cmd.ready) ||
    (isTex && cpuTexBus.cmd.ready)

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

  when(rspTarget === 0) {
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

  // Connect address remapper between internal register bus and register bank
  // AddressRemapper handles RGBZASTW → standard layout translation for remapped addresses
  addressRemapper.io.input <> internalRegBus
  addressRemapper.io.output <> regBank.io.bus
  regBank.io.statusInputs <> io.statusInputs
  regBank.io.statisticsIn <> io.statisticsIn

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
  swapBuffer.io.swapCmdEnqueued := regBank.io.swapCmdEnqueued

  regBank.io.swapDisplayedBuffer := swapBuffer.io.swapCount
  regBank.io.swapsPending := swapBuffer.io.swapsPending
  io.swapDisplayedBuffer := swapBuffer.io.swapCount
  io.swapsPending := swapBuffer.io.swapsPending

  // ========================================================================
  // Draw buffer selection (double-buffering)
  // ========================================================================
  // fbiInit2[20:11] = buffer offset in 4KB units
  val bufferOffsetBytes =
    regBank.init.fbiInit2_bufferOffset.resize(c.addressWidth.value bits) |<< 12
  val buffer0Base = io.fbBaseAddr
  val buffer1Base = io.fbBaseAddr + bufferOffsetBytes
  val frontBufferBase = swapBuffer.io.swapCount(0) ? buffer1Base | buffer0Base
  val backBufferBase = swapBuffer.io.swapCount(0) ? buffer0Base | buffer1Base

  // Triangle/fastfill draw target: fbzMode[15:14] drawBuffer (0=front, 1=back)
  val drawBufferBase = regBank.renderConfig.fbzMode.drawBuffer(0) ? backBufferBase | frontBufferBase

  // LFB buffer bases: lfbMode[5:4] writeBufferSelect, lfbMode[7:6] readBufferSelect
  val lfbWriteBufferBase =
    regBank.renderConfig.lfbMode.writeBufferSelect(0) ? backBufferBase | frontBufferBase
  val lfbReadBufferBase =
    regBank.renderConfig.lfbMode.readBufferSelect(0) ? backBufferBase | frontBufferBase

  // ========================================================================
  // Triangle Command Handling
  // ========================================================================
  // Both triangleCMD and FtriangleCMD read from the same integer registers.
  // Float addresses (0x088-0x0FC) are converted to fixed-point at FIFO input time
  // by BmbBusInterface, so both commands see identical fixed-point data.

  // Data-driven gradient capture: zips (start, dX, dY) Bits triplets with GradientBundle.all
  def captureGradientsFrom(
      sources: Seq[(Bits, Bits, Bits)]
  ): Rasterizer.GradientBundle[Rasterizer.InputGradient] = {
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)
    grads.all.zip(sources).foreach { case (grad, (start, dx, dy)) =>
      grad.start.raw := start
      grad.d(0).raw := dx
      grad.d(1).raw := dy
    }
    grads
  }

  // Capture gradients from integer registers at command time
  def captureGradients(): Rasterizer.GradientBundle[Rasterizer.InputGradient] = {
    val g = regBank.triangleGeometry
    captureGradientsFrom(
      Seq(
        (g.startR.asBits, g.dRdX.asBits, g.dRdY.asBits), // red   (12.12)
        (g.startG.asBits, g.dGdX.asBits, g.dGdY.asBits), // green (12.12)
        (g.startB.asBits, g.dBdX.asBits, g.dBdY.asBits), // blue  (12.12)
        (g.startZ.asBits, g.dZdX.asBits, g.dZdY.asBits), // depth (20.12)
        (g.startA.asBits, g.dAdX.asBits, g.dAdY.asBits), // alpha (12.12)
        (g.startW.asBits, g.dWdX.asBits, g.dWdY.asBits), // W     (2.30)
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
    cfg.tmudSdX.raw := g.dSdX.asBits
    cfg.tmudTdX.raw := g.dTdX.asBits
    cfg.tmudSdY.raw := g.dSdY.asBits
    cfg.tmudTdY.raw := g.dTdY.asBits

    // Constant colors (captured per-triangle to avoid pipelineBusy gap)
    cfg.color0 := regBank.renderConfig.color0
    cfg.color1 := regBank.renderConfig.color1
    cfg.fogColor := regBank.renderConfig.fogColor

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
  def buildTriangleInput(signBit: Bool): TriangleSetup.Input = {
    val out = TriangleSetup.Input(c)
    out.triWithSign.tri(0)(0).raw := regBank.triangleGeometry.vertexAx.asBits
    out.triWithSign.tri(0)(1).raw := regBank.triangleGeometry.vertexAy.asBits
    out.triWithSign.tri(1)(0).raw := regBank.triangleGeometry.vertexBx.asBits
    out.triWithSign.tri(1)(1).raw := regBank.triangleGeometry.vertexBy.asBits
    out.triWithSign.tri(2)(0).raw := regBank.triangleGeometry.vertexCx.asBits
    out.triWithSign.tri(2)(1).raw := regBank.triangleGeometry.vertexCy.asBits
    out.triWithSign.signBit := signBit
    out.grads := captureGradients()
    out.config := capturePerTriangleConfig()
    out
  }

  val triangleCmdPath = regBank.commands.triangleCmd.translateWith {
    buildTriangleInput(regBank.commands.triangleSignBit)
  }

  val ftriangleCmdPath = regBank.commands.ftriangleCmd.translateWith {
    buildTriangleInput(regBank.commands.ftriangleSignBit)
  }

  // Merge integer and float triangle paths
  // Use assumeOhInput since only one path is active at a time (no arbitration needed)
  triangleSetup.i << StreamArbiterFactory.assumeOhInput.on(
    Seq(triangleCmdPath, ftriangleCmdPath)
  )

  // Make triangle streams accessible for simulation monitoring
  triangleCmdPath.simPublic()
  triangleSetup.i.simPublic()
  triangleSetup.o.simPublic()
  rasterizer.i.simPublic()
  rasterizer.o.simPublic()
  write.o.fbWrite.simPublic()

  // Make TMU and ColorCombine streams accessible for stall monitoring
  tmu.io.input.simPublic()
  tmu.io.output.simPublic()
  colorCombine.io.input.simPublic()
  colorCombine.io.output.simPublic()
  write.i.fromPipeline.simPublic()

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
  lfb.io.fbWriteBaseAddr := lfbWriteBufferBase
  lfb.io.fbReadBaseAddr := lfbReadBufferBase

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

  // Fork rasterizer output: one to TMU, one to a queue for synchronization
  val rasterFork = StreamFork2(rasterYTransformed, synchronous = true)

  // Queue to hold rasterizer data while TMU processes
  // Depth must be >= TMU's maximum in-flight transactions
  val tmuGradQueue = rasterFork._2.queue(4)

  // Connect fork path 1 to TMU
  tmu.io.input.translateFrom(rasterFork._1) { (out, in) =>
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
    out.config.ncc := in.config.ncc
    // Texture coordinate gradients for LOD calculation (from config, captured at command time)
    out.dSdX := in.config.tmudSdX
    out.dTdX := in.config.tmudTdX
    out.dSdY := in.config.tmudSdY
    out.dTdY := in.config.tmudTdY
  }

  // Join TMU output with queued gradients
  val tmuJoined = StreamJoin(tmu.io.output, tmuGradQueue)

  // Connect TMU joined output to ColorCombine
  colorCombine.io.input.translateFrom(tmuJoined) { (out, payload) =>
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
    val alphaSat = rasterOut.grads.alphaGrad.sat(satMax = 255, satMin = 0, exp = 0 exp)

    // Extract integer part (upper 8 bits) from saturated values
    // Color format after sat is UQ(8,12) - 8 integer bits, 12 fractional bits = 20 total bits
    out.iterated.r := (redSat.asBits >> 12).asUInt.resize(8 bits)
    out.iterated.g := (greenSat.asBits >> 12).asUInt.resize(8 bits)
    out.iterated.b := (blueSat.asBits >> 12).asUInt.resize(8 bits)
    out.iteratedAlpha := (alphaSat.asBits >> 12).asUInt.resize(8 bits)

    // Upper 8 bits of Z for alpha local select option (Z is 20.12)
    // Saturate to [0,255] at exp=12 (so bits [19:12] become [7:0])
    // Then extract the integer part
    val depthSat = rasterOut.grads.depthGrad.sat(satMax = 255, satMin = 0, exp = 12 exp)
    out.iteratedZ := (depthSat.asBits >> 12).asUInt.resize(8 bits)

    // Pass through depth for later stages
    out.depth := rasterOut.grads.depthGrad

    // Pass through raw W value for fog wDepth calculation (SQ(32,30) → raw 32-bit SInt)
    out.rawW := rasterOut.grads.wGrad.asSInt

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

    // Decode configuration from captured per-triangle fbzColorPath
    out.config := decodeColorCombineConfig(rasterOut.config.fbzColorPath)

    // Per-triangle FIFO registers for downstream stages (alpha test, fog, framebuffer access)
    out.alphaMode := rasterOut.config.alphaMode
    out.fogMode := rasterOut.config.fogMode
    out.fbzMode := rasterOut.config.fbzMode
  }

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
  val ckBits = regBank.renderConfig.chromaKey
  val ckR = ckBits(23 downto 16).asUInt
  val ckG = ckBits(15 downto 8).asUInt
  val ckB = ckBits(7 downto 0).asUInt
  val ccColor = colorCombine.io.output.payload.color
  val chromaKill = colorCombine.io.output.payload.fbzMode.enableChromaKey &&
    ccColor.r === ckR && ccColor.g === ckG && ccColor.b === ckB
  val afterChromaKey = colorCombine.io.output.throwWhen(chromaKill)

  val fog = Fog(c)
  fog.io.fogTable := fogTableVec
  // Arbiter: CC output (after chromakey, priority) and LFB pipeline output feed into Fog
  fog.io.input << StreamArbiterFactory.lowerFirst.on(
    Seq(afterChromaKey, lfb.io.pipelineOutput)
  )

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
  val afterAlphaTest = fog.io.output.throwWhen(alphaKill)

  // ========================================================================
  // Framebuffer Access (depth test + alpha blend)
  // ========================================================================
  val fbAccess = FramebufferAccess(c)
  fbAccess.io.input << afterAlphaTest
  fbAccess.io.fbBaseAddr := drawBufferBase

  // fbzMode fields are now per-pixel (carried through pipeline in Fog.Output.fbzMode)
  // alphaMode blend fields are now per-pixel (carried through pipeline in Fog.Output.alphaMode)
  fbAccess.io.zaColor := regBank.renderConfig.zaColor

  // ========================================================================
  // Triangle → Write path (from FramebufferAccess output)
  // ========================================================================
  val triangleWriteInput = fbAccess.io.output.translateWith {
    val in = fbAccess.io.output.payload
    val out = Write.Input(c)
    out.coords := in.coords

    val fbWord = cloneOf(out.toFb)

    // Dither 8-bit RGB to 5-6-5 format
    val triDither = Dither()
    triDither.io.r := in.color.r
    triDither.io.g := in.color.g
    triDither.io.b := in.color.b
    triDither.io.x := in.coords(0).asUInt.resize(2 bits)
    triDither.io.y := in.coords(1).asUInt.resize(2 bits)
    triDither.io.enable := in.enableDithering
    triDither.io.use2x2 := in.ditherAlgorithm

    fbWord.color.r := triDither.io.ditR
    fbWord.color.g := triDither.io.ditG
    fbWord.color.b := triDither.io.ditB

    // Depth or alpha planes: use newDepth from FramebufferAccess (already computed)
    fbWord.depthAlpha := (in.enableAlphaPlanes ? in.alpha.resize(16 bits) | in.newDepth).asBits

    out.toFb := fbWord
    out.rgbWrite := in.rgbWrite
    out.auxWrite := in.auxWrite
    out.fbBaseAddr := drawBufferBase
    out
  }

  // ========================================================================
  // Fastfill → Write path (direct color1/zaColor with dithering)
  // ========================================================================
  val fastfillWrite = FastfillWrite(c)
  fastfillWrite.io.pixels << fastfill.o
  fastfillWrite.io.cmdFire := regBank.commands.fastfillCmd.fire
  fastfillWrite.io.running := fastfill.running
  fastfillWrite.io.regs.color1 := regBank.renderConfig.color1
  fastfillWrite.io.regs.zaColor := regBank.renderConfig.zaColor
  fastfillWrite.io.regs.fbzMode := regBank.renderConfig.fbzModeBundle
  fastfillWrite.io.regs.drawBufferBase := drawBufferBase
  fastfillWrite.io.regs.yOriginSwapValue := yOriginSwapValue

  // Merge fastfill, triangle, and LFB paths — lowerFirst gives fastfill priority (index 0)
  write.i.fromPipeline << StreamArbiterFactory.lowerFirst.on(
    Seq(fastfillWrite.io.output, triangleWriteInput, lfb.io.writeOutput)
  )

  // ========================================================================
  // Framebuffer arbiter (3 inputs → fbMem)
  // ========================================================================
  val fbArbiter = BmbArbiter(
    inputsParameter = Seq(
      Write.baseBmbParams(c),
      FramebufferAccess.bmbParams(c),
      Lfb.fbReadBmbParams(c)
    ),
    outputParameter = Core.fbMemBmbParams(c),
    lowerFirstPriority = true // fbWrite gets priority
  )

  // Pipe fbWrite cmd.ready to break combinational loop: Core's fbAccess has
  // a combinational path from fbWrite.cmd.ready -> fbRead.rsp.ready, which
  // loops back through the arbiter and BmbOnChipRam's !rsp.isStall gating.
  fbArbiter.io.inputs(0).cmd << write.o.fbWrite.cmd.s2mPipe()
  fbArbiter.io.inputs(0).rsp >> write.o.fbWrite.rsp

  // Buffer fbRead response to prevent deadlock: FramebufferAccess does
  // read-then-write (fbRead -> pipeline -> fbWrite). With single-ported RAM,
  // the read response blocks the RAM until consumed, but consumption requires
  // the write to complete (backpressure through the pipeline). The s2mPipe
  // decouples the response handshake, freeing the RAM for the write.
  fbArbiter.io.inputs(1).cmd << fbAccess.io.fbRead.cmd
  fbAccess.io.fbRead.rsp << fbArbiter.io.inputs(1).rsp.s2mPipe()

  fbArbiter.io.inputs(2) <> lfb.io.fbReadBus
  fbArbiter.io.output <> io.fbMem

  // ========================================================================
  // Texture arbiter (2 inputs → texMem)
  // ========================================================================
  val texArbiter = BmbArbiter(
    inputsParameter = Seq(
      Tmu.bmbParams(c),
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

  texArbiter.io.inputs(1) <> cpuTexBus
  texArbiter.io.output <> io.texMem

  // Pipeline busy signal: any stage has valid data (placed after all stages instantiated)
  regBank.io.pipelineBusy.simPublic()
  triangleSetup.o.valid.simPublic()
  rasterizer.o.valid.simPublic()
  tmu.io.input.valid.simPublic()
  tmu.io.busy.simPublic()
  fbAccess.io.busy.simPublic()
  colorCombine.io.input.valid.simPublic()
  fog.io.input.valid.simPublic()
  fbAccess.io.input.valid.simPublic()
  write.i.fromPipeline.valid.simPublic()
  fastfill.running.simPublic()
  swapBuffer.io.waiting.simPublic()
  lfb.io.busy.simPublic()
  // Pipeline busy: any stage active.
  // rasterizer.running covers the gap between accepting a triangle and producing
  // first output pixel (rasterizer.o.valid only asserts once iteration begins).
  // This matches SST-1 hardware where SST_BUSY reflects actual pipeline activity.
  rasterizer.running.simPublic()
  val pipelineBusySignal =
    triangleSetup.o.valid || rasterizer.running || tmu.io.input.valid ||
      tmu.io.busy || fbAccess.io.busy ||
      colorCombine.io.input.valid || fog.io.input.valid || fbAccess.io.input.valid ||
      write.i.fromPipeline.valid || fastfill.running || swapBuffer.io.waiting || lfb.io.busy
  regBank.io.pipelineBusy := pipelineBusySignal
}
