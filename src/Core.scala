package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import _root_.math.{Fpxx, FpxxConfig, Fpxx2AFix}

case class Core(c: Config) extends Component {
  val io = new Bundle {
    // Register bus interface
    val regBus = slave(Bmb(RegisterBank.bmbParams(c)))

    // Framebuffer write bus
    val fbWrite = master(Bmb(Write.baseBmbParams(c)))

    // Texture memory read buses (one per TMU)
    val texRead0 = master(Bmb(Tmu.bmbParams(c)))
    val texRead1 = master(Bmb(Tmu.bmbParams(c)))

    // Status inputs (hardware state)
    // Note: pciFifoFree now comes from RegisterBank's internal FIFO
    val statusInputs = in(new Bundle {
      val vRetrace = Bool()
      val fbiBusy = Bool()
      val trexBusy = Bool()
      val sstBusy = Bool()
      val displayedBuffer = UInt(2 bits)
      val memFifoFree = UInt(16 bits)
      val swapsPending = UInt(3 bits)
      val pciInterrupt = Bool()
    })

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
  val regBank = RegisterBank(c)
  val triangleSetup = TriangleSetup(c)
  val rasterizer = Rasterizer(c)
  val tmu0 = Tmu(c)
  val tmu1 = Tmu(c)
  val colorCombine = ColorCombine(c)
  val write = Write(c)

  // Make triangle command streams accessible for simulation monitoring
  regBank.commands.triangleCmd.simPublic()
  regBank.commands.ftriangleCmd.simPublic()

  // Connect register bank
  regBank.io.bus <> io.regBus
  regBank.io.statusInputs <> io.statusInputs
  regBank.io.statisticsIn <> io.statisticsIn

  // Pipeline busy signal: any stage has valid data
  regBank.io.pipelineBusy := triangleSetup.o.valid || rasterizer.o.valid || tmu0.io.input.valid || tmu1.io.input.valid || colorCombine.io.input.valid || write.i.fromPipeline.valid

  // TODO: Wire unused command streams to always ready
  regBank.commands.fastfillCmd.ready := True
  regBank.commands.nopCmd.ready := True
  regBank.commands.swapbufferCmd.ready := True

  // ========================================================================
  // Float to Fixed-Point Conversion for fTriangleCMD
  // ========================================================================
  // Convert float registers to fixed-point using Fpxx2AFix
  // Note: Fpxx2AFix requires target format to have enough bits to hold the
  // full mantissa (23 bits for float32). We convert to SQ(16, 16) first,
  // then truncate to the target vertex format SQ(12, 4).

  val floatConfig = FpxxConfig.float32()

  // Intermediate format: SQ(16, 16) = 32 bits (enough for float32 mantissa)
  val intermediateFormat = SQ(16, 16)

  // Create individual converters for each vertex coordinate
  val convVax = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVay = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVbx = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVby = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVcx = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVcy = new Fpxx2AFix(16 bits, 16 bits, floatConfig)

  // Fork ftriangleCmd to feed all 6 converters
  val ftriangleForks = StreamFork(regBank.commands.ftriangleCmd, 6, synchronous = true)

  // Feed each forked stream to its converter with appropriate data
  convVax.io.op.translateFrom(ftriangleForks(0)) { (fpxx, _) =>
    fpxx.assignFromBits(regBank.floatTriangleGeometry.fvertexAx)
  }
  convVay.io.op.translateFrom(ftriangleForks(1)) { (fpxx, _) =>
    fpxx.assignFromBits(regBank.floatTriangleGeometry.fvertexAy)
  }
  convVbx.io.op.translateFrom(ftriangleForks(2)) { (fpxx, _) =>
    fpxx.assignFromBits(regBank.floatTriangleGeometry.fvertexBx)
  }
  convVby.io.op.translateFrom(ftriangleForks(3)) { (fpxx, _) =>
    fpxx.assignFromBits(regBank.floatTriangleGeometry.fvertexBy)
  }
  convVcx.io.op.translateFrom(ftriangleForks(4)) { (fpxx, _) =>
    fpxx.assignFromBits(regBank.floatTriangleGeometry.fvertexCx)
  }
  convVcy.io.op.translateFrom(ftriangleForks(5)) { (fpxx, _) =>
    fpxx.assignFromBits(regBank.floatTriangleGeometry.fvertexCy)
  }

  // Join all converter results
  val joinedResults = StreamJoin.vec(
    Seq(
      convVax.io.result,
      convVay.io.result,
      convVbx.io.result,
      convVby.io.result,
      convVcx.io.result,
      convVcy.io.result
    )
  )

  // Build triangle from joined results, converting to target format
  // Gradients and render config are captured at command time to ensure proper pipeline synchronization
  val ftriangleConverted = joinedResults.map { results =>
    val out = TriangleSetup.Input(c)
    out.triWithSign.tri(0)(0) := results(0).number.fixTo(c.vertexFormat)
    out.triWithSign.tri(0)(1) := results(1).number.fixTo(c.vertexFormat)
    out.triWithSign.tri(1)(0) := results(2).number.fixTo(c.vertexFormat)
    out.triWithSign.tri(1)(1) := results(3).number.fixTo(c.vertexFormat)
    out.triWithSign.tri(2)(0) := results(4).number.fixTo(c.vertexFormat)
    out.triWithSign.tri(2)(1) := results(5).number.fixTo(c.vertexFormat)
    out.triWithSign.signBit := regBank.commands.ftriangleSignBit
    out.grads := captureGradients()
    out.config := capturePerTriangleConfig()
    out
  }

  // Helper function to capture all gradients from registers at command time
  // This must be called during translateWith to capture register values when command fires
  def captureGradients(): Rasterizer.GradientBundle[Rasterizer.InputGradient] = {
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)

    // Red gradient (12.12 fixed point)
    grads.redGrad.start.raw := regBank.triangleGeometry.startR.asBits
    grads.redGrad.d(0).raw := regBank.triangleGeometry.dRdX.asBits
    grads.redGrad.d(1).raw := regBank.triangleGeometry.dRdY.asBits

    // Green gradient (12.12 fixed point)
    grads.greenGrad.start.raw := regBank.triangleGeometry.startG.asBits
    grads.greenGrad.d(0).raw := regBank.triangleGeometry.dGdX.asBits
    grads.greenGrad.d(1).raw := regBank.triangleGeometry.dGdY.asBits

    // Blue gradient (12.12 fixed point)
    grads.blueGrad.start.raw := regBank.triangleGeometry.startB.asBits
    grads.blueGrad.d(0).raw := regBank.triangleGeometry.dBdX.asBits
    grads.blueGrad.d(1).raw := regBank.triangleGeometry.dBdY.asBits

    // Depth gradient (20.12 fixed point)
    grads.depthGrad.start.raw := regBank.triangleGeometry.startZ.asBits
    grads.depthGrad.d(0).raw := regBank.triangleGeometry.dZdX.asBits
    grads.depthGrad.d(1).raw := regBank.triangleGeometry.dZdY.asBits

    // Alpha gradient (12.12 fixed point)
    grads.alphaGrad.start.raw := regBank.triangleGeometry.startA.asBits
    grads.alphaGrad.d(0).raw := regBank.triangleGeometry.dAdX.asBits
    grads.alphaGrad.d(1).raw := regBank.triangleGeometry.dAdY.asBits

    // W gradient (2.30 fixed point)
    grads.wGrad.start.raw := regBank.triangleGeometry.startW.asBits
    grads.wGrad.d(0).raw := regBank.triangleGeometry.dWdX.asBits
    grads.wGrad.d(1).raw := regBank.triangleGeometry.dWdY.asBits

    // TMU0 S gradient (14.18 fixed point)
    grads.s0Grad.start.raw := regBank.triangleGeometry.startS.asBits
    grads.s0Grad.d(0).raw := regBank.triangleGeometry.dSdX.asBits
    grads.s0Grad.d(1).raw := regBank.triangleGeometry.dSdY.asBits

    // TMU0 T gradient (14.18 fixed point)
    grads.t0Grad.start.raw := regBank.triangleGeometry.startT.asBits
    grads.t0Grad.d(0).raw := regBank.triangleGeometry.dTdX.asBits
    grads.t0Grad.d(1).raw := regBank.triangleGeometry.dTdY.asBits

    // TMU1 S gradient (14.18 fixed point)
    grads.s1Grad.start.raw := regBank.tmu1Coords.startS1.asBits
    grads.s1Grad.d(0).raw := regBank.tmu1Coords.dS1dX.asBits
    grads.s1Grad.d(1).raw := regBank.tmu1Coords.dS1dY.asBits

    // TMU1 T gradient (14.18 fixed point)
    grads.t1Grad.start.raw := regBank.tmu1Coords.startT1.asBits
    grads.t1Grad.d(0).raw := regBank.tmu1Coords.dT1dX.asBits
    grads.t1Grad.d(1).raw := regBank.tmu1Coords.dT1dY.asBits

    grads
  }

  // Helper function to capture per-triangle render configuration from registers at command time
  // These are registers with FIFO=Yes, Sync=No in the datasheet
  def capturePerTriangleConfig(): TriangleSetup.PerTriangleConfig = {
    val cfg = TriangleSetup.PerTriangleConfig()

    // FBI registers
    cfg.fbzColorPath := regBank.renderConfig.fbzColorPath
    cfg.fogMode := regBank.renderConfig.fogMode
    cfg.alphaMode := regBank.renderConfig.alphaMode

    // TMU0 registers
    cfg.tmu0TextureMode := regBank.tmu0Config.textureMode
    cfg.tmu0TexBaseAddr := regBank.tmu0Config.texBaseAddr

    // TMU1 registers
    cfg.tmu1TextureMode := regBank.tmu1Config.textureMode
    cfg.tmu1TexBaseAddr := regBank.tmu1Config.texBaseAddr

    cfg
  }

  // Connect triangle command to triangle setup
  // Build triangle from vertex registers (integer path)
  // Note: Register values are already in 12.4 fixed-point format, so we interpret
  // the raw bits directly rather than converting (which would multiply by 16)
  // Gradients and render config are captured at command time to ensure proper pipeline synchronization
  val triangleIntPath = regBank.commands.triangleCmd.translateWith {
    val out = TriangleSetup.Input(c)
    out.triWithSign.tri(0)(0).raw := regBank.triangleGeometry.vertexAx.asBits
    out.triWithSign.tri(0)(1).raw := regBank.triangleGeometry.vertexAy.asBits
    out.triWithSign.tri(1)(0).raw := regBank.triangleGeometry.vertexBx.asBits
    out.triWithSign.tri(1)(1).raw := regBank.triangleGeometry.vertexBy.asBits
    out.triWithSign.tri(2)(0).raw := regBank.triangleGeometry.vertexCx.asBits
    out.triWithSign.tri(2)(1).raw := regBank.triangleGeometry.vertexCy.asBits
    out.triWithSign.signBit := regBank.commands.triangleSignBit
    out.grads := captureGradients()
    out.config := capturePerTriangleConfig()
    out
  }

  // Merge integer and float triangle paths
  // Use assumeOhInput since only one path is active at a time (no arbitration needed)
  triangleSetup.i << StreamArbiterFactory.assumeOhInput.on(Seq(triangleIntPath, ftriangleConverted))

  // Make triangle streams accessible for simulation monitoring
  joinedResults.simPublic()
  ftriangleConverted.simPublic()
  triangleIntPath.simPublic()
  triangleSetup.i.simPublic()
  triangleSetup.o.simPublic()
  rasterizer.i.simPublic()
  rasterizer.o.simPublic()
  write.o.fbWrite.simPublic()

  // Connect triangle setup directly to rasterizer
  // Gradients are now captured at command time and flow through TriangleSetup
  rasterizer.i << triangleSetup.o

  // ========================================================================
  // Color Combine Unit
  // ========================================================================
  // Helper to decode enum from bits (takes captured fbzColorPath per-triangle)
  def decodeColorCombineConfig(fbzColorPath: Bits): ColorCombine.Config = {
    val cfg = ColorCombine.Config()

    // RGB channel controls (from fbzColorPath)
    cfg.rgbSel.assignFromBits(fbzColorPath(1 downto 0))
    cfg.localSelect.assignFromBits(fbzColorPath(4 downto 4))
    cfg.localSelectOverride := fbzColorPath(7)
    cfg.zeroOther := fbzColorPath(8)
    cfg.subClocal := fbzColorPath(9)
    cfg.mselect.assignFromBits(fbzColorPath(12 downto 10))
    cfg.reverseBlend := fbzColorPath(13)
    cfg.add.assignFromBits(fbzColorPath(15 downto 14))
    cfg.invertOutput := fbzColorPath(16)

    // Alpha channel controls (from fbzColorPath)
    cfg.alphaSel.assignFromBits(fbzColorPath(3 downto 2))
    cfg.alphaLocalSelect.assignFromBits(fbzColorPath(6 downto 5))
    cfg.alphaZeroOther := fbzColorPath(17)
    cfg.alphaSubClocal := fbzColorPath(18)
    cfg.alphaMselect.assignFromBits(fbzColorPath(21 downto 19))
    cfg.alphaReverseBlend := fbzColorPath(22)
    cfg.alphaAdd.assignFromBits(fbzColorPath(24 downto 23))
    cfg.alphaInvertOutput := fbzColorPath(25)

    cfg.textureEnable := fbzColorPath(27)

    cfg
  }

  // ========================================================================
  // Texture Mapping Units (TMU0 → TMU1 sequential chain)
  // ========================================================================
  // Architecture:
  //   Rasterizer output is forked:
  //     - One path goes to TMU0 for texture fetch
  //     - Other path is queued to preserve gradients during TMU0's latency
  //   TMU0's texture output is joined with the queued gradients
  //   This joined result is then forked again for TMU1, etc.
  //
  // TMU configuration is now captured per-triangle and flows through the stream

  // Connect texture memory buses
  io.texRead0 <> tmu0.io.texRead
  io.texRead1 <> tmu1.io.texRead

  // Fork rasterizer output: one to TMU0, one to a queue for synchronization
  val rasterFork = StreamFork2(rasterizer.o, synchronous = true)

  // Queue to hold rasterizer data while TMU0 processes
  // Depth must be >= TMU0's maximum in-flight transactions
  val tmu0GradQueue = rasterFork._2.queue(4)

  // Connect fork path 1 to TMU0
  tmu0.io.input.translateFrom(rasterFork._1) { (out, in) =>
    out.coords := in.coords
    out.s := in.grads.s0Grad // TMU0's S coordinate
    out.t := in.grads.t0Grad // TMU0's T coordinate
    out.w := in.grads.wGrad // Shared W
    out.cOther.r := 0 // No upstream texture for TMU0
    out.cOther.g := 0
    out.cOther.b := 0
    out.aOther := 0
    out.grads := in.grads // Pass through (but won't be used from TMU output)
    // TMU0 config from captured per-triangle state
    out.config.textureMode := in.config.tmu0TextureMode
    out.config.texBaseAddr := in.config.tmu0TexBaseAddr
  }

  // Join TMU0 output with queued gradients
  val tmu0Joined = StreamJoin(tmu0.io.output, tmu0GradQueue)

  // Fork the joined result: one to TMU1, one to a queue for synchronization
  val tmu1InputFork = StreamFork2(tmu0Joined, synchronous = true)

  // Queue to hold data while TMU1 processes
  val tmu1GradQueue = tmu1InputFork._2.queue(4)

  // Connect fork path 1 to TMU1
  tmu1.io.input.translateFrom(tmu1InputFork._1) { (out, payload) =>
    val tmu0Out = payload._1
    val rasterOut = payload._2
    out.coords := tmu0Out.coords
    out.s := rasterOut.grads.s1Grad // TMU1's S coordinate from original rasterizer output
    out.t := rasterOut.grads.t1Grad // TMU1's T coordinate from original rasterizer output
    out.w := rasterOut.grads.wGrad // Shared W
    out.cOther := tmu0Out.texture // TMU0's texture output becomes TMU1's c_other
    out.aOther := tmu0Out.textureAlpha
    out.grads := rasterOut.grads // Pass through original gradients
    // TMU1 config from captured per-triangle state
    out.config.textureMode := rasterOut.config.tmu1TextureMode
    out.config.texBaseAddr := rasterOut.config.tmu1TexBaseAddr
  }

  // Join TMU1 output with queued data (contains TMU0 output + original raster data)
  val tmu1Joined = StreamJoin(tmu1.io.output, tmu1GradQueue)

  // Connect TMU1 joined output to ColorCombine
  colorCombine.io.input.translateFrom(tmu1Joined) { (out, payload) =>
    val tmu1Out = payload._1
    val tmu0JoinedData = payload._2
    val rasterOut = tmu0JoinedData._2 // Extract original rasterizer output

    out.coords := tmu1Out.coords

    // Convert interpolated color values from 12.12 fixed-point to 8-bit unsigned
    // Use gradients from original rasterizer output (properly synchronized)
    out.iterated.r := rasterOut.grads.redGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt
    out.iterated.g := rasterOut.grads.greenGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt
    out.iterated.b := rasterOut.grads.blueGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt
    out.iteratedAlpha := rasterOut.grads.alphaGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt

    // Upper 8 bits of Z for alpha local select option
    out.iteratedZ := rasterOut.grads.depthGrad.sat(satMax = 255, satMin = 0, exp = 12 exp).asUInt

    // Pass through depth for later stages
    out.depth := rasterOut.grads.depthGrad

    // Use texture from TMU1 (final texture output)
    out.texture := tmu1Out.texture
    out.textureAlpha := tmu1Out.textureAlpha

    // Constant colors from registers (packed as ARGB in 32-bit register)
    val color0Bits = regBank.renderConfig.color0
    val color1Bits = regBank.renderConfig.color1
    out.color0.r := color0Bits(23 downto 16).asUInt
    out.color0.g := color0Bits(15 downto 8).asUInt
    out.color0.b := color0Bits(7 downto 0).asUInt
    out.color1.r := color1Bits(23 downto 16).asUInt
    out.color1.g := color1Bits(15 downto 8).asUInt
    out.color1.b := color1Bits(7 downto 0).asUInt

    // Decode configuration from captured per-triangle fbzColorPath
    out.config := decodeColorCombineConfig(rasterOut.config.fbzColorPath)
  }

  // Connect color combine unit to write stage
  write.i.fromPipeline.translateFrom(colorCombine.io.output) { (out, in) =>
    out.coords := in.coords

    val fbWord = cloneOf(out.toFb)

    // Convert 8-bit RGB to 5-6-5 format (take MSBs)
    fbWord.color.r := in.color.r(7 downto 3)
    fbWord.color.g := in.color.g(7 downto 2)
    fbWord.color.b := in.color.b(7 downto 3)

    // fbzMode bit 18: 0=depth buffering, 1=destination alpha planes
    val useAlpha = regBank.renderConfig.fbzMode.enableAlphaPlanes

    // Depth: convert from AFix to 16-bit integer
    val depth16 = in.depth.sat(satMax = 0xffff, satMin = 0, exp = 0 exp).asUInt.resize(16 bits)

    // Alpha: extend 8-bit to 16-bit
    val alpha16 = in.alpha.resize(16 bits)

    // Select based on fbzMode bit 18
    fbWord.depthAlpha := (useAlpha ? alpha16 | depth16).asBits

    out.toFb := fbWord
  }

  write.i.fbBaseAddr := io.fbBaseAddr

  // Connect framebuffer write bus
  write.o.fbWrite <> io.fbWrite
}
