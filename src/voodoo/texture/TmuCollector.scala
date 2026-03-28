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
  val storedR = Vec(Reg(UInt(8 bits)), 3)
  val storedG = Vec(Reg(UInt(8 bits)), 3)
  val storedB = Vec(Reg(UInt(8 bits)), 3)
  val storedA = Vec(Reg(UInt(8 bits)), 3)
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
      assignTexel(result.texels(0), storedR(0), storedG(0), storedB(0), storedA(0))
      assignTexel(result.texels(1), storedR(1), storedG(1), storedB(1), storedA(1))
      assignTexel(result.texels(2), storedR(2), storedG(2), storedB(2), storedA(2))
      assignTexel(
        result.texels(3),
        io.decoded.payload.r,
        io.decoded.payload.g,
        io.decoded.payload.b,
        io.decoded.payload.a
      )
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
      assignStored(collectCount, io.decoded.payload)
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
