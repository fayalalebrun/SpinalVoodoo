package voodoo

import spinal.core._
import spinal.lib._

/** Fog output bundle — same as CC Output but without rawW/iteratedAlpha (consumed by fog) */
object Fog {
  case class Output(c: Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val color = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val alpha = UInt(8 bits)
    val depth = AFix(c.vDepthFormat)
    val wDepth = UInt(16 bits) // precomputed wDepth for W-buffer depth mode
    val colorBeforeFog = Color(
      UInt(8 bits),
      UInt(8 bits),
      UInt(8 bits)
    ) // pre-fog color for ACOLORBEFOREFOG blend factor
    val chromaKey = Bits(32 bits)
    val zaColor = Bits(32 bits)

    // Per-triangle FIFO registers (pass-through for alpha test and framebuffer access)
    val alphaMode = AlphaMode()
    val fbzMode = FbzMode()
    val drawColorBufferBase = UInt(c.addressWidth.value bits)
    val drawAuxBufferBase = UInt(c.addressWidth.value bits)
    val fbPixelStride = UInt(11 bits)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }
}

/** Fog stage — applies fog blending between color combine and alpha test.
  *
  * Combinational transform (no pipeline stages). Fog blends pixel colors toward a fog color based
  * on a fog factor derived from depth, alpha, or W.
  */
