package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._

case class TriangleSetup(c: Config, formalStrong: Boolean = false) extends Component {
  val i = slave(Stream(TriangleSetup.Input(c)))
  val o = master(Stream(TriangleSetup.Output(c)))

  o << i.map { input =>
    val tri = input.triWithSign.tri
    val signBit = input.triWithSign.signBit
    val out = cloneOf(o.payload)

    // Compute bounding box from raw vertex coordinates
    val xminRaw = tri.map(_(0)).reduceBalancedTree((a, b) => a.min(b))
    val xmaxRaw = tri.map(_(0)).reduceBalancedTree((a, b) => a.max(b))
    val yminRaw = tri.map(_(1)).reduceBalancedTree((a, b) => a.min(b))
    val ymaxRaw = tri.map(_(1)).reduceBalancedTree((a, b) => a.max(b))

    // Bounding box: snap to integer pixel positions
    out.xrange(0) := xminRaw.floor(0).fixTo(c.vertexFormat)
    out.xrange(1) := xmaxRaw.ceil(0).fixTo(c.vertexFormat)

    // Y range: match 86Box's scanline computation
    // 86Box: ystart = (vertexAy + 7) >> 4, yend = (vertexCy + 7) >> 4
    // Vertices are sorted A=top, C=bottom by the driver.
    // (raw + 7) >> 4 on a 12.4 value = roundHalfDown to integer (ties toward -inf)
    out.yrange(0) := tri(0)(1).roundHalfDown(0).fixTo(c.vertexFormat)
    out.yrange(1) := tri(2)(1).roundHalfDown(0).fixTo(c.vertexFormat)

    // Compute edge coefficients for all 3 edges
    // Note: Our formula produces inverted signs. For CCW triangles (signBit=0),
    // we need to negate coefficients to get positive values inside the triangle.
    val coeffsVec = Vec(Coefficients(c), 3)
    Seq((0, 1), (1, 2), (2, 0)).zipWithIndex.foreach { case ((i0, i1), idx) =>
      val v0 = tri(i0)
      val v1 = tri(i1)

      val a_raw = v0(1) - v1(1)
      val b_raw = v1(0) - v0(0)

      // Voodoo hardware uses span-based rasterization with inclusive-left fill rule,
      // not the OpenGL/D3D top-left edge function fill rule. With pixel-center sampling
      // (bounding box offset by +0.5), the edge function naturally produces >= 0 for
      // pixels whose centers are inside the triangle span. No fill bias is needed.
      //
      // Compute c using raw coefficients
      // Standard formula: c = x0*y1 - x1*y0
      val c_raw = v0(0) * v1(1) - v1(0) * v0(1)

      // Flip signs for CW triangles (signBit=1) to match rasterizer expectation (edge >= 0 inside)
      // CCW (signBit=0): use raw coefficients (naturally positive inside)
      // CW (signBit=1): negate coefficients (to make positive inside)
      val a_neg = -a_raw
      val b_neg = -b_raw
      val c_neg = -c_raw

      val a_coeff = Mux(signBit, a_neg, a_raw)
      val b_coeff = Mux(signBit, b_neg, b_raw)
      val c_coeff = Mux(signBit, c_neg, c_raw)

      // Use fixTo() to properly convert formats with correct fractional bit scaling
      // a,b are SQ(16,4) -> SQ(32,8) needs left shift by 4
      // c is already SQ(32,8) -> SQ(32,8) just needs truncation
      coeffsVec(idx).a := a_coeff.fixTo(c.coefficientFormat)
      coeffsVec(idx).b := b_coeff.fixTo(c.coefficientFormat)
      coeffsVec(idx).c := c_coeff.fixTo(c.coefficientFormat)
    }
    out.coeffs := coeffsVec

    // Compute starting edge values at pixel center (bounding box origin + 0.5)
    // The rasterizer samples at pixel centers (0.5, 1.5, 2.5, ...) to match the
    // Voodoo hardware's span-based fill convention. We evaluate the edge function
    // at (xrange(0)+0.5, yrange(0)+0.5) so that the >= 0 test correctly includes
    // pixels whose centers are inside the triangle. This is critical for grDrawPoint
    // degenerate triangles where vertices land exactly at pixel centers.
    // The +0.5 is NOT applied to xrange/yrange themselves because the gradient
    // origin shift (dx = xrange(0) - Ax) must use integer positions to avoid
    // double-counting with paramAdjust's subpixel correction.
    val halfPixel = AFix.SQ(3 bits, 4 bits)
    halfPixel := 0.5
    val edgeStartVec = Vec(AFix(c.coefficientFormat), 3)
    val xCenter = (out.xrange(0) + halfPixel).fixTo(c.vertexFormat)
    val yCenter = (out.yrange(0) + halfPixel).fixTo(c.vertexFormat)
    out.coeffs.zipWithIndex.foreach { case (coeff, i) =>
      edgeStartVec(i) := (coeff.a * xCenter + coeff.b * yCenter + coeff.c).truncated
    }
    out.edgeStart := edgeStartVec

    // Subpixel correction (FBZ_PARAM_ADJUST = fbzColorPath bit 26)
    // When enabled, adjusts gradient start values for the sub-pixel offset of vertex A.
    // dx_sub = (8 - fracAx) mod 16, dy_sub = (8 - fracAy) mod 16
    // correction = (dx_sub * dGrad_dX + dy_sub * dGrad_dY) >> 4
    val paramAdjust = input.config.fbzColorPath.paramAdjust
    val fracAx = tri(0)(0).raw(3 downto 0).asUInt // 4-bit fractional part of vertex A x
    val dxSubRaw = (U(8, 5 bits) - fracAx.resize(5 bits))(3 downto 0) // (8 - frac) mod 16
    val fracAy = tri(0)(1).raw(3 downto 0).asUInt
    val dySubRaw = (U(8, 5 bits) - fracAy.resize(5 bits))(3 downto 0)
    val dxSubS = (False ## dxSubRaw).asSInt // 5-bit signed (0-15)
    val dySubS = (False ## dySubRaw).asSInt

    // 86Box applies subpixel correction first, then shifts gradients by integer pixel deltas
    // from rounded vertex A to the raster start point.
    val startXPix = out.xrange(0).floor(0).asSInt
    val startYPix = out.yrange(0).floor(0).asSInt
    val aXPix = tri(0)(0).roundHalfDown(0).asSInt
    val aYPix = tri(0)(1).roundHalfDown(0).asSInt
    val dxPix = (startXPix - aXPix).resize(16 bits)
    val dyPix = (startYPix - aYPix).resize(16 bits)

    out.grads.all.zip(input.grads.all).foreach { case (outG, inG) =>
      outG.d := inG.d // pass through per-pixel gradients unchanged

      // Subpixel correction: (dxSub * dX_raw + dySub * dY_raw) >> 4
      val dxGrad = inG.d(0).raw.asSInt
      val dyGrad = inG.d(1).raw.asSInt
      val corrRaw = (dxSubS * dxGrad + dySubS * dyGrad) >> 4

      // Conditionally apply: wrapping add to match 86Box trunc32 behavior
      val startRaw = inG.start.raw.asSInt
      val N = startRaw.getWidth
      val correctedRaw = (startRaw + corrRaw.resize(N))(N - 1 downto 0)
      val adjustedStart = cloneOf(inG.start)
      adjustedStart.raw := Mux(paramAdjust, correctedRaw.asBits, inG.start.raw)

      // Then apply integer origin shift with wrapping semantics (matches 86Box style).
      val shiftedRaw = (adjustedStart.raw.asSInt +
        (dxPix * dxGrad + dyPix * dyGrad).resize(N))(N - 1 downto 0)
      outG.start.raw := shiftedRaw.asBits
    }

    out.texHi.dSdX := input.texHi.dSdX
    out.texHi.dTdX := input.texHi.dTdX
    out.texHi.dSdY := input.texHi.dSdY
    out.texHi.dTdY := input.texHi.dTdY
    out.hiAlpha.dAdX := input.hiAlpha.dAdX
    out.hiAlpha.dAdY := input.hiAlpha.dAdY

    for (
      (outStart, inStart, dxRaw, dyRaw) <- Seq(
        (
          out.texHi.sStart,
          input.texHi.sStart,
          input.texHi.dSdX.raw.asSInt,
          input.texHi.dSdY.raw.asSInt
        ),
        (
          out.texHi.tStart,
          input.texHi.tStart,
          input.texHi.dTdX.raw.asSInt,
          input.texHi.dTdY.raw.asSInt
        )
      )
    ) {
      val corrRaw = (dxSubS * dxRaw + dySubS * dyRaw) >> 4
      val startRaw = inStart.raw.asSInt
      val N = startRaw.getWidth
      val correctedRaw = (startRaw + corrRaw.resize(N))(N - 1 downto 0)
      val adjustedStart = cloneOf(inStart)
      adjustedStart.raw := Mux(paramAdjust, correctedRaw.asBits, inStart.raw)

      val shiftedRaw = (adjustedStart.raw.asSInt +
        (dxPix * dxRaw + dyPix * dyRaw).resize(N))(N - 1 downto 0)
      outStart.raw := shiftedRaw.asBits
    }

    {
      val dxRaw = input.hiAlpha.dAdX.raw.asSInt
      val dyRaw = input.hiAlpha.dAdY.raw.asSInt
      val corrRaw = (dxSubS * dxRaw + dySubS * dyRaw) >> 4
      val startRaw = input.hiAlpha.start.raw.asSInt
      val N = startRaw.getWidth
      val correctedRaw = (startRaw + corrRaw.resize(N))(N - 1 downto 0)
      val adjustedStart = cloneOf(input.hiAlpha.start)
      adjustedStart.raw := Mux(paramAdjust, correctedRaw.asBits, input.hiAlpha.start.raw)

      val shiftedRaw = (adjustedStart.raw.asSInt +
        (dxPix * dxRaw + dyPix * dyRaw).resize(N))(N - 1 downto 0)
      out.hiAlpha.start.raw := shiftedRaw.asBits
    }

    out.config := input.config
    if (c.trace.enabled) {
      out.trace := input.trace
    }

    GenerationFlags.formal {
      when(i.valid) {
        val tri = input.triWithSign.tri

        // Assert bounding box properties
        val minX = tri.map(_(0)).reduceBalancedTree((a, b) => a.min(b))
        val maxX = tri.map(_(0)).reduceBalancedTree((a, b) => a.max(b))

        // Bounding box xrange must bound all vertices
        assert(out.xrange(0) <= minX)
        assert(out.xrange(1) >= maxX)
        assert(out.xrange(1) >= out.xrange(0))
      }
    }

    out
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
