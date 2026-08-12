package voodoo.raster

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import voodoo._
import voodoo.xls.XlsSpanStep

case class SpanWalker(c: Config, formalStrong: Boolean = false) extends Component {
  val i = slave Stream (TriangleSetup.Output(c))
  val o = master Stream (SpanWalker.Output(c))
  val enableClipping = in Bool ()
  val clipLeft = in UInt (10 bits)
  val clipRight = in UInt (10 bits)
  val clipLowY = in UInt (10 bits)
  val clipHighY = in UInt (10 bits)
  val running = out(Bool())

  object WalkerState extends SpinalEnum {
    val Idle, Decide, RecoverLeft, SearchRightToEnter, SearchLeftToExit, SearchRightToExit,
        EmitSpan, AdvanceRow =
      newElement()
  }

  val state = RegInit(WalkerState.Idle)
  val triangleConst = Reg(SpanWalker.TriangleConst(c))
  val rowGuess = Reg(SpanWalker.Cursor(c))
  val probe = Reg(SpanWalker.Cursor(c))
  val bookmark = Reg(SpanWalker.Cursor(c))
  val leftEdge = Reg(SpanWalker.Cursor(c))
  val nextRowBase = Reg(SpanWalker.Cursor(c))
  val nextRowLeftBiased = Reg(Bool()) init (False)
  val emitRight = Reg(AFix(c.vertexFormat))
  val emitFirstSpan = Reg(Bool()) init (False)
  val firstSpanPending = Reg(Bool()) init (False)
  val recoverFoundInside = Reg(Bool()) init (False)

  running := state =/= WalkerState.Idle
  i.ready := state === WalkerState.Idle
  val one = AFix(c.vertexFormat)
  one := 1.0
  val zero = 0.SQ(1 bits, 0 bits)
  val vertexFracZeros = U(0, c.vertexFormat.fraction bits)

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
    dst.enableClipping := input.config.enableClipping
    dst.clipLeft := input.config.clipLeft
    dst.clipRight := input.config.clipRight
    dst.clipLowY := input.config.clipLowY
    dst.clipHighY := input.config.clipHighY
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
  val clipLeftPix = triangleConst.clipLeft.resize(c.vertexFormat.nonFraction bits).asSInt
  val clipRightPix = triangleConst.clipRight.resize(c.vertexFormat.nonFraction bits).asSInt
  val clipLowYPix = triangleConst.clipLowY.resize(c.vertexFormat.nonFraction bits).asSInt
  val clipHighYPix = triangleConst.clipHighY.resize(c.vertexFormat.nonFraction bits).asSInt
  val effectiveClipLeftPix = Mux(triangleConst.enableClipping, clipLeftPix, triangleXStartPix)
  val effectiveClipRightPix = Mux(triangleConst.enableClipping, clipRightPix, triangleXEndPix)
  val effectiveClipLowYPix =
    Mux(triangleConst.enableClipping, clipLowYPix, triangleConst.yrange(0).floor(0).asSInt)
  val effectiveClipHighYPix =
    Mux(triangleConst.enableClipping, clipHighYPix, triangleConst.yrange(1).floor(0).asSInt)
  val visibleStartPix =
    Mux(effectiveClipLeftPix > triangleXStartPix, effectiveClipLeftPix, triangleXStartPix)
  val visibleEndPix =
    Mux(effectiveClipRightPix < triangleXEndPix, effectiveClipRightPix, triangleXEndPix)
  val emitStartPix = leftEdge.coords(0).floor(0).asSInt
  val emitEndPix = emitRight.floor(0).asSInt
  val emitVisibleX =
    emitStartPix <= emitEndPix && emitEndPix >= effectiveClipLeftPix && emitStartPix < effectiveClipRightPix
  val emitVisibleY = {
    val emitYPix = leftEdge.coords(1).floor(0).asSInt
    emitYPix >= effectiveClipLowYPix && emitYPix < effectiveClipHighYPix
  }
  val emitVisible = emitVisibleX && emitVisibleY

  o.valid := state === WalkerState.EmitSpan && emitVisible

  def xlsStep(current: SInt, dx: SInt, dy: SInt, mode: Int): Bits = {
    val step = new XlsSpanStep
    step.io.current := current.resize(48 bits)
    step.io.dx := dx.resize(48 bits)
    step.io.dy := dy.resize(48 bits)
    step.io.mode := mode
    step.io.out.resize(current.getWidth bits).asBits
  }

  def stepHorizontal(dst: SpanWalker.Cursor, src: SpanWalker.Cursor, moveRight: Boolean): Unit = {
    val mode = if (moveRight) 0 else 1
    dst.coords(0).raw := xlsStep(src.coords(0).raw.asSInt, one.raw.asSInt, one.raw.asSInt, mode)
    dst.coords(1) := src.coords(1)
    dst.edge.zip(src.edge).zip(triangleConst.coeffs).foreach { case ((nxt, cur), coeff) =>
      nxt.raw := xlsStep(cur.raw.asSInt, coeff.a.raw.asSInt, coeff.a.raw.asSInt, mode)
    }
    dst.linear.all.zip(src.linear.all).zip(triangleConst.linearDx.all).foreach {
      case ((nxt, cur), grad) =>
        nxt.raw := xlsStep(cur.raw.asSInt, grad.raw.asSInt, grad.raw.asSInt, mode)
    }
    dst.hiS.raw := xlsStep(
      src.hiS.raw.asSInt,
      triangleConst.texDx(0).raw.asSInt,
      triangleConst.texDx(0).raw.asSInt,
      mode
    )
    dst.hiT.raw := xlsStep(
      src.hiT.raw.asSInt,
      triangleConst.texDx(1).raw.asSInt,
      triangleConst.texDx(1).raw.asSInt,
      mode
    )
    dst.hiAlpha.raw := xlsStep(
      src.hiAlpha.raw.asSInt,
      triangleConst.alphaDx.raw.asSInt,
      triangleConst.alphaDx.raw.asSInt,
      mode
    )
  }

  def stepVertical(dst: SpanWalker.Cursor, src: SpanWalker.Cursor): Unit = {
    dst.coords(0) := src.coords(0)
    dst.coords(1).raw := xlsStep(src.coords(1).raw.asSInt, one.raw.asSInt, one.raw.asSInt, 2)
    dst.edge.zip(src.edge).zip(triangleConst.coeffs).foreach { case ((nxt, cur), coeff) =>
      nxt.raw := xlsStep(cur.raw.asSInt, coeff.b.raw.asSInt, coeff.b.raw.asSInt, 2)
    }
    dst.linear.all.zip(src.linear.all).zip(triangleConst.linearDy.all).foreach {
      case ((nxt, cur), grad) =>
        nxt.raw := xlsStep(cur.raw.asSInt, grad.raw.asSInt, grad.raw.asSInt, 2)
    }
    dst.hiS.raw := xlsStep(
      src.hiS.raw.asSInt,
      triangleConst.texDy(0).raw.asSInt,
      triangleConst.texDy(0).raw.asSInt,
      2
    )
    dst.hiT.raw := xlsStep(
      src.hiT.raw.asSInt,
      triangleConst.texDy(1).raw.asSInt,
      triangleConst.texDy(1).raw.asSInt,
      2
    )
    dst.hiAlpha.raw := xlsStep(
      src.hiAlpha.raw.asSInt,
      triangleConst.alphaDy.raw.asSInt,
      triangleConst.alphaDy.raw.asSInt,
      2
    )
  }

  def stepDownLeft(dst: SpanWalker.Cursor, src: SpanWalker.Cursor): Unit = {
    dst.coords(0).raw := xlsStep(src.coords(0).raw.asSInt, one.raw.asSInt, one.raw.asSInt, 3)
    dst.coords(1).raw := xlsStep(src.coords(1).raw.asSInt, one.raw.asSInt, one.raw.asSInt, 2)
    dst.edge.zip(src.edge).zip(triangleConst.coeffs).foreach { case ((nxt, cur), coeff) =>
      nxt.raw := xlsStep(cur.raw.asSInt, coeff.a.raw.asSInt, coeff.b.raw.asSInt, 3)
    }
    dst.linear.all
      .zip(src.linear.all)
      .zip(triangleConst.linearDy.all.zip(triangleConst.linearDx.all))
      .foreach { case ((nxt, cur), (dy, dx)) =>
        nxt.raw := xlsStep(cur.raw.asSInt, dx.raw.asSInt, dy.raw.asSInt, 3)
      }
    dst.hiS.raw := xlsStep(
      src.hiS.raw.asSInt,
      triangleConst.texDx(0).raw.asSInt,
      triangleConst.texDy(0).raw.asSInt,
      3
    )
    dst.hiT.raw := xlsStep(
      src.hiT.raw.asSInt,
      triangleConst.texDx(1).raw.asSInt,
      triangleConst.texDy(1).raw.asSInt,
      3
    )
    dst.hiAlpha.raw := xlsStep(
      src.hiAlpha.raw.asSInt,
      triangleConst.alphaDx.raw.asSInt,
      triangleConst.alphaDy.raw.asSInt,
      3
    )
  }

  def prepareNextRow(base: SpanWalker.Cursor, leftBiasedGuess: Boolean): Unit = {
    nextRowBase := base
    nextRowLeftBiased := (if (leftBiasedGuess) True else False)
    state := WalkerState.AdvanceRow
  }

  def captureVisibleLeftEdgeFromProbe(): Unit = {
    when(xPix(probe) < visibleStartPix) {
      // Recover/search-left can overshoot by one pixel when walking clipped rows.
      // Clamp that case back onto the first visible pixel instead of preserving -1.
      stepHorizontal(leftEdge, probe, moveRight = true)
    }.otherwise {
      leftEdge := probe
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
        captureVisibleLeftEdgeFromProbe()
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
    }.elsewhen(xPix(probe) >= visibleEndPix) {
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
        captureVisibleLeftEdgeFromProbe()
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
      when(xPix(probe) >= visibleEndPix) {
        emitRight := pixelToVertex(visibleEndPix - 1)
        emitFirstSpan := firstSpanPending
        state := WalkerState.EmitSpan
      }.otherwise {
        stepHorizontal(probe, probe, moveRight = true)
      }
    }.otherwise {
      emitRight.raw := xlsStep(probe.coords(0).raw.asSInt, one.raw.asSInt, one.raw.asSInt, 1)
      emitFirstSpan := firstSpanPending
      state := WalkerState.EmitSpan
    }
  }

  when(state === WalkerState.EmitSpan && emitVisible && o.ready) {
    firstSpanPending := False
    prepareNextRow(leftEdge, leftBiasedGuess = true)
  }

  // Span discovery still reaches EmitSpan for some fully off-screen or reversed
  // rows. Drop them here so negative or out-of-range coordinates do not wrap in
  // later framebuffer address generation.
  when(state === WalkerState.EmitSpan && !emitVisible) {
    firstSpanPending := False
    prepareNextRow(leftEdge, leftBiasedGuess = true)
  }

  when(state === WalkerState.AdvanceRow) {
    val nextY = AFix(c.vertexFormat)
    nextY.raw := xlsStep(nextRowBase.coords(1).raw.asSInt, one.raw.asSInt, one.raw.asSInt, 0)
    when(nextY >= triangleConst.yrange(1)) {
      state := WalkerState.Idle
      firstSpanPending := False
      recoverFoundInside := False
    }.otherwise {
      val vertical = SpanWalker.Cursor(c)
      val downLeft = SpanWalker.Cursor(c)
      val next = SpanWalker.Cursor(c)
      stepVertical(vertical, nextRowBase)
      stepHorizontal(downLeft, vertical, moveRight = false)
      next := vertical
      when(nextRowLeftBiased) {
        next := downLeft
      }
      rowGuess := next
      probe := next
      bookmark := next
      recoverFoundInside := False
      state := WalkerState.Decide
      when(nextRowLeftBiased) {
        state := WalkerState.RecoverLeft
      }
    }
  }

  GenerationFlags.formal {
    if (formalStrong) {
      val outputStartPix = o.payload.xStart.floor(0).asSInt
      val outputEndPix = o.payload.xEnd.floor(0).asSInt
      val outputYPix = o.payload.y.floor(0).asSInt

      when(o.fire) {
        assert(outputStartPix <= outputEndPix)
        assert(outputStartPix >= effectiveClipLeftPix)
        assert(outputEndPix < effectiveClipRightPix)
        assert(outputYPix >= effectiveClipLowYPix)
        assert(outputYPix < effectiveClipHighYPix)
      }
    }
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
    val enableClipping = Bool()
    val clipLeft = UInt(10 bits)
    val clipRight = UInt(10 bits)
    val clipLowY = UInt(10 bits)
    val clipHighY = UInt(10 bits)
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
