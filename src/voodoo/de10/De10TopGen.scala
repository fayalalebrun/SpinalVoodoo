package voodoo.de10

import spinal.core._
import voodoo.{Config, GenSupport}

object De10TopGen extends App {
  GenSupport.de10Verilog().generate(De10Top(Config.voodoo1()))
}
