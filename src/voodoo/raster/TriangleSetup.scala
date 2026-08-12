package voodoo.raster

import voodoo._
import voodoo.xls._
import spinal.core._
import spinal.core.formal._
import spinal.lib._

case class TriangleSetup(c: Config, formalStrong: Boolean = false) extends Component {
  val i = slave(Stream(TriangleSetup.Input(c)))
  val o = master(Stream(TriangleSetup.Output(c)))

  object SetupState extends SpinalEnum {
    val Idle, Prep, Iterate, Done = newElement()
  }

  val state = RegInit(SetupState.Idle)
  val pending = Reg(TriangleSetup.Input(c))
  val outputReg = Reg(TriangleSetup.Output(c))
  val iterIndex = Reg(UInt(3 bits)) init (0)
  val dxMagWork = Reg(UInt(16 bits)) init (0)
  val dyMagWork = Reg(UInt(16 bits)) init (0)

  def copyTrace[T <: Data](dst: T, src: T): Unit = dst := src

  def copySetupInput(dst: TriangleSetup.Input, src: TriangleSetup.Input): Unit = {
    dst.triWithSign := src.triWithSign
    dst.grads.all.zip(src.grads.all).foreach { case (d, s) =>
      d.start.raw := s.start.raw
      d.d := s.d
    }
    dst.hiAlpha.start.raw := src.hiAlpha.start.raw
    dst.hiAlpha.dAdX.raw := src.hiAlpha.dAdX.raw
    dst.hiAlpha.dAdY.raw := src.hiAlpha.dAdY.raw
    dst.texHi.sStart.raw := src.texHi.sStart.raw
    dst.texHi.tStart.raw := src.texHi.tStart.raw
    dst.texHi.dSdX.raw := src.texHi.dSdX.raw
    dst.texHi.dTdX.raw := src.texHi.dTdX.raw
    dst.texHi.dSdY.raw := src.texHi.dSdY.raw
    dst.texHi.dTdY.raw := src.texHi.dTdY.raw
    dst.config := src.config
    if (c.trace.enabled) copyTrace(dst.trace, src.trace)
  }

  def copySetupOutput(dst: TriangleSetup.Output, src: TriangleSetup.Output): Unit = {
    dst.coeffs := src.coeffs
    dst.edgeStart := src.edgeStart
    dst.xrange := src.xrange
    dst.yrange := src.yrange
    dst.grads.all.zip(src.grads.all).foreach { case (d, s) =>
      d.d := s.d
    }
    dst.texHi.dSdX := src.texHi.dSdX
    dst.texHi.dTdX := src.texHi.dTdX
    dst.texHi.dSdY := src.texHi.dSdY
    dst.texHi.dTdY := src.texHi.dTdY
    dst.hiAlpha.dAdX := src.hiAlpha.dAdX
    dst.hiAlpha.dAdY := src.hiAlpha.dAdY
    dst.config := src.config
    if (c.trace.enabled) copyTrace(dst.trace, src.trace)
  }

  case class IterChannel(inStart: AFix, dxRaw: SInt, dyRaw: SInt) extends Area {
    val startWidth = inStart.raw.getWidth
    val accWidth = startWidth + 18
    val acc = Reg(SInt(accWidth bits)) init (0)
    val dxTerm = Reg(SInt(accWidth bits)) init (0)
    val dyTerm = Reg(SInt(accWidth bits)) init (0)

    def load(dxSubRaw: UInt, dySubRaw: UInt, paramAdjust: Bool, dxNeg: Bool, dyNeg: Bool): Unit = {
      val load = new XlsTriangleIterLoad
      load.io.start := inStart.raw.asSInt.resize(48 bits)
      load.io.dx := dxRaw.resize(48 bits)
      load.io.dy := dyRaw.resize(48 bits)
      load.io.dx_subpixel := dxSubRaw
      load.io.dy_subpixel := dySubRaw
      load.io.adjust := paramAdjust
      load.io.dx_negative := dxNeg
      load.io.dy_negative := dyNeg
      acc := load.io.out(197 downto 132).asSInt.resize(accWidth bits)
      dxTerm := load.io.out(131 downto 66).asSInt.resize(accWidth bits)
      dyTerm := load.io.out(65 downto 0).asSInt.resize(accWidth bits)
    }

    def step(dxChunk: UInt, dyChunk: UInt): Unit = {
      val step = new XlsTriangleIterStep
      step.io.accumulator := acc.resize(66 bits)
      step.io.dx_term := dxTerm.resize(66 bits)
      step.io.dy_term := dyTerm.resize(66 bits)
      step.io.dx_chunk := dxChunk
      step.io.dy_chunk := dyChunk
      acc := step.io.out(197 downto 132).asSInt.resize(accWidth bits)
      dxTerm := step.io.out(131 downto 66).asSInt.resize(accWidth bits)
      dyTerm := step.io.out(65 downto 0).asSInt.resize(accWidth bits)
    }

    def resultBits: Bits = acc(startWidth - 1 downto 0).asBits
  }

  val redStart =
    IterChannel(
      pending.grads.redGrad.start,
      pending.grads.redGrad.d(0).raw.asSInt,
      pending.grads.redGrad.d(1).raw.asSInt
    )
  val greenStart = IterChannel(
    pending.grads.greenGrad.start,
    pending.grads.greenGrad.d(0).raw.asSInt,
    pending.grads.greenGrad.d(1).raw.asSInt
  )
  val blueStart = IterChannel(
    pending.grads.blueGrad.start,
    pending.grads.blueGrad.d(0).raw.asSInt,
    pending.grads.blueGrad.d(1).raw.asSInt
  )
  val depthStart = IterChannel(
    pending.grads.depthGrad.start,
    pending.grads.depthGrad.d(0).raw.asSInt,
    pending.grads.depthGrad.d(1).raw.asSInt
  )
  val alphaStart = IterChannel(
    pending.grads.alphaGrad.start,
    pending.grads.alphaGrad.d(0).raw.asSInt,
    pending.grads.alphaGrad.d(1).raw.asSInt
  )
  val wStart = IterChannel(
    pending.grads.wGrad.start,
    pending.grads.wGrad.d(0).raw.asSInt,
    pending.grads.wGrad.d(1).raw.asSInt
  )
  val sStart = IterChannel(
    pending.grads.sGrad.start,
    pending.grads.sGrad.d(0).raw.asSInt,
    pending.grads.sGrad.d(1).raw.asSInt
  )
  val tStart = IterChannel(
    pending.grads.tGrad.start,
    pending.grads.tGrad.d(0).raw.asSInt,
    pending.grads.tGrad.d(1).raw.asSInt
  )
  val hiSStart =
    IterChannel(pending.texHi.sStart, pending.texHi.dSdX.raw.asSInt, pending.texHi.dSdY.raw.asSInt)
  val hiTStart =
    IterChannel(pending.texHi.tStart, pending.texHi.dTdX.raw.asSInt, pending.texHi.dTdY.raw.asSInt)
  val hiAlphaStart = IterChannel(
    pending.hiAlpha.start,
    pending.hiAlpha.dAdX.raw.asSInt,
    pending.hiAlpha.dAdY.raw.asSInt
  )
  val allStarts = Seq(
    redStart,
    greenStart,
    blueStart,
    depthStart,
    alphaStart,
    wStart,
    sStart,
    tStart,
    hiSStart,
    hiTStart,
    hiAlphaStart
  )

  i.ready := state === SetupState.Idle
  o.valid := state === SetupState.Done
  copySetupOutput(o.payload, outputReg)
  o.payload.grads.redGrad.start.raw := redStart.resultBits
  o.payload.grads.greenGrad.start.raw := greenStart.resultBits
  o.payload.grads.blueGrad.start.raw := blueStart.resultBits
  o.payload.grads.depthGrad.start.raw := depthStart.resultBits
  o.payload.grads.alphaGrad.start.raw := alphaStart.resultBits
  o.payload.grads.wGrad.start.raw := wStart.resultBits
  o.payload.grads.sGrad.start.raw := sStart.resultBits
  o.payload.grads.tGrad.start.raw := tStart.resultBits
  o.payload.texHi.sStart.raw := hiSStart.resultBits
  o.payload.texHi.tStart.raw := hiTStart.resultBits
  o.payload.hiAlpha.start.raw := hiAlphaStart.resultBits

  when(state === SetupState.Idle && i.fire) {
    copySetupInput(pending, i.payload)
    state := SetupState.Prep
  }

  when(state === SetupState.Prep) {
    val input = pending
    val tri = input.triWithSign.tri
    val signBit = input.triWithSign.signBit

    val setupWidth = if (tri(0)(0).raw.getWidth == 16) 16 else 20
    val setup = new XlsTriangleSetup(setupWidth)
    setup.io.ax := tri(0)(0).raw.asSInt.resize(setupWidth)
    setup.io.ay := tri(0)(1).raw.asSInt.resize(setupWidth)
    setup.io.bx := tri(1)(0).raw.asSInt.resize(setupWidth)
    setup.io.by := tri(1)(1).raw.asSInt.resize(setupWidth)
    setup.io.cx := tri(2)(0).raw.asSInt.resize(setupWidth)
    setup.io.cy := tri(2)(1).raw.asSInt.resize(setupWidth)
    setup.io.reverse := signBit

    val xrange0 = AFix(c.vertexFormat)
    val xrange1 = AFix(c.vertexFormat)
    val yrange0 = AFix(c.vertexFormat)
    val yrange1 = AFix(c.vertexFormat)
    xrange0.raw := setup.io.out(559 downto 540).resize(xrange0.raw.getWidth bits)
    xrange1.raw := setup.io.out(539 downto 520).resize(xrange1.raw.getWidth bits)
    yrange0.raw := setup.io.out(519 downto 500).resize(yrange0.raw.getWidth bits)
    yrange1.raw := setup.io.out(499 downto 480).resize(yrange1.raw.getWidth bits)

    val coeffsVec = Vec(Coefficients(c), 3)
    for (idx <- 0 until 3) {
      val high = 479 - idx * 120
      coeffsVec(idx).a.raw := setup.io
        .out(high downto high - 39)
        .resize(coeffsVec(idx).a.raw.getWidth bits)
      coeffsVec(idx).b.raw := setup.io
        .out(high - 40 downto high - 79)
        .resize(coeffsVec(idx).b.raw.getWidth bits)
      coeffsVec(idx).c.raw := setup.io
        .out(high - 80 downto high - 119)
        .resize(coeffsVec(idx).c.raw.getWidth bits)
    }
    val edgeStartVec = Vec(AFix(c.coefficientFormat), 3)
    edgeStartVec(0).raw := setup.io.out(119 downto 80).resize(edgeStartVec(0).raw.getWidth bits)
    edgeStartVec(1).raw := setup.io.out(79 downto 40).resize(edgeStartVec(1).raw.getWidth bits)
    edgeStartVec(2).raw := setup.io.out(39 downto 0).resize(edgeStartVec(2).raw.getWidth bits)

    val paramAdjust = input.config.fbzColorPath.paramAdjust
    val fracAx = tri(0)(0).raw(3 downto 0).asUInt
    val dxSubRaw = (U(8, 5 bits) - fracAx.resize(5 bits))(3 downto 0)
    val fracAy = tri(0)(1).raw(3 downto 0).asUInt
    val dySubRaw = (U(8, 5 bits) - fracAy.resize(5 bits))(3 downto 0)

    val startXPix = xrange0.floor(0).asSInt
    val startYPix = yrange0.floor(0).asSInt
    val aXPix = tri(0)(0).roundHalfDown(0).asSInt
    val aYPix = tri(0)(1).roundHalfDown(0).asSInt
    val dxPix = (startXPix - aXPix).resize(16 bits)
    val dyPix = (startYPix - aYPix).resize(16 bits)

    outputReg.coeffs := coeffsVec
    outputReg.xrange(0) := xrange0
    outputReg.xrange(1) := xrange1
    outputReg.yrange(0) := yrange0
    outputReg.yrange(1) := yrange1
    outputReg.grads.all.zip(input.grads.all).foreach { case (outG, inG) =>
      outG.start.raw := inG.start.raw
      outG.d := inG.d
    }
    outputReg.texHi := input.texHi
    outputReg.hiAlpha := input.hiAlpha
    outputReg.edgeStart := edgeStartVec
    outputReg.config := input.config
    if (c.trace.enabled) copyTrace(outputReg.trace, input.trace)

    val dxNeg = dxPix.msb
    val dyNeg = dyPix.msb
    val dxMagnitude = new XlsSignedMagnitude16
    val dyMagnitude = new XlsSignedMagnitude16
    dxMagnitude.io.value := dxPix
    dyMagnitude.io.value := dyPix
    dxMagWork := dxMagnitude.io.out
    dyMagWork := dyMagnitude.io.out
    allStarts.foreach(_.load(dxSubRaw, dySubRaw, paramAdjust, dxNeg, dyNeg))
    iterIndex := 0
    state := SetupState.Iterate
  }

  when(state === SetupState.Iterate) {
    val dxChunk = dxMagWork(1 downto 0)
    val dyChunk = dyMagWork(1 downto 0)
    allStarts.foreach(_.step(dxChunk, dyChunk))
    dxMagWork := (dxMagWork >> 2).resize(16 bits)
    dyMagWork := (dyMagWork >> 2).resize(16 bits)
    when(iterIndex === 7) {
      state := SetupState.Done
    }.otherwise {
      iterIndex := iterIndex + 1
    }
  }

  when(state === SetupState.Done && o.ready) {
    state := SetupState.Idle
  }

  GenerationFlags.formal {
    when(o.valid) {
      val tri = pending.triWithSign.tri
      val minX = tri.map(_(0)).reduceBalancedTree((a, b) => a.min(b))
      val maxX = tri.map(_(0)).reduceBalancedTree((a, b) => a.max(b))

      assert(o.payload.xrange(0) <= minX)
      assert(o.payload.xrange(1) >= maxX)
      assert(o.payload.xrange(1) >= o.payload.xrange(0))
    }
  }
}

