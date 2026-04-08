package voodoo.frontend

import voodoo._
import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.regif._
import spinal.lib.bus.misc.SizeMapping
import voodoo.core.{CoreDebug, CoreStats}
import voodoo.bus.{BmbBusInterface, FloatShadowWrite, PciFifo, RegisterCategory}
import voodoo.texture.Tmu

/** Voodoo Graphics Register Bank
  *
  * Register organization:
  *   - Status Area: Status register with hardware inputs
  *   - Triangle Geometry Area: Vertex coordinates, start values, and gradients
  *   - Command Area: Command streams with backpressure
  *   - Render Config Area: Rendering modes, clipping, and color constants
  *   - Statistics Area: Read-only performance counters
  *   - Fog Table Area: 64-entry fog lookup table
  *   - Initialization Area: Display timing and hardware configuration
  *
  * @param config
  *   Voodoo hardware configuration
  */
case class RegisterBank(config: Config) extends Component {
  val io = new Bundle {
    // BMB bus interface for register access
    val bus = slave(Bmb(RegisterBank.bmbParams(config)))

    // Hardware inputs for status register (read-only fields)
    val statusInputs = in(new Bundle {
      val vRetrace = Bool()
      val memFifoFree = UInt(16 bits)
      val pciInterrupt = Bool()
    })

    // SwapBuffer-driven status fields (outputs from SwapBuffer component)
    val swapDisplayedBuffer = in UInt (2 bits)
    val swapsPending = in UInt (3 bits)

    // Statistics counter inputs (hardware → register bank, read-only)
    val statistics = in(CoreStats())

    // Pipeline sync pulse - asserted when Sync=Yes register written
    val syncPulse = out Bool ()

    val debug = in(CoreDebug())

    // PciFifo signals (replaces internal busif FIFO signals)
    val pciFifoEmpty = in Bool ()
    val pciFifoFree = in UInt (7 bits)
    val swapCmdEnqueued = in Bool () // from PciFifo wasEnqueued
    val syncDrained = in Bool () // Sync=Yes register was drained

    // FIFO empty signal - True when PCI FIFO has no pending commands
    val fifoEmpty = out Bool ()

    // Command stream ready signals (exposed for PciFifo drain blocking)
    // One per command register, sorted by address to match PciFifo.commandAddresses ordering
    val commandReady = out Vec (Bool(), 5)

    val floatShadow = in(Flow(FloatShadowWrite(RegisterBank.bmbParams(config).access.addressWidth)))
    val drawRouting = in(FbRouting(config))
    val paletteWrite = master(Flow(RegisterBank.PaletteWrite()))
  }

  io.paletteWrite.valid := False
  io.paletteWrite.payload.address := 0
  io.paletteWrite.payload.data := 0

  var texTablesRegOpt: Option[TexLayoutTables.Tables] = None
  var triangleTexTablesRegOpt: Option[TexLayoutTables.Tables] = None

  // Create BMB bus interface for RegIf - shared across all Areas
  // Address remapping (for bit 21 set) is handled by AddressRemapper before this
  implicit val moduleName: spinal.lib.bus.regif.ClassName =
    spinal.lib.bus.regif.ClassName("RegisterBank")
  val busif = BmbBusInterface(io.bus, SizeMapping(0x000, 4 KiB), "VDO")
  import busif.FieldFloatAlias // Bring .withFloatAlias() implicit into scope

  private val triangleDrawTraceId = if (config.trace.enabled) Reg(UInt(32 bits)) init (0) else null
  private val trianglePrimitiveTraceId =
    if (config.trace.enabled) Reg(UInt(32 bits)) init (0) else null

  private val floatShadowStartS = Reg(SInt(60 bits)) init (0)
  private val floatShadowStartT = Reg(SInt(60 bits)) init (0)
  private val floatShadowStartW = Reg(SInt(60 bits)) init (0)
  private val floatShadowDSdX = Reg(SInt(60 bits)) init (0)
  private val floatShadowDTdX = Reg(SInt(60 bits)) init (0)
  private val floatShadowDWdX = Reg(SInt(60 bits)) init (0)
  private val floatShadowDSdY = Reg(SInt(60 bits)) init (0)
  private val floatShadowDTdY = Reg(SInt(60 bits)) init (0)
  private val floatShadowDWdY = Reg(SInt(60 bits)) init (0)
  private val floatShadowStartA = Reg(SInt(60 bits)) init (0)
  private val floatShadowDAdX = Reg(SInt(60 bits)) init (0)
  private val floatShadowDAdY = Reg(SInt(60 bits)) init (0)

  private def setFloatShadowRaw(address: UInt, raw: SInt): Unit = {
    switch(address) {
      is(U(0x030, 12 bits)) { floatShadowStartA := raw.resize(60 bits) }
      is(U(0x034, 12 bits)) { floatShadowStartS := raw.resize(60 bits) }
      is(U(0x038, 12 bits)) { floatShadowStartT := raw.resize(60 bits) }
      is(U(0x03c, 12 bits)) { floatShadowStartW := raw.resize(60 bits) }
      is(U(0x050, 12 bits)) { floatShadowDAdX := raw.resize(60 bits) }
      is(U(0x054, 12 bits)) { floatShadowDSdX := raw.resize(60 bits) }
      is(U(0x058, 12 bits)) { floatShadowDTdX := raw.resize(60 bits) }
      is(U(0x05c, 12 bits)) { floatShadowDWdX := raw.resize(60 bits) }
      is(U(0x070, 12 bits)) { floatShadowDAdY := raw.resize(60 bits) }
      is(U(0x074, 12 bits)) { floatShadowDSdY := raw.resize(60 bits) }
      is(U(0x078, 12 bits)) { floatShadowDTdY := raw.resize(60 bits) }
      is(U(0x07c, 12 bits)) { floatShadowDWdY := raw.resize(60 bits) }
    }
  }

  when(io.floatShadow.valid) {
    setFloatShadowRaw(io.floatShadow.address, io.floatShadow.raw)
  }.elsewhen(io.bus.cmd.fire && io.bus.cmd.opcode === Bmb.Cmd.Opcode.WRITE) {
    val paletteRegHit = io.bus.cmd.address.mux(
      U(0x334, 12 bits) -> True,
      U(0x338, 12 bits) -> True,
      U(0x33c, 12 bits) -> True,
      U(0x340, 12 bits) -> True,
      U(0x344, 12 bits) -> True,
      U(0x348, 12 bits) -> True,
      U(0x34c, 12 bits) -> True,
      U(0x350, 12 bits) -> True,
      default -> False
    )
    when(paletteRegHit && io.bus.cmd.data(31)) {
      val evenEntry = io.bus.cmd.data(30 downto 23).asUInt & U(0xfe, 8 bits)
      val isOddReg = io.bus.cmd.address === U(0x338, 12 bits) ||
        io.bus.cmd.address === U(0x340, 12 bits) ||
        io.bus.cmd.address === U(0x348, 12 bits) ||
        io.bus.cmd.address === U(0x350, 12 bits)
      io.paletteWrite.valid := True
      io.paletteWrite.payload.address := (evenEntry | isOddReg.asUInt.resize(8 bits)).resized
      io.paletteWrite.payload.data := io.bus.cmd.data(23 downto 0)
    }
    switch(io.bus.cmd.address) {
      is(U(0x030, 12 bits)) { floatShadowStartA := (io.bus.cmd.data.asSInt.resize(60 bits) |<< 18) }
      is(U(0x034, 12 bits)) { floatShadowStartS := (io.bus.cmd.data.asSInt.resize(60 bits) |<< 12) }
      is(U(0x038, 12 bits)) { floatShadowStartT := (io.bus.cmd.data.asSInt.resize(60 bits) |<< 12) }
      is(U(0x03c, 12 bits)) { floatShadowStartW := io.bus.cmd.data.asSInt.resize(60 bits) }
      is(U(0x050, 12 bits)) { floatShadowDAdX := (io.bus.cmd.data.asSInt.resize(60 bits) |<< 18) }
      is(U(0x054, 12 bits)) { floatShadowDSdX := (io.bus.cmd.data.asSInt.resize(60 bits) |<< 12) }
      is(U(0x058, 12 bits)) { floatShadowDTdX := (io.bus.cmd.data.asSInt.resize(60 bits) |<< 12) }
      is(U(0x05c, 12 bits)) { floatShadowDWdX := io.bus.cmd.data.asSInt.resize(60 bits) }
      is(U(0x070, 12 bits)) { floatShadowDAdY := (io.bus.cmd.data.asSInt.resize(60 bits) |<< 18) }
      is(U(0x074, 12 bits)) { floatShadowDSdY := (io.bus.cmd.data.asSInt.resize(60 bits) |<< 12) }
      is(U(0x078, 12 bits)) { floatShadowDTdY := (io.bus.cmd.data.asSInt.resize(60 bits) |<< 12) }
      is(U(0x07c, 12 bits)) { floatShadowDWdY := io.bus.cmd.data.asSInt.resize(60 bits) }
    }
  }

  // Track sync pulse - from PciFifo syncDrained signal
  val syncPulse = Reg(Bool()) init (False)
  io.syncPulse := syncPulse
  syncPulse := io.syncDrained

  // Wire FIFO empty status from PciFifo
  io.fifoEmpty := io.pciFifoEmpty

  // ========================================================================
  // Status Register (0x000)
  // ========================================================================
  val status = new Area {
    val reg = busif.newRegAt(0x000, "status")
    val pciFifoFree = reg.field(UInt(6 bits), AccessType.RO, 0x3f, "PCI FIFO freespace")
    pciFifoFree := io.pciFifoFree.sat(1) // Saturate 7-bit (0-64) to 6-bit (0-63)

    val vRetrace = reg.fieldAt(6, Bool(), AccessType.RO, 0, "Vertical retrace")
    vRetrace := io.statusInputs.vRetrace

    val fbiBusy = reg.fieldAt(7, Bool(), AccessType.RO, 0, "FBI graphics engine busy")
    fbiBusy := io.debug.pipelineBusy

    val trexBusy = reg.fieldAt(8, Bool(), AccessType.RO, 0, "TREX busy")
    trexBusy := io.debug.pipelineBusy

    val sstBusy = reg.fieldAt(9, Bool(), AccessType.RO, 0, "SST-1 busy")
    sstBusy := io.debug.pipelineBusy

    val displayedBuffer = reg.fieldAt(10, UInt(2 bits), AccessType.RO, 0, "Displayed buffer")
    displayedBuffer := io.swapDisplayedBuffer

    val memFifoFree = reg.fieldAt(12, UInt(16 bits), AccessType.RO, 0xffff, "Memory FIFO freespace")
    memFifoFree := io.statusInputs.memFifoFree

    val swapsPending = reg.fieldAt(28, UInt(3 bits), AccessType.RO, 0, "Swap buffers pending")
    swapsPending := io.swapsPending

    val pciInterrupt = reg.fieldAt(31, Bool(), AccessType.RO, 0, "PCI interrupt generated")
    pciInterrupt := io.statusInputs.pciInterrupt
  }

  // ========================================================================
  // Triangle Geometry (0x008-0x07C)
  // ========================================================================
  val triangleGeometry = new Area {
    private def fifoNoSyncFloatReg(
        addr: Int,
        name: String,
        proto: AFix,
        resetValue: BigInt,
        doc: String
    ): AFix =
      busif
        .newRegAtWithCategory(addr, name, RegisterCategory.fifoNoSync)
        .field(proto, AccessType.WO, resetValue, doc)
        .withFloatAlias()
        .asOutput()

    // Triangle geometry registers are FIFO=Yes, Sync=No
    // Each register has a float alias at addr+0x080 that converts IEEE754→fixed at write time
    val vertices = new Area {
      val vertexAx =
        fifoNoSyncFloatReg(0x008, "vertexAx", AFix(config.vertexFormat), 0, "Vertex A X coordinate")
      val vertexAy =
        fifoNoSyncFloatReg(0x00c, "vertexAy", AFix(config.vertexFormat), 0, "Vertex A Y coordinate")
      val vertexBx =
        fifoNoSyncFloatReg(0x010, "vertexBx", AFix(config.vertexFormat), 0, "Vertex B X coordinate")
      val vertexBy =
        fifoNoSyncFloatReg(0x014, "vertexBy", AFix(config.vertexFormat), 0, "Vertex B Y coordinate")
      val vertexCx =
        fifoNoSyncFloatReg(0x018, "vertexCx", AFix(config.vertexFormat), 0, "Vertex C X coordinate")
      val vertexCy =
        fifoNoSyncFloatReg(0x01c, "vertexCy", AFix(config.vertexFormat), 0, "Vertex C Y coordinate")
    }

    val startValues = new Area {
      val startR = fifoNoSyncFloatReg(
        0x020,
        "startR",
        AFix(config.vColorFormat),
        255 << 12,
        "Starting red value (12.12 fixed)"
      )
      val startG = fifoNoSyncFloatReg(
        0x024,
        "startG",
        AFix(config.vColorFormat),
        255 << 12,
        "Starting green value (12.12 fixed)"
      )
      val startB = fifoNoSyncFloatReg(
        0x028,
        "startB",
        AFix(config.vColorFormat),
        255 << 12,
        "Starting blue value (12.12 fixed)"
      )
      val startZ = fifoNoSyncFloatReg(
        0x02c,
        "startZ",
        AFix(config.vDepthFormat),
        0,
        "Starting Z depth (20.12 fixed)"
      )
      val startA = fifoNoSyncFloatReg(
        0x030,
        "startA",
        AFix(config.vColorFormat),
        0,
        "Starting alpha value (12.12 fixed)"
      )
      val startS = fifoNoSyncFloatReg(
        0x034,
        "startS",
        AFix(config.texCoordsFormat),
        0,
        "Starting S texture coord (14.18 fixed)"
      )
      val startT = fifoNoSyncFloatReg(
        0x038,
        "startT",
        AFix(config.texCoordsFormat),
        0,
        "Starting T texture coord (14.18 fixed)"
      )
      val startW = fifoNoSyncFloatReg(
        0x03c,
        "startW",
        AFix(config.wFormat),
        0,
        "Starting W value (2.30 fixed)"
      )
    }

    val gradX = new Area {
      val dRdX = fifoNoSyncFloatReg(
        0x040,
        "dRdX",
        AFix(config.vColorFormat),
        0,
        "Red gradient dR/dX (12.12 fixed)"
      )
      val dGdX = fifoNoSyncFloatReg(
        0x044,
        "dGdX",
        AFix(config.vColorFormat),
        0,
        "Green gradient dG/dX (12.12 fixed)"
      )
      val dBdX = fifoNoSyncFloatReg(
        0x048,
        "dBdX",
        AFix(config.vColorFormat),
        0,
        "Blue gradient dB/dX (12.12 fixed)"
      )
      val dZdX = fifoNoSyncFloatReg(
        0x04c,
        "dZdX",
        AFix(config.vDepthFormat),
        0,
        "Z gradient dZ/dX (20.12 fixed)"
      )
      val dAdX = fifoNoSyncFloatReg(
        0x050,
        "dAdX",
        AFix(config.vColorFormat),
        0,
        "Alpha gradient dA/dX (12.12 fixed)"
      )
      val dSdX = fifoNoSyncFloatReg(
        0x054,
        "dSdX",
        AFix(config.texCoordsFormat),
        0,
        "S texture gradient dS/dX (14.18 fixed)"
      )
      val dTdX = fifoNoSyncFloatReg(
        0x058,
        "dTdX",
        AFix(config.texCoordsFormat),
        0,
        "T texture gradient dT/dX (14.18 fixed)"
      )
      val dWdX =
        fifoNoSyncFloatReg(0x05c, "dWdX", AFix(config.wFormat), 0, "W gradient dW/dX (2.30 fixed)")
    }

    val gradY = new Area {
      val dRdY = fifoNoSyncFloatReg(
        0x060,
        "dRdY",
        AFix(config.vColorFormat),
        0,
        "Red gradient dR/dY (12.12 fixed)"
      )
      val dGdY = fifoNoSyncFloatReg(
        0x064,
        "dGdY",
        AFix(config.vColorFormat),
        0,
        "Green gradient dG/dY (12.12 fixed)"
      )
      val dBdY = fifoNoSyncFloatReg(
        0x068,
        "dBdY",
        AFix(config.vColorFormat),
        0,
        "Blue gradient dB/dY (12.12 fixed)"
      )
      val dZdY = fifoNoSyncFloatReg(
        0x06c,
        "dZdY",
        AFix(config.vDepthFormat),
        0,
        "Z gradient dZ/dY (20.12 fixed)"
      )
      val dAdY = fifoNoSyncFloatReg(
        0x070,
        "dAdY",
        AFix(config.vColorFormat),
        0,
        "Alpha gradient dA/dY (12.12 fixed)"
      )
      val dSdY = fifoNoSyncFloatReg(
        0x074,
        "dSdY",
        AFix(config.texCoordsFormat),
        0,
        "S texture gradient dS/dY (14.18 fixed)"
      )
      val dTdY = fifoNoSyncFloatReg(
        0x078,
        "dTdY",
        AFix(config.texCoordsFormat),
        0,
        "T texture gradient dT/dY (14.18 fixed)"
      )
      val dWdY =
        fifoNoSyncFloatReg(0x07c, "dWdY", AFix(config.wFormat), 0, "W gradient dW/dY (2.30 fixed)")
    }

    val vertexAx = vertices.vertexAx
    val vertexAy = vertices.vertexAy
    val vertexBx = vertices.vertexBx
    val vertexBy = vertices.vertexBy
    val vertexCx = vertices.vertexCx
    val vertexCy = vertices.vertexCy
    val startR = startValues.startR
    val startG = startValues.startG
    val startB = startValues.startB
    val startZ = startValues.startZ
    val startA = startValues.startA
    val startS = startValues.startS
    val startT = startValues.startT
    val startW = startValues.startW
    val dRdX = gradX.dRdX
    val dGdX = gradX.dGdX
    val dBdX = gradX.dBdX
    val dZdX = gradX.dZdX
    val dAdX = gradX.dAdX
    val dSdX = gradX.dSdX
    val dTdX = gradX.dTdX
    val dWdX = gradX.dWdX
    val dRdY = gradY.dRdY
    val dGdY = gradY.dGdY
    val dBdY = gradY.dBdY
    val dZdY = gradY.dZdY
    val dAdY = gradY.dAdY
    val dSdY = gradY.dSdY
    val dTdY = gradY.dTdY
    val dWdY = gradY.dWdY
  }

  private def captureTriangleGradientsFrom(
      sources: Seq[(Bits, Bits, Bits)]
  ): Rasterizer.GradientBundle[Rasterizer.InputGradient] = {
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), config)
    grads.all.zip(sources).foreach { case (grad, (start, dx, dy)) =>
      grad.start.raw := start.asSInt.resize(grad.start.raw.getWidth).asBits
      grad.d(0).raw := dx.asSInt.resize(grad.d(0).raw.getWidth).asBits
      grad.d(1).raw := dy.asSInt.resize(grad.d(1).raw.getWidth).asBits
    }
    grads
  }

  private def captureTriangleHiTexCoords(useFloatShadow: Bool): TriangleSetup.HiTexCoords = {
    val hi = TriangleSetup.HiTexCoords(config)
    val startSInt = triangleGeometry.startS.raw.asSInt.resize(60) |<< 12
    val startTInt = triangleGeometry.startT.raw.asSInt.resize(60) |<< 12
    val dSdXInt = triangleGeometry.dSdX.raw.asSInt.resize(60) |<< 12
    val dTdXInt = triangleGeometry.dTdX.raw.asSInt.resize(60) |<< 12
    val dSdYInt = triangleGeometry.dSdY.raw.asSInt.resize(60) |<< 12
    val dTdYInt = triangleGeometry.dTdY.raw.asSInt.resize(60) |<< 12

    hi.sStart.raw := Mux(
      useFloatShadow,
      floatShadowStartS.asBits,
      startSInt.asBits
    )
    hi.tStart.raw := Mux(
      useFloatShadow,
      floatShadowStartT.asBits,
      startTInt.asBits
    )
    hi.dSdX.raw := Mux(useFloatShadow, floatShadowDSdX.asBits, dSdXInt.asBits)
    hi.dTdX.raw := Mux(useFloatShadow, floatShadowDTdX.asBits, dTdXInt.asBits)
    hi.dSdY.raw := Mux(useFloatShadow, floatShadowDSdY.asBits, dSdYInt.asBits)
    hi.dTdY.raw := Mux(useFloatShadow, floatShadowDTdY.asBits, dTdYInt.asBits)
    hi
  }

  private def captureTriangleHiAlpha(useFloatShadow: Bool): TriangleSetup.HiAlpha = {
    val hi = TriangleSetup.HiAlpha(config)
    val startAInt = triangleGeometry.startA.raw.asSInt.resize(60) |<< 18
    val dAdXInt = triangleGeometry.dAdX.raw.asSInt.resize(60) |<< 18
    val dAdYInt = triangleGeometry.dAdY.raw.asSInt.resize(60) |<< 18

    hi.start.raw := Mux(
      useFloatShadow,
      floatShadowStartA.asBits,
      startAInt.asBits
    )
    hi.dAdX.raw := Mux(useFloatShadow, floatShadowDAdX.asBits, dAdXInt.asBits)
    hi.dAdY.raw := Mux(useFloatShadow, floatShadowDAdY.asBits, dAdYInt.asBits)
    hi
  }

  private def captureTriangleGradients(
      useFloatShadow: Bool
  ): Rasterizer.GradientBundle[Rasterizer.InputGradient] = {
    val g = triangleGeometry
    captureTriangleGradientsFrom(
      Seq(
        (g.startR.asBits, g.dRdX.asBits, g.dRdY.asBits),
        (g.startG.asBits, g.dGdX.asBits, g.dGdY.asBits),
        (g.startB.asBits, g.dBdX.asBits, g.dBdY.asBits),
        (g.startZ.asBits, g.dZdX.asBits, g.dZdY.asBits),
        (g.startA.asBits, g.dAdX.asBits, g.dAdY.asBits),
        (
          Mux(
            useFloatShadow,
            floatShadowStartW.asBits,
            g.startW.raw.asSInt.resize(60).asBits
          ),
          Mux(
            useFloatShadow,
            floatShadowDWdX.asBits,
            g.dWdX.raw.asSInt.resize(60).asBits
          ),
          Mux(
            useFloatShadow,
            floatShadowDWdY.asBits,
            g.dWdY.raw.asSInt.resize(60).asBits
          )
        ),
        (g.startS.asBits, g.dSdX.asBits, g.dSdY.asBits),
        (g.startT.asBits, g.dTdX.asBits, g.dTdY.asBits)
      )
    )
  }

  private def captureTriangleConfig(): TriangleSetup.PerTriangleConfig = {
    val g = triangleGeometry
    val cfg = TriangleSetup.PerTriangleConfig(config)

    cfg.fbzColorPath := renderConfig.fbzColorPathBundle
    cfg.fogMode := renderConfig.fogModeBundle
    cfg.alphaMode := renderConfig.alphaModeBundle
    cfg.enableClipping := renderConfig.fbzMode.enableClipping
    cfg.clipLeft := renderConfig.clipLeftX
    cfg.clipRight := renderConfig.clipRightX
    cfg.clipLowY := renderConfig.clipLowY
    cfg.clipHighY := renderConfig.clipHighY

    cfg.tmuTextureMode := tmuConfig.textureMode
    cfg.tmuTexBaseAddr := tmuConfig.texBaseAddr
    cfg.tmuTexBaseAddr1 := tmuConfig.texBaseAddr1
    cfg.tmuTexBaseAddr2 := tmuConfig.texBaseAddr2
    cfg.tmuTexBaseAddr38 := tmuConfig.texBaseAddr38
    cfg.tmuTLOD := tmuConfig.tLOD.resized
    cfg.tmudSdX.raw := g.dSdX.asBits
    cfg.tmudTdX.raw := g.dTdX.asBits
    cfg.tmudSdY.raw := g.dSdY.asBits
    cfg.tmudTdY.raw := g.dTdY.asBits

    if (config.packedTexLayout) {
      cfg.texTables := triangleTexTablesRegOpt.get
    }

    cfg
  }

  private def buildTriangleCommandInput(
      signBit: Bool,
      useFloatShadow: Bool
  ): TriangleSetup.Input = {
    TriangleSetup.buildInput(
      config,
      triangleGeometry.vertexAx,
      triangleGeometry.vertexAy,
      triangleGeometry.vertexBx,
      triangleGeometry.vertexBy,
      triangleGeometry.vertexCx,
      triangleGeometry.vertexCy,
      signBit,
      captureTriangleGradients(useFloatShadow),
      captureTriangleHiAlpha(useFloatShadow),
      captureTriangleHiTexCoords(useFloatShadow),
      triangleConfigReg,
      triangleDrawTraceId,
      trianglePrimitiveTraceId
    )
  }

  private def fifoRegBits(addr: Int, name: String, category: RegisterCategory, doc: String): Bits =
    busif
      .newRegAtWithCategory(addr, name, category)
      .field(Bits(32 bits), AccessType.WO, 0, doc)
      .asOutput()

  private def fogEntryPair(addr: Int, baseName: String, index: Int): Seq[(Bits, Bits)] = {
    val reg = busif.newRegAtWithCategory(addr, baseName, RegisterCategory.fifoWithSync)
    val entry0Dfog =
      reg.field(Bits(8 bits), AccessType.WO, 0, s"Fog entry ${index * 2} delta").asOutput()
    val entry0Fog =
      reg.fieldAt(8, Bits(8 bits), AccessType.WO, 0, s"Fog entry ${index * 2} value").asOutput()
    val entry1Dfog = reg
      .fieldAt(16, Bits(8 bits), AccessType.WO, 0, s"Fog entry ${index * 2 + 1} delta")
      .asOutput()
    val entry1Fog = reg
      .fieldAt(24, Bits(8 bits), AccessType.WO, 0, s"Fog entry ${index * 2 + 1} value")
      .asOutput()
    Seq((entry0Dfog, entry0Fog), (entry1Dfog, entry1Fog))
  }

  private def nccBlock(table: Int, kind: String, baseAddr: Int): Vec[Bits] =
    Vec(
      (0 until 4).map(i =>
        fifoRegBits(
          baseAddr + i * 4,
          s"nccTable${table}${kind}${i}",
          RegisterCategory.fifoWithSync,
          s"NCC table $table $kind$i"
        )
      )
    )

  val triangleConfigReg = Reg(TriangleSetup.PerTriangleConfig(config))

  // Float Triangle Geometry Area (0x088-0x0FC) is handled by float alias conversion
  // in BmbBusInterface. Writes to float addresses are automatically converted to fixed-point
  // and written to the corresponding integer registers above.

  // ========================================================================
  // Command Area (0x080-0x100)
  // Command registers with Stream outputs - FIFO queueing and backpressure handled by BusIf
  // ========================================================================
  val commands = new Area {
    val (triangleCmdReg, triangleCmdStream) =
      busif.newCommandRegWithPayload(
        0x080,
        "triangleCMD",
        RegisterCategory.fifoNoSync,
        HardType(TriangleSetup.Input(config))
      ) { (reg, payload) =>
        Component.current.addPrePopTask { () =>
          payload.assignFromBits(buildTriangleCommandInput(io.bus.cmd.data(31), False).asBits)
        }
      }
    val (ftriangleCmdReg, ftriangleCmdStream) =
      busif.newCommandRegWithPayload(
        0x100,
        "ftriangleCMD",
        RegisterCategory.fifoNoSync,
        HardType(TriangleSetup.Input(config))
      ) { (reg, payload) =>
        Component.current.addPrePopTask { () =>
          payload.assignFromBits(buildTriangleCommandInput(io.bus.cmd.data(31), True).asBits)
        }
      }
    val (nopCmdReg, nopCmdStream) =
      busif.newCommandRegWithPayload(
        0x120,
        "nopCMD",
        RegisterCategory.fifoWithSync,
        HardType(Bits(32 bits))
      ) { (_, payload) =>
        Component.current.addPrePopTask { () =>
          payload := io.bus.cmd.data.asBits
        }
      }
    val (fastfillCmdReg, fastfillCmdStream) =
      busif.newCommandReg(0x124, "fastfillCMD", RegisterCategory.fifoWithSync)
    val (swapbufferCmdReg, swapbufferCmdStream) =
      busif.newCommandReg(0x128, "swapbufferCMD", RegisterCategory.fifoWithSync)

      if (config.trace.enabled) {
        when(swapbufferCmdStream.fire) {
          triangleDrawTraceId := triangleDrawTraceId + 1
        }
        when(triangleCmdStream.valid || ftriangleCmdStream.valid) {
          trianglePrimitiveTraceId := trianglePrimitiveTraceId + 1
        }
      }

    // Define full 32-bit fields for other command registers
    nopCmdReg.field(Bits(32 bits), AccessType.RW, 0, "NOP command data")
    fastfillCmdReg.field(Bits(32 bits), AccessType.RW, 0, "Fastfill command data")
    val swapVsyncEnable = swapbufferCmdReg
      .field(Bool(), AccessType.RW, 0, "Vsync synchronization enable")
      .asOutput()
    val swapInterval = swapbufferCmdReg
      .field(UInt(8 bits), AccessType.RW, 0, "Swap interval (vsyncs to wait)")
      .asOutput()

    // swapCmdEnqueued comes from PciFifo wasEnqueued signal (via Core)
    // (io.swapCmdEnqueued is an input, wired from PciFifo in Core)

    // Expose streams for convenience (backwards compatibility)
    val triangleCmd = master(triangleCmdStream)
    val ftriangleCmd = master(ftriangleCmdStream)
    val nopCmd = master(nopCmdStream)
    val fastfillCmd = master(fastfillCmdStream)
    val swapbufferCmd = master(swapbufferCmdStream)
  }

  // Wire command stream ready signals to IO (sorted by address for deterministic ordering)
  Component.current.addPrePopTask { () =>
    val sorted = busif.getCommandStreamReady.toSeq.sortBy(_._1)
    for (((_, signal), idx) <- sorted.zipWithIndex) {
      io.commandReady(idx) := signal
    }
  }

  // ========================================================================
  // Render Configuration Area (0x104-0x148)
  // Rendering modes, clipping, and color constants
  // ========================================================================
  val renderConfig = new Area {
    // Rendering Mode Registers (0x104-0x10c) - Sync=No, FIFO=Yes
    val fbzColorPath = busif
      .newRegAtWithCategory(0x104, "fbzColorPath", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Color combine path control")
      .asOutput()

    val fogMode = busif
      .newRegAtWithCategory(0x108, "fogMode", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Fog mode control")
      .asOutput()

    val alphaMode = busif
      .newRegAtWithCategory(0x10c, "alphaMode", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Alpha test and blend control")
      .asOutput()

    // Rendering Mode Registers (0x110-0x114) - Sync=Yes, FIFO=Yes
    val fbzModeReg = busif.newRegAtWithCategory(0x110, "fbzMode", RegisterCategory.fifoWithSync)
    val fbzMode = new Area {
      val enableClipping =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable clipping rectangle").asOutput()
      val enableChromaKey =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable chroma-keying").asOutput()
      val enableStipple =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable stipple register masking").asOutput()
      val wBufferSelect = fbzModeReg
        .field(Bool(), AccessType.RW, 0, "W-Buffer Select (0=Z-value, 1=W-value)")
        .asOutput()
      val enableDepthBuffer =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable depth-buffering").asOutput()
      val depthFunction =
        fbzModeReg.field(UInt(3 bits), AccessType.RW, 0, "Depth-buffer function").asOutput()
      val enableDithering =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable dithering").asOutput()
      val rgbBufferMask =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "RGB buffer write mask").asOutput()
      val auxBufferMask =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Depth/alpha buffer write mask").asOutput()
      val ditherAlgorithm =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Dither algorithm (0=4x4, 1=2x2)").asOutput()
      val enableStipplePattern =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable Stipple pattern masking").asOutput()
      val enableAlphaMask =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable Alpha-channel mask").asOutput()
      val drawBuffer = fbzModeReg
        .field(UInt(2 bits), AccessType.RW, 0, "Draw buffer (0=Front, 1=Back, 2-3=Reserved)")
        .asOutput()
      val enableDepthBias =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable depth-biasing").asOutput()
      val yOrigin =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Y origin (0=top, 1=bottom)").asOutput()
      val enableAlphaPlanes =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable alpha planes").asOutput()
      val enableDitherSubtract = fbzModeReg
        .field(Bool(), AccessType.RW, 0, "Enable alpha-blending dither subtraction")
        .asOutput()
      val depthSourceSelect = fbzModeReg
        .field(Bool(), AccessType.RW, 0, "Depth source (0=normal, 1=zaColor[15:0])")
        .asOutput()
      val reserved = fbzModeReg.field(Bits(11 bits), AccessType.RW, 0, "Reserved").asOutput()
    }

    val lfbModeReg = busif.newRegAtWithCategory(0x114, "lfbMode", RegisterCategory.fifoWithSync)
    val lfbMode = new Area {
      val writeFormat = lfbModeReg
        .field(UInt(4 bits), AccessType.RW, 0, "Linear frame buffer write format")
        .asOutput()
      val writeBufferSelect = lfbModeReg
        .field(
          UInt(2 bits),
          AccessType.RW,
          0,
          "Write buffer select (0=front, 1=back, 2-3=reserved)"
        )
        .asOutput()
      val readBufferSelect = lfbModeReg
        .field(
          UInt(2 bits),
          AccessType.RW,
          0,
          "Read buffer select (0=front, 1=back, 2=depth/alpha, 3=reserved)"
        )
        .asOutput()
      val pixelPipelineEnable = lfbModeReg
        .field(Bool(), AccessType.RW, 0, "Enable pixel pipeline for LFB writes")
        .asOutput()
      val rgbaLanes = lfbModeReg
        .field(UInt(2 bits), AccessType.RW, 0, "RGBA lanes (0=ARGB, 1=ABGR, 2=RGBA, 3=BGRA)")
        .asOutput()
      val wordSwapWrites =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "16-bit word swap LFB writes").asOutput()
      val byteSwizzleWrites =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "Byte swizzle LFB writes").asOutput()
      val yOrigin =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "LFB Y origin (0=top, 1=bottom)").asOutput()
      val wSelect = lfbModeReg
        .field(Bool(), AccessType.RW, 0, "LFB write W select (0=LFB, 1=zaColor[15:0])")
        .asOutput()
      val wordSwapReads =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "16-bit word swap LFB reads").asOutput()
      val byteSwizzleReads =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "Byte swizzle LFB reads").asOutput()
      val reserved = lfbModeReg.field(Bits(15 bits), AccessType.RW, 0, "Reserved").asOutput()
    }

    // Clipping Registers (0x118-0x11c) - Sync=Yes, FIFO=Yes
    // Datasheet: clipLeftRight bits[9:0]=right, bits[25:16]=left
    val clipLeftRight =
      busif.newRegAtWithCategory(0x118, "clipLeftRight", RegisterCategory.fifoWithSync)
    val clipRightX =
      clipLeftRight.field(UInt(10 bits), AccessType.RW, 0x3ff, "Right clip boundary").asOutput()
    val clipLeftX = clipLeftRight
      .fieldAt(16, UInt(10 bits), AccessType.RW, 0, "Left clip boundary")
      .asOutput()

    // Datasheet: clipLowYHighY bits[9:0]=highY, bits[25:16]=lowY
    val clipLowYHighY =
      busif.newRegAtWithCategory(0x11c, "clipLowYHighY", RegisterCategory.fifoWithSync)
    val clipHighY =
      clipLowYHighY.field(UInt(10 bits), AccessType.RW, 0x3ff, "Bottom clip boundary").asOutput()
    val clipLowY = clipLowYHighY
      .fieldAt(16, UInt(10 bits), AccessType.RW, 0, "Top clip boundary")
      .asOutput()

    // Color and Constant Registers (0x12c-0x148) - Sync=Yes, FIFO=Yes
    val fogColor = busif
      .newRegAtWithCategory(0x12c, "fogColor", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.WO, 0, "RGBA fog color")
      .asOutput()

    val zaColor = busif
      .newRegAtWithCategory(0x130, "zaColor", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Z/alpha constant for fills")
      .asOutput()

    val chromaKey = busif
      .newRegAtWithCategory(0x134, "chromaKey", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Chroma key color")
      .asOutput()

    val stipple = busif
      .newRegAtWithCategory(0x140, "stipple", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Stipple pattern")
      .asOutput()

    val color0 = busif
      .newRegAtWithCategory(0x144, "color0", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Constant color 0")
      .asOutput()

    val color1 = busif
      .newRegAtWithCategory(0x148, "color1", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Constant color 1")
      .asOutput()

    // Bundle accessors — decode flat register bits into named fields

    def fbzColorPathBundle: FbzColorPath = {
      val b = FbzColorPath()
      b.assignFromBits(fbzColorPath.resized)
      b
    }

    def fogModeBundle: FogMode = {
      val b = FogMode()
      b.assignFromBits(fogMode.resized)
      b
    }

    def alphaModeBundle: AlphaMode = {
      val b = AlphaMode()
      b.assignFromBits(alphaMode)
      b
    }

    def fbzModeBundle: FbzMode = {
      val b = voodoo.FbzMode()
      b.enableClipping := fbzMode.enableClipping
      b.enableChromaKey := fbzMode.enableChromaKey
      b.enableStipple := fbzMode.enableStipple
      b.wBufferSelect := fbzMode.wBufferSelect
      b.enableDepthBuffer := fbzMode.enableDepthBuffer
      b.depthFunction := fbzMode.depthFunction
      b.enableDithering := fbzMode.enableDithering
      b.rgbBufferMask := fbzMode.rgbBufferMask
      b.auxBufferMask := fbzMode.auxBufferMask
      b.ditherAlgorithm := fbzMode.ditherAlgorithm
      b.enableStipplePattern := fbzMode.enableStipplePattern
      b.enableAlphaMask := fbzMode.enableAlphaMask
      b.drawBuffer := fbzMode.drawBuffer
      b.enableDepthBias := fbzMode.enableDepthBias
      b.yOrigin := fbzMode.yOrigin
      b.enableAlphaPlanes := fbzMode.enableAlphaPlanes
      b.enableDitherSubtract := fbzMode.enableDitherSubtract
      b.depthSourceSelect := fbzMode.depthSourceSelect
      b
    }

    def lfbModeBundle: LfbMode = {
      val b = LfbMode()
      b.writeFormat := lfbMode.writeFormat
      b.writeBufferSelect := lfbMode.writeBufferSelect
      b.readBufferSelect := lfbMode.readBufferSelect
      b.pixelPipelineEnable := lfbMode.pixelPipelineEnable
      b.rgbaLanes := lfbMode.rgbaLanes
      b.wordSwapWrites := lfbMode.wordSwapWrites
      b.byteSwizzleWrites := lfbMode.byteSwizzleWrites
      b.yOrigin := lfbMode.yOrigin
      b.wSelect := lfbMode.wSelect
      b.wordSwapReads := lfbMode.wordSwapReads
      b.byteSwizzleReads := lfbMode.byteSwizzleReads
      b
    }
  }

  // ========================================================================
  // Statistics Area (0x14c-0x15c)
  // Read-only performance counters
  // ========================================================================
  val statistics = new Area {
    val statsPixelsIn = busif
      .newRegAt(0x14c, "fbiPixelsIn")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels entering pipeline")
    statsPixelsIn := io.statistics.pixelsIn

    val statsChromaFail = busif
      .newRegAt(0x150, "fbiChromaFail")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels failed chroma key test")
    statsChromaFail := io.statistics.chromaFail

    val statsZFuncFail = busif
      .newRegAt(0x154, "fbiZfuncFail")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels failed Z test")
    statsZFuncFail := io.statistics.zFuncFail

    val statsAFuncFail = busif
      .newRegAt(0x158, "fbiAfuncFail")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels failed alpha test")
    statsAFuncFail := io.statistics.aFuncFail

    val statsPixelsOut = busif
      .newRegAt(0x15c, "fbiPixelsOut")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels written to framebuffer")
    statsPixelsOut := io.statistics.pixelsOut
  }

  // ========================================================================
  // Fog Table Area (0x160-0x1DC)
  // 64-entry fog lookup table (32 registers, 2 entries per register)
  // ========================================================================
  val fogTable = new Area {
    val fogTable = (0 until 32).flatMap(i => fogEntryPair(0x160 + i * 4, s"fogTable${i * 2}", i))
  }

  // ========================================================================
  // Initialization Area (0x200-0x24C)
  // Display timing and hardware configuration registers
  // ========================================================================
  val init = new Area {
    // All init registers bypass FIFO (FIFO=No)
    // fbiInit4 (0x200) - PCI read timing
    val fbiInit4Reg = busif.newRegAtWithCategory(0x200, "fbiInit4", RegisterCategory.bypassFifo)
    val fbiInit4_pciReadWaitStates = fbiInit4Reg
      .field(Bool(), AccessType.RW, 0, "PCI read wait states: 0=1 wait state, 1=2 wait states")
      .asOutput()

    // backPorch (0x208) - Display back porch timing
    val backPorch = busif
      .newRegAtWithCategory(0x208, "backPorch", RegisterCategory.bypassFifo)
      .field(Bits(32 bits), AccessType.WO, 0, "Display back porch timing")
      .asOutput()

    // videoDimensions (0x20C) - Display resolution
    val videoDimensionsReg =
      busif.newRegAtWithCategory(0x20c, "videoDimensions", RegisterCategory.bypassFifo)
    val hDisp = videoDimensionsReg
      .field(UInt(12 bits), AccessType.WO, 0, "Horizontal display width - 1")
      .asOutput()
    val vDisp = videoDimensionsReg
      .fieldAt(16, UInt(12 bits), AccessType.WO, 0, "Vertical display height")
      .asOutput()

    // fbiInit0 (0x210) - VGA passthrough and graphics reset
    val fbiInit0Reg = busif.newRegAtWithCategory(0x210, "fbiInit0", RegisterCategory.bypassFifo)
    val fbiInit0_vgaPassthrough = fbiInit0Reg
      .field(Bool(), AccessType.RW, 0, "VGA passthrough: 0=VGA blocked, 1=VGA passed")
      .asOutput()
    val fbiInit0_graphicsReset =
      fbiInit0Reg.fieldAt(1, Bool(), AccessType.RW, 0, "Graphics reset").asOutput()

    // fbiInit1 (0x214) - PCI timing and SLI enable
    val fbiInit1Reg = busif.newRegAtWithCategory(0x214, "fbiInit1", RegisterCategory.bypassFifo)
    val fbiInit1_pciWriteWaitStates = fbiInit1Reg
      .fieldAt(1, Bool(), AccessType.RW, 0, "PCI write wait states: 0=fast, 1=slow")
      .asOutput()
    val fbiInit1_multiSst =
      fbiInit1Reg.fieldAt(2, Bool(), AccessType.RW, 0, "Multi-SST (SLI) mode [V1]").asOutput()
    val fbiInit1_videoTilesX = fbiInit1Reg
      .fieldAt(
        4,
        UInt(4 bits),
        AccessType.RW,
        10,
        "Video tiles in X / 2 (stride = val * 64 pixels)"
      )
      .asOutput()
    val fbiInit1_videoReset =
      fbiInit1Reg.fieldAt(8, Bool(), AccessType.RW, 0, "Video timing reset").asOutput()
    // Note: yOriginSwap is in fbiInit3 (0x21C), not fbiInit1 — see below

    // fbiInit2 (0x218) - Buffer config and swap algorithm
    val fbiInit2Reg = busif.newRegAtWithCategory(0x218, "fbiInit2", RegisterCategory.bypassFifo)
    val fbiInit2_swapAlgorithm = fbiInit2Reg
      .fieldAt(
        9,
        UInt(2 bits),
        AccessType.RW,
        0,
        "Swap algorithm: 0=DAC vsync, 1=DAC data, 2=PCI FIFO stall, 3=SLI sync"
      )
      .asOutput()
    val fbiInit2_bufferOffset = fbiInit2Reg
      .fieldAt(11, UInt(9 bits), AccessType.RW, 0, "Buffer offset in 4KB units")
      .asOutput()

    // fbiInit3 (0x21C) - Register remapping and Y origin
    val fbiInit3Reg = busif.newRegAtWithCategory(0x21c, "fbiInit3", RegisterCategory.bypassFifo)
    val fbiInit3_remapEnable =
      fbiInit3Reg.field(Bool(), AccessType.RW, 0, "Enable register address remapping").asOutput()
    val fbiInit3_yOriginSwap = fbiInit3Reg
      .fieldAt(22, UInt(10 bits), AccessType.RW, 0, "Y origin swap subtraction value")
      .asOutput()

    // hSync (0x220) - Horizontal sync timing
    val hSyncReg = busif.newRegAtWithCategory(0x220, "hSync", RegisterCategory.bypassFifo)
    val hSyncOn = hSyncReg.field(UInt(8 bits), AccessType.WO, 0, "Horizontal sync start").asOutput()
    val hSyncOff =
      hSyncReg.fieldAt(16, UInt(10 bits), AccessType.WO, 0, "Horizontal sync end").asOutput()

    // vSync (0x224) - Vertical sync timing
    val vSyncReg = busif.newRegAtWithCategory(0x224, "vSync", RegisterCategory.bypassFifo)
    val vSyncOn =
      vSyncReg.field(UInt(16 bits), AccessType.WO, 0, "Vertical sync start (lines)").asOutput()
    val vSyncOff =
      vSyncReg.fieldAt(16, UInt(16 bits), AccessType.WO, 0, "Vertical sync end (lines)").asOutput()

    // maxRgbDelta (0x230) - Max RGB difference for video filtering
    val maxRgbDelta = busif
      .newRegAtWithCategory(0x230, "maxRgbDelta", RegisterCategory.bypassFifo)
      .field(Bits(32 bits), AccessType.WO, 0, "Max RGB difference for video filtering")
      .asOutput()

    // debugBusy (0x240) - Internal busy-source debug bitfield
    val debugBusy = busif
      .newRegAt(0x240, "debugBusy")
      .field(Bits(32 bits), AccessType.RO, 0, "Internal pipeline busy-source debug bitfield")
    debugBusy := io.debug.busy.bitsValue

    // fbiInit5 (0x244) - Multi-chip config
    val fbiInit5Reg = busif.newRegAtWithCategory(0x244, "fbiInit5", RegisterCategory.bypassFifo)
    val fbiInit5_multiCvg = fbiInit5Reg
      .fieldAt(14, Bool(), AccessType.RW, 0, "Multi-chip coverage (SLI for V2)")
      .asOutput()

    // fbiInit6 (0x248) - Extended init
    val fbiInit6Reg = busif.newRegAtWithCategory(0x248, "fbiInit6", RegisterCategory.bypassFifo)
    val fbiInit6_blockWidthExtend = fbiInit6Reg
      .fieldAt(30, Bool(), AccessType.RW, 0, "Extends block width calculation")
      .asOutput()

    // fbiInit7 (0x24C) - Command FIFO enable [V2+]
    val fbiInit7Reg = busif.newRegAtWithCategory(0x24c, "fbiInit7", RegisterCategory.bypassFifo)
    val fbiInit7_cmdFifoEnable =
      fbiInit7Reg.fieldAt(8, Bool(), AccessType.RW, 0, "Enable command FIFO mode [V2+]").asOutput()

    // writePathDebug (0x250) - Write-side pipeline and arbiter handshake debug
    val writePathDebugReg = busif
      .newRegAt(0x250, "writePathDebug")
      .field(
        Bits(32 bits),
        AccessType.RO,
        0,
        "Write-side pipeline and arbiter handshake debug bitfield"
      )
    writePathDebugReg := io.debug.writePath.bitsValue
  }

  // ========================================================================
  // TMU Configuration Registers (0x300-0x320)
  // Single TMU support only (Voodoo 1 level functionality)
  // These control texture format, filtering, and base addresses
  // ========================================================================
  val tmuConfig = new Area {
    // textureMode (0x300) - Texture format, filtering, clamp, combine modes
    val textureModeReg =
      busif.newRegAtWithCategory(0x300, "textureMode", RegisterCategory.fifoNoSync)
    val textureMode =
      textureModeReg.field(Bits(32 bits), AccessType.WO, 0, "Texture mode").asOutput()

    // tLOD (0x304) - LOD configuration
    val tLODReg = busif.newRegAtWithCategory(0x304, "tLOD", RegisterCategory.fifoNoSync)
    val tLOD = tLODReg.field(Bits(32 bits), AccessType.WO, 0, "LOD configuration").asOutput()

    // tDetail (0x308) - Detail texture parameters
    val tDetailReg = busif.newRegAtWithCategory(0x308, "tDetail", RegisterCategory.fifoNoSync)
    val tDetail =
      tDetailReg.field(Bits(32 bits), AccessType.WO, 0, "Detail texture params").asOutput()

    // texBaseAddr (0x30C) - Texture base address
    val texBaseAddrReg =
      busif.newRegAtWithCategory(0x30c, "texBaseAddr", RegisterCategory.fifoNoSync)
    val texBaseAddr =
      texBaseAddrReg.field(UInt(24 bits), AccessType.WO, 0, "Texture base address").asOutput()

    val texBaseAddr1Reg =
      busif.newRegAtWithCategory(0x310, "texBaseAddr_1", RegisterCategory.fifoNoSync)
    val texBaseAddr1 =
      texBaseAddr1Reg
        .field(UInt(24 bits), AccessType.WO, 0, "Texture base address for LOD 1")
        .asOutput()

    val texBaseAddr2Reg =
      busif.newRegAtWithCategory(0x314, "texBaseAddr_2", RegisterCategory.fifoNoSync)
    val texBaseAddr2 =
      texBaseAddr2Reg
        .field(UInt(24 bits), AccessType.WO, 0, "Texture base address for LOD 2")
        .asOutput()

    val texBaseAddr38Reg =
      busif.newRegAtWithCategory(0x318, "texBaseAddr_3_8", RegisterCategory.fifoNoSync)
    val texBaseAddr38 = texBaseAddr38Reg
      .field(UInt(24 bits), AccessType.WO, 0, "Texture base address for LOD 3 through 8")
      .asOutput()

    // trexInit0 (0x31C) - TMU hardware init / memory config
    val trexInit0Reg = busif.newRegAtWithCategory(0x31c, "trexInit0", RegisterCategory.fifoWithSync)
    val trexInit0 = trexInit0Reg.field(Bits(32 bits), AccessType.WO, 0, "TMU init 0").asOutput()

    // trexInit1 (0x320) - TMU hardware init / config output control
    val trexInit1Reg = busif.newRegAtWithCategory(0x320, "trexInit1", RegisterCategory.fifoWithSync)
    val trexInit1 = trexInit1Reg.field(Bits(32 bits), AccessType.WO, 0, "TMU init 1").asOutput()
  }

  if (config.packedTexLayout) {
    val texCfg = TexLayoutTables.TexConfig()
    texCfg.texBaseAddr := tmuConfig.texBaseAddr(18 downto 0)
    texCfg.texBaseAddr1 := tmuConfig.texBaseAddr1(18 downto 0)
    texCfg.texBaseAddr2 := tmuConfig.texBaseAddr2(18 downto 0)
    texCfg.texBaseAddr38 := tmuConfig.texBaseAddr38(18 downto 0)
    texCfg.tformat := tmuConfig.textureMode(11 downto 8).asUInt
    texCfg.tLOD_aspect := tmuConfig.tLOD(22 downto 21).asUInt
    texCfg.tLOD_sIsWider := tmuConfig.tLOD(20)
    texCfg.tLOD_multibase := tmuConfig.tLOD(24)
    val texTablesReg = Reg(TexLayoutTables.Tables())
    val triangleTexTablesReg = Reg(TexLayoutTables.Tables())
    texTablesReg := TexLayoutTables.compute(texCfg)
    triangleTexTablesReg := texTablesReg
    texTablesRegOpt = Some(texTablesReg)
    triangleTexTablesRegOpt = Some(triangleTexTablesReg)
  }

  // ========================================================================
  // NCC Table Registers (0x324-0x380)
  // Two NCC tables for YIQ compressed texture decode
  // ========================================================================
  val nccTable = new Area {
    val table0Y = nccBlock(0, "Y", 0x324)
    val table0I = nccBlock(0, "I", 0x334)
    val table0Q = nccBlock(0, "Q", 0x344)
    val table1Y = nccBlock(1, "Y", 0x354)
    val table1I = nccBlock(1, "I", 0x364)
    val table1Q = nccBlock(1, "Q", 0x374)

    val table0Y0 = table0Y(0); val table0Y1 = table0Y(1); val table0Y2 = table0Y(2);
    val table0Y3 = table0Y(3)
    val table0I0 = table0I(0); val table0I1 = table0I(1); val table0I2 = table0I(2);
    val table0I3 = table0I(3)
    val table0Q0 = table0Q(0); val table0Q1 = table0Q(1); val table0Q2 = table0Q(2);
    val table0Q3 = table0Q(3)
    val table1Y0 = table1Y(0); val table1Y1 = table1Y(1); val table1Y2 = table1Y(2);
    val table1Y3 = table1Y(3)
    val table1I0 = table1I(0); val table1I1 = table1I(1); val table1I2 = table1I(2);
    val table1I3 = table1I(3)
    val table1Q0 = table1Q(0); val table1Q1 = table1Q(1); val table1Q2 = table1Q(2);
    val table1Q3 = table1Q(3)
  }

  triangleConfigReg := captureTriangleConfig()

  // ========================================================================
  // Simulation Support - Make all register fields accessible during simulation
  // ========================================================================
  busif.slices.foreach { slice =>
    val addr = slice.getAddr()
    var fieldIndex = 0
    slice.getFields().foreach { field =>
      // Simple naming: address + field index (no string manipulation needed)
      field.hardbit.setName(f"sim_reg_${addr}%03x_f$fieldIndex")
      field.hardbit.simPublic()
      fieldIndex += 1
    }
  }
}

object RegisterBank {
  case class PaletteWrite() extends Bundle {
    val address = UInt(8 bits)
    val data = Bits(24 bits)
  }

  def bmbParams(c: Config) = BmbParameter(
    addressWidth = 12, // 4KB address space (remapping handled by AddressRemapper)
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )

  // External bus params include bit 21 for remap detection
  def externalBmbParams(c: Config) = BmbParameter(
    addressWidth = 22, // 4MB address space to include bit 21 for remapped registers
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )
}
