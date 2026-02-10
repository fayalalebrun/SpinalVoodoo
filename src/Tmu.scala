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
  *   1. Perspective correction: S' = S/W, T' = T/W (skip if W ≈ 1)
  *   2. Address generation: compute texel address from S', T'
  *   3. Fork: send address to memory, queue passthrough data
  *   4. Join: combine memory response with queued data
  *   5. Format conversion: decode texel data to RGB888
  *
  * The pipeline uses StreamFork to split the input into two paths:
  *   - Path A: Goes to BMB memory bus for texture fetch
  *   - Path B: Queued to preserve pixel data while waiting for memory
  * StreamJoin reunites both paths when the memory response arrives.
  */
case class Tmu(c: voodoo.Config) extends Component {
  val io = new Bundle {
    // Input stream (from Rasterizer for TMU0, from TMU0 for TMU1)
    val input = slave Stream (Tmu.Input(c))

    // Output stream (to TMU1 for TMU0, to ColorCombine for TMU1)
    val output = master Stream (Tmu.Output(c))

    // Texture memory read bus
    val texRead = master(Bmb(Tmu.bmbParams(c)))
  }

  // ========================================================================
  // Stage 1: Address Generation (combinational on input)
  // ========================================================================

  // Decode textureMode register
  val texMode = Tmu.TextureMode.decode(io.input.payload.config.textureMode)

  // Perspective Correction
  // The rasterizer interpolates S/W, T/W, and 1/W.
  // When tpersp_st=1: compute S = (S/W) / (1/W) via reciprocal LUT
  // When tpersp_st=0: pass through S/W and T/W directly (correct when W=1)
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
  // Maps normalized mantissa fraction to sub-integer log2 value
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

  // Compute perspective-corrected integer texel coordinates
  val sInt = SInt(14 bits)
  val tInt = SInt(14 bits)

  // Per-pixel LOD adjustment for perspective correction
  // log2(W) = log2(2^30 / oow_raw) = 30 - log2(oow_raw)
  // adjustment_8_8 = ((30 - msbPos) << 8) - logtable[frac]
  val perspLodAdjust = SInt(16 bits)
  perspLodAdjust := S(0, 16 bits)

  when(!texMode.perspectiveEnable) {
    // Non-perspective: sow/tow ARE the S/T coords, extract integer part
    sInt := sow.floor(0).asSInt.resized
    tInt := tow.floor(0).asSInt.resized
  } otherwise {
    // Perspective: S = (S/W) / (1/W) via reciprocal LUT
    val oowRaw = oow.raw.asSInt                         // SInt(32 bits)
    val absOow = oowRaw.abs.resize(32 bits)                // UInt(32 bits)
    val clz = clz32(absOow)
    val msbPos = U(31, 6 bits) - clz                     // UInt(6 bits)

    // Normalize: shift left so MSB is at bit 31
    val norm = (absOow |<< clz).resize(32 bits)
    val index = norm(30 downto 23)                        // 8-bit table index
    val frac = norm(22 downto 15)                         // 8-bit interpolation fraction

    // LUT lookup + linear interpolation
    val base = recipTable(index.resize(9 bits))
    val next = recipTable((index +^ U(1)).resize(9 bits))
    val diff = base - next                                // UInt(17 bits)
    val interp = base - ((diff * frac) >> 8).resize(17 bits)

    // Handle oow <= 0 (degenerate): reciprocal = 0
    val validOow = !oowRaw.msb && (absOow =/= 0)
    val safeInterp = validOow ? interp | U(0, 17 bits)

    // Per-pixel LOD adjustment: log2(W) = 30 - log2(oow_raw)
    // adjustment = ((30 - msbPos) << 8) - logTable[index]
    when(validOow) {
      val adjInt = (U(30, 6 bits) - msbPos).resize(8 bits)   // UInt, 0-30
      val logFrac = logTable(index)                            // UInt(8 bits)
      // adjInt*256 - logFrac, as signed 16-bit
      perspLodAdjust := ((False ## adjInt ## U(0, 8 bits)).asSInt
                       - (False ## U(0, 8 bits) ## logFrac).asSInt).resize(16 bits)
    }

    // Multiply: product = sow_raw * interp (signed × unsigned)
    val sowRaw = sow.raw.asSInt                           // SInt(32 bits)
    val towRaw = tow.raw.asSInt                           // SInt(32 bits)
    val interpSigned = (False ## safeInterp).asSInt        // 18-bit signed (always positive)
    val productS = sowRaw * interpSigned                   // 50-bit signed
    val productT = towRaw * interpSigned                   // 50-bit signed

    // Shift to get integer texel coordinate: product >> (msbPos + 4)
    val shiftAmount = (msbPos + U(4)).resize(6 bits)
    sInt := (productS >> shiftAmount).resized
    tInt := (productT >> shiftAmount).resized
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
      // Negation adds an extra bit, truncate it back
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
  // Full-precision baseLod using CLZ + logTable on raw 32-bit gradient value
  // LOD = log2(gradient_texels_per_pixel) = log2(raw / 2^18) = log2(raw) - 18
  // baseLod_8_8 = ((msbPos - 18) << 8) + logTable[mantissa]  (8.8 fixed-point)
  val maxGradRaw = maxGrad.raw.asUInt.resize(32 bits)

  val baseLod_8_8 = SInt(16 bits)
  when(maxGradRaw === 0) {
    baseLod_8_8 := S(-18 * 256, 16 bits)
  }.otherwise {
    val gradClz = clz32(maxGradRaw)
    val gradMsbPos = (U(31, 6 bits) - gradClz).resize(6 bits)
    val gradNorm = (maxGradRaw |<< gradClz).resize(32 bits)
    val gradIndex = gradNorm(30 downto 23)  // 8-bit logTable index

    // Integer part: msbPos - 18 (range -18..+13)
    val lodIntPart = ((False ## gradMsbPos).asSInt.resize(8 bits) - S(18, 8 bits)).resize(8 bits)
    // Fractional part from logTable (0-255)
    val lodFracPart = logTable(gradIndex)
    // Combine: 8-bit signed integer || 8-bit unsigned fraction = 16-bit 8.8 format
    baseLod_8_8 := (lodIntPart.asBits ## lodFracPart.asBits).asSInt
  }

  // 8.8 fixed-point LOD pipeline
  val lodbias_8_8 = (tLOD.lodbias.asSInt << 6).resize(16 bits)    // 4.2 → 8.8
  val lod_8_8 = baseLod_8_8 + perspLodAdjust + lodbias_8_8        // SInt(16)

  // Clamp bounds in 8.8 format: lodmin/lodmax are in 4.2 format, shift <<6 for 8.8
  val lodMin_8_8 = (tLOD.lodmin << 6).resize(12 bits)             // UInt(12)
  val lodMax_8_8_raw = (tLOD.lodmax << 6).resize(12 bits)
  val lodMax_8_8 = Mux(lodMax_8_8_raw > U(0x800, 12 bits), U(0x800, 12 bits), lodMax_8_8_raw)

  // Clamp lod_8_8 to [lodMin_8_8, lodMax_8_8]
  val lodClamped_8_8 = SInt(16 bits)
  when(lod_8_8 < (False ## lodMin_8_8).asSInt.resize(16 bits)) {
    lodClamped_8_8 := (False ## lodMin_8_8).asSInt.resize(16 bits)
  }.elsewhen(lod_8_8 > (False ## lodMax_8_8).asSInt.resize(16 bits)) {
    lodClamped_8_8 := (False ## lodMax_8_8).asSInt.resize(16 bits)
  }.otherwise {
    lodClamped_8_8 := lod_8_8
  }

  // Extract integer LOD level: lodClamped_8_8 >> 8, clamped to [0, 8]
  val lodInt = (lodClamped_8_8 >> 8).resize(16 bits)
  val lodLevel = UInt(4 bits)
  when(lodInt < 0) {
    lodLevel := 0
  }.elsewhen(lodInt > 8) {
    lodLevel := 8
  }.otherwise {
    lodLevel := lodInt.asUInt.resize(4 bits)
  }

  // Coordinate Wrapping/Clamping (sInt/tInt computed above in perspective correction)

  // Compute texture dimensions based on LOD and aspect ratio
  val baseDimBits = U(8, 4 bits)
  val lodDimBits = (baseDimBits - lodLevel).resize(4 bits)
  val aspectRatio = tLOD.lodAspect
  val sIsWider = tLOD.lodSIsWider

  val texWidthBits = UInt(4 bits)
  val texHeightBits = UInt(4 bits)

  when(aspectRatio === 0) {
    texWidthBits := lodDimBits
    texHeightBits := lodDimBits
  }.elsewhen(aspectRatio === 1) {
    when(sIsWider) {
      texWidthBits := lodDimBits + 1
      texHeightBits := lodDimBits
    }.otherwise {
      texWidthBits := lodDimBits
      texHeightBits := lodDimBits + 1
    }
  }.elsewhen(aspectRatio === 2) {
    when(sIsWider) {
      texWidthBits := lodDimBits + 2
      texHeightBits := lodDimBits
    }.otherwise {
      texWidthBits := lodDimBits
      texHeightBits := lodDimBits + 2
    }
  }.otherwise {
    when(sIsWider) {
      texWidthBits := lodDimBits + 3
      texHeightBits := lodDimBits
    }.otherwise {
      texWidthBits := lodDimBits
      texHeightBits := lodDimBits + 3
    }
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

  val texelX = clampToZero ? U(0, 10 bits) | wrapOrClamp(sInt, texWidthBits, texMode.clampS)
  val texelY = clampToZero ? U(0, 10 bits) | wrapOrClamp(tInt, texHeightBits, texMode.clampT)

  // Address Generation
  val is16BitFormat = texMode.format >= Tmu.TextureFormat.ARGB8332
  val bytesPerTexel = Mux(is16BitFormat, U(2), U(1))

  val lodBaseOffset = UInt(24 bits)
  lodBaseOffset := 0
  when(lodLevel === 0) {
    lodBaseOffset := 0
  }.elsewhen(lodLevel === 1) {
    lodBaseOffset := (U(256 * 256) * bytesPerTexel).resize(24 bits)
  }.elsewhen(lodLevel === 2) {
    lodBaseOffset := ((U(256 * 256) + U(128 * 128)) * bytesPerTexel).resize(24 bits)
  }.elsewhen(lodLevel === 3) {
    lodBaseOffset := ((U(256 * 256) + U(128 * 128) + U(64 * 64)) * bytesPerTexel).resize(24 bits)
  }.elsewhen(lodLevel === 4) {
    lodBaseOffset := ((U(256 * 256) + U(128 * 128) + U(64 * 64) + U(32 * 32)) * bytesPerTexel)
      .resize(24 bits)
  }.elsewhen(lodLevel === 5) {
    lodBaseOffset := ((U(256 * 256) + U(128 * 128) + U(64 * 64) + U(32 * 32) + U(
      16 * 16
    )) * bytesPerTexel).resize(24 bits)
  }.elsewhen(lodLevel === 6) {
    lodBaseOffset := ((U(256 * 256) + U(128 * 128) + U(64 * 64) + U(32 * 32) + U(16 * 16) + U(
      8 * 8
    )) * bytesPerTexel).resize(24 bits)
  }.elsewhen(lodLevel === 7) {
    lodBaseOffset := ((U(256 * 256) + U(128 * 128) + U(64 * 64) + U(32 * 32) + U(16 * 16) + U(
      8 * 8
    ) + U(4 * 4)) * bytesPerTexel).resize(24 bits)
  }.otherwise {
    lodBaseOffset := ((U(256 * 256) + U(128 * 128) + U(64 * 64) + U(32 * 32) + U(16 * 16) + U(
      8 * 8
    ) + U(4 * 4) + U(2 * 2)) * bytesPerTexel).resize(24 bits)
  }

  val texStride = texWidth
  val texelOffset = (texelY.resize(20 bits) * texStride.resize(20 bits) + texelX.resize(20 bits))
  val texelByteOffset =
    Mux(is16BitFormat, (texelOffset << 1).resize(24 bits), texelOffset.resize(24 bits))

  // texBaseAddr register value is address/8 (8-byte aligned), so shift left by 3
  val texBaseByteAddr = (io.input.payload.config.texBaseAddr << 3).resize(c.addressWidth.value bits)
  val texAddr = texBaseByteAddr +
    lodBaseOffset.resize(c.addressWidth.value bits) +
    texelByteOffset.resize(c.addressWidth.value bits)

  // ========================================================================
  // Stage 2: Fork input into memory request and passthrough queue
  // ========================================================================

  // Bundle to pass through the queue (everything except what we compute from memory)
  case class TmuPassthrough() extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val textureMode = Bits(32 bits) // Need format for decode
  }

  // Create passthrough data
  val passthrough = TmuPassthrough()
  passthrough.coords := io.input.payload.coords
  passthrough.textureMode := io.input.payload.config.textureMode

  // Create the stream with computed address and passthrough data
  case class TmuInternal() extends Bundle {
    val address = UInt(c.addressWidth.value bits)
    val passthrough = TmuPassthrough()
  }

  val internalStream = io.input.translateWith {
    val data = TmuInternal()
    data.address := texAddr
    data.passthrough := passthrough
    data
  }

  // Fork into two streams: one for memory, one for passthrough queue
  val (toMemory, toQueue) = StreamFork2(internalStream)

  // ========================================================================
  // Stage 3a: Memory request path - translate to BMB command
  // ========================================================================

  // Convert to BMB command stream
  val cmdStream = toMemory.translateWith {
    val cmd = Fragment(BmbCmd(Tmu.bmbParams(c)))
    cmd.fragment.address := toMemory.payload.address
    cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
    cmd.fragment.length := 1 // 2 bytes - 1
    cmd.fragment.source := 0
    cmd.last := True
    cmd
  }

  // Connect to BMB command port
  io.texRead.cmd << cmdStream

  // ========================================================================
  // Stage 3b: Passthrough queue - buffer data while waiting for memory
  // ========================================================================

  // Queue passthrough data (depth of 4 to handle memory latency)
  val queuedPassthrough = toQueue.map(_.passthrough).queue(4)

  // ========================================================================
  // Stage 4: Join memory response with queued passthrough data
  // ========================================================================

  // Extract texel data from BMB response
  val rspStream = io.texRead.rsp.takeWhen(io.texRead.rsp.last).translateWith {
    io.texRead.rsp.fragment.data(15 downto 0)
  }

  // Join response with passthrough data
  val joined = StreamJoin(rspStream, queuedPassthrough)

  // ========================================================================
  // Stage 5: Format decode and output
  // ========================================================================

  io.output << joined.translateWith {
    val texelData = joined.payload._1
    val pass = joined.payload._2

    // Decode texture format
    val texModeDecoded = Tmu.TextureMode.decode(pass.textureMode)

    // Helper: expand N bits to 8 bits by replicating MSBs
    def expand5to8(v: UInt): UInt = v @@ v(4 downto 2)
    def expand6to8(v: UInt): UInt = v @@ v(5 downto 4)
    def expand4to8(v: UInt): UInt = v @@ v
    def expand3to8(v: UInt): UInt = v @@ v @@ v(2 downto 1)
    def expand2to8(v: UInt): UInt = v @@ v @@ v @@ v
    def expand1to8(v: UInt): UInt = Mux(v(0), U(255, 8 bits), U(0, 8 bits))

    val decodedR = UInt(8 bits)
    val decodedG = UInt(8 bits)
    val decodedB = UInt(8 bits)
    val decodedA = UInt(8 bits)

    switch(texModeDecoded.format) {
      is(Tmu.TextureFormat.RGB332) {
        decodedR := expand3to8(texelData(7 downto 5).asUInt)
        decodedG := expand3to8(texelData(4 downto 2).asUInt)
        decodedB := expand2to8(texelData(1 downto 0).asUInt)
        decodedA := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.A8) {
        decodedR := U(255, 8 bits)
        decodedG := U(255, 8 bits)
        decodedB := U(255, 8 bits)
        decodedA := texelData(7 downto 0).asUInt
      }
      is(Tmu.TextureFormat.I8) {
        val intensity = texelData(7 downto 0).asUInt
        decodedR := intensity
        decodedG := intensity
        decodedB := intensity
        decodedA := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.AI44) {
        val alpha = texelData(7 downto 4).asUInt
        val intensity = texelData(3 downto 0).asUInt
        decodedR := expand4to8(intensity)
        decodedG := expand4to8(intensity)
        decodedB := expand4to8(intensity)
        decodedA := expand4to8(alpha)
      }
      is(Tmu.TextureFormat.ARGB8332) {
        decodedA := texelData(15 downto 8).asUInt
        decodedR := expand3to8(texelData(7 downto 5).asUInt)
        decodedG := expand3to8(texelData(4 downto 2).asUInt)
        decodedB := expand2to8(texelData(1 downto 0).asUInt)
      }
      is(Tmu.TextureFormat.RGB565) {
        decodedR := expand5to8(texelData(15 downto 11).asUInt)
        decodedG := expand6to8(texelData(10 downto 5).asUInt)
        decodedB := expand5to8(texelData(4 downto 0).asUInt)
        decodedA := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.ARGB1555) {
        decodedA := expand1to8(texelData(15 downto 15).asUInt)
        decodedR := expand5to8(texelData(14 downto 10).asUInt)
        decodedG := expand5to8(texelData(9 downto 5).asUInt)
        decodedB := expand5to8(texelData(4 downto 0).asUInt)
      }
      is(Tmu.TextureFormat.ARGB4444) {
        decodedA := expand4to8(texelData(15 downto 12).asUInt)
        decodedR := expand4to8(texelData(11 downto 8).asUInt)
        decodedG := expand4to8(texelData(7 downto 4).asUInt)
        decodedB := expand4to8(texelData(3 downto 0).asUInt)
      }
      is(Tmu.TextureFormat.AI88) {
        val alpha = texelData(15 downto 8).asUInt
        val intensity = texelData(7 downto 0).asUInt
        decodedR := intensity
        decodedG := intensity
        decodedB := intensity
        decodedA := alpha
      }
      default {
        // Treat as RGB565 for unsupported formats
        decodedR := expand5to8(texelData(15 downto 11).asUInt)
        decodedG := expand6to8(texelData(10 downto 5).asUInt)
        decodedB := expand5to8(texelData(4 downto 0).asUInt)
        decodedA := U(255, 8 bits)
      }
    }

    val out = Tmu.Output(c)
    out.coords := pass.coords
    out.texture.r := decodedR
    out.texture.g := decodedG
    out.texture.b := decodedB
    out.textureAlpha := decodedA
    out
  }
}

object Tmu {

  /** Decoded textureMode register fields */
  case class TextureMode() extends Bundle {
    val perspectiveEnable = Bool() // Bit 0: tpersp_st - perspective correction
    val minFilter = Bool() // Bit 1: tminfilter - 0=point, 1=bilinear (TODO)
    val magFilter = Bool() // Bit 2: tmagfilter - 0=point, 1=bilinear (TODO)
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

  /** Per-TMU configuration (captured per-triangle) */
  case class TmuConfig() extends Bundle {
    val textureMode = Bits(32 bits)
    val texBaseAddr = UInt(24 bits)
    val tLOD = Bits(27 bits)
  }

  /** TMU input bundle */
  case class Input(c: voodoo.Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
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
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
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
