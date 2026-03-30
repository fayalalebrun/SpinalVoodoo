//> using target.scope test

package voodoo

import org.scalatest.funsuite.AnyFunSuite
import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Paths}
import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim._
import voodoo.core.{
  CoreDebug,
  CoreStats,
  FramebufferMemStats,
  FramebufferMemStatus,
  FramebufferMemSubsystem,
  PixelPipeline
}

private object TrianglePipelineThroughputTest {
  case class TriangleCmd(c: Config) extends Bundle {
    private val coordWidth = AFix(c.vertexFormat).raw.getWidth
    val ax = SInt(coordWidth bits)
    val ay = SInt(coordWidth bits)
    val bx = SInt(coordWidth bits)
    val by = SInt(coordWidth bits)
    val cx = SInt(coordWidth bits)
    val cy = SInt(coordWidth bits)
    val signBit = Bool()
  }

  case class Harness(c: Config, rectWidth: Int, rectHeight: Int) extends Component {
    val colorBase = 0x00000
    val auxBase = 0x20000

    val io = new Bundle {
      val triangle = slave(Stream(TrianglePipelineThroughputTest.TriangleCmd(c)))
      val fbMemWrite = master(Bmb(Core.fbMemBmbParams(c)))
      val fbColorReadMem = master(Bmb(Core.fbMemBmbParams(c)))
      val fbAuxReadMem = master(Bmb(Core.fbMemBmbParams(c)))
      val texMem = master(Bmb(Tmu.bmbParams(c)))
      val triangleReady = out Bool ()
      val activeBusy = out Bool ()
      val pipelineBusy = out Bool ()
      val stats = out(CoreStats())
      val fbStatus = out(FramebufferMemStatus())
      val fbStats = out(FramebufferMemStats())
      val debug = out(CoreDebug())
    }

    private def zeroBundle[T <: Data](that: T): Unit = {
      that.flatten.foreach(_.allowOverride())
      that.assignFromBits(B(0, widthOf(that) bits))
    }

    private val triangleGradients = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)
    zeroBundle(triangleGradients)
    triangleGradients.redGrad.start := 96.0
    triangleGradients.greenGrad.start := 80.0
    triangleGradients.blueGrad.start := 48.0
    triangleGradients.depthGrad.start := 0.0
    triangleGradients.alphaGrad.start := 128.0
    triangleGradients.wGrad.start := 1.0

    private val triangleHiAlpha = TriangleSetup.HiAlpha(c)
    zeroBundle(triangleHiAlpha)
    triangleHiAlpha.start := 128.0

    private val triangleTexHi = TriangleSetup.HiTexCoords(c)
    zeroBundle(triangleTexHi)
    triangleTexHi.sStart := 0.0
    triangleTexHi.tStart := 0.0

    private val triangleConfig = TriangleSetup.PerTriangleConfig(c)
    zeroBundle(triangleConfig)

    triangleConfig.fbzColorPath.rgbSel := RgbSel.TEXTURE
    triangleConfig.fbzColorPath.alphaSel := AlphaSel.ITERATED
    triangleConfig.fbzColorPath.localSelect := LocalSel.ITERATED
    triangleConfig.fbzColorPath.alphaLocalSelect := AlphaLocalSel.ITERATED
    triangleConfig.fbzColorPath.mselect := MSelect.ZERO
    triangleConfig.fbzColorPath.alphaMselect := MSelect.ZERO
    triangleConfig.fbzColorPath.add := AddMode.NONE
    triangleConfig.fbzColorPath.alphaAdd := AddMode.NONE
    triangleConfig.fbzColorPath.textureEnable := True

    triangleConfig.fogMode.fogEnable := True
    triangleConfig.fogMode.fogAdd := False
    triangleConfig.fogMode.fogMult := False
    triangleConfig.fogMode.fogModeSelect := 1
    triangleConfig.fogMode.fogConstant := False

    triangleConfig.alphaMode.alphaTestEnable := True
    triangleConfig.alphaMode.alphaFunc := 7
    triangleConfig.alphaMode.alphaBlendEnable := True
    triangleConfig.alphaMode.rgbSrcFact := 1
    triangleConfig.alphaMode.rgbDstFact := 5
    triangleConfig.alphaMode.aSrcFact := 1
    triangleConfig.alphaMode.aDstFact := 5
    triangleConfig.alphaMode.alphaRef := 0

    triangleConfig.fbzMode.enableClipping := True
    triangleConfig.fbzMode.enableChromaKey := True
    triangleConfig.fbzMode.enableDepthBuffer := True
    triangleConfig.fbzMode.depthFunction := 7
    triangleConfig.fbzMode.enableDithering := True
    triangleConfig.fbzMode.rgbBufferMask := True
    triangleConfig.fbzMode.auxBufferMask := True
    triangleConfig.fbzMode.ditherAlgorithm := False
    triangleConfig.fbzMode.drawBuffer := 0
    triangleConfig.fbzMode.enableAlphaPlanes := True
    triangleConfig.fbzMode.enableDitherSubtract := True

    triangleConfig.tmuTextureMode := B(
      (1 << 6) | (1 << 7) | (Tmu.TextureFormat.RGB565 << 8),
      32 bits
    )
    triangleConfig.tmuTexBaseAddr := 0
    triangleConfig.tmuTLOD := B(0x00200, 27 bits)
    triangleConfig.tmuSendConfig := False
    triangleConfig.tmudSdX := 0.0
    triangleConfig.tmudTdX := 0.0
    triangleConfig.tmudSdY := 0.0
    triangleConfig.tmudTdY := 0.0
    triangleConfig.color0 := B(0x80406020L, 32 bits)
    triangleConfig.color1 := B(0xc0907040L, 32 bits)
    triangleConfig.fogColor := B(0x00304050L, 32 bits)
    triangleConfig.chromaKey := B(0x00010203L, 32 bits)
    triangleConfig.zaColor := B(0x00004080L, 32 bits)
    triangleConfig.routing.colorBaseAddr := colorBase
    triangleConfig.routing.auxBaseAddr := auxBase
    triangleConfig.routing.pixelStride := rectWidth

