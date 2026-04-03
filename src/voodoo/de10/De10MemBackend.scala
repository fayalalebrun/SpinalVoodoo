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
  val maxOutstandingReadBursts = 2

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

object De10BmbToAvalonMm {
  def requireSingleSource(bmbParams: BmbParameter): BmbParameter = {
    require(
      bmbParams.access.sources.size == 1 && bmbParams.access.sources.contains(0),
      s"De10BmbToAvalonMm only supports a single BMB source id 0, got sources=${bmbParams.access.sources.keys.toSeq.sorted.mkString(",")}. Wrap multi-source buses with BmbSourceRemover before bridging to Avalon-MM."
    )
    bmbParams
  }

  def singleSourceBridgeParams(bmbParams: BmbParameter): BmbParameter =
    BmbSourceRemover.getOutputParameter(bmbParams)
}

case class De10BmbReadRspPayload(dataWidth: Int, contextWidth: Int) extends Bundle {
  val data = Bits(dataWidth bits)
  val last = Bool()
  val context = contextWidth > 0 generate Bits(contextWidth bits)
}

case class De10BmbReadCmdPayload(addressWidth: Int, contextWidth: Int, beatCountWidth: Int)
    extends Bundle {
  val address = UInt(addressWidth bits)
  val beatCount = UInt(beatCountWidth bits)
  val burstCount = UInt(De10MemBackend.avalonBurstCountWidth bits)
  val context = contextWidth > 0 generate Bits(contextWidth bits)
}

