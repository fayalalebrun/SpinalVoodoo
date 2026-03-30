//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib.formal.SpinalFormalFunSuite
import voodoo.raster._

class RasterizerFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig(enabled = true))
    .copy(
      addressWidth = 8 bits,
      maxFbDims = (8, 8)
    )

  val dut = Rasterizer(c, formalStrong = formalStrong)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  def buildBox(payload: TriangleSetup.Output, primitiveId: Int, yStart: Int): Unit = {
    payload.xrange(0) := 0.0
    payload.xrange(1) := 5.0
    payload.yrange(0) := yStart.toDouble
    payload.yrange(1) := (yStart + 3).toDouble
    for (idx <- 0 until 3) {
      payload.coeffs(idx).a := 0.0
      payload.coeffs(idx).b := 0.0
      payload.coeffs(idx).c := 1.0
      payload.edgeStart(idx) := 1.0
    }
    payload.grads.all.foreach { grad =>
      grad.start := 0.0
      grad.d(0) := 0.0
      grad.d(1) := 0.0
    }
    payload.texHi.sStart := 0.0
    payload.texHi.tStart := 0.0
    payload.texHi.dSdX := 0.0
    payload.texHi.dTdX := 0.0
    payload.texHi.dSdY := 0.0
    payload.texHi.dTdY := 0.0
    payload.hiAlpha.start := 0.0
    payload.hiAlpha.dAdX := 0.0
    payload.hiAlpha.dAdY := 0.0
    payload.config.assignFromBits(B(0, payload.config.getBitsWidth bits))
    payload.trace.valid := True
    payload.trace.origin := U(Trace.Origin.triangle, 2 bits)
    payload.trace.drawId := 0
    payload.trace.primitiveId := primitiveId
  }

  val triangle0 = TriangleSetup.Output(c)
  val triangle1 = TriangleSetup.Output(c)
  buildBox(triangle0, primitiveId = 0, yStart = 0)
  buildBox(triangle1, primitiveId = 1, yStart = 3)

  val sendIndex = Reg(UInt(2 bits)) init (0)
  dut.i.valid := !reset && sendIndex =/= 2
  dut.i.payload := triangle0
  when(sendIndex === 1) {
    dut.i.payload := triangle1
  }
  when(dut.i.fire) {
    sendIndex := sendIndex + 1
  }

  dut.o.ready := True
  dut.enableClipping := False
  dut.clipLeft := 0
  dut.clipRight := 0
  dut.clipLowY := 0
  dut.clipHighY := 0

  assumeInitial(reset)
  when(pastValid) {
    assume(!reset)
  }

  if (formalStrong) {
    when(dut.o.valid) {
      val prim = dut.o.payload.trace.primitive.primitiveId
      val x = dut.o.payload.coords(0)
      val y = dut.o.payload.coords(1)
      assert(dut.o.payload.trace.primitive.valid)
      assert(dut.o.payload.trace.primitive.origin === U(Trace.Origin.triangle, 2 bits))
      assert(x >= 0)
      assert(x <= 5)
      when(prim === 0) {
        assert(y >= 0)
        assert(y <= 2)
      }.otherwise {
        assert(prim === 1)
        assert(y >= 3)
        assert(y <= 5)
      }
    }

    when(pastValid && past(dut.o.fire)) {
      val prevPrim = past(dut.o.payload.trace.primitive.primitiveId)
      when(dut.o.fire && dut.o.payload.trace.primitive.primitiveId === prevPrim) {
        assert(dut.o.payload.trace.pixelSeq === past(dut.o.payload.trace.pixelSeq) + 1)
      }
      when(dut.o.fire && dut.o.payload.trace.primitive.primitiveId =/= prevPrim) {
        assert(prevPrim === 0)
        assert(dut.o.payload.trace.primitive.primitiveId === 1)
        assert(dut.o.payload.trace.pixelSeq === 0)
      }
    }

  } else {
    assume(sendIndex <= 2)
    assert(dut.i.valid === (!reset && sendIndex =/= 2))
  }
}