case class Fog(c: Config) extends Component {
  val io = new Bundle {
    val input = slave Stream (ColorCombine.Output(c))
    val output = master Stream (Fog.Output(c))
    val fogTable = in Vec (Bits(16 bits), 64)
  }

  // Pass-through stream handshake
  io.output.valid := io.input.valid
  io.input.ready := io.output.ready

  val payload = io.input.payload

  // Pass through non-fog fields
  io.output.payload.coords := payload.coords
  io.output.payload.alpha := payload.alpha
  io.output.payload.depth := payload.depth
  io.output.payload.colorBeforeFog := payload.color
  io.output.payload.chromaKey := payload.chromaKey
  io.output.payload.zaColor := payload.zaColor
  io.output.payload.alphaMode := payload.alphaMode
  io.output.payload.fbzMode := payload.fbzMode
  io.output.payload.drawColorBufferBase := payload.drawColorBufferBase
  io.output.payload.drawAuxBufferBase := payload.drawAuxBufferBase
  io.output.payload.fbPixelStride := payload.fbPixelStride
  if (c.trace.enabled) {
    io.output.payload.trace := payload.trace
  }

  // Fog mode bits (from per-triangle captured config, not live register)
  val fogEnable = payload.fogMode.fogEnable
  val fogAdd = payload.fogMode.fogAdd
  val fogMult = payload.fogMode.fogMult
  val fogConstant = payload.fogMode.fogConstant
  val fogModeSelect = payload.fogMode.fogModeSelect

  // Fog color as Color bundle (from per-triangle captured config)
  val fogRgb = Color.u8()
  fogRgb.r := payload.fogColor(23 downto 16).asUInt
  fogRgb.g := payload.fogColor(15 downto 8).asUInt
  fogRgb.b := payload.fogColor(7 downto 0).asUInt

  // --- Fog factor (fogA) selection ---
  // Depth value for Z-based fog: bits [27:20] of the 20.12 depth
  // depth is AFix SQ(32,12), raw is 32 bits signed
  // We want (z >> 20) & 0xFF — z is the raw 32-bit depth value
  val depthRaw = payload.depth.raw.asSInt
  val zFogFactor = (depthRaw >> 20).asUInt.resize(8 bits)

  // Alpha-based fog factor
  val alphaFogFactor = payload.iteratedAlpha

  // W-based wDepth calculation for table lookup
  // hw_w is raw 32-bit SInt from wGrad (SQ(32,30) = 2.30 format)
  // 86Box relationship: 86box_w = hw_w << 2
  val hwW = payload.rawW

  // voodooFls: count leading zeros in 16-bit value (binary search, no loops)
  def voodooFls(v: UInt): UInt = {
    val v0 = v.resize(16 bits)

    val test0 = (v0(15 downto 8) === 0)
    val n0 = test0 ? U(8, 5 bits) | U(0, 5 bits)
    val s0 = test0 ? (v0 |<< 8) | v0

    val test1 = (s0(15 downto 12) === 0)
    val n1 = n0 + (test1 ? U(4, 5 bits) | U(0, 5 bits))
    val s1 = test1 ? (s0 |<< 4) | s0

    val test2 = (s1(15 downto 14) === 0)
    val n2 = n1 + (test2 ? U(2, 5 bits) | U(0, 5 bits))
    val s2 = test2 ? (s1 |<< 2) | s1

    val test3 = !s2(15)
    (n2 + (test3 ? U(1, 5 bits) | U(0, 5 bits))).resize(5 bits)
  }

  // Compute wDepth (matching VoodooReference.depthCalc)
  // 86box_w = hw_w << 2, so:
  //   (86box_w >> 16) & 0xffff = (hw_w << 2)(31 downto 16) = hw_w(29 downto 14) (when top 2 bits are 0)
  //   ~86box_w.toInt = ~(hw_w << 2).toInt
  val wUpper = hwW(29 downto 14).asUInt // (86box_w >> 16) & 0xffff
  val wExp = voodooFls(wUpper)

  // mant = ((~86box_w) >>> (19 - exp)) & 0xfff
  // ~86box_w = ~(hw_w << 2) — we compute this as ~hwW shifted left by 2, but since
  // we only need bits, we can compute ~(hwW << 2) = ~hwW_shifted
  val hwWShifted = (hwW << 2).resize(32 bits)
  val notW = ~hwWShifted

  // Variable right shift by (19 - exp): exp ranges 0-15, so shift ranges 4-19
  val shiftAmount = U(19, 5 bits) - wExp.resize(5 bits)
  val mantissa = (notW.asUInt >> shiftAmount)(11 downto 0)

  // wDepthRaw = (exp << 12) | mant + 1; max value = (15 << 12) | 0xfff + 1 = 0xffff + 1
  val expShifted = UInt(17 bits)
  expShifted := (wExp.resize(17 bits) |<< 12)
  val wDepthRaw = (expShifted | mantissa.resize(17 bits)) + 1
  val wDepthClamped = UInt(16 bits)
  when(wDepthRaw > 0xffff) {
    wDepthClamped := U(0xffff, 16 bits)
  }.otherwise {
    wDepthClamped := wDepthRaw.resize(16 bits)
  }

  // Edge cases: if hw_w negative or bits [31:30] nonzero → wDepth = 0; if wUpper == 0 → wDepth = 0xF001
  val wDepth = UInt(16 bits)
  when(hwW.msb || hwW(30)) {
    wDepth := 0
  }.elsewhen(wUpper === 0) {
    wDepth := U(0xf001, 16 bits)
  }.otherwise {
    wDepth := wDepthClamped.resize(16 bits)
  }

  // Export wDepth for FramebufferAccess W-buffer depth mode
  io.output.payload.wDepth := wDepth

  // --- W-table interpolation ---
  val tableIdx = wDepth(15 downto 10) // 6-bit index
  val tableEntry = io.fogTable(tableIdx)
  val fogBase = tableEntry(7 downto 0).asUInt // 8-bit unsigned value
  val fogDelta = tableEntry(15 downto 8).asSInt // 8-bit signed delta
  val wBlend = wDepth(9 downto 2).resize(8 bits) // 8-bit fraction

  // fogA = clamp(fog_base + ((fog_delta * blend) >> 8), 0, 255)
  val deltaProduct = (fogDelta * wBlend.asSInt.resize(16 bits)) >> 8
  val wFogFactorRaw = (fogBase.resize(10 bits).asSInt + deltaProduct.resize(10 bits))
  val wFogFactor = UInt(8 bits)
  when(wFogFactorRaw < 0) {
    wFogFactor := 0
  }.elsewhen(wFogFactorRaw > 255) {
    wFogFactor := 255
  }.otherwise {
    wFogFactor := wFogFactorRaw.asUInt.resize(8 bits)
  }

  // Select fog factor based on mode
  val fogA = UInt(8 bits)
  switch(fogModeSelect) {
    is(0) { fogA := wFogFactor } // W-table lookup
    is(1) { fogA := alphaFogFactor } // FOG_ALPHA
    is(2) { fogA := zFogFactor } // FOG_Z
    is(3) { fogA := 0 } // FOG_W direct (rarely used)
  }

  // --- Fog blending ---
  // fogA + 1 for range 1-256 (allows exact 1.0 scaling)
  val fogAPlus1 = (fogA.resize(9 bits) + 1).resize(9 bits)

  // Compute fogged color
  val outColor = Color.u8()

  when(!fogEnable) {
    outColor := payload.color
  }.elsewhen(fogConstant) {
    outColor.assignFromSeq(payload.color.zipWith(fogRgb) { (s, f) =>
      (s.resize(9 bits) + f.resize(9 bits)).min(U(255, 9 bits)).resize(8 bits)
    })
  }.otherwise {
    // Standard fog blending
    val base = Color.s10()
    when(fogAdd) {
      base.foreach(_ := 0)
    }.otherwise {
      base.assignFromSeq(fogRgb.map(_.resize(10 bits).asSInt))
    }

    val diff = Color.s10()
    when(!fogMult) {
      diff.assignFromSeq(base.zipWith(payload.color) { (b, s) =>
        b - s.resize(10 bits).asSInt
      })
    }.otherwise {
      diff := base
    }

    // fogAPlus1 is UInt(9 bits) range 1-256. Must zero-extend to SInt(10 bits) to avoid sign issues.
    val fogAPlus1Signed = (False ## fogAPlus1).asSInt // SInt(10 bits), always positive
    val mulChannels = diff.map(d => ((d * fogAPlus1Signed) >> 8).resize(10 bits))

    val finalColor = Color.s10()
    when(fogMult) {
      finalColor.assignFromSeq(mulChannels)
    }.otherwise {
      finalColor.assignFromSeq(
        payload.color.channels.zip(mulChannels).map { case (s, m) =>
          s.resize(10 bits).asSInt + m
        }
      )
    }

    // Clamp to [0, 255]
    outColor.assignFromSeq(finalColor.map(clampToU8))
  }

  io.output.payload.color := outColor
}
