package voodoo.de10

import spinal.core._
import voodoo.{Config, GenSupport, TraceConfig}

object CoreDe10SimGen extends App {
  val enableTrace = args.contains("--trace-pipeline")
  val defaultConfig = Config.voodoo1(trace = TraceConfig(enabled = enableTrace))
  val useFbWriteBuffer =
    if (args.contains("--fb-write-buffer") || args.contains("--fb-fill-cache")) true
    else if (args.contains("--no-fb-write-buffer") || args.contains("--no-fb-fill-cache")) false
    else defaultConfig.useFbWriteBuffer
  val useTexFillCache =
    if (args.contains("--tex-fill-cache")) true
    else if (args.contains("--no-tex-fill-cache")) false
    else defaultConfig.useTexFillCache

  val report = GenSupport
    .simVerilog()
    .generate(
      CoreDe10(
        defaultConfig
          .copy(
            useFbWriteBuffer = useFbWriteBuffer,
            useTexFillCache = useTexFillCache
          )
      )
    )
  GenSupport.mirrorRomSidecars("emu/sim/rtl", report.toplevelName)
}
