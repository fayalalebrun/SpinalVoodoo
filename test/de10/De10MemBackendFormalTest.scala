//> using target.scope test

package voodoo.de10

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.formal._
import voodoo.{Config, TraceConfig}

class H2fLwToBmbBridgeFormalDut(formalStrong: Boolean) extends Component {
  val dut = H2fLwToBmbBridge(24, voodoo.Core.cpuBmbParams)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init False

  dut.io.h2fLw.address := anyseq(UInt(22 bits))
  dut.io.h2fLw.read := anyseq(Bool())
  dut.io.h2fLw.write := anyseq(Bool())
  dut.io.h2fLw.byteenable := anyseq(Bits(4 bits))
  dut.io.h2fLw.writedata := anyseq(Bits(32 bits))
  dut.io.cpuBus.cmd.ready := anyseq(Bool())
  dut.io.cpuBus.rsp.valid := anyseq(Bool())
  dut.io.cpuBus.rsp.last := anyseq(Bool())
  dut.io.cpuBus.rsp.source := anyseq(UInt(voodoo.Core.cpuBmbParams.access.sourceWidth bits))
  dut.io.cpuBus.rsp.opcode := anyseq(Bits(1 bits))
  dut.io.cpuBus.rsp.data := anyseq(Bits(32 bits))

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.h2fLw.read)
    assume(!dut.io.h2fLw.write)
    assume(!dut.io.cpuBus.rsp.valid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.cpuBus.rsp.formalAssumesSlave()
  when(
    (dut.io.h2fLw.read || dut.io.h2fLw.write) && dut.io.h2fLw.waitrequest && pastValid && past(
      dut.io.h2fLw.read || dut.io.h2fLw.write
    ) && past(dut.io.h2fLw.waitrequest)
  ) {
    assume(dut.io.h2fLw.address === past(dut.io.h2fLw.address))
    assume(dut.io.h2fLw.read === past(dut.io.h2fLw.read))
    assume(dut.io.h2fLw.write === past(dut.io.h2fLw.write))
    assume(dut.io.h2fLw.byteenable === past(dut.io.h2fLw.byteenable))
    assume(dut.io.h2fLw.writedata === past(dut.io.h2fLw.writedata))
  }

  val cmdAccepted = dut.io.cpuBus.cmd.valid && dut.io.cpuBus.cmd.ready
  val pendingCmd = RegInit(False)

  when(reset) {
    pendingCmd := False
  } otherwise {
    when(cmdAccepted && !dut.io.cpuBus.rsp.valid) {
      pendingCmd := True
    }
    when(dut.io.cpuBus.rsp.valid) {
      assume(pendingCmd || cmdAccepted)
      assume(dut.io.cpuBus.rsp.last)
      assume(dut.io.cpuBus.rsp.source === 0)
      pendingCmd := False
    }
  }

  val readRspSeen = RegInit(False)
  val writeSeen = RegInit(False)
  val mixedSeen = RegInit(False)
  when(dut.io.cpuBus.cmd.fire && dut.io.cpuBus.cmd.opcode === Bmb.Cmd.Opcode.WRITE) {
    writeSeen := True
  }
  when(dut.io.h2fLw.read && dut.io.h2fLw.write) {
    mixedSeen := True
  }
  when(dut.io.h2fLw.readdatavalid) {
    readRspSeen := True
  }

  cover(writeSeen)
  cover(mixedSeen && dut.io.cpuBus.cmd.fire)
  cover(readRspSeen)
}

class H2fLwToBmbBridgeFormalBmcDut extends H2fLwToBmbBridgeFormalDut(formalStrong = true)
class H2fLwToBmbBridgeFormalProveDut extends H2fLwToBmbBridgeFormalDut(formalStrong = true)
class H2fLwToBmbBridgeFormalCoverDut extends H2fLwToBmbBridgeFormalDut(formalStrong = true)

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

