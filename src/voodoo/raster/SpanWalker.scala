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
    val Idle, Decide, RecoverLeft, SearchRightToEnter, SearchLeftToExit, SearchRightToExit,
        EmitSpan =
      newElement()
  }

  val state = RegInit(WalkerState.Idle)
  val triangleConst = Reg(SpanWalker.TriangleConst(c))
  val rowGuess = Reg(SpanWalker.Cursor(c))
  val probe = Reg(SpanWalker.Cursor(c))
  val bookmark = Reg(SpanWalker.Cursor(c))
  val leftEdge = Reg(SpanWalker.Cursor(c))
  val emitRight = Reg(AFix(c.vertexFormat))
  val emitFirstSpan = Reg(Bool()) init (False)
  val firstSpanPending = Reg(Bool()) init (False)
  val recoverFoundInside = Reg(Bool()) init (False)

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

  def initTriangleConst(dst: SpanWalker.TriangleConst, input: TriangleSetup.Output): Unit = {
    dst.coeffs := input.coeffs
    dst.xrange := input.xrange
    dst.yrange := input.yrange
    dst.linearDx.all.zip(input.grads.all.take(6)).foreach { case (out, in) =>
      out := in.d(0)
    }
    dst.linearDy.all.zip(input.grads.all.take(6)).foreach { case (out, in) =>
      out := in.d(1)
    }
    dst.texDx(0) := input.texHi.dSdX
    dst.texDx(1) := input.texHi.dTdX
    dst.texDy(0) := input.texHi.dSdY
    dst.texDy(1) := input.texHi.dTdY
    dst.alphaDx := input.hiAlpha.dAdX
    dst.alphaDy := input.hiAlpha.dAdY
    dst.tmuConfig := TriangleSetup.TmuConfig.fromPerTriangle(c, input.config)
    dst.pixelConfig := TriangleSetup.PixelPipelineConfig.fromPerTriangle(c, input.config)
    if (c.trace.enabled) {
      dst.trace := input.trace
    }
  }

  def initCursor(dst: SpanWalker.Cursor, input: TriangleSetup.Output): Unit = {
    dst.coords(0) := input.xrange(0)
    dst.coords(1) := input.yrange(0)
    dst.edge := input.edgeStart
    dst.linear.all.zip(input.grads.all.take(6)).foreach { case (out, in) =>
      out := in.start
    }
    dst.hiS := input.texHi.sStart
    dst.hiT := input.texHi.tStart
    dst.hiAlpha := input.hiAlpha.start
  }

  def insideTriangle(cursor: SpanWalker.Cursor): Bool =
    (cursor.edge(0) >= zero) && (cursor.edge(1) >= zero) && (cursor.edge(2) >= zero)

  def xPix(cursor: SpanWalker.Cursor): SInt = cursor.coords(0).floor(0).asSInt

  def pixelToVertex(pix: SInt): AFix = {
    val out = AFix(c.vertexFormat)
    out.raw := (pix.resize(c.vertexFormat.nonFraction bits) ## vertexFracZeros).asBits
    out
  }

  val triangleXStartPix = triangleConst.xrange(0).floor(0).asSInt
  val triangleXEndPix = triangleConst.xrange(1).floor(0).asSInt
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
    dst.edge.zip(src.edge).zip(triangleConst.coeffs).foreach { case ((nxt, cur), coeff) =>
      val delta = if (moveRight) coeff.a else (-coeff.a).fixTo(c.coefficientFormat)
      nxt := (cur + delta).fixTo(c.coefficientFormat)
    }
    dst.linear.all
      .zip(src.linear.all)
      .zip(triangleConst.linearDx.all)
      .zipWithIndex
      .foreach { case (((nxt, cur), grad), idx) =>
        val delta = if (moveRight) grad else (-grad).fixTo(lowGradFormats(idx))
        nxt := (cur + delta).fixTo(lowGradFormats(idx))
      }
    val sDelta = if (moveRight) triangleConst.texDx(0) else (-triangleConst.texDx(0))
    val tDelta = if (moveRight) triangleConst.texDx(1) else (-triangleConst.texDx(1))
    val aDelta = if (moveRight) triangleConst.alphaDx else (-triangleConst.alphaDx)
    dst.hiS := (src.hiS + sDelta).fixTo(c.texCoordsHiFormat)
    dst.hiT := (src.hiT + tDelta).fixTo(c.texCoordsHiFormat)
    dst.hiAlpha := (src.hiAlpha + aDelta).fixTo(c.texCoordsHiFormat)
  }

  def stepVertical(dst: SpanWalker.Cursor, src: SpanWalker.Cursor): Unit = {
    dst.coords(0) := src.coords(0)
    dst.coords(1) := (src.coords(1) + one).fixTo(c.vertexFormat)
    dst.edge.zip(src.edge).zip(triangleConst.coeffs).foreach { case ((nxt, cur), coeff) =>
      nxt := (cur + coeff.b).fixTo(c.coefficientFormat)
    }
    dst.linear.all
      .zip(src.linear.all)
      .zip(triangleConst.linearDy.all)
      .zipWithIndex
      .foreach { case (((nxt, cur), grad), idx) =>
        nxt := (cur + grad).fixTo(lowGradFormats(idx))
      }
    dst.hiS := (src.hiS + triangleConst.texDy(0)).fixTo(c.texCoordsHiFormat)
    dst.hiT := (src.hiT + triangleConst.texDy(1)).fixTo(c.texCoordsHiFormat)
    dst.hiAlpha := (src.hiAlpha + triangleConst.alphaDy).fixTo(c.texCoordsHiFormat)
  }

  def stepDownLeft(dst: SpanWalker.Cursor, src: SpanWalker.Cursor): Unit = {
    dst.coords(0) := (src.coords(0) + negOne).fixTo(c.vertexFormat)
    dst.coords(1) := (src.coords(1) + one).fixTo(c.vertexFormat)
    dst.edge.zip(src.edge).zip(triangleConst.coeffs).foreach { case ((nxt, cur), coeff) =>
      nxt := (cur + coeff.b - coeff.a).fixTo(c.coefficientFormat)
    }
    dst.linear.all
      .zip(src.linear.all)
      .zip(triangleConst.linearDy.all.zip(triangleConst.linearDx.all))
      .zipWithIndex
      .foreach { case (((nxt, cur), (dy, dx)), idx) =>
        nxt := (cur + dy - dx).fixTo(lowGradFormats(idx))
      }
    dst.hiS := (src.hiS + triangleConst.texDy(0) - triangleConst.texDx(0)).fixTo(
      c.texCoordsHiFormat
    )
    dst.hiT := (src.hiT + triangleConst.texDy(1) - triangleConst.texDx(1)).fixTo(
      c.texCoordsHiFormat
    )
    dst.hiAlpha := (src.hiAlpha + triangleConst.alphaDy - triangleConst.alphaDx).fixTo(
      c.texCoordsHiFormat
    )
  }

  def prepareNextRow(base: SpanWalker.Cursor, leftBiasedGuess: Boolean): Unit = {
    val nextY = (base.coords(1) + one).fixTo(c.vertexFormat)
    when(nextY >= triangleConst.yrange(1)) {
      state := WalkerState.Idle
      firstSpanPending := False
      recoverFoundInside := False
    }.otherwise {
      if (leftBiasedGuess) {
        stepDownLeft(rowGuess, base)
        stepDownLeft(probe, base)
        stepDownLeft(bookmark, base)
        recoverFoundInside := False
        state := WalkerState.RecoverLeft
      } else {
        stepVertical(rowGuess, base)
        stepVertical(probe, base)
        stepVertical(bookmark, base)
        recoverFoundInside := False
        state := WalkerState.Decide
      }
    }
  }

  o.payload.xStart := leftEdge.coords(0)
  o.payload.xEnd := emitRight
  o.payload.y := leftEdge.coords(1)
  o.payload.linear := leftEdge.linear
  o.payload.hiS := leftEdge.hiS
  o.payload.hiT := leftEdge.hiT
  o.payload.hiAlpha := leftEdge.hiAlpha
  o.payload.stepX.linear := triangleConst.linearDx
  o.payload.stepX.tex := triangleConst.texDx
  o.payload.stepX.alpha := triangleConst.alphaDx
  o.payload.tmuConfig := triangleConst.tmuConfig
  o.payload.pixelConfig := triangleConst.pixelConfig
  o.payload.firstSpan := emitFirstSpan
  if (c.trace.enabled) {
    o.payload.trace := triangleConst.trace
  }

  when(state === WalkerState.Idle && i.fire) {
    initTriangleConst(triangleConst, i.payload)
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

  when(state === WalkerState.RecoverLeft) {
    when(insideTriangle(probe)) {
      when(!recoverFoundInside) {
        bookmark := probe
        recoverFoundInside := True
      }
      when(xPix(probe) <= visibleStartPix) {
        leftEdge := probe
        probe := bookmark
        recoverFoundInside := False
        state := WalkerState.SearchRightToExit
      }.otherwise {
        stepHorizontal(probe, probe, moveRight = false)
      }
    }.otherwise {
      when(recoverFoundInside) {
        stepHorizontal(leftEdge, probe, moveRight = true)
        probe := bookmark
        recoverFoundInside := False
        state := WalkerState.SearchRightToExit
      }.elsewhen(xPix(probe) <= visibleStartPix) {
        probe := bookmark
        state := WalkerState.SearchRightToEnter
      }.otherwise {
        stepHorizontal(probe, probe, moveRight = false)
      }
    }
  }

  when(state === WalkerState.SearchRightToEnter) {
    when(insideTriangle(probe) && xPix(probe) >= visibleStartPix) {
      leftEdge := probe
      bookmark := probe
      state := WalkerState.SearchRightToExit
    }.elsewhen(
      (!enableClipping && probe.coords(0) >= triangleConst.xrange(1)) || (enableClipping && xPix(
        probe
      ) >= visibleEndPix)
    ) {
      when(firstSpanPending) {
        prepareNextRow(rowGuess, leftBiasedGuess = false)
      }.otherwise {
        prepareNextRow(bookmark, leftBiasedGuess = true)
      }
    }.otherwise {
      stepHorizontal(probe, probe, moveRight = true)
    }
  }

  when(state === WalkerState.SearchLeftToExit) {
    when(insideTriangle(probe)) {
      when(xPix(probe) <= visibleStartPix) {
        leftEdge := probe
        probe := bookmark
        state := WalkerState.SearchRightToExit
      }.otherwise {
        stepHorizontal(probe, probe, moveRight = false)
      }
    }.otherwise {
      stepHorizontal(leftEdge, probe, moveRight = true)
      probe := bookmark
      state := WalkerState.SearchRightToExit
    }
  }

  when(state === WalkerState.SearchRightToExit) {
    when(insideTriangle(probe)) {
      when(
        (!enableClipping && probe.coords(0) >= triangleConst.xrange(1)) || (enableClipping && xPix(
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
  case class LinearState(c: Config) extends Bundle {
    val red = AFix(c.vColorFormat)
    val green = AFix(c.vColorFormat)
    val blue = AFix(c.vColorFormat)
    val depth = AFix(c.vDepthFormat)
    val alpha = AFix(c.vColorFormat)
    val w = AFix(c.wAccumFormat)

    def all: Seq[AFix] = Seq(red, green, blue, depth, alpha, w)
  }

  case class StepX(c: Config) extends Bundle {
    val linear = LinearState(c)
    val tex = Vec.fill(2)(AFix(c.texCoordsHiFormat))
    val alpha = AFix(c.texCoordsHiFormat)
  }

  case class TriangleConst(c: Config) extends Bundle {
    val coeffs = Vec.fill(3)(Coefficients(c))
    val xrange = vertex2d(c.vertexFormat)
    val yrange = vertex2d(c.vertexFormat)
    val linearDx = LinearState(c)
    val linearDy = LinearState(c)
    val texDx = Vec.fill(2)(AFix(c.texCoordsHiFormat))
    val texDy = Vec.fill(2)(AFix(c.texCoordsHiFormat))
    val alphaDx = AFix(c.texCoordsHiFormat)
    val alphaDy = AFix(c.texCoordsHiFormat)
    val tmuConfig = TriangleSetup.TmuConfig(c)
    val pixelConfig = TriangleSetup.PixelPipelineConfig(c)
    val trace = if (c.trace.enabled) Trace.PrimitiveKey() else null
  }

  case class Cursor(c: Config) extends Bundle {
    val coords = vertex2d(c.vertexFormat)
    val linear = LinearState(c)
    val hiS = AFix(c.texCoordsHiFormat)
    val hiT = AFix(c.texCoordsHiFormat)
    val hiAlpha = AFix(c.texCoordsHiFormat)
    val edge = Vec.fill(3)(AFix(c.coefficientFormat))
  }

  case class Output(c: Config) extends Bundle {
    val xStart = AFix(c.vertexFormat)
    val xEnd = AFix(c.vertexFormat)
    val y = AFix(c.vertexFormat)
    val linear = LinearState(c)
    val hiS = AFix(c.texCoordsHiFormat)
    val hiT = AFix(c.texCoordsHiFormat)
    val hiAlpha = AFix(c.texCoordsHiFormat)
    val stepX = StepX(c)
    val tmuConfig = TriangleSetup.TmuConfig(c)
    val pixelConfig = TriangleSetup.PixelPipelineConfig(c)
    val firstSpan = Bool()
    val trace = if (c.trace.enabled) Trace.PrimitiveKey() else null
  }
}
