package voodoo.texture

import voodoo._
import spinal.core._
import spinal.core.formal._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

case class TmuTextureCacheSlotMeta(c: voodoo.Config) extends Bundle {
  val base = UInt(c.addressWidth.value bits)
  val valid = Bool()
  val ready = Bool()
  val epoch = UInt(8 bits)
}

case class TmuTextureCacheSetMeta(c: voodoo.Config, wayCount: Int) extends Bundle {
  val ways = Vec(TmuTextureCacheSlotMeta(c), wayCount)
  val victim = UInt(scala.math.max(1, log2Up(wayCount)) bits)
}

case class TmuTextureCacheLookupCtx(c: voodoo.Config, setBits: Int, bankEntryWidth: Int)
    extends Bundle {
  val sample = Tmu.SampleRequest(c)
  val epoch = UInt(8 bits)
  val tapActive = Vec(Bool(), 4)
  val tapBank = Vec(UInt(2 bits), 4)
  val tapPair = Vec(Bool(), 4)
  val tapSet = Vec(UInt(setBits bits), 4)
  val tapLineBase = Vec(UInt(c.addressWidth.value bits), 4)
  val tapBankEntry = Vec(UInt(bankEntryWidth bits), 4)
}

case class TmuTextureCacheCompareStage(
    c: voodoo.Config,
    setBits: Int,
    bankEntryWidth: Int,
    wayCount: Int
) extends Bundle {
  val sample = Tmu.SampleRequest(c)
  val epoch = UInt(8 bits)
  val allHits = Bool()
  val anyMiss = Bool()
  val tapHit = Vec(Bool(), 4)
  val tapSetMeta = Vec(TmuTextureCacheSetMeta(c, wayCount), 4)
  val tapData = Vec(Bits(32 bits), 4)
  val missSet = UInt(setBits bits)
  val missLineBase = UInt(c.addressWidth.value bits)
  val missBank = UInt(2 bits)
  val missBankEntry = UInt(bankEntryWidth bits)
  val missValidVec = Vec(Bool(), wayCount)
  val missSetMeta = TmuTextureCacheSetMeta(c, wayCount)
}

case class TmuTextureCacheDataStage(c: voodoo.Config) extends Bundle {
  val bilinear = Bool()
  val passthrough = Tmu.TmuPassthrough(c)
  val texels = Vec(Bits(16 bits), 4)
}

case class TmuTextureCacheReplayReq(c: voodoo.Config) extends Bundle {
  val sample = Tmu.SampleRequest(c)
  val epoch = UInt(8 bits)
}

case class TmuTextureCacheFillStart(
    c: voodoo.Config,
    slotBits: Int,
    setBits: Int,
    bankEntryWidth: Int,
    wayBits: Int,
    wayCount: Int
) extends Bundle {
  val set = UInt(setBits bits)
  val way = UInt(wayBits bits)
  val slot = UInt(slotBits bits)
  val evictValid = Bool()
  val evictReady = Bool()
  val setMeta = TmuTextureCacheSetMeta(c, wayCount)
  val req = Tmu.CachedReq(c, bankEntryWidth)
  val directPoint = Bool()
  val passthrough = Tmu.TmuPassthrough(c)
}

case class TmuTextureCacheDirectWordReq(c: voodoo.Config) extends Bundle {
  val address = UInt(c.addressWidth.value bits)
  val is16Bit = Bool()
  val firstWord = Bool()
  val lastWord = Bool()
  val bilinear = Bool()
  val tapMask = Bits(4 bits)
  val tapHalf = Vec(Bool(), 4)
  val tapByte = Vec(Bool(), 4)
  val passthrough = Tmu.TmuPassthrough(c)
}

case class TmuTexturePathIo(c: voodoo.Config) extends Bundle {
  val sampleRequest = slave Stream (Tmu.SampleRequest(c))
  val sampleFetch = master Stream (Tmu.SampleFetch(c))
  val texRead = master(Bmb(Tmu.bmbParams(c)))
  val invalidate = in Bool ()
  val busy = out Bool ()
}

abstract class TmuTexturePathBase(val c: voodoo.Config) extends Component {
  val io = TmuTexturePathIo(c)

  protected def markSimulationOnly(x: Data*): Unit = GenerationFlags.simulation {
    x.foreach(_.simPublic())
  }

  protected def withTrace(x: Data*): Unit = if (c.trace.enabled) markSimulationOnly(x: _*)

  protected def pointOrBilinearAddr(req: Tmu.SampleRequest, i: Int): UInt =
    if (i == 0) (req.bilinear ? req.biAddr0 | req.pointAddr)
    else Seq(req.biAddr1, req.biAddr2, req.biAddr3)(i - 1)

  protected def pointOrBilinearBank(req: Tmu.SampleRequest, i: Int): UInt = {
    val loc = Tmu.packedLocation22(
      pointOrBilinearAddr(req, i),
      req.is16Bit,
      req.lodBase,
      req.lodShift,
      (0 until 9).map(req.texTables.texBase(_)),
      (0 until 9).map(req.texTables.texEnd(_)),
      (0 until 9).map(req.texTables.texShift(_))
    )
    loc._2
  }

