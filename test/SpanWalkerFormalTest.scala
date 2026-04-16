//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib.formal.SpinalFormalFunSuite
import voodoo.raster._

class SpanWalkerFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig(enabled = true))
    .copy(
      vertexFormat = QFormat(5, 1, true),
      vColorFormat = QFormat(4, 1, true),
      vDepthFormat = QFormat(4, 1, true),
      wAccumFormat = QFormat(4, 1, true),
      coefficientFormat = QFormat(6, 1, true),
      texCoordsFormat = QFormat(4, 1, true),
      texCoordsAccumFormat = QFormat(5, 1, true),
      texCoordsHiFormat = QFormat(5, 1, true),
      addressWidth = 4 bits,
      maxFbDims = (4, 4)
    )

  val dut = SpanWalker(c, formalStrong = formalStrong)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  def buildBox(
      payload: TriangleSetup.Output,
      primitiveId: Int,
      x0: Int,
      x1: Int,
      y0: Int,
      y1: Int
  ): Unit = {
    payload.xrange(0) := x0.toDouble
    payload.xrange(1) := x1.toDouble
    payload.yrange(0) := y0.toDouble
    payload.yrange(1) := y1.toDouble
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
    val config = TriangleSetup.PerTriangleConfig(c)
    config.assignFromBits(B(0, payload.config.getBitsWidth bits))
    config.clipLeft.allowOverride
    config.clipRight.allowOverride
    config.clipLowY.allowOverride
    config.clipHighY.allowOverride
    config.clipLeft := 0
    config.clipRight := c.maxFbDims._1
    config.clipLowY := 0
    config.clipHighY := c.maxFbDims._2
    payload.config := config
    payload.trace.valid := True
    payload.trace.origin := U(Trace.Origin.triangle, 2 bits)
    payload.trace.drawId := 0
    payload.trace.primitiveId := primitiveId
  }

  val tri0 = TriangleSetup.Output(c)
  val tri1 = TriangleSetup.Output(c)
  buildBox(tri0, primitiveId = 0, x0 = 1, x1 = 2, y0 = 0, y1 = 2)
  buildBox(tri1, primitiveId = 1, x0 = 0, x1 = 1, y0 = 2, y1 = 4)

  val sendIndex = Reg(UInt(2 bits)) init (0)
  dut.i.valid := !reset && sendIndex =/= 2
  dut.i.payload := tri0
  when(sendIndex === 1) {
    dut.i.payload := tri1
  }
  when(dut.i.fire) {
    sendIndex := sendIndex + 1
  }

  dut.o.ready := True

  assumeInitial(reset)
  when(pastValid) {
    assume(!reset)
  }

  if (formalStrong) {
    when(dut.o.valid) {
      val xStart = dut.o.payload.xStart.floor(0).asSInt
      val xEnd = dut.o.payload.xEnd.floor(0).asSInt
      val y = dut.o.payload.y.floor(0).asSInt
      assert(dut.o.payload.trace.valid)
      assert(dut.o.payload.trace.origin === U(Trace.Origin.triangle, 2 bits))
      when(dut.o.payload.trace.primitiveId === 0) {
        assert(xStart === 1)
        assert(xEnd === 2)
        assert(y >= 0)
        assert(y <= 1)
      }.otherwise {
        assert(dut.o.payload.trace.primitiveId === 1)
        assert(xStart === 0)
        assert(xEnd === 1)
        assert(y >= 2)
        assert(y <= 3)
      }
    }

    when(pastValid && past(dut.o.fire)) {
      val prevPrim = past(dut.o.payload.trace.primitiveId)
      val y = dut.o.payload.y.floor(0).asSInt
      val prevY = past(dut.o.payload.y.floor(0).asSInt)
      when(dut.o.fire && dut.o.payload.trace.primitiveId === prevPrim) {
        assert(y === prevY + 1)
        assert(!dut.o.payload.firstSpan)
      }
      when(dut.o.fire && dut.o.payload.trace.primitiveId =/= prevPrim) {
        assert(prevPrim === 0)
        assert(dut.o.payload.trace.primitiveId === 1)
        assert(dut.o.payload.firstSpan)
        assert(y === 2)
      }
    }

    when(
      dut.o.fire && dut.o.payload.trace.primitiveId === 0 && dut.o.payload.y.floor(0).asSInt === 0
    ) {
      assert(dut.o.payload.firstSpan)
    }
  } else {
    assume(sendIndex <= 2)
    assert(dut.i.valid === (!reset && sendIndex =/= 2))
  }

  val spanCount = Reg(UInt(3 bits)) init (0)
  when(reset) {
    spanCount := 0
  }.elsewhen(dut.o.fire && spanCount =/= 7) {
    spanCount := spanCount + 1
  }

  cover(
    !reset &&
      spanCount === 1 &&
      dut.o.fire &&
      dut.o.payload.trace.primitiveId === 0 &&
      !dut.o.payload.firstSpan &&
      dut.o.payload.y.floor(0).asSInt === 1
  )
}

