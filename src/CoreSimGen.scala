package voodoo

import spinal.core._

object CoreSimGen extends App {
  SpinalConfig(
    mode = Verilog,
    targetDirectory = "emu/sim/rtl"
  ).generate(CoreSim(Config.voodoo1()))
}
