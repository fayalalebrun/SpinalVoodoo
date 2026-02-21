package voodoo

import spinal.core._
import spinal.lib._

/** Fastfill → Write.Input conversion with register capture.
  *
  * Captures rendering registers at fastfill command fire time to avoid FIFO synchronization hazards
  * (pipelineBusyReg has a 2-cycle latency, so the FIFO can drain the next write before the busy
  * signal stalls it). On the fire cycle the live register is still valid; RegNextWhen latches it
  * for subsequent cycles.
  */
case class FastfillWrite(c: Config) extends Component {
  val io = new Bundle {
    val pixels = slave Stream (Fastfill.Output(c))
    val output = master Stream (Write.Input(c))

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

  io.output.translateFrom(io.pixels) { (out, in) =>
    out.coords := in.coords
    when(regs.fbzMode.yOrigin) {
      out.coords(1) := regs.yOriginSwapValue.resize(c.vertexFormat.nonFraction bits).asSInt - in
        .coords(1)
    }

    // Dither color1 RGB to 5-6-5
    val dither = Dither()
    dither.io.r := regs.color1(23 downto 16).asUInt
    dither.io.g := regs.color1(15 downto 8).asUInt
    dither.io.b := regs.color1(7 downto 0).asUInt
    dither.io.x := in.coords(0).asUInt.resize(2 bits)
    dither.io.y := in.coords(1).asUInt.resize(2 bits)
    dither.io.enable := regs.fbzMode.enableDithering
    dither.io.use2x2 := regs.fbzMode.ditherAlgorithm

    val fbWord = cloneOf(out.toFb)
    fbWord.color.r := dither.io.ditR
    fbWord.color.g := dither.io.ditG
    fbWord.color.b := dither.io.ditB
    fbWord.depthAlpha := regs.zaColor(15 downto 0)

    out.toFb := fbWord
    out.rgbWrite := regs.fbzMode.rgbBufferMask
    out.auxWrite := regs.fbzMode.auxBufferMask
    out.fbBaseAddr := regs.drawBufferBase
  }
}

object FastfillWrite {
  case class Regs(c: Config) extends Bundle {
    val color1 = Bits(32 bits)
    val zaColor = Bits(32 bits)
    val fbzMode = FbzMode()
    val drawBufferBase = UInt(c.addressWidth)
    val yOriginSwapValue = UInt(10 bits)
  }
}
