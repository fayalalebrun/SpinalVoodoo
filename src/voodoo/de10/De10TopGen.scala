package voodoo.de10

import spinal.core._
import voodoo.{Config, GenSupport}

object De10TopGen extends App {
  // The checked-in DE10 platform only wires the shared framebuffer write bridge.
  // Keep board RTL on the buffered write path so color/aux writes don't rely on
  // separate write ports that the Platform Designer wrapper doesn't export.
  GenSupport.de10Verilog().generate(De10Top(Config.voodoo1().copy(useFbWriteBuffer = true)))
}