    if (c.packedTexLayout) {
      var offset = 0
      for (lod <- 0 until 9) {
        val wBits = scala.math.max(8 - lod, 0)
        val hBits = scala.math.max(8 - lod, 0)
        val size = 1 << (wBits + hBits + 1)
        triangleConfig.texTables.texBase(lod) := offset
        triangleConfig.texTables.texEnd(lod) := offset + size
        triangleConfig.texTables.texShift(lod) := wBits
        offset += size
      }
    }

    val controls = PixelPipeline.Controls(c)
    zeroBundle(controls)
    controls.clip.left := 0
    controls.clip.right := rectWidth
    controls.clip.lowY := 0
    controls.clip.highY := rectHeight
    controls.fastfill.fbzMode.enableClipping := True
    controls.fastfill.routing.colorBaseAddr := colorBase
    controls.fastfill.routing.auxBaseAddr := auxBase
    controls.fastfill.routing.pixelStride := rectWidth

    val externalBusy = PixelPipeline.ExternalBusy()
    zeroBundle(externalBusy)

    val pipeline = PixelPipeline(c)
    val framebuffer = FramebufferMemSubsystem(c)

    val triangleInput = Stream(TriangleSetup.Input(c))
    triangleInput.valid := io.triangle.valid
    io.triangle.ready := triangleInput.ready
    triangleInput.payload.triWithSign.tri(0)(0).raw := io.triangle.payload.ax.asBits
    triangleInput.payload.triWithSign.tri(0)(1).raw := io.triangle.payload.ay.asBits
    triangleInput.payload.triWithSign.tri(1)(0).raw := io.triangle.payload.bx.asBits
    triangleInput.payload.triWithSign.tri(1)(1).raw := io.triangle.payload.by.asBits
    triangleInput.payload.triWithSign.tri(2)(0).raw := io.triangle.payload.cx.asBits
    triangleInput.payload.triWithSign.tri(2)(1).raw := io.triangle.payload.cy.asBits
    triangleInput.payload.triWithSign.signBit := io.triangle.payload.signBit
    triangleInput.payload.grads := triangleGradients
    triangleInput.payload.hiAlpha := triangleHiAlpha
    triangleInput.payload.texHi := triangleTexHi
    triangleInput.payload.config := triangleConfig

    pipeline.io.triangleCmd << triangleInput
    pipeline.io.ftriangleCmd.valid := False
    zeroBundle(pipeline.io.ftriangleCmd.payload)
    pipeline.io.fastfillCmd.valid := False

    pipeline.io.lfbBus.cmd.valid := False
    pipeline.io.lfbBus.cmd.fragment.address := 0
    pipeline.io.lfbBus.cmd.fragment.opcode := Bmb.Cmd.Opcode.WRITE
    pipeline.io.lfbBus.cmd.fragment.length := 0
    pipeline.io.lfbBus.cmd.fragment.source := 0
    pipeline.io.lfbBus.cmd.fragment.data := 0
    pipeline.io.lfbBus.cmd.fragment.mask := 0
    pipeline.io.lfbBus.cmd.last := True
    pipeline.io.lfbBus.rsp.ready := True

    pipeline.io.controls := controls
    pipeline.io.externalBusy := externalBusy
    pipeline.io.tmuInvalidate := False
    pipeline.io.pciFifoEmpty := True
    pipeline.io.fbStatus := framebuffer.io.status
    pipeline.io.fbStats := framebuffer.io.stats

    framebuffer.io.colorWrite <> pipeline.io.colorWrite
    framebuffer.io.auxWrite <> pipeline.io.auxWrite
    framebuffer.io.colorReadReq <> pipeline.io.colorReadReq
    framebuffer.io.colorReadRsp <> pipeline.io.colorReadRsp
    framebuffer.io.auxReadReq <> pipeline.io.auxReadReq
    framebuffer.io.auxReadRsp <> pipeline.io.auxReadRsp
    framebuffer.io.prefetchColor <> pipeline.io.prefetchColor
    framebuffer.io.prefetchAux <> pipeline.io.prefetchAux
    framebuffer.io.lfbReadBus <> pipeline.io.lfbReadBus
    framebuffer.io.flush := False

    io.fbMemWrite <> framebuffer.io.fbMemWrite
    io.fbColorReadMem <> framebuffer.io.fbColorReadMem
    io.fbAuxReadMem <> framebuffer.io.fbAuxReadMem
    io.texMem <> pipeline.io.texRead

    val activeBusySignal =
      pipeline.io.debug.busy.triangleSetupValid ||
        pipeline.io.debug.busy.rasterizerRunning ||
        pipeline.io.debug.busy.tmuInputValid ||
        pipeline.io.debug.busy.tmuBusy ||
        pipeline.io.debug.busy.fbAccessBusy ||
        pipeline.io.debug.busy.colorCombineInputValid ||
        pipeline.io.debug.busy.fogBusy ||
        pipeline.io.debug.busy.fbAccessInputValid ||
        pipeline.io.debug.busy.writeColorInputValid ||
        pipeline.io.debug.busy.writeAuxInputValid ||
        pipeline.io.debug.busy.fastfillRunning ||
        pipeline.io.debug.busy.lfbBusy ||
        pipeline.io.debug.busy.fastfillOutputValid ||
        pipeline.io.debug.busy.fastfillWriteValid ||
        pipeline.io.debug.busy.preDitherMergedValid ||
        pipeline.io.debug.busy.preDitherPipedValid ||
        pipeline.io.debug.busy.ditherOutputValid ||
        pipeline.io.debug.busy.ditherJoinedValid ||
        pipeline.io.debug.busy.colorForkValid ||
        pipeline.io.debug.busy.auxForkValid ||
        pipeline.io.debug.writePath.colorWriteFbValid ||
        pipeline.io.debug.writePath.auxWriteFbValid

