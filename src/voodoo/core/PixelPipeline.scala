package voodoo.core

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo._
import voodoo.frontend.RegisterBank
import voodoo.raster.FastfillWordWriter

object PixelPipeline {
  case class ClipRect() extends Bundle {
    val left = UInt(10 bits)
    val right = UInt(10 bits)
    val lowY = UInt(10 bits)
    val highY = UInt(10 bits)
  }

  case class PaletteRegs() extends Bundle {
    val table0I0 = Bits(32 bits)
    val table0I1 = Bits(32 bits)
    val table0I2 = Bits(32 bits)
    val table0I3 = Bits(32 bits)
    val table0Q0 = Bits(32 bits)
    val table0Q1 = Bits(32 bits)
    val table0Q2 = Bits(32 bits)
    val table0Q3 = Bits(32 bits)
  }

  case class Controls(c: Config) extends Bundle {
    val clip = ClipRect()
    val lfb = Lfb.Regs(c)
    val fastfill = FastfillWrite.Regs(c)
    val fogTable = Vec(Bits(16 bits), 64)
    val paletteRegs = PaletteRegs()
  }

  object Controls {
    def fromRegisterBank(c: Config, regBank: RegisterBank, layout: FramebufferLayout): Controls = {
      val ctrl = Controls(c)
      ctrl.clip.left := regBank.renderConfig.clipLeftX
      ctrl.clip.right := regBank.renderConfig.clipRightX
      ctrl.clip.lowY := regBank.renderConfig.clipLowY
      ctrl.clip.highY := regBank.renderConfig.clipHighY
      ctrl.lfb := Lfb.Regs.fromRegisterBank(c, regBank, layout)
      ctrl.fastfill := FastfillWrite.Regs.fromRegisterBank(c, regBank, layout.draw)
      for ((entry, (dfog, fog)) <- ctrl.fogTable.zip(regBank.fogTable.fogTable)) {
        entry(15 downto 8) := dfog
        entry(7 downto 0) := fog
      }
      ctrl.paletteRegs.table0I0 := regBank.nccTable.table0I0
      ctrl.paletteRegs.table0I1 := regBank.nccTable.table0I1
      ctrl.paletteRegs.table0I2 := regBank.nccTable.table0I2
      ctrl.paletteRegs.table0I3 := regBank.nccTable.table0I3
      ctrl.paletteRegs.table0Q0 := regBank.nccTable.table0Q0
      ctrl.paletteRegs.table0Q1 := regBank.nccTable.table0Q1
      ctrl.paletteRegs.table0Q2 := regBank.nccTable.table0Q2
      ctrl.paletteRegs.table0Q3 := regBank.nccTable.table0Q3
      ctrl
    }
  }

  case class ExternalBusy() extends Bundle {
    val nopCmd = Bool()
    val nopCmdReset = Bool()
    val fastfillCmd = Bool()
    val swapbufferCmd = Bool()
    val swapWaiting = Bool()
  }

  object ExternalBusy {
    def fromCore(regBank: RegisterBank, swapBuffer: SwapBuffer): ExternalBusy = {
      val busy = ExternalBusy()
      busy.nopCmd := regBank.commands.nopCmd.valid
      busy.nopCmdReset := regBank.commands.nopCmd.valid && regBank.commands.nopCmd.payload(0)
      busy.fastfillCmd := regBank.commands.fastfillCmd.valid
      busy.swapbufferCmd := regBank.commands.swapbufferCmd.valid
      busy.swapWaiting := swapBuffer.io.waiting
      busy
    }
  }

