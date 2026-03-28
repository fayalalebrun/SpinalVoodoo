package voodoo.raster

import voodoo._
import spinal.core._
import spinal.lib._
import voodoo.bus.StreamWhile

/** Fastfill: rectangle pixel generator for the fastfillCMD screen-clear command.
  *
  * Scans left-to-right, top-to-bottom over the clip rectangle, emitting one pixel coordinate per
  * cycle. Uses the same StreamWhile pattern as the Rasterizer.
  */
case class Fastfill(c: Config) extends Component {
  val i = slave Stream (NoData)
  val o = master Stream (Fastfill.Output(c))

  val clipLeft = in UInt (10 bits)
  val clipRight = in UInt (10 bits)
  val clipLowY = in UInt (10 bits)
  val clipHighY = in UInt (10 bits)

  val streamWhileResult = StreamWhile(
    i,
    stateType = HardType(Fastfill.State()),
    outputType = HardType(Fastfill.OutputWithFlag(c)),
    maxIterations = c.maxFbDims._1 * c.maxFbDims._2,
    init = (_: NoData) => {
      val state = cloneOf(Fastfill.State())
      state.x := clipLeft
      state.y := clipLowY
      state.clipLeft := clipLeft
      state.clipRight := clipRight
      state.clipLowY := clipLowY
      state.clipHighY := clipHighY
      state.empty := (clipRight <= clipLeft) || (clipHighY <= clipLowY)
      state
    },
    step = (_: UInt, state: Fastfill.State, output: Fastfill.OutputWithFlag) => {
      // Output current pixel
      // Use (False ## x).asSInt to avoid UInt→SInt sign reinterpretation (MEMORY.md pitfall)
      output.data.coords(0) := (False ## state.x).asSInt.resize(c.vertexFormat.nonFraction bits)
      output.data.coords(1) := (False ## state.y).asSInt.resize(c.vertexFormat.nonFraction bits)
      output.pixelValid := !state.empty

      val next = cloneOf(state)
      next.clipLeft := state.clipLeft
      next.clipRight := state.clipRight
      next.clipLowY := state.clipLowY
      next.clipHighY := state.clipHighY
      next.empty := state.empty

      // Advance: x+1, wrap to next row when reaching clipRight
      val atRowEnd = (state.x + 1) >= state.clipRight
      when(atRowEnd) {
        next.x := state.clipLeft
        next.y := state.y + 1
      } otherwise {
        next.x := state.x + 1
        next.y := state.y
      }

      // Last pixel: either empty rectangle, or at row end and next row is past clipHighY
      val isLast = state.empty || (atRowEnd && ((state.y + 1) >= state.clipHighY))

      (next, isLast)
    }
  )

  // Filter out invalid pixels (empty rectangle case) and strip the flag
  val withFlag = streamWhileResult.output
  o << withFlag.takeWhen(withFlag.payload.pixelValid).map(_.data)

  val running = out(Bool())
  running := streamWhileResult.running
}

object Fastfill {
  case class Output(c: Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
  }

  case class OutputWithFlag(c: Config) extends Bundle {
    val data = Output(c)
    val pixelValid = Bool()
  }

  case class State() extends Bundle {
    val x = UInt(10 bits)
    val y = UInt(10 bits)
    val clipLeft = UInt(10 bits)
    val clipRight = UInt(10 bits)
    val clipLowY = UInt(10 bits)
    val clipHighY = UInt(10 bits)
    val empty = Bool()
  }
}
