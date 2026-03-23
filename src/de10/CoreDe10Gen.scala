package voodoo.de10

import spinal.core._
import voodoo.Config

object CoreDe10Gen extends App {
  SpinalConfig(
    mode = Verilog,
    targetDirectory = "hw/de10/rtl",
    defaultConfigForClockDomains = ClockDomainConfig(
      resetKind = ASYNC,
      resetActiveLevel = HIGH
    )
  ).generate(CoreDe10(Config.voodoo1()))
}