    io.triangleReady := io.triangle.ready
    io.activeBusy := activeBusySignal
    io.pipelineBusy := pipeline.io.debug.pipelineBusy
    io.stats := pipeline.io.stats
    io.fbStatus := framebuffer.io.status
    io.fbStats := framebuffer.io.stats
    io.debug := pipeline.io.debug
  }
}

class TrianglePipelineThroughputTest extends AnyFunSuite {
  import TrianglePipelineThroughputTest._

  private def mkConfig =
    Config
      .voodoo1()
      .copy(
        addressWidth = 20 bits,
        memBurstLengthWidth = 6,
        fbWriteBufferLineWords = 64,
        fbWriteBufferCount = 2,
        texFillLineWords = 8,
        useFbWriteBuffer = true,
        useTexFillCache = true,
        texFillCacheSlots = 16,
        texFillRequestWindow = 16,
        trace = TraceConfig()
      )

  private val rectWidth = 128
  private val rectHeight = 64
  private val fixedScale = 16
  private val measuredPairs = 4
  private val warmupPixels = 2048
  private val measuredPixels = 16384

  private def fixedCoord(v: Int): Int = v * fixedScale

  private def initDut(dut: Harness): Unit = {
    dut.clockDomain.forkStimulus(10)
    dut.io.triangle.valid #= false
    dut.io.triangle.payload.ax #= 0
    dut.io.triangle.payload.ay #= 0
    dut.io.triangle.payload.bx #= 0
    dut.io.triangle.payload.by #= 0
    dut.io.triangle.payload.cx #= 0
    dut.io.triangle.payload.cy #= 0
    dut.io.triangle.payload.signBit #= false
    dut.clockDomain.waitSampling()
  }

  private def attachFramebufferMemory(dut: Harness): BmbMemoryAgent = {
    val memory = new BmbMemoryAgent(1 << dut.c.addressWidth.value)
    memory.addPort(dut.io.fbMemWrite, 0, dut.clockDomain, withDriver = true, withStall = false)
    memory.addPort(dut.io.fbColorReadMem, 0, dut.clockDomain, withDriver = true, withStall = false)
    memory.addPort(dut.io.fbAuxReadMem, 0, dut.clockDomain, withDriver = true, withStall = false)
    memory
  }

  private def attachTextureMemory(dut: Harness): BmbMemoryAgent = {
    val memory = new BmbMemoryAgent(1 << dut.c.addressWidth.value)
    memory.addPort(dut.io.texMem, 0, dut.clockDomain, withDriver = true, withStall = false)
    memory
  }

  private def sendTriangle(
      dut: Harness,
      ax: Int,
      ay: Int,
      bx: Int,
      by: Int,
      cx: Int,
      cy: Int,
      signBit: Boolean = false
  ): Unit = {
    dut.io.triangle.valid #= true
    dut.io.triangle.payload.ax #= fixedCoord(ax)
    dut.io.triangle.payload.ay #= fixedCoord(ay)
    dut.io.triangle.payload.bx #= fixedCoord(bx)
    dut.io.triangle.payload.by #= fixedCoord(by)
    dut.io.triangle.payload.cx #= fixedCoord(cx)
    dut.io.triangle.payload.cy #= fixedCoord(cy)
    dut.io.triangle.payload.signBit #= signBit
    dut.clockDomain.waitSamplingWhere(dut.io.triangleReady.toBoolean)
    dut.clockDomain.waitSampling()
    dut.io.triangle.valid #= false
  }

  private def sendRectangleAsTwoTriangles(dut: Harness): Unit = {
    sendTriangle(dut, 0, 0, rectWidth, 0, 0, rectHeight)
    sendTriangle(dut, rectWidth, 0, rectWidth, rectHeight, 0, rectHeight)
  }

  private case class OutputThroughputMetrics(
      warmupReachedAtCycle: Long,
      measuredWindowCycles: Long,
      measuredWindowPixels: Long,
      cyclesPerPixel: Double,
      pixelsPerCycle: Double,
      totalPixelsOut: Long
  )

  private def measureOutputThroughput(dut: Harness, pairCount: Int): OutputThroughputMetrics = {
    fork {
      for (_ <- 0 until pairCount) {
        sendRectangleAsTwoTriangles(dut)
      }
    }

    var cycle = 0L
    var warmupStartCycle = -1L
    var warmupStartPixels = -1L
    var measuredEndCycle = -1L
    var totalPixelsOut = 0L
    val timeout = rectWidth * rectHeight * pairCount * 32

    while (cycle < timeout && measuredEndCycle < 0) {
      dut.clockDomain.waitSampling()
      cycle += 1
      totalPixelsOut = dut.io.stats.pixelsOut.toLong

      if (warmupStartCycle < 0 && totalPixelsOut >= warmupPixels) {
        warmupStartCycle = cycle
        warmupStartPixels = totalPixelsOut
      }

      if (warmupStartCycle >= 0 && totalPixelsOut - warmupStartPixels >= measuredPixels) {
        measuredEndCycle = cycle
      }
    }

    assert(warmupStartCycle >= 0, s"did not reach warmup threshold of $warmupPixels output pixels")
    assert(
      measuredEndCycle >= 0,
      s"did not measure $measuredPixels output pixels within $timeout cycles"
    )

    val measuredWindowCycles = measuredEndCycle - warmupStartCycle
    val measuredWindowPixels = totalPixelsOut - warmupStartPixels
    val cyclesPerPixel = measuredWindowCycles.toDouble / measuredWindowPixels.toDouble
    val pixelsPerCycle = measuredWindowPixels.toDouble / measuredWindowCycles.toDouble

    OutputThroughputMetrics(
      warmupReachedAtCycle = warmupStartCycle,
      measuredWindowCycles = measuredWindowCycles,
      measuredWindowPixels = measuredWindowPixels,
      cyclesPerPixel = cyclesPerPixel,
      pixelsPerCycle = pixelsPerCycle,
      totalPixelsOut = totalPixelsOut
    )
  }

