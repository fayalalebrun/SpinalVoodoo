package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._

/** Framebuffer Access stage — reads existing FB contents for depth test and alpha blend.
  *
  * Pipeline position: after AlphaTest, before Dither/Write. Uses fork-queue-join pattern (same as
  * TMU) to overlap FB reads with pixel processing.
  *
  * Flow:
  *   1. Compute newDepth (combinational, before fork)
  *   2. Fork: one path issues Bmb read, other path queues pixel data
  *   3. Join: pairs Bmb response with queued pixel data
  *   4. Depth test: compare newDepth against old depth from FB (discard on fail)
  *   5. Alpha blend: blend source color with old color from FB
  *   6. Output: blended color + newDepth + write masks
  */
case class FramebufferAccess(c: Config) extends Component {
  val io = new Bundle {
    val input = slave Stream (Fog.Output(c))
    val output = master Stream (FramebufferAccess.Output(c))
    val fbRead = master(Bmb(FramebufferAccess.bmbParams(c)))

    // Other registers (not yet per-triangle)
    val zaColor = in Bits (32 bits)
    val fbBaseAddr = in UInt (c.addressWidth)
  }

  // ========================================================================
  // fbzMode bit extraction helpers (bits [20:0] of fbzMode register)
  // ========================================================================
  // Bit layout: [0]=enableClipping [1]=enableChromaKey [2]=enableStipple
  //   [3]=wBufferSelect [4]=enableDepthBuffer [7:5]=depthFunction
  //   [8]=enableDithering [9]=rgbBufferMask [10]=auxBufferMask
  //   [11]=ditherAlgorithm [12]=enableStipplePattern [13]=enableAlphaMask
  //   [15:14]=drawBuffer [16]=enableDepthBias [17]=yOrigin
  //   [18]=enableAlphaPlanes [19]=enableDitherSubtract [20]=depthSourceSelect

  val payload = io.input.payload
  val fbzMode = payload.fbzMode

  // ========================================================================
  // Step 1: Compute newDepth (combinational, before fork)
  // ========================================================================

  // Depth source selection
  val zaDepth = io.zaColor(15 downto 0).asUInt

  // Z-buffer depth: clamp SQ(32,12) >> 12 to 16 bits
  val depthRaw = payload.depth.raw.asSInt
  val zDepthShifted = (depthRaw >> 12).resize(20 bits)
  val zDepth = UInt(16 bits)
  when(zDepthShifted < 0) {
    zDepth := 0
  }.elsewhen(zDepthShifted > 0xffff) {
    zDepth := U(0xffff, 16 bits)
  }.otherwise {
    zDepth := zDepthShifted.asUInt.resize(16 bits)
  }

  // Select base depth: Z or W buffer (per-pixel fbzMode bit 3)
  val baseDepth = fbzMode(3) ? payload.wDepth | zDepth

  // Depth source: normal pipeline or zaColor register (per-pixel fbzMode bit 20)
  val compDepthPreBias = fbzMode(20) ? zaDepth | baseDepth

  // Depth bias: signed add of zaColor[15:0]
  val zaColorSigned = io.zaColor(15 downto 0).asSInt
  val biasedDepthRaw = (compDepthPreBias.resize(17 bits).asSInt + zaColorSigned.resize(17 bits))
  val biasedDepth = UInt(16 bits)
  when(biasedDepthRaw < 0) {
    biasedDepth := 0
  }.elsewhen(biasedDepthRaw > 0xffff) {
    biasedDepth := U(0xffff, 16 bits)
  }.otherwise {
    biasedDepth := biasedDepthRaw.asUInt.resize(16 bits)
  }

  // Per-pixel fbzMode bit 16: enableDepthBias
  val compDepth = fbzMode(16) ? biasedDepth | compDepthPreBias

  // ========================================================================
  // Step 2: Fork-Queue-Join
  // ========================================================================

  // Bundle to pass through the queue
  case class Passthrough() extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val color = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val alpha = UInt(8 bits)
    val colorBeforeFog = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val newDepth = UInt(16 bits)
    // Per-pixel alpha blend config (from per-triangle captured alphaMode)
    val alphaBlendEnable = Bool()
    val srcBlendFunc = UInt(4 bits)
    val dstBlendFunc = UInt(4 bits)
    // Per-pixel fbzMode fields (from per-triangle captured fbzMode)
    val enableDepthBuffer = Bool()
    val depthFunction = UInt(3 bits)
    val rgbBufferMask = Bool()
    val auxBufferMask = Bool()
    val enableAlphaPlanes = Bool()
    val enableDithering = Bool()
    val ditherAlgorithm = Bool()
  }

  // Create internal stream with computed address and passthrough data
  case class Internal() extends Bundle {
    val address = UInt(c.addressWidth.value bits)
    val passthrough = Passthrough()
  }

  val internalStream = io.input.translateWith {
    val data = Internal()
    val pixelFlat = (payload.coords(1) * c.fbPixelStride + payload.coords(0)).asUInt
    data.address := (io.fbBaseAddr + (pixelFlat << 2)).resized
    data.passthrough.coords := payload.coords
    data.passthrough.color := payload.color
    data.passthrough.alpha := payload.alpha
    data.passthrough.colorBeforeFog := payload.colorBeforeFog
    data.passthrough.newDepth := compDepth
    // Per-pixel alpha blend config from per-triangle captured alphaMode
    data.passthrough.alphaBlendEnable := payload.alphaMode(4)
    data.passthrough.srcBlendFunc := payload.alphaMode(11 downto 8).asUInt
    data.passthrough.dstBlendFunc := payload.alphaMode(15 downto 12).asUInt
    // Per-pixel fbzMode fields from per-triangle captured fbzMode
    data.passthrough.enableDepthBuffer := fbzMode(4)
    data.passthrough.depthFunction := fbzMode(7 downto 5).asUInt
    data.passthrough.rgbBufferMask := fbzMode(9)
    data.passthrough.auxBufferMask := fbzMode(10)
    data.passthrough.enableAlphaPlanes := fbzMode(18)
    data.passthrough.enableDithering := fbzMode(8)
    data.passthrough.ditherAlgorithm := fbzMode(11)
    data
  }

  // Fork into two streams
  val (toMemory, toQueue) = StreamFork2(internalStream)

  // ========================================================================
  // Step 2a: Memory request path
  // ========================================================================

  val cmdStream = toMemory.translateWith {
    val cmd = Fragment(BmbCmd(FramebufferAccess.bmbParams(c)))
    cmd.fragment.address := toMemory.payload.address
    cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
    cmd.fragment.length := 3 // 4 bytes - 1
    cmd.fragment.source := 0
    cmd.last := True
    cmd
  }

  io.fbRead.cmd << cmdStream

  // ========================================================================
  // Step 2b: Passthrough queue
  // ========================================================================

  val queuedPassthrough = toQueue.map(_.passthrough).queue(4)

  // ========================================================================
  // Step 2c: Join memory response with queued data
  // ========================================================================

  val rspStream = io.fbRead.rsp.takeWhen(io.fbRead.rsp.last).translateWith {
    io.fbRead.rsp.fragment.data
  }

  val joined = StreamJoin(rspStream, queuedPassthrough)

  // ========================================================================
  // Step 3: Depth test (per-pixel fbzMode)
  // ========================================================================

  val fbData = joined.payload._1
  val pdata = joined.payload._2
  val oldDepth = fbData(31 downto 16).asUInt

  val depthPassed = pdata.depthFunction.mux(
    U(0) -> False,
    U(1) -> (pdata.newDepth < oldDepth),
    U(2) -> (pdata.newDepth === oldDepth),
    U(3) -> (pdata.newDepth <= oldDepth),
    U(4) -> (pdata.newDepth > oldDepth),
    U(5) -> (pdata.newDepth =/= oldDepth),
    U(6) -> (pdata.newDepth >= oldDepth),
    U(7) -> True
  )

  val depthKill = pdata.enableDepthBuffer && !depthPassed
  val afterDepthTest = joined.throwWhen(depthKill)

  // ========================================================================
  // Step 4: Alpha blend
  // ========================================================================

  val afterData = afterDepthTest.payload._1
  val afterPdata = afterDepthTest.payload._2

  // RGB565 to 8-bit expansion matching 86Box rgb565toRGB8:
  //   r = (rgb >> 8) & 0xf8; r |= (r >> 5)
  //   g = (rgb >> 3) & 0xfc; g |= (g >> 6)
  //   b = (rgb << 3) & 0xf8; b |= (b >> 5)
  val oldRgb565 = afterData(15 downto 0).asUInt
  val rBase = (oldRgb565 >> 8).resize(8 bits) & 0xf8
  val gBase = (oldRgb565 >> 3).resize(8 bits) & 0xfc
  val bBase = (oldRgb565 << 3).resize(8 bits) & 0xf8
  val destR = rBase | (rBase >> 5).resize(8 bits)
  val destG = gBase | (gBase >> 6).resize(8 bits)
  val destB = bBase | (bBase >> 5).resize(8 bits)
  val destA = U(0xff, 8 bits)

  // Source color
  val srcR = afterPdata.color.r
  val srcG = afterPdata.color.g
  val srcB = afterPdata.color.b
  val srcA = afterPdata.alpha

  // Color before fog (for ACOLORBEFOREFOG blend mode)
  val colbfogR = afterPdata.colorBeforeFog.r
  val colbfogG = afterPdata.colorBeforeFog.g
  val colbfogB = afterPdata.colorBeforeFog.b

  // Integer division by 255: (v + 1 + (v >> 8)) >> 8 gives exact a*b/255 for 0 <= a*b <= 65025
  def divBy255(v: UInt): UInt = {
    val wide = v.resize(17 bits)
    ((wide + 1 + (wide >> 8)) >> 8).resize(8 bits)
  }

  // Blend factor computation: factor * color / 255
  def blendMul(factor: UInt, color: UInt): UInt = {
    divBy255(factor.resize(16 bits) * color.resize(16 bits))
  }

  // 255 - x
  def oneMinusU8(x: UInt): UInt = (U(255, 8 bits) - x)

  // Source blend result: src * srcFactor
  val srcFactorR = UInt(8 bits)
  val srcFactorG = UInt(8 bits)
  val srcFactorB = UInt(8 bits)
  switch(afterPdata.srcBlendFunc) {
    is(0x0) { // ZERO
      srcFactorR := 0; srcFactorG := 0; srcFactorB := 0
    }
    is(0x1) { // SRC_ALPHA
      srcFactorR := blendMul(srcA, srcR); srcFactorG := blendMul(srcA, srcG);
      srcFactorB := blendMul(srcA, srcB)
    }
    is(0x2) { // COLOR (use dst color as factor for src)
      srcFactorR := blendMul(destR, srcR); srcFactorG := blendMul(destG, srcG);
      srcFactorB := blendMul(destB, srcB)
    }
    is(0x3) { // DST_ALPHA
      srcFactorR := blendMul(destA, srcR); srcFactorG := blendMul(destA, srcG);
      srcFactorB := blendMul(destA, srcB)
    }
    is(0x4) { // ONE
      srcFactorR := srcR; srcFactorG := srcG; srcFactorB := srcB
    }
    is(0x5) { // ONE_MINUS_SRC_ALPHA
      srcFactorR := blendMul(oneMinusU8(srcA), srcR);
      srcFactorG := blendMul(oneMinusU8(srcA), srcG); srcFactorB := blendMul(oneMinusU8(srcA), srcB)
    }
    is(0x6) { // ONE_MINUS_COLOR
      srcFactorR := blendMul(oneMinusU8(destR), srcR);
      srcFactorG := blendMul(oneMinusU8(destG), srcG);
      srcFactorB := blendMul(oneMinusU8(destB), srcB)
    }
    is(0x7) { // ONE_MINUS_DST_ALPHA
      srcFactorR := blendMul(oneMinusU8(destA), srcR);
      srcFactorG := blendMul(oneMinusU8(destA), srcG);
      srcFactorB := blendMul(oneMinusU8(destA), srcB)
    }
    is(0xf) { // SATURATE: min(srcA, 255-destA)
      val satFactor = srcA.min(oneMinusU8(destA))
      srcFactorR := blendMul(satFactor, srcR); srcFactorG := blendMul(satFactor, srcG);
      srcFactorB := blendMul(satFactor, srcB)
    }
    default {
      srcFactorR := srcR; srcFactorG := srcG; srcFactorB := srcB
    }
  }

  // Destination blend result: dst * dstFactor
  val dstFactorR = UInt(8 bits)
  val dstFactorG = UInt(8 bits)
  val dstFactorB = UInt(8 bits)
  switch(afterPdata.dstBlendFunc) {
    is(0x0) { // ZERO
      dstFactorR := 0; dstFactorG := 0; dstFactorB := 0
    }
    is(0x1) { // SRC_ALPHA
      dstFactorR := blendMul(srcA, destR); dstFactorG := blendMul(srcA, destG);
      dstFactorB := blendMul(srcA, destB)
    }
    is(0x2) { // COLOR (use src color as factor for dst)
      dstFactorR := blendMul(srcR, destR); dstFactorG := blendMul(srcG, destG);
      dstFactorB := blendMul(srcB, destB)
    }
    is(0x3) { // DST_ALPHA
      dstFactorR := blendMul(destA, destR); dstFactorG := blendMul(destA, destG);
      dstFactorB := blendMul(destA, destB)
    }
    is(0x4) { // ONE
      dstFactorR := destR; dstFactorG := destG; dstFactorB := destB
    }
    is(0x5) { // ONE_MINUS_SRC_ALPHA
      dstFactorR := blendMul(oneMinusU8(srcA), destR);
      dstFactorG := blendMul(oneMinusU8(srcA), destG);
      dstFactorB := blendMul(oneMinusU8(srcA), destB)
    }
    is(0x6) { // ONE_MINUS_COLOR
      dstFactorR := blendMul(oneMinusU8(srcR), destR);
      dstFactorG := blendMul(oneMinusU8(srcG), destG);
      dstFactorB := blendMul(oneMinusU8(srcB), destB)
    }
    is(0x7) { // ONE_MINUS_DST_ALPHA
      dstFactorR := blendMul(oneMinusU8(destA), destR);
      dstFactorG := blendMul(oneMinusU8(destA), destG);
      dstFactorB := blendMul(oneMinusU8(destA), destB)
    }
    is(0xf) { // ACOLORBEFOREFOG
      dstFactorR := blendMul(colbfogR, destR); dstFactorG := blendMul(colbfogG, destG);
      dstFactorB := blendMul(colbfogB, destB)
    }
    default {
      dstFactorR := 0; dstFactorG := 0; dstFactorB := 0
    }
  }

  // Sum and clamp to [0, 255]
  def clampAdd(a: UInt, b: UInt): UInt = {
    val sum = (a.resize(9 bits) + b.resize(9 bits))
    sum.min(U(255, 9 bits)).resize(8 bits)
  }

  val blendedR = clampAdd(srcFactorR, dstFactorR)
  val blendedG = clampAdd(srcFactorG, dstFactorG)
  val blendedB = clampAdd(srcFactorB, dstFactorB)

  // ========================================================================
  // Step 5: Output
  // ========================================================================

  io.output.translateFrom(afterDepthTest) { (out, in) =>
    val pd = in._2
    out.coords := pd.coords
    when(pd.alphaBlendEnable) {
      out.color.r := blendedR
      out.color.g := blendedG
      out.color.b := blendedB
    }.otherwise {
      out.color := pd.color
    }
    out.alpha := pd.alpha
    out.newDepth := pd.newDepth
    out.rgbWrite := pd.rgbBufferMask
    out.auxWrite := pd.auxBufferMask
    out.enableAlphaPlanes := pd.enableAlphaPlanes
    out.enableDithering := pd.enableDithering
    out.ditherAlgorithm := pd.ditherAlgorithm
  }
}

object FramebufferAccess {
  case class Output(c: Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val color = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val alpha = UInt(8 bits)
    val newDepth = UInt(16 bits)
    val rgbWrite = Bool()
    val auxWrite = Bool()
    val enableAlphaPlanes = Bool()
    val enableDithering = Bool()
    val ditherAlgorithm = Bool()
  }

  def bmbParams(c: Config) = BmbParameter(
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
