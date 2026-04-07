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
      val pointSample = in Bool ()
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

    val textureMode = Bits(32 bits)
    textureMode := B(Tmu.TextureFormat.RGB565 << 8, 32 bits)
    when(!io.pointSample) {
      textureMode(6) := True
      textureMode(7) := True
    }
    triangleConfig.tmuTextureMode := textureMode
    triangleConfig.tmuTexBaseAddr := 0
    triangleConfig.tmuTLOD := B(0x00200, 27 bits)
    triangleConfig.tmudSdX := 0.0
    triangleConfig.tmudTdX := 0.0
    triangleConfig.tmudSdY := 0.0
    triangleConfig.tmudTdY := 0.0

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
    controls.syncRender.fbzMode.enableClipping := True
    controls.syncRender.fbzMode.enableChromaKey := True
    controls.syncRender.fbzMode.enableDepthBuffer := True
    controls.syncRender.fbzMode.depthFunction := 7
    controls.syncRender.fbzMode.enableDithering := True
    controls.syncRender.fbzMode.rgbBufferMask := True
    controls.syncRender.fbzMode.auxBufferMask := True
    controls.syncRender.fbzMode.ditherAlgorithm := False
    controls.syncRender.fbzMode.drawBuffer := 0
    controls.syncRender.fbzMode.enableAlphaPlanes := True
    controls.syncRender.fbzMode.enableDitherSubtract := True
    controls.syncRender.color0 := B(0x80406020L, 32 bits)
    controls.syncRender.color1 := B(0xc0907040L, 32 bits)
    controls.syncRender.fogColor := B(0x00304050L, 32 bits)
    controls.syncRender.chromaKey := B(0x00010203L, 32 bits)
    controls.syncRender.zaColor := B(0x00004080L, 32 bits)
    controls.syncRender.routing.colorBaseAddr := colorBase
    controls.syncRender.routing.auxBaseAddr := auxBase
    controls.syncRender.routing.pixelStride := rectWidth
    controls.syncRender.tmuSendConfig := False
    controls.fastfill.fbzMode.enableClipping := True
    controls.fastfill.routing.colorBaseAddr := colorBase
    controls.fastfill.routing.auxBaseAddr := auxBase
    controls.fastfill.routing.pixelStride := rectWidth

    val externalBusy = PixelPipeline.ExternalBusy()
    zeroBundle(externalBusy)

    val pipeline = PixelPipeline(c)
    val framebuffer = FramebufferMemSubsystem(c)
    framebuffer.io.colorReadRsp.valid.simPublic()
    framebuffer.io.colorReadRsp.ready.simPublic()
    framebuffer.io.auxReadRsp.valid.simPublic()
    framebuffer.io.auxReadRsp.ready.simPublic()

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
    pipeline.io.paletteWrite.valid := False
    pipeline.io.paletteWrite.payload.address := 0
    pipeline.io.paletteWrite.payload.data := 0

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

  private def mkConfig(useFbWriteBuffer: Boolean = true, useFbReadCache: Boolean = false) =
    Config
      .voodoo1()
      .copy(
        addressWidth = 20 bits,
        memBurstLengthWidth = 6,
        fbWriteBufferLineWords = 64,
        fbWriteBufferCount = 2,
        texFillLineWords = 8,
        useFbWriteBuffer = useFbWriteBuffer,
        useFbReadCache = useFbReadCache,
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
    dut.io.pointSample #= false
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
      totalPixelsOut: Long,
      fbAccessInputStallCycles: Long,
      ditherJoinStallCycles: Long,
      colorWriteBackpressureCycles: Long,
      auxWriteBackpressureCycles: Long,
      colorForkWaitCycles: Long,
      auxForkWaitCycles: Long,
      colorReadFireCycles: Long,
      auxReadFireCycles: Long,
      colorReadBackpressureCycles: Long,
      auxReadBackpressureCycles: Long,
      colorCtlBackpressureCycles: Long,
      passMetaBackpressureCycles: Long,
      dualReadFireCycles: Long,
      fbAccessOutputFireCycles: Long,
      fbAccessOutputBackpressureCycles: Long,
      preDitherMergedBackpressureCycles: Long,
      colorReadRspFireCycles: Long,
      auxReadRspFireCycles: Long,
      maxFbAccessInFlight: Long
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
    var fbAccessInputStallCycles = 0L
    var ditherJoinStallCycles = 0L
    var colorWriteBackpressureCycles = 0L
    var auxWriteBackpressureCycles = 0L
    var colorForkWaitCycles = 0L
    var auxForkWaitCycles = 0L
    var colorReadFireCycles = 0L
    var auxReadFireCycles = 0L
    var colorReadBackpressureCycles = 0L
    var auxReadBackpressureCycles = 0L
    var colorCtlBackpressureCycles = 0L
    var passMetaBackpressureCycles = 0L
    var dualReadFireCycles = 0L
    var fbAccessOutputFireCycles = 0L
    var fbAccessOutputBackpressureCycles = 0L
    var preDitherMergedBackpressureCycles = 0L
    var colorReadRspFireCycles = 0L
    var auxReadRspFireCycles = 0L
    var maxFbAccessInFlight = 0L
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

      if (warmupStartCycle >= 0 && measuredEndCycle < 0) {
        if (
          dut.io.debug.busy.fbAccessInputValid.toBoolean && !dut.pipeline.fbAccess.io.input.ready.toBoolean
        ) {
          fbAccessInputStallCycles += 1
        }
        if (
          dut.io.debug.busy.ditherJoinedValid.toBoolean && !dut.io.debug.busy.ditherJoinedReady.toBoolean
        ) {
          ditherJoinStallCycles += 1
        }
        if (
          dut.io.debug.writePath.colorWriteFbValid.toBoolean && !dut.io.debug.writePath.colorWriteFbReady.toBoolean
        ) {
          colorWriteBackpressureCycles += 1
        }
        if (
          dut.io.debug.writePath.auxWriteFbValid.toBoolean && !dut.io.debug.writePath.auxWriteFbReady.toBoolean
        ) {
          auxWriteBackpressureCycles += 1
        }
        if (
          dut.io.debug.busy.colorForkValid.toBoolean && !dut.io.debug.busy.colorForkReady.toBoolean
        ) {
          colorForkWaitCycles += 1
        }
        if (dut.io.debug.busy.auxForkValid.toBoolean && !dut.io.debug.busy.auxForkReady.toBoolean) {
          auxForkWaitCycles += 1
        }
        val colorReadFire =
          dut.pipeline.fbAccess.io.fbReadColorReq.valid.toBoolean && dut.pipeline.fbAccess.io.fbReadColorReq.ready.toBoolean
        val auxReadFire =
          dut.pipeline.fbAccess.io.fbReadAuxReq.valid.toBoolean && dut.pipeline.fbAccess.io.fbReadAuxReq.ready.toBoolean
        if (colorReadFire) {
          colorReadFireCycles += 1
        }
        if (auxReadFire) {
          auxReadFireCycles += 1
        }
        if (
          dut.pipeline.fbAccess.io.fbReadColorReq.valid.toBoolean && !dut.pipeline.fbAccess.io.fbReadColorReq.ready.toBoolean
        ) {
          colorReadBackpressureCycles += 1
        }
        if (
          dut.pipeline.fbAccess.io.fbReadAuxReq.valid.toBoolean && !dut.pipeline.fbAccess.io.fbReadAuxReq.ready.toBoolean
        ) {
          auxReadBackpressureCycles += 1
        }
        if (
          dut.pipeline.fbAccess.colorCtlBase.valid.toBoolean && !dut.pipeline.fbAccess.colorCtlBase.ready.toBoolean
        ) {
          colorCtlBackpressureCycles += 1
        }
        if (
          dut.pipeline.fbAccess.passMeta.valid.toBoolean && !dut.pipeline.fbAccess.passMeta.ready.toBoolean
        ) {
          passMetaBackpressureCycles += 1
        }
        if (colorReadFire && auxReadFire) {
          dualReadFireCycles += 1
        }
        if (
          dut.pipeline.fbAccess.io.output.valid.toBoolean && dut.pipeline.fbAccess.io.output.ready.toBoolean
        ) {
          fbAccessOutputFireCycles += 1
        }
        if (
          dut.pipeline.fbAccess.io.output.valid.toBoolean && !dut.pipeline.fbAccess.io.output.ready.toBoolean
        ) {
          fbAccessOutputBackpressureCycles += 1
        }
        if (
          dut.io.debug.busy.preDitherMergedValid.toBoolean && !dut.io.debug.busy.preDitherMergedReady.toBoolean
        ) {
          preDitherMergedBackpressureCycles += 1
        }
        if (
          dut.framebuffer.io.colorReadRsp.valid.toBoolean && dut.framebuffer.io.colorReadRsp.ready.toBoolean
        ) {
          colorReadRspFireCycles += 1
        }
        if (
          dut.framebuffer.io.auxReadRsp.valid.toBoolean && dut.framebuffer.io.auxReadRsp.ready.toBoolean
        ) {
          auxReadRspFireCycles += 1
        }
        maxFbAccessInFlight = maxFbAccessInFlight.max(dut.pipeline.fbAccess.inFlightCount.toLong)
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
      totalPixelsOut = totalPixelsOut,
      fbAccessInputStallCycles = fbAccessInputStallCycles,
      ditherJoinStallCycles = ditherJoinStallCycles,
      colorWriteBackpressureCycles = colorWriteBackpressureCycles,
      auxWriteBackpressureCycles = auxWriteBackpressureCycles,
      colorForkWaitCycles = colorForkWaitCycles,
      auxForkWaitCycles = auxForkWaitCycles,
      colorReadFireCycles = colorReadFireCycles,
      auxReadFireCycles = auxReadFireCycles,
      colorReadBackpressureCycles = colorReadBackpressureCycles,
      auxReadBackpressureCycles = auxReadBackpressureCycles,
      colorCtlBackpressureCycles = colorCtlBackpressureCycles,
      passMetaBackpressureCycles = passMetaBackpressureCycles,
      dualReadFireCycles = dualReadFireCycles,
      fbAccessOutputFireCycles = fbAccessOutputFireCycles,
      fbAccessOutputBackpressureCycles = fbAccessOutputBackpressureCycles,
      preDitherMergedBackpressureCycles = preDitherMergedBackpressureCycles,
      colorReadRspFireCycles = colorReadRspFireCycles,
      auxReadRspFireCycles = auxReadRspFireCycles,
      maxFbAccessInFlight = maxFbAccessInFlight
    )
  }

  private def runThroughputTest(
      config: Config,
      maxCyclesPerPixel: Double,
      pointSample: Boolean = false
  ): Unit = {
    SimConfig.withVerilator.compile(Harness(config, rectWidth, rectHeight)).doSim { dut =>
      initDut(dut)
      dut.io.pointSample #= pointSample
      attachFramebufferMemory(dut)
      attachTextureMemory(dut)

      val measured = measureOutputThroughput(dut, measuredPairs)
      println(
        f"Triangle pipeline output throughput after $warmupPixels%d warmup pixels: ${measured.measuredWindowPixels}%d pixels over ${measured.measuredWindowCycles}%d cycles = ${measured.pixelsPerCycle}%.3f px/cycle (${measured.cyclesPerPixel}%.3f cycles/px)"
      )
      println(
        s"Measured-window stalls: fbAccessInput=${measured.fbAccessInputStallCycles} ditherJoin=${measured.ditherJoinStallCycles} colorWriteBp=${measured.colorWriteBackpressureCycles} auxWriteBp=${measured.auxWriteBackpressureCycles} colorForkWait=${measured.colorForkWaitCycles} auxForkWait=${measured.auxForkWaitCycles} colorReadFire=${measured.colorReadFireCycles} auxReadFire=${measured.auxReadFireCycles} colorReadBp=${measured.colorReadBackpressureCycles} auxReadBp=${measured.auxReadBackpressureCycles} colorCtlBp=${measured.colorCtlBackpressureCycles} passMetaBp=${measured.passMetaBackpressureCycles} dualReadFire=${measured.dualReadFireCycles} colorReadRspFire=${measured.colorReadRspFireCycles} auxReadRspFire=${measured.auxReadRspFireCycles} fbAccessOutputFire=${measured.fbAccessOutputFireCycles} fbAccessOutputBp=${measured.fbAccessOutputBackpressureCycles} preDitherMergedBp=${measured.preDitherMergedBackpressureCycles} maxFbAccessInFlight=${measured.maxFbAccessInFlight}"
      )
      println(
        s"Framebuffer stats: fillHits=${dut.io.fbStats.fillHits.toLong} fillMisses=${dut.io.fbStats.fillMisses.toLong} fillBurstCount=${dut.io.fbStats.fillBurstCount.toLong} fillBurstBeats=${dut.io.fbStats.fillBurstBeats.toLong} fillStallCycles=${dut.io.fbStats.fillStallCycles.toLong} writeStallCycles=${dut.io.fbStats.writeStallCycles.toLong} writeDrainCount=${dut.io.fbStats.writeDrainCount.toLong} writeFullDrainCount=${dut.io.fbStats.writeFullDrainCount.toLong} writePartialDrainCount=${dut.io.fbStats.writePartialDrainCount.toLong} writeDrainDirtyWordTotal=${dut.io.fbStats.writeDrainDirtyWordTotal.toLong} memColorWriteCmdCount=${dut.io.fbStats.memColorWriteCmdCount.toLong} memAuxWriteCmdCount=${dut.io.fbStats.memAuxWriteCmdCount.toLong} memColorReadCmdCount=${dut.io.fbStats.memColorReadCmdCount.toLong} memAuxReadCmdCount=${dut.io.fbStats.memAuxReadCmdCount.toLong} memColorWriteBlockedCycles=${dut.io.fbStats.memColorWriteBlockedCycles.toLong} memAuxWriteBlockedCycles=${dut.io.fbStats.memAuxWriteBlockedCycles.toLong} memColorReadBlockedCycles=${dut.io.fbStats.memColorReadBlockedCycles.toLong} memAuxReadBlockedCycles=${dut.io.fbStats.memAuxReadBlockedCycles.toLong} readReqCount=${dut.io.fbStats.readReqCount.toLong} readReqSameWordCount=${dut.io.fbStats.readReqSameWordCount.toLong} readReqSameLineCount=${dut.io.fbStats.readReqSameLineCount.toLong} readReqOtherCount=${dut.io.fbStats.readReqOtherCount.toLong} readSingleBeatBurstCount=${dut.io.fbStats.readSingleBeatBurstCount.toLong} readMultiBeatBurstCount=${dut.io.fbStats.readMultiBeatBurstCount.toLong} readMaxQueueOccupancy=${dut.io.fbStats.readMaxQueueOccupancy.toLong}"
      )
      assert(
        measured.cyclesPerPixel <= maxCyclesPerPixel,
        f"expected the warmed output-side throughput to stay below $maxCyclesPerPixel%.1f cycles/pixel, got ${measured.cyclesPerPixel}%.3f"
      )
    }
  }

  test("triangle pipeline reaches near-full throughput with full feature path enabled") {
    val config = mkConfig()

    runThroughputTest(config, maxCyclesPerPixel = 5.0)
  }

  test("triangle pipeline throughput without framebuffer write buffer") {
    val config = mkConfig(useFbWriteBuffer = false)

    runThroughputTest(config, maxCyclesPerPixel = 20.0)
  }

  test("triangle pipeline throughput with point sampling") {
    val config = mkConfig()

    runThroughputTest(config, maxCyclesPerPixel = 5.0, pointSample = true)
  }

  test("triangle pipeline throughput with framebuffer read cache and write buffer") {
    val config = mkConfig(useFbWriteBuffer = true, useFbReadCache = true)

    runThroughputTest(config, maxCyclesPerPixel = 5.0)
  }
}

object TrianglePipelineTraceVizGenerator {
  def generate(): Unit = {
    throw new UnsupportedOperationException(
      "TrianglePipelineTraceVizGenerator is stale on the rebased tree and needs its reader debug hooks updated before use"
    )
  }

  def main(args: Array[String]): Unit = generate()
}

class TrianglePipelineTraceVizTest extends AnyFunSuite {
  ignore("generate framebuffer trace visualization") {
    TrianglePipelineTraceVizGenerator.generate()
  }
}
