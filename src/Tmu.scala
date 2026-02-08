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
  // TODO: Implement proper perspective division when tpersp_st=1
  // For now, pass through S/W and T/W directly (correct when W=1)
  val oow = io.input.payload.w // 1/W in 2.30 format
  val sow = io.input.payload.s // S/W in 14.18 format
  val tow = io.input.payload.t // T/W in 14.18 format
  val sCorrected = sow
  val tCorrected = tow

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
  val maxGradInt = maxGrad.floor(0).asUInt.resize(14 bits)

  val lodRaw = SInt(6 bits)
  when(maxGradInt === 0) {
    lodRaw := S(-18)
  }.otherwise {
    val msbPos = UInt(4 bits)
    msbPos := 0
    for (i <- 0 until 14) {
      when(maxGradInt(i)) {
        msbPos := i
      }
    }
    lodRaw := msbPos.asSInt.resize(6 bits)
  }

  val lodBiasInt = tLOD.lodbias.asSInt >> 2
  val lodWithBias = lodRaw + lodBiasInt
  val lodMinInt = (tLOD.lodmin >> 2).asSInt.resize(6 bits)
  val lodMaxInt = (tLOD.lodmax >> 2).asSInt.resize(6 bits)

  val lodClamped = SInt(6 bits)
  when(lodWithBias < lodMinInt) {
    lodClamped := lodMinInt
  }.elsewhen(lodWithBias > lodMaxInt) {
    lodClamped := lodMaxInt
  }.otherwise {
    lodClamped := lodWithBias
  }

  val lodLevel = UInt(4 bits)
  when(lodClamped < 0) {
    lodLevel := 0
  }.elsewhen(lodClamped > 8) {
    lodLevel := 8
  }.otherwise {
    lodLevel := lodClamped.asUInt.resize(4 bits)
  }

  // Coordinate Wrapping/Clamping
  val sInt = sCorrected.floor(0).asSInt
  val tInt = tCorrected.floor(0).asSInt

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
