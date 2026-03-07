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
    val fbReadColor = master(Bmb(FramebufferAccess.bmbParams(c)))
    val fbReadAux = master(Bmb(FramebufferAccess.bmbParams(c)))

    // Other registers (not yet per-triangle)
    val zaColor = in Bits (32 bits)
    val fbColorBaseAddr = in UInt (c.addressWidth)
    val fbAuxBaseAddr = in UInt (c.addressWidth)
    val fbPixelStride = in UInt (11 bits)

    // Pipeline busy: pixels in flight inside fork-queue-join
    val busy = out Bool ()
  }

  // Track in-flight pixels (max 4 in queue)
  // Pixels exit via joined.fire (covers both depth-test pass and discard).
  // The exitFire signal is assigned later after the joined stream is defined.
  val inFlightCount = Reg(UInt(3 bits)) init 0
  val exitFire = Bool()
  when(io.input.fire && !exitFire) {
    inFlightCount := inFlightCount + 1
  }.elsewhen(!io.input.fire && exitFire) {
    inFlightCount := inFlightCount - 1
  }
  io.busy := inFlightCount =/= 0

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

  // Select base depth: Z or W buffer
  val baseDepth = fbzMode.wBufferSelect ? payload.wDepth | zDepth

  // Depth source: normal pipeline or zaColor register
  val compDepthPreBias = fbzMode.depthSourceSelect ? zaDepth | baseDepth

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

  val compDepth = fbzMode.enableDepthBias ? biasedDepth | compDepthPreBias

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
    val alphaMode = AlphaMode()
    val fbzMode = FbzMode()
    val colorLaneHi = Bool()
    val auxLaneHi = Bool()
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  // Create internal stream with computed address and passthrough data
  case class Internal() extends Bundle {
    val colorAddress = UInt(c.addressWidth.value bits)
    val auxAddress = UInt(c.addressWidth.value bits)
    val passthrough = Passthrough()
  }

  val internalStream = io.input.translateWith {
    val data = Internal()
    val strideSInt = (False ## io.fbPixelStride).asSInt
    val pixelFlat = (payload.coords(1) * strideSInt + payload.coords(0)).asUInt
    val colorPlaneAddress = (io.fbColorBaseAddr + (pixelFlat << 1)).resized
    val auxPlaneAddress = (io.fbAuxBaseAddr + (pixelFlat << 1)).resized
    data.colorAddress := (colorPlaneAddress(c.addressWidth.value - 1 downto 2) ## U"2'b00").asUInt
    data.auxAddress := (auxPlaneAddress(c.addressWidth.value - 1 downto 2) ## U"2'b00").asUInt
    data.passthrough.coords := payload.coords
    data.passthrough.color := payload.color
    data.passthrough.alpha := payload.alpha
    data.passthrough.colorBeforeFog := payload.colorBeforeFog
    data.passthrough.newDepth := compDepth
    data.passthrough.alphaMode := payload.alphaMode
    data.passthrough.fbzMode := fbzMode
    data.passthrough.colorLaneHi := colorPlaneAddress(1)
    data.passthrough.auxLaneHi := auxPlaneAddress(1)
    if (c.trace.enabled) {
      data.passthrough.trace := payload.trace
    }
    data
  }

  // Fork into two streams
  val (toMemory, toQueue) = StreamFork2(internalStream)

  // ========================================================================
  // Step 2a: Memory request path (color + aux reads)
  // ========================================================================

  val (toColorCmd, toAuxCmd) = StreamFork2(toMemory)

  val colorCmdStream = toColorCmd.translateWith {
    val cmd = Fragment(BmbCmd(FramebufferAccess.bmbParams(c)))
    cmd.fragment.address := toColorCmd.payload.colorAddress
    cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
    cmd.fragment.length := 3 // 4 bytes - 1
    cmd.fragment.source := 0
    cmd.last := True
    cmd
  }

  val auxCmdStream = toAuxCmd.translateWith {
    val cmd = Fragment(BmbCmd(FramebufferAccess.bmbParams(c)))
    cmd.fragment.address := toAuxCmd.payload.auxAddress
    cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
    cmd.fragment.length := 3 // 4 bytes - 1
    cmd.fragment.source := 0
    cmd.last := True
    cmd
  }

  io.fbReadColor.cmd << colorCmdStream
  io.fbReadAux.cmd << auxCmdStream

  // ========================================================================
  // Step 2b: Passthrough queue
  // ========================================================================

  val queuedPassthrough = toQueue.map(_.passthrough).queue(4)

  // ========================================================================
  // Step 2c: Join memory response with queued data
  // ========================================================================

  val colorRspStream = io.fbReadColor.rsp.takeWhen(io.fbReadColor.rsp.last).translateWith {
    io.fbReadColor.rsp.fragment.data
  }
  val auxRspStream = io.fbReadAux.rsp.takeWhen(io.fbReadAux.rsp.last).translateWith {
    io.fbReadAux.rsp.fragment.data
  }
  val joinedRsp = StreamJoin(colorRspStream, auxRspStream)
  val joined = StreamJoin(joinedRsp, queuedPassthrough)
  exitFire := joined.fire // Pixel exits fork-queue-join (pass or discard)

  // ========================================================================
  // Step 3: Depth test (per-pixel fbzMode)
  // ========================================================================

  val colorData = joined.payload._1._1
  val auxData = joined.payload._1._2
  val pdata = joined.payload._2
  val oldDepth =
    pdata.auxLaneHi ? auxData(31 downto 16).asUInt | auxData(15 downto 0).asUInt

  val depthPassed = pdata.fbzMode.depthFunction.mux(
    U(0) -> False,
    U(1) -> (pdata.newDepth < oldDepth),
    U(2) -> (pdata.newDepth === oldDepth),
    U(3) -> (pdata.newDepth <= oldDepth),
    U(4) -> (pdata.newDepth > oldDepth),
    U(5) -> (pdata.newDepth =/= oldDepth),
    U(6) -> (pdata.newDepth >= oldDepth),
    U(7) -> True
  )

  val depthKill = pdata.fbzMode.enableDepthBuffer && !depthPassed
  val afterDepthTest = joined.throwWhen(depthKill)

  // ========================================================================
  // Step 4: Alpha blend
  // ========================================================================

  val afterColorData = afterDepthTest.payload._1._1
  val afterPdata = afterDepthTest.payload._2

  val oldColor565 =
    afterPdata.colorLaneHi ? afterColorData(31 downto 16).asUInt | afterColorData(
      15 downto 0
    ).asUInt
  val destColor = expandRgb565(oldColor565)
  val destA = U(0xff, 8 bits)

  val srcColor = afterPdata.color
  val srcA = afterPdata.alpha
  val colorBeforeFog = afterPdata.colorBeforeFog

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

  // Blend factor computation: computes factored color for one side of the blend equation
  def computeBlendFactor(
      factorSel: UInt,
      selfColor: Color[UInt],
      srcAlpha: UInt,
      destAlpha: UInt,
      colorFactor: Color[UInt],
      isSrc: Boolean,
      colBeforeFog: Color[UInt]
  ): Color[UInt] = {
    val result = Color.u8()
    switch(factorSel) {
      is(0x0) { result.foreach(_ := U(0, 8 bits)) } // ZERO
      is(0x1) { result.assignFromSeq(selfColor.map(blendMul(srcAlpha, _))) } // SRC_ALPHA
      is(0x2) { // COLOR
        result.assignFromSeq(selfColor.zipWith(colorFactor)((s, f) => blendMul(f, s)))
      }
      is(0x3) { result.assignFromSeq(selfColor.map(blendMul(destAlpha, _))) } // DST_ALPHA
      is(0x4) { result := selfColor } // ONE
      is(0x5) { // ONE_MINUS_SRC_ALPHA
        result.assignFromSeq(selfColor.map(blendMul(oneMinusU8(srcAlpha), _)))
      }
      is(0x6) { // ONE_MINUS_COLOR
        result.assignFromSeq(selfColor.zipWith(colorFactor)((s, f) => blendMul(oneMinusU8(f), s)))
      }
      is(0x7) { // ONE_MINUS_DST_ALPHA
        result.assignFromSeq(selfColor.map(blendMul(oneMinusU8(destAlpha), _)))
      }
      is(0xf) {
        if (isSrc) { // SATURATE: min(srcA, 255-destA)
          val satFactor = srcAlpha.min(oneMinusU8(destAlpha))
          result.assignFromSeq(selfColor.map(blendMul(satFactor, _)))
        } else { // ACOLORBEFOREFOG
          result.assignFromSeq(selfColor.zipWith(colBeforeFog)((s, f) => blendMul(f, s)))
        }
      }
      default {
        if (isSrc) result := selfColor // ONE
        else result.foreach(_ := U(0, 8 bits)) // ZERO
      }
    }
    result
  }

  val srcFactor = computeBlendFactor(
    afterPdata.alphaMode.rgbSrcFact,
    srcColor,
    srcA,
    destA,
    destColor,
    isSrc = true,
    colorBeforeFog
  )
  val dstFactor = computeBlendFactor(
    afterPdata.alphaMode.rgbDstFact,
    destColor,
    srcA,
    destA,
    srcColor,
    isSrc = false,
    colorBeforeFog
  )

  // Sum and clamp to [0, 255]
  def clampAdd(a: UInt, b: UInt): UInt = {
    val sum = (a.resize(9 bits) + b.resize(9 bits))
    sum.min(U(255, 9 bits)).resize(8 bits)
  }

  val blended = Color.u8()
  blended.assignFromSeq(srcFactor.zipWith(dstFactor)(clampAdd))

  // ========================================================================
  // Step 5: Output
  // ========================================================================

  io.output.translateFrom(afterDepthTest) { (out, in) =>
    val pd = in._2
    out.coords := pd.coords
    when(pd.alphaMode.alphaBlendEnable) {
      out.color := blended
    }.otherwise {
      out.color := pd.color
    }
    out.alpha := pd.alpha
    out.newDepth := pd.newDepth
    out.rgbWrite := pd.fbzMode.rgbBufferMask
    out.auxWrite := pd.fbzMode.auxBufferMask
    out.enableAlphaPlanes := pd.fbzMode.enableAlphaPlanes
    out.enableDithering := pd.fbzMode.enableDithering
    out.ditherAlgorithm := pd.fbzMode.ditherAlgorithm
    if (c.trace.enabled) {
      out.trace := pd.trace
    }
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
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
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
