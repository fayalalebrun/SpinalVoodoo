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
  }
}

/** Fog stage — applies fog blending between color combine and alpha test.
  *
  * Combinational transform (no pipeline stages). Fog blends pixel colors toward
  * a fog color based on a fog factor derived from depth, alpha, or W.
  */
case class Fog(c: Config) extends Component {
  val io = new Bundle {
    val input = slave Stream (ColorCombine.Output(c))
    val output = master Stream (Fog.Output(c))
    val fogMode = in Bits (32 bits)
    val fogColor = in Bits (32 bits)
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

  // Fog mode bits
  val fogEnable = io.fogMode(0)
  val fogAdd = io.fogMode(1)
  val fogMult = io.fogMode(2)
  val fogConstant = io.fogMode(5)
  val fogModeSelect = io.fogMode(4 downto 3).asUInt

  // Fog color components
  val fogColorR = io.fogColor(23 downto 16).asUInt
  val fogColorG = io.fogColor(15 downto 8).asUInt
  val fogColorB = io.fogColor(7 downto 0).asUInt

  // Source color (from color combine)
  val srcR = payload.color.r
  val srcG = payload.color.g
  val srcB = payload.color.b

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

  // --- W-table interpolation ---
  val tableIdx = wDepth(15 downto 10)   // 6-bit index
  val tableEntry = io.fogTable(tableIdx)
  val fogBase = tableEntry(7 downto 0).asUInt   // 8-bit unsigned value
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
    is(0) { fogA := wFogFactor }    // W-table lookup
    is(1) { fogA := alphaFogFactor } // FOG_ALPHA
    is(2) { fogA := zFogFactor }     // FOG_Z
    is(3) { fogA := 0 }              // FOG_W direct (rarely used)
  }

  // --- Fog blending ---
  // fogA + 1 for range 1-256 (allows exact 1.0 scaling)
  val fogAPlus1 = (fogA.resize(9 bits) + 1).resize(9 bits)

  // Compute fogged color
  val outR = UInt(8 bits)
  val outG = UInt(8 bits)
  val outB = UInt(8 bits)

  when(!fogEnable) {
    // Fog disabled — pass through
    outR := srcR
    outG := srcG
    outB := srcB
  }.elsewhen(fogConstant) {
    // FOG_CONSTANT: just add fog color
    val addR = (srcR.resize(9 bits) + fogColorR.resize(9 bits))
    val addG = (srcG.resize(9 bits) + fogColorG.resize(9 bits))
    val addB = (srcB.resize(9 bits) + fogColorB.resize(9 bits))
    outR := addR.min(U(255, 9 bits)).resize(8 bits)
    outG := addG.min(U(255, 9 bits)).resize(8 bits)
    outB := addB.min(U(255, 9 bits)).resize(8 bits)
  }.otherwise {
    // Standard fog blending
    // fog_rgb = FOG_ADD ? (0,0,0) : (fogColorR, fogColorG, fogColorB)
    val baseR = SInt(10 bits)
    val baseG = SInt(10 bits)
    val baseB = SInt(10 bits)
    when(fogAdd) {
      baseR := 0; baseG := 0; baseB := 0
    }.otherwise {
      baseR := fogColorR.resize(10 bits).asSInt
      baseG := fogColorG.resize(10 bits).asSInt
      baseB := fogColorB.resize(10 bits).asSInt
    }

    // if !FOG_MULT: fog_rgb -= src_rgb (standard blend: fog - src)
    val diffR = SInt(10 bits)
    val diffG = SInt(10 bits)
    val diffB = SInt(10 bits)
    when(!fogMult) {
      diffR := baseR - srcR.resize(10 bits).asSInt
      diffG := baseG - srcG.resize(10 bits).asSInt
      diffB := baseB - srcB.resize(10 bits).asSInt
    }.otherwise {
      diffR := baseR; diffG := baseG; diffB := baseB
    }

    // result = (diff * (fogA+1)) >> 8
    // fogAPlus1 is UInt(9 bits) range 1-256. Must zero-extend to SInt(10 bits) to avoid sign issues.
    val fogAPlus1Signed = (False ## fogAPlus1).asSInt // SInt(10 bits), always positive
    val mulR = (diffR * fogAPlus1Signed) >> 8
    val mulG = (diffG * fogAPlus1Signed) >> 8
    val mulB = (diffB * fogAPlus1Signed) >> 8

    // if FOG_MULT: out = result; else: out = src + result
    val finalR = SInt(10 bits)
    val finalG = SInt(10 bits)
    val finalB = SInt(10 bits)
    when(fogMult) {
      finalR := mulR.resize(10 bits)
      finalG := mulG.resize(10 bits)
      finalB := mulB.resize(10 bits)
    }.otherwise {
      finalR := srcR.resize(10 bits).asSInt + mulR.resize(10 bits)
      finalG := srcG.resize(10 bits).asSInt + mulG.resize(10 bits)
      finalB := srcB.resize(10 bits).asSInt + mulB.resize(10 bits)
    }

    // Clamp to [0, 255]
    outR := finalR.max(S(0, 10 bits)).min(255).asUInt.resize(8 bits)
    outG := finalG.max(S(0, 10 bits)).min(255).asUInt.resize(8 bits)
    outB := finalB.max(S(0, 10 bits)).min(255).asUInt.resize(8 bits)
  }

  io.output.payload.color.r := outR
  io.output.payload.color.g := outG
  io.output.payload.color.b := outB
}
