//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.formal._

class TmuTextureCacheFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 12 bits,
      memBurstLengthWidth = 5,
      useTexFillCache = true,
      texFillLineWords = 4,
      texFillCacheSlots = 2,
      texFillRequestWindow = 2,
      packedTexLayout = false
    )

  val dut = TmuTextureCache(c, formalStrong = formalStrong)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  val sampleRequestPayload = anyseq(cloneOf(dut.io.sampleRequest.payload))
  val texRspFragment = anyseq(cloneOf(dut.io.texRead.rsp.fragment))

  dut.io.sampleRequest.valid := anyseq(Bool())
  dut.io.sampleRequest.payload := sampleRequestPayload
  dut.io.fetched.ready := anyseq(Bool())
  dut.io.fastFetch.ready := anyseq(Bool())
  dut.io.texRead.cmd.ready := anyseq(Bool())
  dut.io.texRead.rsp.valid := anyseq(Bool())
  dut.io.texRead.rsp.fragment := texRspFragment
  dut.io.texRead.rsp.last := anyseq(Bool())

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.sampleRequest.valid)
    assume(!dut.io.texRead.rsp.valid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.sampleRequest.formalAssumesSlave()
  if (formalStrong) {
    dut.io.fetched.formalAssertsMaster()
    dut.io.fastFetch.formalAssertsMaster()
  }

  if (formalStrong) {
    val readOutstanding = Reg(UInt(log2Up(c.texFillLineWords + 1) bits)) init (0)

    when(dut.io.texRead.cmd.fire) {
      assert(readOutstanding === 0)
      readOutstanding := U(c.texFillLineWords, readOutstanding.getWidth bits)
    }

    when(
      dut.io.texRead.rsp.valid && !dut.io.texRead.rsp.ready && pastValid && past(
        dut.io.texRead.rsp.valid && !dut.io.texRead.rsp.ready
      )
    ) {
      assume(dut.io.texRead.rsp.fragment.asBits === past(dut.io.texRead.rsp.fragment.asBits))
      assume(dut.io.texRead.rsp.last === past(dut.io.texRead.rsp.last))
    }

    when(readOutstanding === 0) {
      assume(!dut.io.texRead.rsp.valid)
    }
    when(dut.io.texRead.rsp.valid) {
      assume(readOutstanding =/= 0)
      assume(dut.io.texRead.rsp.last === (readOutstanding === 1))
    }

    when(dut.io.texRead.rsp.fire) {
      readOutstanding := readOutstanding - 1
    }

    when(dut.io.texRead.cmd.valid) {
      assert(dut.io.texRead.cmd.fragment.opcode === Bmb.Cmd.Opcode.READ)
      assert(dut.io.texRead.cmd.last)
    }
  }
}

class TmuTextureCacheFormalBmcDut extends TmuTextureCacheFormalDut(formalStrong = true)

class TmuTextureCacheFormalProveDut extends TmuTextureCacheFormalDut(formalStrong = false)

class TmuTextureCacheFormalTest extends SpinalFormalFunSuite {
  test("TMU-specific texture cache invariants bmc") {
    FormalConfig
      .withBMC(14)
      .withAsync
      .doVerify(new TmuTextureCacheFormalBmcDut)
  }

  test("TMU-specific texture cache invariants prove") {
    FormalConfig
      .withProve(14)
      .withAsync
      .doVerify(new TmuTextureCacheFormalProveDut)
  }
}
