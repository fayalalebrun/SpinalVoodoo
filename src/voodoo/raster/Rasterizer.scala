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
  spanWalker.clipLowY := clipLowY
  spanWalker.clipHighY := clipHighY
  spanRasterizer.i << queuedSpan
  prefetchSpan.translateFrom(queuedPrefetchSpan) { (out, in) =>
    out := Rasterizer.PrefetchSpan.fromSpanWalker(c, in)
  }
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
    val tmuConfig = TriangleSetup.TmuConfig(c)
    val pixelConfig = TriangleSetup.PixelPipelineConfig(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  case class PrefetchSpan(c: Config) extends Bundle {
    val xStart = AFix(c.vertexFormat)
    val xEnd = AFix(c.vertexFormat)
    val y = AFix(c.vertexFormat)
    val pixelConfig = TriangleSetup.PixelPipelineConfig(c)
  }

  object PrefetchSpan {
    def fromSpanWalker(c: Config, in: SpanWalker.Output): PrefetchSpan = {
      val out = PrefetchSpan(c)
      out.xStart := in.xStart
      out.xEnd := in.xEnd
      out.y := in.y
      out.pixelConfig := in.pixelConfig
      out
    }
  }

  case class PostTmu(c: Config) extends Bundle {
    val grads = GradientBundle(AFix(_), c)
    val alphaGradHi = AFix(c.texCoordsHiFormat)
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val config = TriangleSetup.PixelPipelineConfig(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  object PostTmu {
    def fromOutput(c: Config, in: Output): PostTmu = {
      val out = PostTmu(c)
      out.grads := in.grads
      out.alphaGradHi := in.alphaGradHi
      out.coords := in.coords
      out.config := in.pixelConfig
      if (c.trace.enabled) {
        out.trace := in.trace
      }
      out
    }
  }

  case class SpanState(c: Config) extends Bundle {
    val x = AFix(c.vertexFormat)
    val y = AFix(c.vertexFormat)
    val linear = SpanWalker.LinearState(c)
    val hiS = AFix(c.texCoordsHiFormat)
    val hiT = AFix(c.texCoordsHiFormat)
    val hiAlpha = AFix(c.texCoordsHiFormat)
    val xEnd = AFix(c.vertexFormat)
    val stepX = SpanWalker.StepX(c)
    val tmuConfig = TriangleSetup.TmuConfig(c)
    val pixelConfig = TriangleSetup.PixelPipelineConfig(c)
    val trace = if (c.trace.enabled) Trace.PrimitiveKey() else null
  }

  case class SpanRasterizer(c: Config) extends Component {
    val i = slave Stream (SpanWalker.Output(c))
    val o = master Stream (Output(c))
    val running = out(Bool())

    val pixelSeqCounter = if (c.trace.enabled) Reg(UInt(24 bits)) init (0) else null
    val currentPixelSeq = if (c.trace.enabled) pixelSeqCounter else null

    def toPixelCoord(value: AFix): SInt = {
      // Span state carries 12.4 fixed-point positions; downstream coords are integer pixels.
      // Use an arithmetic shift on the full fixed-point encoding so x>=128 does not wrap.
      (value.asBits.asSInt >> c.vertexFormat.fraction).resize(c.vertexFormat.nonFraction bits)
    }

    val streamWhileResult = StreamWhile(
      i,
      stateType = HardType(SpanState(c)),
      outputType = HardType(Output(c)),
      maxIterations = c.maxFbDims._1,
      init = (input: SpanWalker.Output) => {
        val state = cloneOf(SpanState(c))
        state.x := input.xStart
        state.y := input.y
        state.linear := input.linear
        state.hiS := input.hiS
        state.hiT := input.hiT
        state.hiAlpha := input.hiAlpha
        state.xEnd := input.xEnd
        state.stepX := input.stepX
        state.tmuConfig := input.tmuConfig
        state.pixelConfig := input.pixelConfig
        if (c.trace.enabled) {
          state.trace := input.trace
        }
        state
      },
      step = (idx: UInt, state: SpanState, output: Output) => {
        output.coords(0) := toPixelCoord(state.x)
        output.coords(1) := toPixelCoord(state.y)
        output.grads.redGrad := state.linear.red
        output.grads.greenGrad := state.linear.green
        output.grads.blueGrad := state.linear.blue
        output.grads.depthGrad := state.linear.depth
        output.grads.alphaGrad := state.linear.alpha
        output.grads.wGrad := state.linear.w
        output.grads.sGrad := state.hiS.fixTo(c.texCoordsAccumFormat)
        output.grads.tGrad := state.hiT.fixTo(c.texCoordsAccumFormat)
        output.alphaGradHi := state.hiAlpha
        output.tmuConfig := state.tmuConfig
        output.pixelConfig := state.pixelConfig
        if (c.trace.enabled) {
          output.trace.primitive := state.trace
          output.trace.pixelSeq := currentPixelSeq
        }

        val next = cloneOf(state)
        next.y := state.y
        next.xEnd := state.xEnd
        next.stepX := state.stepX
        next.tmuConfig := state.tmuConfig
        next.pixelConfig := state.pixelConfig
        if (c.trace.enabled) {
          next.trace := state.trace
        }
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

        next.x := (state.x + one).fixTo(c.vertexFormat)
        next.linear.all
          .zip(state.linear.all)
          .zip(state.stepX.linear.all)
          .zipWithIndex
          .foreach { case (((nxt, cur), grad), gradIdx) =>
            nxt := (cur + grad).fixTo(lowGradFormats(gradIdx))
          }
        next.hiS := (state.hiS + state.stepX.tex(0)).fixTo(c.texCoordsHiFormat)
        next.hiT := (state.hiT + state.stepX.tex(1)).fixTo(c.texCoordsHiFormat)
        next.hiAlpha := (state.hiAlpha + state.stepX.alpha).fixTo(c.texCoordsHiFormat)

        val isLast = toPixelCoord(state.x) >= toPixelCoord(state.xEnd)
        (next, isLast)
      }
    )

    running := streamWhileResult.running

    o << streamWhileResult.output

    if (c.trace.enabled) {
      when(i.fire && i.payload.firstSpan) {
        pixelSeqCounter := 0
      }.elsewhen(streamWhileResult.output.fire) {
        pixelSeqCounter := currentPixelSeq + 1
      }
    }

  }
}
