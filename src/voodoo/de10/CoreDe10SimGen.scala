package voodoo.de10

import spinal.core._
import voodoo.{Config, GenSupport, TraceConfig}

object CoreDe10SimGen extends App {
  val enableTrace = args.contains("--trace-pipeline")
  def argValue(name: String): Option[String] = {
    val idx = args.indexOf(name)
    if (idx >= 0 && idx + 1 < args.length) Some(args(idx + 1)) else None
  }
  def argIntValue(name: String): Option[Int] = argValue(name).map(_.toInt)

  val defaultConfig = Config.voodoo1(trace = TraceConfig(enabled = enableTrace))
  val useFbWriteBuffer =
    if (args.contains("--no-fb-write-buffer")) false
    else defaultConfig.useFbWriteBuffer
  val useFbReadCache =
    if (args.contains("--fb-fill-cache")) true
    else if (args.contains("--no-fb-fill-cache")) false
    else defaultConfig.useFbReadCache
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
            useFbReadCache = useFbReadCache,
            useTexFillCache = useTexFillCache,
            texFillLineWords =
              argIntValue("--tex-fill-line-words").getOrElse(Config.voodoo1().texFillLineWords),
            texFillCacheSlots =
              argIntValue("--tex-fill-cache-slots").getOrElse(Config.voodoo1().texFillCacheSlots),
            texFillWayCount =
              argIntValue("--tex-fill-way-count").getOrElse(Config.voodoo1().texFillWayCount),
            texFillXorIndex = args.contains("--tex-fill-xor-index"),
            texFillRequestWindow = argIntValue("--tex-fill-request-window")
              .getOrElse(Config.voodoo1().texFillRequestWindow)
          )
      )
    )
  GenSupport.mirrorRomSidecars("emu/sim/rtl", report.toplevelName)
}
