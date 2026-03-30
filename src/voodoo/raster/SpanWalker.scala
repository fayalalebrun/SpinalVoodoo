package voodoo.raster

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import voodoo._

case class SpanWalker(c: Config, formalStrong: Boolean = false) extends Component {
  val i = slave Stream (TriangleSetup.Output(c))
  val o = master Stream (SpanWalker.Output(c))
  val enableClipping = in Bool ()
  val clipLeft = in UInt (10 bits)
  val clipRight = in UInt (10 bits)
  val running = out(Bool())

  object WalkerState extends SpinalEnum {
    val Idle, Decide, SearchRightToEnter, SearchLeftToExit, SearchRightToExit, EmitSpan =
      newElement()
  }

  val state = RegInit(WalkerState.Idle)
  val triangleReg = Reg(TriangleSetup.Output(c))
  val rowGuess = Reg(SpanWalker.Cursor(c))
  val probe = Reg(SpanWalker.Cursor(c))
  val bookmark = Reg(SpanWalker.Cursor(c))
  val leftEdge = Reg(SpanWalker.Cursor(c))
  val emitRight = Reg(AFix(c.vertexFormat))
  val emitFirstSpan = Reg(Bool()) init (False)
  val firstSpanPending = Reg(Bool()) init (False)

  running := state =/= WalkerState.Idle
  i.ready := state === WalkerState.Idle
  o.valid := state === WalkerState.EmitSpan

  val one = AFix(c.vertexFormat)
  one := 1.0
  val negOne = AFix(c.vertexFormat)
  negOne := -1.0
  val zero = 0.SQ(1 bits, 0 bits)
  val vertexFracZeros = U(0, c.vertexFormat.fraction bits)

  private val lowGradFormats = Seq(
    c.vColorFormat,
    c.vColorFormat,
    c.vColorFormat,
    c.vDepthFormat,
    c.vColorFormat,
    c.wAccumFormat
  )

  def initCursor(dst: SpanWalker.Cursor, input: TriangleSetup.Output): Unit = {
    dst.coords(0) := input.xrange(0)
    dst.coords(1) := input.yrange(0)
    dst.edge := input.edgeStart
    dst.grads.all.take(6).zip(input.grads.all.take(6)).foreach { case (out, in) =>
      out := in.start
    }
    dst.grads.sGrad := input.texHi.sStart.fixTo(c.texCoordsAccumFormat)
    dst.grads.tGrad := input.texHi.tStart.fixTo(c.texCoordsAccumFormat)
    dst.texHi := input.texHi
    dst.alphaHi := input.hiAlpha
  }

  def insideTriangle(cursor: SpanWalker.Cursor): Bool =
    (cursor.edge(0) >= zero) && (cursor.edge(1) >= zero) && (cursor.edge(2) >= zero)

  def xPix(cursor: SpanWalker.Cursor): SInt = cursor.coords(0).floor(0).asSInt

  def pixelToVertex(pix: SInt): AFix = {
    val out = AFix(c.vertexFormat)
    out.raw := (pix.resize(c.vertexFormat.nonFraction bits) ## vertexFracZeros).asBits
    out
  }

  def projectCursorToX(dst: SpanWalker.Cursor, src: SpanWalker.Cursor, targetPix: SInt): Unit = {
    projectCursorToPixel(dst, src, targetPix, src.coords(1).floor(0).asSInt)
  }

  def projectCursorToPixel(
      dst: SpanWalker.Cursor,
      src: SpanWalker.Cursor,
      targetXPix: SInt,
      targetYPix: SInt
  ): Unit = {
    val targetX = pixelToVertex(targetXPix)
    val targetY = pixelToVertex(targetYPix)
    val deltaX = (targetX - src.coords(0)).fixTo(c.vertexFormat)
    val deltaY = (targetY - src.coords(1)).fixTo(c.vertexFormat)
    dst.coords(0) := targetX
    dst.coords(1) := targetY
    dst.edge.zip(src.edge).zip(triangleReg.coeffs).foreach { case ((nxt, cur), coeff) =>
      nxt := (cur + (coeff.a * deltaX) + (coeff.b * deltaY)).fixTo(c.coefficientFormat)
    }
    dst.grads.all
      .take(6)
      .zip(src.grads.all.take(6))
      .zip(triangleReg.grads.all.take(6))
      .zipWithIndex
      .foreach { case (((nxt, cur), grad), idx) =>
        nxt := (cur + (grad.d(0) * deltaX) + (grad.d(1) * deltaY)).fixTo(lowGradFormats(idx))
      }
    val nextS =
      (src.texHi.sStart + triangleReg.texHi.dSdX * deltaX + triangleReg.texHi.dSdY * deltaY)
        .fixTo(c.texCoordsHiFormat)
    val nextT =
      (src.texHi.tStart + triangleReg.texHi.dTdX * deltaX + triangleReg.texHi.dTdY * deltaY)
        .fixTo(c.texCoordsHiFormat)
    val nextA =
      (src.alphaHi.start + triangleReg.hiAlpha.dAdX * deltaX + triangleReg.hiAlpha.dAdY * deltaY)
        .fixTo(c.texCoordsHiFormat)
    dst.texHi.sStart := nextS
    dst.texHi.tStart := nextT
    dst.texHi.dSdX := triangleReg.texHi.dSdX
    dst.texHi.dTdX := triangleReg.texHi.dTdX
    dst.texHi.dSdY := triangleReg.texHi.dSdY
    dst.texHi.dTdY := triangleReg.texHi.dTdY
    dst.alphaHi.start := nextA
    dst.alphaHi.dAdX := triangleReg.hiAlpha.dAdX
    dst.alphaHi.dAdY := triangleReg.hiAlpha.dAdY
    dst.grads.sGrad := nextS.fixTo(c.texCoordsAccumFormat)
    dst.grads.tGrad := nextT.fixTo(c.texCoordsAccumFormat)
  }

  val triangleXStartPix = triangleReg.xrange(0).floor(0).asSInt
  val triangleXEndPix = triangleReg.xrange(1).floor(0).asSInt
  val clipLeftPix = clipLeft.resize(c.vertexFormat.nonFraction bits).asSInt
  val clipRightPix = clipRight.resize(c.vertexFormat.nonFraction bits).asSInt
  val visibleStartPix =
    Mux(enableClipping && clipLeftPix > triangleXStartPix, clipLeftPix, triangleXStartPix)
  val visibleEndPix =
    Mux(enableClipping && clipRightPix < triangleXEndPix, clipRightPix, triangleXEndPix)

  def stepHorizontal(dst: SpanWalker.Cursor, src: SpanWalker.Cursor, moveRight: Boolean): Unit = {
    val xDelta = if (moveRight) one else negOne
    dst.coords(0) := (src.coords(0) + xDelta).fixTo(c.vertexFormat)
    dst.coords(1) := src.coords(1)
    dst.edge.zip(src.edge).zip(triangleReg.coeffs).foreach { case ((nxt, cur), coeff) =>
      val delta = if (moveRight) coeff.a else (-coeff.a).fixTo(c.coefficientFormat)
      nxt := (cur + delta).fixTo(c.coefficientFormat)
    }
    dst.grads.all
      .take(6)
      .zip(src.grads.all.take(6))
      .zip(triangleReg.grads.all.take(6))
      .zipWithIndex
      .foreach { case (((nxt, cur), grad), idx) =>
        val delta = if (moveRight) grad.d(0) else (-grad.d(0)).fixTo(lowGradFormats(idx))
        nxt := (cur + delta).fixTo(lowGradFormats(idx))
      }
    val sDelta = if (moveRight) triangleReg.texHi.dSdX else (-triangleReg.texHi.dSdX)
    val tDelta = if (moveRight) triangleReg.texHi.dTdX else (-triangleReg.texHi.dTdX)
    val aDelta = if (moveRight) triangleReg.hiAlpha.dAdX else (-triangleReg.hiAlpha.dAdX)
    val nextS = (src.texHi.sStart + sDelta).fixTo(c.texCoordsHiFormat)
    val nextT = (src.texHi.tStart + tDelta).fixTo(c.texCoordsHiFormat)
    val nextA = (src.alphaHi.start + aDelta).fixTo(c.texCoordsHiFormat)
    dst.texHi.sStart := nextS
    dst.texHi.tStart := nextT
    dst.texHi.dSdX := triangleReg.texHi.dSdX
    dst.texHi.dTdX := triangleReg.texHi.dTdX
    dst.texHi.dSdY := triangleReg.texHi.dSdY
    dst.texHi.dTdY := triangleReg.texHi.dTdY
    dst.alphaHi.start := nextA
    dst.alphaHi.dAdX := triangleReg.hiAlpha.dAdX
    dst.alphaHi.dAdY := triangleReg.hiAlpha.dAdY
    dst.grads.sGrad := nextS.fixTo(c.texCoordsAccumFormat)
    dst.grads.tGrad := nextT.fixTo(c.texCoordsAccumFormat)
  }

  def stepVertical(dst: SpanWalker.Cursor, src: SpanWalker.Cursor): Unit = {
    dst.coords(0) := src.coords(0)
    dst.coords(1) := (src.coords(1) + one).fixTo(c.vertexFormat)
    dst.edge.zip(src.edge).zip(triangleReg.coeffs).foreach { case ((nxt, cur), coeff) =>
      nxt := (cur + coeff.b).fixTo(c.coefficientFormat)
    }
    dst.grads.all
      .take(6)
      .zip(src.grads.all.take(6))
      .zip(triangleReg.grads.all.take(6))
      .zipWithIndex
      .foreach { case (((nxt, cur), grad), idx) =>
        nxt := (cur + grad.d(1)).fixTo(lowGradFormats(idx))
      }
    val nextS = (src.texHi.sStart + triangleReg.texHi.dSdY).fixTo(c.texCoordsHiFormat)
    val nextT = (src.texHi.tStart + triangleReg.texHi.dTdY).fixTo(c.texCoordsHiFormat)
    val nextA = (src.alphaHi.start + triangleReg.hiAlpha.dAdY).fixTo(c.texCoordsHiFormat)
    dst.texHi.sStart := nextS
    dst.texHi.tStart := nextT
    dst.texHi.dSdX := triangleReg.texHi.dSdX
    dst.texHi.dTdX := triangleReg.texHi.dTdX
    dst.texHi.dSdY := triangleReg.texHi.dSdY
    dst.texHi.dTdY := triangleReg.texHi.dTdY
    dst.alphaHi.start := nextA
    dst.alphaHi.dAdX := triangleReg.hiAlpha.dAdX
    dst.alphaHi.dAdY := triangleReg.hiAlpha.dAdY
    dst.grads.sGrad := nextS.fixTo(c.texCoordsAccumFormat)
    dst.grads.tGrad := nextT.fixTo(c.texCoordsAccumFormat)
  }

  def prepareNextRow(base: SpanWalker.Cursor, leftBiasedGuess: Boolean): Unit = {
    val nextY = (base.coords(1) + one).fixTo(c.vertexFormat)
    when(nextY >= triangleReg.yrange(1)) {
      state := WalkerState.Idle
      firstSpanPending := False
    }.otherwise {
      if (leftBiasedGuess) {
        val nextGuessX = xPix(base) - 1
        val nextGuessY = base.coords(1).floor(0).asSInt + 1
        projectCursorToPixel(rowGuess, base, nextGuessX, nextGuessY)
        projectCursorToPixel(probe, base, nextGuessX, nextGuessY)
        projectCursorToPixel(bookmark, base, nextGuessX, nextGuessY)
      } else {
        stepVertical(rowGuess, base)
        stepVertical(probe, base)
        stepVertical(bookmark, base)
      }
      state := WalkerState.Decide
    }
  }

  o.payload.xStart := leftEdge.coords(0)
  o.payload.xEnd := emitRight
  o.payload.y := leftEdge.coords(1)
  o.payload.grads.all.zip(leftEdge.grads.all).zip(triangleReg.grads.all).foreach {
    case ((out, cur), grad) =>
      out.start := cur
      out.d := grad.d
  }
  o.payload.texHi.sStart := leftEdge.texHi.sStart
  o.payload.texHi.tStart := leftEdge.texHi.tStart
  o.payload.texHi.dSdX := triangleReg.texHi.dSdX
  o.payload.texHi.dTdX := triangleReg.texHi.dTdX
  o.payload.texHi.dSdY := triangleReg.texHi.dSdY
  o.payload.texHi.dTdY := triangleReg.texHi.dTdY
  o.payload.hiAlpha.start := leftEdge.alphaHi.start
  o.payload.hiAlpha.dAdX := triangleReg.hiAlpha.dAdX
  o.payload.hiAlpha.dAdY := triangleReg.hiAlpha.dAdY
  o.payload.config := triangleReg.config
  o.payload.firstSpan := emitFirstSpan
  if (c.trace.enabled) {
    o.payload.trace := triangleReg.trace
  }

  when(state === WalkerState.Idle && i.fire) {
    triangleReg := i.payload
    initCursor(rowGuess, i.payload)
    initCursor(probe, i.payload)
    initCursor(bookmark, i.payload)
    firstSpanPending := True
    state := WalkerState.Decide
  }

  when(state === WalkerState.Decide) {
    when(insideTriangle(probe)) {
      bookmark := probe
      state := WalkerState.SearchLeftToExit
    }.otherwise {
      state := WalkerState.SearchRightToEnter
    }
  }

  when(state === WalkerState.SearchRightToEnter) {
    when(insideTriangle(probe) && xPix(probe) >= visibleStartPix) {
      leftEdge := probe
      bookmark := probe
      state := WalkerState.SearchRightToExit
    }.elsewhen(
      (!enableClipping && probe.coords(0) >= triangleReg.xrange(1)) || (enableClipping && xPix(
        probe
      ) >= visibleEndPix)
    ) {
      prepareNextRow(rowGuess, leftBiasedGuess = false)
    }.otherwise {
      stepHorizontal(probe, probe, moveRight = true)
    }
  }

  when(state === WalkerState.SearchLeftToExit) {
    when(insideTriangle(probe)) {
      when(xPix(probe) <= visibleStartPix) {
        projectCursorToX(leftEdge, probe, visibleStartPix)
        probe := bookmark
        state := WalkerState.SearchRightToExit
      }.otherwise {
        stepHorizontal(probe, probe, moveRight = false)
      }
    }.otherwise {
      projectCursorToX(
        leftEdge,
        probe,
        Mux(xPix(probe) + 1 < visibleStartPix, visibleStartPix, xPix(probe) + 1)
      )
      probe := bookmark
      state := WalkerState.SearchRightToExit
    }
  }

  when(state === WalkerState.SearchRightToExit) {
    when(insideTriangle(probe)) {
      when(
        (!enableClipping && probe.coords(0) >= triangleReg.xrange(1)) || (enableClipping && xPix(
          probe
        ) >= visibleEndPix)
      ) {
        emitRight := Mux(enableClipping, pixelToVertex(visibleEndPix - 1), probe.coords(0))
        emitFirstSpan := firstSpanPending
        state := WalkerState.EmitSpan
      }.otherwise {
        stepHorizontal(probe, probe, moveRight = true)
      }
    }.otherwise {
      emitRight := (probe.coords(0) + negOne).fixTo(c.vertexFormat)
      emitFirstSpan := firstSpanPending
      state := WalkerState.EmitSpan
    }
  }

  when(state === WalkerState.EmitSpan && o.ready) {
    firstSpanPending := False
    prepareNextRow(leftEdge, leftBiasedGuess = true)
  }

}

object SpanWalker {
  case class Cursor(c: Config) extends Bundle {
    val coords = vertex2d(c.vertexFormat)
    val grads = Rasterizer.GradientBundle(AFix(_), c)
    val alphaHi = TriangleSetup.HiAlpha(c)
    val texHi = TriangleSetup.HiTexCoords(c)
    val edge = Vec.fill(3)(AFix(c.coefficientFormat))
  }

  case class Output(c: Config) extends Bundle {
    val xStart = AFix(c.vertexFormat)
    val xEnd = AFix(c.vertexFormat)
    val y = AFix(c.vertexFormat)
    val grads = Rasterizer.GradientBundle(Rasterizer.InputGradient(_), c)
    val hiAlpha = TriangleSetup.HiAlpha(c)
    val texHi = TriangleSetup.HiTexCoords(c)
    val config = TriangleSetup.PerTriangleConfig(c)
    val firstSpan = Bool()
    val trace = if (c.trace.enabled) Trace.PrimitiveKey() else null
  }
}
