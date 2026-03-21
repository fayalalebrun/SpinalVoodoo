package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

case class FramebufferPlaneCache(c: Config, formalStrong: Boolean = true) extends Component {
  import FramebufferPlaneCache._

  val io = new Bundle {
    val readReq = slave Stream (ReadReq(c))
    val readRsp = master Stream (ReadRsp())
    val writeReq = slave Stream (WriteReq(c))
    val flush = in Bool ()
    val mem = master(Bmb(bmbParams(c)))
    val busy = out Bool ()
    val fillHits = out UInt (32 bits)
    val fillMisses = out UInt (32 bits)
    val fillBurstCount = out UInt (32 bits)
    val fillBurstBeats = out UInt (32 bits)
    val fillStallCycles = out UInt (32 bits)
  }

  require(c.fbFillLineWords > 0 && ((c.fbFillLineWords & (c.fbFillLineWords - 1)) == 0))
  require(c.fbFillLineWords >= 1)

  val lineWords = c.fbFillLineWords
  val lineShift = log2Up(lineWords)
  val slotCount = 2
  val slotIndexWidth = log2Up(slotCount)
  val lineWordAddrWidth = c.addressWidth.value - 2
  val lineBaseWidth = c.addressWidth.value
  val lineBytes = lineWords * 4
  val lineLength = U(lineBytes - 1, c.memBurstLengthWidth bits)

  def lineBaseOf(address: UInt): UInt = {
    val shifted = (address >> (lineShift + 2)).resize(lineWordAddrWidth bits)
    ((shifted << (lineShift + 2)).resize(lineBaseWidth bits))
  }

  def wordIndexOf(address: UInt): UInt = ((address >> 2).resize(lineShift bits))

  def memIndex(slot: UInt, word: UInt): UInt = {
    ((slot.resize(slotIndexWidth bits) ## U(0, lineShift bits)) | word
      .resize(slotIndexWidth + lineShift bits)
      .asBits).asUInt
  }

  val lineData = Seq.fill(4)(Mem(Bits(8 bits), slotCount * lineWords))
  val lineReadPort = lineData.map(_.readSyncPort(readUnderWrite = readFirst))
  val lineReadRsp =
    lineReadPort(3).rsp ## lineReadPort(2).rsp ## lineReadPort(1).rsp ## lineReadPort(0).rsp

  val slotValid = Vec(Reg(Bool()) init (False), slotCount)
  val slotDirty = Vec(Reg(Bool()) init (False), slotCount)
  val slotBase = Vec(Reg(UInt(c.addressWidth.value bits)) init (0), slotCount)
  val slotKnownMask = Vec(
    (0 until slotCount).map(_ => Vec((0 until lineWords).map(_ => Reg(Bits(4 bits)) init (0))))
  )
  val slotDirtyMask = Vec(
    (0 until slotCount).map(_ => Vec((0 until lineWords).map(_ => Reg(Bits(4 bits)) init (0))))
  )
  val nextVictim = Reg(UInt(slotIndexWidth bits)) init (0)

  val fillHits = Reg(UInt(32 bits)) init (0)
  val fillMisses = Reg(UInt(32 bits)) init (0)
  val fillBurstCount = Reg(UInt(32 bits)) init (0)
  val fillBurstBeats = Reg(UInt(32 bits)) init (0)
  val fillStallCycles = Reg(UInt(32 bits)) init (0)
  if (c.trace.enabled) {
    fillHits.simPublic()
    fillMisses.simPublic()
    fillBurstCount.simPublic()
    fillBurstBeats.simPublic()
    fillStallCycles.simPublic()
  }
  io.fillHits := fillHits
  io.fillMisses := fillMisses
  io.fillBurstCount := fillBurstCount
  io.fillBurstBeats := fillBurstBeats
  io.fillStallCycles := fillStallCycles

  object OpState extends SpinalEnum {
    val Idle, ReadWait, EvictRsp, EvictRead, EvictSend, FillCmd, FillWait, FillRead, DirectReadCmd,
        DirectReadRsp, DirectWriteCmd, DirectWriteRsp = newElement()
  }
  val state = RegInit(OpState.Idle)

  val pendingReadReq = Reg(ReadReq(c))
  val pendingWriteReq = Reg(WriteReq(c))
  val pendingSlot = Reg(UInt(slotIndexWidth bits)) init (0)
  val pendingIsRead = Reg(Bool()) init (False)
  val pendingFlushOnly = Reg(Bool()) init (False)
  val evictBeat = Reg(UInt(lineShift bits)) init (0)
  val evictData = Reg(Bits(32 bits)) init (0)
  val evictDataValid = Reg(Bool()) init (False)
  val fillRspCount = Reg(UInt(log2Up(lineWords + 1) bits)) init (0)
  val directRspData = Reg(Bits(32 bits)) init (0)
  val readRspValid = Reg(Bool()) init (False)
  val readRspData = Reg(Bits(32 bits)) init (0)
  val lineReadRspValid = Reg(Bool()) init (False)
  val pendingReadWasHit = Reg(Bool()) init (False)
  val writeReqPipe = io.writeReq.m2sPipe()
  val fillStoreCmd = Stream(FillStore(c, lineShift, slotIndexWidth))
  val fillStorePipe = fillStoreCmd.m2sPipe()

  val readReqLineBase = lineBaseOf(io.readReq.address)
  val readReqWordIndex = wordIndexOf(io.readReq.address)
  val writeReqLineBase = lineBaseOf(writeReqPipe.payload.address)
  val writeReqWordIndex = wordIndexOf(writeReqPipe.payload.address)

  val readHitVec = Bits(slotCount bits)
  val readFullHitVec = Bits(slotCount bits)
  val writeHitVec = Bits(slotCount bits)
  for (i <- 0 until slotCount) {
    val readTagHit = slotValid(i) && slotBase(i) === readReqLineBase
    readHitVec(i) := io.readReq.valid && readTagHit
    readFullHitVec(i) := io.readReq.valid && readTagHit && slotKnownMask(i)(
      readReqWordIndex
    ) === B"1111"
    writeHitVec(i) := writeReqPipe.valid && slotValid(i) && slotBase(i) === writeReqLineBase
  }
  val readHit = readFullHitVec.orR
  val writeHit = writeHitVec.orR
  val readHitSlot = OHToUInt(readFullHitVec)
  val writeHitSlot = OHToUInt(writeHitVec)

  val victimSlot = UInt(slotIndexWidth bits)
  victimSlot := nextVictim
  when(!slotValid(0)) {
    victimSlot := 0
  }.elsewhen(!slotValid(1)) {
    victimSlot := 1
  }

  val readMissSlot = UInt(slotIndexWidth bits)
  readMissSlot := victimSlot
  when(readHitVec.orR) {
    readMissSlot := OHToUInt(readHitVec)
  }

  val dirtySlotAvailable = slotDirty(0) || slotDirty(1)
  val dirtySlot = UInt(slotIndexWidth bits)
  dirtySlot := 0
  when(!slotDirty(0) && slotDirty(1)) {
    dirtySlot := 1
  }

  val lineWriteValid = Bool()
  val lineWriteAddress = UInt((slotIndexWidth + lineShift) bits)
  val lineWriteData = Bits(32 bits)
  val lineWriteMask = Bits(4 bits)
  lineWriteValid := False
  lineWriteAddress := 0
  lineWriteData := 0
  lineWriteMask := 0
  for (lane <- 0 until 4) {
    lineData(lane).write(
      address = lineWriteAddress,
      data = lineWriteData(8 * lane + 7 downto 8 * lane),
      enable = lineWriteValid && lineWriteMask(lane)
    )
  }

  val readPortCmdValid = Bool()
  val readPortCmdAddress = UInt((slotIndexWidth + lineShift) bits)
  readPortCmdValid := False
  readPortCmdAddress := 0
  for (lane <- 0 until 4) {
    lineReadPort(lane).cmd.valid := readPortCmdValid
    lineReadPort(lane).cmd.payload := readPortCmdAddress
  }

  io.readRsp.valid := readRspValid
  io.readRsp.data := readRspData
  when(io.readRsp.fire) {
    readRspValid := False
  }

  io.mem.cmd.valid := False
  io.mem.cmd.fragment.address := 0
  io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
  io.mem.cmd.fragment.length := 3
  io.mem.cmd.fragment.source := 0
  io.mem.cmd.fragment.data := 0
  io.mem.cmd.fragment.mask := 0
  io.mem.cmd.last := True
  io.mem.rsp.ready := False

  io.readReq.ready := False
  writeReqPipe.ready := False

  fillStoreCmd.valid := False
  fillStoreCmd.payload.slot := pendingSlot
  fillStoreCmd.payload.beat := 0
  fillStoreCmd.payload.data := 0
  fillStoreCmd.payload.mask := 0
  fillStoreCmd.payload.last := False
  fillStorePipe.ready := True

  def clearSlot(slot: UInt): Unit = {
    slotValid(slot) := False
    slotDirty(slot) := False
    for (w <- 0 until lineWords) {
      slotKnownMask(slot)(w) := B(0, 4 bits)
      slotDirtyMask(slot)(w) := B(0, 4 bits)
    }
  }

  def allocateLine(slot: UInt, base: UInt): Unit = {
    slotValid(slot) := True
    slotDirty(slot) := False
    slotBase(slot) := base
    for (w <- 0 until lineWords) {
      slotKnownMask(slot)(w) := B(0, 4 bits)
      slotDirtyMask(slot)(w) := B(0, 4 bits)
    }
  }

  lineReadRspValid := readPortCmdValid

  if (c.useFbFillCache) {
    when(lineReadRspValid && state === OpState.ReadWait) {
      readRspData := lineReadRsp
      readRspValid := True
      when(pendingReadWasHit) {
        fillHits := fillHits + 1
      }
      state := OpState.Idle
    }

    when(state === OpState.Idle) {
      when(writeReqPipe.valid) {
        when(writeHit) {
          writeReqPipe.ready := True
          pendingFlushOnly := False
          val slot = writeHitSlot
          lineWriteValid := True
          lineWriteAddress := memIndex(slot, writeReqWordIndex)
          lineWriteData := writeReqPipe.payload.data
          lineWriteMask := writeReqPipe.payload.mask
          slotKnownMask(slot)(writeReqWordIndex) := slotKnownMask(slot)(
            writeReqWordIndex
          ) | writeReqPipe.payload.mask
          slotDirtyMask(slot)(writeReqWordIndex) := slotDirtyMask(slot)(
            writeReqWordIndex
          ) | writeReqPipe.payload.mask
          slotDirty(slot) := True
        }.otherwise {
          writeReqPipe.ready := True
          pendingIsRead := False
          pendingFlushOnly := False
          pendingWriteReq := writeReqPipe.payload
          pendingSlot := victimSlot
          when(slotValid(victimSlot) && slotDirty(victimSlot)) {
            evictBeat := 0
            state := OpState.EvictRead
          }.otherwise {
            allocateLine(victimSlot, writeReqLineBase)
            nextVictim := nextVictim + 1
            lineWriteValid := True
            lineWriteAddress := memIndex(victimSlot, writeReqWordIndex)
            lineWriteData := writeReqPipe.payload.data
            lineWriteMask := writeReqPipe.payload.mask
            slotKnownMask(victimSlot)(writeReqWordIndex) := writeReqPipe.payload.mask
            slotDirtyMask(victimSlot)(writeReqWordIndex) := writeReqPipe.payload.mask
            slotDirty(victimSlot) := True
          }
        }
      } elsewhen (!readRspValid && io.readReq.valid) {
        when(readHit) {
          io.readReq.ready := True
          pendingReadWasHit := True
          pendingFlushOnly := False
          readPortCmdValid := True
          readPortCmdAddress := memIndex(readHitSlot, readReqWordIndex)
          state := OpState.ReadWait
        }.otherwise {
          io.readReq.ready := True
          pendingReadWasHit := False
          pendingIsRead := True
          pendingFlushOnly := False
          pendingReadReq := io.readReq.payload
          pendingSlot := readMissSlot
          fillMisses := fillMisses + 1
          fillStallCycles := fillStallCycles + 1
          when(
            slotValid(readMissSlot) && slotDirty(readMissSlot) && slotBase(
              readMissSlot
            ) =/= readReqLineBase
          ) {
            evictBeat := 0
            state := OpState.EvictRead
          }.otherwise {
            when(!(slotValid(readMissSlot) && slotBase(readMissSlot) === readReqLineBase)) {
              allocateLine(readMissSlot, readReqLineBase)
              nextVictim := nextVictim + 1
            }
            fillRspCount := 0
            state := OpState.FillCmd
          }
        }
      } elsewhen (io.flush && dirtySlotAvailable) {
        pendingIsRead := False
        pendingFlushOnly := True
        pendingSlot := dirtySlot
        evictBeat := 0
        state := OpState.EvictRead
      }
    }

    when(state === OpState.EvictRead) {
      readPortCmdValid := True
      readPortCmdAddress := memIndex(pendingSlot, evictBeat)
      evictDataValid := False
      state := OpState.EvictSend
    }

    when(state === OpState.EvictSend) {
      when(lineReadRspValid) {
        evictData := lineReadRsp
        evictDataValid := True
      }
      io.mem.cmd.fragment.address := (slotBase(pendingSlot) + (evictBeat << 2).resized).resized
      io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.WRITE
      io.mem.cmd.fragment.length := lineLength
      io.mem.cmd.fragment.source := 0
      io.mem.cmd.fragment.data := evictData
      io.mem.cmd.fragment.mask := slotDirtyMask(pendingSlot)(evictBeat)
      io.mem.cmd.last := evictBeat === (lineWords - 1)
      io.mem.cmd.valid := evictDataValid
      when(io.mem.cmd.fire) {
        evictDataValid := False
        slotDirtyMask(pendingSlot)(evictBeat) := B(0, 4 bits)
        when(evictBeat === (lineWords - 1)) {
          slotDirty(pendingSlot) := False
          state := OpState.EvictRsp
        }.otherwise {
          evictBeat := evictBeat + 1
          state := OpState.EvictRead
        }
      }
    }

    when(state === OpState.EvictRsp) {
      io.mem.rsp.ready := True
      when(io.mem.rsp.fire) {
        when(pendingFlushOnly) {
          pendingFlushOnly := False
          state := OpState.Idle
        }.otherwise {
          when(pendingIsRead) {
            when(slotBase(pendingSlot) =/= lineBaseOf(pendingReadReq.address)) {
              allocateLine(pendingSlot, lineBaseOf(pendingReadReq.address))
              nextVictim := nextVictim + 1
            }
            fillRspCount := 0
            state := OpState.FillCmd
          }.otherwise {
            allocateLine(pendingSlot, lineBaseOf(pendingWriteReq.address))
            nextVictim := nextVictim + 1
            lineWriteValid := True
            lineWriteAddress := memIndex(pendingSlot, wordIndexOf(pendingWriteReq.address))
            lineWriteData := pendingWriteReq.data
            lineWriteMask := pendingWriteReq.mask
            slotKnownMask(pendingSlot)(wordIndexOf(pendingWriteReq.address)) := pendingWriteReq.mask
            slotDirtyMask(pendingSlot)(wordIndexOf(pendingWriteReq.address)) := pendingWriteReq.mask
            slotDirty(pendingSlot) := True
            state := OpState.Idle
          }
        }
      }
    }

    when(state === OpState.FillCmd) {
      io.mem.cmd.valid := True
      io.mem.cmd.fragment.address := slotBase(pendingSlot)
      io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
      io.mem.cmd.fragment.length := lineLength
      io.mem.cmd.fragment.source := 0
      io.mem.cmd.last := True
      when(io.mem.cmd.fire) {
        fillBurstCount := fillBurstCount + 1
        fillBurstBeats := fillBurstBeats + U(lineWords, 32 bits)
        state := OpState.FillWait
      }
    }

    when(state === OpState.FillWait) {
      val beat = fillRspCount.resize(lineShift bits)
      val fillMask = ~slotDirtyMask(pendingSlot)(beat)
      fillStoreCmd.valid := io.mem.rsp.valid
      fillStoreCmd.payload.slot := pendingSlot
      fillStoreCmd.payload.beat := beat
      fillStoreCmd.payload.data := io.mem.rsp.fragment.data
      fillStoreCmd.payload.mask := fillMask
      fillStoreCmd.payload.last := io.mem.rsp.last
      io.mem.rsp.ready := fillStoreCmd.ready
      when(fillStoreCmd.fire) {
        fillRspCount := fillRspCount + 1
      }
    }

    when(fillStorePipe.valid) {
      lineWriteValid := fillStorePipe.payload.mask.orR
      lineWriteAddress := memIndex(fillStorePipe.payload.slot, fillStorePipe.payload.beat)
      lineWriteData := fillStorePipe.payload.data
      lineWriteMask := fillStorePipe.payload.mask
      slotKnownMask(fillStorePipe.payload.slot)(fillStorePipe.payload.beat) := B"1111"
      when(fillStorePipe.payload.last) {
        state := OpState.FillRead
      }
    }

    when(state === OpState.FillRead) {
      readPortCmdValid := True
      readPortCmdAddress := memIndex(pendingSlot, wordIndexOf(pendingReadReq.address))
      state := OpState.ReadWait
    }
  } else {
    when(state === OpState.Idle) {
      when(writeReqPipe.valid) {
        writeReqPipe.ready := True
        pendingWriteReq := writeReqPipe.payload
        state := OpState.DirectWriteCmd
      } elsewhen (!readRspValid && io.readReq.valid) {
        io.readReq.ready := True
        pendingReadReq := io.readReq.payload
        state := OpState.DirectReadCmd
      }
    }

    when(state === OpState.DirectReadCmd) {
      io.mem.cmd.valid := True
      io.mem.cmd.fragment.address := pendingReadReq.address
      io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
      io.mem.cmd.fragment.length := 3
      io.mem.cmd.fragment.source := 0
      io.mem.cmd.last := True
      when(io.mem.cmd.fire) {
        state := OpState.DirectReadRsp
      }
    }

    when(state === OpState.DirectReadRsp) {
      io.mem.rsp.ready := !readRspValid
      when(io.mem.rsp.fire) {
        readRspValid := True
        readRspData := io.mem.rsp.fragment.data
        state := OpState.Idle
      }
    }

    when(state === OpState.DirectWriteCmd) {
      io.mem.cmd.valid := True
      io.mem.cmd.fragment.address := pendingWriteReq.address
      io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.WRITE
      io.mem.cmd.fragment.length := 3
      io.mem.cmd.fragment.source := 0
      io.mem.cmd.fragment.data := pendingWriteReq.data
      io.mem.cmd.fragment.mask := pendingWriteReq.mask
      io.mem.cmd.last := True
      when(io.mem.cmd.fire) {
        state := OpState.DirectWriteRsp
      }
    }

    when(state === OpState.DirectWriteRsp) {
      io.mem.rsp.ready := True
      when(io.mem.rsp.fire) {
        state := OpState.Idle
      }
    }
  }

  GenerationFlags.formal {
    val formalReset = ClockDomain.current.isResetActive
    if (formalStrong) {
      val fWordAddr = anyconst(UInt(c.addressWidth.value bits))
      val fInitData = anyconst(Bits(32 bits))
      val fData = Reg(Bits(32 bits)) init (fInitData)
      val fLineBase = lineBaseOf(fWordAddr)
      val fWordIndex = wordIndexOf(fWordAddr)
      assume(fWordAddr(1 downto 0) === U(0, 2 bits))

      val fSlotData = Vec((0 until slotCount).map(_ => Reg(Bits(32 bits)) init (0)))
      when(formalReset) {
        fData := fInitData
        for (slot <- 0 until slotCount) {
          fSlotData(slot) := 0
        }
      }
      for (slot <- 0 until slotCount) {
        when(
          lineWriteValid && lineWriteAddress === memIndex(U(slot, slotIndexWidth bits), fWordIndex)
        ) {
          for (lane <- 0 until 4) {
            when(lineWriteMask(lane)) {
              fSlotData(slot)(8 * lane + 7 downto 8 * lane) := lineWriteData(
                8 * lane + 7 downto 8 * lane
              )
            }
          }
        }
      }

      val fTrackedWriteNow =
        writeReqPipe.fire && writeReqLineBase === fLineBase && writeReqWordIndex === fWordIndex &&
          (writeHit || !(slotValid(victimSlot) && slotDirty(victimSlot)))
      when(fTrackedWriteNow) {
        for (lane <- 0 until 4) {
          when(writeReqPipe.payload.mask(lane)) {
            fData(8 * lane + 7 downto 8 * lane) := writeReqPipe.payload.data(
              8 * lane + 7 downto 8 * lane
            )
          }
        }
      }

      val fTrackedDelayedWrite = state === OpState.EvictRsp && io.mem.rsp.fire && !pendingIsRead &&
        lineBaseOf(pendingWriteReq.address) === fLineBase && wordIndexOf(
          pendingWriteReq.address
        ) === fWordIndex
      when(fTrackedDelayedWrite) {
        for (lane <- 0 until 4) {
          when(pendingWriteReq.mask(lane)) {
            fData(8 * lane + 7 downto 8 * lane) := pendingWriteReq.data(
              8 * lane + 7 downto 8 * lane
            )
          }
        }
      }

      val fTrackedFillBeat = fillRspCount.resize(lineShift bits) === fWordIndex
      when(
        io.mem.rsp.fire && state === OpState.FillWait && slotBase(
          pendingSlot
        ) === fLineBase && fTrackedFillBeat
      ) {
        val fFillMask = ~slotDirtyMask(pendingSlot)(fWordIndex)
        for (lane <- 0 until 4) {
          when(fFillMask(lane)) {
            assume(
              io.mem.rsp.fragment.data(8 * lane + 7 downto 8 * lane) === fData(
                8 * lane + 7 downto 8 * lane
              )
            )
          }
        }
      }

      val fReadPending = Reg(Bool()) init (False)
      val fReadExpected = Reg(Bits(32 bits)) init (0)
      when(formalReset) {
        fReadPending := False
        fReadExpected := 0
      }
      when(io.readReq.fire && io.readReq.address === fWordAddr && !fReadPending) {
        fReadPending := True
        fReadExpected := fData
      }
      when(!formalReset && io.readRsp.fire && fReadPending) {
        assert(io.readRsp.data === fReadExpected)
        fReadPending := False
      }

      when(!formalReset) {
        for (slot <- 0 until slotCount) {
          when(slotValid(slot) && slotBase(slot) === fLineBase) {
            for (lane <- 0 until 4) {
              when(slotKnownMask(slot)(fWordIndex)(lane)) {
                assert(
                  fSlotData(slot)(8 * lane + 7 downto 8 * lane) === fData(
                    8 * lane + 7 downto 8 * lane
                  )
                )
              }
            }
          }
        }
      }
    }

    when(!formalReset) {
      if (formalStrong) {
        for (slot <- 0 until slotCount) {
          when(!slotValid(slot)) {
            assert(!slotDirty(slot))
            for (word <- 0 until lineWords) {
              assert(slotKnownMask(slot)(word) === B(0, 4 bits))
              assert(slotDirtyMask(slot)(word) === B(0, 4 bits))
            }
          }
          for (word <- 0 until lineWords) {
            assert((slotDirtyMask(slot)(word) & ~slotKnownMask(slot)(word)) === B(0, 4 bits))
          }
        }
      }

      when(readHit) {
        assert(io.readReq.valid)
        assert(slotValid(readHitSlot))
        assert(slotBase(readHitSlot) === readReqLineBase)
        assert(slotKnownMask(readHitSlot)(readReqWordIndex) === B"1111")
      }

      when(writeHit) {
        assert(writeReqPipe.valid)
        assert(slotValid(writeHitSlot))
        assert(slotBase(writeHitSlot) === writeReqLineBase)
      }

      when(state === OpState.FillCmd) {
        assert(io.mem.cmd.valid)
        assert(io.mem.cmd.fragment.opcode === Bmb.Cmd.Opcode.READ)
        assert(io.mem.cmd.fragment.address === slotBase(pendingSlot))
        assert(io.mem.cmd.fragment.length === lineLength)
        assert(io.mem.cmd.last)
      }

      when(state === OpState.EvictSend && io.mem.cmd.valid) {
        assert(io.mem.cmd.fragment.opcode === Bmb.Cmd.Opcode.WRITE)
        assert(
          io.mem.cmd.fragment.address === (slotBase(pendingSlot) + (evictBeat << 2).resized).resized
        )
        assert(io.mem.cmd.fragment.length === lineLength)
        assert(io.mem.cmd.fragment.mask === slotDirtyMask(pendingSlot)(evictBeat))
        assert(io.mem.cmd.last === (evictBeat === (lineWords - 1)))
        assert(evictDataValid)
      }

      if (formalStrong) {
        val prevFillStoreFire = RegNext(fillStorePipe.fire) init (False)
        val prevFillSlot = RegNext(fillStorePipe.payload.slot) init (0)
        val prevFillBeat = RegNext(fillStorePipe.payload.beat) init (0)

        when(prevFillStoreFire) {
          assert(slotKnownMask(prevFillSlot)(prevFillBeat) === B"1111")
        }
      }
    }
  }

  io.busy := state =/= OpState.Idle || readRspValid || writeReqPipe.valid || fillStorePipe.valid ||
    (io.flush && dirtySlotAvailable)
}

object FramebufferPlaneCache {
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

  case class FillStore(c: Config, lineShift: Int, slotIndexWidth: Int) extends Bundle {
    val slot = UInt(slotIndexWidth bits)
    val beat = UInt(lineShift bits)
    val data = Bits(32 bits)
    val mask = Bits(4 bits)
    val last = Bool()
  }

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
