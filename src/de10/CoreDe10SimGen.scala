package voodoo.de10

import spinal.core._
import voodoo.Config

object CoreDe10SimGen extends App {
  SpinalConfig(
    mode = Verilog,
    targetDirectory = "emu/sim/rtl"
  ).generate(CoreDe10(Config.voodoo1()))
}
