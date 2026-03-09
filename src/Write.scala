package voodoo

import spinal.core._
import spinal.lib._

case class Write(c: Config) extends Component {
  val i = new Bundle {
    val fromPipeline = slave Stream (Write.Input(c))
  }

  val o = new Bundle {
    val fbWrite = master Stream (FramebufferPlaneCache.WriteReq(c))
  }

  o.fbWrite.translateFrom(i.fromPipeline) { (out, in) =>
    val strideSInt = (False ## in.fbPixelStride).asSInt
    val pixelFlat = (in.coords(1) * strideSInt + in.coords(0)).asUInt
    val planeAddress = (in.fbBaseAddr + (pixelFlat << 1)).resized
    val laneHi = planeAddress(1)
    val alignedAddress = (planeAddress(c.addressWidth.value - 1 downto 2) ## U"2'b00").asUInt
    val laneData = Bits(32 bits)
    when(laneHi) {
      laneData := in.data ## B(0, 16 bits)
    }.otherwise {
      laneData := B(0, 16 bits) ## in.data
    }

    out.address := alignedAddress
    out.data := laneData
    out.mask := laneHi ? B"4'b1100" | B"4'b0011"
  }
}

object Write {
  case class Input(c: Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))

    val data = Bits(16 bits)
    val fbBaseAddr = UInt(c.addressWidth)
    val fbPixelStride = UInt(11 bits)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  /** Pre-dither payload: RGB888 color + all Write.Input pass-through fields.
    *
    * Produced by triangle, fastfill, and LFB bypass paths before the shared Dither instance.
    */
  case class PreDither(c: Config) extends Bundle {
    val r = UInt(8 bits)
    val g = UInt(8 bits)
    val b = UInt(8 bits)
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val enableDithering = Bool()
    val ditherAlgorithm = Bool()
    val depthAlpha = Bits(16 bits)
    val rgbWrite = Bool()
    val auxWrite = Bool()
    val fbBaseAddr = UInt(c.addressWidth)
    val auxBaseAddr = UInt(c.addressWidth)
    val fbPixelStride = UInt(11 bits)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }
}
