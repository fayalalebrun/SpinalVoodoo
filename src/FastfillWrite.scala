package voodoo

import spinal.core._
import spinal.lib._

/** Fastfill → Write.PreDither conversion with register capture.
  *
  * Captures rendering registers at fastfill command fire time to avoid FIFO synchronization hazards
  * (pipelineBusyReg has a 2-cycle latency, so the FIFO can drain the next write before the busy
  * signal stalls it). On the fire cycle the live register is still valid; RegNextWhen latches it
  * for subsequent cycles.
  */
case class FastfillWrite(c: Config) extends Component {
  val io = new Bundle {
    val pixels = slave Stream (Fastfill.Output(c))
    val output = master Stream (Write.PreDither(c))

    val cmdFire = in Bool ()
    val running = in Bool ()

    val regs = in(FastfillWrite.Regs(c))
  }

  // Capture-on-fire: latch all registers at command fire, mux live vs captured
  val ffActive = RegInit(False)
  when(io.cmdFire) { ffActive := True }
  when(!io.running) { ffActive := False }

  val captured = RegNextWhen(io.regs, io.cmdFire)
  val regs = FastfillWrite.Regs(c)
  when(ffActive) { regs := captured }.otherwise { regs := io.regs }

  io.output.translateFrom(io.pixels) { (out, pxIn) =>
    out.r := regs.color1(23 downto 16).asUInt
    out.g := regs.color1(15 downto 8).asUInt
    out.b := regs.color1(7 downto 0).asUInt
    out.coords := pxIn.coords
    when(regs.fbzMode.yOrigin) {
      out.coords(1) := regs.yOriginSwapValue.resize(c.vertexFormat.nonFraction bits).asSInt - pxIn
        .coords(1)
    }
    out.enableDithering := regs.fbzMode.enableDithering
    out.ditherAlgorithm := regs.fbzMode.ditherAlgorithm
    out.depthAlpha := regs.zaColor(15 downto 0)
    out.rgbWrite := regs.fbzMode.rgbBufferMask
    out.auxWrite := regs.fbzMode.auxBufferMask
    out.fbBaseAddr := regs.drawBufferBase
    out.fbPixelStride := regs.fbPixelStride
  }
}

object FastfillWrite {
  case class Regs(c: Config) extends Bundle {
    val color1 = Bits(32 bits)
    val zaColor = Bits(32 bits)
    val fbzMode = FbzMode()
    val drawBufferBase = UInt(c.addressWidth)
    val yOriginSwapValue = UInt(10 bits)
    val fbPixelStride = UInt(11 bits)
  }
}
