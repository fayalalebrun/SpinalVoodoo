package voodoo

import spinal.core._

object GenSupport {
  def simVerilog(targetDirectory: String = "emu/sim/rtl"): SpinalConfig =
    SpinalConfig(mode = Verilog, targetDirectory = targetDirectory)

  def de10Verilog(targetDirectory: String = "hw/de10/rtl"): SpinalConfig =
    SpinalConfig(
      mode = Verilog,
      targetDirectory = targetDirectory,
      defaultConfigForClockDomains = ClockDomainConfig(
        resetKind = ASYNC,
        resetActiveLevel = HIGH
      )
    )
}