class SpanWalkerClipLeftFormalDut extends Component {
  val c = Config
    .voodoo1(TraceConfig(enabled = true))
    .copy(
      vertexFormat = QFormat(5, 1, true),
      vColorFormat = QFormat(4, 1, true),
      vDepthFormat = QFormat(4, 1, true),
      wAccumFormat = QFormat(4, 1, true),
      coefficientFormat = QFormat(6, 1, true),
      texCoordsFormat = QFormat(4, 1, true),
      texCoordsAccumFormat = QFormat(5, 1, true),
      texCoordsHiFormat = QFormat(5, 1, true),
      addressWidth = 4 bits,
      maxFbDims = (4, 4)
    )

  val dut = SpanWalker(c, formalStrong = true)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  val clipped = TriangleSetup.Output(c)
  clipped.xrange(0) := -1.0
  clipped.xrange(1) := 1.0
  clipped.yrange(0) := 0.0
  clipped.yrange(1) := 1.0
  for (idx <- 0 until 3) {
    clipped.coeffs(idx).a := 0.0
    clipped.coeffs(idx).b := 0.0
    clipped.coeffs(idx).c := 1.0
    clipped.edgeStart(idx) := 1.0
  }
  clipped.grads.all.foreach { grad =>
    grad.start := 0.0
    grad.d(0) := 0.0
    grad.d(1) := 0.0
  }
  clipped.texHi.sStart := 0.0
  clipped.texHi.tStart := 0.0
  clipped.texHi.dSdX := 0.0
  clipped.texHi.dTdX := 0.0
  clipped.texHi.dSdY := 0.0
  clipped.texHi.dTdY := 0.0
  clipped.hiAlpha.start := 0.0
  clipped.hiAlpha.dAdX := 0.0
  clipped.hiAlpha.dAdY := 0.0
  val clippedConfig = TriangleSetup.PerTriangleConfig(c)
  clippedConfig.assignFromBits(B(0, clipped.config.getBitsWidth bits))
  clippedConfig.enableClipping.allowOverride
  clippedConfig.clipLeft.allowOverride
  clippedConfig.clipRight.allowOverride
  clippedConfig.clipLowY.allowOverride
  clippedConfig.clipHighY.allowOverride
  clippedConfig.enableClipping := True
  clippedConfig.clipLeft := 0
  clippedConfig.clipRight := c.maxFbDims._1
  clippedConfig.clipLowY := 0
  clippedConfig.clipHighY := c.maxFbDims._2
  clipped.config := clippedConfig
  clipped.trace.valid := True
  clipped.trace.origin := U(Trace.Origin.triangle, 2 bits)
  clipped.trace.drawId := 0
  clipped.trace.primitiveId := 0

  val sent = RegInit(False)
  dut.i.valid := !reset && !sent
  dut.i.payload := clipped
  when(dut.i.fire) {
    sent := True
  }

  dut.o.ready := True

  assumeInitial(reset)
  when(pastValid) {
    assume(!reset)
  }

  when(dut.o.valid) {
    val xStart = dut.o.payload.xStart.floor(0).asSInt
    val xEnd = dut.o.payload.xEnd.floor(0).asSInt
    val y = dut.o.payload.y.floor(0).asSInt
    assert(xStart === 0)
    assert(xEnd === 1)
    assert(y === 0)
  }

  cover(!reset && dut.o.fire)
}

class SpanWalkerFormalBmcDut extends SpanWalkerFormalDut(formalStrong = true)

class SpanWalkerFormalProveDut extends SpanWalkerFormalDut(formalStrong = false)

class SpanWalkerFormalCoverDut extends SpanWalkerFormalDut(formalStrong = true)

class SpanWalkerFormalTest extends SpinalFormalFunSuite {
  test("SpanWalker invariants bmc") {
    FormalConfig
      .withBMC(12)
      .withAsync
      .doVerify(new SpanWalkerFormalBmcDut)
  }

  test("SpanWalker invariants prove") {
    FormalConfig
      .withProve(12)
      .withAsync
      .doVerify(new SpanWalkerFormalProveDut)
  }

  test("SpanWalker full-span cover") {
    FormalConfig
      .withCover(24)
      .withAsync
      .doVerify(new SpanWalkerFormalCoverDut)
  }

  test("SpanWalker clips left-offscreen span") {
    FormalConfig
      .withBMC(8)
      .withAsync
      .doVerify(new SpanWalkerClipLeftFormalDut)
  }
}
