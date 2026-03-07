package voodoo

import spinal.core._

object CoreSimGen extends App {
  val enableTrace = args.contains("--trace-pipeline")
  SpinalConfig(
    mode = Verilog,
    targetDirectory = "emu/sim/rtl"
  ).generate(CoreSim(Config.voodoo1(trace = TraceConfig(enabled = enableTrace))))
}
