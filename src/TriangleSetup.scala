package voodoo

import spinal.core._
import spinal.lib._

case class TriangleSetup(c: Config) extends Component {
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

    // Convert to integer pixel positions for rasterization
    // The rasterizer iterates through integer pixels, so bounds must be integers
    // floor(min) gives first pixel that could be inside
    // ceil(max) gives last pixel that could be inside (handles subpixel vertices)
    out.xrange(0) := xminRaw.floor(0).fixTo(c.vertexFormat)
    out.xrange(1) := xmaxRaw.ceil(0).fixTo(c.vertexFormat)

    // Y range: match 86Box's scanline computation
    // 86Box: ystart = (vertexAy + 7) >> 4, yend = (vertexCy + 7) >> 4
    // Vertices are sorted A=top, C=bottom by the driver.
    // (raw + 7) >> 4 on a 12.4 value = roundHalfDown to integer (ties toward -inf)
    out.yrange(0) := tri(0)(1).roundHalfDown(0).fixTo(c.vertexFormat) // ystart (inclusive)
    out.yrange(1) := tri(2)(1).roundHalfDown(0).fixTo(c.vertexFormat) // yend (exclusive)

    // Compute edge coefficients for all 3 edges
    // Note: Our formula produces inverted signs. For CCW triangles (signBit=0),
    // we need to negate coefficients to get positive values inside the triangle.
    val coeffsVec = Vec(Coefficients(c), 3)
    Seq((0, 1), (1, 2), (2, 0)).zipWithIndex.foreach { case ((i0, i1), idx) =>
      val v0 = tri(i0)
      val v1 = tri(i1)

      val a_raw = v0(1) - v1(1)
      val b_raw = v1(0) - v0(0)

      // Top-left fill rule: computed using raw edge orientation (before sign flip)
      val isTopEdge = a_raw === AFix(0) && (b_raw < AFix(0))
      val isLeftEdge = (a_raw > AFix(0))
      val fillBias = AFix(!isTopEdge && !isLeftEdge)

      // Compute c using raw coefficients
      // Standard formula: c = x0*y1 - x1*y0
      val c_raw = v0(0) * v1(1) - v1(0) * v0(1) - fillBias

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

    // Compute starting edge values at the integer pixel position (floored bounding box corner)
    // This ensures edge values match the actual pixel positions being tested by the rasterizer
    // Use coefficient format since edge values can be as large as coefficients
    val edgeStartVec = Vec(AFix(c.coefficientFormat), 3)
    out.coeffs.zipWithIndex.foreach { case (coeff, i) =>
      // xrange(0) and yrange(0) are already floored to integers above
      edgeStartVec(i) := (coeff.a * out.xrange(0) + coeff.b * out.yrange(0) + coeff.c).truncated
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

    // Adjust gradient start values for origin shift from bbox corner to vertex A
    // 86Box uses vertex A as gradient origin; SpinalVoodoo rasterizer starts at (xmin, ymin)
    // adjustedStart = start + (xmin - Ax) * dX + (ymin - Ay) * dY
    val dx = out.xrange(0) - tri(0)(0) // xmin - Ax
    val dy = out.yrange(0) - tri(0)(1) // ymin - Ay

    val gradFormats = Seq(
      c.vColorFormat,
      c.vColorFormat,
      c.vColorFormat,
      c.vDepthFormat,
      c.vColorFormat,
      c.wFormat,
      c.texCoordsFormat,
      c.texCoordsFormat
    )

    out.grads.all.zip(input.grads.all).zip(gradFormats).foreach { case ((outG, inG), fmt) =>
      outG.d := inG.d // pass through per-pixel gradients unchanged

      // Subpixel correction: (dxSub * dX_raw + dySub * dY_raw) >> 4
      val dxGrad = inG.d(0).raw.asSInt
      val dyGrad = inG.d(1).raw.asSInt
      val dxSubS = (False ## dxSubRaw).asSInt // 5-bit signed (0-15)
      val dySubS = (False ## dySubRaw).asSInt
      val corrRaw = (dxSubS * dxGrad + dySubS * dyGrad) >> 4

      // Conditionally apply: wrapping add to match 86Box trunc32 behavior
      val startRaw = inG.start.raw.asSInt
      val N = startRaw.getWidth
      val correctedRaw = (startRaw + corrRaw.resize(N))(N - 1 downto 0)
      val adjustedStart = cloneOf(inG.start)
      adjustedStart.raw := Mux(paramAdjust, correctedRaw.asBits, inG.start.raw)

      // Then apply origin shift (xmin-Ax, ymin-Ay)
      outG.start := (adjustedStart + dx * inG.d(0) + dy * inG.d(1)).fixTo(fmt)
    }

    out.config := input.config

    out
  }
}

object TriangleSetup {

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
    // TMU texture coordinate gradients for LOD calculation
    val tmudSdX = AFix(c.texCoordsFormat)
    val tmudTdX = AFix(c.texCoordsFormat)
    val tmudSdY = AFix(c.texCoordsFormat)
    val tmudTdY = AFix(c.texCoordsFormat)
    // NCC table data (pre-extracted at capture time)
    val ncc = Tmu.NccTableData()

    // Constant colors (captured per-triangle to avoid pipelineBusy gap)
    val color0 = Bits(32 bits)
    val color1 = Bits(32 bits)
    val fogColor = Bits(32 bits)
  }

  /** Input bundle - triangle with gradients and render config captured at command time */
  case class Input(c: Config) extends Bundle {
    val triWithSign = TriangleWithSign(c.vertexFormat)
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)
    val config = PerTriangleConfig(c)
  }

  /** Output bundle - includes computed edge setup, pass-through gradients and config */
  case class Output(c: Config) extends Bundle {
    val coeffs = Vec.fill(3)(Coefficients(c))
    val xrange = vertex2d(c.vertexFormat)
    val yrange = vertex2d(c.vertexFormat)
    val edgeStart = Vec.fill(3)(AFix(c.coefficientFormat))
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)
    val config = PerTriangleConfig(c)
  }
}
