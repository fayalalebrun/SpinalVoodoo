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
  val write = Write(c)

  // Make triangle command streams accessible for simulation monitoring
  regBank.commands.triangleCmd.simPublic()
  regBank.commands.ftriangleCmd.simPublic()

  // Connect register bank
  regBank.io.bus <> io.regBus
  regBank.io.statusInputs <> io.statusInputs
  regBank.io.statisticsIn <> io.statisticsIn

  // Pipeline busy signal: rasterizer or write stage has valid data
  regBank.io.pipelineBusy := triangleSetup.o.valid || rasterizer.o.valid || write.i.fromPipeline.valid

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

  // Connect rasterizer to write stage
  write.i.fromPipeline.translateFrom(rasterizer.o) { (out, in) =>
    out.coords := in.coords

    // Convert interpolated color values to RGB565
    val fbWord = cloneOf(out.toFb)

    // Convert color from 12.12 to 8-bit integer
    // Use sat() to saturate to 8-bit range (0-255) at integer exponent (0)
    val red8 = in.grads.redGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt
    val green8 = in.grads.greenGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt
    val blue8 = in.grads.blueGrad.sat(satMax = 255, satMin = 0, exp = 0 exp).asUInt

    // Convert 8-bit RGB to 5-6-5 format (take MSBs)
    fbWord.color.r := red8(7 downto 3)
    fbWord.color.g := green8(7 downto 2)
    fbWord.color.b := blue8(7 downto 3)

    // fbzMode bit 18: 0=depth buffering, 1=destination alpha planes
    val useAlpha = regBank.renderConfig.fbzMode.enableAlphaPlanes

    // Depth: convert 20.12 to 16-bit integer
    val depth16 =
      in.grads.depthGrad.sat(satMax = 0xffff, satMin = 0, exp = 0 exp).asUInt.resize(16 bits)

    // Alpha: convert 12.12 to 16-bit integer
    val alpha16 =
      in.grads.alphaGrad.sat(satMax = 0xffff, satMin = 0, exp = 0 exp).asUInt.resize(16 bits)

    // Select based on fbzMode bit 18
    fbWord.depthAlpha := (useAlpha ? alpha16 | depth16).asBits

    out.toFb := fbWord
  }

  write.i.fbBaseAddr := io.fbBaseAddr

  // Connect framebuffer write bus
  write.o.fbWrite <> io.fbWrite
}