object TriangleSetup {
  def buildInput(
      c: Config,
      ax: AFix,
      ay: AFix,
      bx: AFix,
      by: AFix,
      cx: AFix,
      cy: AFix,
      signBit: Bool,
      grads: Rasterizer.GradientBundle[Rasterizer.InputGradient],
      hiAlpha: HiAlpha,
      texHi: HiTexCoords,
      config: PerTriangleConfig,
      traceDrawId: UInt,
      tracePrimitiveId: UInt
  ): Input = {
    val out = Input(c)
    out.triWithSign.tri(0)(0).raw := ax.asBits
    out.triWithSign.tri(0)(1).raw := ay.asBits
    out.triWithSign.tri(1)(0).raw := bx.asBits
    out.triWithSign.tri(1)(1).raw := by.asBits
    out.triWithSign.tri(2)(0).raw := cx.asBits
    out.triWithSign.tri(2)(1).raw := cy.asBits
    out.triWithSign.signBit := signBit
    out.grads := grads
    out.hiAlpha := hiAlpha
    out.texHi := texHi
    out.config := config
    if (c.trace.enabled) {
      out.trace.valid := True
      out.trace.origin := Trace.Origin.triangle
      out.trace.drawId := traceDrawId
      out.trace.primitiveId := tracePrimitiveId
    }
    out
  }

