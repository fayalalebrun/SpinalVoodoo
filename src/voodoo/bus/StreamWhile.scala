package voodoo.bus

import voodoo._
import spinal.core._
import spinal.core.sim._
import spinal.core.formal._
import spinal.lib._

/** Result of StreamWhile containing both output stream and running flag */
case class StreamWhileResult[T <: Data](output: Stream[T], running: Bool)

/** StreamWhile: Generates a stream of outputs from a single trigger, running while a condition
  * holds
  *
  * This component converts a single trigger event into a stream of outputs, with state carried
  * between iterations. Useful for iterative algorithms that need to run until a completion
  * condition.
  *
  * @param trigger
  *   Input stream that starts the iteration
  * @param stateType
  *   Type of state carried between iterations
  * @param outputType
  *   Type of output stream elements
  * @param maxIterations
  *   Maximum number of iterations (prevents infinite loops)
  * @param init
  *   Function to initialize state from trigger payload
  * @param step
  *   Function called each iteration: (counter, state, output) => (nextState, isLast)
  * @return
  *   StreamWhileResult with output stream and running flag
  */
object StreamWhile {
  def apply[TIn <: Data, TState <: Data, TOut <: Data](
      trigger: Stream[TIn],
      stateType: HardType[TState],
      outputType: HardType[TOut],
      maxIterations: Int,
      init: TIn => TState, // Initialize loop state from trigger
      step: (UInt, TState, TOut) => (TState, Bool) // Assign output, return (next state, isLast)
  ): StreamWhileResult[TOut] = {
    val composite = new Composite(trigger, "streamWhile") {

      val counter = Counter(maxIterations)
      val running = RegInit(False)
      running.simPublic() // Expose for simulation monitoring
      val state = Reg(stateType())

      val output = Stream(outputType)

      // Start new loop iteration
      when(trigger.fire) {
        running := True
        state := init(trigger.payload)
        counter.clear()
      }

      // Call step to get output, next state, and last flag
      val (nextState, isLast) = step(counter.value, state, output.payload)

      output.valid := running

      // Stop after outputting last value
      when(output.fire && isLast) {
        running := False
      }

      // Step to next iteration when output fires and not last
      when(output.fire && !isLast) {
        state := nextState
        counter.increment()

        // Stop if max iterations will be exceeded
        when(counter.willOverflowIfInc) {
          running := False
        }
      }

      trigger.ready := !running

      // Formal verification invariants
      GenerationFlags.formal {
        // Invariant 1: output.valid equals running state
        // This is the key invariant needed for inductive proofs with Stream protocol assertions
        assert(output.valid === running)

        // Invariant 2: trigger.ready is inverse of running
        assert(trigger.ready === !running)
      }

    }
    StreamWhileResult(composite.output, composite.running)
  }
}
