package voodoo.core

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo._
import voodoo.bus.TexWritePayload

case class CoreCpuFrontdoor(c: Config) extends Component {
  val io = new Bundle {
    val cpuBus = slave(Bmb(voodoo.Core.cpuBmbParams))
    val regBus = master(Bmb(RegisterBank.externalBmbParams(c)))
    val lfbBus = master(Bmb(Lfb.bmbParams(c)))
    val texReadBus = master(Bmb(voodoo.Core.cpuTexBmbParams))
    val texWrite = master(Stream(TexWritePayload()))
    val texBaseAddr = in UInt (24 bits)
    val invalidate = out Bool ()
  }

  private val cpuCmd = io.cpuBus.cmd
  private val cpuRsp = io.cpuBus.rsp
  private val target = cpuCmd.address(23 downto 22)
  private val rspTarget = RegNextWhen(target, cpuCmd.fire) init (0)
  private val isReg = target === 0
  private val isLfb = target === 1
  private val isTex = target >= 2
  private val isTexWrite = isTex && cpuCmd.opcode(0)
  private val isTexRead = isTex && !cpuCmd.opcode(0)

  private def routeCmd(bus: Bmb, select: Bool, address: UInt): Unit =
    BmbRouting.driveCmdFrom(bus, io.cpuBus, cpuCmd.valid && select, address)

  private def routeRsp(selected: Bool, bus: Bmb): Unit =
    BmbRouting.routeRspTo(selected, bus, io.cpuBus)

  private val regAddress =
    (cpuCmd.address(21 downto 14) ## U"4'0" ## cpuCmd.address(9 downto 0)).asUInt
  routeCmd(io.regBus, isReg, regAddress)
  routeCmd(io.lfbBus, isLfb, cpuCmd.address.resize(22 bits))

  io.texWrite.valid := cpuCmd.valid && isTexWrite
  io.texWrite.pciAddr := cpuCmd.address(22 downto 0)
  io.texWrite.data := cpuCmd.data
  io.texWrite.mask := cpuCmd.mask

  private val texReadPciOffset = cpuCmd.address(22 downto 0)
  private val texReadBaseAddr = io.texBaseAddr(18 downto 0)
  private val texReadSramAddr = ((texReadBaseAddr << 3) +^ texReadPciOffset).resize(26 bits)
  routeCmd(io.texReadBus, isTexRead, texReadSramAddr)

  private val texWriteRspPending = RegInit(False)
  private val texWriteRspSource = Reg(cpuCmd.source)
  private val texWriteAccepted = cpuCmd.fire && isTexWrite

  when(texWriteAccepted) {
    texWriteRspPending := True
    texWriteRspSource := cpuCmd.source
  }
  when(texWriteRspPending) {
    texWriteRspPending := False
  }

  io.invalidate := texWriteAccepted

  cpuCmd.ready :=
    (isReg && io.regBus.cmd.ready) ||
      (isLfb && io.lfbBus.cmd.ready) ||
      (isTexWrite && io.texWrite.ready) ||
      (isTexRead && io.texReadBus.cmd.ready)

  cpuRsp.valid := False
  cpuRsp.data := 0
  cpuRsp.last := True
  cpuRsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  cpuRsp.source := 0

  private val regRspSelected = rspTarget === 0
  private val lfbRspSelected = rspTarget === 1
  private val texRspSelected = rspTarget >= 2

  io.regBus.rsp.ready := False
  io.lfbBus.rsp.ready := False
  io.texReadBus.rsp.ready := False

  when(texWriteRspPending) {
    cpuRsp.valid := True
    cpuRsp.source := texWriteRspSource.resize(cpuRsp.p.access.sourceWidth bits)
  } otherwise {
    routeRsp(regRspSelected, io.regBus)
    routeRsp(lfbRspSelected, io.lfbBus)
    routeRsp(texRspSelected, io.texReadBus)
  }
}
