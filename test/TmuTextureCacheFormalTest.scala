//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.formal._

class TmuTextureCacheFormalDut extends Component {
  val c = Config
    .voodoo1(TraceConfig(false))
    .copy(
      addressWidth = 5 bits,
      memBurstLengthWidth = 4,
      useTexFillCache = true,
      texFillLineWords = 4,
      texFillCacheSlots = 2,
      texFillRequestWindow = 1,
      packedTexLayout = true
    )

  val dut = TmuTextureCache(c, formalStrong = false)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init False

  def initReq(req: Tmu.SampleRequest, addr: Int, reqId: Int): Unit = {
    req.pointAddr := U(addr, c.addressWidth.value bits)
    req.biAddr0 := U(addr, c.addressWidth.value bits)
    req.biAddr1 := U(addr, c.addressWidth.value bits)
    req.biAddr2 := U(addr, c.addressWidth.value bits)
    req.biAddr3 := U(addr, c.addressWidth.value bits)
    req.pointBankSel := 0
    req.biBankSel0 := 0
    req.biBankSel1 := 0
    req.biBankSel2 := 0
    req.biBankSel3 := 0
    req.lodBase := 0
    req.lodEnd := U((1 << c.addressWidth.value) - 1, c.addressWidth.value bits)
    req.lodShift := 0
    req.is16Bit := True
    req.bilinear := False
    req.passthrough.format := Tmu.TextureFormat.RGB565
    req.passthrough.bilinear := False
    req.passthrough.nccTableSelect := False
    req.passthrough.ds := 0
    req.passthrough.dt := 0
    req.passthrough.readIdx := 0
    req.passthrough.requestId := U(reqId, Tmu.requestIdWidth bits)
    for (lod <- 0 until 9) {
      val base = if (lod == 0) 0 else (1 << c.addressWidth.value) - 1
      val end = if (lod == 0) (1 << c.addressWidth.value) else (1 << c.addressWidth.value) - 1
      req.texTables.texBase(lod) := U(base, 22 bits)
      req.texTables.texEnd(lod) := U(end, 22 bits)
      req.texTables.texShift(lod) := 0
    }
  }

  val trackedReq = Tmu.SampleRequest(c)
  initReq(trackedReq, addr = 0x000, reqId = 0)

  val accepted = RegInit(False)
  val completed = RegInit(False)
  val seenMiss = RegInit(False)
  val seenCmd = RegInit(False)
  val seenRsp = RegInit(False)
  val seenLastRsp = RegInit(False)

  dut.io.sampleRequest.valid := !reset && !accepted
  dut.io.sampleRequest.payload := trackedReq
  dut.io.sampleFetch.ready := !reset
  dut.io.invalidate := False
  dut.io.texRead.cmd.ready := !reset

  val burstActive = RegInit(False)
  val burstCount = Reg(UInt(log2Up(c.texFillLineWords) bits)) init 0
  val burstSeed = Reg(UInt(scala.math.min(8, c.addressWidth.value) bits)) init 0

  dut.io.texRead.rsp.valid := burstActive
  dut.io.texRead.rsp.last := burstCount === U(c.texFillLineWords - 1, burstCount.getWidth bits)
  dut.io.texRead.rsp.fragment.source := 0
  dut.io.texRead.rsp.fragment.context := 0
  dut.io.texRead.rsp.fragment.data := (B(
    0,
    32 - burstSeed.getWidth bits
  ) ## burstSeed.asBits) ^ burstCount.asBits.resize(32 bits)

  assumeInitial(reset)
  when(reset) {
    assume(!burstActive)
  }
  when(pastValid) {
    assume(!reset)
  }

  when(dut.io.sampleRequest.fire) {
    accepted := True
  }
  when(dut.io.sampleFetch.fire) {
    completed := True
  }
  when(dut.io.texRead.cmd.fire) {
    seenMiss := True
  }
  when(dut.io.texRead.cmd.fire) {
    seenCmd := True
  }
  when(dut.io.texRead.rsp.fire) {
    seenRsp := True
  }
  when(dut.io.texRead.rsp.fire && dut.io.texRead.rsp.last) {
    seenLastRsp := True
  }

  when(dut.io.texRead.cmd.fire) {
    burstActive := True
    burstCount := 0
    burstSeed := dut.io.texRead.cmd.fragment.address.resize(burstSeed.getWidth bits)
  } elsewhen (dut.io.texRead.rsp.fire) {
    when(dut.io.texRead.rsp.last) {
      burstActive := False
    } otherwise {
      burstCount := burstCount + 1
    }
  }

  when(dut.io.texRead.cmd.valid) {
    assert(dut.io.texRead.cmd.fragment.opcode === Bmb.Cmd.Opcode.READ)
    assert(dut.io.texRead.cmd.last)
  }

  assert(dut.io.sampleRequest.valid === (!reset && !accepted))
  assert(dut.io.sampleFetch.ready === !reset)
  assert(dut.io.texRead.cmd.ready === !reset)
  assert(dut.io.texRead.rsp.valid === burstActive)
  when(accepted) {
    assert(!dut.io.sampleRequest.valid)
  }

  cover(accepted)
  cover(seenMiss)
  cover(seenCmd)
  cover(seenRsp)
  cover(seenLastRsp)
  cover(completed)
}

class TmuTextureCacheHitThroughputFormalDut(bilinear: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig(false))
    .copy(
      addressWidth = 5 bits,
      memBurstLengthWidth = 4,
      useTexFillCache = true,
      texFillLineWords = 4,
      texFillCacheSlots = 2,
      texFillRequestWindow = 1,
      packedTexLayout = true
    )