case class De10BmbToAvalonMm(
    bmbParams: BmbParameter,
    avalonAddressWidth: Int,
    addressBase: BigInt,
    addressMask: BigInt
) extends Component {
  De10BmbToAvalonMm.requireSingleSource(bmbParams)

  val io = new Bundle {
    val bmb = slave(Bmb(bmbParams))
    val mem = master(AvalonMM(De10MemBackend.avalonConfig(avalonAddressWidth)))
  }

  val writeBurstActive = RegInit(False)
  val writeRspPending = RegInit(False)
  val writeRspContext = bmbParams.access.contextWidth > 0 generate Reg(
    Bits(bmbParams.access.contextWidth bits)
  ) init (0)

  private val cmdBeatCountMinusOneWidth = io.bmb.cmd.fragment.transferBeatCountMinusOne.getWidth
  private val maxReadBurstBeats = 1 << cmdBeatCountMinusOneWidth
  private val readCountWidth = cmdBeatCountMinusOneWidth + 1
  private val readRspFifoDepth =
    scala.math.max(2, maxReadBurstBeats * De10MemBackend.maxOutstandingReadBursts)
  val readRspFifo = StreamFifo(
    De10BmbReadRspPayload(bmbParams.access.dataWidth, bmbParams.access.contextWidth),
    readRspFifoDepth
  )
  val readRspBurstActive = RegInit(False)
  val readRspBurstBeatsLeft = Reg(UInt(readCountWidth bits)) init (0)
  val readRspBurstContext = bmbParams.access.contextWidth > 0 generate Reg(
    Bits(bmbParams.access.contextWidth bits)
  ) init (0)
  val outstandingReadBeats = Reg(UInt(log2Up(readRspFifoDepth + 1) bits)) init (0)
  val queuedReadCmdValid = RegInit(False)
  val queuedReadCmd = Reg(
    De10BmbReadCmdPayload(avalonAddressWidth, bmbParams.access.contextWidth, readCountWidth)
  )

  private val bmbAddressWidth = bmbParams.access.addressWidth
  private val translatedAddressMin = U(addressBase, avalonAddressWidth bits)
  private val translatedAddressExclusive = U(
    addressBase + (addressMask & ((BigInt(1) << bmbAddressWidth) - 1)) + 1,
    avalonAddressWidth bits
  )
  private def translateAddress(address: UInt): UInt = {
    val truncatedMask = addressMask & ((BigInt(1) << bmbAddressWidth) - 1)
    val maskedAddress = address & U(truncatedMask, bmbAddressWidth bits)
    (maskedAddress.resize(avalonAddressWidth) +^ U(addressBase, avalonAddressWidth bits))
      .resize(avalonAddressWidth)
  }

  private val translatedAddress = translateAddress(io.bmb.cmd.address)
  private val beatShift = log2Up(bmbParams.access.byteCount)
  private val fullMask =
    B((BigInt(1) << bmbParams.access.maskWidth) - 1, bmbParams.access.maskWidth bits)
  private val cmdByteCount =
    io.bmb.cmd.length.resize(io.bmb.cmd.length.getWidth + 1 bits) + 1
  private val cmdBeatCount = (cmdByteCount >> beatShift).resize(readCountWidth bits)
  private val cmdBurstCount = cmdBeatCount.resize(De10MemBackend.avalonBurstCountWidth)
  private val cmdPayload =
    De10BmbReadCmdPayload(avalonAddressWidth, bmbParams.access.contextWidth, readCountWidth)
  cmdPayload.address := translatedAddress
  cmdPayload.beatCount := cmdBeatCount
  cmdPayload.burstCount := cmdBurstCount
  if (bmbParams.access.contextWidth > 0) {
    cmdPayload.context := io.bmb.cmd.context
  }

  val cmdIsWrite = io.bmb.cmd.opcode === Bmb.Cmd.Opcode.WRITE
  val queuedReadCmdAddressInRange =
    queuedReadCmd.address >= translatedAddressMin && queuedReadCmd.address < translatedAddressExclusive
  val queuedReadCmdSafe = queuedReadCmdValid && queuedReadCmdAddressInRange
  val pendingReadTraffic =
    queuedReadCmdValid || outstandingReadBeats =/= 0 || readRspFifo.io.occupancy =/= 0
  val readRspNow = readRspFifo.io.pop.valid && !writeRspPending
  val writeRspBlocksCmd = writeRspPending && !io.bmb.rsp.ready
  val readStorageUsedWidth = log2Up(readRspFifoDepth * 3 + 2)
  val queuedReadCmdBeats =
    Mux(queuedReadCmdValid, queuedReadCmd.beatCount, U(0, readCountWidth bits))
  val readStorageUsed = readRspFifo.io.occupancy.resize(readStorageUsedWidth) +
    outstandingReadBeats.resize(readStorageUsedWidth) +
    queuedReadCmdBeats.resize(readStorageUsedWidth)
  val readStorageCanAcceptCmd =
    readStorageUsed + cmdBeatCount.resize(readStorageUsedWidth) <= U(
      readRspFifoDepth,
      readStorageUsedWidth bits
    )
  val canLaunchRead =
    !writeBurstActive && !writeRspBlocksCmd && outstandingReadBeats === 0 && !io.mem.readDataValid && io.mem.waitRequestn
  val launchQueuedRead = canLaunchRead && queuedReadCmdSafe
  val launchDirectRead = False
  val readLaunch = launchQueuedRead || launchDirectRead
  val readQueueFreeThisCycle = canLaunchRead
  val acceptWriteBeat =
    io.bmb.cmd.valid && cmdIsWrite && !pendingReadTraffic && !writeRspBlocksCmd && io.mem.waitRequestn
  val queueAcceptedReadCmd =
    io.bmb.cmd.valid && !cmdIsWrite && !queuedReadCmdValid && readStorageCanAcceptCmd
  val acceptReadCmd = queueAcceptedReadCmd
  val useWriteRsp = writeRspPending
  val launchedReadCmd = queuedReadCmd

  io.mem.address := translatedAddress
  io.mem.read := False
  io.mem.write := False
  io.mem.burstCount := 1
  io.mem.byteEnable := 0
  io.mem.writeData := io.bmb.cmd.data

  io.bmb.cmd.ready := False
  io.bmb.rsp.valid := useWriteRsp || readRspNow
  io.bmb.rsp.last := Mux(useWriteRsp, True, readRspFifo.io.pop.payload.last)
  io.bmb.rsp.source := 0
  io.bmb.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  io.bmb.rsp.data := Mux(
    useWriteRsp,
    B(0, bmbParams.access.dataWidth bits),
    readRspFifo.io.pop.payload.data
  )
  if (bmbParams.access.contextWidth > 0) {
    io.bmb.rsp.context := Mux(useWriteRsp, writeRspContext, readRspFifo.io.pop.payload.context)
  }
  readRspFifo.io.pop.ready := !writeRspPending && io.bmb.rsp.ready

  readRspFifo.io.push.valid := io.mem.readDataValid
  readRspFifo.io.push.payload.data := io.mem.readData
  readRspFifo.io.push.payload.last := readRspBurstBeatsLeft === 1
  if (bmbParams.access.contextWidth > 0) {
    readRspFifo.io.push.payload.context := readRspBurstContext
  }

  when(writeRspPending && io.bmb.rsp.fire && !(acceptWriteBeat && io.bmb.cmd.last)) {
    writeRspPending := False
  }

  when(acceptWriteBeat) {
    io.mem.write := True
    io.mem.burstCount := 1
    io.mem.byteEnable := io.bmb.cmd.mask
    io.bmb.cmd.ready := True
    writeBurstActive := !io.bmb.cmd.last
    when(io.bmb.cmd.last) {
      writeRspPending := True
      if (bmbParams.access.contextWidth > 0) {
        writeRspContext := io.bmb.cmd.context
      }
    }
  }

  when(queueAcceptedReadCmd) {
    queuedReadCmdValid := True
    queuedReadCmd := cmdPayload
  }

  when(queuedReadCmdValid && (!queuedReadCmdAddressInRange || queuedReadCmd.beatCount === 0)) {
    queuedReadCmdValid := False
  } elsewhen (launchQueuedRead) {
    queuedReadCmdValid := False
  }

  when(readLaunch) {
    io.mem.read := True
    io.mem.address := launchedReadCmd.address
    io.mem.burstCount := launchedReadCmd.burstCount
    io.mem.byteEnable := fullMask
    outstandingReadBeats := outstandingReadBeats + launchedReadCmd.beatCount.resized
    readRspBurstActive := True
    readRspBurstBeatsLeft := launchedReadCmd.beatCount
    if (bmbParams.access.contextWidth > 0) {
      readRspBurstContext := launchedReadCmd.context
    }
  }

  when(acceptReadCmd) {
    io.bmb.cmd.ready := True
  }

  when(io.mem.readDataValid) {
    outstandingReadBeats := outstandingReadBeats - 1
    when(readRspBurstBeatsLeft === 1) {
      readRspBurstActive := False
      readRspBurstBeatsLeft := 0
    } otherwise {
      readRspBurstBeatsLeft := readRspBurstBeatsLeft - 1
    }
  }

  when(!readRspBurstActive && !readLaunch && !io.mem.readDataValid) {
    outstandingReadBeats := 0
  }

  when(outstandingReadBeats === 0 && !readLaunch && !io.mem.readDataValid) {
    readRspBurstActive := False
    readRspBurstBeatsLeft := 0
  }

  GenerationFlags.formal {
    assert(cmdBeatCount =/= 0)
    assert(io.bmb.rsp.source === 0)
    when(launchDirectRead) {
      assert(io.mem.burstCount === cmdBurstCount)
    }
    when(io.mem.write) {
      assert(io.mem.burstCount === 1)
    }
    when(io.bmb.rsp.valid && !useWriteRsp) {
      assert(readRspFifo.io.pop.valid)
    }
    when(readLaunch) {
      assert(io.mem.read)
    }
  }

  GenerationFlags.formal {
    val pastValid = RegNext(True) init False
    val memReadFire = io.mem.read && io.mem.waitRequestn
    val readRspFire = readRspFifo.io.pop.fire
    val readCmdHazard =
      writeBurstActive || writeRspBlocksCmd || !readStorageCanAcceptCmd || !readQueueFreeThisCycle
    val writeCmdHazard = pendingReadTraffic || writeRspBlocksCmd || !io.mem.waitRequestn
    val trackedOutstandingWidth = log2Up(readRspFifoDepth + maxReadBurstBeats + 1)
    val trackedPipelineWidth = log2Up(readRspFifoDepth * 2 + maxReadBurstBeats + 1)
    val trackedLaunchedOutstanding = Reg(UInt(trackedOutstandingWidth bits)) init (0)
    val trackedAcceptedOutstanding = Reg(UInt(trackedOutstandingWidth bits)) init (0)
    val trackedReturnedBuffered = Reg(UInt(log2Up(readRspFifoDepth + 1) bits)) init (0)
    val trackedAcceptedPipeline = Reg(UInt(trackedPipelineWidth bits)) init (0)

    when(memReadFire && !io.mem.readDataValid) {
      trackedLaunchedOutstanding := trackedLaunchedOutstanding + io.mem.burstCount.resize(
        trackedOutstandingWidth
      )
    } elsewhen (!memReadFire && io.mem.readDataValid) {
      trackedLaunchedOutstanding := trackedLaunchedOutstanding - 1
    }

    when(acceptReadCmd && !io.mem.readDataValid) {
      trackedAcceptedOutstanding := trackedAcceptedOutstanding + cmdBeatCount.resize(
        trackedOutstandingWidth
      )
    } elsewhen (!acceptReadCmd && io.mem.readDataValid) {
      trackedAcceptedOutstanding := trackedAcceptedOutstanding - 1
    }

    when(io.mem.readDataValid && !readRspFire) {
      trackedReturnedBuffered := trackedReturnedBuffered + 1
    } elsewhen (!io.mem.readDataValid && readRspFire) {
      trackedReturnedBuffered := trackedReturnedBuffered - 1
    }

    when(acceptReadCmd && !readRspFire) {
      trackedAcceptedPipeline := trackedAcceptedPipeline + cmdBeatCount.resize(trackedPipelineWidth)
    } elsewhen (!acceptReadCmd && readRspFire) {
      trackedAcceptedPipeline := trackedAcceptedPipeline - 1
    } elsewhen (acceptReadCmd && readRspFire) {
      trackedAcceptedPipeline := trackedAcceptedPipeline + cmdBeatCount.resize(
        trackedPipelineWidth
      ) - 1
    }

    assert(!(io.mem.read && io.mem.write))
    assert(!(writeRspPending && readRspNow))
    assert(io.mem.read === readLaunch)

    when(io.mem.read) {
      assert(io.mem.waitRequestn)
    }

    when(io.mem.read) {
      assert(io.mem.byteEnable === fullMask)
    }

    when(readRspBurstActive) {
      assert(readRspBurstBeatsLeft =/= 0)
      assert(outstandingReadBeats =/= 0)
      assert(readRspBurstBeatsLeft === outstandingReadBeats.resize(readCountWidth))
    } otherwise {
      assert(readRspBurstBeatsLeft === 0)
      assert(outstandingReadBeats === 0)
    }

    assert(trackedLaunchedOutstanding === outstandingReadBeats.resize(trackedOutstandingWidth))
    assert(
      trackedAcceptedOutstanding ===
        (outstandingReadBeats.resize(trackedOutstandingWidth) + queuedReadCmdBeats.resize(
          trackedOutstandingWidth
        ))
    )
    assert(trackedReturnedBuffered === readRspFifo.io.occupancy)
    assert(
      trackedAcceptedPipeline ===
        (outstandingReadBeats.resize(trackedPipelineWidth) +
          queuedReadCmdBeats.resize(trackedPipelineWidth) +
          readRspFifo.io.occupancy.resize(trackedPipelineWidth))
    )

    when(io.mem.readDataValid) {
      assert(readRspFifo.io.push.payload.last === (readRspBurstBeatsLeft === 1))
      if (bmbParams.access.contextWidth > 0) {
        assert(readRspFifo.io.push.payload.context === readRspBurstContext)
      }
    }

    when(readRspFifo.io.pop.valid && !writeRspPending) {
      assert(io.bmb.rsp.valid)
      assert(io.bmb.rsp.opcode === Bmb.Rsp.Opcode.SUCCESS)
      assert(io.bmb.rsp.source === 0)
      assert(io.bmb.rsp.last === readRspFifo.io.pop.payload.last)
      assert(io.bmb.rsp.data === readRspFifo.io.pop.payload.data)
      if (bmbParams.access.contextWidth > 0) {
        assert(io.bmb.rsp.context === readRspFifo.io.pop.payload.context)
      }
    }

    when(acceptWriteBeat) {
      assert(io.bmb.cmd.ready)
      assert(io.mem.write)
      assert(!io.mem.read)
      assert(io.mem.address === translatedAddress)
      assert(io.mem.byteEnable === io.bmb.cmd.mask)
      assert(io.mem.writeData === io.bmb.cmd.data)
    }

    when(readLaunch) {
      assert(io.mem.read)
      assert(!io.mem.write)
      assert(io.mem.byteEnable === fullMask)
      assert(io.mem.waitRequestn)
    }

    when(launchDirectRead) {
      assert(io.bmb.cmd.ready)
      assert(io.bmb.cmd.fire)
      assert(io.mem.address === translatedAddress)
      assert(io.mem.burstCount === cmdBurstCount)
    }

    when(launchQueuedRead) {
      assert(io.mem.address === queuedReadCmd.address)
      assert(io.mem.burstCount === queuedReadCmd.burstCount)
    }

    when(io.bmb.cmd.valid && !cmdIsWrite && !readCmdHazard) {
      assert(io.bmb.cmd.ready)
    }

    when(io.bmb.cmd.valid && cmdIsWrite && !writeCmdHazard) {
      assert(io.bmb.cmd.ready)
      assert(io.mem.write)
    }

    when(useWriteRsp) {
      assert(io.bmb.rsp.valid)
      assert(io.bmb.rsp.last)
      assert(io.bmb.rsp.source === 0)
      assert(io.bmb.rsp.data === 0)
      if (bmbParams.access.contextWidth > 0) {
        assert(io.bmb.rsp.context === writeRspContext)
      }
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

  private def fbBridge() = {
    val sourceRemover = BmbSourceRemover(Core.fbMemBmbParams(c))
    val bridge = De10BmbToAvalonMm(
      bmbParams = De10BmbToAvalonMm.singleSourceBridgeParams(Core.fbMemBmbParams(c)),
      avalonAddressWidth = De10MemBackend.physicalAddressWidth,
      addressBase = De10AddressMap.fbMemBase,
      addressMask = De10AddressMap.fbMemMask
    )
    sourceRemover.io.output <> bridge.io.bmb
    (sourceRemover, bridge)
  }

  private def texBridge() = {
    val sourceRemover = BmbSourceRemover(Core.texMemBmbParams(c))
    val bridge = De10BmbToAvalonMm(
      bmbParams = De10BmbToAvalonMm.singleSourceBridgeParams(Core.texMemBmbParams(c)),
      avalonAddressWidth = De10MemBackend.physicalAddressWidth,
      addressBase = De10AddressMap.texMemBase,
      addressMask = De10AddressMap.texMemMask
    )
    sourceRemover.io.output <> bridge.io.bmb
    (sourceRemover, bridge)
  }

  val (fbWriteSourceRemover, fbWriteBridge) = fbBridge()
  val (fbColorReadSourceRemover, fbColorReadBridge) = fbBridge()
  val (fbAuxReadSourceRemover, fbAuxReadBridge) = fbBridge()
  val (texSourceRemover, texBridgeInst) = texBridge()

  Seq(
    io.fbMemWrite <> fbWriteSourceRemover.io.input,
    io.fbColorReadMem <> fbColorReadSourceRemover.io.input,
    io.fbAuxReadMem <> fbAuxReadSourceRemover.io.input,
    io.texMem <> texSourceRemover.io.input,
    io.memFbWrite <> fbWriteBridge.io.mem,
    io.memFbColorRead <> fbColorReadBridge.io.mem,
    io.memFbAuxRead <> fbAuxReadBridge.io.mem,
    io.memTex <> texBridgeInst.io.mem
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
