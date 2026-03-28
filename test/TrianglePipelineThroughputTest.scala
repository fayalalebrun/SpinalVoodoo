//> using target.scope test

package voodoo

import org.scalatest.funsuite.AnyFunSuite
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
        fbWriteBufferLineWords = 16,
        fbWriteBufferCount = 2,
        texFillLineWords = 8,
        useFbWriteBuffer = true,
        useTexFillCache = true,
        texFillCacheSlots = 16,
        texFillRequestWindow = 16,
        trace = TraceConfig()
      )

  private val rectWidth = 8
  private val rectHeight = 8
  private val fixedScale = 16
  private val measuredPairs = 64

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

  private case class PassMetrics(
      pixelsOut: Long,
      pixelsIn: Long,
      chromaFail: Long,
      zFail: Long,
      alphaFail: Long,
      fillMisses: Long,
      activeCycles: Int
  )

  private def runPass(dut: Harness, pairCount: Int): PassMetrics = {
    val startPixelsOut = dut.io.stats.pixelsOut.toLong
    val startPixelsIn = dut.io.stats.pixelsIn.toLong
    val startChromaFail = dut.io.stats.chromaFail.toLong
    val startZFail = dut.io.stats.zFuncFail.toLong
    val startAlphaFail = dut.io.stats.aFuncFail.toLong
    val startFillMisses = dut.io.fbStats.fillMisses.toLong

    for (_ <- 0 until pairCount) {
      sendRectangleAsTwoTriangles(dut)
    }

    var seenBusy = false
    var activeCycles = 0
    var guard = 0
    val timeout = rectWidth * rectHeight * pairCount * 16
    while ((!seenBusy || dut.io.activeBusy.toBoolean) && guard < timeout) {
      if (dut.io.activeBusy.toBoolean) {
        seenBusy = true
        activeCycles += 1
      }
      dut.clockDomain.waitSampling()
      guard += 1
    }

    assert(seenBusy, "expected pipeline activity during triangle pass")
    assert(
      !dut.io.activeBusy.toBoolean,
      s"pipeline active work should drain within $timeout cycles; activeBusy=${dut.io.activeBusy.toBoolean} busy flags: ts=${dut.io.debug.busy.triangleSetupValid.toBoolean} rast=${dut.io.debug.busy.rasterizerRunning.toBoolean} tmuIn=${dut.io.debug.busy.tmuInputValid.toBoolean} tmu=${dut.io.debug.busy.tmuBusy.toBoolean} fb=${dut.io.debug.busy.fbAccessBusy.toBoolean} fog=${dut.io.debug.busy.fogBusy.toBoolean} wc=${dut.io.debug.busy.writeColorInputValid.toBoolean} wa=${dut.io.debug.busy.writeAuxInputValid.toBoolean} fbColor=${dut.io.fbStatus.colorBusy.toBoolean} fbAux=${dut.io.fbStatus.auxBusy.toBoolean} fillMisses=${dut.io.fbStats.fillMisses.toLong} pixelsOut=${dut.io.stats.pixelsOut.toLong}"
    )

    PassMetrics(
      pixelsOut = dut.io.stats.pixelsOut.toLong - startPixelsOut,
      pixelsIn = dut.io.stats.pixelsIn.toLong - startPixelsIn,
      chromaFail = dut.io.stats.chromaFail.toLong - startChromaFail,
      zFail = dut.io.stats.zFuncFail.toLong - startZFail,
      alphaFail = dut.io.stats.aFuncFail.toLong - startAlphaFail,
      fillMisses = dut.io.fbStats.fillMisses.toLong - startFillMisses,
      activeCycles = activeCycles
    )
  }

  test("triangle pipeline reaches near-full throughput with full feature path enabled") {
    val config = mkConfig

    SimConfig.withIVerilog.compile(Harness(config, rectWidth, rectHeight)).doSim { dut =>
      initDut(dut)
      attachFramebufferMemory(dut)
      attachTextureMemory(dut)

      val warmPass = runPass(dut, 1)
      val measuredPass = runPass(dut, measuredPairs)

      assert(warmPass.pixelsOut > 0, "expected warm-up pass to produce pixels")
      assert(
        (measuredPass.pixelsOut - warmPass.pixelsOut * measuredPairs).abs <= measuredPairs,
        s"expected stable per-pair pixel count between warm and measured passes, saw ${warmPass.pixelsOut} then ${measuredPass.pixelsOut} over $measuredPairs pairs"
      )
      assert(
        measuredPass.pixelsIn == measuredPass.pixelsOut,
        s"expected all pixels to survive the enabled feature path, saw in=${measuredPass.pixelsIn} out=${measuredPass.pixelsOut}"
      )
      assert(
        measuredPass.chromaFail == 0,
        s"expected chroma key path to stay active without kills, saw ${measuredPass.chromaFail}"
      )
      assert(
        measuredPass.zFail == 0,
        s"expected depth path to stay active without kills, saw ${measuredPass.zFail}"
      )
      assert(
        measuredPass.alphaFail == 0,
        s"expected alpha test path to stay active without kills, saw ${measuredPass.alphaFail}"
      )

      val throughput = measuredPass.pixelsOut.toDouble / measuredPass.activeCycles.toDouble
      assert(
        throughput >= 0.40,
        f"expected at least 0.40 pixels/cycle with the full feature path hot, got $throughput%.3f"
      )
    }
  }
}