  case class HiAlpha(c: Config) extends Bundle {
    val start = AFix(c.texCoordsHiFormat)
    val dAdX = AFix(c.texCoordsHiFormat)
    val dAdY = AFix(c.texCoordsHiFormat)
  }

  case class HiTexCoords(c: Config) extends Bundle {
    val sStart = AFix(c.texCoordsHiFormat)
    val tStart = AFix(c.texCoordsHiFormat)
    val dSdX = AFix(c.texCoordsHiFormat)
    val dTdX = AFix(c.texCoordsHiFormat)
    val dSdY = AFix(c.texCoordsHiFormat)
    val dTdY = AFix(c.texCoordsHiFormat)
  }

  case class PrefetchConfig(c: Config) extends Bundle {}

  case class PixelPipelineConfig(c: Config) extends Bundle {
    val fbzColorPath = FbzColorPath()
    val fogMode = FogMode()
    val alphaMode = AlphaMode()
  }

  object PixelPipelineConfig {
    def fromPerTriangle(c: Config, src: PerTriangleConfig): PixelPipelineConfig = {
      val out = PixelPipelineConfig(c)
      out.fbzColorPath := src.fbzColorPath
      out.fogMode := src.fogMode
      out.alphaMode := src.alphaMode
      out
    }
  }

