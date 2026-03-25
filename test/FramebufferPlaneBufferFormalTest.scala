//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.formal._

class FramebufferPlaneBufferFormalDut(formalStrong: Boolean) extends Component {
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

  val dut = FramebufferPlaneBuffer(c, formalStrong = formalStrong)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  val writeReqPayload = anyseq(cloneOf(dut.io.writeReq.payload))
  val memRspFragment = anyseq(cloneOf(dut.io.mem.rsp.fragment))

  dut.io.readReq.valid := False
  dut.io.readReq.address := 0
  dut.io.writeReq.valid := anyseq(Bool())
  dut.io.writeReq.payload := writeReqPayload
  dut.io.readRsp.ready := True
  dut.io.flush := anyseq(Bool())
  dut.io.mem.cmd.ready := anyseq(Bool())
  dut.io.mem.rsp.valid := anyseq(Bool())
  dut.io.mem.rsp.fragment := memRspFragment
  dut.io.mem.rsp.last := anyseq(Bool())

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.writeReq.valid)
    assume(!dut.io.flush)
    assume(!dut.io.mem.rsp.valid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.writeReq.formalAssumesSlave()

  if (formalStrong) {
    val writeRspOutstanding = Reg(Bool()) init (False)

    when(dut.io.mem.cmd.fire) {
      assert(dut.io.mem.cmd.fragment.opcode === Bmb.Cmd.Opcode.WRITE)
      when(dut.io.mem.cmd.last) {
        writeRspOutstanding := True
      }
    }

    when(writeRspOutstanding) {
      assume(!dut.io.mem.rsp.valid || dut.io.mem.rsp.fragment.source === 1)
    }

    when(dut.io.mem.rsp.fire) {
      when(dut.io.mem.rsp.fragment.source === 1) {
        writeRspOutstanding := False
      }
    }
    assert(!dut.io.readRsp.valid)
  }
}

class FramebufferPlaneBufferCoverageDut extends Component {
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

class FramebufferPlaneBufferFormalBmcDut
    extends FramebufferPlaneBufferFormalDut(formalStrong = true)

class FramebufferPlaneBufferFormalProveDut
    extends FramebufferPlaneBufferFormalDut(formalStrong = false)

class FramebufferPlaneBufferFormalTest extends SpinalFormalFunSuite {
  test("Framebuffer plane buffer invariants bmc") {
    FormalConfig
      .withBMC(10)
      .withAsync
      .doVerify(new FramebufferPlaneBufferFormalBmcDut)
  }

  test("Framebuffer plane buffer invariants prove") {
    FormalConfig
      .withProve(10)
      .withAsync
      .doVerify(new FramebufferPlaneBufferFormalProveDut)
  }

  test("Framebuffer plane buffer throughput cover") {
    FormalConfig
      .withProve(12)
      .withBMC(12)
      .withAsync
      .doVerify(new FramebufferPlaneBufferCoverageDut)
  }
}
