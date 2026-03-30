package voodoo.framebuffer

import voodoo._
import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

case class FramebufferPlaneReader(c: Config, prefetchWords: Int = 16, bufferWords: Int = 256)
    extends Component {
  import FramebufferPlaneBuffer._

  case class SpanCmd() extends Bundle {
    val base = UInt(addrWidth bits)
    val last = UInt(addrWidth bits)
    val words = UInt(spanWordCountWidth bits)
  }

  case class BufferLookup() extends Bundle {
    val hit = Bool()
    val hitIndex = UInt(log2Up(bufferWords) bits)
    val occupancy = UInt(log2Up(bufferWords + 1) bits)
    val freeCount = UInt(log2Up(bufferWords + 1) bits)
    val firstFreeIndex = UInt(log2Up(bufferWords) bits)
  }

  val io = new Bundle {
    val prefetchReq = slave Stream (FramebufferPlaneReader.PrefetchReq(c))
    val readReq = slave Stream (ReadReq(c))
    val readRsp = master Stream (ReadRsp())
    val mem = master(Bmb(FramebufferPlaneReader.bmbParams(c)))
    val busy = out Bool ()
    val fillHits = out UInt (32 bits)
    val fillMisses = out UInt (32 bits)
    val fillBurstCount = out UInt (32 bits)
    val fillBurstBeats = out UInt (32 bits)
    val fillStallCycles = out UInt (32 bits)
    val reqCount = out UInt (32 bits)
    val reqForwardStepCount = out UInt (32 bits)
    val reqBackwardStepCount = out UInt (32 bits)
    val reqSameWordCount = out UInt (32 bits)
    val reqSameLineCount = out UInt (32 bits)
    val reqOtherCount = out UInt (32 bits)
    val singleBeatBurstCount = out UInt (32 bits)
    val multiBeatBurstCount = out UInt (32 bits)
    val maxOccupancy = out UInt (8 bits)
  }
  io.prefetchReq.valid.simPublic()
  io.prefetchReq.ready.simPublic()
  io.prefetchReq.startAddress.simPublic()
  io.prefetchReq.endAddress.simPublic()
  io.readReq.valid.simPublic()
  io.readReq.ready.simPublic()
  io.readReq.address.simPublic()
  io.readRsp.valid.simPublic()
  io.readRsp.ready.simPublic()
  io.mem.cmd.valid.simPublic()
  io.mem.cmd.ready.simPublic()
  io.mem.cmd.fragment.address.simPublic()
  io.mem.cmd.fragment.length.simPublic()
  io.mem.rsp.valid.simPublic()
  io.mem.rsp.ready.simPublic()

  require(bufferWords >= prefetchWords)

  val addrWidth = c.addressWidth.value
  val countWidth = log2Up(bufferWords + 1)
  val lineBytes = c.fbWriteBufferLineWords * 4
  val maxSpanWords = (c.maxFbDims._1 + 1) / 2
  val maxSpanBytes = maxSpanWords * 4
  val spanWordCountWidth = log2Up(maxSpanWords + 1)

  def wordBase(address: UInt): UInt = ((address >> 2) << 2).resize(addrWidth bits)
  def lineBaseOf(address: UInt): UInt = {
    val shift = log2Up(lineBytes)
    ((address >> shift) << shift).resize(addrWidth bits)
  }
  def classifyPrefetchStep(curr: UInt, prev: UInt): UInt = {
    val sameWord = wordBase(curr) === wordBase(prev)
    val forward = curr === (prev + U(2, addrWidth bits)).resized
    val backward = prev === (curr + U(2, addrWidth bits)).resized
    val sameLine = lineBaseOf(curr) === lineBaseOf(prev)
    sameWord ? U(0, 3 bits) | (forward ? U(1, 3 bits) | (backward ? U(2, 3 bits) | (sameLine ? U(
      3,
      3 bits
    ) | U(4, 3 bits))))
  }

  val bufferValid = Vec(Reg(Bool()) init (False), bufferWords)
  val bufferAddr = Vec(Reg(UInt(addrWidth bits)) init (0), bufferWords)
  val bufferData = Vec(Reg(Bits(32 bits)) init (0), bufferWords)
  bufferValid.foreach(_.simPublic())
  bufferAddr.foreach(_.simPublic())

  val replayValid = Reg(Bool()) init (False)
  val replayAddr = Reg(UInt(addrWidth bits)) init (0)
  val replayData = Reg(Bits(32 bits)) init (0)
  replayValid.simPublic()
  replayAddr.simPublic()
  replayData.simPublic()

  val spanQueueDepth = c.maxFbDims._2 * 2
  val pendingSpanQueue = StreamFifo(HardType(SpanCmd()), spanQueueDepth)
  val issuedSpanQueue = StreamFifo(HardType(SpanCmd()), spanQueueDepth)
  val issuedRspIndex = Reg(UInt(spanWordCountWidth bits)) init (0)
  pendingSpanQueue.io.occupancy.simPublic()
  issuedSpanQueue.io.occupancy.simPublic()
  issuedRspIndex.simPublic()

  val readRspFifo = StreamFifo(ReadRsp(), 16)
  readRspFifo.io.push.valid := False
  readRspFifo.io.push.data := 0

  val fillHits = Reg(UInt(32 bits)) init (0)
  val fillMisses = Reg(UInt(32 bits)) init (0)
  val fillBurstCount = Reg(UInt(32 bits)) init (0)
  val fillBurstBeats = Reg(UInt(32 bits)) init (0)
  val fillStallCycles = Reg(UInt(32 bits)) init (0)
  val reqCount = Reg(UInt(32 bits)) init (0)
  val reqForwardStepCount = Reg(UInt(32 bits)) init (0)
  val reqBackwardStepCount = Reg(UInt(32 bits)) init (0)
  val reqSameWordCount = Reg(UInt(32 bits)) init (0)
  val reqSameLineCount = Reg(UInt(32 bits)) init (0)
  val reqOtherCount = Reg(UInt(32 bits)) init (0)
  val singleBeatBurstCount = Reg(UInt(32 bits)) init (0)
  val multiBeatBurstCount = Reg(UInt(32 bits)) init (0)
  val maxOccupancy = Reg(UInt(8 bits)) init (0)
  val prevPrefetchValid = Reg(Bool()) init (False)
  val prevPrefetchAddr = Reg(UInt(addrWidth bits)) init (0)

  val internalMem = Bmb(FramebufferPlaneReader.internalBmbParams(c))
  if (FramebufferPlaneReader.needBurstSplitter(c)) {
    val burstSplitter =
      BmbAlignedSpliter(FramebufferPlaneReader.internalBmbParams(c), 1 << c.memBurstLengthWidth)
    val contextRemover =
      BmbContextRemover(FramebufferPlaneReader.splitterBmbParams(c), pendingMax = spanQueueDepth)
    burstSplitter.io.input <> internalMem
    contextRemover.io.input <> burstSplitter.io.output
    contextRemover.io.output <> io.mem
  } else {
    internalMem <> io.mem
  }

  if (c.trace.enabled) {
    fillHits.simPublic()
    fillMisses.simPublic()
    fillBurstCount.simPublic()
    fillBurstBeats.simPublic()
    fillStallCycles.simPublic()
    reqCount.simPublic()
    reqForwardStepCount.simPublic()
    reqBackwardStepCount.simPublic()
    reqSameWordCount.simPublic()
    reqSameLineCount.simPublic()
    reqOtherCount.simPublic()
    singleBeatBurstCount.simPublic()
    multiBeatBurstCount.simPublic()
    maxOccupancy.simPublic()
  }

  io.fillHits := fillHits
  io.fillMisses := fillMisses
  io.fillBurstCount := fillBurstCount
  io.fillBurstBeats := fillBurstBeats
  io.fillStallCycles := fillStallCycles
  io.reqCount := reqCount
  io.reqForwardStepCount := reqForwardStepCount
  io.reqBackwardStepCount := reqBackwardStepCount
  io.reqSameWordCount := reqSameWordCount
  io.reqSameLineCount := reqSameLineCount
  io.reqOtherCount := reqOtherCount
  io.singleBeatBurstCount := singleBeatBurstCount
  io.multiBeatBurstCount := multiBeatBurstCount
  io.maxOccupancy := maxOccupancy

  fillMisses := fillMisses
  fillStallCycles := fillStallCycles
  singleBeatBurstCount := singleBeatBurstCount
  multiBeatBurstCount := multiBeatBurstCount

  def bufferLookup(address: UInt): BufferLookup = {
    val lookup = BufferLookup()
    val hitVec = Vec(Bool(), bufferWords)
    val freeVec = Vec(Bool(), bufferWords)
    val word = wordBase(address)
    for (idx <- 0 until bufferWords) {
      freeVec(idx) := !bufferValid(idx)
      hitVec(idx) := bufferValid(idx) && bufferAddr(idx) === word
    }
    lookup.hit := hitVec.asBits.orR
    lookup.hitIndex := OHToUInt(hitVec.asBits)
    lookup.occupancy := bufferValid.map(_.asUInt.resize(countWidth bits)).reduce(_ + _)
    lookup.freeCount := freeVec.map(_.asUInt.resize(countWidth bits)).reduce(_ + _)
    lookup.firstFreeIndex := OHToUInt(OHMasking.first(freeVec.asBits))
    lookup
  }

  def recordPrefetchAddress(address: UInt): Unit = {
    reqCount := reqCount + 1
    when(prevPrefetchValid) {
      switch(classifyPrefetchStep(address, prevPrefetchAddr)) {
        is(U(0, 3 bits)) {
          reqSameWordCount := reqSameWordCount + 1
        }
        is(U(1, 3 bits)) {
          reqForwardStepCount := reqForwardStepCount + 1
        }
        is(U(2, 3 bits)) {
          reqBackwardStepCount := reqBackwardStepCount + 1
        }
        is(U(3, 3 bits)) {
          reqSameLineCount := reqSameLineCount + 1
        }
        default {
          reqOtherCount := reqOtherCount + 1
        }
      }
    }
    prevPrefetchValid := True
    prevPrefetchAddr := address
  }

  def serveReadHit(hitIndex: UInt, replayHit: Bool): Unit = {
    fillHits := fillHits + 1
    readRspFifo.io.push.valid := True
    when(replayHit) {
      readRspFifo.io.push.data := replayData
    }.otherwise {
      readRspFifo.io.push.data := bufferData(hitIndex)
      replayValid := True
      replayAddr := bufferAddr(hitIndex)
      replayData := bufferData(hitIndex)
      bufferValid(hitIndex) := False
    }
  }

  val readLookup = bufferLookup(io.readReq.address)
  val bufferHit = readLookup.hit
  val bufferHitIndex = readLookup.hitIndex
  val bufferOccupancy = readLookup.occupancy
  val freeCount = readLookup.freeCount
  val firstFreeIndex = readLookup.firstFreeIndex
  val replayHit = replayValid && replayAddr === wordBase(io.readReq.address)
  val queueEmpty = !pendingSpanQueue.io.pop.valid && !issuedSpanQueue.io.pop.valid

  pendingSpanQueue.io.push.valid := io.prefetchReq.valid
  pendingSpanQueue.io.push.payload.base := wordBase(io.prefetchReq.startAddress)
  pendingSpanQueue.io.push.payload.last := wordBase(io.prefetchReq.endAddress)
  pendingSpanQueue.io.push.payload.words := (((wordBase(io.prefetchReq.endAddress) - wordBase(
    io.prefetchReq.startAddress
  )) >> 2).resize(spanWordCountWidth bits) + 1).resized
  io.prefetchReq.ready := pendingSpanQueue.io.push.ready
  when(io.prefetchReq.fire) {
    fillMisses := fillMisses + 1
    recordPrefetchAddress(wordBase(io.prefetchReq.startAddress))
  }

  val canIssueSpan = pendingSpanQueue.io.pop.valid && issuedSpanQueue.io.push.ready
  internalMem.cmd.valid := canIssueSpan
  internalMem.cmd.fragment.address := pendingSpanQueue.io.pop.payload.base
  internalMem.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
  internalMem.cmd.fragment.length :=
    (pendingSpanQueue.io.pop.payload.last - pendingSpanQueue.io.pop.payload.base + 3).resized
  internalMem.cmd.fragment.source := 0
  internalMem.cmd.fragment.data := 0
  internalMem.cmd.fragment.mask := 0
  internalMem.cmd.last := True
  pendingSpanQueue.io.pop.ready := internalMem.cmd.ready && issuedSpanQueue.io.push.ready
  issuedSpanQueue.io.push.valid := internalMem.cmd.fire
  issuedSpanQueue.io.push.payload := pendingSpanQueue.io.pop.payload
  when(internalMem.cmd.fire) {
    fillBurstCount := fillBurstCount + 1
    fillBurstBeats := fillBurstBeats + pendingSpanQueue.io.pop.payload.words.resize(32 bits)
    when(pendingSpanQueue.io.pop.payload.words === 1) {
      singleBeatBurstCount := singleBeatBurstCount + 1
    }.otherwise {
      multiBeatBurstCount := multiBeatBurstCount + 1
    }
  }

  internalMem.rsp.ready := issuedSpanQueue.io.pop.valid && freeCount =/= 0 && internalMem.rsp.source === 0
  when(internalMem.rsp.fire && internalMem.rsp.source === 0) {
    bufferValid(firstFreeIndex) := True
    bufferAddr(firstFreeIndex) :=
      (issuedSpanQueue.io.pop.payload.base + (issuedRspIndex.resize(addrWidth bits) << 2)).resized
    bufferData(firstFreeIndex) := internalMem.rsp.fragment.data
    when(internalMem.rsp.last || issuedRspIndex === issuedSpanQueue.io.pop.payload.words - 1) {
      issuedRspIndex := 0
    }.otherwise {
      issuedRspIndex := issuedRspIndex + 1
    }
  }
  issuedSpanQueue.io.pop.ready := internalMem.rsp.fire &&
    (internalMem.rsp.last || issuedRspIndex === issuedSpanQueue.io.pop.payload.words - 1)

  when(bufferOccupancy.resize(8 bits) > maxOccupancy) {
    maxOccupancy := bufferOccupancy.resize(8 bits)
  }

  io.readReq.ready := readRspFifo.io.push.ready && (replayHit || bufferHit)
  when(io.readReq.valid && !io.readReq.ready) {
    fillStallCycles := fillStallCycles + 1
  }
  when(io.readReq.fire) {
    serveReadHit(bufferHitIndex, replayHit)
  }

  io.readRsp << readRspFifo.io.pop

  io.busy := !queueEmpty || readRspFifo.io.pop.valid || bufferOccupancy =/= 0
}

object FramebufferPlaneReader {
  case class PrefetchReq(c: Config) extends Bundle {
    val startAddress = UInt(c.addressWidth.value bits)
    val endAddress = UInt(c.addressWidth.value bits)
  }

  def needBurstSplitter(c: Config): Boolean = {
    val maxSpanBytes = ((c.maxFbDims._1 + 1) / 2) * 4
    maxSpanBytes > (1 << c.memBurstLengthWidth)
  }

  def internalBmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = if (needBurstSplitter(c)) {
      (c.memBurstLengthWidth + 1) max log2Up(((c.maxFbDims._1 + 1) / 2) * 4)
    } else {
      c.memBurstLengthWidth
    },
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  def splitterBmbParams(c: Config) =
    BmbParameter(
      BmbAlignedSpliter.outputParameter(internalBmbParams(c).access, 1 << c.memBurstLengthWidth)
    )

  def bmbParams(c: Config) = if (needBurstSplitter(c)) {
    BmbContextRemover.getOutputParameter(splitterBmbParams(c))
  } else {
    internalBmbParams(c)
  }
}
