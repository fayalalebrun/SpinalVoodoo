package voodoo.mister

import spinal.core._

object MisterRtlGen extends App {
  val config = SpinalConfig(
    mode = Verilog,
    privateNamespace = true,
    targetDirectory = "build/generated/scala/mister",
    defaultConfigForClockDomains = ClockDomainConfig(resetKind = SYNC, resetActiveLevel = HIGH)
  )
  config.generate(MisterAvalonToDdr())
  config.generate(MisterMemoryArbiter(8))
  config.generate(MisterFramebufferControl())
  config.generate(MisterKeyboardState())
  config.generate(MisterMailbox())
  config.generate(MisterPlatform())
  config
    .copy(defaultConfigForClockDomains = ClockDomainConfig(resetKind = BOOT))
    .generate(MisterResetBoundary())
  config
    .copy(defaultConfigForClockDomains = ClockDomainConfig(resetKind = BOOT))
    .generate(MisterCore())
}
