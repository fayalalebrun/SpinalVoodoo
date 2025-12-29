package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import _root_.math.{Fpxx, FpxxConfig, Fpxx2AFix}

case class Core(c: Config) extends Component {
  val io = new Bundle {
    // Register bus interface (external: 22-bit address for remap detection)
    val regBus = slave(Bmb(RegisterBank.externalBmbParams(c)))

    // Framebuffer write bus
    val fbWrite = master(Bmb(Write.baseBmbParams(c)))

    // Texture memory read bus (single TMU - Voodoo 1 level)
    val texRead = master(Bmb(Tmu.bmbParams(c)))

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

  // Connect address remapper between external bus and register bank
  // AddressRemapper handles RGBZASTW → standard layout translation for remapped addresses
  addressRemapper.io.input <> io.regBus
  addressRemapper.io.output <> regBank.io.bus
  regBank.io.statusInputs <> io.statusInputs
  regBank.io.statisticsIn <> io.statisticsIn

  // Pipeline busy signal: any stage has valid data
  regBank.io.pipelineBusy := triangleSetup.o.valid || rasterizer.o.valid || tmu.io.input.valid || colorCombine.io.input.valid || write.i.fromPipeline.valid

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

  // Combinatorial IEEE 754 float32 to fixed-point conversion
  // Used for gradient registers where pipelined conversion isn't needed
  def floatToFixed(floatBits: Bits, intBits: Int, fracBits: Int): SInt = {
    val sign = floatBits(31)
    val exponent = floatBits(30 downto 23).asUInt
    val mantissa = floatBits(22 downto 0).asUInt

    // Total output bits
    val totalBits = intBits + fracBits

    // For a float, value = (-1)^sign * 2^(exp-127) * 1.mantissa
    // For fixed point Q(intBits, fracBits), the binary point is after fracBits bits from LSB

    // Build the mantissa with implicit 1: 1.mantissa = 24 bits total
    val mantissaWithOne = U(1, 1 bit) @@ mantissa // 24 bits

    // The shift amount depends on exponent and target format
    // When exp=127, value = 1.xxx (no shift needed relative to integer 1)
    // For Q(intBits, fracBits): integer 1 is at bit position fracBits
    // So mantissa MSB (the implicit 1) should end up at position fracBits + (exp - 127)

    // We'll shift left if exp > 127, right if exp < 127
    // Base position: mantissa bit 23 (the implicit 1) represents 2^0
    // In target format, 2^0 is at bit position fracBits

    // Calculate shift: positive = left, negative = right
    // We need mantissa positioned so implicit 1 is at bit (fracBits + exp - 127)
    // Mantissa is 24 bits, with bit 23 = implicit 1 (2^0), bit 0 = 2^-23

    // To place bit 23 at position P, we need to shift by (P - 23)
    // P = fracBits + exp - 127
    // shift = fracBits + exp - 127 - 23 = fracBits + exp - 150

    val shiftAmount = (exponent.resize(9 bits).asSInt + S(fracBits - 150, 9 bits)).resize(9 bits)

    // Widen mantissa to accommodate shifts
    val wideMantissa = mantissaWithOne.resize(64 bits)

    // Apply shift - use barrel shifter approach
    val maxShift = 40 // Reasonable max shift for our formats
    val shiftedValue = UInt(64 bits)

    when(shiftAmount >= 0) {
      // Left shift
      val leftShift = shiftAmount.asUInt.resize(6 bits)
      when(leftShift > maxShift) {
        shiftedValue := U(0) // Overflow - saturate to max or 0
      } otherwise {
        shiftedValue := wideMantissa |<< leftShift
      }
    } otherwise {
      // Right shift
      val rightShift = (-shiftAmount).asUInt.resize(6 bits)
      when(rightShift > 63) {
        shiftedValue := U(0) // Underflow to 0
      } otherwise {
        shiftedValue := wideMantissa |>> rightShift
      }
    }

    // Extract result bits and apply sign
    val unsignedResult = shiftedValue(totalBits - 1 downto 0)
    val signedResult = SInt(totalBits bits)

    when(exponent === 0) {
      // Zero or denormal - treat as zero
      signedResult := S(0)
    } otherwise {
      when(sign) {
        signedResult := -unsignedResult.asSInt
      } otherwise {
        signedResult := unsignedResult.asSInt
      }
    }

    signedResult
  }

  // Create individual converters for each vertex coordinate
  val convVax = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVay = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVbx = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVby = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVcx = new Fpxx2AFix(16 bits, 16 bits, floatConfig)
  val convVcy = new Fpxx2AFix(16 bits, 16 bits, floatConfig)

  // Fork ftriangleCmd to feed all 6 converters
  val ftriangleForks = StreamFork(regBank.commands.ftriangleCmd, 6, synchronous = true)

  val fvertexAx = regBank.floatTriangleGeometry.fvertexAx
  val fvertexAy = regBank.floatTriangleGeometry.fvertexAy
  val fvertexBx = regBank.floatTriangleGeometry.fvertexBx
  val fvertexBy = regBank.floatTriangleGeometry.fvertexBy
  val fvertexCx = regBank.floatTriangleGeometry.fvertexCx
  val fvertexCy = regBank.floatTriangleGeometry.fvertexCy

  val fstartR = regBank.floatTriangleGeometry.fstartR
  val fstartG = regBank.floatTriangleGeometry.fstartG
  val fstartB = regBank.floatTriangleGeometry.fstartB
  val fstartZ = regBank.floatTriangleGeometry.fstartZ
  val fstartA = regBank.floatTriangleGeometry.fstartA
  val fstartS = regBank.floatTriangleGeometry.fstartS
  val fstartT = regBank.floatTriangleGeometry.fstartT
  val fstartW = regBank.floatTriangleGeometry.fstartW

  val fdRdX = regBank.floatTriangleGeometry.fdRdX
  val fdGdX = regBank.floatTriangleGeometry.fdGdX
  val fdBdX = regBank.floatTriangleGeometry.fdBdX
  val fdZdX = regBank.floatTriangleGeometry.fdZdX
  val fdAdX = regBank.floatTriangleGeometry.fdAdX
  val fdSdX = regBank.floatTriangleGeometry.fdSdX
  val fdTdX = regBank.floatTriangleGeometry.fdTdX
  val fdWdX = regBank.floatTriangleGeometry.fdWdX

  val fdRdY = regBank.floatTriangleGeometry.fdRdY
  val fdGdY = regBank.floatTriangleGeometry.fdGdY
  val fdBdY = regBank.floatTriangleGeometry.fdBdY
  val fdZdY = regBank.floatTriangleGeometry.fdZdY
  val fdAdY = regBank.floatTriangleGeometry.fdAdY
  val fdSdY = regBank.floatTriangleGeometry.fdSdY
  val fdTdY = regBank.floatTriangleGeometry.fdTdY
  val fdWdY = regBank.floatTriangleGeometry.fdWdY

  // Feed each forked stream to its converter with appropriate data
  convVax.io.op.translateFrom(ftriangleForks(0)) { (fpxx, _) =>
    fpxx.assignFromBits(fvertexAx)
  }
  convVay.io.op.translateFrom(ftriangleForks(1)) { (fpxx, _) =>
    fpxx.assignFromBits(fvertexAy)
  }
  convVbx.io.op.translateFrom(ftriangleForks(2)) { (fpxx, _) =>
    fpxx.assignFromBits(fvertexBx)
  }
  convVby.io.op.translateFrom(ftriangleForks(3)) { (fpxx, _) =>
    fpxx.assignFromBits(fvertexBy)
  }
  convVcx.io.op.translateFrom(ftriangleForks(4)) { (fpxx, _) =>
    fpxx.assignFromBits(fvertexCx)
  }
  convVcy.io.op.translateFrom(ftriangleForks(5)) { (fpxx, _) =>
    fpxx.assignFromBits(fvertexCy)
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
    out.grads := captureFloatGradients() // Use float-to-fixed conversion for ftriangleCmd
    out.config := captureFloatPerTriangleConfig() // Also convert float gradients in config
    out
  }

  // Helper function to capture gradients from floating-point registers with conversion
  // This is used for ftriangleCmd path where gradients are in IEEE 754 float format
  def captureFloatGradients(): Rasterizer.GradientBundle[Rasterizer.InputGradient] = {
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)

    // Red gradient (12.12 fixed point) - convert from float
    grads.redGrad.start.raw := floatToFixed(fstartR, 12, 12).asBits
    grads.redGrad.d(0).raw := floatToFixed(fdRdX, 12, 12).asBits
    grads.redGrad.d(1).raw := floatToFixed(fdRdY, 12, 12).asBits

    // Green gradient (12.12 fixed point)
    grads.greenGrad.start.raw := floatToFixed(fstartG, 12, 12).asBits
    grads.greenGrad.d(0).raw := floatToFixed(fdGdX, 12, 12).asBits
    grads.greenGrad.d(1).raw := floatToFixed(fdGdY, 12, 12).asBits

    // Blue gradient (12.12 fixed point)
    grads.blueGrad.start.raw := floatToFixed(fstartB, 12, 12).asBits
    grads.blueGrad.d(0).raw := floatToFixed(fdBdX, 12, 12).asBits
    grads.blueGrad.d(1).raw := floatToFixed(fdBdY, 12, 12).asBits

    // Depth gradient (20.12 fixed point)
    grads.depthGrad.start.raw := floatToFixed(fstartZ, 20, 12).asBits
    grads.depthGrad.d(0).raw := floatToFixed(fdZdX, 20, 12).asBits
    grads.depthGrad.d(1).raw := floatToFixed(fdZdY, 20, 12).asBits

    // Alpha gradient (12.12 fixed point)
    grads.alphaGrad.start.raw := floatToFixed(fstartA, 12, 12).asBits
    grads.alphaGrad.d(0).raw := floatToFixed(fdAdX, 12, 12).asBits
    grads.alphaGrad.d(1).raw := floatToFixed(fdAdY, 12, 12).asBits

    // W gradient (2.30 fixed point)
    grads.wGrad.start.raw := floatToFixed(fstartW, 2, 30).asBits
    grads.wGrad.d(0).raw := floatToFixed(fdWdX, 2, 30).asBits
    grads.wGrad.d(1).raw := floatToFixed(fdWdY, 2, 30).asBits

    // TMU S gradient (14.18 fixed point)
    grads.sGrad.start.raw := floatToFixed(fstartS, 14, 18).asBits
    grads.sGrad.d(0).raw := floatToFixed(fdSdX, 14, 18).asBits
    grads.sGrad.d(1).raw := floatToFixed(fdSdY, 14, 18).asBits

    // TMU T gradient (14.18 fixed point)
    grads.tGrad.start.raw := floatToFixed(fstartT, 14, 18).asBits
    grads.tGrad.d(0).raw := floatToFixed(fdTdX, 14, 18).asBits
    grads.tGrad.d(1).raw := floatToFixed(fdTdY, 14, 18).asBits

    grads
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

    // TMU S gradient (14.18 fixed point) - single TMU (Voodoo 1 level)
    grads.sGrad.start.raw := regBank.triangleGeometry.startS.asBits
    grads.sGrad.d(0).raw := regBank.triangleGeometry.dSdX.asBits
    grads.sGrad.d(1).raw := regBank.triangleGeometry.dSdY.asBits

    // TMU T gradient (14.18 fixed point) - single TMU (Voodoo 1 level)
    grads.tGrad.start.raw := regBank.triangleGeometry.startT.asBits
    grads.tGrad.d(0).raw := regBank.triangleGeometry.dTdX.asBits
    grads.tGrad.d(1).raw := regBank.triangleGeometry.dTdY.asBits

    grads
  }

  // Helper function to capture per-triangle render configuration from registers at command time
  // These are registers with FIFO=Yes, Sync=No in the datasheet
  def capturePerTriangleConfig(): TriangleSetup.PerTriangleConfig = {
    val cfg = TriangleSetup.PerTriangleConfig(c)

    // FBI registers - extract relevant bits from 32-bit registers
    cfg.fbzColorPath := regBank.renderConfig.fbzColorPath.resized
    cfg.fogMode := regBank.renderConfig.fogMode.resized
    cfg.alphaMode := regBank.renderConfig.alphaMode

    // TMU registers (single TMU - Voodoo 1 level)
    cfg.tmuTextureMode := regBank.tmuConfig.textureMode
    cfg.tmuTexBaseAddr := regBank.tmuConfig.texBaseAddr
    cfg.tmuTLOD := regBank.tmuConfig.tLOD.resized
    // TMU texture coordinate gradients (dX and dY for S and T)
    // Using .raw because AFix isn't a BaseType and can't be used directly with BusIf register fields
    cfg.tmudSdX.raw := regBank.triangleGeometry.dSdX.asBits
    cfg.tmudTdX.raw := regBank.triangleGeometry.dTdX.asBits
    cfg.tmudSdY.raw := regBank.triangleGeometry.dSdY.asBits
    cfg.tmudTdY.raw := regBank.triangleGeometry.dTdY.asBits

    cfg
  }

  // Float version of capturePerTriangleConfig - converts float S/T gradients
  def captureFloatPerTriangleConfig(): TriangleSetup.PerTriangleConfig = {
    val cfg = TriangleSetup.PerTriangleConfig(c)

    // FBI registers - same as fixed-point version (these aren't float registers)
    cfg.fbzColorPath := regBank.renderConfig.fbzColorPath.resized
    cfg.fogMode := regBank.renderConfig.fogMode.resized
    cfg.alphaMode := regBank.renderConfig.alphaMode

    // TMU registers (single TMU - Voodoo 1 level)
    cfg.tmuTextureMode := regBank.tmuConfig.textureMode
    cfg.tmuTexBaseAddr := regBank.tmuConfig.texBaseAddr
    cfg.tmuTLOD := regBank.tmuConfig.tLOD.resized

    // TMU texture coordinate gradients - convert from float (14.18 fixed point)
    cfg.tmudSdX.raw := floatToFixed(fdSdX, 14, 18).asBits
    cfg.tmudTdX.raw := floatToFixed(fdTdX, 14, 18).asBits
    cfg.tmudSdY.raw := floatToFixed(fdSdY, 14, 18).asBits
    cfg.tmudTdY.raw := floatToFixed(fdTdY, 14, 18).asBits

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

  // Make TMU and ColorCombine streams accessible for stall monitoring
  tmu.io.input.simPublic()
  tmu.io.output.simPublic()
  colorCombine.io.input.simPublic()
  colorCombine.io.output.simPublic()
  write.i.fromPipeline.simPublic()

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

  // Connect texture memory bus
  io.texRead <> tmu.io.texRead

  // Fork rasterizer output: one to TMU, one to a queue for synchronization
  val rasterFork = StreamFork2(rasterizer.o, synchronous = true)

  // Queue to hold rasterizer data while TMU processes
  // Depth must be >= TMU's maximum in-flight transactions
  val tmuGradQueue = rasterFork._2.queue(4)

  // Connect fork path 1 to TMU
  tmu.io.input.translateFrom(rasterFork._1) { (out, in) =>
    out.coords := in.coords
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

    out.coords := tmuOut.coords

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

    // Use texture from TMU (single TMU output)
    out.texture := tmuOut.texture
    out.textureAlpha := tmuOut.textureAlpha

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