  protected def texel16(word: Bits, half: Bool, byte: Bool, is16Bit: Bool): Bits = {
    val out = Bits(16 bits)
    val halfWord = Mux(half, word(31 downto 16), word(15 downto 0))
    val texByte = Mux(byte, halfWord(15 downto 8), halfWord(7 downto 0))
    out := is16Bit ? halfWord | (texByte ## texByte)
    out
  }

  protected def packedPairSel(req: Tmu.SampleRequest, i: Int): Bool = {
    val loc = Tmu.packedLocation22(
      pointOrBilinearAddr(req, i),
      req.is16Bit,
      req.lodBase,
      req.lodShift,
      (0 until 9).map(req.texTables.texBase(_)),
      (0 until 9).map(req.texTables.texEnd(_)),
      (0 until 9).map(req.texTables.texShift(_))
    )
    loc._3
  }

  protected def bankTexel(word: Bits, pairSel: Bool): Bits =
    Mux(pairSel, word(31 downto 16), word(15 downto 0))
}

case class TmuTextureCache(
    override val c: voodoo.Config,
    formalStrong: Boolean = true,
    formalHotInit: Boolean = false
) extends TmuTexturePathBase(c) {

  def noTapBankAlias(req: Tmu.SampleRequest): Bool = {
    val pairOk = for (i <- 0 until 4; j <- i + 1 until 4) yield {
      val activeI = if (i == 0) True else req.bilinear
      val activeJ = if (j == 0) True else req.bilinear
      !(activeI && activeJ && pointOrBilinearBank(req, i) === pointOrBilinearBank(
        req,
        j
      ) && pointOrBilinearAddr(
        req,
        i
      ) =/= pointOrBilinearAddr(req, j))
    }
    pairOk.reduce(_ && _)
  }

  def cacheIndex(slot: UInt, entry: UInt, slotBits: Int, entryBits: Int): UInt = (((slot.resize(
    slotBits bits
  ) ## U(0, entryBits bits)) | entry.resize(slotBits + entryBits bits).asBits).asUInt)

  val texFillHits = Reg(UInt(32 bits)) init 0
  val texFillMisses = Reg(UInt(32 bits)) init 0
  val texFillBurstCount = Reg(UInt(32 bits)) init 0
  val texFillBurstBeats = Reg(UInt(32 bits)) init 0
  val texFillStallCycles = Reg(UInt(32 bits)) init 0
  val texFastBilinearHits = Reg(UInt(32 bits)) init 0
  val texCompareMissSamples = Reg(UInt(32 bits)) init 0
  val texLookupBlockedCycles = Reg(UInt(32 bits)) init 0
  val texLookupBlockedByOwnerCycles = Reg(UInt(32 bits)) init 0
  val texLookupBlockedByFillCycles = Reg(UInt(32 bits)) init 0
  val texLookupBlockedByHoldCycles = Reg(UInt(32 bits)) init 0
  val texLookupBlockedByLiveCycles = Reg(UInt(32 bits)) init 0
  val texFillEvictValid = Reg(UInt(32 bits)) init 0
  val texFillEvictReady = Reg(UInt(32 bits)) init 0
  val texFillEvictInflight = Reg(UInt(32 bits)) init 0
  markSimulationOnly(
    texFillHits,
    texFillMisses,
    texFastBilinearHits,
    texCompareMissSamples,
    texLookupBlockedCycles,
    texLookupBlockedByOwnerCycles,
    texLookupBlockedByFillCycles,
    texLookupBlockedByHoldCycles,
    texLookupBlockedByLiveCycles,
    texFillEvictValid,
    texFillEvictReady,
    texFillEvictInflight
  )
  withTrace(
    texFillHits,
    texFillMisses,
    texFillBurstCount,
    texFillBurstBeats,
    texFillStallCycles,
    texFastBilinearHits,
    texCompareMissSamples,
    texLookupBlockedCycles,
    texLookupBlockedByOwnerCycles,
    texLookupBlockedByFillCycles,
    texLookupBlockedByHoldCycles,
    texLookupBlockedByLiveCycles,
    texFillEvictValid,
    texFillEvictReady,
    texFillEvictInflight
  )

  val reqStream = io.sampleRequest.queue(c.texFillRequestWindow)

  GenerationFlags.formal {
    when(reqStream.valid) {
      assume(noTapBankAlias(reqStream.payload))
    }
  }

  require(c.useTexFillCache)
  require(c.packedTexLayout)
  require(c.texFillLineWords > 0 && ((c.texFillLineWords & (c.texFillLineWords - 1)) == 0))
  require(c.texFillLineWords >= 4 && (c.texFillLineWords % 4) == 0)
  require((c.texFillLineWords * 4 - 1) < (1 << c.memBurstLengthWidth))
  require(c.texFillCacheSlots > 0)

  val lineWords = c.texFillLineWords
  val lineBytes = lineWords * 4
  val lineByteShift = log2Up(lineBytes)
  val bankCount = 4
  val tapCount = 4
  val slotCount = c.texFillCacheSlots
  val slotBits = scala.math.max(1, log2Up(slotCount))
  val wayCount = if (slotCount > 1) c.texFillWayCount else 1
  require(wayCount > 0)
  require((slotCount % wayCount) == 0)
  val setCount = slotCount / wayCount
  val setBits = scala.math.max(1, log2Up(setCount))
  val wayBits = scala.math.max(1, log2Up(wayCount))
  val bankEntries = lineWords
  val bankEntryWidth = log2Up(bankEntries)
  val fillLength = U(lineWords * 4 - 1, c.memBurstLengthWidth bits)

  def slotOf(set: UInt, way: UInt): UInt = {
    if (wayCount == 1) set.resize(slotBits bits)
    else
      (((set.resize(slotBits bits) |<< wayBits) +^ way.resize(slotBits bits))
        .resize(slotBits bits))
  }

  def bankIndex(set: UInt, entry: UInt): UInt = (((set.resize(setBits bits) ## U(
    0,
    bankEntryWidth bits
  )) | entry.resize(setBits + bankEntryWidth bits).asBits).asUInt)

  def lineBaseOf(addr: UInt): UInt =
    ((addr >> lineByteShift) << lineByteShift).resize(c.addressWidth.value bits)
  def lineBase22Of(addr: UInt): UInt =
    Tmu.lineBase22Of(addr, lineByteShift)
  def pointOrBilinearLineBase22(req: Tmu.SampleRequest, i: Int): UInt =
    if (i == 0) (req.bilinear ? lineBase22Of(req.biAddr0) | lineBase22Of(req.pointAddr))
    else lineBase22Of(Seq(req.biAddr1, req.biAddr2, req.biAddr3)(i - 1))
  def setOf(base: UInt): UInt = {
    if (setCount == 1) U(0, setBits bits)
    else {
      val line = base >> lineByteShift
      if (c.texFillXorIndex) {
        val folded = line.resize(setBits bits) ^
          (line >> setBits).resize(setBits bits) ^
          (line >> (setBits * 2)).resize(setBits bits)
        folded.resize(setBits bits)
      } else {
        line.resize(setBits bits)
      }
    }
  }
  def bankEntryOf(addr: UInt, base: UInt): UInt = ((addr - base) >> 2).resize(bankEntryWidth bits)

  def firstFree(valid: Vec[Bool], fallback: UInt): UInt = new Composite(valid) {
    val idx = UInt(fallback.getWidth bits)
    idx := fallback
    for (i <- valid.indices.reverse) {
      when(!valid(i)) {
        idx := U(i, fallback.getWidth bits)
      }
    }
  }.idx

  def zeroMeta(): TmuTextureCacheSlotMeta = {
    val z = TmuTextureCacheSlotMeta(c)
    z.base := 0
    z.valid := False
    z.ready := False
    z.epoch := 0
    z
  }

  def hotMeta(): TmuTextureCacheSetMeta = {
    val z = TmuTextureCacheSetMeta(c, wayCount)
    for (way <- 0 until wayCount) {
      if (way == 0) {
        z.ways(way).base := 0
        z.ways(way).valid := True
        z.ways(way).ready := True
        z.ways(way).epoch := 0
      } else {
        z.ways(way).base := 0
        z.ways(way).valid := False
        z.ways(way).ready := False
        z.ways(way).epoch := 0
      }
    }
    z.victim := (if (wayCount > 1) U(1, z.victim.getWidth bits) else U(0, z.victim.getWidth bits))
    z
  }

  val currentEpoch = Reg(UInt(8 bits)) init 0

  val metaMem = Seq.fill(tapCount)(Mem(TmuTextureCacheSetMeta(c, wayCount), setCount))
  val metaRead = Seq.tabulate(tapCount) { tap =>
    metaMem(tap).readSyncPort()
  }
  val metaWrite = Seq.tabulate(tapCount) { tap =>
    metaMem(tap).writePort
  }

  val bankMem = Seq.fill(bankCount)(Seq.fill(wayCount)(Mem(Bits(32 bits), setCount * bankEntries)))
  val bankRead = bankMem.map(_.map(_.readSyncPort()))

  def bankRsp(sel: UInt, way: Int): Bits = Vec(bankRead.map(_(way).rsp))(sel)

  val metaWriteEnable = Bool()
  val metaWriteSet = UInt(setBits bits)
  val metaWriteData = TmuTextureCacheSetMeta(c, wayCount)
  metaWriteEnable := False
  metaWriteSet := 0
  for (way <- 0 until wayCount) metaWriteData.ways(way) := zeroMeta()
  metaWriteData.victim := 0
  for (tap <- 0 until tapCount) {
    metaWrite(tap).valid := metaWriteEnable
    metaWrite(tap).address := metaWriteSet
    metaWrite(tap).data := metaWriteData
  }

  val compareHold = Reg(TmuTextureCacheCompareStage(c, setBits, bankEntryWidth, wayCount))
  val compareHoldValid = RegInit(False)
  val compareStage = TmuTextureCacheCompareStage(c, setBits, bankEntryWidth, wayCount)
  val compareValid = Bool()
  compareStage := compareHold
  compareValid := compareHoldValid
  markSimulationOnly(compareValid)
  val dbgCompareValid = compareValid.setName("dbgCompareValid")
  markSimulationOnly(dbgCompareValid)

  val fillFetchRsp = Stream(Tmu.SampleFetch(c))
  val fillFetchPendingValid = RegInit(False)
  val continuePendingValid = RegInit(False)
  val continuePendingIdx = Reg(UInt(2 bits)) init 0
  val hitRsp = Tmu.SampleFetch(c)
  io.sampleFetch.valid := False
  io.sampleFetch.payload.assignDontCare()
  fillFetchRsp.ready := False
  val dbgDataValid = io.sampleFetch.valid.setName("dbgDataValid")
  markSimulationOnly(dbgDataValid)
  val dbgOutputHoldValid =
    (io.sampleFetch.valid && !io.sampleFetch.ready).setName("dbgOutputHoldValid")
  markSimulationOnly(dbgOutputHoldValid)

  val dbgReplayPending = False.setName("dbgReplayPending")
  markSimulationOnly(dbgReplayPending)

  val owner = new Area {
    val active = RegInit(False)
    val sample = Reg(Tmu.SampleRequest(c))
    val epoch = Reg(UInt(8 bits)) init 0
    val resolved = Reg(Bits(tapCount bits)) init 0
    val tapMeta = Vec.fill(tapCount)(Reg(TmuTextureCacheSetMeta(c, wayCount)))
    val texels = Vec.fill(tapCount)(Reg(Bits(16 bits)))
  }

  val fill = new Area {
    val dbgVictimPending = False.setName("dbgVictimPending")
    markSimulationOnly(dbgVictimPending)
    val setMeta = Reg(TmuTextureCacheSetMeta(c, wayCount))

    val activeReg = RegInit(False)
    markSimulationOnly(activeReg)
    val dbgFillActive = activeReg.setName("dbgFillActive")
    markSimulationOnly(dbgFillActive)
    val slot = Reg(UInt(slotBits bits)) init 0
    val set = Reg(UInt(setBits bits)) init 0
    val way = Reg(UInt(wayBits bits)) init 0
    val rspCount = Reg(UInt(log2Up(lineWords + 1) bits)) init 0
    val req = Reg(Tmu.CachedReq(c, bankEntryWidth))
    val passthrough = Reg(Tmu.TmuPassthrough(c))
    val texBase = Vec.fill(9)(Reg(UInt(22 bits)))
    val texEnd = Vec.fill(9)(Reg(UInt(22 bits)))
    val texShift = Vec.fill(9)(Reg(UInt(4 bits)))
    val decodeValid = Vec.fill(4)(Reg(Bool()) init False)
    val decodeBank = Vec.fill(4)(Reg(UInt(2 bits)) init 0)
    val decodePair = Vec.fill(4)(Reg(Bool()) init False)
  }

  val lookupSource = reqStream.translateWith {
    val replay = TmuTextureCacheReplayReq(c)
    replay.sample := reqStream.payload
    replay.epoch := currentEpoch
    replay
  }

  val lookupCtx = TmuTextureCacheLookupCtx(c, setBits, bankEntryWidth)
  lookupCtx.sample := lookupSource.payload.sample
  lookupCtx.epoch := lookupSource.payload.epoch
  for (i <- 0 until tapCount) {
    val active = if (i == 0) True else lookupSource.payload.sample.bilinear
    val addr = pointOrBilinearAddr(lookupSource.payload.sample, i)
    val bank = pointOrBilinearBank(lookupSource.payload.sample, i)
    val base = lineBaseOf(addr)
    lookupCtx.tapActive(i) := active
    lookupCtx.tapBank(i) := bank
    lookupCtx.tapPair(i) := packedPairSel(lookupSource.payload.sample, i)
    lookupCtx.tapLineBase(i) := base
    lookupCtx.tapSet(i) := setOf(base)
    lookupCtx.tapBankEntry(i) := bankEntryOf(addr, base)
  }

  val initSweepActive = RegInit(if (formalHotInit) False else True)
  val initSweepSet = Reg(UInt(setBits bits)) init 0
  val formalHotInitDone = RegInit(if (formalHotInit) False else True)

  def buildFillStart(
      sample: Tmu.SampleRequest,
      epoch: UInt,
      tapSetMeta: TmuTextureCacheSetMeta,
      tapIdx: UInt
  ): TmuTextureCacheFillStart = {
    val start = TmuTextureCacheFillStart(c, slotBits, setBits, bankEntryWidth, wayBits, wayCount)
    val tapAddrs = Vec((0 until tapCount).map(i => pointOrBilinearAddr(sample, i)))
    val tapBanks = Vec((0 until tapCount).map(i => pointOrBilinearBank(sample, i)))
    val tapAddr = tapAddrs(tapIdx)
    val tapLineBase = lineBaseOf(tapAddr)
    val tapSet = setOf(tapLineBase)
    val tapEntry = bankEntryOf(tapAddr, tapLineBase)
    val validVec = Vec(Bool(), wayCount)
    for (way <- 0 until wayCount) {
      validVec(way) := tapSetMeta.ways(way).valid
    }
    val chosenWay = firstFree(validVec, tapSetMeta.victim)

    start.set := tapSet
    start.way := chosenWay
    start.slot := slotOf(tapSet, chosenWay)
    start.evictValid := False
    start.evictReady := False
    for (way <- 0 until wayCount) {
      start.setMeta.ways(way) := tapSetMeta.ways(way)
      when(chosenWay === U(way, wayBits bits)) {
        start.evictValid := tapSetMeta.ways(way).valid
        start.evictReady := tapSetMeta.ways(way).ready
      }
    }
    start.setMeta.victim := (chosenWay + 1).resize(start.setMeta.victim.getWidth bits)
    for (way <- 0 until wayCount) {
      when(chosenWay === U(way, wayBits bits)) {
        start.setMeta.ways(way).base := tapLineBase
        start.setMeta.ways(way).valid := True
        start.setMeta.ways(way).ready := False
        start.setMeta.ways(way).epoch := epoch
      }
    }
    start.req.lineBase := tapLineBase
    start.req.lodBase := sample.lodBase
    start.req.lodShift := sample.lodShift
    start.req.is16Bit := sample.is16Bit
    start.req.texTables := sample.texTables
    start.req.startupDecode := sample.tapStartupDecode(tapIdx)
    start.req.bankSel := tapBanks(tapIdx)
    start.req.bankEntry := tapEntry
    start.directPoint := !sample.bilinear
    start.passthrough := sample.passthrough
    start
  }

  val compareConsumeHit = Bool()
  val compareConsumeMiss = Bool()
  val compareConsumes = compareConsumeHit || compareConsumeMiss
  val lookupBlocked = Bool()
  lookupSource.ready := !lookupBlocked
  val lookupLaunch = lookupSource.fire

  for (tap <- 0 until tapCount) {
    metaRead(tap).cmd.valid := lookupSource.fire && lookupCtx.tapActive(tap)
    metaRead(tap).cmd.payload := lookupCtx.tapSet(tap)
  }

  for (b <- 0 until bankCount) {
    val bankUse =
      (0 until tapCount).map(i => lookupCtx.tapActive(i) && lookupCtx.tapBank(i) === U(b, 2 bits))
    val bankSet = MuxOH(bankUse, (0 until tapCount).map(lookupCtx.tapSet(_)))
    val bankEntry = MuxOH(bankUse, (0 until tapCount).map(lookupCtx.tapBankEntry(_)))
    for (way <- 0 until wayCount) {
      bankRead(b)(way).cmd.valid := lookupLaunch && bankUse.reduce(_ || _)
      bankRead(b)(way).cmd.payload := bankIndex(bankSet, bankEntry)
    }
  }

  val lookupRspValid = RegNext(lookupSource.fire) init False
  val lookupRspCtx = RegNextWhen(lookupCtx, lookupSource.fire)

  val arrivedCompare = TmuTextureCacheCompareStage(c, setBits, bankEntryWidth, wayCount)
  val hitOk = Vec(Bool(), tapCount)
  val hitWay = Vec(UInt(wayBits bits), tapCount)
  for (i <- 0 until tapCount) {
    val hits = Bits(wayCount bits)
    for (way <- 0 until wayCount) {
      val meta = metaRead(i).rsp.ways(way)
      hits(way) := lookupRspCtx.tapActive(
        i
      ) && meta.valid && meta.ready && meta.epoch === lookupRspCtx.epoch && meta.base === lookupRspCtx
        .tapLineBase(i)
    }
    hitOk(i) := hits.orR
    hitWay(i) := (if (wayCount == 1) U(0, wayBits bits) else OHToUInt(hits))
    arrivedCompare.tapHit(i) := hitOk(i)
    arrivedCompare.tapSetMeta(i) := metaRead(i).rsp
    arrivedCompare.tapData(i) := hitOk(i) ? MuxOH(
      hits.asBools,
      (0 until wayCount).map(way => bankRsp(lookupRspCtx.tapBank(i), way))
    ) | B(0, 32 bits)
  }

  arrivedCompare.sample := lookupRspCtx.sample
  arrivedCompare.epoch := lookupRspCtx.epoch
  arrivedCompare.allHits := (0 until tapCount)
    .map(i => !lookupRspCtx.tapActive(i) || hitOk(i))
    .reduce(_ && _)
  arrivedCompare.anyMiss := (0 until tapCount)
    .map(i => lookupRspCtx.tapActive(i) && !hitOk(i))
    .reduce(_ || _)

  val missIdx = UInt(2 bits)
  missIdx := 0
  when(lookupRspCtx.tapActive(0) && !hitOk(0)) {
    missIdx := 0
  } elsewhen (lookupRspCtx.tapActive(1) && !hitOk(1)) {
    missIdx := 1
  } elsewhen (lookupRspCtx.tapActive(2) && !hitOk(2)) {
    missIdx := 2
  } elsewhen (lookupRspCtx.tapActive(3) && !hitOk(3)) {
    missIdx := 3
  }

  arrivedCompare.missSet := lookupRspCtx.tapSet(missIdx)
  arrivedCompare.missLineBase := lookupRspCtx.tapLineBase(missIdx)
  arrivedCompare.missBank := lookupRspCtx.tapBank(missIdx)
  arrivedCompare.missBankEntry := lookupRspCtx.tapBankEntry(missIdx)
  for (way <- 0 until wayCount) {
    arrivedCompare.missValidVec(way) := metaRead(0).rsp.ways(way).valid
    when(missIdx === 1) { arrivedCompare.missValidVec(way) := metaRead(1).rsp.ways(way).valid }
    when(missIdx === 2) { arrivedCompare.missValidVec(way) := metaRead(2).rsp.ways(way).valid }
    when(missIdx === 3) { arrivedCompare.missValidVec(way) := metaRead(3).rsp.ways(way).valid }
    arrivedCompare.missSetMeta.ways(way) := metaRead(0).rsp.ways(way)
    when(missIdx === 1) { arrivedCompare.missSetMeta.ways(way) := metaRead(1).rsp.ways(way) }
    when(missIdx === 2) { arrivedCompare.missSetMeta.ways(way) := metaRead(2).rsp.ways(way) }
    when(missIdx === 3) { arrivedCompare.missSetMeta.ways(way) := metaRead(3).rsp.ways(way) }
  }
  arrivedCompare.missSetMeta.victim := metaRead(0).rsp.victim
  when(missIdx === 1) { arrivedCompare.missSetMeta.victim := metaRead(1).rsp.victim }
  when(missIdx === 2) { arrivedCompare.missSetMeta.victim := metaRead(2).rsp.victim }
  when(missIdx === 3) { arrivedCompare.missSetMeta.victim := metaRead(3).rsp.victim }

  when(!compareHoldValid) {
    compareStage := arrivedCompare
    compareValid := lookupRspValid
  }

  val liveHitCanConsume =
    lookupRspValid && arrivedCompare.allHits && !fillFetchRsp.valid && io.sampleFetch.ready
  val liveCompareBlocks = lookupRspValid && (!arrivedCompare.allHits || !liveHitCanConsume)
  lookupBlocked := initSweepActive || owner.active || fill.activeReg || compareHoldValid || liveCompareBlocks
  compareConsumeHit := compareValid && compareStage.allHits && !fillFetchRsp.valid && io.sampleFetch.ready
  when(lookupBlocked) {
    texLookupBlockedCycles := texLookupBlockedCycles + 1
    when(owner.active) {
      texLookupBlockedByOwnerCycles := texLookupBlockedByOwnerCycles + 1
    }
    when(fill.activeReg) {
      texLookupBlockedByFillCycles := texLookupBlockedByFillCycles + 1
    }
    when(compareHoldValid) {
      texLookupBlockedByHoldCycles := texLookupBlockedByHoldCycles + 1
    }
    when(liveCompareBlocks) {
      texLookupBlockedByLiveCycles := texLookupBlockedByLiveCycles + 1
    }
  }

  val compareTapActive = Vec(
    (0 until tapCount).map(i => if (i == 0) True else compareStage.sample.bilinear)
  )
  val compareTapLineBase = Vec(
    (0 until tapCount).map(i => pointOrBilinearLineBase22(compareStage.sample, i))
  )
  val compareTapSet = Vec(
    (0 until tapCount).map(i => setOf(compareTapLineBase(i)))
  )
  val compareMissSel = Vec(Bool(), tapCount)
  compareMissSel(0) := compareTapActive(0) && !compareStage.tapHit(0)
  compareMissSel(1) := !compareMissSel(0) && compareTapActive(1) && !compareStage.tapHit(1)
  compareMissSel(2) := !(compareMissSel(0) || compareMissSel(1)) && compareTapActive(
    2
  ) && !compareStage.tapHit(2)
  compareMissSel(3) := !(compareMissSel(0) || compareMissSel(1) || compareMissSel(
    2
  )) && compareTapActive(3) && !compareStage.tapHit(3)
  val compareMissIdx = UInt(2 bits)
  compareMissIdx := OHToUInt(compareMissSel.asBits)

  val compareFillStart = Stream(
    TmuTextureCacheFillStart(c, slotBits, setBits, bankEntryWidth, wayBits, wayCount)
  )
  compareFillStart.valid := compareValid && compareStage.anyMiss && !owner.active && !fill.activeReg
  compareFillStart.payload := buildFillStart(
    compareStage.sample,
    compareStage.epoch,
    compareStage.tapSetMeta(compareMissIdx),
    compareMissIdx
  )

  compareConsumeMiss := compareFillStart.fire
  when(!compareHoldValid && lookupRspValid && !compareConsumes) {
    compareHold := arrivedCompare
    compareHoldValid := True
  }
  when(compareHoldValid && compareConsumes) {
    compareHoldValid := False
  }

  when(compareConsumeHit) {
    texFillHits := texFillHits + (compareStage.sample.bilinear ? U(4, 32 bits) | U(1, 32 bits))
    when(compareStage.sample.bilinear) {
      texFastBilinearHits := texFastBilinearHits + 1
    }
  }

  when(compareConsumeMiss) {
    texCompareMissSamples := texCompareMissSamples + 1
    owner.active := True
    owner.sample := compareStage.sample
    owner.epoch := compareStage.epoch
    owner.resolved := compareStage.tapHit.asBits
    for (i <- 0 until tapCount) {
      owner.tapMeta(i) := compareStage.tapSetMeta(i)
      owner.texels(i) := Mux(
        compareStage.tapHit(i),
        bankTexel(compareStage.tapData(i), packedPairSel(compareStage.sample, i)),
        B(0, 16 bits)
      )
      when(compareTapSet(i) === compareFillStart.payload.set) {
        owner.tapMeta(i) := compareFillStart.payload.setMeta
      }
    }
  }

  val ownerTapActive = Vec(
    (0 until tapCount).map(i => if (i == 0) True else owner.sample.bilinear)
  )
  val ownerTapBank = Vec((0 until tapCount).map(i => pointOrBilinearBank(owner.sample, i)))
  val ownerTapLineBase = Vec(
    (0 until tapCount).map(i => lineBaseOf(pointOrBilinearAddr(owner.sample, i)))
  )
  val ownerTapSet = Vec((0 until tapCount).map(i => setOf(ownerTapLineBase(i))))
  val ownerTapEntry = Vec(
    (0 until tapCount).map(i =>
      bankEntryOf(pointOrBilinearAddr(owner.sample, i), ownerTapLineBase(i))
    )
  )
  val ownerTapPair = Vec((0 until tapCount).map(i => packedPairSel(owner.sample, i)))

  val fillRsp = io.texRead.rsp.haltWhen(!fill.activeReg)

  val wordAddr =
    (fill.req.lineBase + (fill.rspCount << 2).resized).resize(c.addressWidth.value bits)
  val wordEntry = fill.rspCount.resize(bankEntryWidth bits)

  def packedLocation(
      addr: UInt,
      is16Bit: Bool,
      lodBase: UInt,
      lodShift: UInt,
      texBase: Seq[UInt],
      texEnd: Seq[UInt],
      texShift: Seq[UInt]
  ): (Bool, UInt, Bool) =
    Tmu.packedLocation22(addr, is16Bit, lodBase, lodShift, texBase, texEnd, texShift)

  def packedLocation(addr: UInt): (Bool, UInt, Bool) =
    packedLocation(
      addr,
      fill.req.is16Bit,
      fill.req.lodBase,
      fill.req.lodShift,
      fill.texBase,
      fill.texEnd,
      fill.texShift
    )

  val mask = Bits(bankCount bits)
  mask := 0
  val pairMask = Vec.fill(bankCount)(Bits(2 bits))
  val writeData = Vec.fill(bankCount)(Bits(32 bits))
  for (b <- 0 until bankCount) {
    pairMask(b) := 0
    writeData(b) := 0
  }
  for (byteIdx <- 0 until 4) {
    val fmt = if ((byteIdx & 1) == 0) True else !fill.req.is16Bit
    val locValid = fill.decodeValid(byteIdx)
    val locBank = fill.decodeBank(byteIdx)
    val locPair = fill.decodePair(byteIdx)
    val texel = byteIdx match {
      case 0 => fillRsp.fragment.data(15 downto 0)
      case 2 => fillRsp.fragment.data(31 downto 16)
      case _ => B(0, 16 bits)
    }
    val expanded = byteIdx match {
      case 0 => fillRsp.fragment.data(7 downto 0) ## fillRsp.fragment.data(7 downto 0)
      case 1 => fillRsp.fragment.data(15 downto 8) ## fillRsp.fragment.data(15 downto 8)
      case 2 => fillRsp.fragment.data(23 downto 16) ## fillRsp.fragment.data(23 downto 16)
      case 3 => fillRsp.fragment.data(31 downto 24) ## fillRsp.fragment.data(31 downto 24)
    }
    when(fmt && locValid) {
      val texelData = fill.req.is16Bit ? texel | expanded
      mask(locBank) := True
      pairMask(locBank)(locPair.asUInt) := True
      when(locPair) {
        writeData(locBank)(31 downto 16) := texelData
      } otherwise {
        writeData(locBank)(15 downto 0) := texelData
      }
    }
  }

  for (b <- 0 until bankCount) {
    for (way <- 0 until wayCount) {
      bankMem(b)(way).write(
        bankIndex(fill.set, wordEntry),
        writeData(b),
        fillRsp.fire && mask(b) && fill.way === U(way, wayBits bits)
      )
    }
  }

  val ownerFillCaptureMask = Bits(tapCount bits)
  val ownerFillCommitMask = Bits(tapCount bits)
  val ownerResolvedPotential = Bits(tapCount bits)
  val ownerResolvedNext = Bits(tapCount bits)
  val ownerActiveMask = Bits(tapCount bits)
  for (i <- 0 until tapCount) {
    ownerFillCaptureMask(i) := fillRsp.valid && owner.active && ownerTapActive(i) && !owner
      .resolved(i) && ownerTapLineBase(i) === fill.req.lineBase && ownerTapEntry(
      i
    ) === wordEntry && pairMask(ownerTapBank(i))(ownerTapPair(i).asUInt)
    ownerFillCommitMask(i) := fillRsp.fire && owner.active && ownerTapActive(i) && !owner
      .resolved(i) && ownerTapLineBase(i) === fill.req.lineBase && ownerTapEntry(
      i
    ) === wordEntry && pairMask(ownerTapBank(i))(ownerTapPair(i).asUInt)
    ownerResolvedPotential(i) := owner.resolved(i) || ownerFillCaptureMask(i)
    ownerResolvedNext(i) := owner.resolved(i) || ownerFillCommitMask(i)
    ownerActiveMask(i) := owner.active && ownerTapActive(i)
  }

  val ownerRsp = Tmu.SampleFetch(c)
  ownerRsp.bilinear := owner.sample.bilinear
  ownerRsp.passthrough := owner.sample.passthrough
  for (i <- 0 until tapCount) {
    val nextTexel = Mux(
      ownerFillCaptureMask(i),
      bankTexel(writeData(ownerTapBank(i)), ownerTapPair(i)),
      owner.texels(i)
    )
    ownerRsp.texels(i) := (if (i == 0) nextTexel
                           else Mux(owner.sample.bilinear, nextTexel, B(0, 16 bits)))
  }

  val ownerRspCommitted = Tmu.SampleFetch(c)
  ownerRspCommitted.bilinear := owner.sample.bilinear
  ownerRspCommitted.passthrough := owner.sample.passthrough
  for (i <- 0 until tapCount) {
    ownerRspCommitted.texels(i) := (if (i == 0) owner.texels(i)
                                    else Mux(owner.sample.bilinear, owner.texels(i), B(0, 16 bits)))
  }

  val readySetMeta = cloneOf(fill.setMeta)
  readySetMeta := fill.setMeta
  for (way <- 0 until wayCount) {
    when(fill.way === U(way, wayBits bits)) {
      readySetMeta.ways(way).ready := True
    }
  }

  val continueDone = Vec(Bool(), tapCount)
  val continueDoneNext = Vec(Bool(), tapCount)
  for (i <- 0 until tapCount) {
    continueDone(i) := !owner.active || !ownerTapActive(i) || ownerResolvedPotential(i)
    continueDoneNext(i) := !owner.active || !ownerTapActive(i) || ownerResolvedNext(i)
  }
  val continueIdx = firstFree(continueDone, U(0, 2 bits))
  val continueIdxNext = firstFree(continueDoneNext, U(0, 2 bits))
  val continueDoneAll = continueDone.asBits.andR
  val continueDoneNextAll = continueDoneNext.asBits.andR
  val fillFetchEmit = ownerFillCaptureMask.orR && continueDoneAll
  val continuePayload = Vec.fill(tapCount)(
    TmuTextureCacheFillStart(c, slotBits, setBits, bankEntryWidth, wayBits, wayCount)
  )
  for (i <- 0 until tapCount) {
    val tapSetMeta = TmuTextureCacheSetMeta(c, wayCount)
    tapSetMeta := owner.tapMeta(i)
    continuePayload(i) := buildFillStart(owner.sample, owner.epoch, tapSetMeta, U(i, 2 bits))
  }

  val continueFillStart = Stream(
    TmuTextureCacheFillStart(c, slotBits, setBits, bankEntryWidth, wayBits, wayCount)
  )
  continueFillStart.valid := continuePendingValid
  continueFillStart.payload := continuePayload(continuePendingIdx)

  val fillStart =
    StreamArbiterFactory.lowerFirst.noLock.onArgs(continueFillStart, compareFillStart)

  io.texRead.cmd << fillStart.translateWith {
    val cmd = cloneOf(io.texRead.cmd.payload)
    cmd.fragment.address := fillStart.payload.req.lineBase
    cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
    cmd.fragment.length := fillLength
    cmd.fragment.source := 0
    cmd.last := True
    cmd
  }

  when(fillStart.fire) {
    fill.activeReg := True
    fill.set := fillStart.payload.set
    fill.way := fillStart.payload.way
    fill.slot := fillStart.payload.slot
    fill.setMeta := fillStart.payload.setMeta
    fill.rspCount := 0
    fill.req := fillStart.payload.req
    fill.passthrough := fillStart.payload.passthrough
    metaWriteEnable := True
    metaWriteSet := fillStart.payload.set
    metaWriteData := fillStart.payload.setMeta
    texFillMisses := texFillMisses + 1
    texFillBurstCount := texFillBurstCount + 1
    texFillBurstBeats := texFillBurstBeats + U(lineWords, 32 bits)
    when(fillStart.payload.evictValid) {
      texFillEvictValid := texFillEvictValid + 1
      when(fillStart.payload.evictReady) {
        texFillEvictReady := texFillEvictReady + 1
      } otherwise {
        texFillEvictInflight := texFillEvictInflight + 1
      }
    }
  }

  val startTexEnd = Vec((0 until 9).map { lod =>
    val base = fillStart.payload.req.texTables.texBase(lod)
    val rawEnd = fillStart.payload.req.texTables.texEnd(lod)
    val fallback =
      if (lod < 8) fillStart.payload.req.texTables.texBase(lod + 1)
      else (base + (fillStart.payload.req.is16Bit ? U(2, 22 bits) | U(1, 22 bits))).resized
    (rawEnd > base) ? rawEnd | fallback
  })

  when(fillStart.fire) {
    for (lod <- 0 until 9) {
      fill.texBase(lod) := fillStart.payload.req.texTables.texBase(lod)
      fill.texEnd(lod) := startTexEnd(lod)
      fill.texShift(lod) := fillStart.payload.req.texTables.texShift(lod)
    }
    for (byteIdx <- 0 until 4) {
      fill.decodeValid(byteIdx) := fillStart.payload.req.startupDecode.valid(byteIdx)
      fill.decodeBank(byteIdx) := fillStart.payload.req.startupDecode.bank(byteIdx)
      fill.decodePair(byteIdx) := fillStart.payload.req.startupDecode.pair(byteIdx)
    }
  }

  when(continueFillStart.fire) {
    continuePendingValid := False
    for (i <- 0 until tapCount) {
      when(ownerTapSet(i) === continueFillStart.payload.set) {
        owner.tapMeta(i) := continueFillStart.payload.setMeta
      }
    }
  }

  fillRsp.ready := True
  fillFetchRsp.valid := fillFetchPendingValid
  fillFetchRsp.payload := ownerRspCommitted

  when(fillRsp.fire && fillFetchEmit) {
    fillFetchPendingValid := True
  }
  when(fillFetchPendingValid && io.sampleFetch.ready) {
    fillFetchPendingValid := False
  }

  when(fillRsp.fire) {
    val nextWordAddr =
      (fill.req.lineBase + ((fill.rspCount + 1) << 2).resized).resize(c.addressWidth.value bits)
    for (byteIdx <- 0 until 4) {
      val nextAddr = (nextWordAddr + byteIdx).resize(c.addressWidth.value bits)
      val loc = packedLocation(nextAddr)
      fill.decodeValid(byteIdx) := loc._1
      fill.decodeBank(byteIdx) := loc._2
      fill.decodePair(byteIdx) := loc._3
    }
    for (i <- 0 until tapCount) {
      when(ownerFillCommitMask(i)) {
        owner.texels(i) := bankTexel(writeData(ownerTapBank(i)), ownerTapPair(i))
      }
    }
    owner.resolved := ownerResolvedNext
  }

  when(fillRsp.fire) {
    fill.rspCount := fill.rspCount + 1
    when(fill.req.bankEntry === wordEntry && pairMask(fill.req.bankSel).orR) {
      texFillStallCycles := texFillStallCycles + 1
    }
    when(fillRsp.last) {
      fill.activeReg := False
      fill.setMeta := readySetMeta
      metaWriteEnable := True
      metaWriteSet := fill.set
      metaWriteData := readySetMeta
      continuePendingValid := !continueDoneNextAll
      continuePendingIdx := continueIdxNext
      for (i <- 0 until tapCount) {
        when(ownerTapSet(i) === fill.set) {
          owner.tapMeta(i) := readySetMeta
        }
      }
      when(continueDoneNextAll) {
        owner.active := False
      }
    }
  }

  when(io.invalidate) {
    currentEpoch := currentEpoch + 1
  }

  when(!formalHotInitDone) {
    metaWriteEnable := True
    metaWriteSet := 0
    metaWriteData := hotMeta()
    formalHotInitDone := True
  }

  when(initSweepActive) {
    val clearSet = TmuTextureCacheSetMeta(c, wayCount)
    for (way <- 0 until wayCount) {
      clearSet.ways(way) := zeroMeta()
    }
    clearSet.victim := 0
    val sweepSet = initSweepSet
    metaWriteEnable := True
    metaWriteSet := sweepSet
    metaWriteData := clearSet
    when(initSweepSet === U(setCount - 1, setBits bits)) {
      initSweepActive := False
    } otherwise {
      initSweepSet := initSweepSet + 1
    }
  }

  hitRsp.bilinear := compareStage.sample.bilinear
  hitRsp.passthrough := compareStage.sample.passthrough
  for (i <- 0 until tapCount) {
    val texel = bankTexel(compareStage.tapData(i), packedPairSel(compareStage.sample, i))
    hitRsp.texels(i) := (if (i == 0) texel
                         else Mux(compareStage.sample.bilinear, texel, B(0, 16 bits)))
  }

  when(fillFetchRsp.valid) {
    io.sampleFetch.valid := True
    io.sampleFetch.payload := fillFetchRsp.payload
    fillFetchRsp.ready := io.sampleFetch.ready
  } otherwise {
    io.sampleFetch.valid := compareValid && compareStage.allHits
    io.sampleFetch.payload := hitRsp
  }

  io.busy := reqStream.valid || lookupRspValid || compareValid || owner.active || fill.activeReg || io.sampleFetch.valid || initSweepActive

  GenerationFlags.formal {
    when(io.texRead.cmd.valid) {
      assert(io.texRead.cmd.fragment.opcode === Bmb.Cmd.Opcode.READ)
      assert(io.texRead.cmd.last)
    }
    if (formalStrong) {
      cover(compareConsumeHit)
      cover(compareConsumeMiss)
    }
  }
}

case class TmuNoTextureCache(override val c: voodoo.Config) extends TmuTexturePathBase(c) {
  require(!c.useTexFillCache)

  val reqStream = io.sampleRequest.queue(c.texFillRequestWindow)

  def pendingFirst(mask: Bits, fallback: UInt): UInt = new Composite(mask) {
    val idx = UInt(2 bits)
    idx := fallback.resized
    when(mask(0)) {
      idx := 0
    } elsewhen (mask(1)) {
      idx := 1
    } elsewhen (mask(2)) {
      idx := 2
    } elsewhen (mask(3)) {
      idx := 3
    }
  }.idx

  val expanded = new Area {
    val stream = Stream(TmuTextureCacheDirectWordReq(c))
    val running = RegInit(False)
    val hold = Reg(Tmu.SampleRequest(c))
    val pending = Reg(Bits(4 bits)) init 0

    val req = running ? hold | reqStream.payload
    val initPending = Mux(reqStream.payload.bilinear, B"1111", B"0001")
    val activePending = running ? pending | initPending

    val tapAddrs = Vec(req.pointAddr, req.biAddr1, req.biAddr2, req.biAddr3)
    when(req.bilinear) {
      tapAddrs(0) := req.biAddr0
    }
    val tapWordAddrs = Vec.fill(4)(UInt(c.addressWidth.value bits))
    for (i <- 0 until 4) {
      tapWordAddrs(i) := ((tapAddrs(i) >> 2) << 2).resize(c.addressWidth.value bits)
      stream.payload.tapHalf(i) := tapAddrs(i)(1)
      stream.payload.tapByte(i) := tapAddrs(i)(0)
    }

    val currentIdx = pendingFirst(activePending, U(0, 2 bits))
    val currentWord = tapWordAddrs(currentIdx)
    val currentMask = Bits(4 bits)
    val nextPending = Bits(4 bits)
    for (i <- 0 until 4) {
      currentMask(i) := activePending(i) && tapWordAddrs(i) === currentWord
      nextPending(i) := activePending(i) && !(tapWordAddrs(i) === currentWord)
    }

    stream.valid := running || reqStream.valid
    stream.payload.address := currentWord
    stream.payload.is16Bit := req.is16Bit
    stream.payload.firstWord := !running
    stream.payload.lastWord := nextPending === 0
    stream.payload.bilinear := req.bilinear
    stream.payload.tapMask := currentMask
    stream.payload.passthrough := req.passthrough

    reqStream.ready := !running && stream.ready

    when(stream.fire) {
      when(!running && !stream.payload.lastWord) {
        hold := reqStream.payload
        pending := nextPending
        running := True
      } elsewhen (running && !stream.payload.lastWord) {
        pending := nextPending
      } elsewhen (running) {
        running := False
        pending := 0
      }
    }
  }

  val (toMem, toQ) = StreamFork2(expanded.stream)
  io.texRead.cmd << toMem.translateWith {
    val cmd = cloneOf(io.texRead.cmd.payload)
    cmd.fragment.address := toMem.payload.address
    cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
    cmd.fragment.length := 3
    cmd.fragment.source := 0
    cmd.last := True
    cmd
  }

  val queued = toQ.queue(16)
  val rsp = io.texRead.rsp.takeWhen(io.texRead.rsp.last).translateWith(io.texRead.rsp.fragment.data)
  val raw = StreamJoin(rsp, queued)

  val assembled = Reg(Tmu.SampleFetch(c))
  val outValid = RegInit(False)

  raw.ready := !outValid

  when(raw.fire) {
    val meta = raw.payload._2

    when(meta.firstWord) {
      assembled.bilinear := meta.bilinear
      assembled.passthrough := meta.passthrough
      for (i <- 0 until 4) {
        assembled.texels(i) := 0
      }
    }

    for (i <- 0 until 4) {
      when(meta.tapMask(i)) {
        assembled
          .texels(i) := texel16(raw.payload._1, meta.tapHalf(i), meta.tapByte(i), meta.is16Bit)
      }
    }

    when(meta.lastWord) {
      outValid := True
    }
  }

  io.sampleFetch.valid := outValid
  io.sampleFetch.payload := assembled
  when(io.sampleFetch.fire) {
    outValid := False
  }
  io.busy := reqStream.valid || expanded.running || queued.valid || outValid

  GenerationFlags.formal {
    when(io.texRead.cmd.valid) {
      assert(io.texRead.cmd.fragment.opcode === Bmb.Cmd.Opcode.READ)
      assert(io.texRead.cmd.last)
      assert(io.texRead.cmd.fragment.length === 3)
    }
  }
}
