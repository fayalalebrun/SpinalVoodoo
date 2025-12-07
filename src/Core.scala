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
  regBank.io.pipelineBusy := triangleSetup.o.valid || rasterizer.o.valid || colorCombine.io.input.valid || write.i.fromPipeline.valid

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
  val ftriangleConverted = joinedResults.map { results =>
    val out = TriangleWithSign(c.vertexFormat)
    out.tri(0)(0) := results(0).number.fixTo(c.vertexFormat)
    out.tri(0)(1) := results(1).number.fixTo(c.vertexFormat)
    out.tri(1)(0) := results(2).number.fixTo(c.vertexFormat)
    out.tri(1)(1) := results(3).number.fixTo(c.vertexFormat)
    out.tri(2)(0) := results(4).number.fixTo(c.vertexFormat)
    out.tri(2)(1) := results(5).number.fixTo(c.vertexFormat)
    out.signBit := regBank.commands.ftriangleSignBit
    out
  }

  // Connect triangle command to triangle setup
  // Build triangle from vertex registers (integer path)
  // Note: Register values are already in 12.4 fixed-point format, so we interpret
  // the raw bits directly rather than converting (which would multiply by 16)
  val triangleIntPath = regBank.commands.triangleCmd.translateWith {
    val out = TriangleWithSign(c.vertexFormat)
    out.tri(0)(0).raw := regBank.triangleGeometry.vertexAx.asBits
    out.tri(0)(1).raw := regBank.triangleGeometry.vertexAy.asBits
    out.tri(1)(0).raw := regBank.triangleGeometry.vertexBx.asBits
    out.tri(1)(1).raw := regBank.triangleGeometry.vertexBy.asBits
    out.tri(2)(0).raw := regBank.triangleGeometry.vertexCx.asBits
    out.tri(2)(1).raw := regBank.triangleGeometry.vertexCy.asBits
    out.signBit := regBank.commands.triangleSignBit
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

  // Connect triangle setup to rasterizer
  rasterizer.i.translateFrom(triangleSetup.o) { (out, in) =>
    out.tri := in

    // Connect gradients from register bank
    // Register values are already in the target fixed-point format, so we interpret
    // the raw bits directly rather than converting (which would shift values incorrectly)

    // Red gradient (12.12 fixed point)
    out.grads.redGrad.start.raw := regBank.triangleGeometry.startR.asBits
    out.grads.redGrad.d(0).raw := regBank.triangleGeometry.dRdX.asBits
    out.grads.redGrad.d(1).raw := regBank.triangleGeometry.dRdY.asBits

    // Green gradient (12.12 fixed point)
    out.grads.greenGrad.start.raw := regBank.triangleGeometry.startG.asBits
    out.grads.greenGrad.d(0).raw := regBank.triangleGeometry.dGdX.asBits
    out.grads.greenGrad.d(1).raw := regBank.triangleGeometry.dGdY.asBits

    // Blue gradient (12.12 fixed point)
    out.grads.blueGrad.start.raw := regBank.triangleGeometry.startB.asBits
    out.grads.blueGrad.d(0).raw := regBank.triangleGeometry.dBdX.asBits
    out.grads.blueGrad.d(1).raw := regBank.triangleGeometry.dBdY.asBits

    // Depth gradient (20.12 fixed point)
    out.grads.depthGrad.start.raw := regBank.triangleGeometry.startZ.asBits
    out.grads.depthGrad.d(0).raw := regBank.triangleGeometry.dZdX.asBits
    out.grads.depthGrad.d(1).raw := regBank.triangleGeometry.dZdY.asBits

    // Alpha gradient (12.12 fixed point)
    out.grads.alphaGrad.start.raw := regBank.triangleGeometry.startA.asBits
    out.grads.alphaGrad.d(0).raw := regBank.triangleGeometry.dAdX.asBits
    out.grads.alphaGrad.d(1).raw := regBank.triangleGeometry.dAdY.asBits

    // W gradient (2.30 fixed point)
    out.grads.wGrad.start.raw := regBank.triangleGeometry.startW.asBits
    out.grads.wGrad.d(0).raw := regBank.triangleGeometry.dWdX.asBits
    out.grads.wGrad.d(1).raw := regBank.triangleGeometry.dWdY.asBits
  }

  // ========================================================================
  // Color Combine Unit
  // ========================================================================
  // Decode fbzColorPath register bits into ColorCombine configuration enums
  val fbzColorPath = regBank.renderConfig.fbzColorPath

  // Helper to decode enum from bits
  def decodeColorCombineConfig(): ColorCombine.Config = {
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

  // Connect rasterizer to color combine unit
  colorCombine.io.input.translateFrom(rasterizer.o) { (out, in) =>
    out.coords := in.coords

    // Convert interpolated color values from 12.12 fixed-point to 9-bit signed
    // Use sat() to saturate to 8-bit range at integer exponent, then extend to 9-bit signed
    val red8 = in.grads.redGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt
    val green8 = in.grads.greenGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt
    val blue8 = in.grads.blueGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt
    val alpha8 = in.grads.alphaGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt

    // Zero-extend to 9-bit signed (values are always positive 0-255)
    out.iterated.r := (False ## red8).asSInt
    out.iterated.g := (False ## green8).asSInt
    out.iterated.b := (False ## blue8).asSInt
    out.iteratedAlpha := (False ## alpha8).asSInt

    // Upper 8 bits of Z for alpha local select option
    val z8 = in.grads.depthGrad.sat(satMax = 255, satMin = 0, exp = 12 exp).asUInt
    out.iteratedZ := (False ## z8).asSInt

    // Pass through depth for later stages
    out.depth := in.grads.depthGrad

    // Texture is stubbed as zero until TREX is implemented
    out.texture.r := 0
    out.texture.g := 0
    out.texture.b := 0
    out.textureAlpha := 0

    // Constant colors from registers (packed as ARGB in 32-bit register)
    val color0Bits = regBank.renderConfig.color0
    val color1Bits = regBank.renderConfig.color1
    out.color0.r := color0Bits(23 downto 16).asUInt
    out.color0.g := color0Bits(15 downto 8).asUInt
    out.color0.b := color0Bits(7 downto 0).asUInt
    out.color1.r := color1Bits(23 downto 16).asUInt
    out.color1.g := color1Bits(15 downto 8).asUInt
    out.color1.b := color1Bits(7 downto 0).asUInt

    // Decode configuration from fbzColorPath register
    out.config := decodeColorCombineConfig()
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
