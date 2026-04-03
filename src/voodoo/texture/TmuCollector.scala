package voodoo.texture

import voodoo._
import spinal.core._
import spinal.lib._

case class TmuCollector(c: voodoo.Config) extends Component {
  val io = new Bundle {
    val decoded = slave Stream (Tmu.DecodedTexel(c))
    val output = master Stream (Tmu.Output(c))
  }

  case class TexelRgba() extends Bundle {
    val r, g, b, a = UInt(8 bits)
  }

  case class PreBlend() extends Bundle {
    val requestId = cloneOf(io.decoded.payload.passthrough.requestId)
    val bilinear = Bool()
    val texels = Vec(TexelRgba(), 4)
    val weights = Tmu.BilinearWeights()
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  val collectCount = Reg(UInt(2 bits)) init 0
  val storedR = Vec(Reg(UInt(8 bits)), 4)
  val storedG = Vec(Reg(UInt(8 bits)), 4)
  val storedB = Vec(Reg(UInt(8 bits)), 4)
  val storedA = Vec(Reg(UInt(8 bits)), 4)
  val storedDs = Reg(UInt(4 bits))
  val storedDt = Reg(UInt(4 bits))

  def assignStored(slot: UInt, dec: Tmu.DecodedTexel): Unit = {
    storedR(slot) := dec.r
    storedG(slot) := dec.g
    storedB(slot) := dec.b
    storedA(slot) := dec.a
  }

  def assignTexel(dst: TexelRgba, r: UInt, g: UInt, b: UInt, a: UInt): Unit = {
    dst.r := r
    dst.g := g
    dst.b := b
    dst.a := a
  }

  val blendDs = Mux(collectCount =/= 0, storedDs, io.decoded.payload.passthrough.ds)
  val blendDt = Mux(collectCount =/= 0, storedDt, io.decoded.payload.passthrough.dt)
  val weights = Tmu.bilinearWeights(blendDs, blendDt)

  val isCollecting = collectCount =/= 0
  val isBilinear = Mux(isCollecting, True, io.decoded.payload.passthrough.bilinear)
  val bilinearAccumulating = isBilinear && (collectCount < 3)

  val preBlend = io.decoded
    .throwWhen(bilinearAccumulating)
    .translateWith {
      val result = PreBlend()
      result.requestId := io.decoded.payload.passthrough.requestId
      result.bilinear := isBilinear
      result.weights := weights
      for (idx <- 0 until 4) {
        val useCurrent = io.decoded.payload.passthrough.readIdx === U(idx, 2 bits)
        assignTexel(
          result.texels(idx),
          Mux(useCurrent, io.decoded.payload.r, storedR(idx)),
          Mux(useCurrent, io.decoded.payload.g, storedG(idx)),
          Mux(useCurrent, io.decoded.payload.b, storedB(idx)),
          Mux(useCurrent, io.decoded.payload.a, storedA(idx))
        )
      }
      if (c.trace.enabled) {
        result.trace := io.decoded.payload.passthrough.trace
      }
      result
    }
    .m2sPipe()

  io.output << preBlend.translateWith {
    val result = Tmu.Output(c)
    result.requestId := preBlend.payload.requestId
    Seq(
      result.texture.r,
      result.texture.g,
      result.texture.b,
      result.textureAlpha
    ).zipWithIndex.foreach { case (out, idx) =>
      val channelSeq = Seq(
        preBlend.payload.texels(0),
        preBlend.payload.texels(1),
        preBlend.payload.texels(2),
        preBlend.payload.texels(3)
      ).map(t => Seq(t.r, t.g, t.b, t.a)(idx))
      out := Mux(
        preBlend.payload.bilinear,
        Tmu.blendChannel(
          preBlend.payload.weights,
          channelSeq(0),
          channelSeq(1),
          channelSeq(2),
          channelSeq(3)
        ),
        channelSeq(3)
      )
    }
    if (c.trace.enabled) {
      result.trace := preBlend.payload.trace
    }
    result
  }

  when(io.decoded.fire) {
    when(isBilinear && collectCount < 3) {
      assignStored(io.decoded.payload.passthrough.readIdx, io.decoded.payload)
      when(collectCount === 0) {
        storedDs := io.decoded.payload.passthrough.ds
        storedDt := io.decoded.payload.passthrough.dt
      }
      collectCount := collectCount + 1
    }.otherwise {
      collectCount := 0
    }
  }
}
