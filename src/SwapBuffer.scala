package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._

/** SwapBuffer command handler
  *
  * Implements swapbufferCMD (0x128) — the Voodoo1 double-buffer swap command. When vsync is
  * disabled, swap completes immediately. When vsync is enabled, blocks until the retrace counter
  * exceeds the swap interval.
  *
  * The fifoWithSync mechanism ensures the pipeline is idle before SwapBuffer receives the command.
  * While SwapBuffer holds the stream not-ready (waiting for vsync), the FIFO is stalled — no
  * further commands drain.
  *
  * Note: The command stream (from s2mPipe) presents valid one cycle before the register field
  * values are updated. We use a SAMPLING state to wait one cycle after cmd.valid before reading
  * vsyncEnable/swapInterval.
  */
case class SwapBuffer(formalStrong: Boolean = false) extends Component {
  val io = new Bundle {
    val cmd = slave Stream (NoData)
    val vRetrace = in Bool ()
    val vsyncEnable = in Bool ()
    val swapInterval = in UInt (8 bits)
    val swapCmdEnqueued = in Bool () // Pulses when swapbufferCMD enters FIFO

    val waiting = out Bool ()
    val swapCount = out UInt (2 bits)
    val swapsPending = out UInt (3 bits)
  }

  // States: IDLE → SAMPLING (1 cycle to let register settle) → WAITING or immediate swap
  val isSampling = RegInit(False)
  val isWaiting = RegInit(False)
  val retraceCounter = Reg(UInt(8 bits)) init (0)
  val swapCountReg = Reg(UInt(2 bits)) init (0)
  val swapsPendingReg = Reg(UInt(3 bits)) init (0)
  val vRetracePrev = RegNext(io.vRetrace) init (False)
  val capturedInterval = Reg(UInt(8 bits)) init (0)

  val vRetraceRising = io.vRetrace && !vRetracePrev

  // swapsPending: increment when command enters FIFO, decrement when swap completes.
  // Per SST-1 spec 5.24: "When a swapbufferCMD is received in the front-end PCI
  // host FIFO, the swap buffers pending field in the status register is incremented."
  val swapCompleted = False
  when(io.swapCmdEnqueued && !swapCompleted) {
    swapsPendingReg := swapsPendingReg + 1
  } elsewhen (!io.swapCmdEnqueued && swapCompleted) {
    swapsPendingReg := swapsPendingReg - 1
  }
  // If both happen on the same cycle (impossible in practice since the command must
  // traverse the FIFO), they cancel out and swapsPendingReg stays unchanged.

  io.waiting := isWaiting || isSampling
  io.swapCount := swapCountReg
  io.swapsPending := swapsPendingReg

  io.cmd.ready := False

  when(!isWaiting && !isSampling) {
    // IDLE: accept command and enter sampling state
    when(io.cmd.valid) {
      isSampling := True
    }
  } elsewhen (isSampling) {
    // SAMPLING: register fields are now stable — read them
    isSampling := False
    when(!io.vsyncEnable) {
      // Immediate swap — no vsync wait
      io.cmd.ready := True
      swapCountReg := swapCountReg + 1
      swapCompleted := True
    } otherwise {
      // Enter waiting state — block until retrace condition met
      isWaiting := True
      retraceCounter := 0
      capturedInterval := io.swapInterval
    }
  } otherwise {
    // WAITING: count vRetrace rising edges, swap when count exceeds interval
    when(vRetraceRising) {
      when(retraceCounter >= capturedInterval) {
        // Retrace count exceeds interval (>= with check-before-increment
        // matches 86Box's > with increment-before-check) — swap now
        io.cmd.ready := True
        isWaiting := False
        swapCountReg := swapCountReg + 1
        swapCompleted := True
      } otherwise {
        retraceCounter := retraceCounter + 1
      }
    }
  }

  GenerationFlags.formal {
    val formalReset = ClockDomain.current.isResetActive

    when(!formalReset) {
      // --- Combinational / tautological properties (induction-safe) ---
      // cmd.ready defaults to False; only overridden in SAMPLING or WAITING branches
      when(!isSampling && !isWaiting) {
        assert(!io.cmd.ready)
      }

      // cmd.ready and swapCompleted are wired identically (same assignments in every branch)
      assert(io.cmd.ready === swapCompleted)

      // Output port wiring correctness
      assert(io.waiting === (isWaiting || isSampling))
      assert(io.swapCount === swapCountReg)
      assert(io.swapsPending === swapsPendingReg)
    }

    if (formalStrong) {
      when(!formalReset) {
        // --- State machine well-formedness (BMC-only) ---
        // States are mutually exclusive: never both sampling and waiting
        assert(!(isSampling && isWaiting))

        // --- Stream protocol properties (BMC-only) ---
        // SAMPLING and WAITING states always have cmd.valid held high
        // (entered via cmd.valid in IDLE without consuming; Stream holds valid until fire)
        when(isSampling) {
          assert(io.cmd.valid)
        }
        when(isWaiting) {
          assert(io.cmd.valid)
        }

        // --- Counter bounds (BMC-only) ---
        // Retrace counter never exceeds captured interval while waiting
        when(isWaiting) {
          assert(retraceCounter <= capturedInterval)
        }
      }

      // --- Reachability covers ---
      // Immediate swap path reachable (vsync disabled)
      cover(io.cmd.ready && isSampling)
      // Vsync swap path reachable (waited for retrace)
      cover(io.cmd.ready && !isSampling)
      // Waiting state with nonzero retrace count reachable
      cover(isWaiting && retraceCounter > 0)
    }
  }
}
