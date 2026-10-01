//> using target.scope test

package voodoo.de10

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.formal._

class De10BmbToAvalonMmFormalDut(formalStrong: Boolean) extends Component {
  val multiSourceParams = BmbParameter(
    addressWidth = 10,
    dataWidth = 32,
    sourceWidth = 2,
    contextWidth = 0,
    lengthWidth = 4,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
  val bmbParams = De10BmbToAvalonMm.singleSourceBridgeParams(multiSourceParams)

  val dut = De10BmbToAvalonMm(
    bmbParams = bmbParams,
    avalonAddressWidth = 12,
    addressBase = 0x100,
    addressMask = 0x3ff
  )

  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init False
  val cmdPayload = anyseq(cloneOf(dut.io.bmb.cmd.fragment))

  dut.io.bmb.cmd.valid := anyseq(Bool())
  dut.io.bmb.cmd.fragment := cmdPayload
  dut.io.bmb.cmd.last := anyseq(Bool())
  dut.io.bmb.rsp.ready := anyseq(Bool())
  dut.io.mem.waitRequestn := anyseq(Bool())
  dut.io.mem.readDataValid := anyseq(Bool())
  dut.io.mem.readData := anyseq(Bits(32 bits))

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.bmb.cmd.valid)
    assume(!dut.io.mem.readDataValid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.bmb.cmd.formalAssumesSlave()
  if (formalStrong) {
    dut.io.bmb.rsp.formalAssertsMaster()
  }

  when(
    dut.io.bmb.cmd.valid && !dut.io.bmb.cmd.ready && pastValid && past(
      dut.io.bmb.cmd.valid && !dut.io.bmb.cmd.ready
    )
  ) {
    assume(dut.io.bmb.cmd.fragment.asBits === past(dut.io.bmb.cmd.fragment.asBits))
    assume(dut.io.bmb.cmd.last === past(dut.io.bmb.cmd.last))
  }

  val pendingReadBeats = Reg(UInt(7 bits)) init (0)
  when(dut.io.mem.read && dut.io.mem.waitRequestn) {
    assume(!dut.io.mem.readDataValid)
    assume(dut.io.mem.burstCount =/= 0)
    assume(pendingReadBeats === 0)
    pendingReadBeats := pendingReadBeats + dut.io.mem.burstCount.resize(7)
  }
  when(dut.io.mem.readDataValid) {
    assume(pendingReadBeats =/= 0)
    pendingReadBeats := pendingReadBeats - 1
  }
  when(pendingReadBeats === 0 && !(dut.io.mem.read && dut.io.mem.waitRequestn)) {
    assume(!dut.io.mem.readDataValid)
  }

  when(dut.io.mem.write) {
    assert(dut.io.mem.burstCount === 1)
  }

  val sawMaskedWrite = RegInit(False)
  val sawBurstRead = RegInit(False)
  val sawBurstRead4 = RegInit(False)
  val sawBurstRsp = RegInit(False)
  val sawRspStall = RegInit(False)
  val sawNoBubbleReadRestart = RegInit(False)
  val sawBackToBackWriteBeats = RegInit(False)
  val sawBackToBackReadData = RegInit(False)

  val memReadFire = dut.io.mem.read && dut.io.mem.waitRequestn
  val memWriteFire = dut.io.mem.write && dut.io.mem.waitRequestn

  when(memWriteFire && dut.io.bmb.cmd.mask === B"4'b0101") {
    sawMaskedWrite := True
  }
  when(memReadFire && dut.io.mem.burstCount > 1) {
    sawBurstRead := True
    when(dut.io.mem.burstCount === 4) {
      sawBurstRead4 := True
    }
  }
  when(dut.io.mem.readDataValid && pendingReadBeats === 1) {
    sawBurstRsp := True
  }
  when(dut.io.bmb.rsp.valid && !dut.io.bmb.rsp.ready) {
    sawRspStall := True
  }
  when(dut.io.bmb.rsp.fire && dut.io.bmb.rsp.last && memReadFire) {
    sawNoBubbleReadRestart := True
  }
  when(memWriteFire && pastValid && past(memWriteFire)) {
    sawBackToBackWriteBeats := True
  }
  when(dut.io.mem.readDataValid && pastValid && past(dut.io.mem.readDataValid)) {
    sawBackToBackReadData := True
  }

  cover(sawMaskedWrite)
  cover(sawBurstRead)
  cover(sawBurstRead4 && sawBurstRsp)
  cover(sawBurstRead4 && sawRspStall && dut.io.bmb.rsp.fire && dut.io.bmb.rsp.last)
  cover(sawNoBubbleReadRestart)
  cover(sawBackToBackWriteBeats)
  cover(sawBackToBackReadData)
}

class De10BmbToAvalonMmFormalBmcDut extends De10BmbToAvalonMmFormalDut(formalStrong = true)
class De10BmbToAvalonMmFormalProveDut extends De10BmbToAvalonMmFormalDut(formalStrong = false)
class De10BmbToAvalonMmFormalCoverDut extends De10BmbToAvalonMmFormalDut(formalStrong = true)

class De10MemBackendFormalTest extends SpinalFormalFunSuite {
  test("DE10 burst bridge invariants bmc") {
    FormalConfig
      .withBMC(20)
      .withAsync
      .doVerify(new De10BmbToAvalonMmFormalBmcDut)
  }

  test("DE10 burst bridge invariants k-induction") {
    FormalConfig
      .withProve(20)
      .withAsync
      .doVerify(new De10BmbToAvalonMmFormalProveDut)
  }

  test("DE10 burst bridge cover") {
    FormalConfig
      .withCover(32)
      .withAsync
      .doVerify(new De10BmbToAvalonMmFormalCoverDut)
  }
}
