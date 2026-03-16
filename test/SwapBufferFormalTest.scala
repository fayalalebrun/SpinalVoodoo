//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.formal._

class SwapBufferFormalDut(formalStrong: Boolean) extends Component {
  val dut = SwapBuffer(formalStrong = formalStrong)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  dut.io.cmd.valid := anyseq(Bool())
  dut.io.vRetrace := anyseq(Bool())
  dut.io.vsyncEnable := anyseq(Bool())
  dut.io.swapInterval := anyseq(UInt(8 bits))
  dut.io.swapCmdEnqueued := anyseq(Bool())

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.cmd.valid)
    assume(!dut.io.swapCmdEnqueued)
  }
  when(pastValid) {
    assume(!reset)
  }

  // Stream protocol: master holds valid until fire
  dut.io.cmd.formalAssumesSlave()
}

class SwapBufferFormalBmcDut extends SwapBufferFormalDut(formalStrong = true)

class SwapBufferFormalProveDut extends SwapBufferFormalDut(formalStrong = false)

class SwapBufferFormalTest extends SpinalFormalFunSuite {
  test("SwapBuffer invariants bmc") {
    FormalConfig
      .withBMC(20)
      .withAsync
      .doVerify(new SwapBufferFormalBmcDut)
  }

  test("SwapBuffer invariants prove") {
    FormalConfig
      .withProve(10)
      .withAsync
      .doVerify(new SwapBufferFormalProveDut)
  }
}
