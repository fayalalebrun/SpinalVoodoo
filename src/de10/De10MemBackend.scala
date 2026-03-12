package voodoo.de10

import spinal.core._
import spinal.lib._
import spinal.lib.bus.avalon._
import spinal.lib.bus.bmb._
import voodoo.{Config, Core}

object De10MemBackend {
  val physicalAddressWidth = 32

  def avalonConfig(addressWidth: Int): AvalonMMConfig = AvalonMMConfig.pipelined(
    addressWidth = addressWidth,
    dataWidth = 32,
    useByteEnable = true
  )
}

case class De10BmbToAvalonMm(
    bmbParams: BmbParameter,
    avalonAddressWidth: Int,
    addressBase: BigInt,
    addressMask: BigInt
) extends Component {
  val io = new Bundle {
    val bmb = slave(Bmb(bmbParams))
    val mem = master(AvalonMM(De10MemBackend.avalonConfig(avalonAddressWidth)))
  }

  val readOutstanding = RegInit(False)
  val rspPending = RegInit(False)
  val rspData = Reg(Bits(bmbParams.access.dataWidth bits)) init (0)
  val rspSource = Reg(UInt(bmbParams.access.sourceWidth bits)) init (0)

  private val bmbAddressWidth = bmbParams.access.addressWidth
  private val maskedAddress = io.bmb.cmd.address & U(addressMask, bmbAddressWidth bits)
  private val translatedAddress =
    (maskedAddress.resize(avalonAddressWidth) +^ U(addressBase, avalonAddressWidth bits))
      .resize(avalonAddressWidth)

  io.mem.address := translatedAddress
  io.mem.read := False
  io.mem.write := False
  io.mem.byteEnable := io.bmb.cmd.mask
  io.mem.writeData := io.bmb.cmd.data

  val cmdIsWrite = io.bmb.cmd.opcode === Bmb.Cmd.Opcode.WRITE
  val cmdCanIssue = !readOutstanding && !rspPending && io.bmb.cmd.valid && io.mem.waitRequestn
  val writeCmdFire = cmdCanIssue && cmdIsWrite
  val readCmdFire = cmdCanIssue && !cmdIsWrite
  val writeRspNow = writeCmdFire

  io.bmb.cmd.ready := False
  io.bmb.rsp.valid := rspPending || writeRspNow
  io.bmb.rsp.last := True
  io.bmb.rsp.source := writeRspNow ? io.bmb.cmd.source | rspSource
  io.bmb.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  io.bmb.rsp.data := writeRspNow ? B(0, bmbParams.access.dataWidth bits) | rspData
  if (bmbParams.access.contextWidth > 0) {
    io.bmb.rsp.context := 0
  }

  when(writeCmdFire) {
    io.mem.write := True
    io.bmb.cmd.ready := True
    when(!io.bmb.rsp.ready) {
      rspPending := True
      rspSource := io.bmb.cmd.source
      rspData := 0
    }
  }

  when(readCmdFire) {
    io.mem.read := True
    io.mem.byteEnable := B"4'xF"
    io.bmb.cmd.ready := True
    readOutstanding := True
    rspSource := io.bmb.cmd.source
  }

  when(readOutstanding && io.mem.readDataValid) {
    readOutstanding := False
    rspPending := True
    rspData := io.mem.readData
  }

  when(rspPending && io.bmb.rsp.fire) {
    rspPending := False
  }
}

case class De10MemBackend(c: Config) extends Component {
  val io = new Bundle {
    val fbMem = slave(Bmb(Core.fbMemBmbParams(c)))
    val texMem = slave(Bmb(Core.texMemBmbParams(c)))
    val memFb = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
    val memTex = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
  }

  val fbBridge = De10BmbToAvalonMm(
    bmbParams = Core.fbMemBmbParams(c),
    avalonAddressWidth = De10MemBackend.physicalAddressWidth,
    addressBase = De10AddressMap.fbMemBase,
    addressMask = De10AddressMap.fbMemMask
  )
  val texBridge = De10BmbToAvalonMm(
    bmbParams = Core.texMemBmbParams(c),
    avalonAddressWidth = De10MemBackend.physicalAddressWidth,
    addressBase = De10AddressMap.texMemBase,
    addressMask = De10AddressMap.texMemMask
  )

  io.fbMem <> fbBridge.io.bmb
  io.texMem <> texBridge.io.bmb

  io.memFb <> fbBridge.io.mem
  io.memTex <> texBridge.io.mem
}
