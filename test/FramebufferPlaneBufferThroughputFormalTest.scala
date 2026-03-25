//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.formal._

class FramebufferPlaneBufferThroughputFormalDut extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 8 bits,
      memBurstLengthWidth = 4,
      fbWriteBufferLineWords = 2,
      fbWriteBufferCount = 2,
      useFbWriteBuffer = true,
      useTexFillCache = false
    )

  val dut = FramebufferPlaneBuffer(c, formalStrong = false)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)
  val sawFirstWord = Reg(Bool()) init (False)
  val sawFullSequentialLine = Reg(Bool()) init (False)

  dut.io.readReq.valid := False
  dut.io.readReq.address := 0
  dut.io.readRsp.ready := True
  dut.io.flush := False

  dut.io.writeReq.valid := anyseq(Bool())
  dut.io.writeReq.payload := anyseq(cloneOf(dut.io.writeReq.payload))

  val rspValid = RegNext(dut.io.mem.cmd.fire && dut.io.mem.cmd.last) init (False)
  val rspSource =
    RegNextWhen(dut.io.mem.cmd.fragment.source, dut.io.mem.cmd.fire && dut.io.mem.cmd.last) init (0)
  dut.io.mem.cmd.ready := True
  dut.io.mem.rsp.valid := rspValid
  dut.io.mem.rsp.fragment.data := 0
  dut.io.mem.rsp.fragment.source := rspSource
  dut.io.mem.rsp.fragment.context := 0
  dut.io.mem.rsp.last := True

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.writeReq.valid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.writeReq.formalAssumesSlave()

  when(reset) {
    sawFirstWord := False
    sawFullSequentialLine := False
  }.elsewhen(dut.io.writeReq.fire && dut.io.writeReq.address === U(0, c.addressWidth.value bits)) {
    sawFirstWord := True
  }.elsewhen(
    sawFirstWord && dut.io.writeReq.fire && dut.io.writeReq.address === U(
      4,
      c.addressWidth.value bits
    )
  ) {
    sawFullSequentialLine := True
  }

  cover(!reset && dut.io.writeReq.fire)
  cover(
    !reset &&
      sawFullSequentialLine &&
      dut.io.mem.cmd.fire &&
      dut.io.mem.cmd.fragment.source === 1
  )
}

class FramebufferPlaneBufferThroughputFormalTest extends SpinalFormalFunSuite {
  test("write throughput property prove") {
    FormalConfig
      .withProve(12)
      .withBMC(12)
      .withAsync
      .doVerify(new FramebufferPlaneBufferThroughputFormalDut)
  }
}
