package voodoo.mister

import spinal.core._
import spinal.lib._
import spinal.lib.bus.avalon._
import voodoo.Config
import voodoo.de10.{De10MemBackend, H2fLwMmio}
import voodoo.hdmi.HdmiScanoutPort

case class MisterXlsRenderer() extends BlackBox {
  setDefinitionName("XlsDe10Top")
  addGeneric("EXTERNAL_SCANOUT", 1)
  val io = new Bundle {
    val externalRetrace = in Bool ()
    val h2fLw = slave(H2fLwMmio(22))
    val memFbWrite = master(AvalonMM(De10MemBackend.avalonConfig(32)))
    val memFbColorWrite = master(AvalonMM(De10MemBackend.avalonConfig(32)))
    val memFbAuxWrite = master(AvalonMM(De10MemBackend.avalonConfig(32)))
    val memFbColorRead = master(AvalonMM(De10MemBackend.avalonConfig(32)))
    val memFbAuxRead = master(AvalonMM(De10MemBackend.avalonConfig(32)))
    val memTex = master(AvalonMM(De10MemBackend.avalonConfig(32)))
    val memScanout = master(AvalonMM(De10MemBackend.avalonConfig(32)))
    val hdmi = master(HdmiScanoutPort(Config()))
    val clk = in Bool ()
    val reset = in Bool ()
  }
  io.clk.setName("clk")
  io.reset.setName("reset")
  mapCurrentClockDomain(io.clk, io.reset)
  val memories = Seq(
    io.memFbWrite,
    io.memFbColorWrite,
    io.memFbAuxWrite,
    io.memFbColorRead,
    io.memFbAuxRead,
    io.memTex,
    io.memScanout
  )
}

case class MisterPlatform(readSlots: Int = 8) extends Component {
  val io = new Bundle {
    val ddr = master(MisterDdrPort())
    val ps2Key = in Bits (11 bits)
    val osdOpen = in Bool ()
    val vblank = in Bool ()
    val frame = out(MisterFrame())
    val hdmi = master(HdmiScanoutPort(Config()))
  }
  val renderer = MisterXlsRenderer()
  val scanout = MisterScanout()
  val arbiter = MisterMemoryArbiter(renderer.memories.size + 1, readSlots)
  renderer.io.memScanout.waitRequestn := False
  renderer.io.memScanout.readData := 0
  renderer.io.memScanout.readDataValid := False
  for ((memory, index) <- renderer.memories.dropRight(1).zipWithIndex) {
    val bridge = MisterAvalonToDdr()
    bridge.io.avalon <> memory
    arbiter.io.ports(index) <> bridge.io.memory
  }
  val mailbox = MisterMailbox()
  val framebuffer = MisterFramebufferControl()
  val keyboard = MisterKeyboardState()
  arbiter.io.ports.last <> mailbox.io.memory
  arbiter.io.ports(renderer.memories.size - 1) <> scanout.io.memory
  scanout.io.frame := framebuffer.io.active
  scanout.io.pixelClock := io.hdmi.clock
  scanout.io.pixelReset := io.hdmi.reset
  renderer.io.externalRetrace := scanout.io.newFrame
  renderer.io.hdmi.clock := io.hdmi.clock
  renderer.io.hdmi.reset := io.hdmi.reset
  renderer.io.h2fLw <> mailbox.io.mmio
  framebuffer.io.submit << mailbox.io.frame
  mailbox.io.frameAck << framebuffer.io.acknowledge
  framebuffer.io.vblank := scanout.io.boundary
  keyboard.io.ps2Key := io.ps2Key
  keyboard.io.osdOpen := io.osdOpen
  mailbox.io.keys := keyboard.io.keys
  mailbox.io.keySequence := keyboard.io.sequence
  io.frame := framebuffer.io.active
  io.ddr <> arbiter.io.ddr
  io.hdmi.video := scanout.io.video
  io.hdmi.status := renderer.io.hdmi.status
  io.hdmi.underflow := scanout.io.underflow
  io.hdmi.fifoLevel := 0
}
