package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

case class FramebufferPlaneReader(c: Config, prefetchWords: Int = 16, bufferWords: Int = 32)
    extends Component {
  import FramebufferPlaneBuffer._

  val io = new Bundle {
    val prefetchReq = slave Flow (ReadReq(c))
    val readReq = slave Stream (ReadReq(c))
    val readRsp = master Stream (ReadRsp())
    val mem = master(Bmb(bmbParams(c)))
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

  require(prefetchWords >= 2 && ((prefetchWords & (prefetchWords - 1)) == 0))
  require(bufferWords >= prefetchWords)

  val addrWidth = c.addressWidth.value
  val beatIndexWidth = log2Up(prefetchWords)
  val countWidth = log2Up(bufferWords + 1)
  val lineBytes = c.fbWriteBufferLineWords * 4

  def wordBase(address: UInt): UInt = ((address >> 2) << 2).resize(addrWidth bits)
  def lineBaseOf(address: UInt): UInt = {
    val shift = log2Up(lineBytes)
    ((address >> shift) << shift).resize(addrWidth bits)
  }
  def inWindow(address: UInt, base: UInt, words: Int): Bool = {
    val aligned = wordBase(address)
    aligned >= base && aligned < (base + U(words * 4, addrWidth bits)).resized
  }

  val bufferValid = Vec(Reg(Bool()) init (False), bufferWords)
  val bufferAddr = Vec(Reg(UInt(addrWidth bits)) init (0), bufferWords)
  val bufferData = Vec(Reg(Bits(32 bits)) init (0), bufferWords)

  val replayValid = Reg(Bool()) init (False)
  val replayAddr = Reg(UInt(addrWidth bits)) init (0)
  val replayData = Reg(Bits(32 bits)) init (0)

  val fillActive = Reg(Bool()) init (False)
  val fillCmdIssued = Reg(Bool()) init (False)
  val fillBase = Reg(UInt(addrWidth bits)) init (0)
  val fillRspIndex = Reg(UInt(beatIndexWidth bits)) init (0)
  val queuedFillValid = Reg(Bool()) init (False)
  val queuedFillBase = Reg(UInt(addrWidth bits)) init (0)

  val readRspValid = Reg(Bool()) init (False)
  val readRspData = Reg(Bits(32 bits)) init (0)

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

  def startFill(base: UInt): Unit = {
    fillActive := True
    fillCmdIssued := False
    fillBase := base
    fillRspIndex := 0
    fillBurstCount := fillBurstCount + 1
    fillBurstBeats := fillBurstBeats + U(prefetchWords, 32 bits)
    if (prefetchWords == 1) {
      singleBeatBurstCount := singleBeatBurstCount + 1
    } else {
      multiBeatBurstCount := multiBeatBurstCount + 1
    }
  }

  val freeVec = Vec(Bool(), bufferWords)
  val hitVec = Vec(Bool(), bufferWords)
  val prefetchWord = wordBase(io.prefetchReq.address)
  val readWord = wordBase(io.readReq.address)
  for (idx <- 0 until bufferWords) {
    freeVec(idx) := !bufferValid(idx)
    hitVec(idx) := bufferValid(idx) && bufferAddr(idx) === readWord
  }
  val bufferHit = hitVec.asBits.orR
  val bufferHitIndex = OHToUInt(hitVec.asBits)
  val bufferOccupancy = bufferValid.map(_.asUInt.resize(countWidth bits)).reduce(_ + _)
  val freeCount = freeVec.map(_.asUInt.resize(countWidth bits)).reduce(_ + _)
  val firstFreeOH = OHMasking.first(freeVec.asBits)
  val firstFreeIndex = OHToUInt(firstFreeOH)

  val outstandingFillWords = UInt(countWidth bits)
  outstandingFillWords := 0
  when(fillActive) {
    outstandingFillWords := U(prefetchWords, countWidth bits) - fillRspIndex.resize(countWidth bits)
  }
  val reservedWords =
    outstandingFillWords + (queuedFillValid ? U(prefetchWords, countWidth bits) | U(
      0,
      countWidth bits
    ))
  val canReserveBurst = freeCount >= (U(prefetchWords, countWidth bits) + reservedWords)

  val prefetchCovered = replayValid && replayAddr === prefetchWord ||
    (0 until bufferWords)
      .map(idx => bufferValid(idx) && bufferAddr(idx) === prefetchWord)
      .reduce(_ || _) ||
    (fillActive && inWindow(prefetchWord, fillBase, prefetchWords)) ||
    (queuedFillValid && inWindow(prefetchWord, queuedFillBase, prefetchWords))

  when(io.prefetchReq.valid) {
    reqCount := reqCount + 1
    when(prevPrefetchValid) {
      when(wordBase(io.prefetchReq.address) === wordBase(prevPrefetchAddr)) {
        reqSameWordCount := reqSameWordCount + 1
      }.elsewhen(io.prefetchReq.address === (prevPrefetchAddr + U(2, addrWidth bits)).resized) {
        reqForwardStepCount := reqForwardStepCount + 1
      }.elsewhen(prevPrefetchAddr === (io.prefetchReq.address + U(2, addrWidth bits)).resized) {
        reqBackwardStepCount := reqBackwardStepCount + 1
      }.elsewhen(lineBaseOf(io.prefetchReq.address) === lineBaseOf(prevPrefetchAddr)) {
        reqSameLineCount := reqSameLineCount + 1
      }.otherwise {
        reqOtherCount := reqOtherCount + 1
      }
    }
    prevPrefetchValid := True
    prevPrefetchAddr := io.prefetchReq.address

    when(!prefetchCovered && canReserveBurst) {
      when(!fillActive) {
        startFill(prefetchWord)
      }.elsewhen(!queuedFillValid) {
        queuedFillValid := True
        queuedFillBase := prefetchWord
      }
    }
  }

  when(bufferOccupancy.resize(8 bits) > maxOccupancy) {
    maxOccupancy := bufferOccupancy.resize(8 bits)
  }

  val replayHit = replayValid && replayAddr === readWord
  io.readReq.ready := !readRspValid && (replayHit || bufferHit)
  when(io.readReq.valid && !io.readReq.ready) {
    fillStallCycles := fillStallCycles + 1
  }
  when(io.readReq.fire) {
    fillHits := fillHits + 1
    readRspValid := True
    when(replayHit) {
      readRspData := replayData
    }.otherwise {
      readRspData := bufferData(bufferHitIndex)
      replayValid := True
      replayAddr := bufferAddr(bufferHitIndex)
      replayData := bufferData(bufferHitIndex)
      bufferValid(bufferHitIndex) := False
    }
  }

  io.readRsp.valid := readRspValid
  io.readRsp.data := readRspData
  when(io.readRsp.fire) {
    readRspValid := False
  }

  io.mem.cmd.valid := fillActive && !fillCmdIssued
  io.mem.cmd.fragment.address := fillBase
  io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
  io.mem.cmd.fragment.length := U(prefetchWords * 4 - 1, c.memBurstLengthWidth bits)
  io.mem.cmd.fragment.source := 0
  io.mem.cmd.fragment.data := 0
  io.mem.cmd.fragment.mask := 0
  io.mem.cmd.last := True
  when(io.mem.cmd.fire) {
    fillCmdIssued := True
  }

  io.mem.rsp.ready := fillActive && fillCmdIssued && freeVec.asBits.orR && io.mem.rsp.source === 0
  when(io.mem.rsp.fire && io.mem.rsp.source === 0) {
    bufferValid(firstFreeIndex) := True
    bufferAddr(firstFreeIndex) := (fillBase + (fillRspIndex.resize(addrWidth bits) << 2)).resized
    bufferData(firstFreeIndex) := io.mem.rsp.fragment.data
    when(io.mem.rsp.last || fillRspIndex === U(prefetchWords - 1, beatIndexWidth bits)) {
      fillActive := False
      fillCmdIssued := False
      fillRspIndex := 0
      when(queuedFillValid) {
        startFill(queuedFillBase)
        queuedFillValid := False
      }
    }.otherwise {
      fillRspIndex := fillRspIndex + 1
    }
  }

  GenerationFlags.formal {
    val formalReset = ClockDomain.current.isResetActive
    when(!formalReset) {
      when(fillActive && !fillCmdIssued) {
        assert(io.mem.cmd.valid)
        assert(io.mem.cmd.fragment.address === fillBase)
      }
    }
  }

  io.busy := fillActive || queuedFillValid || readRspValid || bufferOccupancy =/= 0
}

object FramebufferPlaneReader {
  def bmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}
