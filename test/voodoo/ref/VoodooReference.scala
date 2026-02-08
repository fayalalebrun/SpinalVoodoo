package voodoo.ref

/** Faithful Scala port of 86Box vid_voodoo_render.c
  *
  * This is a reference model for integration testing against the SpinalVoodoo HDL. All arithmetic
  * mirrors the C implementation exactly, including fixed-point formats, sign extension, and
  * overflow behavior.
  *
  * Reference: emu/86Box/src/video/vid_voodoo_render.c
  */
object VoodooReference {

  // ========================================================================
  // Data structures (mirror 86Box's voodoo_params_t and voodoo_state_t)
  // ========================================================================

  /** Mirror of voodoo_params_t — all values as stored after register writes.
    *
    * Vertices are 12.4 fixed-point (raw 16-bit signed, stored in Int). Colors (R/G/B/A) are 12.12
    * fixed-point stored in Int (via uint32 in C). Z is 20.12 fixed-point stored in Int (via uint32
    * in C). S/T are 14.18 fixed-point stored in Long (via int64 in C). W is 2.30 fixed-point stored
    * in Long (via int64 in C).
    *
    * 86Box stores S/T with <<14 shift and W with <<2 shift from register writes. SpinalVoodoo
    * stores raw register values. The test harness applies the shift when constructing VoodooParams
    * from register values.
    */
  case class VoodooParams(
      // Vertices: 12.4 fixed (raw 16-bit signed, stored in lower 16 bits of Int)
      vertexAx: Int,
      vertexAy: Int,
      vertexBx: Int,
      vertexBy: Int,
      vertexCx: Int,
      vertexCy: Int,
      // Start values: stored as uint32 in C (use Long to avoid sign issues)
      startR: Long,
      startG: Long,
      startB: Long,
      startA: Long,
      startZ: Long,
      // TMU0 texture coords: int64 in C (already shifted: S/T <<14, W <<2)
      startS: Long,
      startT: Long,
      startW: Long,
      // FBI W: int64 in C (already shifted: W <<2)
      startWFBI: Long,
      // Gradients per X (int32 for colors/Z, int64 for S/T/W)
      dRdX: Int,
      dGdX: Int,
      dBdX: Int,
      dAdX: Int,
      dZdX: Int,
      dSdX: Long,
      dTdX: Long,
      dWdX: Long,
      dWdXFBI: Long,
      // Gradients per Y
      dRdY: Int,
      dGdY: Int,
      dBdY: Int,
      dAdY: Int,
      dZdY: Int,
      dSdY: Long,
      dTdY: Long,
      dWdY: Long,
      dWdYFBI: Long,
      // Config registers
      fbzColorPath: Int,
      fbzMode: Int,
      fogMode: Int = 0,
      alphaMode: Int = 0,
      textureMode: Int = 0,
      tLOD: Int = 0,
      color0: Int = 0,
      color1: Int = 0,
      zaColor: Int = 0,
      sign: Boolean = false,
      // Clip
      clipLeft: Int = 0,
      clipRight: Int = 0x3ff,
      clipLowY: Int = 0,
      clipHighY: Int = 0x3ff,
      // Texture data: pre-decoded ARGB32 per LOD level
      // texData(lod)(texelIndex) = 0xAARRGGBB
      texData: Array[Array[Int]] = Array.fill(9)(Array.empty[Int]),
      // Per-LOD texture dimension masks and shift
      texWMask: Array[Int] = Array.fill(9)(0),
      texHMask: Array[Int] = Array.fill(9)(0),
      texShift: Array[Int] = Array.fill(9)(0),
      texLod: Array[Int] = Array.fill(9)(0)
  )

  /** Per-pixel output */
  case class RefPixel(x: Int, y: Int, rgb565: Int, depth16: Int)

  // ========================================================================
  // Constants (from vid_voodoo_regs.h)
  // ========================================================================
  val FBZ_CHROMAKEY = 1 << 1
  val FBZ_W_BUFFER = 1 << 3
  val FBZ_DEPTH_ENABLE = 1 << 4
  val FBZ_DITHER = 1 << 8
  val FBZ_RGB_WMASK = 1 << 9
  val FBZ_DEPTH_WMASK = 1 << 10
  val FBZ_DEPTH_BIAS = 1 << 16
  val FBZ_DEPTH_SOURCE = 1 << 20
  val FBZ_PARAM_ADJUST = 1 << 26
  val FBZCP_TEXTURE_ENABLED = 1 << 27

  val LOD_MAX = 8

  val TEXTUREMODE_TCLAMPS = 1 << 6
  val TEXTUREMODE_TCLAMPT = 1 << 7

  // ========================================================================
  // Helper functions
  // ========================================================================

  def CLAMP(x: Int): Int = if (x < 0) 0 else if (x > 0xff) 0xff else x
  def CLAMP16(x: Int): Int = if (x < 0) 0 else if (x > 0xffff) 0xffff else x