  test("triangle pipeline reaches near-full throughput with full feature path enabled") {
    val config = mkConfig

    SimConfig.withIVerilog.compile(Harness(config, rectWidth, rectHeight)).doSim { dut =>
      initDut(dut)
      attachFramebufferMemory(dut)
      attachTextureMemory(dut)

      val measured = measureOutputThroughput(dut, measuredPairs)
      println(
        f"Triangle pipeline output throughput after $warmupPixels%d warmup pixels: ${measured.measuredWindowPixels}%d pixels over ${measured.measuredWindowCycles}%d cycles = ${measured.pixelsPerCycle}%.3f px/cycle (${measured.cyclesPerPixel}%.3f cycles/px)"
      )
      assert(
        measured.cyclesPerPixel <= 5.0,
        f"expected the warmed output-side throughput to stay below 5.0 cycles/pixel, got ${measured.cyclesPerPixel}%.3f"
      )
    }
  }
}

object TrianglePipelineTraceVizGenerator {
  import TrianglePipelineThroughputTest._

  private case class Sample(
      cycle: Int,
      pixelsOut: Long,
      inFlight: Int,
      fbAccessInputStall: Boolean,
      fbAccessOutputFire: Boolean,
      prefetchColorFire: Boolean,
      prefetchColorStart: Long,
      prefetchColorEnd: Long,
      prefetchAuxFire: Boolean,
      prefetchAuxStart: Long,
      prefetchAuxEnd: Long,
      colorPendingOcc: Int,
      colorIssuedOcc: Int,
      auxPendingOcc: Int,
      auxIssuedOcc: Int,
      colorBufOcc: Int,
      auxBufOcc: Int,
      colorMemCmdFire: Boolean,
      colorMemCmdAddr: Long,
      colorMemCmdLen: Int,
      auxMemCmdFire: Boolean,
      auxMemCmdAddr: Long,
      auxMemCmdLen: Int,
      colorMemRspFire: Boolean,
      colorMemRspStall: Boolean,
      auxMemRspFire: Boolean,
      auxMemRspStall: Boolean,
      colorReadReqFire: Boolean,
      colorReadReqStall: Boolean,
      colorReadReqAddr: Long,
      colorReplayHit: Boolean,
      colorBufferHit: Boolean,
      auxReadReqFire: Boolean,
      auxReadReqStall: Boolean,
      auxReadReqAddr: Long,
      auxReplayHit: Boolean,
      auxBufferHit: Boolean
  )

  private def fixedCoord(v: Int): Int = v * 16

  private def initDut(dut: Harness): Unit = {
    dut.clockDomain.forkStimulus(10)
    dut.io.triangle.valid #= false
    dut.io.triangle.payload.ax #= 0
    dut.io.triangle.payload.ay #= 0
    dut.io.triangle.payload.bx #= 0
    dut.io.triangle.payload.by #= 0
    dut.io.triangle.payload.cx #= 0
    dut.io.triangle.payload.cy #= 0
    dut.io.triangle.payload.signBit #= false
    dut.clockDomain.waitSampling()
  }

  private def attachFramebufferMemory(dut: Harness): BmbMemoryAgent = {
    val memory = new BmbMemoryAgent(1 << dut.c.addressWidth.value)
    memory.addPort(dut.io.fbMemWrite, 0, dut.clockDomain, withDriver = true, withStall = false)
    memory.addPort(dut.io.fbColorReadMem, 0, dut.clockDomain, withDriver = true, withStall = false)
    memory.addPort(dut.io.fbAuxReadMem, 0, dut.clockDomain, withDriver = true, withStall = false)
    memory
  }

  private def attachTextureMemory(dut: Harness): BmbMemoryAgent = {
    val memory = new BmbMemoryAgent(1 << dut.c.addressWidth.value)
    memory.addPort(dut.io.texMem, 0, dut.clockDomain, withDriver = true, withStall = false)
    memory
  }

  private def sendTriangle(
      dut: Harness,
      ax: Int,
      ay: Int,
      bx: Int,
      by: Int,
      cx: Int,
      cy: Int,
      signBit: Boolean = false
  ): Unit = {
    dut.io.triangle.valid #= true
    dut.io.triangle.payload.ax #= fixedCoord(ax)
    dut.io.triangle.payload.ay #= fixedCoord(ay)
    dut.io.triangle.payload.bx #= fixedCoord(bx)
    dut.io.triangle.payload.by #= fixedCoord(by)
    dut.io.triangle.payload.cx #= fixedCoord(cx)
    dut.io.triangle.payload.cy #= fixedCoord(cy)
    dut.io.triangle.payload.signBit #= signBit
    dut.clockDomain.waitSamplingWhere(dut.io.triangleReady.toBoolean)
    dut.clockDomain.waitSampling()
    dut.io.triangle.valid #= false
  }

  private def sendRectangleAsTwoTriangles(dut: Harness): Unit = {
    sendTriangle(dut, 0, 0, 8, 0, 0, 8)
    sendTriangle(dut, 8, 0, 8, 8, 0, 8)
  }

  private def wordBase(value: Long): Long = value & ~0x3L

  private def bufferOccupancy(reader: FramebufferPlaneReader): Int =
    reader.bufferValid.map(_.toBoolean).count(identity)

  private def bufferHit(reader: FramebufferPlaneReader, reqAddr: Long): Boolean = {
    val base = wordBase(reqAddr)
    reader.bufferValid.zip(reader.bufferAddr).exists { case (valid, addr) =>
      valid.toBoolean && addr.toLong == base
    }
  }

