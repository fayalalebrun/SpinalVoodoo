package voodoo.raster

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import voodoo._
import voodoo.bus.StreamWhile

case class Rasterizer(c: Config, formalStrong: Boolean = false) extends Component {
  val i = slave Stream (TriangleSetup.Output(c))
  val o = master Stream (Rasterizer.Output(c))
  val prefetchSpan = master Stream (Rasterizer.PrefetchSpan(c))

  val enableClipping = in Bool ()
  val clipLeft = in UInt (10 bits)
  val clipRight = in UInt (10 bits)
  val clipLowY = in UInt (10 bits)
  val clipHighY = in UInt (10 bits)

  val queuedInput = i.queue(2)
  val spanWalker = SpanWalker(c, formalStrong = formalStrong)
  val spanRasterizer = Rasterizer.SpanRasterizer(c)
  val walkerSpanFork = StreamFork2(spanWalker.o, synchronous = true)
  val queuedSpan = walkerSpanFork._1.queue(4)
  val queuedPrefetchSpan = walkerSpanFork._2.queue(16)

  spanWalker.i << queuedInput
  spanWalker.enableClipping := enableClipping
  spanWalker.clipLeft := clipLeft
  spanWalker.clipRight := clipRight
  spanRasterizer.i << queuedSpan
  prefetchSpan.translateFrom(queuedPrefetchSpan) { (out, in) =>
    out.xStart := in.xStart
    out.xEnd := in.xEnd
    out.y := in.y
    out.config := in.config
  }
  spanRasterizer.enableClipping := enableClipping
  spanRasterizer.clipLeft := clipLeft
  spanRasterizer.clipRight := clipRight
  spanRasterizer.clipLowY := clipLowY
  spanRasterizer.clipHighY := clipHighY

  o << spanRasterizer.o

  val running = out(Bool())
  running := queuedInput.valid || spanWalker.running || queuedSpan.valid || spanRasterizer.running
}

object Rasterizer {
  case class InputGradient(fmt: QFormat) extends Bundle {
    val start = AFix(fmt)
    val d = vertex2d(fmt)
  }

  case class GradientBundle[T <: Data](mk: QFormat => T, c: Config) extends Bundle {
    val redGrad = mk(c.vColorFormat)
    val greenGrad = mk(c.vColorFormat)
    val blueGrad = mk(c.vColorFormat)
    val depthGrad = mk(c.vDepthFormat)
    val alphaGrad = mk(c.vColorFormat)
    val wGrad = mk(c.wAccumFormat)
    val sGrad = mk(c.texCoordsAccumFormat)
    val tGrad = mk(c.texCoordsAccumFormat)

    def all: Seq[T] =
      Seq(redGrad, greenGrad, blueGrad, depthGrad, alphaGrad, wGrad, sGrad, tGrad)
  }

  type Input = TriangleSetup.Output

