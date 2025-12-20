package voodoo

import spinal.core._
import spinal.lib._
import voodoo.utils.StreamWhile

case class Rasterizer(c: Config) extends Component {
  val i = slave Stream (TriangleSetup.Output(c))
  val o = master Stream (Rasterizer.Output(c))

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

      // Start going right
      state.goingRight := True

      // Initialize edge function values
      state.edge := input.edgeStart

      // Initialize gradient values
      state.grads.all.zip(input.grads.all).foreach { case (out, in) =>
        out := in.start
      }

      state
    },
    step = (idx: UInt, state: Rasterizer.State, output: Rasterizer.OutputWithFlag) => {
      // Compute inside triangle flag
      val zero = 0.SQ(1 bits, 0 bits)
      val insideTriangle =
        (state.edge(0) >= zero) && (state.edge(1) >= zero) && (state.edge(2) >= zero)

      // Output current state with inside flag
      // Round coordinates to integers (exp=0 means round to whole numbers)
      output.data.coords(0) := state.coords(0).floor(0).asSInt
      output.data.coords(1) := state.coords(1).floor(0).asSInt
      output.data.grads := state.grads
      output.data.config := state.input.config // Pass through per-triangle config
      output.insideTriangle := insideTriangle

      // Compute next state
      val next = cloneOf(state)

      val xmin = state.input.xrange(0)
      val xmax = state.input.xrange(1)
      // At edge when current position is at the boundary for our direction
      val atEdge = (state.goingRight && (state.coords(0) >= xmax)) ||
        (!state.goingRight && (state.coords(0) <= xmin))

      // Create step size of 1.0 in the coordinate format
      // For SQ(12,4): need raw value of 16 (1.0 * 2^4)
      val one = AFix.SQ(7 bits, 4 bits)
      one := 1.0
      val negOne = AFix.SQ(7 bits, 4 bits)
      negOne := -1.0

      // Gradient formats in order: red, green, blue, depth, alpha, w, s0, t0, s1, t1
      val gradFormats = Seq(
        c.vColorFormat,
        c.vColorFormat,
        c.vColorFormat,
        c.vDepthFormat,
        c.vColorFormat,
        c.wFormat,
        c.texCoordsFormat,
        c.texCoordsFormat,
        c.texCoordsFormat,
        c.texCoordsFormat
      )

      when(atEdge) {
        // At edge: move down, flip direction
        next.coords(0) := state.coords(0)
        next.coords(1) := (state.coords(1) + one).fixTo(c.vertexFormat)
        next.goingRight := !state.goingRight

        // Update edges and gradients: add b/dy
        next.edge.zip(state.edge).zip(state.input.coeffs).foreach { case ((nxt, cur), coeff) =>
          nxt := (cur + coeff.b).fixTo(c.coefficientFormat)
        }
        next.grads.all.zip(state.grads.all).zip(state.input.grads.all).zipWithIndex.foreach {
          case (((nxt, cur), grad), idx) =>
            nxt := (cur + grad.d(1)).fixTo(gradFormats(idx))
        }
      }.otherwise {
        // Move horizontally
        val dx = state.goingRight ? one | negOne
        next.coords(0) := (state.coords(0) + dx).fixTo(c.vertexFormat)
        next.coords(1) := state.coords(1)
        next.goingRight := state.goingRight

        // Update edges and gradients: add/subtract a/dx based on direction
        next.edge.zip(state.edge).zip(state.input.coeffs).foreach { case ((nxt, cur), coeff) =>
          val da = state.goingRight ? coeff.a | (-coeff.a)
          nxt := (cur + da).fixTo(c.coefficientFormat)
        }
        next.grads.all.zip(state.grads.all).zip(state.input.grads.all).zipWithIndex.foreach {
          case (((nxt, cur), grad), idx) =>
            val dg = state.goingRight ? grad.d(0) | (-grad.d(0))
            nxt := (cur + dg).fixTo(gradFormats(idx))
        }
      }

      next.input := state.input

      // Check if this is the last pixel: either next Y is out of bounds, or at max iterations
      val nextOutOfBounds = next.coords(1) > state.input.yrange(1)
      val isLast = nextOutOfBounds

      (next, isLast)
    }
  )

  // Extract output stream and running signal
  val withInsideFlag = streamWhileResult.output
  val running = streamWhileResult.running

  // Drop samples outside triangle and remove flag
  o << withInsideFlag.takeWhen(withInsideFlag.payload.insideTriangle).map(_.data)
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
    val wGrad = mk(c.wFormat)
    // TMU0 texture coordinates (14.18 format)
    val s0Grad = mk(c.texCoordsFormat)
    val t0Grad = mk(c.texCoordsFormat)
    // TMU1 texture coordinates (14.18 format)
    val s1Grad = mk(c.texCoordsFormat)
    val t1Grad = mk(c.texCoordsFormat)

    def all: Seq[T] =
      Seq(redGrad, greenGrad, blueGrad, depthGrad, alphaGrad, wGrad, s0Grad, t0Grad, s1Grad, t1Grad)
  }

  /** Input is directly the TriangleSetup output (which includes gradients) */
  type Input = TriangleSetup.Output

  case class Output(c: Config) extends Bundle {
    val grads = GradientBundle(AFix(_), c)
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val config = TriangleSetup.PerTriangleConfig(c) // Per-triangle render configuration
  }

  case class OutputWithFlag(c: Config) extends Bundle {
    val data = Output(c)
    val insideTriangle = Bool()
  }

  case class State(c: Config) extends Bundle {
    val coords = vertex2d(c.vertexFormat) // Internal fixed-point coordinates
    val grads = GradientBundle(AFix(_), c)
    val edge = Vec.fill(3)(AFix(c.coefficientFormat))
    val goingRight = Bool() // Direction flag for serpentine scanning
    val input = TriangleSetup.Output(c) // Keep input for iteration
  }
}
