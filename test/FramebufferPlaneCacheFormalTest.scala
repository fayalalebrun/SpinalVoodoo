//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.formal._

class FramebufferPlaneCacheFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 8 bits,
      memBurstLengthWidth = 4,
      fbFillLineWords = 2,
      useFbFillCache = true,
      useTexFillCache = false
    )

  val dut = FramebufferPlaneCache(c, formalStrong = formalStrong)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  val readReqPayload = anyseq(cloneOf(dut.io.readReq.payload))
  val writeReqPayload = anyseq(cloneOf(dut.io.writeReq.payload))
  val memRspFragment = anyseq(cloneOf(dut.io.mem.rsp.fragment))

  dut.io.readReq.valid := anyseq(Bool())
  dut.io.readReq.payload := readReqPayload
  dut.io.writeReq.valid := anyseq(Bool())
  dut.io.writeReq.payload := writeReqPayload
  dut.io.readRsp.ready := anyseq(Bool())
  dut.io.flush := anyseq(Bool())
  dut.io.mem.cmd.ready := anyseq(Bool())
  dut.io.mem.rsp.valid := anyseq(Bool())
  dut.io.mem.rsp.fragment := memRspFragment
  dut.io.mem.rsp.last := anyseq(Bool())

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.readReq.valid)
    assume(!dut.io.writeReq.valid)
    assume(!dut.io.flush)
    assume(!dut.io.mem.rsp.valid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.readReq.formalAssumesSlave()
  dut.io.writeReq.formalAssumesSlave()
  if (formalStrong) {
    dut.io.readRsp.formalAssertsMaster()
  }

  if (formalStrong) {
    val readOutstanding = Reg(UInt(log2Up(c.fbFillLineWords + 1) bits)) init (0)
    val writeRspOutstanding = Reg(Bool()) init (False)
    val readPending = Reg(Bool()) init (False)

    when(dut.io.mem.cmd.fire) {
      when(dut.io.mem.cmd.fragment.opcode === Bmb.Cmd.Opcode.READ) {
        assert(readOutstanding === 0)
        assert(!writeRspOutstanding)
        readOutstanding := U(c.fbFillLineWords, readOutstanding.getWidth bits)
      } otherwise {
        when(dut.io.mem.cmd.last) {
          writeRspOutstanding := True
        }
      }
    }

    when(
      dut.io.mem.rsp.valid && !dut.io.mem.rsp.ready && pastValid && past(
        dut.io.mem.rsp.valid && !dut.io.mem.rsp.ready
      )
    ) {
      assume(dut.io.mem.rsp.fragment.asBits === past(dut.io.mem.rsp.fragment.asBits))
      assume(dut.io.mem.rsp.last === past(dut.io.mem.rsp.last))
    }

    when(readOutstanding === 0 && !writeRspOutstanding) {
      assume(!dut.io.mem.rsp.valid)
    }
    when(dut.io.mem.rsp.valid) {
      assume(readOutstanding =/= 0 || writeRspOutstanding)
      when(readOutstanding =/= 0) {
        assume(dut.io.mem.rsp.last === (readOutstanding === 1))
      }
    }

    when(dut.io.mem.rsp.fire) {
      when(readOutstanding =/= 0) {
        readOutstanding := readOutstanding - 1
      } otherwise {
        assert(writeRspOutstanding)
        writeRspOutstanding := False
      }
    }

    when(dut.io.readReq.fire) {
      assert(!readPending)
      readPending := True
    }
    when(dut.io.readRsp.fire) {
      assert(readPending)
      readPending := False
    }
    when(readPending) {
      assert(!dut.io.readReq.ready)
    }
    when(dut.io.readRsp.valid) {
      assert(readPending)
    }

    when(pastValid && past(dut.io.readRsp.valid && !dut.io.readRsp.ready)) {
      assert(dut.io.readRsp.valid)
      assert(dut.io.readRsp.data === past(dut.io.readRsp.data))
    }
  }
}

class FramebufferPlaneCacheFormalBmcDut extends FramebufferPlaneCacheFormalDut(formalStrong = true)

class FramebufferPlaneCacheFormalProveDut
    extends FramebufferPlaneCacheFormalDut(formalStrong = false)

class FramebufferPlaneCacheFormalTest extends SpinalFormalFunSuite {
  test("Framebuffer plane cache invariants bmc") {
    FormalConfig
      .withBMC(10)
      .withAsync
      .doVerify(new FramebufferPlaneCacheFormalBmcDut)
  }

  test("Framebuffer plane cache invariants prove") {
    FormalConfig
      .withProve(10)
      .withAsync
      .doVerify(new FramebufferPlaneCacheFormalProveDut)
  }
}
