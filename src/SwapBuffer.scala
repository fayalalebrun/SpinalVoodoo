package voodoo

import spinal.core._
import spinal.lib._

/** SwapBuffer command handler
  *
  * Implements swapbufferCMD (0x128) — the Voodoo1 double-buffer swap command.
  * When vsync is disabled, swap completes immediately.
  * When vsync is enabled, blocks until the retrace counter exceeds the swap interval.
  *
  * The fifoWithSync mechanism ensures the pipeline is idle before SwapBuffer
  * receives the command. While SwapBuffer holds the stream not-ready (waiting
  * for vsync), the FIFO is stalled — no further commands drain.
  *
  * Note: The command stream (from s2mPipe) presents valid one cycle before the
  * register field values are updated. We use a SAMPLING state to wait one cycle
  * after cmd.valid before reading vsyncEnable/swapInterval.
  */
case class SwapBuffer() extends Component {
  val io = new Bundle {
    val cmd             = slave Stream (NoData)
    val vRetrace        = in Bool ()
    val vsyncEnable     = in Bool ()
    val swapInterval    = in UInt (8 bits)
    val swapCmdEnqueued = in Bool () // Pulses when swapbufferCMD enters FIFO

    val waiting      = out Bool ()
    val swapCount    = out UInt (2 bits)
    val swapsPending = out UInt (3 bits)
  }

  // States: IDLE → SAMPLING (1 cycle to let register settle) → WAITING or immediate swap
  val isSampling       = RegInit(False)
  val isWaiting        = RegInit(False)
  val retraceCounter   = Reg(UInt(8 bits)) init (0)
  val swapCountReg     = Reg(UInt(2 bits)) init (0)
  val swapsPendingReg  = Reg(UInt(3 bits)) init (0)
  val vRetracePrev     = RegNext(io.vRetrace) init (False)
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

  io.waiting      := isWaiting || isSampling
  io.swapCount    := swapCountReg
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
}
