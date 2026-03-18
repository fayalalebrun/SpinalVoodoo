package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

case class TmuTextureCache(c: voodoo.Config, formalStrong: Boolean = true) extends Component {
  val io = new Bundle {
    val sampleRequest = slave Stream (Tmu.SampleRequest(c))
    val fetched = master Stream (Tmu.FetchResult(c))
    val fastFetch = master Stream (Tmu.FastFetch(c))
    val outputRoute = master Stream (Bool())
    val texRead = master(Bmb(Tmu.bmbParams(c)))
    val busy = out Bool ()
  }

  val expandedStream = Stream(Tmu.TmuExpanded(c))
  val expandCount = Reg(UInt(2 bits)) init 0
  val isExpanding = expandCount =/= 0
  val useFastBilinear = Bool()

  val regAddr1 = Reg(UInt(c.addressWidth.value bits))
  val regAddr2 = Reg(UInt(c.addressWidth.value bits))
  val regAddr3 = Reg(UInt(c.addressWidth.value bits))
  val regPassthrough = Reg(Tmu.TmuPassthrough(c))

  val fastHoldValid = Reg(Bool()) init False
  val fastHoldPayload = Reg(Tmu.FastFetch(c))
  fastHoldPayload := fastHoldPayload

  expandedStream.valid := (io.sampleRequest.valid && !useFastBilinear && !isExpanding) || isExpanding

  when(isExpanding) {
    expandedStream.payload.passthrough.format := regPassthrough.format
    expandedStream.payload.passthrough.bilinear := regPassthrough.bilinear
    expandedStream.payload.passthrough.sendConfig := regPassthrough.sendConfig
    expandedStream.payload.passthrough.ds := regPassthrough.ds
    expandedStream.payload.passthrough.dt := regPassthrough.dt
    expandedStream.payload.passthrough.readIdx := expandCount
    expandedStream.payload.passthrough.ncc := regPassthrough.ncc
    if (c.trace.enabled) {
      expandedStream.payload.passthrough.trace := regPassthrough.trace
    }
    switch(expandCount) {
      is(1) { expandedStream.payload.address := regAddr1 }
      is(2) { expandedStream.payload.address := regAddr2 }
      default { expandedStream.payload.address := regAddr3 }
    }
  }.otherwise {
    expandedStream.payload.passthrough := io.sampleRequest.payload.passthrough
    expandedStream.payload.address := io.sampleRequest.payload.bilinear ? io.sampleRequest.payload.biAddr0 | io.sampleRequest.payload.pointAddr
  }

  when(expandedStream.fire) {
    when(!isExpanding && io.sampleRequest.payload.bilinear) {
      expandCount := 1
      regAddr1 := io.sampleRequest.payload.biAddr1
      regAddr2 := io.sampleRequest.payload.biAddr2
      regAddr3 := io.sampleRequest.payload.biAddr3
      regPassthrough := io.sampleRequest.payload.passthrough
    }.elsewhen(isExpanding && expandCount < 3) {
      expandCount := expandCount + 1
    }.elsewhen(isExpanding) {
      expandCount := 0
    }
  }

  val texFillHits = Reg(UInt(32 bits)) init (0)
  val texFillMisses = Reg(UInt(32 bits)) init (0)
  val texFillBurstCount = Reg(UInt(32 bits)) init (0)
  val texFillBurstBeats = Reg(UInt(32 bits)) init (0)
  val texFillStallCycles = Reg(UInt(32 bits)) init (0)
  val texFastBilinearHits = Reg(UInt(32 bits)) init (0)
  if (c.trace.enabled) {
    texFillHits.simPublic()
    texFillMisses.simPublic()
    texFillBurstCount.simPublic()
    texFillBurstBeats.simPublic()
    texFillStallCycles.simPublic()
    texFastBilinearHits.simPublic()
  }

  val joined = Stream(Tmu.FetchResult(c))
  val fetchBusy = Bool()

  io.fastFetch.valid := fastHoldValid
  io.fastFetch.payload := fastHoldPayload
  when(io.fastFetch.fire) {
    fastHoldValid := False
  }

  if (c.useTexFillCache) {
    require(c.texFillLineWords > 0 && ((c.texFillLineWords & (c.texFillLineWords - 1)) == 0))
    require(c.texFillLineWords >= 4 && (c.texFillLineWords % 4) == 0)
    require((c.texFillLineWords * 4 - 1) < (1 << c.memBurstLengthWidth))

    val lineWords = c.texFillLineWords
    val lineShift = log2Up(lineWords)
    val bankCount = 4
    val bankEntries = lineWords / bankCount
    val bankEntryWidth = log2Up(bankEntries)
    val slotCount = c.texFillCacheSlots
    val slotIndexWidth = scala.math.max(1, log2Up(slotCount))

    require(slotCount > 0)
    require(c.texFillRequestWindow > 0)

    val requestStream = expandedStream
      .translateWith {
        val req = Tmu.CachedReq(c, bankEntryWidth)
        val wordAddress = (expandedStream.payload.address >> 2).resize(c.addressWidth.value bits)
        val lineBaseWord =
          ((wordAddress >> lineShift) << lineShift).resize(c.addressWidth.value bits)
        val wordIndexInLine = wordAddress(lineShift - 1 downto 0)
        req.lineBase := (lineBaseWord << 2).resized
        req.bankSel := wordIndexInLine(1 downto 0)
        req.bankEntry := (wordIndexInLine >> 2).resize(bankEntryWidth bits)
        req.queued.passthrough := expandedStream.payload.passthrough
        req.queued.addrHalf := expandedStream.payload.address(1)
        req.queued.addrByte := expandedStream.payload.address(0)
        req
      }
      .queue(c.texFillRequestWindow)

    val activeValid = requestStream.valid
    val activeReq = requestStream.payload

    val slotTagValid = Vec(Reg(Bool()) init (False), slotCount)
    val slotBase = Vec(Reg(UInt(c.addressWidth.value bits)) init (0), slotCount)
    val slotBanks = Vec(
      (0 until slotCount).map(_ =>
        Vec(
          (0 until bankCount).map(_ =>
            Vec((0 until bankEntries).map(_ => Reg(Bits(32 bits)) init (0)))
          )
        )
      )
    )
    val slotWordValid = Vec(
      (0 until slotCount).map(_ =>
        Vec(
          (0 until bankCount).map(_ =>
            Vec((0 until bankEntries).map(_ => Reg(Bool()) init (False)))
          )
        )
      )
    )
    val nextVictim = Reg(UInt(slotIndexWidth bits)) init (0)

    def cachedWordForLine(lineBase: UInt, bankSel: UInt, bankEntry: UInt): (Bool, Bits) = {
      val hit = Bool()
      val data = Bits(32 bits)
      hit := False
      data := 0
      for (i <- 0 until slotCount) {
        when(slotTagValid(i) && slotBase(i) === lineBase && slotWordValid(i)(bankSel)(bankEntry)) {
          hit := True
          val bankWords = Vec(Bits(32 bits), bankCount)
          for (b <- 0 until bankCount) {
            bankWords(b) := slotBanks(i)(b)(bankEntry)
          }
          data := bankWords(bankSel)
        }
      }
      (hit, data)
    }

    val (hitAny, hitWord) =
      cachedWordForLine(activeReq.lineBase, activeReq.bankSel, activeReq.bankEntry)

    val fillActive = Reg(Bool()) init False
    val fillSlot = Reg(UInt(slotIndexWidth bits)) init (0)
    val fillCmdIssued = Reg(Bool()) init (False)
    val fillRspCount = Reg(UInt(log2Up(lineWords + 1) bits)) init (0)

    val fillStartSlot = UInt(slotIndexWidth bits)
    fillStartSlot := nextVictim
    val freeSlots = Vec(Bool(), slotCount)
    for (i <- 0 until slotCount) {
      freeSlots(i) := !slotTagValid(i)
    }
    for (i <- 0 until slotCount) {
      val earlierFree = if (i == 0) False else freeSlots.take(i).reduce(_ || _)
      when(freeSlots(i) && !earlierFree) {
        fillStartSlot := i
      }
    }

    val startFill = activeValid && !hitAny && !fillActive
    when(startFill) {
      fillActive := True
      fillSlot := fillStartSlot
      fillCmdIssued := False
      fillRspCount := 0
      slotTagValid(fillStartSlot) := True
      slotBase(fillStartSlot) := activeReq.lineBase
      nextVictim := nextVictim + 1
      for (b <- 0 until bankCount) {
        for (e <- 0 until bankEntries) {
          slotWordValid(fillStartSlot)(b)(e) := False
        }
      }
    }

    val fillLength = U(lineWords * 4 - 1, c.memBurstLengthWidth bits)
    io.texRead.cmd.valid := fillActive && !fillCmdIssued
    io.texRead.cmd.fragment.address := slotBase(fillSlot)
    io.texRead.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
    io.texRead.cmd.fragment.length := fillLength
    io.texRead.cmd.fragment.source := 0
    io.texRead.cmd.last := True
    when(io.texRead.cmd.fire) {
      fillCmdIssued := True
    }

    io.texRead.rsp.ready := fillActive
    when(io.texRead.rsp.fire) {
      val rspWordIndex = fillRspCount.resize(lineShift bits)
      val rspBank = rspWordIndex(1 downto 0)
      val rspEntry = (rspWordIndex >> 2).resize(bankEntryWidth bits)
      slotBanks(fillSlot)(rspBank)(rspEntry) := io.texRead.rsp.fragment.data
      slotWordValid(fillSlot)(rspBank)(rspEntry) := True
      fillRspCount := fillRspCount + 1
      when(io.texRead.rsp.last) {
        fillActive := False
      }
    }

    when(startFill) {
      texFillMisses := texFillMisses + 1
      texFillBurstCount := texFillBurstCount + 1
      texFillBurstBeats := texFillBurstBeats + U(lineWords, 32 bits)
    }
    when(activeValid && !hitAny) {
      texFillStallCycles := texFillStallCycles + 1
    }

    joined.valid := activeValid && hitAny
    joined.payload.rspData32 := hitWord
    joined.payload.queued := activeReq.queued
    requestStream.ready := joined.ready && hitAny
    when(joined.fire) {
      texFillHits := texFillHits + 1
    }

    def cachedWordForAddr(addr: UInt): (Bool, Bits) = {
      val wordAddr = (addr >> 2).resize(c.addressWidth.value bits)
      val localLineBaseWord =
        ((wordAddr >> lineShift) << lineShift).resize(c.addressWidth.value bits)
      val localWordIndex = wordAddr(lineShift - 1 downto 0)
      val localBankSel = localWordIndex(1 downto 0)
      val localBankEntry = (localWordIndex >> 2).resize(bankEntryWidth bits)
      cachedWordForLine((localLineBaseWord << 2).resized, localBankSel, localBankEntry)
    }

    val (fastHit0, fastWord0) = cachedWordForAddr(io.sampleRequest.payload.biAddr0)
    val (fastHit1, fastWord1) = cachedWordForAddr(io.sampleRequest.payload.biAddr1)
    val (fastHit2, fastWord2) = cachedWordForAddr(io.sampleRequest.payload.biAddr2)
    val (fastHit3, fastWord3) = cachedWordForAddr(io.sampleRequest.payload.biAddr3)
    val fastBilinearReady =
      io.sampleRequest.valid && io.sampleRequest.payload.bilinear && !isExpanding && !activeValid && !fastHoldValid &&
        fastHit0 && fastHit1 && fastHit2 && fastHit3
    useFastBilinear := fastBilinearReady

    val fastPayload = Tmu.FastFetch(c)
    fastPayload.passthrough := io.sampleRequest.payload.passthrough
    fastPayload.texels(0).rspData32 := fastWord0
    fastPayload.texels(0).addrHalf := io.sampleRequest.payload.biAddr0(1)
    fastPayload.texels(0).addrByte := io.sampleRequest.payload.biAddr0(0)
    fastPayload.texels(1).rspData32 := fastWord1
    fastPayload.texels(1).addrHalf := io.sampleRequest.payload.biAddr1(1)
    fastPayload.texels(1).addrByte := io.sampleRequest.payload.biAddr1(0)
    fastPayload.texels(2).rspData32 := fastWord2
    fastPayload.texels(2).addrHalf := io.sampleRequest.payload.biAddr2(1)
    fastPayload.texels(2).addrByte := io.sampleRequest.payload.biAddr2(0)
    fastPayload.texels(3).rspData32 := fastWord3
    fastPayload.texels(3).addrHalf := io.sampleRequest.payload.biAddr3(1)
    fastPayload.texels(3).addrByte := io.sampleRequest.payload.biAddr3(0)

    when(io.sampleRequest.fire && useFastBilinear) {
      fastHoldValid := True
      fastHoldPayload := fastPayload
      texFillHits := texFillHits + U(4, 32 bits)
      texFastBilinearHits := texFastBilinearHits + 1
    }

    GenerationFlags.formal {
      if (formalStrong) {
        val fWordAddr = anyconst(UInt(c.addressWidth.value bits))
        val fData = anyconst(Bits(32 bits))
        assume(fWordAddr(1 downto 0) === U(0, 2 bits))

        val fWordIndex = (fWordAddr >> 2).resize(lineShift bits)
        val fLineBase =
          (((fWordAddr >> (lineShift + 2)) << (lineShift + 2))).resize(c.addressWidth.value bits)
        val fBankSel = fWordIndex(1 downto 0)
        val fBankEntry = (fWordIndex >> 2).resize(bankEntryWidth bits)
        val fTrackedActive =
          activeValid && activeReq.lineBase === fLineBase && activeReq.bankSel === fBankSel && activeReq.bankEntry === fBankEntry
        val fRspWordIndex = fillRspCount.resize(lineShift bits)
        val fRspBank = fRspWordIndex(1 downto 0)
        val fRspEntry = (fRspWordIndex >> 2).resize(bankEntryWidth bits)
        val fTrackedRspBeat = io.texRead.rsp.fire && slotBase(
          fillSlot
        ) === fLineBase && fRspBank === fBankSel && fRspEntry === fBankEntry

        when(fTrackedRspBeat) {
          assume(io.texRead.rsp.fragment.data === fData)
        }

        for (i <- 0 until slotCount) {
          when(
            slotTagValid(i) && slotBase(i) === fLineBase && slotWordValid(i)(fBankSel)(fBankEntry)
          ) {
            assert(slotBanks(i)(fBankSel)(fBankEntry) === fData)
          }
        }

        when(fTrackedActive && hitAny) {
          assert(hitWord === fData)
        }

        when(
          io.sampleRequest.fire && useFastBilinear && io.sampleRequest.payload.biAddr0 === fWordAddr
        ) {
          assert(fastPayload.texels(0).rspData32 === fData)
        }
        when(
          io.sampleRequest.fire && useFastBilinear && io.sampleRequest.payload.biAddr1 === fWordAddr
        ) {
          assert(fastPayload.texels(1).rspData32 === fData)
        }
        when(
          io.sampleRequest.fire && useFastBilinear && io.sampleRequest.payload.biAddr2 === fWordAddr
        ) {
          assert(fastPayload.texels(2).rspData32 === fData)
        }
        when(
          io.sampleRequest.fire && useFastBilinear && io.sampleRequest.payload.biAddr3 === fWordAddr
        ) {
          assert(fastPayload.texels(3).rspData32 === fData)
        }
      }

      assert(joined.valid === (activeValid && hitAny))
      assert(requestStream.ready === (joined.ready && hitAny))
      assert(io.texRead.cmd.valid === (fillActive && !fillCmdIssued))

      when(io.texRead.cmd.valid) {
        assert(io.texRead.cmd.fragment.opcode === Bmb.Cmd.Opcode.READ)
        assert(io.texRead.cmd.fragment.address === slotBase(fillSlot))
        assert(io.texRead.cmd.fragment.length === fillLength)
        assert(io.texRead.cmd.last)
      }

      if (formalStrong) {
        val prevStartFill = RegNext(startFill) init (False)
        val prevFillStartSlot = RegNext(fillStartSlot) init (0)
        val prevLineBase = RegNext(activeReq.lineBase) init (0)

        when(prevStartFill) {
          assert(slotTagValid(prevFillStartSlot))
          assert(slotBase(prevFillStartSlot) === prevLineBase)
          for (b <- 0 until bankCount) {
            for (e <- 0 until bankEntries) {
              assert(!slotWordValid(prevFillStartSlot)(b)(e))
            }
          }
        }

        val prevRspFire = RegNext(io.texRead.rsp.fire) init (False)
        val prevFillSlot = RegNext(fillSlot) init (0)
        val prevRspWordIndex = RegNext(fillRspCount.resize(lineShift bits)) init (0)
        val prevRspBank = prevRspWordIndex(1 downto 0)
        val prevRspEntry = (prevRspWordIndex >> 2).resize(bankEntryWidth bits)

        when(prevRspFire) {
          assert(slotWordValid(prevFillSlot)(prevRspBank)(prevRspEntry))
        }
      }

      if (formalStrong) {
        assert(useFastBilinear === fastBilinearReady)
        when(useFastBilinear) {
          assert(io.sampleRequest.valid)
          assert(io.sampleRequest.payload.bilinear)
          assert(!isExpanding)
          assert(!activeValid)
          assert(!fastHoldValid)
          assert(fastHit0)
          assert(fastHit1)
          assert(fastHit2)
          assert(fastHit3)
        }
      }

      cover(startFill)
      cover(hitAny)
      cover(useFastBilinear)
    }

    fetchBusy := activeValid || fillActive
  } else {
    useFastBilinear := False
    fetchBusy := False
    val (toMemory, toQueue) = StreamFork2(expandedStream)

    val cmdStream = toMemory.translateWith {
      val cmd = Fragment(BmbCmd(Tmu.bmbParams(c)))
      cmd.fragment.address := toMemory.payload.address
      cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
      cmd.fragment.length := 3
      cmd.fragment.source := 0
      cmd.last := True
      cmd
    }
    io.texRead.cmd << cmdStream

    val queuedData = toQueue
      .map { e =>
        val d = Tmu.QueuedData(c)
        d.passthrough := e.passthrough
        d.addrHalf := e.address(1)
        d.addrByte := e.address(0)
        d
      }
      .queue(16)

    val rspStream = io.texRead.rsp.takeWhen(io.texRead.rsp.last).translateWith {
      io.texRead.rsp.fragment.data
    }

    val joinedRaw = StreamJoin(rspStream, queuedData)
    joined << joinedRaw.translateWith {
      val result = Tmu.FetchResult(c)
      result.rspData32 := joinedRaw.payload._1
      result.queued := joinedRaw.payload._2
      result
    }
  }

  val acceptSlow = !isExpanding && !useFastBilinear && expandedStream.ready
  val acceptFast = useFastBilinear && !fastHoldValid
  val canAcceptSample = acceptSlow || acceptFast

  io.outputRoute.valid := io.sampleRequest.valid && canAcceptSample
  io.outputRoute.payload := useFastBilinear
  io.sampleRequest.ready := io.outputRoute.ready && canAcceptSample
  io.fetched << joined
  io.busy := fetchBusy
}
