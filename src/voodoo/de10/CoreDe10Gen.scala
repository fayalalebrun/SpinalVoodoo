package voodoo.de10

import spinal.core._
import voodoo.{Config, GenSupport}

object CoreDe10Gen extends App {
  GenSupport.de10Verilog().generate(CoreDe10(Config.voodoo1()))
}