class De10MemBackendFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 10 bits,
      memBurstLengthWidth = 4,
      useFbWriteBuffer = false,
      useTexFillCache = false
    )

  val dut = De10MemBackend(c)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init False

  val fbCmdPayload = anyseq(cloneOf(dut.io.fbMemWrite.cmd.fragment))
  val texCmdPayload = anyseq(cloneOf(dut.io.texMem.cmd.fragment))

  dut.io.fbMemWrite.cmd.valid := anyseq(Bool())
  dut.io.fbMemWrite.cmd.fragment := fbCmdPayload
  dut.io.fbMemWrite.cmd.last := anyseq(Bool())
  dut.io.fbMemWrite.rsp.ready := anyseq(Bool())

  dut.io.texMem.cmd.valid := anyseq(Bool())
  dut.io.texMem.cmd.fragment := texCmdPayload
  dut.io.texMem.cmd.last := anyseq(Bool())
  dut.io.texMem.rsp.ready := anyseq(Bool())

  dut.io.fbColorReadMem.cmd.valid := False
  dut.io.fbColorReadMem.cmd.fragment.assignDontCare()
  dut.io.fbColorReadMem.cmd.last := True
  dut.io.fbColorReadMem.rsp.ready := True

  dut.io.fbColorWriteMem.cmd.valid := False
  dut.io.fbColorWriteMem.cmd.fragment.assignDontCare()
  dut.io.fbColorWriteMem.cmd.last := True
  dut.io.fbColorWriteMem.rsp.ready := True

  dut.io.fbAuxWriteMem.cmd.valid := False
  dut.io.fbAuxWriteMem.cmd.fragment.assignDontCare()
  dut.io.fbAuxWriteMem.cmd.last := True
  dut.io.fbAuxWriteMem.rsp.ready := True

  dut.io.fbAuxReadMem.cmd.valid := False
  dut.io.fbAuxReadMem.cmd.fragment.assignDontCare()
  dut.io.fbAuxReadMem.cmd.last := True
  dut.io.fbAuxReadMem.rsp.ready := True

  dut.io.memFbWrite.waitRequestn := anyseq(Bool())
  dut.io.memFbWrite.readDataValid := anyseq(Bool())
  dut.io.memFbWrite.readData := anyseq(Bits(32 bits))
  dut.io.memFbColorWrite.waitRequestn := True
  dut.io.memFbColorWrite.readDataValid := False
  dut.io.memFbColorWrite.readData := 0
  dut.io.memFbAuxWrite.waitRequestn := True
  dut.io.memFbAuxWrite.readDataValid := False
  dut.io.memFbAuxWrite.readData := 0
  dut.io.memFbColorRead.waitRequestn := True
  dut.io.memFbColorRead.readDataValid := False
  dut.io.memFbColorRead.readData := 0
  dut.io.memFbAuxRead.waitRequestn := True
  dut.io.memFbAuxRead.readDataValid := False
  dut.io.memFbAuxRead.readData := 0
  dut.io.memTex.waitRequestn := anyseq(Bool())
  dut.io.memTex.readDataValid := anyseq(Bool())
  dut.io.memTex.readData := anyseq(Bits(32 bits))

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.fbMemWrite.cmd.valid)
    assume(!dut.io.texMem.cmd.valid)
    assume(!dut.io.memFbWrite.readDataValid)
    assume(!dut.io.memTex.readDataValid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.fbMemWrite.cmd.formalAssumesSlave()
  dut.io.texMem.cmd.formalAssumesSlave()
  if (formalStrong) {
    dut.io.fbMemWrite.rsp.formalAssertsMaster()
    dut.io.texMem.rsp.formalAssertsMaster()
  }

  when(
    dut.io.fbMemWrite.cmd.valid && !dut.io.fbMemWrite.cmd.ready && pastValid && past(
      dut.io.fbMemWrite.cmd.valid && !dut.io.fbMemWrite.cmd.ready
    )
  ) {
    assume(dut.io.fbMemWrite.cmd.fragment.asBits === past(dut.io.fbMemWrite.cmd.fragment.asBits))
    assume(dut.io.fbMemWrite.cmd.last === past(dut.io.fbMemWrite.cmd.last))
  }
  when(
    dut.io.texMem.cmd.valid && !dut.io.texMem.cmd.ready && pastValid && past(
      dut.io.texMem.cmd.valid && !dut.io.texMem.cmd.ready
    )
  ) {
    assume(dut.io.texMem.cmd.fragment.asBits === past(dut.io.texMem.cmd.fragment.asBits))
    assume(dut.io.texMem.cmd.last === past(dut.io.texMem.cmd.last))
  }

  val fbPendingReadBeats = Reg(UInt(7 bits)) init (0)
  when(dut.io.memFbWrite.read && dut.io.memFbWrite.waitRequestn) {
    assume(!dut.io.memFbWrite.readDataValid)
    assume(fbPendingReadBeats === 0)
    fbPendingReadBeats := fbPendingReadBeats + dut.io.memFbWrite.burstCount.resize(7)
  }
  when(dut.io.memFbWrite.readDataValid) {
    assume(fbPendingReadBeats =/= 0)
    fbPendingReadBeats := fbPendingReadBeats - 1
  }
  when(fbPendingReadBeats === 0 && !(dut.io.memFbWrite.read && dut.io.memFbWrite.waitRequestn)) {
    assume(!dut.io.memFbWrite.readDataValid)
  }

  val texPendingReadBeats = Reg(UInt(7 bits)) init (0)
  when(dut.io.memTex.read && dut.io.memTex.waitRequestn) {
    assume(!dut.io.memTex.readDataValid)
    assume(texPendingReadBeats === 0)
    texPendingReadBeats := texPendingReadBeats + dut.io.memTex.burstCount.resize(7)
  }
  when(dut.io.memTex.readDataValid) {
    assume(texPendingReadBeats =/= 0)
    texPendingReadBeats := texPendingReadBeats - 1
  }
  when(texPendingReadBeats === 0 && !(dut.io.memTex.read && dut.io.memTex.waitRequestn)) {
    assume(!dut.io.memTex.readDataValid)
  }

  assert(!(dut.io.memFbWrite.read && dut.io.memFbWrite.write))
  assert(!(dut.io.memTex.read && dut.io.memTex.write))
  when(dut.io.memFbWrite.read) {
    assert(dut.io.memFbWrite.waitRequestn)
  }
  when(dut.io.memTex.read) {
    assert(dut.io.memTex.waitRequestn)
  }
  assert(De10AddressMap.fbMemBase + De10AddressMap.fbMemBytes <= De10AddressMap.texMemBase)
  assert(De10AddressMap.texMemBase + De10AddressMap.texMemBytes <= De10AddressMap.ddrCarveoutEnd)

  when(dut.io.memFbWrite.read || dut.io.memFbWrite.write) {
    assert(dut.io.memFbWrite.address >= U(De10AddressMap.fbMemBase, 32 bits))
    assert(
      dut.io.memFbWrite.address < U(De10AddressMap.fbMemBase + De10AddressMap.fbMemBytes, 32 bits)
    )
  }
  when(dut.io.memTex.read || dut.io.memTex.write) {
    assert(dut.io.memTex.address >= U(De10AddressMap.texMemBase, 32 bits))
    assert(
      dut.io.memTex.address < U(De10AddressMap.texMemBase + De10AddressMap.texMemBytes, 32 bits)
    )
  }

  val sawFbBurstRead = RegInit(False)
  val sawTexBurstRead = RegInit(False)
  val sawConcurrentTraffic = RegInit(False)
  when(
    dut.io.memFbWrite.read && dut.io.memFbWrite.waitRequestn && dut.io.memFbWrite.burstCount > 1
  ) {
    sawFbBurstRead := True
  }
  when(dut.io.memTex.read && dut.io.memTex.waitRequestn && dut.io.memTex.burstCount > 1) {
    sawTexBurstRead := True
  }
  when(
    (dut.io.memFbWrite.read || dut.io.memFbWrite.write) &&
      (dut.io.memTex.read || dut.io.memTex.write)
  ) {
    sawConcurrentTraffic := True
  }
  cover(sawFbBurstRead)
  cover(sawTexBurstRead)
  cover(sawConcurrentTraffic)
  cover(sawFbBurstRead && sawTexBurstRead)
}