  case class SpanPrefetcher(c: Config, colorPlane: Boolean) extends Component {
    val io = new Bundle {
      val span = slave(Stream(Rasterizer.PrefetchSpan(c)))
      val readReq = master(Stream(FramebufferPlaneReader.PrefetchReq(c)))
      val yOriginEnable = in Bool ()
      val yOriginSwapValue = in UInt (c.vertexFormat.nonFraction bits)
    }

    def evenWordX(value: AFix): UInt = {
      val floored = value.floor(0).asUInt.resize(c.vertexFormat.nonFraction bits)
      (floored & ~U(1, c.vertexFormat.nonFraction bits)).resized
    }

    val spanY = io.span.payload.y.floor(0).asUInt.resize(c.vertexFormat.nonFraction bits)
    val transformedY = UInt(c.vertexFormat.nonFraction bits)
    transformedY := spanY
    when(io.yOriginEnable) {
      transformedY := io.yOriginSwapValue.resize(c.vertexFormat.nonFraction bits) - spanY
    }

    val planeBase =
      if (colorPlane) io.span.payload.config.drawColorBufferBase
      else io.span.payload.config.drawAuxBufferBase
    val startAddress = FramebufferAddressMath.planeAddress(
      planeBase,
      evenWordX(io.span.payload.xStart),
      transformedY,
      io.span.payload.config.fbPixelStride
    )
    val endAddress = FramebufferAddressMath.planeAddress(
      planeBase,
      evenWordX(io.span.payload.xEnd),
      transformedY,
      io.span.payload.config.fbPixelStride
    )

    io.readReq.valid := io.span.valid
    io.span.ready := io.readReq.ready
    io.readReq.startAddress := (startAddress(c.addressWidth.value - 1 downto 1) ## U"1'b0").asUInt
    io.readReq.endAddress := (endAddress(c.addressWidth.value - 1 downto 1) ## U"1'b0").asUInt
  }
}

case class PixelPipeline(c: Config) extends Component {
  import PixelPipeline._

  val io = new Bundle {
    val triangleCmd = slave(Stream(TriangleSetup.Input(c)))
    val ftriangleCmd = slave(Stream(TriangleSetup.Input(c)))
    val fastfillCmd = slave Stream (NoData)
    val paletteWrite = slave(Flow(RegisterBank.PaletteWrite()))
    val lfbBus = slave(Bmb(Lfb.bmbParams(c)))
    val controls = in(Controls(c))
    val externalBusy = in(ExternalBusy())
    val tmuInvalidate = in Bool ()
    val pciFifoEmpty = in Bool ()
    val fbStatus = in(FramebufferMemStatus())
    val fbStats = in(FramebufferMemStats())

    val texRead = master(Bmb(Tmu.bmbParams(c)))
    val lfbReadBus = master(Bmb(Lfb.fbReadBmbParams(c)))
    val colorReadReq = master(Stream(FramebufferPlaneBuffer.ReadReq(c)))
    val colorReadRsp = slave(Stream(FramebufferPlaneBuffer.ReadRsp()))
    val auxReadReq = master(Stream(FramebufferPlaneBuffer.ReadReq(c)))
    val auxReadRsp = slave(Stream(FramebufferPlaneBuffer.ReadRsp()))
    val prefetchColor = master(Stream(FramebufferPlaneReader.PrefetchReq(c)))
    val prefetchAux = master(Stream(FramebufferPlaneReader.PrefetchReq(c)))
    val colorWrite = master(Stream(FramebufferPlaneBuffer.WriteReq(c)))
    val auxWrite = master(Stream(FramebufferPlaneBuffer.WriteReq(c)))

    val stats = out(CoreStats())
    val debug = out(CoreDebug())
  }

  private def buildPaletteWriteFlow(regs: PaletteRegs): Flow[Tmu.PaletteWrite] = {
    val flow = Flow(Tmu.PaletteWrite())
    flow.valid := False
    flow.payload.address := 0
    flow.payload.data := 0

    val entries = Seq(
      (regs.table0I0, false),
      (regs.table0I1, true),
      (regs.table0I2, false),
      (regs.table0I3, true),
      (regs.table0Q0, false),
      (regs.table0Q1, true),
      (regs.table0Q2, false),
      (regs.table0Q3, true)
    )

    for ((reg, isOdd) <- entries) {
      val prev = RegNext(reg)
      when(reg =/= prev && reg(31)) {
        flow.valid := True
        val idx = reg(30 downto 23).asUInt & 0xfe
        flow.payload.address := (if (isOdd) (idx | 1).resized else idx.resized)
        flow.payload.data := reg(23 downto 0)
      }
    }
    flow
  }

  val triangleSetup = TriangleSetup(c)
  val rasterizer = Rasterizer(c)
  val fastfillWriter = FastfillWordWriter(c)
  val lfb = Lfb(c)
  val tmu = Tmu(c)
  val colorCombine = ColorCombine(c)
  val fog = Fog(c)
  val fbAccess = FramebufferAccess(c)
  val dither = Dither()
  val writeColor = Write(c)
  val writeAux = Write(c)

  io.triangleCmd.simPublic()
  io.ftriangleCmd.simPublic()
  triangleSetup.i.simPublic()
  triangleSetup.o.simPublic()
  rasterizer.i.simPublic()
  rasterizer.o.simPublic()
  tmu.io.input.simPublic()
  tmu.io.output.simPublic()
  colorCombine.io.input.simPublic()
  colorCombine.io.output.simPublic()
  fog.io.input.simPublic()
  fog.io.output.simPublic()
  fbAccess.io.input.simPublic()
  fbAccess.io.output.simPublic()
  writeColor.i.fromPipeline.simPublic()
  writeAux.i.fromPipeline.simPublic()
  writeColor.o.fbWrite.simPublic()
  writeAux.o.fbWrite.simPublic()

  val triangleSetupInput = StreamArbiterFactory.assumeOhInput
    .on(Seq(io.triangleCmd, io.ftriangleCmd))
  triangleSetup.i << triangleSetupInput

  rasterizer.i << triangleSetup.o
  rasterizer.enableClipping := io.controls.fastfill.fbzMode.enableClipping
  rasterizer.clipLeft := io.controls.clip.left
  rasterizer.clipRight := io.controls.clip.right
  rasterizer.clipLowY := io.controls.clip.lowY
  rasterizer.clipHighY := io.controls.clip.highY

  fastfillWriter.io.cmd << io.fastfillCmd
  fastfillWriter.io.regs := io.controls.fastfill
  fastfillWriter.io.clipLeft := io.controls.clip.left
  fastfillWriter.io.clipRight := io.controls.clip.right
  fastfillWriter.io.clipLowY := io.controls.clip.lowY
  fastfillWriter.io.clipHighY := io.controls.clip.highY

  lfb.io.bus <> io.lfbBus
  lfb.io.regs := io.controls.lfb
  lfb.io.pciFifoEmpty := io.pciFifoEmpty
  io.lfbReadBus <> lfb.io.fbReadBus

  tmu.io.paletteWrite << io.paletteWrite.translateWith {
    val out = Tmu.PaletteWrite()
    out.address := io.paletteWrite.payload.address
    out.data := io.paletteWrite.payload.data
    out
  }
  tmu.io.invalidate := io.tmuInvalidate
  io.texRead <> tmu.io.texRead

  val yOriginEnable = io.controls.fastfill.fbzMode.yOrigin
  val yOriginSwapValue = io.controls.fastfill.yOriginSwapValue
  val rasterYTransformed = rasterizer.o.map { out =>
    val result = cloneOf(out)
    result := out
    when(yOriginEnable) {
      result.coords(1) := yOriginSwapValue.resize(c.vertexFormat.nonFraction bits).asSInt - out
        .coords(1)
    }
    result
  }
  rasterYTransformed.simPublic()

  val rasterYPipe = rasterYTransformed.stage()
  val rasterFork = StreamFork2(rasterYPipe, synchronous = true)
  val tmuGradQueue = rasterFork._2.queue(64).stage().stage()

  val tmuInput = Stream(Tmu.Input(c))
  tmuInput.translateFrom(rasterFork._1.stage())((out, in) => out := Tmu.Input.fromRasterizer(c, in))
  tmuInput >/-> tmu.io.input

  val tmuJoined = StreamJoin(tmu.io.output.stage(), tmuGradQueue)
  if (c.trace.enabled) {
    tmuGradQueue.simPublic()
    tmuJoined.simPublic()
    when(tmuJoined.valid) {
      assert(tmuJoined.payload._1.trace.asBits === tmuJoined.payload._2.trace.asBits)
    }
  }

  val colorCombineInput = Stream(ColorCombine.Input(c))
  colorCombineInput.translateFrom(tmuJoined.stage())((out, payload) =>
    out := ColorCombine.Input.fromTmuAndRaster(c, payload._1, payload._2)
  )
  colorCombineInput >/-> colorCombine.io.input

  val spanPrefetchQueue = rasterizer.prefetchSpan.queue(8)
  val spanPrefetchFork = StreamFork2(spanPrefetchQueue, synchronous = true)
  val colorPrefetcher = SpanPrefetcher(c, colorPlane = true)
  val auxPrefetcher = SpanPrefetcher(c, colorPlane = false)

  colorPrefetcher.io.span << spanPrefetchFork._1
  auxPrefetcher.io.span << spanPrefetchFork._2
  colorPrefetcher.io.yOriginEnable := yOriginEnable
  colorPrefetcher.io.yOriginSwapValue := yOriginSwapValue.resize(c.vertexFormat.nonFraction bits)
  auxPrefetcher.io.yOriginEnable := yOriginEnable
  auxPrefetcher.io.yOriginSwapValue := yOriginSwapValue.resize(c.vertexFormat.nonFraction bits)

  io.prefetchColor << colorPrefetcher.io.readReq
  io.prefetchAux << auxPrefetcher.io.readReq

  val ckBits = colorCombine.io.output.payload.chromaKey
  val ckR = ckBits(23 downto 16).asUInt
  val ckG = ckBits(15 downto 8).asUInt
  val ckB = ckBits(7 downto 0).asUInt
  val ccColor = colorCombine.io.output.payload.color
  val chromaKill = colorCombine.io.output.payload.fbzMode.enableChromaKey &&
    ccColor.r === ckR && ccColor.g === ckG && ccColor.b === ckB
  val afterChromaKey = colorCombine.io.output.throwWhen(chromaKill).stage()

  fog.io.fogTable := io.controls.fogTable
  val fogArbiterInput =
    StreamArbiterFactory.lowerFirst.on(Seq(afterChromaKey, lfb.io.pipelineOutput))
  fogArbiterInput >/-> fog.io.input

  val alphaBits = fog.io.output.payload.alphaMode
  val alphaTestEnable = alphaBits.alphaTestEnable
  val alphaFunc = alphaBits.alphaFunc
  val alphaRef = alphaBits.alphaRef
  val srcAlpha = fog.io.output.payload.alpha
  val alphaPassed = alphaFunc.mux(
    0 -> False,
    1 -> (srcAlpha < alphaRef),
    2 -> (srcAlpha === alphaRef),
    3 -> (srcAlpha <= alphaRef),
    4 -> (srcAlpha > alphaRef),
    5 -> (srcAlpha =/= alphaRef),
    6 -> (srcAlpha >= alphaRef),
    7 -> True
  )
  val alphaKill = alphaTestEnable && !alphaPassed
  val afterAlphaTest = fog.io.output.throwWhen(alphaKill).stage()

  afterAlphaTest >/-> fbAccess.io.input
  fbAccess.io.fbReadColorRsp << io.colorReadRsp
  fbAccess.io.fbReadAuxRsp << io.auxReadRsp
  io.colorReadReq << fbAccess.io.fbReadColorReq.s2mPipe()
  io.auxReadReq << fbAccess.io.fbReadAuxReq.s2mPipe()

  val trianglePreDither = fbAccess.io.output
    .translateWith {
      Write.PreDither.fromFramebufferAccess(c, fbAccess.io.output.payload)
    }
    .m2sPipe()

  val pixelsInCounter = Reg(UInt(24 bits)) init (0)
  val chromaFailCounter = Reg(UInt(24 bits)) init (0)
  val zFuncFailCounter = Reg(UInt(24 bits)) init (0)
  val aFuncFailCounter = Reg(UInt(24 bits)) init (0)
  val pixelsOutCounter = Reg(UInt(24 bits)) init (0)
  val clearPerfCounters = io.externalBusy.nopCmdReset

  when(clearPerfCounters) {
    pixelsInCounter := 0
    chromaFailCounter := 0
    zFuncFailCounter := 0
    aFuncFailCounter := 0
    pixelsOutCounter := 0
  } otherwise {
    when(colorCombine.io.output.fire && chromaKill) {
      chromaFailCounter := chromaFailCounter + 1
    }
    when(fog.io.output.fire && alphaKill) {
      aFuncFailCounter := aFuncFailCounter + 1
    }
    when(fbAccess.io.zFuncFail) {
      zFuncFailCounter := zFuncFailCounter + 1
    }

    val pixelsInDelta =
      colorCombine.io.output.fire.asUInt.resize(3 bits) +
        lfb.io.pipelineOutput.fire.asUInt.resize(3 bits) +
        lfb.io.writeOutput.fire.asUInt.resize(3 bits) +
        fastfillWriter.io.generatedPixels.resize(3 bits)
    pixelsInCounter := pixelsInCounter + pixelsInDelta.resize(24 bits)
  }

  val preDitherMerged = StreamArbiterFactory.lowerFirst
    .on(Seq(trianglePreDither, lfb.io.writeOutput))
    .stage()

  val (forDither, forPipe) = StreamFork2(preDitherMerged, synchronous = true)
  dither.io.input.translateFrom(forDither)((ditIn, pd) => ditIn := Dither.Input.fromPreDither(pd))

  val preDitherPiped = forPipe.m2sPipe()
  val ditherJoined = StreamJoin(dither.io.output, preDitherPiped)
  val (forColorWrite, forAuxWrite) = StreamFork2(ditherJoined, synchronous = true)

  val colorWriteInput = forColorWrite
    .throwWhen(!forColorWrite.payload._2.rgbWrite)
    .translateWith {
      val ditOut = forColorWrite.payload._1
      val pd = forColorWrite.payload._2
      Write.Input.colorFromDither(c, pd, ditOut, if (c.trace.enabled) pd.trace else null)
    }
  colorWriteInput >/-> writeColor.i.fromPipeline

  val auxWriteInput = forAuxWrite
    .throwWhen(!forAuxWrite.payload._2.auxWrite)
    .translateWith {
      val pd = forAuxWrite.payload._2
      Write.Input.auxFromPreDither(c, pd, if (c.trace.enabled) pd.trace else null)
    }
  auxWriteInput >/-> writeAux.i.fromPipeline

  val fastfillColorWrite = fastfillWriter.io.colorWrite.s2mPipe()
  val fastfillAuxWrite = fastfillWriter.io.auxWrite.s2mPipe()
  val colorWriteMerged =
    StreamArbiterFactory.lowerFirst.on(Seq(fastfillColorWrite, writeColor.o.fbWrite))
  val auxWriteMerged = StreamArbiterFactory.lowerFirst.on(Seq(fastfillAuxWrite, writeAux.o.fbWrite))

  io.colorWrite << colorWriteMerged
  io.auxWrite << auxWriteMerged

  when(!clearPerfCounters && writeColor.i.fromPipeline.fire) {
    pixelsOutCounter := pixelsOutCounter + 1
  }
  when(fastfillWriter.io.colorWrite.fire) {
    pixelsOutCounter := pixelsOutCounter + fastfillWriter.io.colorWrittenPixels.resize(24 bits)
  }

  io.stats.pixelsIn := pixelsInCounter
  io.stats.chromaFail := chromaFailCounter
  io.stats.zFuncFail := zFuncFailCounter
  io.stats.aFuncFail := aFuncFailCounter
  io.stats.pixelsOut := pixelsOutCounter

  io.debug.busy.triangleSetupValid := triangleSetup.o.valid
  io.debug.busy.rasterizerRunning := rasterizer.running
  io.debug.busy.tmuInputValid := tmu.io.input.valid
  io.debug.busy.tmuBusy := tmu.io.busy
  io.debug.busy.fbAccessBusy := fbAccess.io.busy
  io.debug.busy.colorCombineInputValid := colorCombine.io.input.valid
  io.debug.busy.fogBusy := fog.io.busy
  io.debug.busy.fbAccessInputValid := fbAccess.io.input.valid
  io.debug.busy.writeColorInputValid := writeColor.i.fromPipeline.valid
  io.debug.busy.writeAuxInputValid := writeAux.i.fromPipeline.valid
  io.debug.busy.fastfillRunning := fastfillWriter.io.running
  io.debug.busy.swapWaiting := io.externalBusy.swapWaiting
  io.debug.busy.lfbBusy := lfb.io.busy
  io.debug.busy.fastfillOutputValid := fastfillWriter.io.wordValid
  io.debug.busy.fastfillOutputReady := fastfillWriter.io.wordReady
  io.debug.busy.fastfillWriteValid := fastfillWriter.io.colorWrite.valid || fastfillWriter.io.auxWrite.valid
  io.debug.busy.fastfillWriteReady := fastfillWriter.io.colorWrite.ready && fastfillWriter.io.auxWrite.ready
  io.debug.busy.preDitherMergedValid := preDitherMerged.valid
  io.debug.busy.preDitherMergedReady := preDitherMerged.ready
  io.debug.busy.preDitherPipedValid := preDitherPiped.valid
  io.debug.busy.preDitherPipedReady := preDitherPiped.ready
  io.debug.busy.ditherOutputValid := dither.io.output.valid
  io.debug.busy.ditherOutputReady := dither.io.output.ready
  io.debug.busy.ditherJoinedValid := ditherJoined.valid
  io.debug.busy.ditherJoinedReady := ditherJoined.ready
  io.debug.busy.colorForkValid := forColorWrite.valid
  io.debug.busy.colorForkReady := forColorWrite.ready
  io.debug.busy.auxForkValid := forAuxWrite.valid
  io.debug.busy.auxForkReady := forAuxWrite.ready
  io.debug.busy.writeColorReady := writeColor.i.fromPipeline.ready
  io.debug.busy.writeAuxReady := writeAux.i.fromPipeline.ready
  io.debug.busy.fastfillWriteAuxWrite := fastfillWriter.io.auxWrite.valid

  io.debug.writePath.fastfillRunning := fastfillWriter.io.running
  io.debug.writePath.fastfillOutputValid := fastfillWriter.io.wordValid
  io.debug.writePath.fastfillOutputReady := fastfillWriter.io.wordReady
  io.debug.writePath.fastfillWriteValid := fastfillWriter.io.colorWrite.valid || fastfillWriter.io.auxWrite.valid
  io.debug.writePath.fastfillWriteReady := fastfillWriter.io.colorWrite.ready && fastfillWriter.io.auxWrite.ready
  io.debug.writePath.preDitherMergedValid := preDitherMerged.valid
  io.debug.writePath.preDitherMergedReady := preDitherMerged.ready
  io.debug.writePath.ditherJoinedValid := ditherJoined.valid
  io.debug.writePath.ditherJoinedReady := ditherJoined.ready
  io.debug.writePath.colorForkValid := forColorWrite.valid
  io.debug.writePath.colorForkReady := forColorWrite.ready
  io.debug.writePath.colorWriteInputValid := colorWriteInput.valid
  io.debug.writePath.colorWriteInputReady := colorWriteInput.ready
  io.debug.writePath.colorWriteFbValid := writeColor.o.fbWrite.valid
  io.debug.writePath.colorWriteFbReady := writeColor.o.fbWrite.ready
  io.debug.writePath.fbColorBusy := io.fbStatus.colorBusy
  io.debug.writePath.fbAuxBusy := io.fbStatus.auxBusy
  io.debug.writePath.auxForkValid := forAuxWrite.valid
  io.debug.writePath.auxForkReady := forAuxWrite.ready
  io.debug.writePath.auxWriteInputValid := auxWriteInput.valid
  io.debug.writePath.auxWriteInputReady := auxWriteInput.ready
  io.debug.writePath.auxWriteFbValid := writeAux.o.fbWrite.valid
  io.debug.writePath.auxWriteFbReady := writeAux.o.fbWrite.ready
  io.debug.writePath.fbFillHitsNonZero := io.fbStats.fillHits.orR
  io.debug.writePath.fbFillMissesNonZero := io.fbStats.fillMisses.orR
  io.debug.writePath.fbFillBurstCountNonZero := io.fbStats.fillBurstCount.orR
  io.debug.writePath.fbFillBurstBeatsNonZero := io.fbStats.fillBurstBeats.orR
  io.debug.writePath.fbFillStallCyclesNonZero := io.fbStats.fillStallCycles.orR
  io.debug.writePath.pixelsInNonZero := pixelsInCounter.orR
  io.debug.writePath.pixelsOutNonZero := pixelsOutCounter.orR
  io.debug.writePath.zFuncFailNonZero := zFuncFailCounter.orR
  io.debug.writePath.aFuncFailNonZero := aFuncFailCounter.orR

  val pipelineBusySignal =
    triangleSetup.o.valid || rasterizer.running || tmu.io.input.valid ||
      tmu.io.busy || fbAccess.io.busy || io.fbStatus.colorBusy || io.fbStatus.auxBusy ||
      colorCombine.io.input.valid || fog.io.busy || fbAccess.io.input.valid ||
      writeColor.i.fromPipeline.valid || writeAux.i.fromPipeline.valid ||
      writeColor.o.fbWrite.valid || writeAux.o.fbWrite.valid ||
      fastfillWriter.io.running || fastfillWriter.io.wordValid || fastfillWriter.io.colorWrite.valid || fastfillWriter.io.auxWrite.valid ||
      preDitherMerged.valid || preDitherPiped.valid ||
      dither.io.output.valid || ditherJoined.valid ||
      forColorWrite.valid || forAuxWrite.valid ||
      colorWriteInput.valid || auxWriteInput.valid ||
      io.externalBusy.nopCmd || io.externalBusy.fastfillCmd ||
      io.externalBusy.swapbufferCmd || io.externalBusy.swapWaiting || lfb.io.busy

  io.debug.pipelineBusy := pipelineBusySignal
  lfb.io.pipelineBusy := pipelineBusySignal

  io.stats.exposeToSim()
  io.debug.exposeToSim()
}
