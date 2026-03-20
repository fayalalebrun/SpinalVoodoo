//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.formal._

class TmuTextureCacheFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 12 bits,
      memBurstLengthWidth = 5,
      useTexFillCache = true,
      texFillLineWords = 4,
      texFillCacheSlots = 2,
      texFillRequestWindow = 2,
      packedTexLayout = false
    )

  val dut = TmuTextureCache(c, formalStrong = formalStrong)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  val sampleRequestPayload = anyseq(cloneOf(dut.io.sampleRequest.payload))
  val texRspFragment = anyseq(cloneOf(dut.io.texRead.rsp.fragment))

  dut.io.sampleRequest.valid := anyseq(Bool())
  dut.io.sampleRequest.payload := sampleRequestPayload
  dut.io.fetched.ready := anyseq(Bool())
  dut.io.fastFetch.ready := anyseq(Bool())
  dut.io.outputRoute.ready := anyseq(Bool())
  dut.io.texRead.cmd.ready := anyseq(Bool())
  dut.io.texRead.rsp.valid := anyseq(Bool())
  dut.io.texRead.rsp.fragment := texRspFragment
  dut.io.texRead.rsp.last := anyseq(Bool())
  dut.io.invalidate := anyseq(Bool())

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.sampleRequest.valid)
    assume(!dut.io.texRead.rsp.valid)
    assume(!dut.io.invalidate)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.sampleRequest.formalAssumesSlave()
  if (formalStrong) {
    dut.io.fetched.formalAssertsMaster()
    dut.io.fastFetch.formalAssertsMaster()
  }

  if (formalStrong) {
    val readOutstanding = Reg(UInt(log2Up(c.texFillLineWords + 1) bits)) init (0)
    val prevFastAccept = Reg(Bool()) init (False)
    val fastAccept =
      dut.io.sampleRequest.fire && dut.io.outputRoute.fire && dut.io.outputRoute.payload

    when(dut.io.texRead.cmd.fire) {
      assert(readOutstanding === 0)
      readOutstanding := U(c.texFillLineWords, readOutstanding.getWidth bits)
    }

    when(
      dut.io.texRead.rsp.valid && !dut.io.texRead.rsp.ready && pastValid && past(
        dut.io.texRead.rsp.valid && !dut.io.texRead.rsp.ready
      )
    ) {
      assume(dut.io.texRead.rsp.fragment.asBits === past(dut.io.texRead.rsp.fragment.asBits))
      assume(dut.io.texRead.rsp.last === past(dut.io.texRead.rsp.last))
    }

    when(readOutstanding === 0) {
      assume(!dut.io.texRead.rsp.valid)
    }
    when(dut.io.texRead.rsp.valid) {
      assume(readOutstanding =/= 0)
      assume(dut.io.texRead.rsp.last === (readOutstanding === 1))
    }

    when(dut.io.texRead.rsp.fire) {
      readOutstanding := readOutstanding - 1
    }

    when(dut.io.texRead.cmd.valid) {
      assert(dut.io.texRead.cmd.fragment.opcode === Bmb.Cmd.Opcode.READ)
      assert(dut.io.texRead.cmd.last)
    }

    cover(
      pastValid &&
        prevFastAccept &&
        fastAccept
    )

    prevFastAccept := fastAccept
  }
}

class TmuTextureCacheFormalBmcDut extends TmuTextureCacheFormalDut(formalStrong = true)

class TmuTextureCacheFormalCoverDut extends TmuTextureCacheFormalDut(formalStrong = true)

class TmuTextureCacheFormalProveDut extends TmuTextureCacheFormalDut(formalStrong = false)