class De10MemBackendFormalBmcDut extends De10MemBackendFormalDut(formalStrong = true)
class De10MemBackendFormalProveDut extends De10MemBackendFormalDut(formalStrong = false)
class De10MemBackendFormalCoverDut extends De10MemBackendFormalDut(formalStrong = true)

class De10MemBackendAuxReadFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 10 bits,
      memBurstLengthWidth = 4,
      useFbWriteBuffer = true,
      useTexFillCache = false
    )

  val dut = De10MemBackend(c)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init False

  val auxCmdPayload = anyseq(cloneOf(dut.io.fbAuxReadMem.cmd.fragment))

  dut.io.fbAuxReadMem.cmd.valid := anyseq(Bool())
  dut.io.fbAuxReadMem.cmd.fragment := auxCmdPayload
  dut.io.fbAuxReadMem.cmd.last := anyseq(Bool())
  dut.io.fbAuxReadMem.rsp.ready := anyseq(Bool())

  dut.io.fbMemWrite.cmd.valid := False
  dut.io.fbMemWrite.cmd.fragment.assignDontCare()
  dut.io.fbMemWrite.cmd.last := True
  dut.io.fbMemWrite.rsp.ready := True

  dut.io.fbColorWriteMem.cmd.valid := False
  dut.io.fbColorWriteMem.cmd.fragment.assignDontCare()
  dut.io.fbColorWriteMem.cmd.last := True
  dut.io.fbColorWriteMem.rsp.ready := True

  dut.io.fbAuxWriteMem.cmd.valid := False
  dut.io.fbAuxWriteMem.cmd.fragment.assignDontCare()
  dut.io.fbAuxWriteMem.cmd.last := True
  dut.io.fbAuxWriteMem.rsp.ready := True

  dut.io.fbColorReadMem.cmd.valid := False
  dut.io.fbColorReadMem.cmd.fragment.assignDontCare()
  dut.io.fbColorReadMem.cmd.last := True
  dut.io.fbColorReadMem.rsp.ready := True

  dut.io.texMem.cmd.valid := False
  dut.io.texMem.cmd.fragment.assignDontCare()
  dut.io.texMem.cmd.last := True
  dut.io.texMem.rsp.ready := True

  dut.io.memFbWrite.waitRequestn := True
  dut.io.memFbWrite.readDataValid := False
  dut.io.memFbWrite.readData := 0
  dut.io.memFbColorWrite.waitRequestn := True
  dut.io.memFbColorWrite.readDataValid := False
  dut.io.memFbColorWrite.readData := 0
  dut.io.memFbAuxWrite.waitRequestn := True
  dut.io.memFbAuxWrite.readDataValid := False
  dut.io.memFbAuxWrite.readData := 0
  dut.io.memFbColorRead.waitRequestn := True
  dut.io.memFbColorRead.readDataValid := False
  dut.io.memFbColorRead.readData := 0
  dut.io.memFbAuxRead.waitRequestn := anyseq(Bool())
  dut.io.memFbAuxRead.readDataValid := anyseq(Bool())
  dut.io.memFbAuxRead.readData := anyseq(Bits(32 bits))
  dut.io.memTex.waitRequestn := True
  dut.io.memTex.readDataValid := False
  dut.io.memTex.readData := 0

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.fbAuxReadMem.cmd.valid)
    assume(!dut.io.memFbAuxRead.readDataValid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.fbAuxReadMem.cmd.formalAssumesSlave()
  if (formalStrong) {
    dut.io.fbAuxReadMem.rsp.formalAssertsMaster()
  }

  when(
    dut.io.fbAuxReadMem.cmd.valid && !dut.io.fbAuxReadMem.cmd.ready && pastValid && past(
      dut.io.fbAuxReadMem.cmd.valid && !dut.io.fbAuxReadMem.cmd.ready
    )
  ) {
    assume(
      dut.io.fbAuxReadMem.cmd.fragment.asBits === past(dut.io.fbAuxReadMem.cmd.fragment.asBits)
    )
    assume(dut.io.fbAuxReadMem.cmd.last === past(dut.io.fbAuxReadMem.cmd.last))
  }

  val auxPendingReadBeats = Reg(UInt(7 bits)) init (0)
  when(dut.io.memFbAuxRead.read && dut.io.memFbAuxRead.waitRequestn) {
    assume(!dut.io.memFbAuxRead.readDataValid)
    assume(auxPendingReadBeats === 0)
    auxPendingReadBeats := auxPendingReadBeats + dut.io.memFbAuxRead.burstCount.resize(7)
  }
  when(dut.io.memFbAuxRead.readDataValid) {
    assume(auxPendingReadBeats =/= 0)
    auxPendingReadBeats := auxPendingReadBeats - 1
  }
  when(
    auxPendingReadBeats === 0 && !(dut.io.memFbAuxRead.read && dut.io.memFbAuxRead.waitRequestn)
  ) {
    assume(!dut.io.memFbAuxRead.readDataValid)
  }

  assert(!(dut.io.memFbAuxRead.read && dut.io.memFbAuxRead.write))
  when(dut.io.memFbAuxRead.read) {
    assert(dut.io.memFbAuxRead.waitRequestn)
  }

  val sawAuxBurstRead = RegInit(False)
  val sawAuxBurstRead4 = RegInit(False)
  val sawAuxRsp = RegInit(False)
  val sawAuxRspStall = RegInit(False)
  when(
    dut.io.memFbAuxRead.read && dut.io.memFbAuxRead.waitRequestn && dut.io.memFbAuxRead.burstCount > 1
  ) {
    sawAuxBurstRead := True
    when(dut.io.memFbAuxRead.burstCount === 4) {
      sawAuxBurstRead4 := True
    }
  }
  when(dut.io.memFbAuxRead.readDataValid && auxPendingReadBeats === 1) {
    sawAuxRsp := True
  }
  when(dut.io.fbAuxReadMem.rsp.valid && !dut.io.fbAuxReadMem.rsp.ready) {
    sawAuxRspStall := True
  }

  cover(sawAuxBurstRead)
  cover(sawAuxBurstRead4 && sawAuxRsp)
  cover(
    sawAuxBurstRead4 && sawAuxRspStall && dut.io.fbAuxReadMem.rsp.fire && dut.io.fbAuxReadMem.rsp.last
  )
}