class SpanRasterizerThroughputCoverDut extends Component {
  val c = Config
    .voodoo1(TraceConfig(enabled = true))
    .copy(
      addressWidth = 4 bits,
      maxFbDims = (1, 1)
    )

  val src = spinal.lib.Stream(SpanWalker.Output(c))
  val queued = src.queue(2)
  val dut = Rasterizer.SpanRasterizer(c)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  def buildSpan(payload: SpanWalker.Output, primitiveId: Int, y: Int): Unit = {
    payload.xStart := 0.0
    payload.xEnd := 0.0
    payload.y := y.toDouble
    payload.grads.all.foreach { grad =>
      grad.start := 0.0
      grad.d(0) := 0.0
      grad.d(1) := 0.0
    }
    payload.texHi.sStart := 0.0
    payload.texHi.tStart := 0.0
    payload.texHi.dSdX := 0.0
    payload.texHi.dTdX := 0.0
    payload.texHi.dSdY := 0.0
    payload.texHi.dTdY := 0.0
    payload.hiAlpha.start := 0.0
    payload.hiAlpha.dAdX := 0.0
    payload.hiAlpha.dAdY := 0.0
    payload.config.assignFromBits(B(0, payload.config.getBitsWidth bits))
    payload.firstSpan := True
    payload.trace.valid := True
    payload.trace.origin := U(Trace.Origin.triangle, 2 bits)
    payload.trace.drawId := 0
    payload.trace.primitiveId := primitiveId
  }

  val span0 = SpanWalker.Output(c)
  val span1 = SpanWalker.Output(c)
  buildSpan(span0, primitiveId = 0, y = 0)
  buildSpan(span1, primitiveId = 1, y = 1)

  dut.i << queued

  val sendIndex = Reg(UInt(2 bits)) init (0)
  src.valid := !reset && sendIndex =/= 2
  src.payload := span0
  when(sendIndex === 1) {
    src.payload := span1
  }
  when(src.fire) {
    sendIndex := sendIndex + 1
  }

  dut.o.ready := True
  dut.enableClipping := False
  dut.clipLeft := 0
  dut.clipRight := 0
  dut.clipLowY := 0
  dut.clipHighY := 0

  assumeInitial(reset)
  when(pastValid) {
    assume(!reset)
  }

  cover(
    !reset &&
      sendIndex === 2 &&
      pastValid &&
      past(pastValid) &&
      !past(dut.o.fire) &&
      dut.o.fire &&
      dut.o.payload.trace.primitive.valid &&
      dut.o.payload.trace.primitive.primitiveId === 1 &&
      dut.o.payload.trace.pixelSeq === 0 &&
      past(past(dut.o.fire)) &&
      past(past(dut.o.payload.trace.primitive.primitiveId)) === 0 &&
      past(past(dut.o.payload.trace.pixelSeq)) === 0
  )
}

class RasterizerFormalBmcDut extends RasterizerFormalDut(formalStrong = true)

class RasterizerFormalProveDut extends RasterizerFormalDut(formalStrong = false)

class RasterizerFormalCoverDut extends RasterizerFormalDut(formalStrong = true)

class RasterizerFormalTest extends SpinalFormalFunSuite {
  test("Rasterizer invariants bmc") {
    FormalConfig
      .withBMC(20)
      .withAsync
      .doVerify(new RasterizerFormalBmcDut)
  }

  test("Rasterizer invariants prove") {
    FormalConfig
      .withProve(20)
      .withAsync
      .doVerify(new RasterizerFormalProveDut)
  }

  test("Rasterizer throughput cover") {
    FormalConfig
      .withCover(12)
      .withAsync
      .doVerify(new SpanRasterizerThroughputCoverDut)
  }
}
