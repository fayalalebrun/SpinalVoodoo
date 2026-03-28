package voodoo.de10

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.avalon._
import spinal.lib.bus.bmb._
import voodoo.{Config, Core}

object De10MemBackend {
  val physicalAddressWidth = 32
  val maxAvalonBurstWords = 1024
  val avalonBurstCountWidth = log2Up(maxAvalonBurstWords + 1)

  def avalonConfig(addressWidth: Int): AvalonMMConfig = AvalonMMConfig
    .pipelined(
      addressWidth = addressWidth,
      dataWidth = 32,
      useByteEnable = true
    )
    .copy(
      burstCountWidth = avalonBurstCountWidth,
      useBurstCount = true
    )
}

case class De10BmbReadRspPayload(dataWidth: Int, sourceWidth: Int) extends Bundle {
  val data = Bits(dataWidth bits)
  val source = UInt(sourceWidth bits)
  val last = Bool()
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

  val readBurstActive = RegInit(False)
  val writeBurstActive = RegInit(False)
  val writeRspPending = RegInit(False)
  val writeRspSource = Reg(UInt(bmbParams.access.sourceWidth bits)) init (0)
  val readSource = Reg(UInt(bmbParams.access.sourceWidth bits)) init (0)

  private val readCountWidth = log2Up(bmbParams.access.transferBeatCount + 1)
  val readRspBeatsLeft = Reg(UInt(readCountWidth bits)) init (0)
  val maxReadRspFifoDepth = scala.math.max(2, bmbParams.access.transferBeatCount)
  val readRspFifo = StreamFifo(
    De10BmbReadRspPayload(bmbParams.access.dataWidth, bmbParams.access.sourceWidth),
    maxReadRspFifoDepth
  )

  private val bmbAddressWidth = bmbParams.access.addressWidth
  private def translateAddress(address: UInt): UInt = {
    val truncatedMask = addressMask & ((BigInt(1) << bmbAddressWidth) - 1)
    val maskedAddress = address & U(truncatedMask, bmbAddressWidth bits)
    (maskedAddress.resize(avalonAddressWidth) +^ U(addressBase, avalonAddressWidth bits))
      .resize(avalonAddressWidth)
  }

  private val translatedAddress = translateAddress(io.bmb.cmd.address)
  private val beatBytes = U(bmbParams.access.byteCount, bmbAddressWidth bits)
  private val fullMask =
    B((BigInt(1) << bmbParams.access.maskWidth) - 1, bmbParams.access.maskWidth bits)
  private val cmdBeatCount =
    io.bmb.cmd.fragment.transferBeatCountMinusOne.resize(readCountWidth) + 1
  private val cmdBurstCount = cmdBeatCount.resize(De10MemBackend.avalonBurstCountWidth)

  val cmdIsWrite = io.bmb.cmd.opcode === Bmb.Cmd.Opcode.WRITE
  val acceptWriteBeat =
    io.bmb.cmd.valid && cmdIsWrite && !readBurstActive && !writeRspPending && !readRspFifo.io.pop.valid && io.mem.waitRequestn
  val acceptReadCmd =
    io.bmb.cmd.valid && !cmdIsWrite && !writeBurstActive && !readBurstActive && !writeRspPending && !readRspFifo.io.pop.valid && io.mem.waitRequestn
  val writeRspNow = acceptWriteBeat && io.bmb.cmd.last
  val readRspNow = readRspFifo.io.pop.valid && !writeRspPending && !writeRspNow
  val useWriteRsp = writeRspNow || writeRspPending

  io.mem.address := translatedAddress
  io.mem.read := False
  io.mem.write := False
  io.mem.burstCount := 1
  io.mem.byteEnable := 0
  io.mem.writeData := io.bmb.cmd.data

  io.bmb.cmd.ready := False
  io.bmb.rsp.valid := useWriteRsp || readRspNow
  io.bmb.rsp.last := Mux(useWriteRsp, True, readRspFifo.io.pop.payload.last)
  io.bmb.rsp.source := Mux(
    writeRspNow,
    io.bmb.cmd.source,
    Mux(writeRspPending, writeRspSource, readRspFifo.io.pop.payload.source)
  )
  io.bmb.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  io.bmb.rsp.data := Mux(
    useWriteRsp,
    B(0, bmbParams.access.dataWidth bits),
    readRspFifo.io.pop.payload.data
  )
  if (bmbParams.access.contextWidth > 0) {
    io.bmb.rsp.context := 0
  }
  readRspFifo.io.pop.ready := !writeRspPending && !writeRspNow && io.bmb.rsp.ready

  readRspFifo.io.push.valid := io.mem.readDataValid
  readRspFifo.io.push.payload.data := io.mem.readData
  readRspFifo.io.push.payload.source := readSource
  readRspFifo.io.push.payload.last := readRspBeatsLeft === 1

  when(acceptWriteBeat) {
    io.mem.write := True
    io.mem.burstCount := 1
    io.mem.byteEnable := io.bmb.cmd.mask
    io.bmb.cmd.ready := True
    writeBurstActive := !io.bmb.cmd.last
    when(io.bmb.cmd.last && !io.bmb.rsp.ready) {
      writeRspPending := True
      writeRspSource := io.bmb.cmd.source
    }
  }

  when(acceptReadCmd) {
    io.mem.read := True
    io.mem.burstCount := cmdBurstCount
    io.mem.byteEnable := fullMask
    io.bmb.cmd.ready := True
    readBurstActive := True
    readSource := io.bmb.cmd.source
    readRspBeatsLeft := cmdBeatCount
  }

  when(io.mem.readDataValid) {
    readRspBeatsLeft := readRspBeatsLeft - 1
    when(readRspBeatsLeft === 1) {
      readBurstActive := False
    }
  }

  when(writeRspPending && io.bmb.rsp.fire) {
    writeRspPending := False
  }

  GenerationFlags.formal {
    assert(cmdBeatCount =/= 0)
    when(acceptReadCmd) {
      assert(io.mem.burstCount === cmdBurstCount)
    }
    when(io.mem.write) {
      assert(io.mem.burstCount === 1)
    }
    when(io.bmb.rsp.valid && !writeRspNow && !writeRspPending) {
      assert(readRspFifo.io.pop.valid)
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

    when(readBurstActive || writeRspPending || readRspFifo.io.pop.valid) {
      assert(!io.bmb.cmd.ready)
    }

    when(writeRspNow) {
      assert(io.bmb.rsp.valid)
      assert(io.bmb.rsp.last)
      assert(io.bmb.rsp.source === io.bmb.cmd.source)
      assert(io.bmb.rsp.data === 0)
    }

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

  def fbBridge() =
    De10BmbToAvalonMm(
      bmbParams = Core.fbMemBmbParams(c),
      avalonAddressWidth = De10MemBackend.physicalAddressWidth,
      addressBase = De10AddressMap.fbMemBase,
      addressMask = De10AddressMap.fbMemMask
    )

  val fbWriteBridge = fbBridge()
  val fbColorReadBridge = fbBridge()
  val fbAuxReadBridge = fbBridge()
  val texBridge = De10BmbToAvalonMm(
    bmbParams = Core.texMemBmbParams(c),
    avalonAddressWidth = De10MemBackend.physicalAddressWidth,
    addressBase = De10AddressMap.texMemBase,
    addressMask = De10AddressMap.texMemMask
  )

  Seq(
    io.fbMemWrite <> fbWriteBridge.io.bmb,
    io.fbColorReadMem <> fbColorReadBridge.io.bmb,
    io.fbAuxReadMem <> fbAuxReadBridge.io.bmb,
    io.texMem <> texBridge.io.bmb,
    io.memFbWrite <> fbWriteBridge.io.mem,
    io.memFbColorRead <> fbColorReadBridge.io.mem,
    io.memFbAuxRead <> fbAuxReadBridge.io.mem,
    io.memTex <> texBridge.io.mem
  )

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

  }
}
