package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._

case class Write(c: Config) extends Component {
  val i = new Bundle {
    val fromPipeline = slave Stream (Write.Input(c))
  }

  val o = new Bundle {
    val fbWrite = master(Bmb(Write.baseBmbParams(c)))
  }

  o.fbWrite.cmd.translateFrom(i.fromPipeline) { (out, in) =>
    val pixelFlat = (in.coords(1) * c.fbPixelStride + in.coords(0)).asUInt

    out.fragment.address := (in.fbBaseAddr + (pixelFlat << 2)).resized
    out.fragment.data := in.toFb.asBits.resized
    out.fragment.opcode := Bmb.Cmd.Opcode.WRITE
    out.fragment.length := 3 // 4 bytes - 1
    out.fragment.source := 0 // Single source
    // bytes 0-1 = RGB565 color, bytes 2-3 = depth/alpha
    out.fragment.mask := in.auxWrite ## in.auxWrite ## in.rgbWrite ## in.rgbWrite
    out.last := True
  }

  // BMB response is always ready (fire and forget writes)
  o.fbWrite.rsp.ready := True
}

object Write {
  case class Input(c: Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))

    val toFb = FbWord()
    val rgbWrite = Bool()
    val auxWrite = Bool()
    val fbBaseAddr = UInt(c.addressWidth)
  }

  case class FbWord() extends Bundle {
    val color = rgb565()
    val depthAlpha = Bits(16 bits)
  }

  def baseBmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = 2, // Need at least 2 bits for length=3 (4 bytes)
    canRead = false,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}