  case class Output(c: Config) extends Bundle {
    val grads = GradientBundle(AFix(_), c)
    val alphaGradHi = AFix(c.texCoordsHiFormat)
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val config = TriangleSetup.PerTriangleConfig(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  case class PrefetchSpan(c: Config) extends Bundle {
    val xStart = AFix(c.vertexFormat)
    val xEnd = AFix(c.vertexFormat)
    val y = AFix(c.vertexFormat)
    val config = TriangleSetup.PerTriangleConfig(c)
  }

  case class SpanPixelOutput(c: Config) extends Bundle {
    val data = Output(c)
    val insideClip = Bool()
  }

  case class SpanState(c: Config) extends Bundle {
    val coords = vertex2d(c.vertexFormat)
    val grads = GradientBundle(AFix(_), c)
    val alphaHi = TriangleSetup.HiAlpha(c)
    val texHi = TriangleSetup.HiTexCoords(c)
    val input = SpanWalker.Output(c)
  }

  case class SpanRasterizer(c: Config) extends Component {
    val i = slave Stream (SpanWalker.Output(c))
    val o = master Stream (Output(c))
    val running = out(Bool())

    val enableClipping = in Bool ()
    val clipLeft = in UInt (10 bits)
    val clipRight = in UInt (10 bits)
    val clipLowY = in UInt (10 bits)
    val clipHighY = in UInt (10 bits)

    val pixelSeqCounter = if (c.trace.enabled) Reg(UInt(24 bits)) init (0) else null
    val currentPixelSeq = if (c.trace.enabled) pixelSeqCounter else null

    val streamWhileResult = StreamWhile(
      i,
      stateType = HardType(SpanState(c)),
      outputType = HardType(SpanPixelOutput(c)),
      maxIterations = c.maxFbDims._1,
      init = (input: SpanWalker.Output) => {
        val state = cloneOf(SpanState(c))
        state.input := input
        state.coords(0) := input.xStart
        state.coords(1) := input.y
        state.grads.all.zip(input.grads.all).foreach { case (out, in) =>
          out := in.start
        }
        state.alphaHi := input.hiAlpha
        state.texHi := input.texHi
        state
      },
      step = (idx: UInt, state: SpanState, output: SpanPixelOutput) => {
        output.data.coords(0) := state.coords(0).floor(0).asSInt
        output.data.coords(1) := state.coords(1).floor(0).asSInt
        output.data.grads := state.grads
        output.data.alphaGradHi := state.alphaHi.start
        output.data.config := state.input.config
        if (c.trace.enabled) {
          output.data.trace.primitive := state.input.trace
          output.data.trace.pixelSeq := currentPixelSeq
        }

        val clipLeftS = clipLeft.resize(c.vertexFormat.nonFraction bits).asSInt
        val clipRightS = clipRight.resize(c.vertexFormat.nonFraction bits).asSInt
        val clipLowYS = clipLowY.resize(c.vertexFormat.nonFraction bits).asSInt
        val clipHighYS = clipHighY.resize(c.vertexFormat.nonFraction bits).asSInt
        val insideClip =
          (output.data.coords(0) >= clipLeftS) &&
            (output.data.coords(0) < clipRightS) &&
            (output.data.coords(1) >= clipLowYS) &&
            (output.data.coords(1) < clipHighYS)
        output.insideClip := !enableClipping || insideClip

        val next = cloneOf(state)
        next.input := state.input
        val one = AFix(c.vertexFormat)
        one := 1.0
        val lowGradFormats = Seq(
          c.vColorFormat,
          c.vColorFormat,
          c.vColorFormat,
          c.vDepthFormat,
          c.vColorFormat,
          c.wAccumFormat
        )

        next.coords(0) := (state.coords(0) + one).fixTo(c.vertexFormat)
        next.coords(1) := state.coords(1)
        next.grads.all
          .take(6)
          .zip(state.grads.all.take(6))
          .zip(state.input.grads.all.take(6))
          .zipWithIndex
          .foreach { case (((nxt, cur), grad), gradIdx) =>
            nxt := (cur + grad.d(0)).fixTo(lowGradFormats(gradIdx))
          }
        val nextS = (state.texHi.sStart + state.input.texHi.dSdX).fixTo(c.texCoordsHiFormat)
        val nextT = (state.texHi.tStart + state.input.texHi.dTdX).fixTo(c.texCoordsHiFormat)
        val nextA = (state.alphaHi.start + state.input.hiAlpha.dAdX).fixTo(c.texCoordsHiFormat)
        next.texHi.sStart := nextS
        next.texHi.tStart := nextT
        next.texHi.dSdX := state.input.texHi.dSdX
        next.texHi.dTdX := state.input.texHi.dTdX
        next.texHi.dSdY := state.input.texHi.dSdY
        next.texHi.dTdY := state.input.texHi.dTdY
        next.alphaHi.start := nextA
        next.alphaHi.dAdX := state.input.hiAlpha.dAdX
        next.alphaHi.dAdY := state.input.hiAlpha.dAdY
        next.grads.sGrad := nextS.fixTo(c.texCoordsAccumFormat)
        next.grads.tGrad := nextT.fixTo(c.texCoordsAccumFormat)

        val isLast = state.coords(0) >= state.input.xEnd
        (next, isLast)
      }
    )

    val withClipFlag = streamWhileResult.output
    running := streamWhileResult.running

    o << withClipFlag.takeWhen(withClipFlag.payload.insideClip).map(_.data)

    if (c.trace.enabled) {
      when(i.fire && i.payload.firstSpan) {
        pixelSeqCounter := 0
      }.elsewhen(withClipFlag.fire && withClipFlag.payload.insideClip) {
        pixelSeqCounter := currentPixelSeq + 1
      }
    }

  }
}