  /** Sign-extend a 16-bit value to 32-bit */
  def signExtend16(v: Int): Int = {
    val masked = v & 0xffff
    if ((masked & 0x8000) != 0) masked | 0xffff0000
    else masked
  }

  /** Truncate to 32-bit int (simulate C uint32 overflow) */
  def trunc32(v: Long): Long = v & 0xffffffffL

  /** Convert Long to signed 32-bit Int (C-style cast) */
  def toInt32(v: Long): Int = v.toInt

  // ========================================================================
  // Log table and fastlog (lines 113-173 of vid_voodoo_render.c)
  // ========================================================================
  val logtable: Array[Int] = Array(
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

  /** Mirrors fastlog() from vid_voodoo_render.c lines 132-173. Computes a fixed-point log2
    * approximation: result = (exponent << 8) | logtable[frac]
    */
  def fastlog(inVal: Long): Int = {
    if (inVal == 0 || (inVal & (1L << 63)) != 0) return 0x80000000

    val oldval = inVal
    var v = inVal
    var exp = 63

    if ((v & 0xffffffff00000000L) == 0) { exp -= 32; v <<= 32 }
    if ((v & 0xffff000000000000L) == 0) { exp -= 16; v <<= 16 }
    if ((v & 0xff00000000000000L) == 0) { exp -= 8; v <<= 8 }
    if ((v & 0xf000000000000000L) == 0) { exp -= 4; v <<= 4 }
    if ((v & 0xc000000000000000L) == 0) { exp -= 2; v <<= 2 }
    if ((v & 0x8000000000000000L) == 0) { exp -= 1; v <<= 1 }

    val frac =
      if (exp >= 8) ((oldval >>> (exp - 8)) & 0xff).toInt
      else ((oldval << (8 - exp)) & 0xff).toInt

    (exp << 8) | logtable(frac)
  }

  /** Mirrors voodoo_fls() from vid_voodoo_render.c lines 175-203. Finds leading set bit position
    * (number of leading zeros from bit 15).
    */
  def voodooFls(inVal: Int): Int = {
    var v = inVal & 0xffff
    var num = 0
    if ((v & 0xff00) == 0) { num += 8; v <<= 8 }
    if ((v & 0xf000) == 0) { num += 4; v <<= 4 }
    if ((v & 0xc000) == 0) { num += 2; v <<= 2 }
    if ((v & 0x8000) == 0) { num += 1; v <<= 1 }
    num
  }

  // ========================================================================
  // Texture decode (pre-decode texture data to ARGB32)
  // ========================================================================

  def expand3to8(v: Int): Int = (v << 5) | (v << 2) | (v >> 1)
  def expand2to8(v: Int): Int = (v << 6) | (v << 4) | (v << 2) | v
  def expand4to8(v: Int): Int = (v << 4) | v
  def expand5to8(v: Int): Int = (v << 3) | (v >> 2)
  def expand6to8(v: Int): Int = (v << 2) | (v >> 4)
  def expand1to8(v: Int): Int = if (v != 0) 0xff else 0x00

  /** Decode a raw texel to ARGB32 (0xAARRGGBB) based on format. Format codes match 86Box TEX_* enum
    * from vid_voodoo_regs.h.
    */
  def decodeTexel(raw: Int, format: Int): Int = {
    format match {
      case 0x0 => // RGB332
        val r = expand3to8((raw >> 5) & 7)
        val g = expand3to8((raw >> 2) & 7)
        val b = expand2to8(raw & 3)
        0xff000000 | (r << 16) | (g << 8) | b
      case 0x2 => // A8
        val a = raw & 0xff
        (a << 24) | 0x00ffffff
      case 0x3 => // I8
        val i = raw & 0xff
        0xff000000 | (i << 16) | (i << 8) | i
      case 0x4 => // AI44
        val a = expand4to8((raw >> 4) & 0xf)
        val i = expand4to8(raw & 0xf)
        (a << 24) | (i << 16) | (i << 8) | i
      case 0x8 => // ARGB8332
        val a = (raw >> 8) & 0xff
        val r = expand3to8((raw >> 5) & 7)
        val g = expand3to8((raw >> 2) & 7)
        val b = expand2to8(raw & 3)
        (a << 24) | (r << 16) | (g << 8) | b
      case 0xa => // R5G6B5 (RGB565)
        val r = expand5to8((raw >> 11) & 0x1f)
        val g = expand6to8((raw >> 5) & 0x3f)
        val b = expand5to8(raw & 0x1f)
        0xff000000 | (r << 16) | (g << 8) | b
      case 0xb => // ARGB1555
        val a = expand1to8((raw >> 15) & 1)
        val r = expand5to8((raw >> 10) & 0x1f)
        val g = expand5to8((raw >> 5) & 0x1f)
        val b = expand5to8(raw & 0x1f)
        (a << 24) | (r << 16) | (g << 8) | b
      case 0xc => // ARGB4444
        val a = expand4to8((raw >> 12) & 0xf)
        val r = expand4to8((raw >> 8) & 0xf)
        val g = expand4to8((raw >> 4) & 0xf)
        val b = expand4to8(raw & 0xf)
        (a << 24) | (r << 16) | (g << 8) | b
      case 0xd => // A8I8 (AI88)
        val a = (raw >> 8) & 0xff
        val i = raw & 0xff
        (a << 24) | (i << 16) | (i << 8) | i
      case _ => 0xff000000 // Unknown format: opaque black
    }
  }

  // ========================================================================
  // TMU fetch (lines 380-418)
  // ========================================================================

  /** Texture read with clamp/wrap. Returns (r, g, b, a) each 0-255. Mirrors tex_read() from
    * vid_voodoo_render.c lines 211-241.
    */
  def texRead(
      texData: Array[Int],
      s: Int,
      t: Int,
      wMask: Int,
      hMask: Int,
      texShift: Int,
      clampS: Boolean,
      clampT: Boolean
  ): (Int, Int, Int, Int) = {
    var cs = s
    var ct = t

    if ((cs & ~wMask) != 0) {
      if (clampS) {
        if (cs < 0) cs = 0
        if (cs > wMask) cs = wMask
      } else cs &= wMask
    }
    if ((ct & ~hMask) != 0) {
      if (clampT) {
        if (ct < 0) ct = 0
        if (ct > hMask) ct = hMask
      } else ct &= hMask
    }

    val idx = cs + (ct << texShift)
    val dat = if (idx >= 0 && idx < texData.length) texData(idx) else 0
    val b = dat & 0xff
    val g = (dat >> 8) & 0xff
    val r = (dat >> 16) & 0xff
    val a = (dat >>> 24) & 0xff
    (r, g, b, a)
  }

  /** TMU fetch with perspective correction. Mirrors voodoo_tmu_fetch() from vid_voodoo_render.c
    * lines 380-418. Returns (texR, texG, texB, texA, lod).
    */
  def tmuFetch(
      params: VoodooParams,
      tmu0_s: Long,
      tmu0_t: Long,
      tmu0_w: Long,
      baseLod: Int,
      lodMin: Int,
      lodMax: Int
  ): (Int, Int, Int, Int) = {
    val perspective = (params.textureMode & 1) != 0
    var texS: Int = 0
    var texT: Int = 0
    var lod: Int = 0

    if (perspective) {
      var _w: Long = 0
      if (tmu0_w != 0)
        _w = java.lang.Long.divideUnsigned(1L << 48, tmu0_w)

      texS = (((((tmu0_s + (1L << 13)) >> 14) * _w) + (1L << 29)) >> 30).toInt
      texT = (((((tmu0_t + (1L << 13)) >> 14) * _w) + (1L << 29)) >> 30).toInt
      lod = baseLod + (fastlog(_w) - (19 << 8))
    } else {
      texS = (tmu0_s >> (14 + 14)).toInt
      texT = (tmu0_t >> (14 + 14)).toInt
      lod = baseLod
    }

    if (lod < lodMin) lod = lodMin
    else if (lod > lodMax) lod = lodMax
    val lodInt = lod >> 8

    val clampS = (params.textureMode & TEXTUREMODE_TCLAMPS) != 0
    val clampT = (params.textureMode & TEXTUREMODE_TCLAMPT) != 0

    // Get tex_lod for this LOD level (shift amount for point sampling)
    val texLodVal = if (lodInt >= 0 && lodInt < params.texLod.length) params.texLod(lodInt) else 0
    val texShift = 8 - texLodVal

    // Point sampling: s >> (4 + tex_lod)
    val s = texS >> (4 + texLodVal)
    val t = texT >> (4 + texLodVal)

    val wMask = if (lodInt >= 0 && lodInt < params.texWMask.length) params.texWMask(lodInt) else 0
    val hMask = if (lodInt >= 0 && lodInt < params.texHMask.length) params.texHMask(lodInt) else 0
    val data =
      if (lodInt >= 0 && lodInt < params.texData.length) params.texData(lodInt)
      else Array.empty[Int]

    texRead(data, s, t, wMask, hMask, texShift, clampS, clampT)
  }

  // ========================================================================
  // Color combine (lines 1051-1266)
  // ========================================================================

  /** Color combine pipeline. Mirrors the color path from vid_voodoo_render.c lines 1051-1266.
    * Returns (r, g, b, a) each 0-255.
    */
  def colorCombine(
      params: VoodooParams,
      ir: Int,
      ig: Int,
      ib: Int,
      ia: Int,
      z: Int,
      texR: Int,
      texG: Int,
      texB: Int,
      texA: Int
  ): (Int, Int, Int, Int) = {
    val fcp = params.fbzColorPath

    // Decode fbzColorPath fields
    val rgbSel = fcp & 3
    val aSel = (fcp >> 2) & 3
    val ccLocalselect = (fcp & (1 << 4)) != 0
    val ccaLocalselect = (fcp >> 5) & 3
    val ccLocalselectOverride = (fcp & (1 << 7)) != 0
    val ccZeroOther = (fcp & (1 << 8)) != 0
    val ccSubClocal = (fcp & (1 << 9)) != 0
    val ccMselect = (fcp >> 10) & 7
    val ccReverseBlend = (fcp & (1 << 13)) != 0
    val ccAdd = (fcp >> 14) & 3
    val ccInvertOutput = (fcp & (1 << 16)) != 0
    val ccaZeroOther = (fcp & (1 << 17)) != 0
    val ccaSubClocal = (fcp & (1 << 18)) != 0
    val ccaMselect = (fcp >> 19) & 7
    val ccaReverseBlend = (fcp & (1 << 22)) != 0
    val ccaAdd = (fcp >> 23) & 3
    val ccaInvertOutput = (fcp & (1 << 25)) != 0

    // clocal selection
    val sel =
      if (ccLocalselectOverride) { if ((texA & 0x80) != 0) true else false }
      else ccLocalselect

    val (clocalR, clocalG, clocalB) =
      if (sel) {
        (
          (params.color0 >> 16) & 0xff,
          (params.color0 >> 8) & 0xff,
          params.color0 & 0xff
        )
      } else {
        (CLAMP(ir >> 12), CLAMP(ig >> 12), CLAMP(ib >> 12))
      }

    // alocal selection
    val alocal: Int = ccaLocalselect match {
      case 0 => CLAMP(ia >> 12) // CCA_LOCALSELECT_ITER_A
      case 1 => (params.color0 >> 24) & 0xff // CCA_LOCALSELECT_COLOR0
      case 2 => CLAMP(z >> 20) // CCA_LOCALSELECT_ITER_Z
      case _ => 0xff
    }

    // cother selection (rgb_sel)
    val (cotherR, cotherG, cotherB): (Int, Int, Int) = rgbSel match {
      case 0 => (CLAMP(ir >> 12), CLAMP(ig >> 12), CLAMP(ib >> 12)) // Iterated RGB
      case 1 => (texR, texG, texB) // TREX Color Output
      case 2 =>
        (
          (params.color1 >> 16) & 0xff,
          (params.color1 >> 8) & 0xff,
          params.color1 & 0xff
        ) // Color1
      case _ => (0, 0, 0) // LFB (unused in our tests)
    }

    // aother selection (a_sel)
    val aother: Int = aSel match {
      case 0 => CLAMP(ia >> 12) // Iterated A
      case 1 => texA // Texture A
      case 2 => (params.color1 >> 24) & 0xff // Color1 A
      case _ => 0
    }

    // zero_other
    var srcR = if (ccZeroOther) 0 else cotherR
    var srcG = if (ccZeroOther) 0 else cotherG
    var srcB = if (ccZeroOther) 0 else cotherB
    var srcA = if (ccaZeroOther) 0 else aother

    // sub_clocal
    if (ccSubClocal) {
      srcR -= clocalR
      srcG -= clocalG
      srcB -= clocalB
    }
    if (ccaSubClocal) srcA -= alocal

    // mselect (blend factor)
    var (mselR, mselG, mselB) = ccMselect match {
      case 0 => (0, 0, 0) // ZERO
      case 1 => (clocalR, clocalG, clocalB) // CLOCAL
      case 2 => (aother, aother, aother) // AOTHER
      case 3 => (alocal, alocal, alocal) // ALOCAL
      case 4 => (texA, texA, texA) // TEX (alpha)
      case 5 => (texR, texG, texB) // TEXRGB
      case _ => (0, 0, 0)
    }

    var mselA = ccaMselect match {
      case 0 => 0 // ZERO
      case 1 => alocal // ALOCAL
      case 2 => aother // AOTHER
      case 3 => alocal // ALOCAL2
      case 4 => texA // TEX
      case _ => 0
    }

    // reverse_blend: invert when flag is NOT set
    if (!ccReverseBlend) {
      mselR ^= 0xff
      mselG ^= 0xff
      mselB ^= 0xff
    }
    mselR += 1
    mselG += 1
    mselB += 1

    if (!ccaReverseBlend) mselA ^= 0xff
    mselA += 1

    // Multiply
    srcR = (srcR * mselR) >> 8
    srcG = (srcG * mselG) >> 8
    srcB = (srcB * mselB) >> 8
    srcA = (srcA * mselA) >> 8

    // Add
    ccAdd match {
      case 1 => // CC_ADD_CLOCAL
        srcR += clocalR; srcG += clocalG; srcB += clocalB
      case 2 => // CC_ADD_ALOCAL
        srcR += alocal; srcG += alocal; srcB += alocal
      case _ => // 0 = none
    }

    if (ccaAdd != 0) srcA += alocal

    // Clamp
    srcR = CLAMP(srcR)
    srcG = CLAMP(srcG)
    srcB = CLAMP(srcB)
    srcA = CLAMP(srcA)

    // Invert
    if (ccInvertOutput) {
      srcR ^= 0xff; srcG ^= 0xff; srcB ^= 0xff
    }
    if (ccaInvertOutput) srcA ^= 0xff

    (srcR, srcG, srcB, srcA)
  }

  // ========================================================================
  // Depth calculation (lines 985-1007)
  // ========================================================================

  /** Depth calculation. Mirrors the W-buffer depth calculation from vid_voodoo_render.c lines
    * 985-1007.
    */
  def depthCalc(params: VoodooParams, w: Long, z: Int): Int = {
    val wDepth: Int =
      if ((w & 0xffff00000000L) != 0) 0
      else if ((w & 0xffff0000L) == 0) 0xf001
      else {
        val wUpper = ((w >> 16) & 0xffff).toInt
        val exp = voodooFls(wUpper)
        val mant = ((~w.toInt) >>> (19 - exp)) & 0xfff
        val d = (exp << 12) + mant + 1
        if (d > 0xffff) 0xffff else d
      }

    var newDepth =
      if ((params.fbzMode & FBZ_W_BUFFER) != 0) wDepth
      else CLAMP16(z >> 12)

    if ((params.fbzMode & FBZ_DEPTH_BIAS) != 0)
      newDepth = CLAMP16(newDepth + params.zaColor.toShort.toInt)

    newDepth
  }

  // ========================================================================
  // Write pixel (lines 1293-1320, no dither)
  // ========================================================================

  /** Convert 8-bit RGB to RGB565 (no dither path). Mirrors lines 1303-1307 of vid_voodoo_render.c.
    */
  def writePixel(srcR: Int, srcG: Int, srcB: Int): Int = {
    val r = srcR >> 3
    val g = srcG >> 2
    val b = srcB >> 3
    b | (g << 5) | (r << 11)
  }

  // ========================================================================
  // voodooTriangle - 86Box edge-walking algorithm
  // (ported from vid_voodoo_render.c voodoo_triangle + voodoo_half_triangle)
  // ========================================================================

  /** Full triangle rasterization using 86Box's edge-walking algorithm.
    *
    * This is a faithful port of 86Box's voodoo_triangle() (lines 1401-1549) and
    * voodoo_half_triangle() (lines 667-1398).
    *
    * Edge slopes (dxAB, dxAC, dxBC) are precomputed from vertices. Per scanline, x boundaries are
    * computed from vertex positions and slopes. Gradients are reset from base values each scanline
    * and adjusted by dx from the vertex A x position. No edge function testing is used.
    */
  def voodooTriangle(params: VoodooParams): Seq[RefPixel] = {
    val pixels = scala.collection.mutable.ArrayBuffer.empty[RefPixel]

    // --- Subpixel correction (lines 1419-1424) ---
    var dx_sub = 8 - (params.vertexAx & 0xf)
    if ((params.vertexAx & 0xf) > 8) dx_sub += 16
    var dy_sub = 8 - (params.vertexAy & 0xf)
    if ((params.vertexAy & 0xf) > 8) dy_sub += 16

    // --- Load base values (lines 1435-1446) ---
    // base_r/g/b/a/z are uint32_t in C. We use Long to hold the unsigned 32-bit value.
    var base_r: Long = params.startR
    var base_g: Long = params.startG
    var base_b: Long = params.startB
    var base_a: Long = params.startA
    var base_z: Long = params.startZ
    var base_s: Long = params.startS
    var base_t: Long = params.startT
    var base_w_tmu0: Long = params.startW
    var base_w: Long = params.startWFBI

    // --- FBZ_PARAM_ADJUST (lines 1448-1461) ---
    if ((params.fbzColorPath & FBZ_PARAM_ADJUST) != 0) {
      // In C: (dx * params->dRdX + dy * params->dRdY) >> 4
      // dx, dy are int; dRdX, dRdY are int32_t. The multiply is 32-bit wrapping.
      // The result is added to uint32_t base_r.
      base_r = trunc32(base_r + ((dx_sub * params.dRdX + dy_sub * params.dRdY) >> 4))
      base_g = trunc32(base_g + ((dx_sub * params.dGdX + dy_sub * params.dGdY) >> 4))
      base_b = trunc32(base_b + ((dx_sub * params.dBdX + dy_sub * params.dBdY) >> 4))
      base_a = trunc32(base_a + ((dx_sub * params.dAdX + dy_sub * params.dAdY) >> 4))
      base_z = trunc32(base_z + ((dx_sub * params.dZdX + dy_sub * params.dZdY) >> 4))
      base_s += (dx_sub * params.dSdX + dy_sub * params.dSdY) >> 4
      base_t += (dx_sub * params.dTdX + dy_sub * params.dTdY) >> 4
      base_w_tmu0 += (dx_sub * params.dWdX + dy_sub * params.dWdY) >> 4
      base_w += (dx_sub * params.dWdXFBI + dy_sub * params.dWdYFBI) >> 4
    }

    // --- Sign-extend vertices to 32-bit (lines 1465-1483) ---
    val vertexAy = signExtend16(params.vertexAy)
    val vertexBy = signExtend16(params.vertexBy)
    val vertexCy = signExtend16(params.vertexCy)
    val vertexAx = signExtend16(params.vertexAx)
    val vertexBx = signExtend16(params.vertexBx)
    val vertexCx = signExtend16(params.vertexCx)

    // --- Edge slopes (lines 1488-1499) ---
    // dxAB = (int)((((int64_t)vertexBx << 12) - ((int64_t)vertexAx << 12)) << 4) / (vertexBy - vertexAy)
    // This is int64 numerator, int32 denominator, result truncated to int32
    val dxAB: Int =
      if (vertexBy - vertexAy != 0)
        (((vertexBx.toLong << 12) - (vertexAx.toLong << 12)) << 4) / (vertexBy - vertexAy) match {
          case v => v.toInt
        }
      else 0
    val dxAC: Int =
      if (vertexCy - vertexAy != 0)
        (((vertexCx.toLong << 12) - (vertexAx.toLong << 12)) << 4) / (vertexCy - vertexAy) match {
          case v => v.toInt
        }
      else 0
    val dxBC: Int =
      if (vertexCy - vertexBy != 0)
        (((vertexCx.toLong << 12) - (vertexBx.toLong << 12)) << 4) / (vertexCy - vertexBy) match {
          case v => v.toInt
        }
      else 0

    // --- LOD calculation (lines 1501-1530) ---
    val lodMin = (params.tLOD & 0x3f) << 6
    var lodMax = ((params.tLOD >> 6) & 0x3f) << 6
    if (lodMax > 0x800) lodMax = 0x800

    val tempdx =
      (params.dSdX >> 14) * (params.dSdX >> 14) + (params.dTdX >> 14) * (params.dTdX >> 14)
    val tempdy =
      (params.dSdY >> 14) * (params.dSdY >> 14) + (params.dTdY >> 14) * (params.dTdY >> 14)
    val tempLOD = if (tempdx > tempdy) tempdx else tempdy

    val LOD =
      if (tempLOD > 0)
        (scala.math.log(tempLOD.toDouble / (1L << 36).toDouble) / scala.math
          .log(2.0) * 256).toInt >> 2
      else 0

    var lodbias = (params.tLOD >> 12) & 0x3f
    if ((lodbias & 0x20) != 0) lodbias |= ~0x3f
    val baseLod = LOD + (lodbias << 6)

    // --- xdir (line 1511) ---
    val xdir: Int = if (params.sign) -1 else 1

    // --- Y range (lines 1485-1486, 1513) ---
    val vertexAy_adjusted = (vertexAy + 7) >> 4
    val vertexCy_adjusted = (vertexCy + 7) >> 4

    // --- voodoo_half_triangle (lines 667-1398) ---
    var ystart = vertexAy_adjusted
    val yend_orig = vertexCy_adjusted

    // Y clipping (lines 730-752)
    if ((params.fbzMode & 1) != 0 && ystart < params.clipLowY) {
      val dy = params.clipLowY - ystart
      base_r = trunc32(base_r + params.dRdY.toLong * dy)
      base_g = trunc32(base_g + params.dGdY.toLong * dy)
      base_b = trunc32(base_b + params.dBdY.toLong * dy)
      base_a = trunc32(base_a + params.dAdY.toLong * dy)
      base_z = trunc32(base_z + params.dZdY.toLong * dy)
      base_s += params.dSdY * dy
      base_t += params.dTdY * dy
      base_w_tmu0 += params.dWdY * dy
      base_w += params.dWdYFBI * dy
      ystart = params.clipLowY
    }

    var yend = yend_orig
    if ((params.fbzMode & 1) != 0 && yend >= params.clipHighY)
      yend = params.clipHighY

    // --- Scanline loop (line 798) ---
    var y = ystart
    while (y < yend) {
      val real_y = (y << 4) + 8

      // Reset interpolants from base values (lines 807-818)
      var ir: Int = base_r.toInt // uint32 → int32 reinterpret
      var ig: Int = base_g.toInt
      var ib: Int = base_b.toInt
      var ia: Int = base_a.toInt
      var iz: Int = base_z.toInt
      var tmu0_s: Long = base_s
      var tmu0_t: Long = base_t
      var tmu0_w: Long = base_w_tmu0
      var w: Long = base_w

      // Compute x boundaries from edge slopes (lines 820-825)
      // x = (vertexAx << 12) + ((dxAC * (real_y - vertexAy)) >> 4)
      var x_full: Long = (vertexAx.toLong << 12) + ((dxAC.toLong * (real_y - vertexAy)) >> 4)
      var x2_full: Long =
        if (real_y < vertexBy)
          (vertexAx.toLong << 12) + ((dxAB.toLong * (real_y - vertexAy)) >> 4)
        else
          (vertexBx.toLong << 12) + ((dxBC.toLong * (real_y - vertexBy)) >> 4)

      // real_y for pixel address (line 828-830)
      // Skip Y-flip (fbzMode bit 17) — our test doesn't use it
      val pixel_y = real_y >> 4

      // Adjust for xdir (lines 842-845)
      if (xdir > 0)
        x2_full -= (1L << 16)
      else
        x_full -= (1L << 16)

      // Compute dx = pixel offset from vertex A's x position (line 846)
      val dx_val: Int =
        (((x_full + 0x7000L) >> 16) - (((vertexAx.toLong << 12) + 0x7000L) >> 16)).toInt

      // Convert to pixel coordinates (lines 847-848)
      var x: Int = ((x_full + 0x7000L) >> 16).toInt
      var x2: Int = ((x2_full + 0x7000L) >> 16).toInt

      // Apply dx to all gradients (lines 852-863)
      ir = (ir + params.dRdX * dx_val)
      ig = (ig + params.dGdX * dx_val)
      ib = (ib + params.dBdX * dx_val)
      ia = (ia + params.dAdX * dx_val)
      iz = (iz + params.dZdX * dx_val)
      tmu0_s += params.dSdX * dx_val
      tmu0_t += params.dTdX * dx_val
      tmu0_w += params.dWdX * dx_val
      w += params.dWdXFBI * dx_val

      // X clipping (lines 867-911)
      if ((params.fbzMode & 1) != 0) {
        if (xdir > 0) {
          if (x < params.clipLeft) {
            val clip_dx = params.clipLeft - x
            ir += params.dRdX * clip_dx
            ig += params.dGdX * clip_dx
            ib += params.dBdX * clip_dx
            ia += params.dAdX * clip_dx
            iz += params.dZdX * clip_dx
            tmu0_s += params.dSdX * clip_dx
            tmu0_t += params.dTdX * clip_dx
            tmu0_w += params.dWdX * clip_dx
            w += params.dWdXFBI * clip_dx
            x = params.clipLeft
          }
          if (x2 >= params.clipRight)
            x2 = params.clipRight - 1
        } else {
          if (x >= params.clipRight) {
            val clip_dx = (params.clipRight - 1) - x
            ir += params.dRdX * clip_dx
            ig += params.dGdX * clip_dx
            ib += params.dBdX * clip_dx
            ia += params.dAdX * clip_dx
            iz += params.dZdX * clip_dx
            tmu0_s += params.dSdX * clip_dx
            tmu0_t += params.dTdX * clip_dx
            tmu0_w += params.dWdX * clip_dx
            w += params.dWdXFBI * clip_dx
            x = params.clipRight - 1
          }
          if (x2 < params.clipLeft)
            x2 = params.clipLeft
        }
      }

      // Skip empty scanlines (lines 913-916)
      val skipLine = (x2 < x && xdir > 0) || (x2 > x && xdir < 0)

      if (!skipLine) {
        // Per-pixel loop (lines 943-1354)
        var px = x
        var firstIter = true
        var continue = true
        while (continue) {
          // Texture fetch (if enabled)
          var (texR, texG, texB, texA) = (0, 0, 0, 0)
          if ((params.fbzColorPath & FBZCP_TEXTURE_ENABLED) != 0) {
            val result = tmuFetch(params, tmu0_s, tmu0_t, tmu0_w, baseLod, lodMin, lodMax)
            texR = result._1; texG = result._2; texB = result._3; texA = result._4
          }

          // Depth
          val newDepth = depthCalc(params, w, iz)

          // Color combine
          val (srcR, srcG, srcB, srcA) =
            colorCombine(params, ir, ig, ib, ia, iz, texR, texG, texB, texA)

          // Write pixel
          if ((params.fbzMode & FBZ_RGB_WMASK) != 0) {
            val rgb565 = writePixel(srcR, srcG, srcB)
            pixels += RefPixel(px, pixel_y, rgb565, newDepth)
          }

          // Advance gradients (lines 1325-1351)
          if (xdir > 0) {
            ir += params.dRdX; ig += params.dGdX; ib += params.dBdX
            ia += params.dAdX; iz += params.dZdX
            tmu0_s += params.dSdX; tmu0_t += params.dTdX; tmu0_w += params.dWdX
            w += params.dWdXFBI
          } else {
            ir -= params.dRdX; ig -= params.dGdX; ib -= params.dBdX
            ia -= params.dAdX; iz -= params.dZdX
            tmu0_s -= params.dSdX; tmu0_t -= params.dTdX; tmu0_w -= params.dWdX
            w -= params.dWdXFBI
          }

          // Loop termination: do { ... } while (start_x != x2) in C
          // C loop: x advances after pixel, checks old x != x2
          val old_px = px
          px += xdir
          if (old_px == x2) continue = false
        }
      }

      // End-of-scanline: advance base values by dY (lines 1380-1393)
      base_r = trunc32(base_r + params.dRdY.toLong)
      base_g = trunc32(base_g + params.dGdY.toLong)
      base_b = trunc32(base_b + params.dBdY.toLong)
      base_a = trunc32(base_a + params.dAdY.toLong)
      base_z = trunc32(base_z + params.dZdY.toLong)
      base_s += params.dSdY
      base_t += params.dTdY
      base_w_tmu0 += params.dWdY
      base_w += params.dWdYFBI

      y += 1
    }

    pixels.toSeq
  }

  // ========================================================================
  // Helper: build VoodooParams from SpinalVoodoo register values
  // ========================================================================

  /** Convert SpinalVoodoo register values to 86Box-style VoodooParams.
    *
    * SpinalVoodoo stores raw register values:
    *   - Colors: SInt(24 bits) in 12.12 format
    *   - Z: SInt(32 bits) in 20.12 format
    *   - S/T: SInt(32 bits) in 14.18 format
    *   - W: SInt(32 bits) in 2.30 format
    *
    * 86Box applies shifts during register writes:
    *   - S/T: <<14 (from 14.18 to 28.32 effectively)
    *   - W: <<2 (from 2.30 to 4.28 effectively)
    *
    * This helper applies those shifts.
    */
  def fromRegisterValues(
      vertexAx: Int,
      vertexAy: Int,
      vertexBx: Int,
      vertexBy: Int,
      vertexCx: Int,
      vertexCy: Int,
      startR: Int,
      startG: Int,
      startB: Int,
      startA: Int,
      startZ: Int,
      startS: Int,
      startT: Int,
      startW: Int,
      dRdX: Int,
      dGdX: Int,
      dBdX: Int,
      dAdX: Int,
      dZdX: Int,
      dSdX: Int,
      dTdX: Int,
      dWdX: Int,
      dRdY: Int,
      dGdY: Int,
      dBdY: Int,
      dAdY: Int,
      dZdY: Int,
      dSdY: Int,
      dTdY: Int,
      dWdY: Int,
      fbzColorPath: Int,
      fbzMode: Int,
      sign: Boolean,
      textureMode: Int = 0,
      tLOD: Int = 0,
      color0: Int = 0,
      color1: Int = 0,
      zaColor: Int = 0,
      fogMode: Int = 0,
      alphaMode: Int = 0,
      clipLeft: Int = 0,
      clipRight: Int = 0x3ff,
      clipLowY: Int = 0,
      clipHighY: Int = 0x3ff,
      texData: Array[Array[Int]] = Array.fill(9)(Array.empty[Int]),
      texWMask: Array[Int] = Array.fill(9)(0),
      texHMask: Array[Int] = Array.fill(9)(0),
      texShift: Array[Int] = Array.fill(9)(0),
      texLod: Array[Int] = Array.fill(9)(0)
  ): VoodooParams = {
    // 86Box shifts: S/T <<14, W <<2
    VoodooParams(
      vertexAx = vertexAx,
      vertexAy = vertexAy,
      vertexBx = vertexBx,
      vertexBy = vertexBy,
      vertexCx = vertexCx,
      vertexCy = vertexCy,
      startR = startR.toLong & 0xffffffffL,
      startG = startG.toLong & 0xffffffffL,
      startB = startB.toLong & 0xffffffffL,
      startA = startA.toLong & 0xffffffffL,
      startZ = startZ.toLong & 0xffffffffL,
      startS = startS.toLong << 14,
      startT = startT.toLong << 14,
      startW = startW.toLong << 2,
      startWFBI = startW.toLong << 2,
      dRdX = dRdX,
      dGdX = dGdX,
      dBdX = dBdX,
      dAdX = dAdX,
      dZdX = dZdX,
      dSdX = dSdX.toLong << 14,
      dTdX = dTdX.toLong << 14,
      dWdX = dWdX.toLong << 2,
      dWdXFBI = dWdX.toLong << 2,
      dRdY = dRdY,
      dGdY = dGdY,
      dBdY = dBdY,
      dAdY = dAdY,
      dZdY = dZdY,
      dSdY = dSdY.toLong << 14,
      dTdY = dTdY.toLong << 14,
      dWdY = dWdY.toLong << 2,
      dWdYFBI = dWdY.toLong << 2,
      fbzColorPath = fbzColorPath,
      fbzMode = fbzMode,
      fogMode = fogMode,
      alphaMode = alphaMode,
      textureMode = textureMode,
      tLOD = tLOD,
      color0 = color0,
      color1 = color1,
      zaColor = zaColor,
      sign = sign,
      clipLeft = clipLeft,
      clipRight = clipRight,
      clipLowY = clipLowY,
      clipHighY = clipHighY,
      texData = texData,
      texWMask = texWMask,
      texHMask = texHMask,
      texShift = texShift,
      texLod = texLod
    )
  }
}
