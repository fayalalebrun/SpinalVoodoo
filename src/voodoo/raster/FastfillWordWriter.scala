package voodoo.raster

import spinal.core._
import spinal.lib._
import voodoo._
import voodoo.framebuffer.FramebufferAddressMath
import voodoo.pixel.Dither

case class FastfillWordWriter(c: Config) extends Component {
  import FastfillWordWriter._

  val io = new Bundle {
    val cmd = slave Stream (NoData)
    val regs = in(FastfillWrite.Regs(c))
    val clipLeft = in UInt (10 bits)
    val clipRight = in UInt (10 bits)
    val clipLowY = in UInt (10 bits)
    val clipHighY = in UInt (10 bits)

    val colorWrite = master Stream (FramebufferPlaneBuffer.WriteReq(c))
    val auxWrite = master Stream (FramebufferPlaneBuffer.WriteReq(c))

    val running = out Bool ()
    val wordValid = out Bool ()
    val wordReady = out Bool ()
    val generatedPixels = out UInt (2 bits)
    val colorWrittenPixels = out UInt (2 bits)
  }

  val active = RegInit(False)
  val regsCapture = RegNextWhen(io.regs, io.cmd.fire)
  val regs = FastfillWrite.Regs(c)
  when(active) { regs := regsCapture }.otherwise { regs := io.regs }

  val clipLeftCapture = RegNextWhen(io.clipLeft, io.cmd.fire)
  val clipRightCapture = RegNextWhen(io.clipRight, io.cmd.fire)
  val clipLowYCapture = RegNextWhen(io.clipLowY, io.cmd.fire)
  val clipHighYCapture = RegNextWhen(io.clipHighY, io.cmd.fire)

  val clipLeft = UInt(10 bits)
  val clipRight = UInt(10 bits)
  val clipLowY = UInt(10 bits)
  val clipHighY = UInt(10 bits)
  when(active) {
    clipLeft := clipLeftCapture
    clipRight := clipRightCapture
    clipLowY := clipLowYCapture
    clipHighY := clipHighYCapture
  } otherwise {
    clipLeft := io.clipLeft
    clipRight := io.clipRight
    clipLowY := io.clipLowY
    clipHighY := io.clipHighY
  }

  val alignedClipLeft = ((clipLeft >> 1) ## False).asUInt.resize(10 bits)
  val emptyRect = (clipRight <= clipLeft) || (clipHighY <= clipLowY)

  val curWordX = Reg(UInt(10 bits)) init (0)
  val curY = Reg(UInt(10 bits)) init (0)

  io.cmd.ready := !active
  when(io.cmd.fire) {
    active := !emptyRect
    curWordX := alignedClipLeft
    curY := clipLowY
  }

  val wordStream = Stream(WordReq(c))
  val lane1X = (curWordX + 1).resize(10 bits)
  wordStream.valid := active
  wordStream.payload.baseX := curWordX
  wordStream.payload.y := curY
  wordStream.payload.lane0Enable := curWordX >= clipLeft && curWordX < clipRight
  wordStream.payload.lane1Enable := lane1X >= clipLeft && lane1X < clipRight
  wordStream.payload.rgbWrite := regs.fbzMode.rgbBufferMask
  wordStream.payload.auxWrite := regs.fbzMode.auxBufferMask
  wordStream.payload.enableDithering := regs.fbzMode.enableDithering
  wordStream.payload.use2x2Dither := regs.fbzMode.ditherAlgorithm
  wordStream.payload.routing := regs.routing
  wordStream.payload.color1 := regs.color1
  wordStream.payload.depthAlpha := regs.zaColor(15 downto 0)
  wordStream.payload.yOrigin := regs.fbzMode.yOrigin
  wordStream.payload.yOriginSwapValue := regs.yOriginSwapValue

  val nextWordX = (curWordX + 2).resize(10 bits)
  val atRowEnd = nextWordX >= clipRight
  val nextY = (curY + 1).resize(10 bits)
  val isLastWord = atRowEnd && nextY >= clipHighY

  when(wordStream.fire) {
    when(isLastWord) {
      active := False
    }.elsewhen(atRowEnd) {
      curWordX := alignedClipLeft
      curY := nextY
    }.otherwise {
      curWordX := nextWordX
    }
  }

  val (forLane0, forRest) = StreamFork2(wordStream, synchronous = true)
  val (forLane1, forMeta) = StreamFork2(forRest, synchronous = true)

  val dither0 = Dither()
  val dither1 = Dither()

  dither0.io.input.translateFrom(forLane0) { (out, in) =>
    val laneY = transformedY(c, in.yOrigin, in.yOriginSwapValue, in.y)
    out.r := in.color1(23 downto 16).asUInt
    out.g := in.color1(15 downto 8).asUInt
    out.b := in.color1(7 downto 0).asUInt
    out.x := in.baseX.resize(2 bits)
    out.y := laneY.resize(2 bits)
    out.enable := in.enableDithering
    out.use2x2 := in.use2x2Dither
  }

  dither1.io.input.translateFrom(forLane1) { (out, in) =>
    val laneX = (in.baseX + 1).resize(10 bits)
    val laneY = transformedY(c, in.yOrigin, in.yOriginSwapValue, in.y)
    out.r := in.color1(23 downto 16).asUInt
    out.g := in.color1(15 downto 8).asUInt
    out.b := in.color1(7 downto 0).asUInt
    out.x := laneX.resize(2 bits)
    out.y := laneY.resize(2 bits)
    out.enable := in.enableDithering
    out.use2x2 := in.use2x2Dither
  }

  val metaPipe = forMeta
    .translateWith {
      val meta = WordMeta(c)
      val outputY = transformedY(
        c,
        forMeta.payload.yOrigin,
        forMeta.payload.yOriginSwapValue,
        forMeta.payload.y
      )
      meta.address := FramebufferAddressMath.planeAddress(
        forMeta.payload.routing.colorBaseAddr,
        forMeta.payload.baseX.resize(c.addressWidth),
        outputY.resize(c.addressWidth),
        forMeta.payload.routing.pixelStride
      )
      meta.auxAddress := FramebufferAddressMath.planeAddress(
        forMeta.payload.routing.auxBaseAddr,
        forMeta.payload.baseX.resize(c.addressWidth),
        outputY.resize(c.addressWidth),
        forMeta.payload.routing.pixelStride
      )
      meta.lane0Enable := forMeta.payload.lane0Enable
      meta.lane1Enable := forMeta.payload.lane1Enable
      meta.rgbWrite := forMeta.payload.rgbWrite
      meta.auxWrite := forMeta.payload.auxWrite
      meta.depthAlpha := forMeta.payload.depthAlpha
      meta
    }
    .m2sPipe()

  val ditherJoined =
    StreamJoin(StreamJoin(dither0.io.output, dither1.io.output), metaPipe).halfPipe()
  val (forColor, forAux) = StreamFork2(ditherJoined, synchronous = true)

  io.colorWrite.valid := forColor.valid && forColor.payload._2.rgbWrite && anyLaneEnabled(
    forColor.payload._2
  )
  io.colorWrite.address := alignedWordAddress(forColor.payload._2.address)
  io.colorWrite.data := packColorWord(
    forColor.payload._1._1,
    forColor.payload._1._2,
    forColor.payload._2
  )
  io.colorWrite.mask := laneMask(forColor.payload._2)
  forColor.ready := (!forColor.payload._2.rgbWrite || !anyLaneEnabled(
    forColor.payload._2
  )) || io.colorWrite.ready

  io.auxWrite.valid := forAux.valid && forAux.payload._2.auxWrite && anyLaneEnabled(
    forAux.payload._2
  )
  io.auxWrite.address := alignedWordAddress(forAux.payload._2.auxAddress)
  io.auxWrite.data := packAuxWord(forAux.payload._2)
  io.auxWrite.mask := laneMask(forAux.payload._2)
  forAux.ready := (!forAux.payload._2.auxWrite || !anyLaneEnabled(
    forAux.payload._2
  )) || io.auxWrite.ready

  io.running := active
  io.wordValid := wordStream.valid
  io.wordReady := wordStream.ready
  io.generatedPixels := wordStream.fire ? laneCount(
    wordStream.payload.lane0Enable,
    wordStream.payload.lane1Enable
  ) | U(0, 2 bits)
  io.colorWrittenPixels := io.colorWrite.fire ? laneCount(
    forColor.payload._2.lane0Enable,
    forColor.payload._2.lane1Enable
  ) | U(0, 2 bits)
}

object FastfillWordWriter {
  case class WordReq(c: Config) extends Bundle {
    val baseX = UInt(10 bits)
    val y = UInt(10 bits)
    val lane0Enable = Bool()
    val lane1Enable = Bool()
    val rgbWrite = Bool()
    val auxWrite = Bool()
    val enableDithering = Bool()
    val use2x2Dither = Bool()
    val routing = FbRouting(c)
    val color1 = Bits(32 bits)
    val depthAlpha = Bits(16 bits)
    val yOrigin = Bool()
    val yOriginSwapValue = UInt(10 bits)
  }

  case class WordMeta(c: Config) extends Bundle {
    val address = UInt(c.addressWidth)
    val auxAddress = UInt(c.addressWidth)
    val lane0Enable = Bool()
    val lane1Enable = Bool()
    val rgbWrite = Bool()
    val auxWrite = Bool()
    val depthAlpha = Bits(16 bits)
  }

  private def transformedY(c: Config, yOrigin: Bool, yOriginSwapValue: UInt, y: UInt): UInt = {
    val transformed = UInt(c.addressWidth.value bits)
    transformed := y.resize(c.addressWidth)
    when(yOrigin) {
      transformed := (yOriginSwapValue
        .resize(c.vertexFormat.nonFraction bits)
        .asSInt - y.resize(c.vertexFormat.nonFraction bits).asSInt).asUInt
        .resize(c.addressWidth)
    }
    transformed
  }

  private def anyLaneEnabled(meta: WordMeta): Bool = meta.lane0Enable || meta.lane1Enable

  private def laneCount(lane0Enable: Bool, lane1Enable: Bool): UInt = {
    lane0Enable.asUInt.resize(2 bits) + lane1Enable.asUInt.resize(2 bits)
  }

  private def laneMask(meta: WordMeta): Bits = {
    val mask = Bits(4 bits)
    mask := 0
    when(meta.lane0Enable) {
      mask(1 downto 0) := B"2'b11"
    }
    when(meta.lane1Enable) {
      mask(3 downto 2) := B"2'b11"
    }
    mask
  }

  private def packColorWord(dit0: Dither.Output, dit1: Dither.Output, meta: WordMeta): Bits = {
    val lo = Bits(16 bits)
    val hi = Bits(16 bits)
    lo := (dit0.ditR ## dit0.ditG ## dit0.ditB).asBits
    hi := (dit1.ditR ## dit1.ditG ## dit1.ditB).asBits
    val data = Bits(32 bits)
    data := 0
    when(meta.lane0Enable) {
      data(15 downto 0) := lo
    }
    when(meta.lane1Enable) {
      data(31 downto 16) := hi
    }
    data
  }

  private def packAuxWord(meta: WordMeta): Bits = {
    val data = Bits(32 bits)
    data := 0
    when(meta.lane0Enable) {
      data(15 downto 0) := meta.depthAlpha
    }
    when(meta.lane1Enable) {
      data(31 downto 16) := meta.depthAlpha
    }
    data
  }
}