  private def sampleReader(
      reader: FramebufferPlaneReader,
      prefetch: Stream[FramebufferPlaneReader.PrefetchReq],
      readReq: Stream[FramebufferPlaneBuffer.ReadReq],
      readRsp: Stream[FramebufferPlaneBuffer.ReadRsp],
      isColor: Boolean,
      pixelsOut: Long,
      inFlight: Int,
      fbAccessInputStall: Boolean,
      fbAccessOutputFire: Boolean,
      cycle: Int
  ): Sample = {
    val prefetchValid = prefetch.valid.toBoolean
    val prefetchFire = prefetch.valid.toBoolean && prefetch.ready.toBoolean
    val prefetchStart = if (prefetchValid) prefetch.startAddress.toLong else 0L
    val prefetchEnd = if (prefetchValid) prefetch.endAddress.toLong else 0L
    val readReqFire = readReq.valid.toBoolean && readReq.ready.toBoolean
    val readReqStall = readReq.valid.toBoolean && !readReq.ready.toBoolean
    val reqAddr = if (readReq.valid.toBoolean) readReq.address.toLong else 0L
    val replayHit = reader.replayValid.toBoolean && reader.replayAddr.toLong == wordBase(reqAddr)
    val bufHit = readReq.valid.toBoolean && bufferHit(reader, reqAddr)
    val memCmdValid = reader.io.mem.cmd.valid.toBoolean
    val memCmdFire = reader.io.mem.cmd.valid.toBoolean && reader.io.mem.cmd.ready.toBoolean
    val memCmdAddr = if (memCmdValid) reader.io.mem.cmd.fragment.address.toLong else 0L
    val memCmdLen = if (memCmdValid) reader.io.mem.cmd.fragment.length.toInt else 0
    val memRspFire = reader.io.mem.rsp.valid.toBoolean && reader.io.mem.rsp.ready.toBoolean
    val memRspStall = reader.io.mem.rsp.valid.toBoolean && !reader.io.mem.rsp.ready.toBoolean

    val common = (
      cycle,
      pixelsOut,
      inFlight,
      fbAccessInputStall,
      fbAccessOutputFire,
      reader.pendingSpanQueue.io.occupancy.toInt,
      reader.issuedSpanQueue.io.occupancy.toInt,
      bufferOccupancy(reader),
      memCmdFire,
      memCmdAddr,
      memCmdLen,
      memRspFire,
      memRspStall,
      readReqFire,
      readReqStall,
      reqAddr,
      replayHit,
      bufHit
    )

    if (isColor) {
      Sample(
        cycle = common._1,
        pixelsOut = common._2,
        inFlight = common._3,
        fbAccessInputStall = common._4,
        fbAccessOutputFire = common._5,
        prefetchColorFire = prefetchFire,
        prefetchColorStart = prefetchStart,
        prefetchColorEnd = prefetchEnd,
        prefetchAuxFire = false,
        prefetchAuxStart = 0,
        prefetchAuxEnd = 0,
        colorPendingOcc = common._6,
        colorIssuedOcc = common._7,
        auxPendingOcc = 0,
        auxIssuedOcc = 0,
        colorBufOcc = common._8,
        auxBufOcc = 0,
        colorMemCmdFire = common._9,
        colorMemCmdAddr = common._10,
        colorMemCmdLen = common._11,
        auxMemCmdFire = false,
        auxMemCmdAddr = 0,
        auxMemCmdLen = 0,
        colorMemRspFire = common._12,
        colorMemRspStall = common._13,
        auxMemRspFire = false,
        auxMemRspStall = false,
        colorReadReqFire = common._14,
        colorReadReqStall = common._15,
        colorReadReqAddr = common._16,
        colorReplayHit = common._17,
        colorBufferHit = common._18,
        auxReadReqFire = false,
        auxReadReqStall = false,
        auxReadReqAddr = 0,
        auxReplayHit = false,
        auxBufferHit = false
      )
    } else {
      Sample(
        cycle = common._1,
        pixelsOut = common._2,
        inFlight = common._3,
        fbAccessInputStall = common._4,
        fbAccessOutputFire = common._5,
        prefetchColorFire = false,
        prefetchColorStart = 0,
        prefetchColorEnd = 0,
        prefetchAuxFire = prefetchFire,
        prefetchAuxStart = prefetchStart,
        prefetchAuxEnd = prefetchEnd,
        colorPendingOcc = 0,
        colorIssuedOcc = 0,
        auxPendingOcc = common._6,
        auxIssuedOcc = common._7,
        colorBufOcc = 0,
        auxBufOcc = common._8,
        colorMemCmdFire = false,
        colorMemCmdAddr = 0,
        colorMemCmdLen = 0,
        auxMemCmdFire = common._9,
        auxMemCmdAddr = common._10,
        auxMemCmdLen = common._11,
        colorMemRspFire = false,
        colorMemRspStall = false,
        auxMemRspFire = common._12,
        auxMemRspStall = common._13,
        colorReadReqFire = false,
        colorReadReqStall = false,
        colorReadReqAddr = 0,
        colorReplayHit = false,
        colorBufferHit = false,
        auxReadReqFire = common._14,
        auxReadReqStall = common._15,
        auxReadReqAddr = common._16,
        auxReplayHit = common._17,
        auxBufferHit = common._18
      )
    }
  }

