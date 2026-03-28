package voodoo.pixel

import voodoo._
import spinal.core._
import spinal.lib._

case class Write(c: Config) extends Component {
  val i = new Bundle {
    val fromPipeline = slave Stream (Write.Input(c))
  }

  val o = new Bundle {
    val fbWrite = master Stream (FramebufferPlaneBuffer.WriteReq(c))
  }

  o.fbWrite.translateFrom(i.fromPipeline) { (out, in) =>
    val planeAddress =
      FramebufferAddressMath.planeAddress(
        in.fbBaseAddr,
        in.coords(0).asUInt,
        in.coords(1).asUInt,
        in.fbPixelStride
      )
    val laneHi = planeAddress(1)

    out.address := alignedWordAddress(planeAddress)
    out.data := packWordLane(in.data, laneHi)
    out.mask := wordLaneMask(laneHi)
  }
}

object Write {
  case class Input(c: Config) extends Bundle {
    val coords = PixelCoords(c)

    val data = Bits(16 bits)
    val fbBaseAddr = UInt(c.addressWidth)
    val fbPixelStride = UInt(11 bits)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  object Input {
    def colorFromDither(
        c: Config,
        pd: PreDither,
        ditOut: Dither.Output,
        trace: Trace.PixelKey = null
    ): Input = {
      val out = Input(c)
      out.coords := pd.coords
      out.data := (ditOut.ditR ## ditOut.ditG ## ditOut.ditB).asBits
      out.fbBaseAddr := pd.fbBaseAddr
      out.fbPixelStride := pd.fbPixelStride
      if (c.trace.enabled) out.trace := trace
      out
    }

    def auxFromPreDither(c: Config, pd: PreDither, trace: Trace.PixelKey = null): Input = {
      val out = Input(c)
      out.coords := pd.coords
      out.data := pd.depthAlpha
      out.fbBaseAddr := pd.auxBaseAddr
      out.fbPixelStride := pd.fbPixelStride
      if (c.trace.enabled) out.trace := trace
      out
    }
  }

  /** Pre-dither payload: RGB888 color + all Write.Input pass-through fields.
    *
    * Produced by triangle, fastfill, and LFB bypass paths before the shared Dither instance.
    */
  case class PreDither(c: Config) extends Bundle {
    val r = UInt(8 bits)
    val g = UInt(8 bits)
    val b = UInt(8 bits)
    val coords = PixelCoords(c)
    val enableDithering = Bool()
    val ditherAlgorithm = Bool()
    val depthAlpha = Bits(16 bits)
    val rgbWrite = Bool()
    val auxWrite = Bool()
    val routing = FbRouting(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null

    def fbBaseAddr: UInt = routing.colorBaseAddr
    def auxBaseAddr: UInt = routing.auxBaseAddr
    def fbPixelStride: UInt = routing.pixelStride
  }

  object PreDither {
    def fromFramebufferAccess(c: Config, fbIn: FramebufferAccess.Output): PreDither = {
      val out = PreDither(c)
      out.r := fbIn.color.r
      out.g := fbIn.color.g
      out.b := fbIn.color.b
      out.coords := fbIn.coords
      out.enableDithering := fbIn.enableDithering
      out.ditherAlgorithm := fbIn.ditherAlgorithm
      out.depthAlpha := (fbIn.enableAlphaPlanes ? fbIn.alpha.resize(16 bits) | fbIn.newDepth).asBits
      out.rgbWrite := fbIn.rgbWrite
      out.auxWrite := fbIn.auxWrite
      out.routing := fbIn.routing
      if (c.trace.enabled) out.trace := fbIn.trace
      out
    }
  }
}