  case class TmuConfig(c: Config) extends Bundle {
    val textureEnable = Bool()
    val textureMode = Bits(12 bits)
    val texBaseAddr = UInt(24 bits)
    val texBaseAddr1 = UInt(24 bits)
    val texBaseAddr2 = UInt(24 bits)
    val texBaseAddr38 = UInt(24 bits)
    val tLOD = Bits(27 bits)
    val dSdX = AFix(c.texCoordsFormat)
    val dTdX = AFix(c.texCoordsFormat)
    val dSdY = AFix(c.texCoordsFormat)
    val dTdY = AFix(c.texCoordsFormat)
    val texTables = if (c.packedTexLayout) TexLayoutTables.Tables() else null
  }

  object TmuConfig {
    def fromPerTriangle(c: Config, src: PerTriangleConfig): TmuConfig = {
      val out = TmuConfig(c)
      out.textureEnable := src.fbzColorPath.textureEnable
      out.textureMode := src.tmuTextureMode
      out.texBaseAddr := src.tmuTexBaseAddr
      out.texBaseAddr1 := src.tmuTexBaseAddr1
      out.texBaseAddr2 := src.tmuTexBaseAddr2
      out.texBaseAddr38 := src.tmuTexBaseAddr38
      out.tLOD := src.tmuTLOD
      out.dSdX := src.tmudSdX
      out.dTdX := src.tmudTdX
      out.dSdY := src.tmudSdY
      out.dTdY := src.tmudTdY
      if (c.packedTexLayout) out.texTables := src.texTables
      out
    }
  }

