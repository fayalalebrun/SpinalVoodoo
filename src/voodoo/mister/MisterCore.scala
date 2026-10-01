package voodoo.mister

import spinal.core._
import spinal.lib._
import voodoo.hdmi.ParallelRgbVideo

case class MisterCore(readSlots: Int = 8) extends Component {
  val io = new Bundle {
    val ddr = master(MisterDdrPort())
    val resetRequest = in Bool ()
    val pixelClock = in Bool ()
    val ps2Key = in Bits (11 bits)
    val osdOpen = in Bool ()
    val vblank = in Bool ()
    val frame = out(MisterFrame())
    val video = out(ParallelRgbVideo())
    val resetting = out Bool ()
    val underflow = out Bool ()
  }
  val boundary = MisterResetBoundary()
  boundary.io.request := io.resetRequest
  io.ddr <> boundary.io.ddr
  io.resetting := boundary.io.coreReset
  val pixel = new ClockingArea(
    ClockDomain(clock = io.pixelClock, config = ClockDomainConfig(resetKind = BOOT))
  ) {
    val reset = BufferCC(boundary.io.coreReset, True)
  }
  val core = new ClockingArea(
    ClockDomain.current
      .copy(reset = boundary.io.coreReset, config = ClockDomainConfig(resetKind = SYNC))
  ) {
    val platform = MisterPlatform(readSlots)
    boundary.io.core <> platform.io.ddr
    platform.io.ps2Key := io.ps2Key
    platform.io.osdOpen := io.osdOpen
    platform.io.vblank := io.vblank
    platform.io.hdmi.clock := io.pixelClock
    platform.io.hdmi.reset := pixel.reset
    io.frame := platform.io.frame
    io.video := platform.io.hdmi.video
    io.underflow := platform.io.hdmi.underflow
  }
}
