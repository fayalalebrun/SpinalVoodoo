package voodoo

import spinal.core._
import spinal.lib._

case class TmuCollector(c: voodoo.Config) extends Component {
  val io = new Bundle {
    val decoded = slave Stream (Tmu.DecodedTexel(c))
    val output = master Stream (Tmu.Output(c))
  }

  val collectCount = Reg(UInt(2 bits)) init 0
  val storedR = Vec(Reg(UInt(8 bits)), 3)
  val storedG = Vec(Reg(UInt(8 bits)), 3)
  val storedB = Vec(Reg(UInt(8 bits)), 3)
  val storedA = Vec(Reg(UInt(8 bits)), 3)
  val storedDs = Reg(UInt(4 bits))
  val storedDt = Reg(UInt(4 bits))

  val blendDs = Mux(collectCount =/= 0, storedDs, io.decoded.payload.passthrough.ds)
  val blendDt = Mux(collectCount =/= 0, storedDt, io.decoded.payload.passthrough.dt)
  val w0 =
    (U(16, 5 bits) - blendDs.resize(5 bits)) * (U(16, 5 bits) - blendDt.resize(5 bits))
  val w1 = blendDs.resize(5 bits) * (U(16, 5 bits) - blendDt.resize(5 bits))
  val w2 = (U(16, 5 bits) - blendDs.resize(5 bits)) * blendDt.resize(5 bits)
  val w3 = blendDs.resize(5 bits) * blendDt.resize(5 bits)

  def blendChannel(t0: UInt, t1: UInt, t2: UInt, t3: UInt): UInt = {
    val sum = (t0.resize(18 bits) * w0.resize(10 bits)) +
      (t1.resize(18 bits) * w1.resize(10 bits)) +
      (t2.resize(18 bits) * w2.resize(10 bits)) +
      (t3.resize(18 bits) * w3.resize(10 bits))
    (sum >> 8).resize(8 bits)
  }

  val storedChannels = Seq(storedR, storedG, storedB, storedA)
  val decodedChannels = Seq(
    io.decoded.payload.r,
    io.decoded.payload.g,
    io.decoded.payload.b,
    io.decoded.payload.a
  )
  val blendedChannels = storedChannels.zip(decodedChannels).map { case (stored, dec) =>
    blendChannel(stored(0), stored(1), stored(2), dec)
  }

  val isCollecting = collectCount =/= 0
  val isBilinear = Mux(isCollecting, True, io.decoded.payload.passthrough.bilinear)
  val bilinearAccumulating = isBilinear && (collectCount < 3)

  io.output.valid := io.decoded.valid && !bilinearAccumulating
  io.decoded.ready := bilinearAccumulating || io.output.ready
  if (c.trace.enabled) {
    io.output.payload.trace := io.decoded.payload.passthrough.trace
  }

  Seq(
    io.output.payload.texture.r,
    io.output.payload.texture.g,
    io.output.payload.texture.b,
    io.output.payload.textureAlpha
  )
    .zip(blendedChannels)
    .zip(decodedChannels)
    .foreach { case ((out, blnd), dec) =>
      out := Mux(isBilinear, blnd, dec)
    }

  when(io.decoded.fire) {
    when(isBilinear && collectCount < 3) {
      storedChannels.zip(decodedChannels).foreach { case (stored, dec) =>
        stored(collectCount) := dec
      }
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
