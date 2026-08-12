package voodoo.pixel

import voodoo._
import voodoo.xls.{XlsAlphaBlend, XlsDepthCompare}
import spinal.core._
import spinal.core.sim._
import spinal.lib._

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
    val fbReadColorReq = master Stream (FramebufferPlaneBuffer.ReadReq(c))
    val fbReadColorRsp = slave Stream (FramebufferPlaneBuffer.ReadRsp())
    val fbReadAuxReq = master Stream (FramebufferPlaneBuffer.ReadReq(c))
    val fbReadAuxRsp = slave Stream (FramebufferPlaneBuffer.ReadRsp())

    // Pipeline busy: pixels in flight inside fork-queue-join
    val busy = out Bool ()
    val zFuncFail = out Bool ()
    val debug = out Bits (32 bits)
  }
  io.input.valid.simPublic()
  io.input.ready.simPublic()
  io.output.valid.simPublic()
  io.output.ready.simPublic()
  io.fbReadColorReq.valid.simPublic()
  io.fbReadColorReq.ready.simPublic()
  io.fbReadColorReq.address.simPublic()
  io.fbReadAuxReq.valid.simPublic()
  io.fbReadAuxReq.ready.simPublic()
  io.fbReadAuxReq.address.simPublic()

  val inFlightCount = Reg(UInt(5 bits)) init 0
  inFlightCount.simPublic()

  val payload = io.input.payload
  val fbzMode = payload.fbzMode

  // ========================================================================
  // Step 1: Compute newDepth (combinational, before fork)
  // ========================================================================

  // Depth source selection
  val zaDepth = payload.zaColor(15 downto 0).asUInt

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
  val zaColorSigned = payload.zaColor(15 downto 0).asSInt
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
  // Step 2: Framebuffer fetch adapter
  // ========================================================================

  case class Passthrough() extends Bundle {
    val coords = PixelCoords(c)
    val color = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val alpha = UInt(8 bits)
    val colorBeforeFog = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val chromaKey = Bits(32 bits)
    val newDepth = UInt(16 bits)
    val alphaMode = AlphaMode()
    val fbzMode = FbzMode()
    val routing = FbRouting(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null

    def fbColorBaseAddr: UInt = routing.colorBaseAddr
    def fbAuxBaseAddr: UInt = routing.auxBaseAddr
    def fbPixelStride: UInt = routing.pixelStride
  }

  // Create internal stream with computed address and passthrough data
  case class Internal() extends Bundle {
    val colorAddress = UInt(c.addressWidth.value bits)
    val auxAddress = UInt(c.addressWidth.value bits)
    val needColorRead = Bool()
    val needAuxRead = Bool()
    val passthrough = Passthrough()
  }

  val request = Internal()
  val pixelX = payload.coords(0).asUInt
  val pixelY = payload.coords(1).asUInt
  val colorPlaneAddress =
    FramebufferAddressMath.planeAddress(
      payload.drawColorBufferBase,
      pixelX,
      pixelY,
      payload.fbPixelStride
    )
  val auxPlaneAddress =
    FramebufferAddressMath.planeAddress(
      payload.drawAuxBufferBase,
      pixelX,
      pixelY,
      payload.fbPixelStride
    )
  request.colorAddress := (colorPlaneAddress(c.addressWidth.value - 1 downto 1) ## U"1'b0").asUInt
  request.auxAddress := (auxPlaneAddress(c.addressWidth.value - 1 downto 1) ## U"1'b0").asUInt
  request.needColorRead := payload.alphaMode.alphaBlendEnable || fbzMode.enableDitherSubtract
  request.needAuxRead := fbzMode.enableDepthBuffer
  request.passthrough.coords := payload.coords
  request.passthrough.color := payload.color
  request.passthrough.alpha := payload.alpha
  request.passthrough.colorBeforeFog := payload.colorBeforeFog
  request.passthrough.chromaKey := payload.chromaKey
  request.passthrough.newDepth := compDepth
  request.passthrough.alphaMode := payload.alphaMode
  request.passthrough.fbzMode := fbzMode
  request.passthrough.routing := payload.routing
  if (c.trace.enabled) {
    request.passthrough.trace := payload.trace
  }

  case class FetchResult() extends Bundle {
    val colorData = Bits(16 bits)
    val auxData = Bits(16 bits)
    val passthrough = Passthrough()
  }

  case class BlendPrep() extends Bundle {
    val passthrough = Passthrough()
    val blendDestColor = Color.u8()
  }

  val requestStream = io.input.translateWith(request)
  requestStream.valid.simPublic()
  requestStream.ready.simPublic()

  val forked = StreamFork(requestStream, portCount = 5, synchronous = false)
  val colorReqBase = forked(0)
  val auxReqBase = forked(1)
  val colorCtlBase = forked(2).queue(32)
  val auxCtlBase = forked(3).queue(32)
  val passMeta = forked(4).queue(32)
  colorCtlBase.valid.simPublic()
  colorCtlBase.ready.simPublic()
  passMeta.valid.simPublic()
  passMeta.ready.simPublic()

  io.fbReadColorReq << colorReqBase
    .throwWhen(!colorReqBase.payload.needColorRead)
    .translateWith {
      val req = FramebufferPlaneBuffer.ReadReq(c)
      req.address := colorReqBase.payload.colorAddress
      req
    }

  io.fbReadAuxReq << auxReqBase
    .throwWhen(!auxReqBase.payload.needAuxRead)
    .translateWith {
      val req = FramebufferPlaneBuffer.ReadReq(c)
      req.address := auxReqBase.payload.auxAddress
      req
    }

  val colorRspBuf = io.fbReadColorRsp.queue(4)
  val auxRspBuf = io.fbReadAuxRsp.queue(4)

  val colorCtlRoutes = StreamDemux(colorCtlBase, colorCtlBase.payload.needColorRead.asUInt, 2)
  val colorFetched = StreamJoin(colorCtlRoutes(1).queue(32), colorRspBuf).translateWith {
    val rsp = FramebufferPlaneBuffer.ReadRsp()
    rsp.data := colorRspBuf.payload.data
    rsp
  }
  val colorBypass = colorCtlRoutes(0).translateWith {
    val rsp = FramebufferPlaneBuffer.ReadRsp()
    rsp.data := 0
    rsp
  }
  val colorRsp = Stream(FramebufferPlaneBuffer.ReadRsp())
  colorRsp.valid := colorFetched.valid || colorBypass.valid
  colorRsp.payload := colorBypass.payload
  when(colorFetched.valid) {
    colorRsp.payload := colorFetched.payload
  }
  colorFetched.ready := colorRsp.ready
  colorBypass.ready := colorRsp.ready && !colorFetched.valid

  val auxCtlRoutes = StreamDemux(auxCtlBase, auxCtlBase.payload.needAuxRead.asUInt, 2)
  val auxFetched = StreamJoin(auxCtlRoutes(1).queue(32), auxRspBuf).translateWith {
    val rsp = FramebufferPlaneBuffer.ReadRsp()
    rsp.data := auxRspBuf.payload.data
    rsp
  }
  val auxBypass = auxCtlRoutes(0).translateWith {
    val rsp = FramebufferPlaneBuffer.ReadRsp()
    rsp.data := 0
    rsp
  }
  val auxRsp = Stream(FramebufferPlaneBuffer.ReadRsp())
  auxRsp.valid := auxFetched.valid || auxBypass.valid
  auxRsp.payload := auxBypass.payload
  when(auxFetched.valid) {
    auxRsp.payload := auxFetched.payload
  }
  auxFetched.ready := auxRsp.ready
  auxBypass.ready := auxRsp.ready && !auxFetched.valid

  val fetchedRaw = StreamJoin(StreamJoin(colorRsp, auxRsp), passMeta).translateWith {
    val out = FetchResult()
    out.colorData := colorRsp.payload.data
    out.auxData := auxRsp.payload.data
    out.passthrough := passMeta.payload.passthrough
    out
  }

  val fetched = fetchedRaw.queue(32)

  val exitFire = fetched.fire
  when(io.input.fire && !exitFire) {
    inFlightCount := inFlightCount + 1
  }.elsewhen(!io.input.fire && exitFire) {
    inFlightCount := inFlightCount - 1
  }
  io.busy := inFlightCount =/= 0

  // ========================================================================
  // Step 3: Depth test (per-pixel fbzMode)
  // ========================================================================

  val colorData = fetched.payload.colorData
  val auxData = fetched.payload.auxData
  val pdata = fetched.payload.passthrough
  val oldDepth = auxData.asUInt

  val depthCompare = new XlsDepthCompare
  depthCompare.io.comparison := pdata.fbzMode.depthFunction
  depthCompare.io.new_depth := pdata.newDepth
  depthCompare.io.old_depth := oldDepth
  val depthPassed = depthCompare.io.out

  val depthKill = pdata.fbzMode.enableDepthBuffer && !depthPassed
  io.zFuncFail := fetched.fire && depthKill
  val afterDepthTest = fetched.throwWhen(depthKill)
  val afterDepthTestPiped = afterDepthTest.m2sPipe()

  // ========================================================================
  // Step 4: Alpha blend
  // ========================================================================

  val afterColorData = afterDepthTestPiped.payload.colorData
  val afterPdata = afterDepthTestPiped.payload.passthrough

  val oldColor565 = afterColorData.asUInt
  val destColor = expandRgb565(oldColor565)
  val dsubRbRom = Vec(DitherTables.dsubRbPacked.map(v => U(v, 8 bits)))
  val dsubGRom = Vec(DitherTables.dsubGPacked.map(v => U(v, 8 bits)))

  def ditherAddr(v: UInt, use2x2: Bool, x: UInt, y: UInt): UInt = {
    val addr4x4 = (v ## y ## x).asUInt
    val addr2x2 = (v ## y(0) ## x(0)).asUInt
    use2x2 ? (True ## addr2x2.resize(12 bits)).asUInt | addr4x4.resize(13 bits)
  }

  val destDsub = Color.u8()
  val dsubX = afterPdata.coords(0)(1 downto 0).asUInt
  val dsubY = afterPdata.coords(1)(1 downto 0).asUInt
  destDsub.r := dsubRbRom(
    ditherAddr(destColor.r, afterPdata.fbzMode.ditherAlgorithm, dsubX, dsubY)
  )
  destDsub.g := dsubGRom(
    ditherAddr(destColor.g, afterPdata.fbzMode.ditherAlgorithm, dsubX, dsubY)
  )
  destDsub.b := dsubRbRom(
    ditherAddr(destColor.b, afterPdata.fbzMode.ditherAlgorithm, dsubX, dsubY)
  )
  val blendDestColor = afterPdata.fbzMode.enableDitherSubtract ? destDsub | destColor
  val blendPrep = afterDepthTestPiped
    .translateWith {
      val prep = BlendPrep()
      prep.passthrough := afterPdata
      prep.blendDestColor := blendDestColor
      prep
    }
    .m2sPipe()

  io.debug := 0
  io.debug(0) := io.input.valid
  io.debug(1) := io.input.ready
  io.debug(2) := io.output.valid
  io.debug(3) := io.output.ready
  io.debug(4) := io.busy
  io.debug(5) := io.fbReadColorReq.valid
  io.debug(6) := io.fbReadColorReq.ready
  io.debug(7) := io.fbReadColorRsp.valid
  io.debug(8) := io.fbReadColorRsp.ready
  io.debug(9) := io.fbReadAuxReq.valid
  io.debug(10) := io.fbReadAuxReq.ready
  io.debug(11) := io.fbReadAuxRsp.valid
  io.debug(12) := io.fbReadAuxRsp.ready
  io.debug(13) := requestStream.valid
  io.debug(14) := requestStream.ready
  io.debug(15) := colorCtlBase.valid
  io.debug(16) := colorCtlBase.ready
  io.debug(17) := auxCtlBase.valid
  io.debug(18) := auxCtlBase.ready
  io.debug(19) := passMeta.valid
  io.debug(20) := passMeta.ready
  io.debug(21) := colorFetched.valid
  io.debug(22) := colorBypass.valid
  io.debug(23) := auxFetched.valid
  io.debug(24) := auxBypass.valid
  io.debug(25) := fetched.valid
  io.debug(26) := fetched.ready
  io.debug(31 downto 27) := inFlightCount.asBits

  val blendPdata = blendPrep.payload.passthrough
  val alphaBlend = new XlsAlphaBlend
  alphaBlend.io.src_rgb := (blendPdata.color.r ## blendPdata.color.g ## blendPdata.color.b).asBits
  alphaBlend.io.dest_rgb := (blendPrep.payload.blendDestColor.r ## blendPrep.payload.blendDestColor.g ## blendPrep.payload.blendDestColor.b).asBits
  alphaBlend.io.before_fog_rgb := (blendPdata.colorBeforeFog.r ## blendPdata.colorBeforeFog.g ## blendPdata.colorBeforeFog.b).asBits
  alphaBlend.io.src_alpha := blendPdata.alpha
  alphaBlend.io.src_selector := blendPdata.alphaMode.rgbSrcFact
  alphaBlend.io.dest_selector := blendPdata.alphaMode.rgbDstFact
  alphaBlend.io.enable := blendPdata.alphaMode.alphaBlendEnable

  // ========================================================================
  // Step 5: Output
  // ========================================================================

  io.output.translateFrom(blendPrep) { (out, in) =>
    val pd = in.passthrough
    out.coords := pd.coords
    out.color.r := alphaBlend.io.out(23 downto 16).asUInt
    out.color.g := alphaBlend.io.out(15 downto 8).asUInt
    out.color.b := alphaBlend.io.out(7 downto 0).asUInt
    out.alpha := pd.alpha
    out.colorBeforeFog := pd.colorBeforeFog
    out.newDepth := pd.newDepth
    out.alphaMode := pd.alphaMode
    out.fbzMode := pd.fbzMode
    out.chromaKey := pd.chromaKey
    out.rgbWrite := pd.fbzMode.rgbBufferMask
    out.auxWrite := pd.fbzMode.auxBufferMask
    out.enableAlphaPlanes := pd.fbzMode.enableAlphaPlanes
    out.enableDithering := pd.fbzMode.enableDithering
    out.ditherAlgorithm := pd.fbzMode.ditherAlgorithm
    out.routing := pd.routing
    if (c.trace.enabled) {
      out.trace := pd.trace
    }
  }
}

object FramebufferAccess {
  case class Output(c: Config) extends Bundle {
    val coords = PixelCoords(c)
    val color = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val alpha = UInt(8 bits)
    val colorBeforeFog = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val newDepth = UInt(16 bits)
    val alphaMode = AlphaMode()
    val fbzMode = FbzMode()
    val chromaKey = Bits(32 bits)
    val rgbWrite = Bool()
    val auxWrite = Bool()
    val enableAlphaPlanes = Bool()
    val enableDithering = Bool()
    val ditherAlgorithm = Bool()
    val routing = FbRouting(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null

    def fbBaseAddr: UInt = routing.colorBaseAddr
    def auxBaseAddr: UInt = routing.auxBaseAddr
    def fbPixelStride: UInt = routing.pixelStride
  }
}
