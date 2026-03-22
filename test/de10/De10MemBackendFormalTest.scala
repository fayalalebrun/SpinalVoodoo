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

  when(pastValid && past(dut.io.cpuBus.rsp.valid && !dut.io.cpuBus.rsp.ready)) {
    assume(dut.io.cpuBus.rsp.valid)
    assume(dut.io.cpuBus.rsp.last === past(dut.io.cpuBus.rsp.last))
    assume(dut.io.cpuBus.rsp.source === past(dut.io.cpuBus.rsp.source))
    assume(dut.io.cpuBus.rsp.opcode === past(dut.io.cpuBus.rsp.opcode))
    assume(dut.io.cpuBus.rsp.data === past(dut.io.cpuBus.rsp.data))
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
class H2fLwToBmbBridgeFormalProveDut extends H2fLwToBmbBridgeFormalDut(formalStrong = false)
class H2fLwToBmbBridgeFormalCoverDut extends H2fLwToBmbBridgeFormalDut(formalStrong = true)

class De10BmbToAvalonMmFormalDut(formalStrong: Boolean) extends Component {
  val bmbParams = BmbParameter(
    addressWidth = 8,
    dataWidth = 32,
    sourceWidth = 2,
    contextWidth = 0,
    lengthWidth = 5,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  val dut = De10BmbToAvalonMm(
    bmbParams = bmbParams,
    avalonAddressWidth = 12,
    addressBase = 0x100,
    addressMask = 0x3f
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

  val avalonReadPending = RegInit(False)
  when(dut.io.mem.read && dut.io.mem.waitRequestn) {
    assume(!avalonReadPending)
    avalonReadPending := True
  }
  when(dut.io.mem.readDataValid) {
    assume(avalonReadPending)
    avalonReadPending := False
  }

  val maxMaskedAddr = U(0x3f, 8 bits)
  val aliasAddr = U(0xff, 8 bits)
  val partialMask = B"4'b0101"

  cover(
    dut.io.mem.write && dut.io.mem.waitRequestn && dut.io.bmb.cmd.mask === partialMask
  )
  cover(
    dut.io.mem.read && dut.io.mem.waitRequestn && dut.io.bmb.cmd.address === maxMaskedAddr
  )
  cover(
    dut.io.mem.write && dut.io.mem.waitRequestn && dut.io.bmb.cmd.address === aliasAddr
  )

  if (formalStrong) {
    when(pastValid && past(dut.io.bmb.rsp.valid && !dut.io.bmb.rsp.ready)) {
      assert(dut.io.bmb.rsp.valid)
      assert(dut.io.bmb.rsp.source === past(dut.io.bmb.rsp.source))
      assert(dut.io.bmb.rsp.data === past(dut.io.bmb.rsp.data))
      assert(dut.io.bmb.rsp.last === past(dut.io.bmb.rsp.last))
    }
  }

  val sawWrite = RegInit(False)
  val sawMultiWrite = RegInit(False)
  val sawRead = RegInit(False)
  val sawReadRsp = RegInit(False)
  val sawRspStall = RegInit(False)

  when(dut.io.mem.write && dut.io.mem.waitRequestn) {
    sawWrite := True
    when(dut.io.bmb.cmd.valid && !dut.io.bmb.cmd.last) {
      sawMultiWrite := True
    }
  }
  when(dut.io.mem.read && dut.io.mem.waitRequestn) {
    sawRead := True
  }
  when(dut.io.mem.readDataValid) {
    sawReadRsp := True
  }
  when(dut.io.bmb.rsp.valid && !dut.io.bmb.rsp.ready) {
    sawRspStall := True
  }

  cover(sawWrite)
  cover(sawMultiWrite)
  cover(sawRead)
  cover(sawReadRsp && sawRspStall)
}

class De10BmbToAvalonMmFormalBmcDut extends De10BmbToAvalonMmFormalDut(formalStrong = true)
class De10BmbToAvalonMmFormalProveDut extends De10BmbToAvalonMmFormalDut(formalStrong = false)
class De10BmbToAvalonMmFormalCoverDut extends De10BmbToAvalonMmFormalDut(formalStrong = true)

class De10MemBackendFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 8 bits,
      memBurstLengthWidth = 5,
      useFbFillCache = false,
      useTexFillCache = false
    )

  val dut = De10MemBackend(c)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init False

  val fbCmdPayload = anyseq(cloneOf(dut.io.fbMem.cmd.fragment))
  val texCmdPayload = anyseq(cloneOf(dut.io.texMem.cmd.fragment))

  dut.io.fbMem.cmd.valid := anyseq(Bool())
  dut.io.fbMem.cmd.fragment := fbCmdPayload
  dut.io.fbMem.cmd.last := anyseq(Bool())
  dut.io.fbMem.rsp.ready := anyseq(Bool())
  dut.io.texMem.cmd.valid := anyseq(Bool())
  dut.io.texMem.cmd.fragment := texCmdPayload
  dut.io.texMem.cmd.last := anyseq(Bool())
  dut.io.texMem.rsp.ready := anyseq(Bool())

  dut.io.memFb.waitRequestn := anyseq(Bool())
  dut.io.memFb.readDataValid := anyseq(Bool())
  dut.io.memFb.readData := anyseq(Bits(32 bits))
  dut.io.memTex.waitRequestn := anyseq(Bool())
  dut.io.memTex.readDataValid := anyseq(Bool())
  dut.io.memTex.readData := anyseq(Bits(32 bits))

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.fbMem.cmd.valid)
    assume(!dut.io.texMem.cmd.valid)
    assume(!dut.io.memFb.readDataValid)
    assume(!dut.io.memTex.readDataValid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.fbMem.cmd.formalAssumesSlave()
  dut.io.texMem.cmd.formalAssumesSlave()
  if (formalStrong) {
    dut.io.fbMem.rsp.formalAssertsMaster()
    dut.io.texMem.rsp.formalAssertsMaster()
  }

  when(
    dut.io.fbMem.cmd.valid && !dut.io.fbMem.cmd.ready && pastValid && past(
      dut.io.fbMem.cmd.valid && !dut.io.fbMem.cmd.ready
    )
  ) {
    assume(dut.io.fbMem.cmd.fragment.asBits === past(dut.io.fbMem.cmd.fragment.asBits))
    assume(dut.io.fbMem.cmd.last === past(dut.io.fbMem.cmd.last))
  }
  when(
    dut.io.texMem.cmd.valid && !dut.io.texMem.cmd.ready && pastValid && past(
      dut.io.texMem.cmd.valid && !dut.io.texMem.cmd.ready
    )
  ) {
    assume(dut.io.texMem.cmd.fragment.asBits === past(dut.io.texMem.cmd.fragment.asBits))
    assume(dut.io.texMem.cmd.last === past(dut.io.texMem.cmd.last))
  }

  val fbReadPending = RegInit(False)
  when(dut.io.memFb.read && dut.io.memFb.waitRequestn) {
    assume(!fbReadPending)
    fbReadPending := True
  }
  when(dut.io.memFb.readDataValid) {
    assume(fbReadPending)
    fbReadPending := False
  }

  val texReadPending = RegInit(False)
  when(dut.io.memTex.read && dut.io.memTex.waitRequestn) {
    assume(!texReadPending)
    texReadPending := True
  }
  when(dut.io.memTex.readDataValid) {
    assume(texReadPending)
    texReadPending := False
  }

  assert(!(dut.io.memFb.read && dut.io.memFb.write))
  assert(!(dut.io.memTex.read && dut.io.memTex.write))
  assert(De10AddressMap.fbMemBase + De10AddressMap.fbMemBytes <= De10AddressMap.texMemBase)
  assert(De10AddressMap.texMemBase + De10AddressMap.texMemBytes <= De10AddressMap.ddrCarveoutEnd)

  when(dut.io.memFb.read || dut.io.memFb.write) {
    assert(dut.io.memFb.address >= U(De10AddressMap.fbMemBase, 32 bits))
    assert(dut.io.memFb.address < U(De10AddressMap.fbMemBase + De10AddressMap.fbMemBytes, 32 bits))
  }
  when(dut.io.memTex.read || dut.io.memTex.write) {
    assert(dut.io.memTex.address >= U(De10AddressMap.texMemBase, 32 bits))
    assert(
      dut.io.memTex.address < U(De10AddressMap.texMemBase + De10AddressMap.texMemBytes, 32 bits)
    )
  }

  cover(dut.io.memFb.write)
  cover(dut.io.memTex.read)
  cover(dut.io.memFb.write && dut.io.memTex.read)
  cover(dut.io.fbMem.rsp.valid && dut.io.texMem.rsp.valid)
}

class De10MemBackendFormalBmcDut extends De10MemBackendFormalDut(formalStrong = true)
class De10MemBackendFormalProveDut extends De10MemBackendFormalDut(formalStrong = false)
class De10MemBackendFormalCoverDut extends De10MemBackendFormalDut(formalStrong = true)

class De10MemBackendFormalTest extends SpinalFormalFunSuite {
  test("DE10 H2F lightweight MMIO bridge invariants bmc") {
    FormalConfig
      .withBMC(20)
      .withAsync
      .doVerify(new H2fLwToBmbBridgeFormalBmcDut)
  }

  test("DE10 H2F lightweight MMIO bridge invariants prove") {
    FormalConfig
      .withProve(20)
      .withAsync
      .doVerify(new H2fLwToBmbBridgeFormalProveDut)
  }

  test("DE10 H2F lightweight MMIO bridge cover") {
    FormalConfig
      .withCover(32)
      .withAsync
      .doVerify(new H2fLwToBmbBridgeFormalCoverDut)
  }

  test("DE10 BMB-to-Avalon bridge invariants bmc") {
    FormalConfig
      .withBMC(20)
      .withAsync
      .doVerify(new De10BmbToAvalonMmFormalBmcDut)
  }

  test("DE10 BMB-to-Avalon bridge invariants prove") {
    FormalConfig
      .withProve(20)
      .withAsync
      .doVerify(new De10BmbToAvalonMmFormalProveDut)
  }

  test("DE10 BMB-to-Avalon bridge cover") {
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

  test("DE10 memory backend invariants prove") {
    FormalConfig
      .withProve(20)
      .withAsync
      .doVerify(new De10MemBackendFormalProveDut)
  }

  test("DE10 memory backend cover") {
    FormalConfig
      .withCover(32)
      .withAsync
      .doVerify(new De10MemBackendFormalCoverDut)
  }
}