  private def merge(color: Sample, aux: Sample): Sample = {
    color.copy(
      prefetchAuxFire = aux.prefetchAuxFire,
      prefetchAuxStart = aux.prefetchAuxStart,
      prefetchAuxEnd = aux.prefetchAuxEnd,
      auxPendingOcc = aux.auxPendingOcc,
      auxIssuedOcc = aux.auxIssuedOcc,
      auxBufOcc = aux.auxBufOcc,
      auxMemCmdFire = aux.auxMemCmdFire,
      auxMemCmdAddr = aux.auxMemCmdAddr,
      auxMemCmdLen = aux.auxMemCmdLen,
      auxMemRspFire = aux.auxMemRspFire,
      auxMemRspStall = aux.auxMemRspStall,
      auxReadReqFire = aux.auxReadReqFire,
      auxReadReqStall = aux.auxReadReqStall,
      auxReadReqAddr = aux.auxReadReqAddr,
      auxReplayHit = aux.auxReplayHit,
      auxBufferHit = aux.auxBufferHit
    )
  }

  private def jsonString(value: String): String =
    value
      .flatMap {
        case '\\' => "\\\\"
        case '"'  => "\\\""
        case '\n' => "\\n"
        case '\r' => "\\r"
        case '\t' => "\\t"
        case c    => c.toString
      }

  private def toJson(samples: Seq[Sample], explanation: String): String = {
    val items = samples
      .map { s =>
        s"""{"cycle":${s.cycle},"pixelsOut":${s.pixelsOut},"inFlight":${s.inFlight},"fbAccessInputStall":${s.fbAccessInputStall},"fbAccessOutputFire":${s.fbAccessOutputFire},"prefetchColorFire":${s.prefetchColorFire},"prefetchColorStart":${s.prefetchColorStart},"prefetchColorEnd":${s.prefetchColorEnd},"prefetchAuxFire":${s.prefetchAuxFire},"prefetchAuxStart":${s.prefetchAuxStart},"prefetchAuxEnd":${s.prefetchAuxEnd},"colorPendingOcc":${s.colorPendingOcc},"colorIssuedOcc":${s.colorIssuedOcc},"auxPendingOcc":${s.auxPendingOcc},"auxIssuedOcc":${s.auxIssuedOcc},"colorBufOcc":${s.colorBufOcc},"auxBufOcc":${s.auxBufOcc},"colorMemCmdFire":${s.colorMemCmdFire},"colorMemCmdAddr":${s.colorMemCmdAddr},"colorMemCmdLen":${s.colorMemCmdLen},"auxMemCmdFire":${s.auxMemCmdFire},"auxMemCmdAddr":${s.auxMemCmdAddr},"auxMemCmdLen":${s.auxMemCmdLen},"colorMemRspFire":${s.colorMemRspFire},"colorMemRspStall":${s.colorMemRspStall},"auxMemRspFire":${s.auxMemRspFire},"auxMemRspStall":${s.auxMemRspStall},"colorReadReqFire":${s.colorReadReqFire},"colorReadReqStall":${s.colorReadReqStall},"colorReadReqAddr":${s.colorReadReqAddr},"colorReplayHit":${s.colorReplayHit},"colorBufferHit":${s.colorBufferHit},"auxReadReqFire":${s.auxReadReqFire},"auxReadReqStall":${s.auxReadReqStall},"auxReadReqAddr":${s.auxReadReqAddr},"auxReplayHit":${s.auxReplayHit},"auxBufferHit":${s.auxBufferHit}}"""
      }
      .mkString("[", ",", "]")
    "{\"explanation\":\"" + jsonString(explanation) + "\",\"samples\":" + items + "}"
  }

