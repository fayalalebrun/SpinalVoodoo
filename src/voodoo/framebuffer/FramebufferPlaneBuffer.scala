package voodoo.framebuffer

import voodoo._
import spinal.core._
import spinal.core.formal._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

case class FramebufferPlaneBuffer(c: Config, formalStrong: Boolean = true) extends Component {
  import FramebufferPlaneBuffer._

  val io = new Bundle {
    val readReq = slave Stream (ReadReq(c))
    val readRsp = master Stream (ReadRsp())
    val writeReq = slave Stream (WriteReq(c))
    val flush = in Bool ()
    val mem = master(Bmb(FramebufferPlaneBuffer.bmbParams(c)))
    val busy = out Bool ()
    val fillHits = out UInt (32 bits)
    val fillMisses = out UInt (32 bits)
    val fillBurstCount = out UInt (32 bits)
    val fillBurstBeats = out UInt (32 bits)
    val fillStallCycles = out UInt (32 bits)
    val writeStallCycles = out UInt (32 bits)
    val writeDrainCount = out UInt (32 bits)
    val writeFullDrainCount = out UInt (32 bits)
    val writePartialDrainCount = out UInt (32 bits)
    val writeDrainReasonFullCount = out UInt (32 bits)
    val writeDrainReasonRotateCount = out UInt (32 bits)
    val writeDrainReasonFlushCount = out UInt (32 bits)
    val writeDrainDirtyWordTotal = out UInt (32 bits)
    val writeRotateBlockedCycles = out UInt (32 bits)
    val writeSingleWordDrainCount = out UInt (32 bits)
    val writeSingleWordDrainStartAtZeroCount = out UInt (32 bits)
    val writeSingleWordDrainStartAtLastCount = out UInt (32 bits)
    val writeRotateAdjacentLineCount = out UInt (32 bits)
    val writeRotateSameLineGapCount = out UInt (32 bits)
    val writeRotateOtherLineCount = out UInt (32 bits)
  }

  require(c.fbWriteBufferLineWords >= 2)
  require(c.fbWriteBufferCount == 2)

  val runDepth = c.fbWriteBufferLineWords
  val fifoDepth = runDepth - 1
  val countWidth = log2Up(runDepth + 1)
  val slotCount = c.fbWriteBufferCount
  val slotIndexWidth = log2Up(slotCount)
  val addrWidth = c.addressWidth.value
  val physicalIndexWidth = log2Up(runDepth)
  val lastPhysicalWordIndex = U(runDepth - 1, physicalIndexWidth bits)

  def slotPush(slot: UInt) = Mux(slot === 0, slot0Fifo.io.push, slot1Fifo.io.push)
  def slotPop(slot: UInt) = Mux(slot === 0, slot0Fifo.io.pop, slot1Fifo.io.pop)
  def slotPushReady(slot: UInt): Bool =
    Mux(slot === 0, slot0Fifo.io.push.ready, slot1Fifo.io.push.ready)
  def slotPopValid(slot: UInt): Bool =
    Mux(slot === 0, slot0Fifo.io.pop.valid, slot1Fifo.io.pop.valid)
  def slotPopData(slot: UInt): Bits =
    Mux(slot === 0, slot0Fifo.io.pop.payload.data, slot1Fifo.io.pop.payload.data)
  def slotPopMask(slot: UInt): Bits =
    Mux(slot === 0, slot0Fifo.io.pop.payload.mask, slot1Fifo.io.pop.payload.mask)

  def driveSlotPush(slot: UInt, address: UInt, data: Bits, mask: Bits): Unit = {
    when(slot === 0) {
      slot0Push.valid := True
      slot0Push.payload.address := address
      slot0Push.payload.data := data
      slot0Push.payload.mask := mask
    }.otherwise {
      slot1Push.valid := True
      slot1Push.payload.address := address
      slot1Push.payload.data := data
      slot1Push.payload.mask := mask
    }
  }

  def driveSlotPopReady(slot: UInt): Unit = {
    when(slot === 0) {
      slot0Pop.ready := True
    }.otherwise {
      slot1Pop.ready := True
    }
  }

  def addrPlusWords(address: UInt, words: UInt): UInt = {
    (address + ((words.resize(addrWidth bits) << 2).resized)).resized
  }

  def physicalWordIndex(address: UInt): UInt = (address >> 2).resize(physicalIndexWidth bits)

  def mergeWord(oldData: Bits, oldMask: Bits, newData: Bits, newMask: Bits): (Bits, Bits) = {
    val mergedData = Bits(32 bits)
    val mergedMask = Bits(4 bits)
    mergedData := oldData
    mergedMask := oldMask | newMask
    for (lane <- 0 until 4) {
      when(newMask(lane)) {
        mergedData(8 * lane + 7 downto 8 * lane) := newData(8 * lane + 7 downto 8 * lane)
      }
    }
    (mergedData, mergedMask)
  }

  val slot0Fifo = StreamFifo(WriteReq(c), fifoDepth)
  val slot1Fifo = StreamFifo(WriteReq(c), fifoDepth)
  slot0Fifo.setName("slot0Fifo")
  slot1Fifo.setName("slot1Fifo")
  val slotFlush = Vec(Bool(), slotCount)
  slotFlush.foreach(_ := False)
  slot0Fifo.io.flush := slotFlush(0)
  slot1Fifo.io.flush := slotFlush(1)

  val slotValid = Vec(Reg(Bool()) init (False), slotCount)
  val slotDraining = Vec(Reg(Bool()) init (False), slotCount)
  val slotStartAddr = Vec(Reg(UInt(addrWidth bits)) init (0), slotCount)
  val slotWordCount = Vec(Reg(UInt(countWidth bits)) init (0), slotCount)
  val slotPendingAddress = Vec(Reg(UInt(addrWidth bits)) init (0), slotCount)
  val slotPendingData = Vec(Reg(Bits(32 bits)) init (0), slotCount)
  val slotPendingMask = Vec(Reg(Bits(4 bits)) init (0), slotCount)
  val activeSlot = Reg(UInt(slotIndexWidth bits)) init (0)

  val writeStallCycles = Reg(UInt(32 bits)) init (0)
  val writeDrainCount = Reg(UInt(32 bits)) init (0)
  val writeFullDrainCount = Reg(UInt(32 bits)) init (0)
  val writePartialDrainCount = Reg(UInt(32 bits)) init (0)
  val writeDrainReasonFullCount = Reg(UInt(32 bits)) init (0)
  val writeDrainReasonRotateCount = Reg(UInt(32 bits)) init (0)
  val writeDrainReasonFlushCount = Reg(UInt(32 bits)) init (0)
  val writeDrainDirtyWordTotal = Reg(UInt(32 bits)) init (0)
  val writeRotateBlockedCycles = Reg(UInt(32 bits)) init (0)
  val writeSingleWordDrainCount = Reg(UInt(32 bits)) init (0)
  val writeSingleWordDrainStartAtZeroCount = Reg(UInt(32 bits)) init (0)
  val writeSingleWordDrainStartAtLastCount = Reg(UInt(32 bits)) init (0)
  val writeRotateAdjacentLineCount = Reg(UInt(32 bits)) init (0)
  val writeRotateSameLineGapCount = Reg(UInt(32 bits)) init (0)
  val writeRotateOtherLineCount = Reg(UInt(32 bits)) init (0)

  if (c.trace.enabled) {
    writeStallCycles.simPublic()
    writeDrainCount.simPublic()
    writeFullDrainCount.simPublic()
    writePartialDrainCount.simPublic()
    writeDrainReasonFullCount.simPublic()
    writeDrainReasonRotateCount.simPublic()
    writeDrainReasonFlushCount.simPublic()
    writeDrainDirtyWordTotal.simPublic()
    writeRotateBlockedCycles.simPublic()
    writeSingleWordDrainCount.simPublic()
    writeSingleWordDrainStartAtZeroCount.simPublic()
    writeSingleWordDrainStartAtLastCount.simPublic()
    writeRotateAdjacentLineCount.simPublic()
    writeRotateSameLineGapCount.simPublic()
    writeRotateOtherLineCount.simPublic()
    slotDraining.foreach(_.simPublic())
  }

  io.fillHits := 0
  io.fillMisses := 0
  io.fillBurstCount := 0
  io.fillBurstBeats := 0
  io.fillStallCycles := 0
  io.writeStallCycles := writeStallCycles
  io.writeDrainCount := writeDrainCount
  io.writeFullDrainCount := writeFullDrainCount
  io.writePartialDrainCount := writePartialDrainCount
  io.writeDrainReasonFullCount := writeDrainReasonFullCount
  io.writeDrainReasonRotateCount := writeDrainReasonRotateCount
  io.writeDrainReasonFlushCount := writeDrainReasonFlushCount
  io.writeDrainDirtyWordTotal := writeDrainDirtyWordTotal
  io.writeRotateBlockedCycles := writeRotateBlockedCycles
  io.writeSingleWordDrainCount := writeSingleWordDrainCount
  io.writeSingleWordDrainStartAtZeroCount := writeSingleWordDrainStartAtZeroCount
  io.writeSingleWordDrainStartAtLastCount := writeSingleWordDrainStartAtLastCount
  io.writeRotateAdjacentLineCount := writeRotateAdjacentLineCount
  io.writeRotateSameLineGapCount := writeRotateSameLineGapCount
  io.writeRotateOtherLineCount := writeRotateOtherLineCount

  object DirectState extends SpinalEnum {
    val Idle, Cmd, Rsp = newElement()
  }
  val directState = RegInit(DirectState.Idle)
  val directWriteAddress = Reg(UInt(addrWidth bits)) init (0)
  val directWriteData = Reg(Bits(32 bits)) init (0)
  val directWriteMask = Reg(Bits(4 bits)) init (0)

  object DrainState extends SpinalEnum {
    val Idle, Send, Rsp = newElement()
  }
  val drainState = RegInit(DrainState.Idle)
  val drainSlot = Reg(UInt(slotIndexWidth bits)) init (0)
  val drainIndex = Reg(UInt(physicalIndexWidth bits)) init (0)
  val drainLastIndex = Reg(UInt(physicalIndexWidth bits)) init (0)
  val drainLength = Reg(UInt(c.memBurstLengthWidth bits)) init (3)

  val writeReqPipe = io.writeReq.m2sPipe()

  def clearSlotMeta(slot: UInt): Unit = {
    slotFlush(slot) := True
    slotValid(slot) := False
    slotDraining(slot) := False
    slotStartAddr(slot) := U(0, addrWidth bits)
    slotWordCount(slot) := U(0, countWidth bits)
    slotPendingAddress(slot) := U(0, addrWidth bits)
    slotPendingData(slot) := B(0, 32 bits)
    slotPendingMask(slot) := B(0, 4 bits)
  }

  def initSlotMeta(slot: UInt, address: UInt, data: Bits, mask: Bits): Unit = {
    clearSlotMeta(slot)
    slotValid(slot) := True
    slotStartAddr(slot) := address
    slotWordCount(slot) := U(1, countWidth bits)
    slotPendingAddress(slot) := address
    slotPendingData(slot) := data
    slotPendingMask(slot) := mask
  }

  def recordDrain(startAddr: UInt, wordCount: UInt, reason: UInt): Unit = {
    writeDrainCount := writeDrainCount + 1
    writeDrainDirtyWordTotal := writeDrainDirtyWordTotal + wordCount.resized
    when(wordCount === U(runDepth, countWidth bits)) {
      writeFullDrainCount := writeFullDrainCount + 1
    }.otherwise {
      writePartialDrainCount := writePartialDrainCount + 1
    }
    when(wordCount === 1) {
      writeSingleWordDrainCount := writeSingleWordDrainCount + 1
      when(physicalWordIndex(startAddr) === 0) {
        writeSingleWordDrainStartAtZeroCount := writeSingleWordDrainStartAtZeroCount + 1
      }
      when(physicalWordIndex(startAddr) === lastPhysicalWordIndex) {
        writeSingleWordDrainStartAtLastCount := writeSingleWordDrainStartAtLastCount + 1
      }
    }
    switch(reason) {
      is(U(0, 2 bits)) { writeDrainReasonFullCount := writeDrainReasonFullCount + 1 }
      is(U(1, 2 bits)) { writeDrainReasonRotateCount := writeDrainReasonRotateCount + 1 }
      is(U(2, 2 bits)) { writeDrainReasonFlushCount := writeDrainReasonFlushCount + 1 }
    }
  }

  def startDrain(slot: UInt, reason: UInt): Unit = {
    val wordCount = slotWordCount(slot)
    recordDrain(slotStartAddr(slot), wordCount, reason)
    slotDraining(slot) := True
    drainSlot := slot
    drainIndex := 0
    drainLastIndex := (wordCount - 1).resize(physicalIndexWidth bits)
    drainLength := (((wordCount.resize(c.memBurstLengthWidth bits)) << 2) - 1).resized
    drainState := DrainState.Send
  }

  val slot0Push = slot0Fifo.io.push
  val slot1Push = slot1Fifo.io.push
  slot0Push.valid := False
  slot0Push.payload.assignDontCare()
  slot1Push.valid := False
  slot1Push.payload.assignDontCare()

  val slot0Pop = slot0Fifo.io.pop
  val slot1Pop = slot1Fifo.io.pop
  slot0Pop.ready := False
  slot1Pop.ready := False

  io.readReq.ready := False
  io.readRsp.valid := False
  io.readRsp.data := 0

  io.mem.cmd.valid := False
  io.mem.cmd.fragment.address := 0
  io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.WRITE
  io.mem.cmd.fragment.length := 3
  io.mem.cmd.fragment.source := 0
  io.mem.cmd.fragment.data := 0
  io.mem.cmd.fragment.mask := 0
  io.mem.cmd.last := True
  io.mem.rsp.ready := False

  directState := directState
  writeReqPipe.ready := False

  val otherSlot = activeSlot ^ U(1, slotIndexWidth bits)
  val activeValid = slotValid(activeSlot)
  val activeCount = slotWordCount(activeSlot)
  val activeStartAddr = slotStartAddr(activeSlot)
  val activePendingAddr = slotPendingAddress(activeSlot)
  val activePendingData = slotPendingData(activeSlot)
  val activePendingMask = slotPendingMask(activeSlot)
  val activeNextAddr = addrPlusWords(activeStartAddr, activeCount.resized)
  val sameWordMerge = activeValid && writeReqPipe.payload.address === activePendingAddr
  val sequentialAppend = activeValid && activeCount =/= U(
    runDepth,
    countWidth bits
  ) && writeReqPipe.payload.address === activeNextAddr
  val needsRotate = activeValid && !(sameWordMerge || sequentialAppend)
  val canRotate = drainState === DrainState.Idle && !slotValid(otherSlot)

  val activePushReady = slotPushReady(activeSlot)
  val activeAppendReady = !activeValid || sameWordMerge || (sequentialAppend && activePushReady)

  when(writeReqPipe.valid && !writeReqPipe.ready) {
    writeStallCycles := writeStallCycles + 1
  }
  when(writeReqPipe.valid && needsRotate && !canRotate) {
    writeRotateBlockedCycles := writeRotateBlockedCycles + 1
  }

  if (c.useFbWriteBuffer) {
    when(drainState === DrainState.Send) {
      val drainPending = drainIndex === drainLastIndex
      val drainAddress = addrPlusWords(slotStartAddr(drainSlot), drainIndex.resized)
      val drainData = Bits(32 bits)
      val drainMask = Bits(4 bits)

      when(drainPending) {
        drainData := slotPendingData(drainSlot)
        drainMask := slotPendingMask(drainSlot)
      }.otherwise {
        drainData := slotPopData(drainSlot)
        drainMask := slotPopMask(drainSlot)
      }

      io.mem.cmd.valid := drainPending || slotPopValid(drainSlot)
      io.mem.cmd.fragment.address := drainAddress
      io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.WRITE
      io.mem.cmd.fragment.length := drainLength
      io.mem.cmd.fragment.source := 1
      io.mem.cmd.fragment.data := drainData
      io.mem.cmd.fragment.mask := drainMask
      io.mem.cmd.last := drainPending

      when(io.mem.cmd.fire) {
        when(!drainPending) {
          driveSlotPopReady(drainSlot)
        }
        when(drainPending) {
          drainState := DrainState.Rsp
        }.otherwise {
          drainIndex := drainIndex + 1
        }
      }
    }

    when(drainState === DrainState.Rsp) {
      io.mem.rsp.ready := io.mem.rsp.source === 1
      when(io.mem.rsp.fire && io.mem.rsp.source === 1) {
        clearSlotMeta(drainSlot)
        drainState := DrainState.Idle
      }
    }

    when(
      drainState === DrainState.Idle && activeValid && activeCount === U(
        runDepth,
        countWidth bits
      ) && !slotValid(otherSlot) && !writeReqPipe.valid
    ) {
      clearSlotMeta(otherSlot)
      startDrain(activeSlot, U(0, 2 bits))
      activeSlot := otherSlot
    }

    when(
      drainState === DrainState.Idle && io.flush && activeValid && !slotValid(
        otherSlot
      ) && !writeReqPipe.valid
    ) {
      clearSlotMeta(otherSlot)
      startDrain(activeSlot, U(2, 2 bits))
      activeSlot := otherSlot
    }

    when(directState === DirectState.Idle && writeReqPipe.valid) {
      when(!activeValid) {
        writeReqPipe.ready := True
      }.elsewhen(sameWordMerge) {
        writeReqPipe.ready := True
      }.elsewhen(sequentialAppend && activePushReady) {
        writeReqPipe.ready := True
      }.elsewhen(needsRotate && canRotate) {
        writeReqPipe.ready := True
      }

      when(writeReqPipe.fire) {
        when(!activeValid) {
          initSlotMeta(
            activeSlot,
            writeReqPipe.payload.address,
            writeReqPipe.payload.data,
            writeReqPipe.payload.mask
          )
        }.elsewhen(sameWordMerge) {
          val merged = mergeWord(
            activePendingData,
            activePendingMask,
            writeReqPipe.payload.data,
            writeReqPipe.payload.mask
          )
          slotPendingData(activeSlot) := merged._1
          slotPendingMask(activeSlot) := merged._2
        }.elsewhen(sequentialAppend) {
          driveSlotPush(activeSlot, activePendingAddr, activePendingData, activePendingMask)

          val newCount = activeCount + 1
          slotWordCount(activeSlot) := newCount
          slotPendingAddress(activeSlot) := writeReqPipe.payload.address
          slotPendingData(activeSlot) := writeReqPipe.payload.data
          slotPendingMask(activeSlot) := writeReqPipe.payload.mask

          when(
            newCount === U(
              runDepth,
              countWidth bits
            ) && drainState === DrainState.Idle && !slotValid(otherSlot)
          ) {
            clearSlotMeta(otherSlot)
            startDrain(activeSlot, U(0, 2 bits))
            activeSlot := otherSlot
          }
        }.otherwise {
          val adjacentPhysicalLine = physicalWordIndex(
            activePendingAddr
          ) === lastPhysicalWordIndex && physicalWordIndex(writeReqPipe.payload.address) === 0
          when(adjacentPhysicalLine) {
            writeRotateAdjacentLineCount := writeRotateAdjacentLineCount + 1
          }.elsewhen(
            writeReqPipe.payload.address > activePendingAddr && writeReqPipe.payload.address < activeNextAddr
          ) {
            writeRotateSameLineGapCount := writeRotateSameLineGapCount + 1
          }.otherwise {
            writeRotateOtherLineCount := writeRotateOtherLineCount + 1
          }

          clearSlotMeta(otherSlot)
          startDrain(activeSlot, U(1, 2 bits))
          initSlotMeta(
            otherSlot,
            writeReqPipe.payload.address,
            writeReqPipe.payload.data,
            writeReqPipe.payload.mask
          )
          activeSlot := otherSlot
        }
      }
    }

    GenerationFlags.formal {
      val formalReset = ClockDomain.current.isResetActive
      when(
        !formalReset && directState === DirectState.Idle && writeReqPipe.valid && (!activeValid || sameWordMerge || (sequentialAppend && activePushReady) || (needsRotate && canRotate))
      ) {
        assert(writeReqPipe.ready)
      }
    }
  } else {
    io.mem.cmd.valid.allowOverride()
    io.mem.cmd.fragment.address.allowOverride()
    io.mem.cmd.fragment.opcode.allowOverride()
    io.mem.cmd.fragment.length.allowOverride()
    io.mem.cmd.fragment.source.allowOverride()
    io.mem.cmd.fragment.data.allowOverride()
    io.mem.cmd.fragment.mask.allowOverride()
    io.mem.cmd.last.allowOverride()
    writeReqPipe.ready.allowOverride()
    io.mem.rsp.ready.allowOverride()

    io.mem.cmd.valid := writeReqPipe.valid
    io.mem.cmd.fragment.address := writeReqPipe.payload.address
    io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.WRITE
    io.mem.cmd.fragment.length := 3
    io.mem.cmd.fragment.source := 0
    io.mem.cmd.fragment.data := writeReqPipe.payload.data
    io.mem.cmd.fragment.mask := writeReqPipe.payload.mask
    io.mem.cmd.last := True
    writeReqPipe.ready := io.mem.cmd.ready
    io.mem.rsp.ready := True
  }

  GenerationFlags.formal {
    val formalReset = ClockDomain.current.isResetActive
    if (formalStrong) {
      when(!formalReset) {
        for (slot <- 0 until slotCount) {
          when(!slotValid(slot)) {
            assert(!slotDraining(slot))
            assert(slotWordCount(slot) === U(0, countWidth bits))
            if (slot == 0) {
              assert(slot0Fifo.io.occupancy === 0)
            } else {
              assert(slot1Fifo.io.occupancy === 0)
            }
          }
          when(slotValid(slot)) {
            assert(slotWordCount(slot) =/= U(0, countWidth bits))
            assert(slotWordCount(slot) <= U(runDepth, countWidth bits))
          }
        }
        when(drainState =/= DrainState.Idle) {
          assert(slotValid(drainSlot))
          assert(slotDraining(drainSlot))
        }
      }
    }
  }

  io.busy := directState =/= DirectState.Idle || drainState =/= DrainState.Idle || writeReqPipe.valid || (io.flush && activeValid)
}

object FramebufferPlaneBuffer {
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

  case class ReadReq(c: Config) extends Bundle {
    val address = UInt(c.addressWidth)
  }

  case class ReadRsp() extends Bundle {
    val data = Bits(32 bits)
  }

  case class WriteReq(c: Config) extends Bundle {
    val address = UInt(c.addressWidth)
    val data = Bits(32 bits)
    val mask = Bits(4 bits)
  }

}
