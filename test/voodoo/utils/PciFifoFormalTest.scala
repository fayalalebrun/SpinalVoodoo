//> using target.scope test

package voodoo.utils

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.formal._

class PciFifoFormalDut(formalStrong: Boolean) extends Component {
  val busParams = BmbParameter(
    addressWidth = 10,
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )

  val dut = PciFifo(
    busParams = busParams,
    categories = Map[BigInt, RegisterCategory](
      BigInt(0x010) -> RegisterCategory.fifoNoSync,
      BigInt(0x014) -> RegisterCategory.fifoNoSync,
      BigInt(0x020) -> RegisterCategory.bypassFifo,
      BigInt(0x030) -> RegisterCategory.fifoWithSync,
      BigInt(0x090) -> RegisterCategory.fifoNoSync
    ),
    floatAliases = Map(BigInt(0x090) -> PciFifo.FloatAliasConfig(BigInt(0x014), 12, 4)),
    commandAddresses = Seq(BigInt(0x010)),
    formalStrong = formalStrong
  )

  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  val cpuAddress = anyseq(UInt(busParams.access.addressWidth bits))
  val cpuData = anyseq(Bits(busParams.access.dataWidth bits))
  val cpuMask = anyseq(Bits(busParams.access.dataWidth / 8 bits))
  val cpuSource = anyseq(UInt(busParams.access.sourceWidth bits))
  val cpuOpcode = anyseq(cloneOf(dut.io.cpuSide.cmd.opcode))
  val texPayload = anyseq(cloneOf(dut.io.texWrite.payload))
  val regRspData = anyseq(Bits(busParams.access.dataWidth bits))
  val regRspSource = anyseq(UInt(busParams.access.sourceWidth bits))

  dut.io.cpuSide.cmd.valid := anyseq(Bool())
  dut.io.cpuSide.cmd.address := cpuAddress
  dut.io.cpuSide.cmd.data := cpuData
  dut.io.cpuSide.cmd.mask := cpuMask
  dut.io.cpuSide.cmd.source := cpuSource
  dut.io.cpuSide.cmd.opcode := cpuOpcode
  dut.io.cpuSide.cmd.length := 3
  dut.io.cpuSide.cmd.last := True
  dut.io.cpuSide.rsp.ready := True

  dut.io.regSide.cmd.ready := True
  dut.io.regSide.rsp.valid := anyseq(Bool())
  dut.io.regSide.rsp.data := regRspData
  dut.io.regSide.rsp.last := True
  dut.io.regSide.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  dut.io.regSide.rsp.source := regRspSource

  dut.io.texWrite.valid := anyseq(Bool())
  dut.io.texWrite.payload := texPayload
  dut.io.texDrain.ready := True

  dut.io.pipelineBusy := anyseq(Bool())
  dut.io.commandReady(0) := anyseq(Bool())
  dut.io.wasEnqueuedAddr := anyseq(UInt(busParams.access.addressWidth bits))

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.cpuSide.cmd.valid)
    assume(!dut.io.texWrite.valid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.cpuSide.cmd.formalAssumesSlave()
  dut.io.regSide.rsp.formalAssumesSlave()
  dut.io.texWrite.formalAssumesSlave()
  if (formalStrong) {
    dut.io.regSide.cmd.formalAssertsMaster()
    dut.io.cpuSide.rsp.formalAssertsMaster()
    dut.io.texDrain.formalAssertsMaster()
  }

  when(pastValid && past(dut.io.cpuSide.cmd.valid && !dut.io.cpuSide.cmd.ready)) {
    assume(dut.io.cpuSide.cmd.address === past(dut.io.cpuSide.cmd.address))
    assume(dut.io.cpuSide.cmd.data === past(dut.io.cpuSide.cmd.data))
    assume(dut.io.cpuSide.cmd.mask === past(dut.io.cpuSide.cmd.mask))
    assume(dut.io.cpuSide.cmd.source === past(dut.io.cpuSide.cmd.source))
    assume(dut.io.cpuSide.cmd.opcode === past(dut.io.cpuSide.cmd.opcode))
  }

  when(dut.io.cpuSide.cmd.valid) {
    assume(dut.io.cpuSide.cmd.address(1 downto 0) === U(0, 2 bits))
    assume(
      dut.io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.WRITE || dut.io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.READ
    )
  }

  val readOutstanding = Reg(Bool()) init (False)
  when(dut.io.regSide.cmd.fire && dut.io.regSide.cmd.opcode === Bmb.Cmd.Opcode.READ) {
    readOutstanding := True
  }
  when(!readOutstanding) {
    assume(!dut.io.regSide.rsp.valid)
  }
  when(dut.io.regSide.rsp.valid) {
    assume(readOutstanding)
    readOutstanding := False
  }

  when(dut.io.regSide.cmd.fire && dut.io.regSide.cmd.opcode === Bmb.Cmd.Opcode.READ) {
    assert(dut.io.regSide.cmd.address === dut.io.cpuSide.cmd.address)
    assert(dut.io.regSide.cmd.source === dut.io.cpuSide.cmd.source)
  }

  if (formalStrong) {
    when(dut.io.floatShadow.valid) {
      assert(dut.io.floatShadow.address === U(0x014, busParams.access.addressWidth bits))
    }
  }

  when(
    dut.io.cpuSide.cmd.fire && dut.io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.WRITE &&
      dut.io.cpuSide.cmd.address === U(0x020, busParams.access.addressWidth bits)
  ) {
    assert(dut.io.regSide.cmd.fire)
    assert(dut.io.regSide.cmd.address === dut.io.cpuSide.cmd.address)
    assert(dut.io.regSide.cmd.data === dut.io.cpuSide.cmd.data)
    assert(dut.io.regSide.cmd.mask === dut.io.cpuSide.cmd.mask)
    assert(dut.io.regSide.cmd.opcode === Bmb.Cmd.Opcode.WRITE)
  }
}

class PciFifoFormalBmcDut extends PciFifoFormalDut(formalStrong = true)

class PciFifoFormalProveDut extends PciFifoFormalDut(formalStrong = false)

class PciFifoFormalTest extends SpinalFormalFunSuite {
  test("PciFifo invariants bmc") {
    FormalConfig
      .withBMC(10)
      .withAsync
      .doVerify(new PciFifoFormalBmcDut)
  }

  test("PciFifo invariants prove") {
    FormalConfig
      .withProve(16)
      .withAsync
      .doVerify(new PciFifoFormalProveDut)
  }
}
