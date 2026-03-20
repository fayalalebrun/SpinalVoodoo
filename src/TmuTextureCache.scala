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
    val invalidate = in Bool ()
    val busy = out Bool ()
  }

  def withTrace(x: Data*): Unit = if (c.trace.enabled) x.foreach(_.simPublic())
  def queuedOf(pass: Tmu.TmuPassthrough, addr: UInt) = new Composite(addr) {
    val q = Tmu.QueuedData(c); q.passthrough := pass; q.fullAddress := addr; q.addrHalf := addr(1);
    q.addrByte := addr(0)
  }.q
  def cacheIndex(slot: UInt, entry: UInt, slotBits: Int, entryBits: Int): UInt = (((slot.resize(
    slotBits bits
  ) ## U(0, entryBits bits)) | entry.resize(slotBits + entryBits bits).asBits).asUInt)
  def firstFree(valid: Vec[Bool], fallback: UInt): UInt = new Composite(valid) {
    val hit = Bits(valid.length bits); for (i <- valid.indices) hit(i) := !valid(i);
    val idx = hit.orR ? OHToUInt(hit) | fallback
  }.idx

  val texFillHits = Reg(UInt(32 bits)) init 0
  val texFillMisses = Reg(UInt(32 bits)) init 0
  val texFillBurstCount = Reg(UInt(32 bits)) init 0
  val texFillBurstBeats = Reg(UInt(32 bits)) init 0
  val texFillStallCycles = Reg(UInt(32 bits)) init 0
  val texFastBilinearHits = Reg(UInt(32 bits)) init 0
  withTrace(
    texFillHits,
    texFillMisses,
    texFillBurstCount,
    texFillBurstBeats,
    texFillStallCycles,
    texFastBilinearHits
  )

  val fastHoldValid = RegInit(False)
  val fastHoldPayload = Reg(Tmu.FastFetch(c))
  fastHoldPayload := fastHoldPayload
  io.fastFetch.valid := fastHoldValid; io.fastFetch.payload := fastHoldPayload
  when(io.fastFetch.fire) { fastHoldValid := False }

  val joined = Stream(Tmu.FetchResult(c))
  val useFastBilinear, fetchBusy = Bool()

  val expanded = new Area {
    val stream = Stream(Tmu.TmuExpanded(c))
    val running = RegInit(False)
    val tap = Reg(UInt(2 bits)) init 0
    val hold = Reg(Tmu.SampleRequest(c))
    val req = running ? hold | io.sampleRequest.payload
    val idx = running ? tap | U(0, 2 bits)
    val biAddr = Vec(req.biAddr0, req.biAddr1, req.biAddr2, req.biAddr3)
    val biBank = Vec(req.biBankSel0, req.biBankSel1, req.biBankSel2, req.biBankSel3)
    val pass = Tmu.TmuPassthrough(c)
    pass.format := req.passthrough.format; pass.bilinear := req.passthrough.bilinear;
    pass.sendConfig := req.passthrough.sendConfig
    pass.ds := req.passthrough.ds; pass.dt := req.passthrough.dt; pass.readIdx := idx;
    pass.requestId := req.passthrough.requestId; pass.ncc := req.passthrough.ncc
    if (c.trace.enabled) pass.trace := req.passthrough.trace
    stream.valid := running || (io.sampleRequest.valid && !useFastBilinear)
    stream.payload.address := req.bilinear ? biAddr(idx) | req.pointAddr
    stream.payload.bankSel := req.bilinear ? biBank(idx) | req.pointBankSel
    stream.payload.lodBase := req.lodBase; stream.payload.lodEnd := req.lodEnd;
    stream.payload.lodShift := req.lodShift; stream.payload.is16Bit := req.is16Bit
    if (c.packedTexLayout) stream.payload.texTables := req.texTables
    stream.payload.passthrough := pass
    when(stream.fire) {
      when(!running && io.sampleRequest.payload.bilinear) {
        hold := io.sampleRequest.payload; running := True; tap := 1
      }
        .elsewhen(running && tap =/= 3) { tap := tap + 1 }
        .elsewhen(running) { running := False }
    }
  }

  if (c.useTexFillCache) {
    require(c.texFillLineWords > 0 && ((c.texFillLineWords & (c.texFillLineWords - 1)) == 0))
    require(c.texFillLineWords >= 4 && (c.texFillLineWords % 4) == 0)
    require((c.texFillLineWords * 4 - 1) < (1 << c.memBurstLengthWidth))
    require(c.texFillCacheSlots > 0 && c.texFillRequestWindow > 0)

    val lineWords = c.texFillLineWords; val lineBytes = lineWords * 4;
    val lineShift = log2Up(lineWords); val lineByteShift = log2Up(lineBytes)
    val bankCount = 4; val bankEntries = lineWords; val bankEntryWidth = log2Up(bankEntries)
    val slotCount = c.texFillCacheSlots; val slotBits = scala.math.max(1, log2Up(slotCount));
    val fillLength = U(lineWords * 4 - 1, c.memBurstLengthWidth bits)

    val reqStream = expanded.stream
      .translateWith {
        val e = expanded.stream.payload; val req = Tmu.CachedReq(c, bankEntryWidth)
        val lineBase =
          ((e.address >> lineByteShift) << lineByteShift).resize(c.addressWidth.value bits)
        val wordAddr = (e.address >> 2).resize(c.addressWidth.value bits);
        val wordIdx = wordAddr(lineShift - 1 downto 0)
        req.lineBase := lineBase; req.lodBase := e.lodBase; req.lodEnd := e.lodEnd;
        req.lodShift := e.lodShift; req.is16Bit := e.is16Bit
        if (c.packedTexLayout) req.texTables := e.texTables
        if (c.packedTexLayout) {
          req.bankSel := e.bankSel
          req.bankEntry := ((e.address - lineBase) >> 2).resize(bankEntryWidth bits)
        } else {
          req.bankSel := wordIdx(1 downto 0)
          req.bankEntry := wordIdx.resize(bankEntryWidth bits)
        }
        req.queued := queuedOf(e.passthrough, e.address); req
      }
      .queue(c.texFillRequestWindow)

    val cache = new Area {
      val slotValid = Vec(Reg(Bool()) init False, slotCount)
      val slotBase = Vec(Reg(UInt(c.addressWidth.value bits)) init 0, slotCount)
      val slotWordValid = Vec(
        (0 until slotCount).map(_ =>
          Vec(
            (0 until bankCount).map(_ =>
              Vec((0 until bankEntries).map(_ => Reg(Bool()) init False))
            )
          )
        )
      )
      val bankMem = Seq.fill(bankCount)(Mem(Bits(32 bits), slotCount * bankEntries))
      val bankRead = bankMem.map(_.readSyncPort(readUnderWrite = readFirst))
      val nextVictim = Reg(UInt(slotBits bits)) init 0
      def hitVec(base: UInt, bank: UInt, entry: UInt, enable: Bool): Bits = {
        val hit = Bits(slotCount bits);
        for (i <- 0 until slotCount)
          hit(i) := enable && slotValid(i) && slotBase(i) === base && slotWordValid(i)(bank)(entry);
        hit
      }
      def alloc(slot: UInt, base: UInt): Unit = {
        slotValid(slot) := True; slotBase(slot) := base;
        for (b <- 0 until bankCount; e <- 0 until bankEntries) slotWordValid(slot)(b)(e) := False
      }
      def bankRsp(sel: UInt): Bits = Vec(bankRead.map(_.rsp))(sel)
    }

    val active = reqStream.payload;
    val hitVec = cache.hitVec(active.lineBase, active.bankSel, active.bankEntry, reqStream.valid)
    val hitAny = hitVec.orR; val hitSlot = OHToUInt(hitVec)
    val hitReadIssued = RegInit(False); val hitReadBank = Reg(UInt(2 bits)) init 0;
    val hitReadQueued = Reg(Tmu.QueuedData(c))
    val hitRspValid = RegInit(False); val hitRspData = Reg(Bits(32 bits)) init 0;
    val hitRspQueued = Reg(Tmu.QueuedData(c))
    for (port <- cache.bankRead) { port.cmd.valid := False; port.cmd.payload := 0 }

    def tapAddr(i: Int): UInt = Seq(
      io.sampleRequest.payload.biAddr0,
      io.sampleRequest.payload.biAddr1,
      io.sampleRequest.payload.biAddr2,
      io.sampleRequest.payload.biAddr3
    )(i)
    def tapBank(i: Int): UInt = Seq(
      io.sampleRequest.payload.biBankSel0,
      io.sampleRequest.payload.biBankSel1,
      io.sampleRequest.payload.biBankSel2,
      io.sampleRequest.payload.biBankSel3
    )(i)
    case class Lookup(
        lineBase: UInt,
        bankSel: UInt,
        bankEntry: UInt,
        hit: Bits,
        slot: UInt,
        ok: Bool
    )
    def lookup(addr: UInt, bankSel: UInt) = {
      val lineBase = ((addr >> lineByteShift) << lineByteShift).resize(c.addressWidth.value bits)
      val bankEntry = ((addr - lineBase) >> 2).resize(bankEntryWidth bits)
      val hit = cache.hitVec(lineBase, bankSel, bankEntry, True)
      Lookup(lineBase, bankSel, bankEntry, hit, OHToUInt(hit), hit.orR)
    }

    reqStream.ready := False
    when(hitReadIssued) {
      hitRspValid := True; hitRspData := cache.bankRsp(hitReadBank); hitRspQueued := hitReadQueued;
      hitReadIssued := False
    }

    val fill = new Area {
      val activeReg = RegInit(False); val slot = Reg(UInt(slotBits bits)) init 0;
      val cmdIssued = RegInit(False); val rspCount = Reg(UInt(log2Up(lineWords + 1) bits)) init 0
      val req = Reg(Tmu.CachedReq(c, bankEntryWidth))
      val rspValid = RegInit(False); val rspData = Reg(Bits(32 bits)) init 0;
      val rspQueued = Reg(Tmu.QueuedData(c))
      val startSlot = firstFree(cache.slotValid, cache.nextVictim)
      val start = reqStream.valid && !hitAny && !activeReg
      when(start) {
        reqStream.ready := True; activeReg := True; slot := startSlot; cmdIssued := False;
        rspCount := 0; req := active; cache.alloc(startSlot, active.lineBase);
        cache.nextVictim := cache.nextVictim + 1; texFillMisses := texFillMisses + 1;
        texFillBurstCount := texFillBurstCount + 1;
        texFillBurstBeats := texFillBurstBeats + U(lineWords, 32 bits)
      }
      when(reqStream.valid && !hitAny) { texFillStallCycles := texFillStallCycles + 1 }
      io.texRead.cmd.valid := activeReg && !cmdIssued;
      io.texRead.cmd.fragment.address := cache.slotBase(slot);
      io.texRead.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ;
      io.texRead.cmd.fragment.length := fillLength; io.texRead.cmd.fragment.source := 0;
      io.texRead.cmd.last := True
      when(io.texRead.cmd.fire) { cmdIssued := True }
      io.texRead.rsp.ready := activeReg
      val wordAddr =
        (cache.slotBase(slot) + (rspCount << 2).resized).resize(c.addressWidth.value bits);
      val wordEntry = rspCount.resize(bankEntryWidth bits)
      def packedBank(addr: UInt): (Bool, UInt) = {
        val in = Bool(); val base = UInt(22 bits); val shift = UInt(4 bits); in := False; base := 0;
        shift := 0
        for (lod <- 0 until 9) {
          val b = req.texTables.texBase(lod).resize(c.addressWidth.value bits)
          val rawEnd = req.texTables.texEnd(lod).resize(c.addressWidth.value bits)
          val fallback =
            if (lod < 8) req.texTables.texBase(lod + 1).resize(c.addressWidth.value bits)
            else
              (b + (req.is16Bit ? U(2, c.addressWidth.value bits) | U(
                1,
                c.addressWidth.value bits
              ))).resized
          val end = (rawEnd > b) ? rawEnd | fallback
          when(addr >= b && addr < end) {
            in := True; base := req.texTables.texBase(lod); shift := req.texTables.texShift(lod)
          }
        }
        val offs = (addr.resize(22 bits) - base).resize(22 bits);
        val x = req.is16Bit ? offs(1) | offs(0);
        val y = offs.asBits((shift +^ req.is16Bit.asUInt.resize(4 bits)).resize(4 bits));
        (in, (y ## x).asUInt)
      }
      def bankMask: Bits = {
        val mask = Bits(bankCount bits); mask := 0
        if (c.packedTexLayout) for (byteIdx <- 0 until 4) {
          val addr = (wordAddr + byteIdx).resize(c.addressWidth.value bits);
          val fmt = if ((byteIdx & 1) == 0) True else !req.is16Bit; val p = packedBank(addr);
          when(fmt && p._1) { mask(p._2) := True }
        }
        else { mask(rspCount.resize(lineShift bits)(1 downto 0)) := True }
        mask
      }
      val mask = bankMask
      for (b <- 0 until bankCount)
        cache
          .bankMem(b)
          .write(
            cacheIndex(slot, wordEntry, slotBits, bankEntryWidth),
            io.texRead.rsp.fragment.data,
            io.texRead.rsp.fire && mask(b)
          )
      when(io.texRead.rsp.fire) {
        for (b <- 0 until bankCount) when(mask(b)) {
          cache.slotWordValid(slot)(b)(wordEntry) := True
        }
        when(wordEntry === req.bankEntry && !rspValid) {
          rspValid := True; rspData := io.texRead.rsp.fragment.data; rspQueued := req.queued
        }
        rspCount := rspCount + 1
        when(io.texRead.rsp.last) { activeReg := False }
      }
    }

    val fast = new Area {
      val issued = RegInit(False); val pass = Reg(Tmu.TmuPassthrough(c))
      val bank = Vec(Reg(UInt(2 bits)) init 0, 4); val half = Vec(Reg(Bool()) init False, 4);
      val byte = Vec(Reg(Bool()) init False, 4)
      val looks = Seq.tabulate(4)(i => lookup(tapAddr(i), tapBank(i)))
      val hitAll = looks.map(_.ok).reduce(_ && _)
      val needed = Vec(
        (0 until bankCount).map(b => looks.map(_.bankSel === U(b, 2 bits)).reduce(_ || _))
      )
      val slot = Vec((0 until bankCount).map { b =>
        MuxOH(looks.map(_.bankSel === U(b, 2 bits)).toIndexedSeq, looks.map(_.slot))
      })
      val entry = Vec((0 until bankCount).map { b =>
        MuxOH(looks.map(_.bankSel === U(b, 2 bits)).toIndexedSeq, looks.map(_.bankEntry))
      })
      val conflict = Bool(); conflict := False
      for (i <- 0 until 4; j <- i + 1 until 4)
        when(
          looks(i).bankSel === looks(j).bankSel && (looks(i).slot =/= looks(j).slot || looks(
            i
          ).bankEntry =/= looks(j).bankEntry)
        ) { conflict := True }
      val canIssue =
        if (c.packedTexLayout)
          io.sampleRequest.valid && io.sampleRequest.payload.bilinear && hitAll && !conflict && !expanded.running && !reqStream.valid && !hitReadIssued && !hitRspValid && !((issued || fastHoldValid) && !io.fastFetch.ready) && io.outputRoute.ready
        else False
      when(canIssue) {
        for (b <- 0 until bankCount) {
          cache.bankRead(b).cmd.valid := needed(b);
          cache.bankRead(b).cmd.payload := cacheIndex(slot(b), entry(b), slotBits, bankEntryWidth)
        }
        for (i <- 0 until 4) {
          bank(i) := tapBank(i); half(i) := tapAddr(i)(1); byte(i) := tapAddr(i)(0)
        }
        pass := io.sampleRequest.payload.passthrough
      }
      when(issued) {
        fastHoldValid := True; fastHoldPayload.passthrough := pass
        for (i <- 0 until 4) {
          fastHoldPayload.texels(i).rspData32 := cache.bankRsp(bank(i));
          fastHoldPayload.texels(i).addrHalf := half(i);
          fastHoldPayload.texels(i).addrByte := byte(i)
        }
        texFastBilinearHits := texFastBilinearHits + 1;
        texFillHits := texFillHits + U(4, 32 bits)
      }
      issued := canIssue
    }

    val canIssueHit = reqStream.valid && hitAny && !hitReadIssued && !hitRspValid && !fill.activeReg
    when(canIssueHit) {
      cache.bankRead(active.bankSel).cmd.valid := True
      cache.bankRead(active.bankSel).cmd.payload := cacheIndex(
        hitSlot,
        active.bankEntry,
        slotBits,
        bankEntryWidth
      )
      reqStream.ready := True
      hitReadIssued := True
      hitReadBank := active.bankSel
      hitReadQueued := active.queued
    }

    joined.valid := fill.rspValid || hitRspValid
    joined.payload.rspData32 := fill.rspValid ? fill.rspData | hitRspData
    joined.payload.queued := fill.rspValid ? fill.rspQueued | hitRspQueued
    when(joined.fire) {
      when(fill.rspValid) {
        fill.rspValid := False
      } otherwise {
        hitRspValid := False
        texFillHits := texFillHits + 1
      }
    }

    when(io.invalidate) {
      for (i <- 0 until slotCount) when(!fill.activeReg || fill.slot =/= i) {
        cache.slotValid(i) := False
      }
    }
    if (c.packedTexLayout) useFastBilinear := fast.canIssue else useFastBilinear := False
    fetchBusy := reqStream.valid || fill.activeReg || hitReadIssued || hitRspValid || fast.issued || fastHoldValid
    GenerationFlags.formal {
      assert(io.texRead.cmd.valid === (fill.activeReg && !fill.cmdIssued))
      when(io.texRead.cmd.valid) {
        assert(io.texRead.cmd.fragment.opcode === Bmb.Cmd.Opcode.READ);
        assert(io.texRead.cmd.fragment.address === cache.slotBase(fill.slot));
        assert(io.texRead.cmd.fragment.length === fillLength); assert(io.texRead.cmd.last)
      }
      if (formalStrong) {
        val prevStart = RegNext(fill.start) init False;
        val prevSlot = RegNext(fill.startSlot) init 0;
        val prevBase = RegNext(active.lineBase) init 0;
        val prevInv = RegNext(io.invalidate) init False
        when(prevStart && !prevInv) {
          assert(cache.slotValid(prevSlot)); assert(cache.slotBase(prevSlot) === prevBase)
        }
      }
      cover(fill.start); cover(hitAny); if (c.packedTexLayout) cover(fast.canIssue)
    }
  } else {
    useFastBilinear := False; fetchBusy := False
    val (toMem, toQ) = StreamFork2(expanded.stream)
    io.texRead.cmd << toMem.translateWith {
      val cmd = Fragment(BmbCmd(Tmu.bmbParams(c))); cmd.fragment.address := toMem.payload.address;
      cmd.fragment.opcode := Bmb.Cmd.Opcode.READ; cmd.fragment.length := 3;
      cmd.fragment.source := 0; cmd.last := True; cmd
    }
    val queued = toQ.map(e => queuedOf(e.passthrough, e.address)).queue(16)
    val rsp =
      io.texRead.rsp.takeWhen(io.texRead.rsp.last).translateWith(io.texRead.rsp.fragment.data)
    val raw = StreamJoin(rsp, queued)
    joined << raw.translateWith {
      val r = Tmu.FetchResult(c); r.rspData32 := raw.payload._1; r.queued := raw.payload._2; r
    }
  }

  val acceptSlow = !expanded.running && !useFastBilinear && expanded.stream.ready
  val acceptFast = useFastBilinear && (!fastHoldValid || io.fastFetch.ready)
  val canAccept = acceptSlow || acceptFast
  io.outputRoute.valid := io.sampleRequest.valid && canAccept;
  io.outputRoute.payload := useFastBilinear;
  io.sampleRequest.ready := io.outputRoute.ready && canAccept
  io.fetched << joined; io.busy := fetchBusy
}