  val dut = TmuTextureCache(c, formalStrong = false, formalHotInit = true)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init False

  def initReq(req: Tmu.SampleRequest, addr: Int, reqId: Int): Unit = {
    req.pointAddr := U(addr, c.addressWidth.value bits)
    req.biAddr0 := U(addr, c.addressWidth.value bits)
    req.biAddr1 := U(addr, c.addressWidth.value bits)
    req.biAddr2 := U(addr, c.addressWidth.value bits)
    req.biAddr3 := U(addr, c.addressWidth.value bits)
    req.pointBankSel := 0
    req.biBankSel0 := 0
    req.biBankSel1 := (if (bilinear) 1 else 0)
    req.biBankSel2 := (if (bilinear) 2 else 0)
    req.biBankSel3 := (if (bilinear) 3 else 0)
    req.lodBase := 0
    req.lodEnd := U((1 << c.addressWidth.value) - 1, c.addressWidth.value bits)
    req.lodShift := 0
    req.is16Bit := True
    req.bilinear := (if (bilinear) True else False)
    req.passthrough.format := Tmu.TextureFormat.RGB565
    req.passthrough.bilinear := (if (bilinear) True else False)
    req.passthrough.nccTableSelect := False
    req.passthrough.ds := 0
    req.passthrough.dt := 0
    req.passthrough.readIdx := 0
    req.passthrough.requestId := U(reqId, Tmu.requestIdWidth bits)
    for (lod <- 0 until 9) {
      val base = if (lod == 0) 0 else (1 << c.addressWidth.value) - 1
      val end = if (lod == 0) (1 << c.addressWidth.value) else (1 << c.addressWidth.value) - 1
      req.texTables.texBase(lod) := U(base, 22 bits)
      req.texTables.texEnd(lod) := U(end, 22 bits)
      req.texTables.texShift(lod) := 0
    }
  }

  val hotReq = Tmu.SampleRequest(c)
  initReq(hotReq, addr = 0x000, reqId = 2)

  val acceptedCount = Reg(UInt(2 bits)) init 0
  val consecutiveFetches = RegInit(False)
  val consecutiveAccepts = RegInit(False)

  dut.io.sampleRequest.valid := pastValid && acceptedCount =/= 2
  dut.io.sampleRequest.payload := hotReq
  dut.io.sampleFetch.ready := !reset
  dut.io.invalidate := False
  dut.io.texRead.cmd.ready := !reset
  dut.io.texRead.rsp.valid := False
  dut.io.texRead.rsp.last := False
  dut.io.texRead.rsp.fragment.source := 0
  dut.io.texRead.rsp.fragment.context := 0
  dut.io.texRead.rsp.fragment.data := 0

  assumeInitial(reset)
  when(pastValid) {
    assume(!reset)
  }

  when(dut.io.sampleRequest.fire) {
    acceptedCount := acceptedCount + 1
  }

  when(pastValid && dut.io.sampleRequest.fire && past(dut.io.sampleRequest.fire)) {
    consecutiveAccepts := True
  }
  when(pastValid && dut.io.sampleFetch.fire && past(dut.io.sampleFetch.fire)) {
    consecutiveFetches := True
  }

  assert(!dut.io.texRead.cmd.valid)
  cover(consecutiveAccepts)
  cover(consecutiveFetches)
}

class TmuTextureCacheFormalTest extends SpinalFormalFunSuite {
  test("TMU texture cache tracked-request proof") {
    FormalConfig
      .withBMC(40)
      .withProve(40)
      .withAsync
      .doVerify(new TmuTextureCacheFormalDut)
  }

  test("TMU texture cache tracked-request covers") {
    FormalConfig
      .withCover(36)
      .withAsync
      .doVerify(new TmuTextureCacheFormalDut)
  }

  test("TMU texture cache point hit throughput covers") {
    FormalConfig
      .withCover(48)
      .withAsync
      .doVerify(new TmuTextureCacheHitThroughputFormalDut(bilinear = false))
  }

  test("TMU texture cache bilinear hit throughput covers") {
    FormalConfig
      .withCover(48)
      .withAsync
      .doVerify(new TmuTextureCacheHitThroughputFormalDut(bilinear = true))
  }
}
