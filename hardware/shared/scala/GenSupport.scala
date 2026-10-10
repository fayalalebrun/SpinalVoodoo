package voodoo

import spinal.core._

object GenSupport {
  def de10Verilog(targetDirectory: String = "build/generated/scala/de10"): SpinalConfig =
    SpinalConfig(
      mode = Verilog,
      targetDirectory = targetDirectory,
      defaultConfigForClockDomains = ClockDomainConfig(
        resetKind = ASYNC,
        resetActiveLevel = HIGH
      )
    )
}
