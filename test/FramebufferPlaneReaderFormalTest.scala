//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib.formal._

class FramebufferPlaneReaderFormalDut extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 8 bits,
      memBurstLengthWidth = 6,
      fbWriteBufferLineWords = 4,
      fbWriteBufferCount = 2,
      useFbWriteBuffer = true,
      useTexFillCache = false
    )

  val dut = FramebufferPlaneReader(c, prefetchWords = 8)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  dut.io.prefetchReq.valid := anyseq(Bool())
  dut.io.prefetchReq.startAddress := anyseq(UInt(c.addressWidth))
  dut.io.prefetchReq.endAddress := anyseq(UInt(c.addressWidth))
  dut.io.readReq.valid := anyseq(Bool())
  dut.io.readReq.address := anyseq(UInt(c.addressWidth))
  dut.io.readRsp.ready := anyseq(Bool())
  dut.io.mem.cmd.ready := anyseq(Bool())
  dut.io.mem.rsp.valid := False
  dut.io.mem.rsp.fragment.data := 0
  dut.io.mem.rsp.fragment.source := 0
  dut.io.mem.rsp.fragment.context := 0
  dut.io.mem.rsp.last := True

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.readReq.valid)
    assume(!dut.io.prefetchReq.valid)
  }
  when(pastValid) {
    assume(!reset)
  }
  when(dut.io.prefetchReq.valid) {
    assume(dut.io.prefetchReq.startAddress <= dut.io.prefetchReq.endAddress)
  }

  dut.io.readReq.formalAssumesSlave()
  cover(!reset && dut.io.mem.cmd.fire)
}

class FramebufferPlaneReaderFormalTest extends SpinalFormalFunSuite {
  test("Framebuffer plane reader issue policy") {
    FormalConfig
      .withBMC(32)
      .withProve(32)
      .withAsync
      .doVerify(new FramebufferPlaneReaderFormalDut)
  }
}
