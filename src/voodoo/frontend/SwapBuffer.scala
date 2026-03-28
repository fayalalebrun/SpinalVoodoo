package voodoo.frontend

import voodoo._
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
    val swapCmdEnqueued = in Bool ()

    val waiting = out Bool ()
    val swapCount = out UInt (2 bits)
    val swapsPending = out UInt (3 bits)
  }

  object State extends SpinalEnum {
    val Idle, Sampling, Waiting = newElement()
  }

  val state = RegInit(State.Idle)
  val retraceCounter = Reg(UInt(8 bits)) init (0)
  val swapCountReg = Reg(UInt(2 bits)) init (0)
  val swapsPendingReg = Reg(UInt(3 bits)) init (0)
  val vRetracePrev = RegNext(io.vRetrace) init (False)
  val capturedInterval = Reg(UInt(8 bits)) init (0)

  val vRetraceRising = io.vRetrace && !vRetracePrev
  val swapCompleted = False

  when(io.swapCmdEnqueued && !swapCompleted) {
    swapsPendingReg := swapsPendingReg + 1
  } elsewhen (!io.swapCmdEnqueued && swapCompleted) {
    swapsPendingReg := swapsPendingReg - 1
  }

  io.waiting := state =/= State.Idle
  io.swapCount := swapCountReg
  io.swapsPending := swapsPendingReg
  io.cmd.ready := False

  switch(state) {
    is(State.Idle) {
      when(io.cmd.valid) {
        state := State.Sampling
      }
    }
    is(State.Sampling) {
      when(!io.vsyncEnable) {
        io.cmd.ready := True
        swapCountReg := swapCountReg + 1
        swapCompleted := True
        state := State.Idle
      } otherwise {
        state := State.Waiting
        retraceCounter := 0
        capturedInterval := io.swapInterval
      }
    }
    is(State.Waiting) {
      when(vRetraceRising) {
        when(retraceCounter >= capturedInterval) {
          io.cmd.ready := True
          swapCountReg := swapCountReg + 1
          swapCompleted := True
          state := State.Idle
        } otherwise {
          retraceCounter := retraceCounter + 1
        }
      }
    }
  }

  GenerationFlags.formal {
    val formalReset = ClockDomain.current.isResetActive

    when(!formalReset) {
      when(state === State.Idle) {
        assert(!io.cmd.ready)
      }

      assert(io.cmd.ready === swapCompleted)
      assert(io.waiting === (state =/= State.Idle))
      assert(io.swapCount === swapCountReg)
      assert(io.swapsPending === swapsPendingReg)
    }

    if (formalStrong) {
      when(!formalReset) {
        when(state === State.Sampling) {
          assert(io.cmd.valid)
        }
        when(state === State.Waiting) {
          assert(io.cmd.valid)
          assert(retraceCounter <= capturedInterval)
        }
      }

      cover(io.cmd.ready && state === State.Sampling)
      cover(io.cmd.ready && state === State.Waiting)
      cover(state === State.Waiting && retraceCounter > 0)
    }
  }
}
