package voodoo.de10

import spinal.core._
import spinal.lib._
import spinal.lib.bus.avalon._
import spinal.lib.bus.bmb._
import voodoo.{Config, GenSupport}
import voodoo.framebuffer.FramebufferPlaneReader
import voodoo.hdmi.{HdmiCdcFramebufferScanout, HdmiScanoutPort}

case class XlsDe10Scanout(c: Config) extends Component {
  val io = new Bundle {
    val base = in UInt (c.addressWidth)
    val pixelStride = in UInt (11 bits)
    val enable = in Bool ()
    val mem = master(
      AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth))
    )
    val hdmi = master(HdmiScanoutPort(c))
  }

  val scanout = HdmiCdcFramebufferScanout(c)
  val reader = FramebufferPlaneReader(c)
  val sourceRemover = BmbSourceRemover(FramebufferPlaneReader.bmbParams(c))
  val bridge = De10BmbToAvalonMm(
    bmbParams = De10BmbToAvalonMm.singleSourceBridgeParams(
      FramebufferPlaneReader.bmbParams(c)
    ),
    avalonAddressWidth = De10MemBackend.physicalAddressWidth,
    addressBase = De10AddressMap.fbMemBase,
    addressMask = De10AddressMap.fbMemMask
  )

  scanout.io.prefetchReq <> reader.io.prefetchReq
  scanout.io.readReq <> reader.io.readReq
  scanout.io.readRsp <> reader.io.readRsp
  reader.io.mem <> sourceRemover.io.input
  sourceRemover.io.output <> bridge.io.bmb
  io.mem <> bridge.io.mem

  scanout.io.regs.frontBase := io.base
  scanout.io.regs.backBase := io.base
  scanout.io.regs.pixelStride := io.pixelStride
  scanout.io.regs.displayWidth := 640
  scanout.io.regs.displayHeight := 480
  scanout.io.regs.framebufferEnable := io.enable
  scanout.io.regs.testPatternEnable := False
  for (index <- 0 until 32) {
    val value = index * 8
    scanout.io.regs.gammaLut(index) := B((value << 16) | (value << 8) | value, 24 bits)
  }
  scanout.io.regs.gammaLut(32) := B(0xffffff, 24 bits)

  scanout.io.hdmiClock := io.hdmi.clock
  scanout.io.hdmiReset := io.hdmi.reset
  io.hdmi.video := scanout.io.video
  io.hdmi.status := scanout.io.status
  io.hdmi.underflow := scanout.io.underflow
  io.hdmi.fifoLevel := scanout.io.fifoPushOccupancy
}

object XlsDe10ScanoutGen extends App {
  GenSupport.de10Verilog().generate(XlsDe10Scanout(Config.voodoo1()))
}

object XlsDe10RtlGen extends App {
  GenSupport.de10Verilog().generate(XlsDe10Scanout(Config.voodoo1()))
  SpinalConfig(
    mode = Verilog,
    targetDirectory = "hw/de10/rtl",
    privateNamespace = true,
    defaultConfigForClockDomains = ClockDomainConfig(
      resetKind = SYNC,
      resetActiveLevel = HIGH
    )
  ).generate(XlsAvalonTransferMasterCore())
}
