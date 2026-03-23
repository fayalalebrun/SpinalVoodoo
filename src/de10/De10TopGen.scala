package voodoo.de10

import spinal.core._
import voodoo.Config

object De10TopGen extends App {
  SpinalConfig(
    mode = Verilog,
    targetDirectory = "hw/de10/rtl",
    defaultConfigForClockDomains = ClockDomainConfig(
      resetKind = ASYNC,
      resetActiveLevel = HIGH
    )
  ).generate(De10Top(Config.voodoo1()))
}