  /** Per-triangle configuration captured at command time. These are registers with FIFO=Yes,
    * Sync=No in the datasheet, meaning they must be captured when the triangle command is issued to
    * avoid synchronization hazards.
    */
  case class PerTriangleConfig(c: Config) extends Bundle {
    // FBI registers
    val fbzColorPath = FbzColorPath() // Color path control (FBI + TREX)
    val fogMode = FogMode() // Fog mode control (FBI)
    val alphaMode = AlphaMode() // Alpha mode control (FBI)
    val enableClipping = Bool()
    val clipLeft = UInt(10 bits)
    val clipRight = UInt(10 bits)
    val clipLowY = UInt(10 bits)
    val clipHighY = UInt(10 bits)

    // TMU registers (single TMU support - Voodoo 1 level functionality)
    val tmuTextureMode = Bits(12 bits) // Texture mode bits used by the pixel/TMU path
    val tmuTexBaseAddr = UInt(24 bits)
    val tmuTexBaseAddr1 = UInt(24 bits)
    val tmuTexBaseAddr2 = UInt(24 bits)
    val tmuTexBaseAddr38 = UInt(24 bits)
    val tmuTLOD = Bits(27 bits)
    // TMU texture coordinate gradients for LOD calculation
    val tmudSdX = AFix(c.texCoordsFormat)
    val tmudTdX = AFix(c.texCoordsFormat)
    val tmudSdY = AFix(c.texCoordsFormat)
    val tmudTdY = AFix(c.texCoordsFormat)

    // Packed texture layout tables (computed at triangle capture time)
    val texTables = if (c.packedTexLayout) TexLayoutTables.Tables() else null
  }

  /** Input bundle - triangle with gradients and render config captured at command time */
  case class Input(c: Config) extends Bundle {
    val triWithSign = TriangleWithSign(c.vertexFormat)
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)
    val hiAlpha = HiAlpha(c)
    val texHi = HiTexCoords(c)
    val config = PerTriangleConfig(c)
    val trace = if (c.trace.enabled) Trace.PrimitiveKey() else null
  }

  /** Output bundle - includes computed edge setup, pass-through gradients and config */
  case class Output(c: Config) extends Bundle {
    val coeffs = Vec.fill(3)(Coefficients(c))
    val xrange = vertex2d(c.vertexFormat)
    val yrange = vertex2d(c.vertexFormat)
    val edgeStart = Vec.fill(3)(AFix(c.coefficientFormat))
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)
    val hiAlpha = HiAlpha(c)
    val texHi = HiTexCoords(c)
    val config = PerTriangleConfig(c)
    val trace = if (c.trace.enabled) Trace.PrimitiveKey() else null
  }
}
