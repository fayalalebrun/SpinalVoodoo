package voodoo.raster

import voodoo._
import spinal.core._
import spinal.core.formal._
import spinal.lib._
import voodoo.bus.StreamWhile

case class Rasterizer(c: Config, formalStrong: Boolean = false) extends Component {
  val i = slave Stream (TriangleSetup.Output(c))
  val o = master Stream (Rasterizer.Output(c))
  val pixelSeqCounter = if (c.trace.enabled) Reg(UInt(24 bits)) init (0) else null
  val currentPixelSeq = if (c.trace.enabled) Mux(i.fire, U(0, 24 bits), pixelSeqCounter) else null

  // Scissor clip inputs
  val enableClipping = in Bool ()
  val clipLeft = in UInt (10 bits)
  val clipRight = in UInt (10 bits)
  val clipLowY = in UInt (10 bits)
  val clipHighY = in UInt (10 bits)

  val streamWhileResult = StreamWhile(
    i,
    stateType = HardType(Rasterizer.State(c)),
    outputType = HardType(Rasterizer.OutputWithFlag(c)),
    maxIterations = c.maxFbDims._1 * c.maxFbDims._2,
    init = (input: TriangleSetup.Output) => {
      val state = cloneOf(Rasterizer.State(c))
      state.input := input

      // Initialize coordinates to top-left of bounding box
      state.coords(0) := input.xrange(0)
      state.coords(1) := input.yrange(0)

      // Initialize edge function values
      state.edge := input.edgeStart
      state.rowStartEdge := input.edgeStart

      // Initialize gradient values
      state.grads.all.take(6).zip(input.grads.all.take(6)).foreach { case (out, in) =>
        out := in.start
      }
      state.rowStartGrads.all.take(6).zip(input.grads.all.take(6)).foreach { case (out, in) =>
        out := in.start
      }
      state.texHi := input.texHi
      state.rowStartTexHi := input.texHi
      state.alphaHi := input.hiAlpha
      state.rowStartAlphaHi := input.hiAlpha
      state.grads.sGrad := input.texHi.sStart.fixTo(c.texCoordsAccumFormat)
      state.grads.tGrad := input.texHi.tStart.fixTo(c.texCoordsAccumFormat)
      state.rowStartGrads.sGrad := input.texHi.sStart.fixTo(c.texCoordsAccumFormat)
      state.rowStartGrads.tGrad := input.texHi.tStart.fixTo(c.texCoordsAccumFormat)

      state
    },
    step = (idx: UInt, state: Rasterizer.State, output: Rasterizer.OutputWithFlag) => {
      // Compute inside triangle flag
      val zero = 0.SQ(1 bits, 0 bits)
      val insideTriangle =
        (state.edge(0) >= zero) && (state.edge(1) >= zero) && (state.edge(2) >= zero)

      // Output current state with inside flag
      // Round coordinates to integers (exp=0 means round to whole numbers)
      val intX = state.coords(0).floor(0).asSInt
      val intY = state.coords(1).floor(0).asSInt
      output.data.coords(0) := intX
      output.data.coords(1) := intY
      output.data.grads.redGrad := state.grads.redGrad
      output.data.grads.greenGrad := state.grads.greenGrad
      output.data.grads.blueGrad := state.grads.blueGrad
      output.data.grads.depthGrad := state.grads.depthGrad
      output.data.grads.alphaGrad := state.grads.alphaGrad
      output.data.alphaGradHi := state.alphaHi.start
      output.data.grads.wGrad := state.grads.wGrad
      output.data.grads.sGrad := state.texHi.sStart.fixTo(c.texCoordsAccumFormat)
      output.data.grads.tGrad := state.texHi.tStart.fixTo(c.texCoordsAccumFormat)
      output.data.config := state.input.config // Pass through per-triangle config
      if (c.trace.enabled) {
        output.data.trace.primitive := state.input.trace
        output.data.trace.pixelSeq := currentPixelSeq
      }

      // Scissor clip test: inclusive of left/lowY, exclusive of right/highY
      // Widen UInt(10) clip bounds to SInt for comparison with signed coords
      val clipLeftS = clipLeft.resize(c.vertexFormat.nonFraction bits).asSInt
      val clipRightS = clipRight.resize(c.vertexFormat.nonFraction bits).asSInt
      val clipLowYS = clipLowY.resize(c.vertexFormat.nonFraction bits).asSInt
      val clipHighYS = clipHighY.resize(c.vertexFormat.nonFraction bits).asSInt
      val insideClip = (intX >= clipLeftS) && (intX < clipRightS) &&
        (intY >= clipLowYS) && (intY < clipHighYS)

      output.insideTriangle := insideTriangle && (!enableClipping || insideClip)
      output.xrange := state.input.xrange
      output.yrange := state.input.yrange

      // Compute next state
      val next = cloneOf(state)
      next := state

      val xmin = state.input.xrange(0)
      val xmax = state.input.xrange(1)
      // At edge when current position reaches the right side of the scanline bbox
      val atEdge = state.coords(0) >= xmax

      // Create step size of 1.0 in the coordinate format
      // For SQ(12,4): need raw value of 16 (1.0 * 2^4)
      val one = AFix.SQ(7 bits, 4 bits)
      one := 1.0
      val negOne = AFix.SQ(7 bits, 4 bits)
      negOne := -1.0

      // Gradient formats in order: red, green, blue, depth, alpha, w, s, t
      val gradFormats = Seq(
        c.vColorFormat,
        c.vColorFormat,
        c.vColorFormat,
        c.vDepthFormat,
        c.vColorFormat,
        c.wAccumFormat
      )

      // Helper: update edges and gradients with selected deltas
      def updateEdgesAndGrads(
          edgeDelta: Coefficients => AFix,
          gradDelta: Rasterizer.InputGradient => AFix
      ): Unit = {
        next.edge.zip(state.edge).zip(state.input.coeffs).foreach { case ((nxt, cur), coeff) =>
          nxt := (cur + edgeDelta(coeff)).fixTo(c.coefficientFormat)
        }
        next.grads.all
          .take(6)
          .zip(state.grads.all.take(6))
          .zip(state.input.grads.all.take(6))
          .zipWithIndex
          .foreach { case (((nxt, cur), grad), idx) =>
            nxt := (cur + gradDelta(grad)).fixTo(gradFormats(idx))
          }
      }

      when(atEdge) {
        // At edge: advance to the next scanline and restart from xmin
        next.coords(0) := xmin
        next.coords(1) := (state.coords(1) + one).fixTo(c.vertexFormat)
        next.rowStartEdge.zip(state.rowStartEdge).zip(state.input.coeffs).foreach {
          case ((nxt, cur), coeff) =>
            nxt := (cur + coeff.b).fixTo(c.coefficientFormat)
        }
        next.edge := next.rowStartEdge
        next.rowStartGrads.all
          .take(6)
          .zip(state.rowStartGrads.all.take(6))
          .zip(state.input.grads.all.take(6))
          .zipWithIndex
          .foreach { case (((nxt, cur), grad), idx) =>
            nxt := (cur + grad.d(1)).fixTo(gradFormats(idx))
          }
        next.rowStartTexHi.sStart := (state.rowStartTexHi.sStart + state.input.texHi.dSdY)
          .fixTo(c.texCoordsHiFormat)
        next.rowStartTexHi.tStart := (state.rowStartTexHi.tStart + state.input.texHi.dTdY)
          .fixTo(c.texCoordsHiFormat)
        next.texHi := next.rowStartTexHi
        next.rowStartAlphaHi.start := (state.rowStartAlphaHi.start + state.input.hiAlpha.dAdY)
          .fixTo(c.texCoordsHiFormat)
        next.alphaHi := next.rowStartAlphaHi
        next.rowStartGrads.sGrad := next.rowStartTexHi.sStart.fixTo(c.texCoordsAccumFormat)
        next.rowStartGrads.tGrad := next.rowStartTexHi.tStart.fixTo(c.texCoordsAccumFormat)
        next.grads := next.rowStartGrads
      }.otherwise {
        // Move horizontally left-to-right
        next.coords(0) := (state.coords(0) + one).fixTo(c.vertexFormat)
        next.coords(1) := state.coords(1)
        next.rowStartEdge := state.rowStartEdge
        next.rowStartGrads := state.rowStartGrads
        next.rowStartTexHi := state.rowStartTexHi
        next.rowStartAlphaHi := state.rowStartAlphaHi
        updateEdgesAndGrads(_.a, _.d(0))
        next.texHi.sStart := (state.texHi.sStart + state.input.texHi.dSdX)
          .fixTo(c.texCoordsHiFormat)
        next.texHi.tStart := (state.texHi.tStart + state.input.texHi.dTdX)
          .fixTo(c.texCoordsHiFormat)
        next.alphaHi.start := (state.alphaHi.start + state.input.hiAlpha.dAdX)
          .fixTo(c.texCoordsHiFormat)
        next.grads.sGrad := next.texHi.sStart.fixTo(c.texCoordsAccumFormat)
        next.grads.tGrad := next.texHi.tStart.fixTo(c.texCoordsAccumFormat)
      }

      // Check if this is the last pixel: either next Y is out of bounds, or at max iterations
      // yrange(1) is the exclusive upper bound, so >= means next scanline is past the end
      val nextOutOfBounds = next.coords(1) >= state.input.yrange(1)
      val isLast = nextOutOfBounds

      (next, isLast)
    }
  )

  // Extract output stream and running signal
  val withInsideFlag = streamWhileResult.output
  val running = out(Bool())
  running := streamWhileResult.running

  GenerationFlags.formal {
    val formalReset = ClockDomain.current.isResetActive

    when(!formalReset && withInsideFlag.valid) {
      val intX = withInsideFlag.payload.data.coords(0)
      val intY = withInsideFlag.payload.data.coords(1)
      val xmin = withInsideFlag.payload.xrange(0).floor(0).asSInt
      val xmax = withInsideFlag.payload.xrange(1).floor(0).asSInt
      val ymin = withInsideFlag.payload.yrange(0).floor(0).asSInt
      val ymax = withInsideFlag.payload.yrange(1).floor(0).asSInt

      // Forward scanline traversal stays within the bbox up to the usual fixed-point edge probe.
      val N = c.vertexFormat.nonFraction + 1
      assert(intX >= xmin.resize(N))
      assert(intX <= xmax.resize(N) + 1)

      // Top/bottom are strict bounds + 1 iteration
      assert(intY >= ymin.resize(N))
      assert(intY <= ymax.resize(N))

      when(enableClipping && withInsideFlag.payload.insideTriangle) {
        val clipLeftS = clipLeft.resize(c.vertexFormat.nonFraction bits).asSInt
        val clipRightS = clipRight.resize(c.vertexFormat.nonFraction bits).asSInt
        val clipLowYS = clipLowY.resize(c.vertexFormat.nonFraction bits).asSInt
        val clipHighYS = clipHighY.resize(c.vertexFormat.nonFraction bits).asSInt

        assert(intX >= clipLeftS)
        assert(intX < clipRightS)
        assert(intY >= clipLowYS)
        assert(intY < clipHighYS)
      }
    }
  }

  // Drop samples outside triangle and remove flag
  o << withInsideFlag.takeWhen(withInsideFlag.payload.insideTriangle).map(_.data)

  if (c.trace.enabled) {
    when(i.fire) {
      pixelSeqCounter := 0
    }
    when(withInsideFlag.fire && withInsideFlag.payload.insideTriangle) {
      pixelSeqCounter := currentPixelSeq + 1
    }
  }
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
    // TMU texture coordinates (14.18 format) - single TMU support (Voodoo 1 level)
    val sGrad = mk(c.texCoordsAccumFormat)
    val tGrad = mk(c.texCoordsAccumFormat)

    def all: Seq[T] =
      Seq(redGrad, greenGrad, blueGrad, depthGrad, alphaGrad, wGrad, sGrad, tGrad)
  }

  /** Input is directly the TriangleSetup output (which includes gradients) */
  type Input = TriangleSetup.Output

  case class Output(c: Config) extends Bundle {
    val grads = GradientBundle(AFix(_), c)
    val alphaGradHi = AFix(c.texCoordsHiFormat)
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val config = TriangleSetup.PerTriangleConfig(c) // Per-triangle render configuration
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  case class OutputWithFlag(c: Config) extends Bundle {
    val data = Output(c)
    val insideTriangle = Bool()
    val xrange = vertex2d(c.vertexFormat)
    val yrange = vertex2d(c.vertexFormat)
  }

  case class State(c: Config) extends Bundle {
    val coords = vertex2d(c.vertexFormat) // Internal fixed-point coordinates
    val grads = GradientBundle(AFix(_), c)
    val rowStartGrads = GradientBundle(AFix(_), c)
    val alphaHi = TriangleSetup.HiAlpha(c)
    val rowStartAlphaHi = TriangleSetup.HiAlpha(c)
    val texHi = TriangleSetup.HiTexCoords(c)
    val rowStartTexHi = TriangleSetup.HiTexCoords(c)
    val edge = Vec.fill(3)(AFix(c.coefficientFormat))
    val rowStartEdge = Vec.fill(3)(AFix(c.coefficientFormat))
    val input = TriangleSetup.Output(c) // Keep input for iteration
  }
}
