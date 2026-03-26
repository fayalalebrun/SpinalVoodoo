package voodoo.de10

import spinal.core._
import spinal.core.formal._
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
  val readBurstActive = RegInit(False)
  val writeBurstActive = RegInit(False)
  val rspPending = RegInit(False)
  val rspData = Reg(Bits(bmbParams.access.dataWidth bits)) init (0)
  val rspSource = Reg(UInt(bmbParams.access.sourceWidth bits)) init (0)
  val rspLast = RegInit(True)
  val readSource = Reg(UInt(bmbParams.access.sourceWidth bits)) init (0)

  private val readCountWidth = log2Up(bmbParams.access.transferBeatCount + 1)
  val readBeatsLeft = Reg(UInt(readCountWidth bits)) init (0)
  val readAddress = Reg(UInt(bmbParams.access.addressWidth bits)) init (0)

  private val bmbAddressWidth = bmbParams.access.addressWidth
  private def translateAddress(address: UInt): UInt = {
    val truncatedMask = addressMask & ((BigInt(1) << bmbAddressWidth) - 1)
    val maskedAddress = address & U(truncatedMask, bmbAddressWidth bits)
    (maskedAddress.resize(avalonAddressWidth) +^ U(addressBase, avalonAddressWidth bits))
      .resize(avalonAddressWidth)
  }

  private val translatedAddress = translateAddress(io.bmb.cmd.address)
  private val translatedReadAddress = translateAddress(readAddress)
  private val beatBytes = U(bmbParams.access.byteCount, bmbAddressWidth bits)
  private val fullMask =
    B((BigInt(1) << bmbParams.access.maskWidth) - 1, bmbParams.access.maskWidth bits)
  private val cmdBeatCount =
    io.bmb.cmd.fragment.transferBeatCountMinusOne.resize(readCountWidth) + 1

  val cmdIsWrite = io.bmb.cmd.opcode === Bmb.Cmd.Opcode.WRITE
  val acceptWriteBeat =
    io.bmb.cmd.valid && cmdIsWrite && !readBurstActive && !rspPending && io.mem.waitRequestn
  val acceptReadCmd =
    io.bmb.cmd.valid && !cmdIsWrite && !writeBurstActive && !readBurstActive && !rspPending && io.mem.waitRequestn
  val issueReadBeat = readBurstActive && !readOutstanding && !rspPending && io.mem.waitRequestn
  val writeRspNow = acceptWriteBeat && io.bmb.cmd.last

  io.mem.address := translatedAddress
  io.mem.read := False
  io.mem.write := False
  io.mem.byteEnable := 0
  io.mem.writeData := io.bmb.cmd.data

  io.bmb.cmd.ready := False
  io.bmb.rsp.valid := rspPending || writeRspNow
  io.bmb.rsp.last := writeRspNow ? True | rspLast
  io.bmb.rsp.source := writeRspNow ? io.bmb.cmd.source | rspSource
  io.bmb.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  io.bmb.rsp.data := writeRspNow ? B(0, bmbParams.access.dataWidth bits) | rspData
  if (bmbParams.access.contextWidth > 0) {
    io.bmb.rsp.context := 0
  }

  when(acceptWriteBeat) {
    io.mem.write := True
    io.mem.byteEnable := io.bmb.cmd.mask
    io.bmb.cmd.ready := True
    writeBurstActive := !io.bmb.cmd.last
    when(io.bmb.cmd.last && !io.bmb.rsp.ready) {
      rspPending := True
      rspSource := io.bmb.cmd.source
      rspData := 0
      rspLast := True
    }
  }

  when(acceptReadCmd) {
    io.mem.read := True
    io.mem.byteEnable := fullMask
    io.bmb.cmd.ready := True
    readBurstActive := True
    readOutstanding := True
    readSource := io.bmb.cmd.source
    readBeatsLeft := cmdBeatCount
    readAddress := io.bmb.cmd.address + beatBytes
  }

  when(readOutstanding && io.mem.readDataValid) {
    readOutstanding := False
    rspPending := True
    rspData := io.mem.readData
    rspSource := readSource
    rspLast := readBeatsLeft === 1
  }

  when(issueReadBeat) {
    io.mem.address := translatedReadAddress
    io.mem.read := True
    io.mem.byteEnable := fullMask
    readOutstanding := True
    readAddress := readAddress + beatBytes
  }

  when(rspPending && io.bmb.rsp.fire) {
    rspPending := False
    when(readBurstActive) {
      readBeatsLeft := readBeatsLeft - 1
      when(readBeatsLeft === 1) {
        readBurstActive := False
      }
    }
  }

  GenerationFlags.formal {
    val pastValid = RegNext(True) init False

    assert(!(io.mem.read && io.mem.write))

    when(io.mem.read) {
      assert(io.mem.byteEnable === fullMask)
    }

    when(acceptWriteBeat) {
      assert(io.bmb.cmd.ready)
      assert(io.mem.write)
      assert(!io.mem.read)
      assert(io.mem.address === translatedAddress)
      assert(io.mem.byteEnable === io.bmb.cmd.mask)
      assert(io.mem.writeData === io.bmb.cmd.data)
    }

    when(acceptReadCmd) {
      assert(io.bmb.cmd.ready)
      assert(io.mem.read)
      assert(!io.mem.write)
      assert(io.mem.address === translatedAddress)
      assert(io.mem.byteEnable === fullMask)
    }

    when(issueReadBeat) {
      assert(io.mem.read)
      assert(!io.mem.write)
      assert(io.mem.address === translatedReadAddress)
      assert(io.mem.byteEnable === fullMask)
    }

    when(readBurstActive || rspPending) {
      assert(!io.bmb.cmd.ready)
    }

    when(writeRspNow) {
      assert(io.bmb.rsp.valid)
      assert(io.bmb.rsp.last)
      assert(io.bmb.rsp.source === io.bmb.cmd.source)
      assert(io.bmb.rsp.data === 0)
    }

    cover(acceptWriteBeat)
    cover(acceptWriteBeat && !io.bmb.cmd.last)
    cover(acceptReadCmd)
    cover(pastValid && past(readOutstanding) && io.mem.readDataValid)
    cover(pastValid && past(io.bmb.rsp.valid && !io.bmb.rsp.ready) && io.bmb.rsp.fire)
  }
}

