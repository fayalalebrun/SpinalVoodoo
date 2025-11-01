package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._

object StreamWhile {
  def apply[TIn <: Data, TState <: Data, TOut <: Data](
      trigger: Stream[TIn],
      stateType: HardType[TState],
      outputType: HardType[TOut],
      maxIterations: Int,
      init: TIn => TState, // Initialize loop state from trigger
      step: (UInt, TState, TOut) => (TState, Bool) // Assign output, return (next state, isLast)
  ): Stream[TOut] = new Composite(trigger, "streamWhile") {

    val counter = Counter(maxIterations)
    val running = RegInit(False)
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

  }.output
}