  private def buildHtml(dataJson: String, defaultCycle: Int): String =
    """<!doctype html>
<html lang="en">
<meta charset="utf-8">
<title>Triangle Pipeline Framebuffer Trace</title>
<style>
body { font-family: ui-sans-serif, system-ui, sans-serif; margin: 0; background: #0b1020; color: #e8edf8; }
.wrap { max-width: 1400px; margin: 0 auto; padding: 24px; }
h1, h2 { margin: 0 0 12px; }
.panel { background: #121936; border: 1px solid #26315e; border-radius: 12px; padding: 16px; margin: 16px 0; }
.grid { display: grid; grid-template-columns: 220px 1fr; gap: 8px 16px; align-items: center; }
.mono { font-family: ui-monospace, SFMono-Regular, monospace; }
.legend { display: flex; flex-wrap: wrap; gap: 10px 18px; margin-top: 8px; }
.swatch { display: inline-flex; align-items: center; gap: 8px; }
.box { width: 14px; height: 14px; border-radius: 3px; display: inline-block; }
canvas { width: 100%; height: 320px; background: #0a0f24; border-radius: 10px; border: 1px solid #243057; }
input[type=range] { width: 100%; }
table { width: 100%; border-collapse: collapse; font-size: 12px; }
th, td { padding: 6px 8px; border-bottom: 1px solid #243057; text-align: left; }
tr.focus { background: #1b2653; }
.pill { display: inline-block; border-radius: 999px; padding: 2px 8px; font-size: 11px; margin-right: 6px; }
.good { background: #123b2a; color: #8ff0b5; }
.warn { background: #493710; color: #ffd98c; }
.bad { background: #4c1f26; color: #ffb3bf; }
</style>
<div class="wrap">
  <h1>Framebuffer Read Trace Viewer</h1>
  <div class="panel">
    <div id="explanation"></div>
  </div>
  <div class="panel">
    <h2>Cycle Focus</h2>
    <input id="cycle" type="range" min="0" max="0" value="__DEFAULT__">
    <div class="grid mono" id="summary"></div>
  </div>
  <div class="panel">
    <h2>Overview</h2>
    <canvas id="overview" width="1320" height="320"></canvas>
    <div class="legend mono">
      <span class="swatch"><span class="box" style="background:#69d2e7"></span>Color buffer occupancy</span>
      <span class="swatch"><span class="box" style="background:#f38630"></span>Aux buffer occupancy</span>
      <span class="swatch"><span class="box" style="background:#f9d423"></span>In-flight pixels</span>
      <span class="swatch"><span class="box" style="background:#ff4e50"></span>FB access input stall</span>
      <span class="swatch"><span class="box" style="background:#8a9b0f"></span>Reader rsp throttled</span>
    </div>
  </div>
  <div class="panel">
    <h2>Window Around Selected Cycle</h2>
    <table id="window"></table>
  </div>
</div>
<script>
const data = __DATA__;
const samples = data.samples;
const cycleInput = document.getElementById('cycle');
const summary = document.getElementById('summary');
const windowTable = document.getElementById('window');
const explanation = document.getElementById('explanation');
cycleInput.max = samples.length - 1;
cycleInput.value = Math.min(__DEFAULT__, samples.length - 1);
explanation.innerHTML = '<p>' + data.explanation.replace(/\\n/g, '</p><p>') + '</p>';

function hex(v){ return '0x' + Number(v >>> 0 === v ? v : v).toString(16); }
function pill(label, cls){ return '<span class="pill ' + cls + '">' + label + '</span>'; }
function fmtHit(s, p){ return s[p + 'ReplayHit'] ? pill('replay', 'good') : (s[p + 'BufferHit'] ? pill('buffer', 'good') : pill('miss/stall', 'bad')); }

function renderSummary(idx){
  const s = samples[idx];
  summary.innerHTML = `
    <div>Cycle</div><div>${s.cycle}</div>
    <div>Pixels out</div><div>${s.pixelsOut}</div>
    <div>In-flight pixels</div><div>${s.inFlight}</div>
    <div>FB access</div><div>${s.fbAccessInputStall ? pill('input stalled','bad') : pill('accepting','good')} ${s.fbAccessOutputFire ? pill('output fire','good') : ''}</div>
    <div>Color prefetch</div><div>${s.prefetchColorFire ? pill('fire','good') : pill('idle','warn')} ${hex(s.prefetchColorStart)}..${hex(s.prefetchColorEnd)}</div>
    <div>Aux prefetch</div><div>${s.prefetchAuxFire ? pill('fire','good') : pill('idle','warn')} ${hex(s.prefetchAuxStart)}..${hex(s.prefetchAuxEnd)}</div>
    <div>Color reader</div><div>pending=${s.colorPendingOcc} issued=${s.colorIssuedOcc} buf=${s.colorBufOcc}</div>
    <div>Aux reader</div><div>pending=${s.auxPendingOcc} issued=${s.auxIssuedOcc} buf=${s.auxBufOcc}</div>
    <div>Color mem</div><div>${s.colorMemCmdFire ? pill('cmd fire','good') : ''} ${s.colorMemRspFire ? pill('rsp fire','good') : ''} ${s.colorMemRspStall ? pill('rsp throttled','bad') : ''} addr=${hex(s.colorMemCmdAddr)} len=${s.colorMemCmdLen}</div>
    <div>Aux mem</div><div>${s.auxMemCmdFire ? pill('cmd fire','good') : ''} ${s.auxMemRspFire ? pill('rsp fire','good') : ''} ${s.auxMemRspStall ? pill('rsp throttled','bad') : ''} addr=${hex(s.auxMemCmdAddr)} len=${s.auxMemCmdLen}</div>
    <div>Color consume</div><div>${s.colorReadReqFire ? pill('req fire','good') : (s.colorReadReqStall ? pill('req stalled','bad') : pill('idle','warn'))} ${hex(s.colorReadReqAddr)} ${fmtHit(s,'color')}</div>
    <div>Aux consume</div><div>${s.auxReadReqFire ? pill('req fire','good') : (s.auxReadReqStall ? pill('req stalled','bad') : pill('idle','warn'))} ${hex(s.auxReadReqAddr)} ${fmtHit(s,'aux')}</div>
  `;
}

function renderWindow(idx){
  const start = Math.max(0, idx - 12);
  const end = Math.min(samples.length - 1, idx + 12);
  let html = '<tr><th>cycle</th><th>prefetch</th><th>reader occ</th><th>mem</th><th>consume</th><th>fbAccess</th></tr>';
  for(let i=start;i<=end;i++){
    const s = samples[i];
    html += `<tr class="${i===idx?'focus':''}">
      <td class="mono">${s.cycle}</td>
      <td class="mono">${s.prefetchColorFire?'C '+hex(s.prefetchColorStart)+'..'+hex(s.prefetchColorEnd):''} ${s.prefetchAuxFire?'A '+hex(s.prefetchAuxStart)+'..'+hex(s.prefetchAuxEnd):''}</td>
      <td class="mono">C p${s.colorPendingOcc}/i${s.colorIssuedOcc}/b${s.colorBufOcc} | A p${s.auxPendingOcc}/i${s.auxIssuedOcc}/b${s.auxBufOcc}</td>
      <td class="mono">${s.colorMemCmdFire?'Ccmd ':''}${s.colorMemRspFire?'Crsp ':''}${s.colorMemRspStall?'Cstall ':''}${s.auxMemCmdFire?'Acmd ':''}${s.auxMemRspFire?'Arsp ':''}${s.auxMemRspStall?'Astall ':''}</td>
      <td class="mono">${s.colorReadReqFire?'Cfire ':''}${s.colorReadReqStall?'Cstall ':''}${s.colorReplayHit?'Creplay ':''}${s.colorBufferHit?'Cbuf ':''}${s.auxReadReqFire?'Afire ':''}${s.auxReadReqStall?'Astall ':''}${s.auxReplayHit?'Areplay ':''}${s.auxBufferHit?'Abuf ':''}</td>
      <td class="mono">${s.fbAccessInputStall?'stall ':''}${s.fbAccessOutputFire?'out ':''}inFlight=${s.inFlight}</td>
    </tr>`;
  }
  windowTable.innerHTML = html;
}

function renderOverview(idx){
  const canvas = document.getElementById('overview');
  const ctx = canvas.getContext('2d');
  const w = canvas.width, h = canvas.height;
  ctx.clearRect(0,0,w,h);
  ctx.fillStyle = '#0a0f24'; ctx.fillRect(0,0,w,h);
  const maxOcc = Math.max(...samples.map(s => Math.max(s.colorBufOcc, s.auxBufOcc, s.inFlight)), 1);
  const xFor = i => i * (w - 1) / Math.max(samples.length - 1, 1);
  const yFor = v => h - 24 - (v / maxOcc) * (h - 40);
  function line(color, getter){
    ctx.beginPath(); ctx.strokeStyle = color; ctx.lineWidth = 2;
    samples.forEach((s,i)=>{ const x=xFor(i), y=yFor(getter(s)); if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y); });
    ctx.stroke();
  }
  line('#69d2e7', s => s.colorBufOcc);
  line('#f38630', s => s.auxBufOcc);
  line('#f9d423', s => s.inFlight);
  samples.forEach((s,i)=>{
    const x = xFor(i);
    if(s.fbAccessInputStall){ ctx.fillStyle='#ff4e50'; ctx.fillRect(x, h-18, 2, 8); }
    if(s.colorMemRspStall || s.auxMemRspStall){ ctx.fillStyle='#8a9b0f'; ctx.fillRect(x, h-28, 2, 8); }
  });
  const x = xFor(idx);
  ctx.strokeStyle = '#ffffff'; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,h); ctx.stroke();
}

function update(){
  const idx = Number(cycleInput.value);
  renderSummary(idx);
  renderWindow(idx);
  renderOverview(idx);
}
cycleInput.addEventListener('input', update);
update();
</script>
</html>
""".replace("__DATA__", dataJson).replace("__DEFAULT__", defaultCycle.toString)