case class De10MemBackend(c: Config) extends Component {
  val io = new Bundle {
    val fbMemWrite = slave(Bmb(Core.fbMemBmbParams(c)))
    val fbColorReadMem = slave(Bmb(Core.fbMemBmbParams(c)))
    val fbAuxReadMem = slave(Bmb(Core.fbMemBmbParams(c)))
    val texMem = slave(Bmb(Core.texMemBmbParams(c)))
    val memFbWrite = master(
      AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth))
    )
    val memFbColorRead = master(
      AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth))
    )
    val memFbAuxRead = master(
      AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth))
    )
    val memTex = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
  }

  val fbWriteBridge = De10BmbToAvalonMm(
    bmbParams = Core.fbMemBmbParams(c),
    avalonAddressWidth = De10MemBackend.physicalAddressWidth,
    addressBase = De10AddressMap.fbMemBase,
    addressMask = De10AddressMap.fbMemMask
  )
  val fbColorReadBridge = De10BmbToAvalonMm(
    bmbParams = Core.fbMemBmbParams(c),
    avalonAddressWidth = De10MemBackend.physicalAddressWidth,
    addressBase = De10AddressMap.fbMemBase,
    addressMask = De10AddressMap.fbMemMask
  )
  val fbAuxReadBridge = De10BmbToAvalonMm(
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

  io.fbMemWrite <> fbWriteBridge.io.bmb
  io.fbColorReadMem <> fbColorReadBridge.io.bmb
  io.fbAuxReadMem <> fbAuxReadBridge.io.bmb
  io.texMem <> texBridge.io.bmb

  io.memFbWrite <> fbWriteBridge.io.mem
  io.memFbColorRead <> fbColorReadBridge.io.mem
  io.memFbAuxRead <> fbAuxReadBridge.io.mem
  io.memTex <> texBridge.io.mem

  assert(De10AddressMap.fbMemBase + De10AddressMap.fbMemBytes <= De10AddressMap.texMemBase)
  assert(De10AddressMap.texMemBase + De10AddressMap.texMemBytes <= De10AddressMap.ddrCarveoutEnd)

  GenerationFlags.formal {
    when(io.memFbWrite.read || io.memFbWrite.write) {
      assert(io.memFbWrite.address >= U(De10AddressMap.fbMemBase, 32 bits))
      assert(
        io.memFbWrite.address < U(De10AddressMap.fbMemBase + De10AddressMap.fbMemBytes, 32 bits)
      )
    }
    when(io.memFbColorRead.read || io.memFbColorRead.write) {
      assert(io.memFbColorRead.address >= U(De10AddressMap.fbMemBase, 32 bits))
      assert(
        io.memFbColorRead.address < U(De10AddressMap.fbMemBase + De10AddressMap.fbMemBytes, 32 bits)
      )
    }
    when(io.memFbAuxRead.read || io.memFbAuxRead.write) {
      assert(io.memFbAuxRead.address >= U(De10AddressMap.fbMemBase, 32 bits))
      assert(
        io.memFbAuxRead.address < U(De10AddressMap.fbMemBase + De10AddressMap.fbMemBytes, 32 bits)
      )
    }
    when(io.memTex.read || io.memTex.write) {
      assert(io.memTex.address >= U(De10AddressMap.texMemBase, 32 bits))
      assert(io.memTex.address < U(De10AddressMap.texMemBase + De10AddressMap.texMemBytes, 32 bits))
    }

    assert(!(io.memFbWrite.read && io.memFbWrite.write))
    assert(!(io.memFbColorRead.read && io.memFbColorRead.write))
    assert(!(io.memFbAuxRead.read && io.memFbAuxRead.write))
    assert(!(io.memTex.read && io.memTex.write))

    cover(io.memFbWrite.write)
    cover(io.memFbColorRead.read)
    cover(io.memFbAuxRead.read)
    cover(io.memTex.read)
    cover(io.memFbWrite.write && io.memFbColorRead.read && io.memFbAuxRead.read && io.memTex.read)
    cover(io.fbMemWrite.rsp.valid && io.texMem.rsp.valid)
  }
}