class TmuTextureCacheFastBilinearCoverDut extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 12 bits,
      memBurstLengthWidth = 5,
      useTexFillCache = true,
      texFillLineWords = 4,
      texFillCacheSlots = 2,
      texFillRequestWindow = 2,
      packedTexLayout = true
    )

  val dut = TmuTextureCache(c, formalStrong = true)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init False
  val reqIndex = Reg(UInt(3 bits)) init 0
  val requestIssued = Reg(Bool()) init False
  val readOutstanding = Reg(UInt(log2Up(c.texFillLineWords + 1) bits)) init 0
  val prevFastAccept = Reg(Bool()) init False
  val fastAccept =
    dut.io.sampleRequest.fire && dut.io.outputRoute.fire && dut.io.outputRoute.payload
  val texRspFragment = cloneOf(dut.io.texRead.rsp.fragment)

  when(dut.io.sampleRequest.fire) {
    when(reqIndex < 4) {
      requestIssued := True
    } otherwise {
      reqIndex := reqIndex + 1
    }
  }
  when(requestIssued && dut.io.fetched.fire) {
    requestIssued := False
    reqIndex := reqIndex + 1
  }

  val bilinearReq = reqIndex >= 4
  val pointAddr = UInt(c.addressWidth.value bits)
  pointAddr := 0
  when(reqIndex === 1) {
    pointAddr := 2
  }
  when(reqIndex === 2) {
    pointAddr := 512
  }
  when(reqIndex === 3) {
    pointAddr := 514
  }

  dut.io.sampleRequest.valid := pastValid && reqIndex =/= 6 && !requestIssued
  dut.io.sampleRequest.payload.pointAddr := pointAddr
  dut.io.sampleRequest.payload.biAddr0 := 0
  dut.io.sampleRequest.payload.biAddr1 := 2
  dut.io.sampleRequest.payload.biAddr2 := 512
  dut.io.sampleRequest.payload.biAddr3 := 514
  dut.io.sampleRequest.payload.pointBankSel := Mux(
    reqIndex === 1,
    U(1, 2 bits),
    Mux(reqIndex === 2, U(2, 2 bits), Mux(reqIndex === 3, U(3, 2 bits), U(0, 2 bits)))
  )
  dut.io.sampleRequest.payload.biBankSel0 := 0
  dut.io.sampleRequest.payload.biBankSel1 := 1
  dut.io.sampleRequest.payload.biBankSel2 := 2
  dut.io.sampleRequest.payload.biBankSel3 := 3
  dut.io.sampleRequest.payload.lodBase := 0
  dut.io.sampleRequest.payload.lodEnd := 1024
  dut.io.sampleRequest.payload.lodShift := 8
  dut.io.sampleRequest.payload.is16Bit := True
  for (lod <- 0 until 9) {
    val base = if (lod == 0) 0 else if (lod == 1) 1024 else 2048 + lod * 64
    dut.io.sampleRequest.payload.texTables.texBase(lod) := U(base, 22 bits)
    dut.io.sampleRequest.payload.texTables.texEnd(lod) := U(base + 1024, 22 bits)
    dut.io.sampleRequest.payload.texTables.texShift(lod) := 8
  }
  dut.io.sampleRequest.payload.bilinear := bilinearReq
  dut.io.sampleRequest.payload.passthrough.format := Tmu.TextureFormat.RGB565
  dut.io.sampleRequest.payload.passthrough.bilinear := bilinearReq
  dut.io.sampleRequest.payload.passthrough.sendConfig := False
  dut.io.sampleRequest.payload.passthrough.ds := 0
  dut.io.sampleRequest.payload.passthrough.dt := 0
  dut.io.sampleRequest.payload.passthrough.readIdx := 0
  dut.io.sampleRequest.payload.passthrough.requestId := 0
  for (i <- 0 until 16) dut.io.sampleRequest.payload.passthrough.ncc.y(i) := 0
  for (i <- 0 until 4) {
    dut.io.sampleRequest.payload.passthrough.ncc.i(i) := 0
    dut.io.sampleRequest.payload.passthrough.ncc.q(i) := 0
  }

  dut.io.fetched.ready := True
  dut.io.fastFetch.ready := True
  dut.io.outputRoute.ready := True
  dut.io.texRead.cmd.ready := True
  dut.io.invalidate := False
  texRspFragment.assignDontCare()
  texRspFragment.data := 0
  texRspFragment.source := 0
  texRspFragment.context := 0
  dut.io.texRead.rsp.fragment := texRspFragment
  dut.io.texRead.rsp.valid := readOutstanding =/= 0
  dut.io.texRead.rsp.last := readOutstanding === 1

  assumeInitial(reset)
  when(reset) {
    assume(!dut.io.sampleRequest.valid)
  }
  when(pastValid) {
    assume(!reset)
  }

  dut.io.sampleRequest.formalAssumesSlave()
  dut.io.fetched.formalAssertsMaster()
  dut.io.fastFetch.formalAssertsMaster()

  when(dut.io.texRead.cmd.fire) {
    assert(readOutstanding === 0)
    readOutstanding := U(c.texFillLineWords, readOutstanding.getWidth bits)
  }

  when(dut.io.texRead.rsp.fire) {
    assert(readOutstanding =/= 0)
    readOutstanding := readOutstanding - 1
  }

  when(readOutstanding === 0) {
    assert(!dut.io.texRead.rsp.valid)
  }
  when(dut.io.texRead.rsp.valid) {
    assert(dut.io.texRead.rsp.last === (readOutstanding === 1))
  }

  cover(pastValid && prevFastAccept && fastAccept)
  prevFastAccept := fastAccept
}

class TmuTextureCacheFormalTest extends SpinalFormalFunSuite {
  test("TMU-specific texture cache invariants bmc") {
    FormalConfig
      .withBMC(14)
      .withAsync
      .doVerify(new TmuTextureCacheFormalBmcDut)
  }

  test("TMU-specific texture cache bilinear hit throughput cover") {
    FormalConfig
      .withCover(64)
      .withAsync
      .doVerify(new TmuTextureCacheFastBilinearCoverDut)
  }

  test("TMU-specific texture cache invariants prove") {
    FormalConfig
      .withProve(14)
      .withAsync
      .doVerify(new TmuTextureCacheFormalProveDut)
  }
}
