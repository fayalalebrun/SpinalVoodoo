package voodoo

import spinal.core._
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

    // Pipeline busy: pixels in flight inside fork-queue-join and collector
    val busy = out Bool ()
  }

  // Track in-flight pixels (max 16 in queue + bilinear expansion)
  val inFlightCount = Reg(UInt(5 bits)) init 0
  when(io.input.fire && !io.output.fire) {
    inFlightCount := inFlightCount + 1
  }.elsewhen(!io.input.fire && io.output.fire) {
    inFlightCount := inFlightCount - 1
  }
  io.busy := inFlightCount =/= 0

  // Palette RAM: 256 entries x 24-bit RGB
  val paletteRam = Mem(Bits(24 bits), 256)
  when(io.paletteWrite.valid) {
    paletteRam.write(io.paletteWrite.payload.address, io.paletteWrite.payload.data)
  }

  // ========================================================================
  // Stage 1: Perspective correction + coordinate generation
  // ========================================================================

  // Decode textureMode register
  val texMode = Tmu.TextureMode.decode(io.input.payload.config.textureMode)

  // Perspective Correction
  val oow = io.input.payload.w // 1/W in 2.30 format
  val sow = io.input.payload.s // S/W in 14.18 format
  val tow = io.input.payload.t // T/W in 14.18 format

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

  // Compute texel coordinates in X.4 fixed-point format (integer texel * 16 + 4-bit fraction)
  // texS/texT contain enough precision for both point sampling and bilinear filtering
  val texS = SInt(18 bits) // 14 integer + 4 fractional
  val texT = SInt(18 bits)

  // Per-pixel LOD adjustment for perspective correction
  val perspLodAdjust = SInt(16 bits)
  perspLodAdjust := S(0, 16 bits)

  when(!texMode.perspectiveEnable) {
    // Non-perspective: sow.raw is SQ(32,18), shift >>14 to get X.4 format
    texS := (sow.raw.asSInt >> 14).resized
    texT := (tow.raw.asSInt >> 14).resized
  } otherwise {
    // Perspective: S = (S/W) / (1/W) via reciprocal LUT
    val oowRaw = oow.raw.asSInt // SInt(32 bits)
    val absOow = oowRaw.abs.resize(32 bits) // UInt(32 bits)
    val clz = clz32(absOow)
    val msbPos = U(31, 6 bits) - clz // UInt(6 bits)

    // Normalize: shift left so MSB is at bit 31
    val norm = (absOow |<< clz).resize(32 bits)
    val index = norm(30 downto 23) // 8-bit table index
    val frac = norm(22 downto 15) // 8-bit interpolation fraction

    // LUT lookup + linear interpolation
    val base = recipTable(index.resize(9 bits))
    val next = recipTable((index +^ U(1)).resize(9 bits))
    val diff = base - next // UInt(17 bits)
    val interp = base - ((diff * frac) >> 8).resize(17 bits)

    // Handle oow <= 0 (degenerate): reciprocal = 0
    val validOow = !oowRaw.msb && (absOow =/= 0)
    val safeInterp = validOow ? interp | U(0, 17 bits)

    // Per-pixel LOD adjustment: log2(W) = 30 - log2(oow_raw)
    when(validOow) {
      val adjInt = (U(30, 6 bits) - msbPos).resize(8 bits)
      val logFrac = logTable(index)
      perspLodAdjust := ((False ## adjInt ## U(0, 8 bits)).asSInt
        - (False ## U(0, 8 bits) ## logFrac).asSInt).resize(16 bits)
    }

    // Multiply: product = sow_raw * interp (signed × unsigned)
    val sowRaw = sow.raw.asSInt // SInt(32 bits)
    val towRaw = tow.raw.asSInt // SInt(32 bits)
    val interpSigned = (False ## safeInterp).asSInt // 18-bit signed (always positive)
    val productS = sowRaw * interpSigned // 50-bit signed
    val productT = towRaw * interpSigned // 50-bit signed

    // Shift to get X.4 texel coordinate: product >> msbPos (was >> msbPos+4 for integer)
    texS := (productS >> msbPos).resized
    texT := (productT >> msbPos).resized
  }

  // Clamp W (force S=T=0 when W is negative)
  val wNegative = oow.raw.msb
  val clampToZero = texMode.clampW && wNegative

  // LOD Calculation
  val tLOD = Tmu.TLOD.decode(io.input.payload.config.tLOD)

  // Compute absolute gradients
  def absAFix(v: AFix, fmt: QFormat): AFix = {
    val result = AFix(fmt)
    when(v.raw.msb) {
      result.raw := (-v).raw.resized
    }.otherwise {
      result.raw := v.raw
    }
    result
  }

  val absDSdX = absAFix(io.input.payload.dSdX, c.texCoordsFormat)
  val absDTdX = absAFix(io.input.payload.dTdX, c.texCoordsFormat)
  val absDSdY = absAFix(io.input.payload.dSdY, c.texCoordsFormat)
  val absDTdY = absAFix(io.input.payload.dTdY, c.texCoordsFormat)

  // Find maximum gradient and compute LOD
  val maxGradX = absDSdX.max(absDTdX)
  val maxGradY = absDSdY.max(absDTdY)
  val maxGrad = maxGradX.max(maxGradY)
  val maxGradRaw = maxGrad.raw.asUInt.resize(32 bits)

  val baseLod_8_8 = SInt(16 bits)
  when(maxGradRaw === 0) {
    baseLod_8_8 := S(-18 * 256, 16 bits)
  }.otherwise {
    val gradClz = clz32(maxGradRaw)
    val gradMsbPos = (U(31, 6 bits) - gradClz).resize(6 bits)
    val gradNorm = (maxGradRaw |<< gradClz).resize(32 bits)
    val gradIndex = gradNorm(30 downto 23)

    val lodIntPart = ((False ## gradMsbPos).asSInt.resize(8 bits) - S(18, 8 bits)).resize(8 bits)
    val lodFracPart = logTable(gradIndex)
    baseLod_8_8 := (lodIntPart.asBits ## lodFracPart.asBits).asSInt
  }

  // 8.8 fixed-point LOD pipeline
  val lodbias_8_8 = (tLOD.lodbias.asSInt << 6).resize(16 bits)
  val lod_8_8 = baseLod_8_8 + perspLodAdjust + lodbias_8_8

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

  val texWidth = (U(1, 10 bits) << texWidthBits).resize(10 bits)
  val texHeight = (U(1, 10 bits) << texHeightBits).resize(10 bits)

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

  val bilinearEnable = texMode.minFilter || texMode.magFilter

  // Point path: integer texel coord at target LOD level
  // texS is X.4 format, shift right by (4 + lodLevel) to get integer texel at target LOD
  val sPoint = (texS >> (U(4) + lodLevel)).resize(14 bits)
  val tPoint = (texT >> (U(4) + lodLevel)).resize(14 bits)

  // Bilinear path: center-adjust by -0.5 texel, then LOD scale, extract fractions
  // adjS = texS - (1 << (3 + lodLevel))  — subtracts 0.5 texel in X.4 at target LOD
  val adjS = (texS - (S(1, 18 bits) |<< (U(3) + lodLevel))).resize(18 bits)
  val adjT = (texT - (S(1, 18 bits) |<< (U(3) + lodLevel))).resize(18 bits)
  val sScaled = (adjS >> lodLevel).resize(18 bits)
  val tScaled = (adjT >> lodLevel).resize(18 bits)
  val ds = sScaled(3 downto 0).asUInt // 4-bit fraction (0-15)
  val dt = tScaled(3 downto 0).asUInt
  val si = (sScaled >> 4).resize(14 bits)
  val ti = (tScaled >> 4).resize(14 bits)

  // Apply clampToZero
  val finalSPoint = clampToZero ? S(0, 14 bits) | sPoint
  val finalTPoint = clampToZero ? S(0, 14 bits) | tPoint
  val finalSi = clampToZero ? S(0, 14 bits) | si
  val finalTi = clampToZero ? S(0, 14 bits) | ti
  val finalDs = clampToZero ? U(0, 4 bits) | ds
  val finalDt = clampToZero ? U(0, 4 bits) | dt

  // ========================================================================
  // Address generation helper
  // ========================================================================

  val is16BitFormat = texMode.format >= Tmu.TextureFormat.ARGB8332
  val bytesPerTexel = Mux(is16BitFormat, U(2), U(1))

  // SST1 texture memory layout: mipmaps are packed sequentially.
  // Cumulative texel count before each LOD level depends on aspect ratio.
  // Table indexed by (aspectRatio, lodLevel), values in texels.
  val lodTexelOffsets = Vec(
    Vec(Seq(0, 65536, 81920, 86016, 87040, 87296, 87360, 87376, 87380).map(v => U(v, 17 bits))),
    Vec(Seq(0, 32768, 40960, 43008, 43520, 43648, 43680, 43688, 43690).map(v => U(v, 17 bits))),
    Vec(Seq(0, 16384, 20480, 21504, 21760, 21824, 21840, 21844, 21846).map(v => U(v, 17 bits))),
    Vec(Seq(0, 8192, 10240, 10752, 10880, 10912, 10920, 10924, 10926).map(v => U(v, 17 bits)))
  )
  val lodTexelOffset = lodTexelOffsets(aspectRatio)(lodLevel)
  val lodBaseOffset =
    Mux(is16BitFormat, (lodTexelOffset << 1).resize(24 bits), lodTexelOffset.resize(24 bits))

  val texBaseByteAddr = (io.input.payload.config.texBaseAddr << 3).resize(c.addressWidth.value bits)

  // Texture memory addressing: linear byte addressing matching real SST-1 layout.
  // Row stride = width_in_texels * bytesPerTexel (i.e. 1 << (texWidthBits + is16bit)).
  def texelAddr(x: UInt, y: UInt): UInt = {
    val rowShift = texWidthBits +^ Mux(is16BitFormat, U(1), U(0))
    val rowOffset = (y.resize(24 bits) << rowShift).resize(24 bits)
    val colOffset = Mux(
      is16BitFormat,
      (x << 1).resize(24 bits), // 16-bit texels: 2 bytes per texel
      x.resize(24 bits) // 8-bit texels:  1 byte per texel
    )
    texBaseByteAddr +
      lodBaseOffset.resize(c.addressWidth.value bits) +
      rowOffset.resize(c.addressWidth.value bits) +
      colOffset.resize(c.addressWidth.value bits)
  }

  // Point: 1 address
  val pointTexelX = wrapOrClamp(finalSPoint, texWidthBits, texMode.clampS)
  val pointTexelY = wrapOrClamp(finalTPoint, texHeightBits, texMode.clampT)
  val pointAddr = texelAddr(pointTexelX, pointTexelY)

  // Bilinear: 4 addresses from 2x2 neighborhood
  val biX0 = wrapOrClamp(finalSi, texWidthBits, texMode.clampS)
  val biX1 = wrapOrClamp((finalSi + 1).resize(14 bits), texWidthBits, texMode.clampS)
  val biY0 = wrapOrClamp(finalTi, texHeightBits, texMode.clampT)
  val biY1 = wrapOrClamp((finalTi + 1).resize(14 bits), texHeightBits, texMode.clampT)
  val biAddr0 = texelAddr(biX0, biY0) // top-left
  val biAddr1 = texelAddr(biX1, biY0) // top-right
  val biAddr2 = texelAddr(biX0, biY1) // bottom-left
  val biAddr3 = texelAddr(biX1, biY1) // bottom-right

  // ========================================================================
  // Stage 2: Expander — 1 input → 1 (point) or 4 (bilinear) outputs
  // ========================================================================

  case class TmuPassthrough() extends Bundle {
    val format = UInt(4 bits)
    val bilinear = Bool()
    val ds = UInt(4 bits)
    val dt = UInt(4 bits)
    val readIdx = UInt(2 bits)
    val ncc = Tmu.NccTableData()
  }

  case class TmuExpanded() extends Bundle {
    val address = UInt(c.addressWidth.value bits)
    val passthrough = TmuPassthrough()
  }

  val expandedStream = Stream(TmuExpanded())
  val expandCount = Reg(UInt(2 bits)) init 0
  val isExpanding = expandCount =/= 0

  // Capture registers for bilinear expansion (addresses 1-3)
  val regAddr1 = Reg(UInt(c.addressWidth.value bits))
  val regAddr2 = Reg(UInt(c.addressWidth.value bits))
  val regAddr3 = Reg(UInt(c.addressWidth.value bits))
  val regPassthrough = Reg(TmuPassthrough())

  // Input is ready only when not in the middle of expanding
  io.input.ready := !isExpanding && expandedStream.ready

  expandedStream.valid := io.input.valid || isExpanding

  // Build passthrough from input
  val inputPassthrough = TmuPassthrough()
  inputPassthrough.format := texMode.format
  inputPassthrough.bilinear := bilinearEnable
  inputPassthrough.ds := finalDs
  inputPassthrough.dt := finalDt
  inputPassthrough.readIdx := 0
  inputPassthrough.ncc := io.input.payload.config.ncc

  // Select address and passthrough based on expansion state.
  // Fields must be assigned individually to avoid SpinalHDL ASSIGNMENT OVERLAP errors
  // (cannot assign a bundle then override a field).
  when(isExpanding) {
    expandedStream.payload.passthrough.format := regPassthrough.format
    expandedStream.payload.passthrough.bilinear := regPassthrough.bilinear
    expandedStream.payload.passthrough.ds := regPassthrough.ds
    expandedStream.payload.passthrough.dt := regPassthrough.dt
    expandedStream.payload.passthrough.readIdx := expandCount
    expandedStream.payload.passthrough.ncc := regPassthrough.ncc
    switch(expandCount) {
      is(1) { expandedStream.payload.address := regAddr1 }
      is(2) { expandedStream.payload.address := regAddr2 }
      default { expandedStream.payload.address := regAddr3 }
    }
  }.otherwise {
    expandedStream.payload.passthrough := inputPassthrough
    expandedStream.payload.address := bilinearEnable ? biAddr0 | pointAddr
  }

  when(expandedStream.fire) {
    when(!isExpanding && bilinearEnable) {
      // First bilinear item fired; start expansion for items 1-3
      expandCount := 1
      regAddr1 := biAddr1
      regAddr2 := biAddr2
      regAddr3 := biAddr3
      regPassthrough := inputPassthrough
    }.elsewhen(isExpanding && expandCount < 3) {
      expandCount := expandCount + 1
    }.elsewhen(isExpanding) {
      expandCount := 0
    }
  }

  // ========================================================================
  // Stage 3: Fork into memory request and passthrough queue
  // ========================================================================

  val (toMemory, toQueue) = StreamFork2(expandedStream)

  // Memory request path
  val cmdStream = toMemory.translateWith {
    val cmd = Fragment(BmbCmd(Tmu.bmbParams(c)))
    cmd.fragment.address := toMemory.payload.address
    cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
    cmd.fragment.length := 1 // 2 bytes - 1
    cmd.fragment.source := 0
    cmd.last := True
    cmd
  }
  io.texRead.cmd << cmdStream

  // Passthrough queue — carries passthrough data plus address bit 1 for byte-lane selection.
  // The address is the single source of truth; we extract the relevant bit here at the fork
  // point rather than duplicating it into the passthrough bundle.
  case class QueuedData() extends Bundle {
    val passthrough = TmuPassthrough()
    val addrHalf = Bool() // address(1): selects upper/lower 16-bit half of 32-bit BMB response
    val addrByte = Bool() // address(0): selects byte within 16-bit half (for 8-bit formats)
  }
  val queuedData = toQueue
    .map { e =>
      val d = QueuedData()
      d.passthrough := e.passthrough
      d.addrHalf := e.address(1)
      d.addrByte := e.address(0)
      d
    }
    .queue(16)

  // ========================================================================
  // Stage 4: Join memory response with queued passthrough data
  // ========================================================================

  val rspStream = io.texRead.rsp.takeWhen(io.texRead.rsp.last).translateWith {
    io.texRead.rsp.fragment.data
  }

  val joined = StreamJoin(rspStream, queuedData)

  // ========================================================================
  // Stage 5: Format decode
  // ========================================================================

  case class DecodedTexel() extends Bundle {
    val r = UInt(8 bits)
    val g = UInt(8 bits)
    val b = UInt(8 bits)
    val a = UInt(8 bits)
    val passthrough = TmuPassthrough()
  }

  val decoded = Stream(DecodedTexel())
  decoded << joined.translateWith {
    val rspData32 = joined.payload._1
    val queued = joined.payload._2
    val pass = queued.passthrough
    // Select correct 16-bit half of 32-bit BMB response based on texel address alignment
    val texelData = Mux(queued.addrHalf, rspData32(31 downto 16), rspData32(15 downto 0))
    // For 8-bit formats: select correct byte within 16-bit half using address bit 0
    val texelByte = Mux(queued.addrByte, texelData(15 downto 8), texelData(7 downto 0))
    val dr = UInt(8 bits)
    val dg = UInt(8 bits)
    val db = UInt(8 bits)
    val da = UInt(8 bits)

    // NCC decode: Y + I + Q chrominance lookup
    def nccDecode(texByte: Bits, ncc: Tmu.NccTableData): (UInt, UInt, UInt) = {
      val yVal = ncc.y(texByte(7 downto 4).asUInt)
      val iEntry = ncc.i(texByte(3 downto 2).asUInt)
      val qEntry = ncc.q(texByte(1 downto 0).asUInt)
      val iR = iEntry(26 downto 18).asSInt; val iG = iEntry(17 downto 9).asSInt;
      val iB = iEntry(8 downto 0).asSInt
      val qR = qEntry(26 downto 18).asSInt; val qG = qEntry(17 downto 9).asSInt;
      val qB = qEntry(8 downto 0).asSInt
      val rRaw = (False ## yVal).asSInt.resize(11 bits) + iR.resize(11 bits) + qR.resize(11 bits)
      val gRaw = (False ## yVal).asSInt.resize(11 bits) + iG.resize(11 bits) + qG.resize(11 bits)
      val bRaw = (False ## yVal).asSInt.resize(11 bits) + iB.resize(11 bits) + qB.resize(11 bits)
      (clampToU8(rRaw), clampToU8(gRaw), clampToU8(bRaw))
    }

    // Palette lookup: read 8-bit index from texelByte (used by P8 and AP88)
    val paletteColor = paletteRam.readAsync(texelByte.asUInt)

    switch(pass.format) {
      is(Tmu.TextureFormat.RGB332) {
        dr := expandTo8(texelByte(7 downto 5).asUInt, 3)
        dg := expandTo8(texelByte(4 downto 2).asUInt, 3)
        db := expandTo8(texelByte(1 downto 0).asUInt, 2)
        da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.YIQ422) {
        val (r, g, b) = nccDecode(texelByte, pass.ncc)
        dr := r; dg := g; db := b; da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.A8) {
        // 86Box: makergba(dat, dat, dat, dat) — R=G=B=A=dat
        val dat = texelByte.asUInt
        dr := dat
        dg := dat
        db := dat
        da := dat
      }
      is(Tmu.TextureFormat.I8) {
        val intensity = texelByte.asUInt
        dr := intensity
        dg := intensity
        db := intensity
        da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.AI44) {
        val alpha = texelByte(7 downto 4).asUInt
        val intensity = texelByte(3 downto 0).asUInt
        dr := expandTo8(intensity, 4)
        dg := expandTo8(intensity, 4)
        db := expandTo8(intensity, 4)
        da := expandTo8(alpha, 4)
      }
      is(Tmu.TextureFormat.P8) {
        dr := paletteColor(23 downto 16).asUInt
        dg := paletteColor(15 downto 8).asUInt
        db := paletteColor(7 downto 0).asUInt
        da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.ARGB8332) {
        da := texelData(15 downto 8).asUInt
        dr := expandTo8(texelData(7 downto 5).asUInt, 3)
        dg := expandTo8(texelData(4 downto 2).asUInt, 3)
        db := expandTo8(texelData(1 downto 0).asUInt, 2)
      }
      is(Tmu.TextureFormat.AYIQ8422) {
        val (r, g, b) = nccDecode(texelData(7 downto 0), pass.ncc)
        dr := r; dg := g; db := b; da := texelData(15 downto 8).asUInt
      }
      is(Tmu.TextureFormat.RGB565) {
        dr := expandTo8(texelData(15 downto 11).asUInt, 5)
        dg := expandTo8(texelData(10 downto 5).asUInt, 6)
        db := expandTo8(texelData(4 downto 0).asUInt, 5)
        da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.ARGB1555) {
        da := expandTo8(texelData(15 downto 15).asUInt, 1)
        dr := expandTo8(texelData(14 downto 10).asUInt, 5)
        dg := expandTo8(texelData(9 downto 5).asUInt, 5)
        db := expandTo8(texelData(4 downto 0).asUInt, 5)
      }
      is(Tmu.TextureFormat.ARGB4444) {
        da := expandTo8(texelData(15 downto 12).asUInt, 4)
        dr := expandTo8(texelData(11 downto 8).asUInt, 4)
        dg := expandTo8(texelData(7 downto 4).asUInt, 4)
        db := expandTo8(texelData(3 downto 0).asUInt, 4)
      }
      is(Tmu.TextureFormat.AI88) {
        val alpha = texelData(15 downto 8).asUInt
        val intensity = texelData(7 downto 0).asUInt
        dr := intensity
        dg := intensity
        db := intensity
        da := alpha
      }
      is(Tmu.TextureFormat.AP88) {
        dr := paletteColor(23 downto 16).asUInt
        dg := paletteColor(15 downto 8).asUInt
        db := paletteColor(7 downto 0).asUInt
        da := texelData(15 downto 8).asUInt
      }
      default {
        dr := expandTo8(texelData(15 downto 11).asUInt, 5)
        dg := expandTo8(texelData(10 downto 5).asUInt, 6)
        db := expandTo8(texelData(4 downto 0).asUInt, 5)
        da := U(255, 8 bits)
      }
    }

    val result = DecodedTexel()
    result.r := dr
    result.g := dg
    result.b := db
    result.a := da
    result.passthrough := pass
    result
  }

  // ========================================================================
  // Stage 6: Collector — point passes through, bilinear accumulates 4 → blend
  // ========================================================================

  val collectCount = Reg(UInt(2 bits)) init 0
  val storedR = Vec(Reg(UInt(8 bits)), 3)
  val storedG = Vec(Reg(UInt(8 bits)), 3)
  val storedB = Vec(Reg(UInt(8 bits)), 3)
  val storedA = Vec(Reg(UInt(8 bits)), 3)
  val storedDs = Reg(UInt(4 bits))
  val storedDt = Reg(UInt(4 bits))

  // Bilinear blend weights
  val blendDs = Mux(collectCount =/= 0, storedDs, decoded.payload.passthrough.ds)
  val blendDt = Mux(collectCount =/= 0, storedDt, decoded.payload.passthrough.dt)
  val w0 =
    (U(16, 5 bits) - blendDs.resize(5 bits)) * (U(16, 5 bits) - blendDt.resize(5 bits)) // max 256
  val w1 = blendDs.resize(5 bits) * (U(16, 5 bits) - blendDt.resize(5 bits))
  val w2 = (U(16, 5 bits) - blendDs.resize(5 bits)) * blendDt.resize(5 bits)
  val w3 = blendDs.resize(5 bits) * blendDt.resize(5 bits)

  // Blend computation: use stored texels 0-2 + current (texel 3)
  def blendChannel(t0: UInt, t1: UInt, t2: UInt, t3: UInt): UInt = {
    val sum = (t0.resize(18 bits) * w0.resize(10 bits)) +
      (t1.resize(18 bits) * w1.resize(10 bits)) +
      (t2.resize(18 bits) * w2.resize(10 bits)) +
      (t3.resize(18 bits) * w3.resize(10 bits))
    (sum >> 8).resize(8 bits)
  }

  val storedChannels = Seq(storedR, storedG, storedB, storedA)
  val decodedChannels =
    Seq(decoded.payload.r, decoded.payload.g, decoded.payload.b, decoded.payload.a)
  val blendedChannels = storedChannels.zip(decodedChannels).map { case (stored, dec) =>
    blendChannel(stored(0), stored(1), stored(2), dec)
  }

  val isCollecting = collectCount =/= 0
  val isBilinear = Mux(isCollecting, True, decoded.payload.passthrough.bilinear)
  val bilinearAccumulating = isBilinear && (collectCount < 3)

  // Output stream
  io.output.valid := decoded.valid && !bilinearAccumulating
  decoded.ready := bilinearAccumulating || io.output.ready

  Seq(
    io.output.payload.texture.r,
    io.output.payload.texture.g,
    io.output.payload.texture.b,
    io.output.payload.textureAlpha
  )
    .zip(blendedChannels)
    .zip(decodedChannels)
    .foreach { case ((out, blnd), dec) =>
      out := Mux(isBilinear, blnd, dec)
    }

  when(decoded.fire) {
    when(isBilinear && collectCount < 3) {
      // Accumulate texels 0, 1, 2
      storedChannels.zip(decodedChannels).foreach { case (stored, dec) =>
        stored(collectCount) := dec
      }
      when(collectCount === 0) {
        storedDs := decoded.payload.passthrough.ds
        storedDt := decoded.payload.passthrough.dt
      }
      collectCount := collectCount + 1
    }.otherwise {
      // Point sample pass-through or bilinear item 3 (blend fires on output)
      collectCount := 0
    }
  }
}

object Tmu {

  /** Palette write command */
  case class PaletteWrite() extends Bundle {
    val address = UInt(8 bits)
    val data = Bits(24 bits)
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
  case class TmuConfig() extends Bundle {
    val textureMode = Bits(32 bits)
    val texBaseAddr = UInt(24 bits)
    val tLOD = Bits(27 bits)
    val ncc = NccTableData()
  }

  /** TMU input bundle */
  case class Input(c: voodoo.Config) extends Bundle {
    val s = AFix(c.texCoordsFormat)
    val t = AFix(c.texCoordsFormat)
    val w = AFix(c.wFormat)
    val cOther = Color.u8()
    val aOther = UInt(8 bits)
    val config = TmuConfig()
    val dSdX = AFix(c.texCoordsFormat)
    val dTdX = AFix(c.texCoordsFormat)
    val dSdY = AFix(c.texCoordsFormat)
    val dTdY = AFix(c.texCoordsFormat)
  }

  /** TMU output bundle */
  case class Output(c: voodoo.Config) extends Bundle {
    val texture = Color.u8()
    val textureAlpha = UInt(8 bits)
  }

  /** BMB parameters for texture memory access */
  def bmbParams(c: voodoo.Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = false,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}