class De10MemBackendAuxReadFormalBmcDut extends De10MemBackendAuxReadFormalDut(formalStrong = true)
class De10MemBackendAuxReadFormalProveDut
    extends De10MemBackendAuxReadFormalDut(formalStrong = false)
class De10MemBackendAuxReadFormalCoverDut
    extends De10MemBackendAuxReadFormalDut(formalStrong = true)

class De10MemBackendFormalTest extends SpinalFormalFunSuite {
  test("DE10 H2F MMIO bridge invariants bmc") {
    FormalConfig
      .withBMC(20)
      .withAsync
      .doVerify(new H2fLwToBmbBridgeFormalBmcDut)
  }

  test("DE10 H2F MMIO bridge invariants k-induction") {
    FormalConfig
      .withProve(20)
      .withAsync
      .doVerify(new H2fLwToBmbBridgeFormalProveDut)
  }

  test("DE10 H2F MMIO bridge cover") {
    FormalConfig
      .withCover(32)
      .withAsync
      .doVerify(new H2fLwToBmbBridgeFormalCoverDut)
  }

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

  test("DE10 memory backend invariants bmc") {
    FormalConfig
      .withBMC(20)
      .withAsync
      .doVerify(new De10MemBackendFormalBmcDut)
  }

  test("DE10 memory backend invariants k-induction") {
    FormalConfig
      .withProve(20)
      .withAsync
      .doVerify(new De10MemBackendFormalProveDut)
  }

  test("DE10 memory backend cover") {
    FormalConfig
      .withCover(24)
      .withAsync
      .doVerify(new De10MemBackendFormalCoverDut)
  }

  test("DE10 memory backend aux-read invariants bmc") {
    FormalConfig
      .withBMC(20)
      .withAsync
      .doVerify(new De10MemBackendAuxReadFormalBmcDut)
  }

  test("DE10 memory backend aux-read invariants k-induction") {
    FormalConfig
      .withProve(20)
      .withAsync
      .doVerify(new De10MemBackendAuxReadFormalProveDut)
  }

  test("DE10 memory backend aux-read cover") {
    FormalConfig
      .withCover(24)
      .withAsync
      .doVerify(new De10MemBackendAuxReadFormalCoverDut)
  }
}
