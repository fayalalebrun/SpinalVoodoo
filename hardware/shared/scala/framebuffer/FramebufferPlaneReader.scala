package voodoo.framebuffer

import voodoo._
import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

case class FramebufferPlaneReader(c: Config) extends Component {
  import FramebufferPlaneReader._

  val addrWidth = c.addressWidth.value
  val maxSpanPixels = c.maxFbDims._1
  val spanLaneCountWidth = log2Up(maxSpanPixels + 1)
  val lineBufferLanes = (maxSpanPixels + 1) max 16

  case class SpanCmd() extends Bundle {
    val startAddress = UInt(addrWidth bits)
    val endAddress = UInt(addrWidth bits)
    val firstWordAddress = UInt(addrWidth bits)
    val lastWordAddress = UInt(addrWidth bits)
    val wordCount = UInt(spanLaneCountWidth bits)
    val laneCount = UInt(spanLaneCountWidth bits)
  }

  val io = new Bundle {
    val prefetchReq = slave(Stream(FramebufferPlaneReader.PrefetchReq(c)))
    val readReq = slave(Stream(ReadReq(c)))
    val readRsp = master(Stream(ReadRsp()))
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
    val cacheDebug = out Bits (64 bits)
    val cacheDebugReadAddr = out UInt (addrWidth bits)
    val cacheDebugExpectedAddr = out UInt (addrWidth bits)
    val cacheDebugRemaining = out UInt (spanLaneCountWidth bits)
    val cacheDebugOccupancy = out Bits (32 bits)
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

  val issueQueueDepth = 64
  val spanIssueQueue = StreamFifo(HardType(SpanCmd()), issueQueueDepth)
  val issuedSpanQueue = StreamFifo(HardType(SpanCmd()), 4)
  val laneFifo = StreamFifo(Bits(16 bits), lineBufferLanes)
  val directMissLaneQueue = StreamFifo(Bool(), 8)
  val readRspFifo = StreamFifo(ReadRsp(), 16)

  val fillHits = Reg(UInt(32 bits)) init (0)
  val fillMisses = Reg(UInt(32 bits)) init (0)
  val fillBurstCount = Reg(UInt(32 bits)) init (0)
  val fillBurstBeats = Reg(UInt(32 bits)) init (0)
  val fillStallCycles = Reg(UInt(32 bits)) init (0)
  val reqCount = Reg(UInt(32 bits)) init (0)
  val reqForwardStepCount = Reg(UInt(32 bits)) init (0)
  val reqBackwardStepCount = Reg(UInt(32 bits)) init (0)
  val reqSameWordCount = Reg(UInt(32 bits)) init (0)

  val reqOtherCount = Reg(UInt(32 bits)) init (0)
  val singleBeatBurstCount = Reg(UInt(32 bits)) init (0)
  val multiBeatBurstCount = Reg(UInt(32 bits)) init (0)
  val maxOccupancy = Reg(UInt(8 bits)) init (0)
  val prevReqValid = RegInit(False)
  val prevReqAddr = Reg(UInt(addrWidth bits)) init (0)

  val internalMem = Bmb(FramebufferPlaneReader.internalBmbParams(c))
  if (FramebufferPlaneReader.needBurstSplitter(c)) {
    val burstSplitter =
      BmbAlignedSpliter(FramebufferPlaneReader.internalBmbParams(c), 1 << c.memBurstLengthWidth)
    val contextRemover =
      BmbContextRemover(FramebufferPlaneReader.splitterBmbParams(c), pendingMax = issueQueueDepth)
    burstSplitter.io.input <> internalMem
    contextRemover.io.input <> burstSplitter.io.output
    contextRemover.io.output <> io.mem
  } else {
    internalMem <> io.mem
  }

  def exactLaneAddress(address: UInt): UInt =
    (address(address.getWidth - 1 downto 1) ## U"1'b0").asUInt

  def makeSpan(req: FramebufferPlaneReader.PrefetchReq): SpanCmd = {
    val span = SpanCmd()
    val startExact = exactLaneAddress(req.startAddress)
    val endExact = exactLaneAddress(req.endAddress)
    span.startAddress := startExact
    span.endAddress := endExact
    span.firstWordAddress := alignedWordAddress(startExact)
    span.lastWordAddress := alignedWordAddress(endExact)
    span.wordCount := (((alignedWordAddress(endExact) - alignedWordAddress(startExact)) >> 2)
      .resize(
        spanLaneCountWidth bits
      ) + 1).resized
    span.laneCount := (((endExact - startExact) >> 1).resize(spanLaneCountWidth bits) + 1).resized
    span
  }

  def classifyReqStep(curr: UInt, prev: UInt): UInt = {
    val sameWord = alignedWordAddress(curr) === alignedWordAddress(prev)
    val forward = curr === (prev + U(2, addrWidth bits)).resized
    val backward = prev === (curr + U(2, addrWidth bits)).resized
    sameWord ? U(0, 3 bits) | (forward ? U(1, 3 bits) | (backward ? U(2, 3 bits) | U(4, 3 bits)))
  }

  def recordReadRequest(address: UInt): Unit = {
    reqCount := reqCount + 1
    when(prevReqValid) {
      switch(classifyReqStep(address, prevReqAddr)) {
        is(U(0, 3 bits)) {
          reqSameWordCount := reqSameWordCount + 1
        }
        is(U(1, 3 bits)) {
          reqForwardStepCount := reqForwardStepCount + 1
        }
        is(U(2, 3 bits)) {
          reqBackwardStepCount := reqBackwardStepCount + 1
        }
        default {
          reqOtherCount := reqOtherCount + 1
        }
      }
    }
    prevReqValid := True
    prevReqAddr := address
  }

  val incomingSpan = makeSpan(io.prefetchReq.payload)
  when(io.prefetchReq.fire) {
    fillMisses := fillMisses + 1
  }

  // The cached reader is fed by a span prefetch stream, while the later pixel
  // stream can legally skip pixels before framebuffer access (for example due
  // to pipeline-side kills).  Track the next prefetched lane expected by the
  // consumer and allow forward skips by discarding unused cached lanes.  A
  // backward/non-monotonic request remains an invariant violation.
  val consumeQueue = StreamFifo(HardType(SpanCmd()), issueQueueDepth)
  val consumeActive = RegInit(False)
  val consumeExpectedAddr = Reg(UInt(addrWidth bits)) init (0)
  val consumeRemainingLanes = Reg(UInt(spanLaneCountWidth bits)) init (0)
  val consumeBoot = !consumeActive && consumeQueue.io.pop.valid
  val consumeCurrentAddr = UInt(addrWidth bits)
  val consumeCurrentRemaining = UInt(spanLaneCountWidth bits)
  consumeCurrentAddr := consumeExpectedAddr
  consumeCurrentRemaining := consumeRemainingLanes
  when(!consumeActive) {
    consumeCurrentAddr := consumeQueue.io.pop.payload.startAddress
    consumeCurrentRemaining := consumeQueue.io.pop.payload.laneCount
  }
  val consumeExpectedValid = consumeActive || consumeBoot
  val consumeAddrMatches = io.readReq.address === consumeCurrentAddr
  val consumeAddrAhead = io.readReq.address > consumeCurrentAddr
  val consumeInvariantOk = !io.readReq.valid || (consumeExpectedValid && consumeAddrMatches)

  spanIssueQueue.io.push.valid := io.prefetchReq.valid && consumeQueue.io.push.ready
  spanIssueQueue.io.push.payload := incomingSpan
  consumeQueue.io.push.valid := io.prefetchReq.valid && spanIssueQueue.io.push.ready
  consumeQueue.io.push.payload := incomingSpan
  io.prefetchReq.ready := spanIssueQueue.io.push.ready && consumeQueue.io.push.ready

  def advanceConsume(onFirstLane: Bool): Unit = {
    when(consumeCurrentRemaining === 1) {
      consumeActive := False
      consumeExpectedAddr := 0
      consumeRemainingLanes := 0
    }.otherwise {
      consumeActive := True
      consumeExpectedAddr := (consumeCurrentAddr + U(2, addrWidth bits)).resized
      consumeRemainingLanes := consumeCurrentRemaining - 1
    }
    consumeQueue.io.pop.ready := onFirstLane
  }
  consumeQueue.io.pop.ready := False

  val activeFillValid = issuedSpanQueue.io.pop.valid
  val activeFillSpan = issuedSpanQueue.io.pop.payload
  val activeFillWordIndex = Reg(UInt(spanLaneCountWidth bits)) init (0)
  activeFillValid.simPublic()
  activeFillSpan.wordCount.simPublic()
  activeFillWordIndex.simPublic()

  val pendingLaneValid = RegInit(False)
  val pendingLaneData = Reg(Bits(16 bits)) init (0)
  pendingLaneValid.simPublic()

  // If the consumer is behind the prefetch stream (or no prefetched span is
  // available), fall back to an ordered direct read.
  val directMissNeeded = !consumeExpectedValid ||
    (consumeExpectedValid && !consumeAddrMatches && !consumeAddrAhead)
  val directMissCmdValid = io.readReq.valid && directMissNeeded && directMissLaneQueue.io.push.ready
  val prefetchCmdValid = spanIssueQueue.io.pop.valid && issuedSpanQueue.io.push.ready
  val directMissCmdSelected = directMissCmdValid
  val prefetchCmdSelected = prefetchCmdValid && !directMissCmdSelected

  internalMem.cmd.valid := directMissCmdSelected || prefetchCmdSelected
  internalMem.cmd.fragment.address := directMissCmdSelected ? alignedWordAddress(
    io.readReq.address
  ) | spanIssueQueue.io.pop.payload.firstWordAddress
  internalMem.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
  internalMem.cmd.fragment.length := directMissCmdSelected ? U(
    3,
    internalMem.cmd.fragment.length.getWidth bits
  ) | (spanIssueQueue.io.pop.payload.lastWordAddress - spanIssueQueue.io.pop.payload.firstWordAddress + 3).resized
  internalMem.cmd.fragment.source := directMissCmdSelected.asUInt.resized
  internalMem.cmd.fragment.data := 0
  internalMem.cmd.fragment.mask := 0
  internalMem.cmd.last := True
  spanIssueQueue.io.pop.ready := internalMem.cmd.ready && issuedSpanQueue.io.push.ready && !directMissCmdSelected
  issuedSpanQueue.io.push.valid := internalMem.cmd.fire && prefetchCmdSelected
  issuedSpanQueue.io.push.payload := spanIssueQueue.io.pop.payload
  directMissLaneQueue.io.push.valid := internalMem.cmd.fire && directMissCmdSelected
  directMissLaneQueue.io.push.payload := io.readReq.address(1)
  issuedSpanQueue.io.pop.ready := False
  when(internalMem.cmd.fire && prefetchCmdSelected) {
    fillBurstCount := fillBurstCount + 1
    fillBurstBeats := fillBurstBeats + spanIssueQueue.io.pop.payload.wordCount.resize(32 bits)
    when(spanIssueQueue.io.pop.payload.wordCount === 1) {
      singleBeatBurstCount := singleBeatBurstCount + 1
    }.otherwise {
      multiBeatBurstCount := multiBeatBurstCount + 1
    }
  }

  val currentFillWordAddress =
    (activeFillSpan.firstWordAddress + (activeFillWordIndex.resize(addrWidth bits) << 2)).resized
  val emitLo =
    currentFillWordAddress =/= activeFillSpan.firstWordAddress || !activeFillSpan.startAddress(1)
  val emitHi =
    currentFillWordAddress =/= activeFillSpan.lastWordAddress || activeFillSpan.endAddress(1)
  val readDataLo = internalMem.rsp.fragment.data(15 downto 0)
  val readDataHi = internalMem.rsp.fragment.data(31 downto 16)
  val prefetchMemRspValid = internalMem.rsp.valid && internalMem.rsp.source === 0
  val directMissMemRspValid = internalMem.rsp.valid && internalMem.rsp.source === 1
  val canAcceptMemRsp = activeFillValid && !pendingLaneValid && laneFifo.io.push.ready

  internalMem.rsp.ready := (canAcceptMemRsp && internalMem.rsp.source === 0) || (directMissMemRspValid && directMissLaneQueue.io.pop.valid && readRspFifo.io.push.ready)
  laneFifo.io.push.valid := pendingLaneValid || (prefetchMemRspValid && activeFillValid && emitLo)
  laneFifo.io.push.payload := pendingLaneValid ? pendingLaneData | readDataLo

  when(pendingLaneValid && laneFifo.io.push.ready) {
    pendingLaneValid := False
  }

  when(internalMem.rsp.fire && internalMem.rsp.source === 0) {
    when(!emitLo && emitHi) {
      laneFifo.io.push.valid := True
      laneFifo.io.push.payload := readDataHi
    }.elsewhen(emitLo && emitHi) {
      pendingLaneValid := True
      pendingLaneData := readDataHi
    }

    // `rsp.last` may mark the end of a splitter fragment rather than the end
    // of the original fill span. Retire only after all requested words return.
    when(activeFillWordIndex === activeFillSpan.wordCount - 1) {
      issuedSpanQueue.io.pop.ready := True
      activeFillWordIndex := 0
    }.otherwise {
      activeFillWordIndex := activeFillWordIndex + 1
    }
  }

  // Direct misses return later through the same ordered read response stream.
  // Do not let a later cached hit push an immediate response ahead of an
  // older outstanding direct miss response.
  val directMissOutstanding = directMissLaneQueue.io.occupancy =/= 0
  val readCanServe =
    laneFifo.io.pop.valid && consumeExpectedValid && consumeAddrMatches && !directMissOutstanding
  val skipCachedLane =
    io.readReq.valid && consumeExpectedValid && !consumeAddrMatches && consumeAddrAhead && laneFifo.io.pop.valid
  val directMissReadReady = directMissCmdSelected && internalMem.cmd.ready
  val cachedReadFire = io.readReq.valid && readRspFifo.io.push.ready && readCanServe
  val directMissRspFire =
    directMissMemRspValid && directMissLaneQueue.io.pop.valid && readRspFifo.io.push.ready
  io.readReq.ready := (readRspFifo.io.push.ready && readCanServe) || directMissReadReady
  readRspFifo.io.push.valid := cachedReadFire || directMissRspFire
  readRspFifo.io.push.payload.data := directMissRspFire ? (directMissLaneQueue.io.pop.payload ? readDataHi | readDataLo) | laneFifo.io.pop.payload
  laneFifo.io.pop.ready := cachedReadFire || skipCachedLane
  directMissLaneQueue.io.pop.ready := directMissRspFire

  when(cachedReadFire || skipCachedLane) {
    advanceConsume(!consumeActive)
  }

  when(io.readReq.valid && !io.readReq.ready) {
    fillStallCycles := fillStallCycles + 1
  }

  when(io.readReq.fire) {
    recordReadRequest(io.readReq.address)
    fillHits := fillHits + 1
  }

  GenerationFlags.formal {
    when(cachedReadFire) {
      assert(consumeExpectedValid)
      assert(consumeAddrMatches)
      assert(!directMissOutstanding)
    }
    when(skipCachedLane) {
      assert(consumeExpectedValid)
      assert(!consumeAddrMatches)
      assert(consumeAddrAhead)
      assert(laneFifo.io.pop.valid)
    }
    when(directMissReadReady) {
      assert(directMissNeeded)
      assert(directMissLaneQueue.io.push.ready)
      assert(!laneFifo.io.pop.ready)
    }
    when(directMissRspFire) {
      assert(directMissLaneQueue.io.pop.valid)
    }
  }

  {
    val cacheDebug = Bits(64 bits)
    val cacheDebugOccupancy = Bits(32 bits)
    cacheDebug := 0
    cacheDebug(0) := io.readReq.valid
    cacheDebug(1) := io.readReq.ready
    cacheDebug(2) := consumeExpectedValid
    cacheDebug(3) := consumeAddrMatches
    cacheDebug(4) := consumeInvariantOk
    cacheDebug(5) := laneFifo.io.pop.valid
    cacheDebug(6) := readRspFifo.io.push.ready
    cacheDebug(7) := io.prefetchReq.fire
    cacheDebug(8) := internalMem.cmd.fire
    cacheDebug(9) := internalMem.rsp.fire
    cacheDebug(10) := consumeActive
    cacheDebug(11) := consumeBoot
    cacheDebug(12) := spanIssueQueue.io.pop.valid
    cacheDebug(13) := issuedSpanQueue.io.pop.valid
    cacheDebug(14) := pendingLaneValid
    cacheDebug(15) := skipCachedLane
    cacheDebug(16) := directMissCmdValid
    cacheDebug(17) := directMissCmdSelected
    cacheDebug(18) := directMissReadReady
    cacheDebug(19) := directMissOutstanding
    cacheDebug(20) := directMissLaneQueue.io.pop.valid
    cacheDebug(21) := directMissMemRspValid
    cacheDebug(22) := laneFifo.io.pop.ready
    io.cacheDebug := cacheDebug
    io.cacheDebugReadAddr := io.readReq.address
    io.cacheDebugExpectedAddr := consumeCurrentAddr
    io.cacheDebugRemaining := consumeCurrentRemaining
    cacheDebugOccupancy := 0
    cacheDebugOccupancy(6 downto 0) := consumeQueue.io.occupancy.asBits.resized
    cacheDebugOccupancy(15 downto 8) := laneFifo.io.occupancy.asBits.resized
    cacheDebugOccupancy(18 downto 16) := issuedSpanQueue.io.occupancy.asBits.resized
    cacheDebugOccupancy(23 downto 19) := readRspFifo.io.occupancy.asBits.resized
    cacheDebugOccupancy(27 downto 24) := directMissLaneQueue.io.occupancy.asBits.resized
    io.cacheDebugOccupancy := cacheDebugOccupancy

    consumeActive.simPublic()
    consumeBoot.simPublic()
    consumeExpectedAddr.simPublic()
    consumeRemainingLanes.simPublic()
    consumeCurrentAddr.simPublic()
    consumeCurrentRemaining.simPublic()
    consumeExpectedValid.simPublic()
    consumeAddrMatches.simPublic()
    consumeInvariantOk.simPublic()
    skipCachedLane.simPublic()
    consumeQueue.io.occupancy.simPublic()
    laneFifo.io.occupancy.simPublic()
    issuedSpanQueue.io.occupancy.simPublic()
    readRspFifo.io.occupancy.simPublic()
  }

  when(laneFifo.io.occupancy.resize(8 bits) > maxOccupancy) {
    maxOccupancy := laneFifo.io.occupancy.resize(8 bits)
  }

  io.readRsp << readRspFifo.io.pop

  io.busy := spanIssueQueue.io.occupancy =/= 0 || issuedSpanQueue.io.occupancy =/= 0 ||
    pendingLaneValid || laneFifo.io.occupancy =/= 0 || readRspFifo.io.occupancy =/= 0

  io.fillHits := fillHits
  io.fillMisses := fillMisses
  io.fillBurstCount := fillBurstCount
  io.fillBurstBeats := fillBurstBeats
  io.fillStallCycles := fillStallCycles
  io.reqCount := reqCount
  io.reqForwardStepCount := reqForwardStepCount
  io.reqBackwardStepCount := reqBackwardStepCount
  io.reqSameWordCount := reqSameWordCount
  io.reqSameLineCount := 0
  io.reqOtherCount := reqOtherCount
  io.singleBeatBurstCount := singleBeatBurstCount
  io.multiBeatBurstCount := multiBeatBurstCount
  io.maxOccupancy := maxOccupancy
}

object FramebufferPlaneReader {
  case class ReadReq(c: Config) extends Bundle {
    val address = UInt(c.addressWidth)
  }

  case class ReadRsp() extends Bundle {
    val data = Bits(16 bits)
  }

  def alignedWordAddress(address: UInt): UInt =
    (address(address.getWidth - 1 downto 2) ## U"2'b00").asUInt

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