  def generate(): Unit = {
    val config =
      Config
        .voodoo1()
        .copy(
          addressWidth = 20 bits,
          memBurstLengthWidth = 6,
          fbWriteBufferLineWords = 16,
          fbWriteBufferCount = 2,
          texFillLineWords = 8,
          useFbWriteBuffer = true,
          useTexFillCache = true,
          texFillCacheSlots = 16,
          texFillRequestWindow = 16,
          trace = TraceConfig()
        )
    val traceCycles = 2600
    val outPath =
      Paths.get("/home/fal/voodoo/SpinalVoodoo_span-refactor/artifacts/framebuffer-trace-view.html")
    Files.createDirectories(outPath.getParent)

    SimConfig.withIVerilog.compile(Harness(config, 8, 8)).doSim { dut =>
      initDut(dut)
      attachFramebufferMemory(dut)
      attachTextureMemory(dut)

      fork {
        for (_ <- 0 until 256) {
          sendRectangleAsTwoTriangles(dut)
        }
      }

      val samples = collection.mutable.ArrayBuffer.empty[Sample]
      for (cycle <- 0 until traceCycles) {
        dut.clockDomain.waitSampling()

        val fbAccess = dut.pipeline.fbAccess
        val colorReader = dut.framebuffer.colorReaderCached
        val auxReader = dut.framebuffer.auxReaderCached
        val pixelsOut = dut.io.stats.pixelsOut.toLong
        val inFlight = fbAccess.inFlightCount.toInt
        val fbAccessInputStall =
          fbAccess.io.input.valid.toBoolean && !fbAccess.io.input.ready.toBoolean
        val fbAccessOutputFire =
          fbAccess.io.output.valid.toBoolean && fbAccess.io.output.ready.toBoolean

        val color = sampleReader(
          colorReader,
          colorReader.io.prefetchReq,
          colorReader.io.readReq,
          colorReader.io.readRsp,
          isColor = true,
          pixelsOut = pixelsOut,
          inFlight = inFlight,
          fbAccessInputStall = fbAccessInputStall,
          fbAccessOutputFire = fbAccessOutputFire,
          cycle = cycle
        )
        val aux = sampleReader(
          auxReader,
          auxReader.io.prefetchReq,
          auxReader.io.readReq,
          auxReader.io.readRsp,
          isColor = false,
          pixelsOut = pixelsOut,
          inFlight = inFlight,
          fbAccessInputStall = fbAccessInputStall,
          fbAccessOutputFire = fbAccessOutputFire,
          cycle = cycle
        )
        samples += merge(color, aux)
      }

      val maxColorBuf = samples.map(_.colorBufOcc).max
      val maxAuxBuf = samples.map(_.auxBufOcc).max
      val colorRspStallCycles = samples.count(_.colorMemRspStall)
      val auxRspStallCycles = samples.count(_.auxMemRspStall)
      val fbAccessStallCycles = samples.count(_.fbAccessInputStall)
      val explanation =
        s"This view is built from a real run of the current throughput harness for $traceCycles cycles. " +
          s"Look first at the color/aux buffer occupancy traces, then the red stall marks (FramebufferAccess input stalled), then the green marks (reader rsp throttled). " +
          s"In this run the color reader reached $maxColorBuf buffered words, aux reached $maxAuxBuf, color rsp throttled for $colorRspStallCycles cycles, aux rsp throttled for $auxRspStallCycles cycles, and FramebufferAccess input stalled for $fbAccessStallCycles cycles. " +
          s"The benchmark also uses pixelStride=8, so Y does not advance the tiled framebuffer address in this setup; repeated start addresses across rows are therefore expected here. " +
          s"What to look for: when buffer occupancy is high and rsp throttling appears, later read requests stop finding immediate hits and FramebufferAccess stalls."

      val html = buildHtml(toJson(samples.toSeq, explanation), defaultCycle = 2000)
      Files.write(outPath, html.getBytes(StandardCharsets.UTF_8))
      println(s"Wrote visualization to $outPath")
    }
  }

  def main(args: Array[String]): Unit = generate()
}

class TrianglePipelineTraceVizTest extends AnyFunSuite {
  test("generate framebuffer trace visualization") {
    TrianglePipelineTraceVizGenerator.generate()
  }
}
