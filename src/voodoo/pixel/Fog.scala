package voodoo.pixel

import voodoo._
import spinal.core._
import spinal.lib._

/** Fog output bundle — same as CC Output but without rawW/iteratedAlpha (consumed by fog) */
object Fog {
  case class WStage(c: Config) extends Bundle {
    val payload = Output(c)
    val fogRgb = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val fogEnable = Bool()
    val fogAdd = Bool()
    val fogMult = Bool()
    val fogConstant = Bool()
    val fogModeSelect = UInt(2 bits)
    val zFogFactor = UInt(8 bits)
    val alphaFogFactor = UInt(8 bits)
  }

  case class Stage1(c: Config) extends Bundle {
    val payload = Output(c)
    val fogA = UInt(8 bits)
    val fogRgb = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val fogEnable = Bool()
    val fogAdd = Bool()
    val fogMult = Bool()
    val fogConstant = Bool()
  }

  case class Output(c: Config) extends Bundle {
    val coords = PixelCoords(c)
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
    val routing = FbRouting(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null

    def drawColorBufferBase: UInt = routing.colorBaseAddr
    def drawAuxBufferBase: UInt = routing.auxBaseAddr
    def fbPixelStride: UInt = routing.pixelStride
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
    val busy = out Bool ()
  }

  val payload = io.input.payload

  def assignFogOutputPayload(dst: Fog.Output, src: Fog.Output): Unit = {
    dst.coords := src.coords
    dst.alpha := src.alpha
    dst.depth := src.depth
    dst.wDepth := src.wDepth
    dst.colorBeforeFog := src.colorBeforeFog
    dst.chromaKey := src.chromaKey
    dst.zaColor := src.zaColor
    dst.alphaMode := src.alphaMode
    dst.fbzMode := src.fbzMode
    dst.routing := src.routing
    if (c.trace.enabled) dst.trace := src.trace
  }

  // Fog mode bits (from per-triangle captured config, not live register)
  val fogEnable = payload.fogMode.fogEnable
  val fogAdd = payload.fogMode.fogAdd
  val fogMult = payload.fogMode.fogMult
  val fogConstant = payload.fogMode.fogConstant
  val fogModeSelect = payload.fogMode.fogModeSelect

  // Fog color as Color bundle (from per-triangle captured config)
  val fogRgb = rgb888Color(payload.fogColor)

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

  val wStage = io.input
    .translateWith {
      val out = Fog.WStage(c)
      out.payload.coords := payload.coords
      out.payload.color := payload.color
      out.payload.alpha := payload.alpha
      out.payload.depth := payload.depth
      out.payload.wDepth := wDepth
      out.payload.colorBeforeFog := payload.color
      out.payload.chromaKey := payload.chromaKey
      out.payload.zaColor := payload.zaColor
      out.payload.alphaMode := payload.alphaMode
      out.payload.fbzMode := payload.fbzMode
      out.payload.routing := payload.routing
      if (c.trace.enabled) {
        out.payload.trace := payload.trace
      }
      out.fogRgb := fogRgb
      out.fogEnable := fogEnable
      out.fogAdd := fogAdd
      out.fogMult := fogMult
      out.fogConstant := fogConstant
      out.fogModeSelect := fogModeSelect
      out.zFogFactor := zFogFactor
      out.alphaFogFactor := alphaFogFactor
      out
    }
    .m2sPipe()

  val stage1 = wStage
    .translateWith {
      val out = Fog.Stage1(c)
      out.payload := wStage.payload.payload

      val tableIdx = wStage.payload.payload.wDepth(15 downto 10)
      val tableEntry = io.fogTable(tableIdx)
      val fogBase = tableEntry(7 downto 0).asUInt
      val fogDelta = tableEntry(15 downto 8).asSInt
      val wBlend = wStage.payload.payload.wDepth(9 downto 2).resize(8 bits)
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

      switch(wStage.payload.fogModeSelect) {
        is(0) { out.fogA := wFogFactor }
        is(1) { out.fogA := wStage.payload.alphaFogFactor }
        is(2) { out.fogA := wStage.payload.zFogFactor }
        default { out.fogA := 0 }
      }

      out.fogRgb := wStage.payload.fogRgb
      out.fogEnable := wStage.payload.fogEnable
      out.fogAdd := wStage.payload.fogAdd
      out.fogMult := wStage.payload.fogMult
      out.fogConstant := wStage.payload.fogConstant
      out
    }
    .m2sPipe()

  val stage2 = stage1
    .translateWith {
      val out = Fog.Output(c)
      assignFogOutputPayload(out, stage1.payload.payload)

      val fogAPlus1 = (stage1.payload.fogA.resize(9 bits) + 1).resize(9 bits)
      val outColor = Color.u8()

      when(!stage1.payload.fogEnable) {
        outColor := stage1.payload.payload.colorBeforeFog
      }.elsewhen(stage1.payload.fogConstant) {
        outColor.assignFromSeq(
          stage1.payload.payload.colorBeforeFog.zipWith(stage1.payload.fogRgb) { (s, f) =>
            (s.resize(9 bits) + f.resize(9 bits)).min(U(255, 9 bits)).resize(8 bits)
          }
        )
      }.otherwise {
        val base = Color.s10()
        when(stage1.payload.fogAdd) {
          base.foreach(_ := 0)
        }.otherwise {
          base.assignFromSeq(stage1.payload.fogRgb.map(_.resize(10 bits).asSInt))
        }

        val diff = Color.s10()
        when(!stage1.payload.fogMult) {
          diff.assignFromSeq(base.zipWith(stage1.payload.payload.colorBeforeFog) { (b, s) =>
            b - s.resize(10 bits).asSInt
          })
        }.otherwise {
          diff := base
        }

        val fogAPlus1Signed = (False ## fogAPlus1).asSInt
        val mulChannels = diff.map(d => ((d * fogAPlus1Signed) >> 8).resize(10 bits))
        val finalColor = Color.s10()
        when(stage1.payload.fogMult) {
          finalColor.assignFromSeq(mulChannels)
        }.otherwise {
          finalColor.assignFromSeq(
            stage1.payload.payload.colorBeforeFog.channels.zip(mulChannels).map { case (s, m) =>
              s.resize(10 bits).asSInt + m
            }
          )
        }
        outColor.assignFromSeq(finalColor.map(clampToU8))
      }

      out.color := outColor
      out
    }
    .m2sPipe()

  io.output << stage2
  io.busy := wStage.valid || stage1.valid || stage2.valid
}
