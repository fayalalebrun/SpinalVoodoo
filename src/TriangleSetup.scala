package voodoo

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

  def mulBySubpixel(value: SInt, coeff: UInt): SInt = {
    val width = value.getWidth + coeff.getWidth
    val zero = S(0, width bits)
    val terms = (0 until coeff.getWidth).map { bit =>
      val shifted = (value.resize(width bits) |<< bit).resize(width bits)
      Mux(coeff(bit), shifted, zero)
    }
    terms.reduce(_ + _).resize(width bits)
  }

  def signedMagnitude(value: SInt): UInt = {
    val wide = value.resize(value.getWidth + 1 bits)
    Mux(
      wide.msb,
      (-wide).asUInt.resize(value.getWidth bits),
      wide.asUInt.resize(value.getWidth bits)
    )
  }

  def signAdjust(value: SInt, negate: Bool, width: Int): SInt = {
    val resized = value.resize(width bits)
    Mux(negate, (-resized).resize(width bits), resized)
  }

  def scaleByChunk(term: SInt, chunk: UInt): SInt = {
    val scaled = SInt(term.getWidth bits)
    scaled := 0
    switch(chunk) {
      is(U"2'b01") {
        scaled := term
      }
      is(U"2'b10") {
        scaled := (term |<< 1).resize(term.getWidth bits)
      }
      is(U"2'b11") {
        scaled := (term + (term |<< 1)).resize(term.getWidth bits)
      }
    }
    scaled
  }

  case class IterChannel(inStart: AFix, dxRaw: SInt, dyRaw: SInt) extends Area {
    val startWidth = inStart.raw.getWidth
    val accWidth = startWidth + 18
    val acc = Reg(SInt(accWidth bits)) init (0)
    val dxTerm = Reg(SInt(accWidth bits)) init (0)
    val dyTerm = Reg(SInt(accWidth bits)) init (0)

    def load(dxSubRaw: UInt, dySubRaw: UInt, paramAdjust: Bool, dxNeg: Bool, dyNeg: Bool): Unit = {
      val corrRaw = (mulBySubpixel(dxRaw, dxSubRaw) + mulBySubpixel(dyRaw, dySubRaw)) >> 4
      val startRaw = inStart.raw.asSInt
      val correctedRaw = (startRaw + corrRaw.resize(startWidth bits))(startWidth - 1 downto 0)
      val adjustedBits = Mux(paramAdjust, correctedRaw.asBits, inStart.raw)
      acc := adjustedBits.asSInt.resize(accWidth bits)
      dxTerm := signAdjust(dxRaw, dxNeg, accWidth)
      dyTerm := signAdjust(dyRaw, dyNeg, accWidth)
    }

    def step(dxChunk: UInt, dyChunk: UInt): Unit = {
      val sumWidth = accWidth + 2
      val nextAcc =
        (acc.resize(sumWidth bits) + scaleByChunk(dxTerm, dxChunk).resize(
          sumWidth bits
        ) + scaleByChunk(
          dyTerm,
          dyChunk
        ).resize(sumWidth bits)).resize(accWidth bits)
      acc := nextAcc
      dxTerm := (dxTerm |<< 2).resize(accWidth bits)
      dyTerm := (dyTerm |<< 2).resize(accWidth bits)
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
  for (idx <- 0 until 3) {
    o.payload.coeffs(idx).a := outputReg.coeffs(idx).a
    o.payload.coeffs(idx).b := outputReg.coeffs(idx).b
    o.payload.coeffs(idx).c := outputReg.coeffs(idx).c
    o.payload.edgeStart(idx) := outputReg.edgeStart(idx)
  }
  o.payload.xrange(0) := outputReg.xrange(0)
  o.payload.xrange(1) := outputReg.xrange(1)
  o.payload.yrange(0) := outputReg.yrange(0)
  o.payload.yrange(1) := outputReg.yrange(1)
  o.payload.grads.redGrad.d := outputReg.grads.redGrad.d
  o.payload.grads.greenGrad.d := outputReg.grads.greenGrad.d
  o.payload.grads.blueGrad.d := outputReg.grads.blueGrad.d
  o.payload.grads.depthGrad.d := outputReg.grads.depthGrad.d
  o.payload.grads.alphaGrad.d := outputReg.grads.alphaGrad.d
  o.payload.grads.wGrad.d := outputReg.grads.wGrad.d
  o.payload.grads.sGrad.d := outputReg.grads.sGrad.d
  o.payload.grads.tGrad.d := outputReg.grads.tGrad.d
  o.payload.texHi.dSdX := outputReg.texHi.dSdX
  o.payload.texHi.dTdX := outputReg.texHi.dTdX
  o.payload.texHi.dSdY := outputReg.texHi.dSdY
  o.payload.texHi.dTdY := outputReg.texHi.dTdY
  o.payload.hiAlpha.dAdX := outputReg.hiAlpha.dAdX
  o.payload.hiAlpha.dAdY := outputReg.hiAlpha.dAdY
  o.payload.config := outputReg.config
  if (c.trace.enabled) {
    o.payload.trace := outputReg.trace
  }
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
    pending.triWithSign.signBit := i.payload.triWithSign.signBit
    for (idx <- 0 until 3) {
      pending.triWithSign.tri(idx)(0) := i.payload.triWithSign.tri(idx)(0)
      pending.triWithSign.tri(idx)(1) := i.payload.triWithSign.tri(idx)(1)
    }
    pending.grads.all.zip(i.payload.grads.all).foreach { case (dst, src) =>
      dst.start.raw := src.start.raw
      dst.d(0).raw := src.d(0).raw
      dst.d(1).raw := src.d(1).raw
    }
    pending.hiAlpha.start.raw := i.payload.hiAlpha.start.raw
    pending.hiAlpha.dAdX.raw := i.payload.hiAlpha.dAdX.raw
    pending.hiAlpha.dAdY.raw := i.payload.hiAlpha.dAdY.raw
    pending.texHi.sStart.raw := i.payload.texHi.sStart.raw
    pending.texHi.tStart.raw := i.payload.texHi.tStart.raw
    pending.texHi.dSdX.raw := i.payload.texHi.dSdX.raw
    pending.texHi.dTdX.raw := i.payload.texHi.dTdX.raw
    pending.texHi.dSdY.raw := i.payload.texHi.dSdY.raw
    pending.texHi.dTdY.raw := i.payload.texHi.dTdY.raw
    pending.config := i.payload.config
    if (c.trace.enabled) {
      pending.trace := i.payload.trace
    }
    state := SetupState.Prep
  }

  when(state === SetupState.Prep) {
    val input = pending
    val tri = input.triWithSign.tri
    val signBit = input.triWithSign.signBit

    val xminRaw = tri.map(_(0)).reduceBalancedTree((a, b) => a.min(b))
    val xmaxRaw = tri.map(_(0)).reduceBalancedTree((a, b) => a.max(b))
    val xrange0 = xminRaw.floor(0).fixTo(c.vertexFormat)
    val xrange1 = xmaxRaw.ceil(0).fixTo(c.vertexFormat)
    val yrange0 = tri(0)(1).roundHalfDown(0).fixTo(c.vertexFormat)
    val yrange1 = tri(2)(1).roundHalfDown(0).fixTo(c.vertexFormat)

    val coeffsVec = Vec(Coefficients(c), 3)
    Seq((0, 1), (1, 2), (2, 0)).zipWithIndex.foreach { case ((i0, i1), idx) =>
      val v0 = tri(i0)
      val v1 = tri(i1)

      val a_raw = v0(1) - v1(1)
      val b_raw = v1(0) - v0(0)
      val c_raw = v0(0) * v1(1) - v1(0) * v0(1)

      val a_coeff = Mux(signBit, -a_raw, a_raw)
      val b_coeff = Mux(signBit, -b_raw, b_raw)
      val c_coeff = Mux(signBit, -c_raw, c_raw)

      coeffsVec(idx).a := a_coeff.fixTo(c.coefficientFormat)
      coeffsVec(idx).b := b_coeff.fixTo(c.coefficientFormat)
      coeffsVec(idx).c := c_coeff.fixTo(c.coefficientFormat)
    }

    val halfPixel = AFix.SQ(3 bits, 4 bits)
    halfPixel := 0.5
    val xCenter = (xrange0 + halfPixel).fixTo(c.vertexFormat)
    val yCenter = (yrange0 + halfPixel).fixTo(c.vertexFormat)
    val edgeStartVec = Vec(AFix(c.coefficientFormat), 3)
    coeffsVec.zipWithIndex.foreach { case (coeff, idx) =>
      edgeStartVec(idx) := (coeff.a * xCenter + coeff.b * yCenter + coeff.c).truncated
    }

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

    for (idx <- 0 until 3) {
      outputReg.coeffs(idx).a := coeffsVec(idx).a
      outputReg.coeffs(idx).b := coeffsVec(idx).b
      outputReg.coeffs(idx).c := coeffsVec(idx).c
      outputReg.edgeStart(idx) := edgeStartVec(idx)
    }
    outputReg.xrange(0) := xrange0
    outputReg.xrange(1) := xrange1
    outputReg.yrange(0) := yrange0
    outputReg.yrange(1) := yrange1
    outputReg.grads.all.zip(input.grads.all).foreach { case (outG, inG) =>
      outG.start.raw := inG.start.raw
      outG.d := inG.d
    }
    outputReg.texHi.sStart.raw := input.texHi.sStart.raw
    outputReg.texHi.tStart.raw := input.texHi.tStart.raw
    outputReg.texHi.dSdX := input.texHi.dSdX
    outputReg.texHi.dTdX := input.texHi.dTdX
    outputReg.texHi.dSdY := input.texHi.dSdY
    outputReg.texHi.dTdY := input.texHi.dTdY
    outputReg.hiAlpha.start.raw := input.hiAlpha.start.raw
    outputReg.hiAlpha.dAdX := input.hiAlpha.dAdX
    outputReg.hiAlpha.dAdY := input.hiAlpha.dAdY
    outputReg.config := input.config
    if (c.trace.enabled) {
      outputReg.trace := input.trace
    }

    val dxNeg = dxPix.msb
    val dyNeg = dyPix.msb
    dxMagWork := signedMagnitude(dxPix)
    dyMagWork := signedMagnitude(dyPix)
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

  /** Per-triangle configuration captured at command time. These are registers with FIFO=Yes,
    * Sync=No in the datasheet, meaning they must be captured when the triangle command is issued to
    * avoid synchronization hazards.
    */
  case class PerTriangleConfig(c: Config) extends Bundle {
    // FBI registers
    val fbzColorPath = FbzColorPath() // Color path control (FBI + TREX)
    val fogMode = FogMode() // Fog mode control (FBI)
    val alphaMode = AlphaMode() // Alpha mode control (FBI)
    val fbzMode = FbzMode() // Framebuffer/depth mode control (FBI)

    // TMU registers (single TMU support - Voodoo 1 level functionality)
    val tmuTextureMode = Bits(32 bits) // Texture mode (format, filtering, clamp/wrap)
    val tmuTexBaseAddr = UInt(24 bits)
    val tmuTLOD = Bits(27 bits) // tLOD register for mipmapping
    val tmuSendConfig = Bool()
    // TMU texture coordinate gradients for LOD calculation
    val tmudSdX = AFix(c.texCoordsFormat)
    val tmudTdX = AFix(c.texCoordsFormat)
    val tmudSdY = AFix(c.texCoordsFormat)
    val tmudTdY = AFix(c.texCoordsFormat)
    // NCC table data (pre-extracted at capture time)
    val ncc = Tmu.NccTableData()

    // Packed texture layout tables (computed at triangle capture time)
    val texTables = if (c.packedTexLayout) TexLayoutTables.Tables() else null

    // Constant colors (captured per-triangle to avoid pipelineBusy gap)
    val color0 = Bits(32 bits)
    val color1 = Bits(32 bits)
    val fogColor = Bits(32 bits)
    val chromaKey = Bits(32 bits)
    val zaColor = Bits(32 bits)

    // Framebuffer routing (captured per-triangle so in-flight pixels survive swaps)
    val drawColorBufferBase = UInt(c.addressWidth.value bits)
    val drawAuxBufferBase = UInt(c.addressWidth.value bits)
    val fbPixelStride = UInt(11 bits)
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
