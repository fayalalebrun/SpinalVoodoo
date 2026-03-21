package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

/** Texture Mapping Unit (TMU)
  *
  * Performs texture sampling for a single texture unit. Two TMUs are instantiated and chained
  * sequentially (TMU0 → TMU1) to support multitexturing.
  *
  * Pipeline stages (fully pipelined using Stream fork/join):
  *   1. Perspective correction + coordinate generation (X.4 format with LOD scaling)
  *   2. Expander: point sampling emits 1 texel request, bilinear emits 4
  *   3. Fork: send address to memory, queue passthrough data
  *   4. Join: combine memory response with queued data
  *   5. Format decode: decode texel data to RGBA8888
  *   6. Collector: point passes through, bilinear accumulates 4 texels and blends
  */
case class Tmu(c: voodoo.Config) extends Component {
  val io = new Bundle {
    // Input stream (from Rasterizer for TMU0, from TMU0 for TMU1)
    val input = slave Stream (Tmu.Input(c))

    // Output stream (to TMU1 for TMU0, to ColorCombine for TMU1)
    val output = master Stream (Tmu.Output(c))

    // Texture memory read bus
    val texRead = master(Bmb(Tmu.bmbParams(c)))

    // Palette write port (from Core, driven by NCC table 0 I/Q register writes with bit 31 set)
    val paletteWrite = slave Flow (Tmu.PaletteWrite())

    // Invalidate texture-side fast caches after texture memory writes
    val invalidate = in Bool ()

    // Pipeline busy: pixels in flight inside fork-queue-join and collector
    val busy = out Bool ()
  }

  val inFlightCount = Reg(UInt(5 bits)) init 0

  // ========================================================================
  // Stage 1: Perspective correction + coordinate generation
  // ========================================================================

  // Decode textureMode register
  val texMode = Tmu.TextureMode.decode(io.input.payload.config.textureMode)

  // Perspective Correction
  val oow = io.input.payload.w // 1/W in 2.30 format (wider internal accumulator)
  val sow = io.input.payload.s // S/W in 14.18 format (wider internal accumulator)
  val tow = io.input.payload.t // T/W in 14.18 format (wider internal accumulator)

  // CLZ32: count leading zeros on 32-bit unsigned value (binary search)
  def clz32(v: UInt): UInt = {
    val v0 = v.resize(32 bits)

    val test0 = (v0(31 downto 16) === 0)
    val n0 = test0 ? U(16, 6 bits) | U(0, 6 bits)
    val s0 = test0 ? (v0 |<< 16) | v0

    val test1 = (s0(31 downto 24) === 0)
    val n1 = n0 + (test1 ? U(8, 6 bits) | U(0, 6 bits))
    val s1 = test1 ? (s0 |<< 8) | s0

    val test2 = (s1(31 downto 28) === 0)
    val n2 = n1 + (test2 ? U(4, 6 bits) | U(0, 6 bits))
    val s2 = test2 ? (s1 |<< 4) | s1

    val test3 = (s2(31 downto 30) === 0)
    val n3 = n2 + (test3 ? U(2, 6 bits) | U(0, 6 bits))
    val s3 = test3 ? (s2 |<< 2) | s2

    val test4 = !s3(31)
    (n3 + (test4 ? U(1, 6 bits) | U(0, 6 bits))).resize(6 bits)
  }

  // CLZ64 built from CLZ32
  def clz64(v: UInt): UInt = {
    val v0 = v.resize(64 bits)
    val hi = v0(63 downto 32)
    val lo = v0(31 downto 0)
    val hiZero = hi === 0
    val clzHi = clz32(hi)
    val clzLo = clz32(lo)
    Mux(hiZero, (U(32, 7 bits) + clzLo.resize(7 bits)).resize(7 bits), clzHi.resize(7 bits))
  }

  // Reciprocal LUT: 257 entries, 17-bit values
  // recipTable[i] = round((1 << 24) / (256 + i))
  val recipTable = Vec(UInt(17 bits), 257)
  for (i <- 0 to 256) {
    recipTable(i) := U(scala.math.round((1 << 24).toDouble / (256.0 + i)).toLong, 17 bits)
  }

  // Log2 mantissa table: 256 entries, 8-bit values (from 86Box vid_voodoo_render.c)
  val logTableValues: Array[Int] = Array(
    0x00, 0x01, 0x02, 0x04, 0x05, 0x07, 0x08, 0x09, 0x0b, 0x0c, 0x0e, 0x0f, 0x10, 0x12, 0x13, 0x15,
    0x16, 0x17, 0x19, 0x1a, 0x1b, 0x1d, 0x1e, 0x1f, 0x21, 0x22, 0x23, 0x25, 0x26, 0x27, 0x28, 0x2a,
    0x2b, 0x2c, 0x2e, 0x2f, 0x30, 0x31, 0x33, 0x34, 0x35, 0x36, 0x38, 0x39, 0x3a, 0x3b, 0x3d, 0x3e,
    0x3f, 0x40, 0x41, 0x43, 0x44, 0x45, 0x46, 0x47, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x50, 0x51,
    0x52, 0x53, 0x54, 0x55, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x60, 0x61, 0x62, 0x63,
    0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x83, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94,
    0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa2, 0xa3,
    0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2,
    0xb3, 0xb4, 0xb5, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0,
    0xc1, 0xc2, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xcd,
    0xce, 0xcf, 0xd0, 0xd1, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xda,
    0xdb, 0xdc, 0xdd, 0xde, 0xde, 0xdf, 0xe0, 0xe1, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe8, 0xe9, 0xea, 0xeb, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xef, 0xf0, 0xf1, 0xf2, 0xf2, 0xf3,
    0xf4, 0xf5, 0xf5, 0xf6, 0xf7, 0xf7, 0xf8, 0xf9, 0xfa, 0xfa, 0xfb, 0xfc, 0xfd, 0xfd, 0xfe, 0xff
  )
  val logTable = Vec(UInt(8 bits), 256)
  for (i <- 0 until 256) {
    logTable(i) := U(logTableValues(i), 8 bits)
  }

  // Per-pixel LOD adjustment for perspective correction
  val perspLodAdjustPre = SInt(16 bits)
  perspLodAdjustPre := S(0, 16 bits)

  val requestIdWidth = 5
  val maxOutstanding = (1 << requestIdWidth) - 1

  val recipInterp = UInt(17 bits)
  val recipMsbPos = UInt(7 bits)
  recipInterp := 0
  recipMsbPos := 0

  when(texMode.perspectiveEnable) {
    // Perspective: S = (S/W) / (1/W) via reciprocal LUT
    val oowRaw = oow.raw.asSInt
    val absOow = oowRaw.abs.resize(64 bits)
    val clz = clz64(absOow)
    val msbPos = U(63, 7 bits) - clz // UInt(7 bits)

    // Normalize: shift left so MSB is at bit 63
    val norm = (absOow |<< clz).resize(64 bits)
    val index = norm(62 downto 55) // 8-bit table index
    val frac = norm(54 downto 47) // 8-bit interpolation fraction

    // LUT lookup + linear interpolation
    val base = recipTable(index.resize(9 bits))
    val next = recipTable((index +^ U(1)).resize(9 bits))
    val diff = base - next // UInt(17 bits)
    val interp = base - ((diff * frac) >> 8).resize(17 bits)

    // Handle oow <= 0 (degenerate): reciprocal = 0
    val validOow = !oowRaw.msb && (absOow =/= 0)
    val safeInterp = validOow ? interp | U(0, 17 bits)
    recipInterp := safeInterp
    recipMsbPos := msbPos

    // Per-pixel LOD adjustment matching 86Box:
    //   86Box: _w = (1ULL << 48) / state->tmu0_w, where tmu0_w = raw_W << 2
    //   adjustment = fastlog(_w) - (19 << 8)
    // SpinalVoodoo uses raw_W (no <<2), so:
    //   _w = (1ULL << 48) / (raw_W << 2) = (1ULL << 46) / raw_W
    //   fastlog(_w) = log2(2^46 / raw_W) * 256 = (46 - log2(raw_W)) * 256
    //   adjustment = (46 - log2(raw_W)) * 256 - 19*256 = (27 - log2(raw_W)) * 256
    when(validOow) {
      // Signed subtraction: adjSInt can be negative when raw_W >= 2^27
      val adjSInt = (S(27, 8 bits) - (False ## msbPos).asSInt.resize(8 bits))
      val logFrac = logTable(index)
      // The LUT path trends one LOD unit high near mip boundaries; bias it down slightly
      // to match 86Box's fastlog-based perspective adjustment more closely.
      perspLodAdjustPre := ((adjSInt << 8).resize(16 bits)
        - (False ## U(0, 8 bits) ## logFrac).asSInt.resize(16 bits)
        - S(1, 16 bits))
    }
  }

  // Clamp W (force S=T=0 when W is negative)
  val wNegative = oow.raw.msb
  val clampToZero = texMode.clampW && wNegative

  val (inputForStage, inputForMeta) = StreamFork2(io.input)

  val stageRecip = inputForStage
    .translateWith {
      val out = Tmu.FrontRecip(c)
      out.s := sow
      out.t := tow
      out.tLOD := inputForStage.payload.config.tLOD
      out.interp := recipInterp
      out.msbPos := recipMsbPos
      out.perspectiveEnable := texMode.perspectiveEnable
      out.clampToZero := clampToZero
      out.perspLodAdjust := perspLodAdjustPre
      out.dSdX := inputForStage.payload.dSdX
      out.dTdX := inputForStage.payload.dTdX
      out.dSdY := inputForStage.payload.dSdY
      out.dTdY := inputForStage.payload.dTdY
      out
    }
    .stage()

  val stageA = stageRecip
    .translateWith {
      val texS = SInt(18 bits)
      val texT = SInt(18 bits)
      when(!stageRecip.payload.perspectiveEnable) {
        texS := (stageRecip.payload.s.raw.asSInt >> 14).resized
        texT := (stageRecip.payload.t.raw.asSInt >> 14).resized
      } otherwise {
        val interpSigned = (False ## stageRecip.payload.interp).asSInt
        val productS = stageRecip.payload.s.raw.asSInt * interpSigned
        val productT = stageRecip.payload.t.raw.asSInt * interpSigned
        val roundBias = (S(1, productS.getWidth bits) |<< (stageRecip.payload.msbPos - 1).resize(
          log2Up(productS.getWidth) bits
        ))
        texS := ((productS + roundBias) >> stageRecip.payload.msbPos).resized
        texT := ((productT + roundBias) >> stageRecip.payload.msbPos).resized
      }
      val out = Tmu.FrontA(c)
      out.texS := texS
      out.texT := texT
      out.tLOD := stageRecip.payload.tLOD
      out.perspLodAdjust := stageRecip.payload.perspLodAdjust
      out.clampToZero := stageRecip.payload.clampToZero
      out.dSdX := stageRecip.payload.dSdX
      out.dTdX := stageRecip.payload.dTdX
      out.dSdY := stageRecip.payload.dSdY
      out.dTdY := stageRecip.payload.dTdY
      out
    }
    .m2sPipe()

  val texS = stageA.payload.texS.setName("texS")
  val texT = stageA.payload.texT.setName("texT")
  texS.simPublic()
  texT.simPublic()

  // LOD Calculation
  // Base LOD computation matching 86Box voodoo_queue_triangle:
  //   tempdx = dSdX^2 + dTdX^2
  //   tempdy = dSdY^2 + dTdY^2
  //   tempLOD = max(tempdx, tempdy)
  //   LOD = log2(tempLOD / 2^36) * 256
  //   LOD >>= 2
  //
  // 86Box stores gradients as (val << 14) then shifts back (>> 14) before squaring,
  // so it squares the original 32-bit register values. We must do the same: use the
  // raw 32-bit SQ(14,18) values directly without any pre-shift.
  // The normalization constant 2^36 = (2^18)^2 accounts for the 18-bit fractional
  // part of SQ(14,18), converting "texel-fractions per pixel" to "texels per pixel".

  val dSdX_raw = stageA.payload.dSdX.raw.asSInt
  val dTdX_raw = stageA.payload.dTdX.raw.asSInt
  val dSdY_raw = stageA.payload.dSdY.raw.asSInt
  val dTdY_raw = stageA.payload.dTdY.raw.asSInt

  // Sum of squares: 32*32=64 bit products, sum fits in 64 bits (always positive).
  val tempdx =
    (dSdX_raw * dSdX_raw).resize(64 bits).asUInt + (dTdX_raw * dTdX_raw).resize(64 bits).asUInt
  val tempdy =
    (dSdY_raw * dSdY_raw).resize(64 bits).asUInt + (dTdY_raw * dTdY_raw).resize(64 bits).asUInt
  val tempLOD = Mux(tempdx > tempdy, tempdx, tempdy).resize(64 bits)

  val stageGrad = stageA
    .translateWith {
      val out = Tmu.FrontGrad(c)
      out.texS := stageA.payload.texS
      out.texT := stageA.payload.texT
      out.tLOD := stageA.payload.tLOD
      out.clampToZero := stageA.payload.clampToZero
      out.tempLOD := tempLOD
      out.perspLodAdjust := stageA.payload.perspLodAdjust
      out
    }
    .stage()

  val metaDelay = inputForMeta
    .translateWith {
      val out = Tmu.FrontMeta(c)
      out.config := inputForMeta.payload.config
      if (c.trace.enabled) {
        out.trace := inputForMeta.payload.trace
      }
      out
    }
    .stage()
    .stage()
    .stage()
    .stage()

  val tLOD = Tmu.TLOD.decode(stageGrad.payload.tLOD)

  // Compute log2(tempLOD) in 8.8 format using CLZ + logTable
  val baseLod_8_8 = SInt(16 bits)
  when(stageGrad.payload.tempLOD === 0) {
    baseLod_8_8 := S(0, 16 bits) // Will be clamped to lodmin
  }.otherwise {
    // CLZ on 64-bit value: find MSB position
    val tempLOD32hi = stageGrad.payload.tempLOD(63 downto 32)
    val tempLOD32lo = stageGrad.payload.tempLOD(31 downto 0)
    val useHi = tempLOD32hi =/= 0
    val clzInput = Mux(useHi, tempLOD32hi, tempLOD32lo)
    val clz32val = clz32(clzInput)
    val msbPos64 =
      Mux(useHi, U(63, 7 bits) - clz32val.resize(7 bits), U(31, 7 bits) - clz32val.resize(7 bits))

    // Normalize to get 8-bit index for logTable
    // We need bits [msb-1 : msb-8] of tempLOD
    val shiftAmt = Mux(msbPos64 >= 8, (msbPos64 - 8).resize(6 bits), U(0, 6 bits))
    val shifted = (stageGrad.payload.tempLOD >> shiftAmt).resize(16 bits)
    val lodIndex = shifted(7 downto 0)

    // log2(tempLOD) * 256 = msbPos64 * 256 + logTable[index]
    // Subtract 36*256 for the /2^36 normalization (matching 86Box)
    // Then >>2 to match 86Box's LOD >>= 2
    val rawLod = ((False ## msbPos64).asSInt.resize(16 bits) << 8).resize(16 bits) +
      (False ## U(0, 8 bits) ## logTable(lodIndex)).asSInt.resize(16 bits) -
      S(36 * 256, 16 bits)
    baseLod_8_8 := (rawLod >> 2).resize(16 bits)
  }

  // 8.8 fixed-point LOD pipeline
  val lodbias_8_8 = (tLOD.lodbias.asSInt << 6).resize(16 bits)
  val lod_8_8 = baseLod_8_8 + stageGrad.payload.perspLodAdjust + lodbias_8_8

  val lodMin_8_8 = (tLOD.lodmin << 6).resize(12 bits)
  val lodMax_8_8_raw = (tLOD.lodmax << 6).resize(12 bits)
  val lodMax_8_8 = Mux(lodMax_8_8_raw > U(0x800, 12 bits), U(0x800, 12 bits), lodMax_8_8_raw)

  val lodClamped_8_8 = SInt(16 bits)
  when(lod_8_8 < (False ## lodMin_8_8).asSInt.resize(16 bits)) {
    lodClamped_8_8 := (False ## lodMin_8_8).asSInt.resize(16 bits)
  }.elsewhen(lod_8_8 > (False ## lodMax_8_8).asSInt.resize(16 bits)) {
    lodClamped_8_8 := (False ## lodMax_8_8).asSInt.resize(16 bits)
  }.otherwise {
    lodClamped_8_8 := lod_8_8
  }

  val lodInt = (lodClamped_8_8 >> 8).resize(16 bits)
  val lodLevel = UInt(4 bits)
  when(lodInt < 0) {
    lodLevel := 0
  }.elsewhen(lodInt > 8) {
    lodLevel := 8
  }.otherwise {
    lodLevel := lodInt.asUInt.resize(4 bits)
  }

  // lodDimBits = log2 of the wider mipmap dimension at this LOD level
  // SST1 convention: lodLevel from tLOD register, baseDimBits=8 (256 base)
  val baseDimBits = U(8, 4 bits)
  val lodDimBits = (baseDimBits - lodLevel).resize(4 bits)
  val aspectRatio = tLOD.lodAspect
  val sIsWider = tLOD.lodSIsWider

  val texWidthBits = UInt(4 bits)
  val texHeightBits = UInt(4 bits)

  val narrowDimBits = UInt(4 bits)
  when(lodDimBits >= aspectRatio) {
    narrowDimBits := (lodDimBits - aspectRatio).resized
  }.otherwise {
    narrowDimBits := 0
  }

  texWidthBits := lodDimBits
  texHeightBits := lodDimBits
  when(sIsWider) {
    texHeightBits := narrowDimBits
  }.elsewhen(aspectRatio =/= 0) {
    texWidthBits := narrowDimBits
  }

  val stageLod = stageGrad
    .translateWith {
      val out = Tmu.FrontLod(c)
      out.texS := stageGrad.payload.texS
      out.texT := stageGrad.payload.texT
      out.clampToZero := stageGrad.payload.clampToZero
      out.lodLevel := lodLevel
      out.texWidthBits := texWidthBits
      out.texHeightBits := texHeightBits
      out
    }
    .stage()

  def wrapOrClamp(coord: SInt, sizeBits: UInt, clampEnable: Bool): UInt = {
    val size = (U(1, 10 bits) << sizeBits).resize(10 bits)
    val maxVal = size - 1
    val wrapped = (coord.resize(10 bits).asUInt & maxVal).resize(10 bits)
    val clamped = UInt(10 bits)
    when(coord < 0) {
      clamped := 0
    }.elsewhen(coord.asUInt >= size) {
      clamped := maxVal
    }.otherwise {
      clamped := coord.resize(10 bits).asUInt
    }
    clampEnable ? clamped | wrapped
  }

  // ========================================================================
  // Bilinear vs point coordinate computation
  // ========================================================================

  val stageLodTexMode = Tmu.TextureMode.decode(metaDelay.payload.config.textureMode)
  val bilinearEnable = stageLodTexMode.minFilter || stageLodTexMode.magFilter

  // Point path: integer texel coord at target LOD level
  // texS is X.4 format, shift right by (4 + lodLevel) to get integer texel at target LOD
  val sPoint = (stageLod.payload.texS >> (U(4) + stageLod.payload.lodLevel)).resize(14 bits)
  val tPoint = (stageLod.payload.texT >> (U(4) + stageLod.payload.lodLevel)).resize(14 bits)

  // Bilinear path: center-adjust by -0.5 texel, then LOD scale, extract fractions
  // adjS = texS - (1 << (3 + lodLevel))  — subtracts 0.5 texel in X.4 at target LOD
  val adjS =
    (stageLod.payload.texS - (S(1, 18 bits) |<< (U(3) + stageLod.payload.lodLevel))).resize(18 bits)
  val adjT =
    (stageLod.payload.texT - (S(1, 18 bits) |<< (U(3) + stageLod.payload.lodLevel))).resize(18 bits)
  val sScaled = (adjS >> stageLod.payload.lodLevel).resize(18 bits)
  val tScaled = (adjT >> stageLod.payload.lodLevel).resize(18 bits)
  val ds = sScaled(3 downto 0).asUInt // 4-bit fraction (0-15)
  val dt = tScaled(3 downto 0).asUInt
  val si = (sScaled >> 4).resize(14 bits)
  val ti = (tScaled >> 4).resize(14 bits)

  // Apply clampToZero
  val finalSPoint = stageLod.payload.clampToZero ? S(0, 14 bits) | sPoint
  val finalTPoint = stageLod.payload.clampToZero ? S(0, 14 bits) | tPoint
  val finalSi = stageLod.payload.clampToZero ? S(0, 14 bits) | si
  val finalTi = stageLod.payload.clampToZero ? S(0, 14 bits) | ti
  val finalDs = stageLod.payload.clampToZero ? U(0, 4 bits) | ds
  val finalDt = stageLod.payload.clampToZero ? U(0, 4 bits) | dt

  val stageB = stageLod
    .translateWith {
      val out = Tmu.FrontB(c)
      out.finalSPoint := finalSPoint
      out.finalTPoint := finalTPoint
      out.finalSi := finalSi
      out.finalTi := finalTi
      out.finalDs := finalDs
      out.finalDt := finalDt
      out.lodLevel := stageLod.payload.lodLevel
      out.texWidthBits := stageLod.payload.texWidthBits
      out.texHeightBits := stageLod.payload.texHeightBits
      out
    }
    .stage()

  val stageMeta = StreamJoin(stageB, metaDelay)
  val stageBTexMode = Tmu.TextureMode.decode(stageMeta.payload._2.config.textureMode)

  // ========================================================================
  // Address generation helper
  // ========================================================================

  val is16BitFormat = stageBTexMode.format >= Tmu.TextureFormat.ARGB8332
  val bytesPerTexel = Mux(is16BitFormat, U(2), U(1))
  val packedTables = if (c.packedTexLayout) stageMeta.payload._2.config.texTables else null
  val packedLodBase =
    if (c.packedTexLayout)
      packedTables.texBase(stageB.payload.lodLevel).resize(c.addressWidth.value bits)
    else U(0, c.addressWidth.value bits)
  val packedLodShift =
    if (c.packedTexLayout) packedTables.texShift(stageB.payload.lodLevel) else U(0, 4 bits)
  val lodWidthBits = stageB.payload.texWidthBits.resize(5 bits)
  val lodHeightBits = stageB.payload.texHeightBits.resize(5 bits)
  val lodSizeShift =
    (lodWidthBits +^ lodHeightBits +^ is16BitFormat.asUInt.resize(5 bits)).resize(6 bits)
  val packedLodEnd = if (c.packedTexLayout) {
    val lodSizeBytes =
      (U(1, c.addressWidth.value bits) |<< lodSizeShift).resize(c.addressWidth.value bits)
    (packedLodBase + lodSizeBytes).resized
  } else {
    U(0, c.addressWidth.value bits)
  }

  // Texture memory addressing — packed or PCI-encoded depending on config
  def texelAddr(x: UInt, y: UInt): UInt = {
    if (c.packedTexLayout) {
      // Packed layout: use per-triangle computed tables
      val tables = stageMeta.payload._2.config.texTables
      val lodBase = tables.texBase(stageB.payload.lodLevel)
      val lodShift = tables.texShift(stageB.payload.lodLevel)
      Mux(
        is16BitFormat,
        lodBase + (x << 1).resize(22 bits) + (y.resize(22 bits) << (lodShift +^ U(1)).resize(
          5 bits
        )).resize(22 bits),
        lodBase + x.resize(22 bits) + (y.resize(22 bits) << lodShift).resize(22 bits)
      ).resize(c.addressWidth.value bits)
    } else {
      // PCI-encoded texture memory addressing matching real SST-1 hardware.
      // Bits [20:17] = LOD, [16:9] = T (row), [8:0] = column byte offset.
      val texBaseByteAddr =
        (stageMeta.payload._2.config.texBaseAddr << 3).resize(c.addressWidth.value bits)
      val lodField = stageB.payload.lodLevel.resize(4 bits)
      val tField = y.resize(8 bits)
      val x8 = x.resize(8 bits)
      val colByteOffset = Mux(
        is16BitFormat,
        (x << 1).resize(9 bits),
        (x8(7 downto 2) ## B"0" ## x8(1 downto 0)).asUInt // two-bank interleaved
      )
      val pciOffset = (lodField ## tField ## colByteOffset).asUInt
      texBaseByteAddr + pciOffset.resize(c.addressWidth.value bits)
    }
  }

  // Point: 1 address
  val pointTexelX =
    wrapOrClamp(stageB.payload.finalSPoint, stageB.payload.texWidthBits, stageBTexMode.clampS)
  val pointTexelY =
    wrapOrClamp(stageB.payload.finalTPoint, stageB.payload.texHeightBits, stageBTexMode.clampT)
  val pointAddr = texelAddr(pointTexelX, pointTexelY)
  val pointBankSel = (pointTexelY(0) ## pointTexelX(0)).asUInt

  // Bilinear: 4 addresses from 2x2 neighborhood
  val biX0 = wrapOrClamp(stageB.payload.finalSi, stageB.payload.texWidthBits, stageBTexMode.clampS)
  val biX1 = wrapOrClamp(
    (stageB.payload.finalSi + 1).resize(14 bits),
    stageB.payload.texWidthBits,
    stageBTexMode.clampS
  )
  val biY0 = wrapOrClamp(stageB.payload.finalTi, stageB.payload.texHeightBits, stageBTexMode.clampT)
  val biY1 = wrapOrClamp(
    (stageB.payload.finalTi + 1).resize(14 bits),
    stageB.payload.texHeightBits,
    stageBTexMode.clampT
  )
  val biAddr0 = texelAddr(biX0, biY0) // top-left
  val biAddr1 = texelAddr(biX1, biY0) // top-right
  val biAddr2 = texelAddr(biX0, biY1) // bottom-left
  val biAddr3 = texelAddr(biX1, biY1) // bottom-right
  val biBankSel0 = (biY0(0) ## biX0(0)).asUInt
  val biBankSel1 = (biY0(0) ## biX1(0)).asUInt
  val biBankSel2 = (biY1(0) ## biX0(0)).asUInt
  val biBankSel3 = (biY1(0) ## biX1(0)).asUInt

  val inputPassthrough = Tmu.TmuPassthrough(c)
  inputPassthrough.format := stageBTexMode.format
  inputPassthrough.bilinear := bilinearEnable
  inputPassthrough.sendConfig := stageMeta.payload._2.config.sendConfig
  inputPassthrough.ds := stageB.payload.finalDs
  inputPassthrough.dt := stageB.payload.finalDt
  inputPassthrough.readIdx := 0
  inputPassthrough.requestId := 0
  inputPassthrough.ncc := stageMeta.payload._2.config.ncc
  if (c.trace.enabled) {
    inputPassthrough.trace := stageMeta.payload._2.trace
  }

  val sampleRequestBase = stageMeta
    .translateWith {
      val req = Tmu.SampleRequest(c)
      req.pointAddr := pointAddr
      req.biAddr0 := biAddr0
      req.biAddr1 := biAddr1
      req.biAddr2 := biAddr2
      req.biAddr3 := biAddr3
      req.pointBankSel := pointBankSel
      req.biBankSel0 := biBankSel0
      req.biBankSel1 := biBankSel1
      req.biBankSel2 := biBankSel2
      req.biBankSel3 := biBankSel3
      req.lodBase := packedLodBase
      req.lodEnd := packedLodEnd
      req.lodShift := packedLodShift
      req.is16Bit := is16BitFormat
      if (c.packedTexLayout) {
        req.texTables := stageMeta.payload._2.config.texTables
      }
      req.bilinear := bilinearEnable
      req.passthrough := inputPassthrough
      req
    }
    .m2sPipe()
    .queue(16)

  val requestIdAlloc = Reg(UInt(requestIdWidth bits)) init 0
  val canAllocate = inFlightCount =/= maxOutstanding
  val sampleRequest = Stream(Tmu.SampleRequest(c))
  sampleRequest.valid := sampleRequestBase.valid && canAllocate
  sampleRequest.payload.pointAddr := sampleRequestBase.payload.pointAddr
  sampleRequest.payload.biAddr0 := sampleRequestBase.payload.biAddr0
  sampleRequest.payload.biAddr1 := sampleRequestBase.payload.biAddr1
  sampleRequest.payload.biAddr2 := sampleRequestBase.payload.biAddr2
  sampleRequest.payload.biAddr3 := sampleRequestBase.payload.biAddr3
  sampleRequest.payload.pointBankSel := sampleRequestBase.payload.pointBankSel
  sampleRequest.payload.biBankSel0 := sampleRequestBase.payload.biBankSel0
  sampleRequest.payload.biBankSel1 := sampleRequestBase.payload.biBankSel1
  sampleRequest.payload.biBankSel2 := sampleRequestBase.payload.biBankSel2
  sampleRequest.payload.biBankSel3 := sampleRequestBase.payload.biBankSel3
  sampleRequest.payload.lodBase := sampleRequestBase.payload.lodBase
  sampleRequest.payload.lodEnd := sampleRequestBase.payload.lodEnd
  sampleRequest.payload.lodShift := sampleRequestBase.payload.lodShift
  sampleRequest.payload.is16Bit := sampleRequestBase.payload.is16Bit
  if (c.packedTexLayout) {
    sampleRequest.payload.texTables := sampleRequestBase.payload.texTables
  }
  sampleRequest.payload.bilinear := sampleRequestBase.payload.bilinear
  sampleRequest.payload.passthrough.format := sampleRequestBase.payload.passthrough.format
  sampleRequest.payload.passthrough.bilinear := sampleRequestBase.payload.passthrough.bilinear
  sampleRequest.payload.passthrough.sendConfig := sampleRequestBase.payload.passthrough.sendConfig
  sampleRequest.payload.passthrough.ds := sampleRequestBase.payload.passthrough.ds
  sampleRequest.payload.passthrough.dt := sampleRequestBase.payload.passthrough.dt
  sampleRequest.payload.passthrough.readIdx := sampleRequestBase.payload.passthrough.readIdx
  sampleRequest.payload.passthrough.requestId := requestIdAlloc
  sampleRequest.payload.passthrough.ncc := sampleRequestBase.payload.passthrough.ncc
  if (c.trace.enabled) {
    sampleRequest.payload.passthrough.trace := sampleRequestBase.payload.passthrough.trace
  }
  sampleRequestBase.ready := sampleRequest.ready && canAllocate

  val textureCache = TmuTextureCache(c)
  sampleRequest >/-> textureCache.io.sampleRequest
  io.texRead <> textureCache.io.texRead
  textureCache.io.invalidate := io.invalidate

  val texelDecoder = TmuTexelDecoder(c)
  textureCache.io.fetched >/-> texelDecoder.io.fetched
  textureCache.io.fastFetch >/-> texelDecoder.io.fastFetch
  texelDecoder.io.paletteWrite <> io.paletteWrite

  val collector = TmuCollector(c)
  texelDecoder.io.decoded >/-> collector.io.decoded

  val fastOutput = texelDecoder.io.fastOutput
  val normalOutput = collector.io.output
  textureCache.io.outputRoute.ready := True

  val retireValid = Vec(Reg(Bool()) init False, 1 << requestIdWidth)
  val retireData = Vec.fill(1 << requestIdWidth)(Reg(Tmu.Output(c)))
  val retireHead = Reg(UInt(requestIdWidth bits)) init 0

  fastOutput.ready := !retireValid(fastOutput.payload.requestId)
  normalOutput.ready := !retireValid(normalOutput.payload.requestId)

  when(fastOutput.fire) {
    retireValid(fastOutput.payload.requestId) := True
    retireData(fastOutput.payload.requestId) := fastOutput.payload
  }
  when(normalOutput.fire) {
    retireValid(normalOutput.payload.requestId) := True
    retireData(normalOutput.payload.requestId) := normalOutput.payload
  }

  io.output.valid := retireValid(retireHead)
  io.output.payload := retireData(retireHead)
  when(io.output.fire) {
    retireValid(retireHead) := False
    retireHead := retireHead + 1
  }

  when(sampleRequest.fire) {
    requestIdAlloc := requestIdAlloc + 1
  }
  when(sampleRequest.fire && !io.output.fire) {
    inFlightCount := inFlightCount + 1
  }.elsewhen(!sampleRequest.fire && io.output.fire) {
    inFlightCount := inFlightCount - 1
  }
  io.busy := inFlightCount =/= 0 || stageRecip.valid || stageA.valid || stageGrad.valid || stageLod.valid || stageB.valid || metaDelay.valid || stageMeta.valid || sampleRequestBase.valid
}

object Tmu {

  /** Palette write command */
  case class PaletteWrite() extends Bundle {
    val address = UInt(8 bits)
    val data = Bits(24 bits)
  }

  /** Per-request metadata carried from address generation through fetch/decode */
  case class TmuPassthrough(c: voodoo.Config) extends Bundle {
    val format = UInt(4 bits)
    val bilinear = Bool()
    val sendConfig = Bool()
    val ds = UInt(4 bits)
    val dt = UInt(4 bits)
    val readIdx = UInt(2 bits)
    val requestId = UInt(5 bits)
    val ncc = Tmu.NccTableData()
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  case class FrontA(c: voodoo.Config) extends Bundle {
    val texS = SInt(18 bits)
    val texT = SInt(18 bits)
    val tLOD = Bits(27 bits)
    val perspLodAdjust = SInt(16 bits)
    val clampToZero = Bool()
    val dSdX = AFix(c.texCoordsFormat)
    val dTdX = AFix(c.texCoordsFormat)
    val dSdY = AFix(c.texCoordsFormat)
    val dTdY = AFix(c.texCoordsFormat)
  }

  case class FrontRecip(c: voodoo.Config) extends Bundle {
    val s = AFix(c.texCoordsAccumFormat)
    val t = AFix(c.texCoordsAccumFormat)
    val tLOD = Bits(27 bits)
    val interp = UInt(17 bits)
    val msbPos = UInt(7 bits)
    val perspectiveEnable = Bool()
    val clampToZero = Bool()
    val perspLodAdjust = SInt(16 bits)
    val dSdX = AFix(c.texCoordsFormat)
    val dTdX = AFix(c.texCoordsFormat)
    val dSdY = AFix(c.texCoordsFormat)
    val dTdY = AFix(c.texCoordsFormat)
  }

  case class FrontB(c: voodoo.Config) extends Bundle {
    val finalSPoint = SInt(14 bits)
    val finalTPoint = SInt(14 bits)
    val finalSi = SInt(14 bits)
    val finalTi = SInt(14 bits)
    val finalDs = UInt(4 bits)
    val finalDt = UInt(4 bits)
    val lodLevel = UInt(4 bits)
    val texWidthBits = UInt(4 bits)
    val texHeightBits = UInt(4 bits)
  }

  case class FrontLod(c: voodoo.Config) extends Bundle {
    val texS = SInt(18 bits)
    val texT = SInt(18 bits)
    val clampToZero = Bool()
    val lodLevel = UInt(4 bits)
    val texWidthBits = UInt(4 bits)
    val texHeightBits = UInt(4 bits)
  }

  case class FrontGrad(c: voodoo.Config) extends Bundle {
    val texS = SInt(18 bits)
    val texT = SInt(18 bits)
    val tLOD = Bits(27 bits)
    val clampToZero = Bool()
    val tempLOD = UInt(64 bits)
    val perspLodAdjust = SInt(16 bits)
  }

  case class FrontMeta(c: voodoo.Config) extends Bundle {
    val config = TmuConfig(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  case class TmuExpanded(c: voodoo.Config) extends Bundle {
    val address = UInt(c.addressWidth.value bits)
    val bankSel = UInt(2 bits)
    val lodBase = UInt(c.addressWidth.value bits)
    val lodEnd = UInt(c.addressWidth.value bits)
    val lodShift = UInt(4 bits)
    val is16Bit = Bool()
    val texTables = if (c.packedTexLayout) TexLayoutTables.Tables() else null
    val passthrough = TmuPassthrough(c)
  }

  case class SampleRequest(c: voodoo.Config) extends Bundle {
    val pointAddr = UInt(c.addressWidth.value bits)
    val biAddr0 = UInt(c.addressWidth.value bits)
    val biAddr1 = UInt(c.addressWidth.value bits)
    val biAddr2 = UInt(c.addressWidth.value bits)
    val biAddr3 = UInt(c.addressWidth.value bits)
    val pointBankSel = UInt(2 bits)
    val biBankSel0 = UInt(2 bits)
    val biBankSel1 = UInt(2 bits)
    val biBankSel2 = UInt(2 bits)
    val biBankSel3 = UInt(2 bits)
    val lodBase = UInt(c.addressWidth.value bits)
    val lodEnd = UInt(c.addressWidth.value bits)
    val lodShift = UInt(4 bits)
    val is16Bit = Bool()
    val texTables = if (c.packedTexLayout) TexLayoutTables.Tables() else null
    val bilinear = Bool()
    val passthrough = TmuPassthrough(c)
  }

  case class QueuedData(c: voodoo.Config) extends Bundle {
    val passthrough = TmuPassthrough(c)
    val fullAddress = UInt(c.addressWidth.value bits)
    val addrHalf = Bool()
    val addrByte = Bool()
  }

  case class CachedReq(c: voodoo.Config, bankEntryWidth: Int) extends Bundle {
    val lineBase = UInt(c.addressWidth.value bits)
    val lodBase = UInt(c.addressWidth.value bits)
    val lodEnd = UInt(c.addressWidth.value bits)
    val lodShift = UInt(4 bits)
    val is16Bit = Bool()
    val texTables = if (c.packedTexLayout) TexLayoutTables.Tables() else null
    val bankSel = UInt(2 bits)
    val bankEntry = UInt(bankEntryWidth bits)
    val queued = QueuedData(c)
  }

  case class FetchResult(c: voodoo.Config) extends Bundle {
    val rspData32 = Bits(32 bits)
    val queued = QueuedData(c)
  }

  case class TexelWordRef() extends Bundle {
    val rspData32 = Bits(32 bits)
    val addrHalf = Bool()
    val addrByte = Bool()
  }

  case class FastFetch(c: voodoo.Config) extends Bundle {
    val texels = Vec.fill(4)(TexelWordRef())
    val passthrough = TmuPassthrough(c)
  }

  case class DecodedTexel(c: voodoo.Config) extends Bundle {
    val r = UInt(8 bits)
    val g = UInt(8 bits)
    val b = UInt(8 bits)
    val a = UInt(8 bits)
    val passthrough = TmuPassthrough(c)
  }

  /** Decoded textureMode register fields */
  case class TextureMode() extends Bundle {
    val perspectiveEnable = Bool() // Bit 0: tpersp_st - perspective correction
    val minFilter = Bool() // Bit 1: tminfilter - 0=point, 1=bilinear
    val magFilter = Bool() // Bit 2: tmagfilter - 0=point, 1=bilinear
    val clampW = Bool() // Bit 3: tclampw - clamp when W negative
    val lodDither = Bool() // Bit 4: tloddither (TODO)
    val nccSelect = Bool() // Bit 5: tnccselect - NCC table 0 or 1 (TODO)
    val clampS = Bool() // Bit 6: tclamps - 0=wrap, 1=clamp
    val clampT = Bool() // Bit 7: tclampt - 0=wrap, 1=clamp
    val format = UInt(4 bits) // Bits 11:8: tformat - texture format
  }

  object TextureMode {
    def decode(bits: Bits): TextureMode = {
      val mode = TextureMode()
      mode.perspectiveEnable := bits(0)
      mode.minFilter := bits(1)
      mode.magFilter := bits(2)
      mode.clampW := bits(3)
      mode.lodDither := bits(4)
      mode.nccSelect := bits(5)
      mode.clampS := bits(6)
      mode.clampT := bits(7)
      mode.format := bits(11 downto 8).asUInt
      mode
    }
  }

  /** Texture format enumeration (tformat field, bits 11:8) */
  object TextureFormat {
    val RGB332 = 0
    val YIQ422 = 1
    val A8 = 2
    val I8 = 3
    val AI44 = 4
    val P8 = 5
    val ARGB8332 = 8
    val AYIQ8422 = 9
    val RGB565 = 10
    val ARGB1555 = 11
    val ARGB4444 = 12
    val AI88 = 13
    val AP88 = 14
  }

  /** Decoded tLOD register fields */
  case class TLOD() extends Bundle {
    val lodmin = UInt(6 bits)
    val lodmax = UInt(6 bits)
    val lodbias = Bits(6 bits)
    val lodOdd = Bool()
    val lodSplit = Bool()
    val lodSIsWider = Bool()
    val lodAspect = UInt(2 bits)
    val lodZeroFrac = Bool()
    val tmultibaseaddr = Bool()
    val tdataSwizzle = Bool()
    val tdataSwap = Bool()
  }

  object TLOD {
    def decode(bits: Bits): TLOD = {
      val lod = TLOD()
      lod.lodmin := bits(5 downto 0).asUInt
      lod.lodmax := bits(11 downto 6).asUInt
      lod.lodbias := bits(17 downto 12)
      lod.lodOdd := bits(18)
      lod.lodSplit := bits(19)
      lod.lodSIsWider := bits(20)
      lod.lodAspect := bits(22 downto 21).asUInt
      lod.lodZeroFrac := bits(23)
      lod.tmultibaseaddr := bits(24)
      lod.tdataSwizzle := bits(25)
      lod.tdataSwap := bits(26)
      lod
    }
  }

  /** NCC table data, pre-extracted at triangle capture time. Y: 16 luminance values (8-bit
    * unsigned), pre-extracted from 4 packed 32-bit registers. I/Q: 4 chrominance entries each,
    * packed as [26:18]=R, [17:9]=G, [8:0]=B (9-bit signed).
    */
  case class NccTableData() extends Bundle {
    val y = Vec(UInt(8 bits), 16)
    val i = Vec(Bits(27 bits), 4)
    val q = Vec(Bits(27 bits), 4)
  }

  /** Per-TMU configuration (captured per-triangle) */
  case class TmuConfig(c: voodoo.Config = null) extends Bundle {
    val textureMode = Bits(32 bits)
    val texBaseAddr = UInt(24 bits)
    val tLOD = Bits(27 bits)
    val sendConfig = Bool()
    val ncc = NccTableData()
    val texTables = if (c != null && c.packedTexLayout) TexLayoutTables.Tables() else null
  }

  /** TMU input bundle */
  case class Input(c: voodoo.Config) extends Bundle {
    val s = AFix(c.texCoordsAccumFormat)
    val t = AFix(c.texCoordsAccumFormat)
    val w = AFix(c.wAccumFormat)
    val cOther = Color.u8()
    val aOther = UInt(8 bits)
    val config = TmuConfig(c)
    val dSdX = AFix(c.texCoordsFormat)
    val dTdX = AFix(c.texCoordsFormat)
    val dSdY = AFix(c.texCoordsFormat)
    val dTdY = AFix(c.texCoordsFormat)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  /** TMU output bundle */
  case class Output(c: voodoo.Config) extends Bundle {
    val texture = Color.u8()
    val textureAlpha = UInt(8 bits)
    val requestId = UInt(5 bits)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  /** BMB parameters for texture memory access */
  def bmbParams(c: voodoo.Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = false,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}
