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

  val defaultConfig: Config =
    Config
      .voodoo1(trace = TraceConfig(enabled = enableTrace))
      .copy(
        useFbWriteBuffer = true,
        fbWriteBufferLineWords = 256,
        useFbReadCache = false,
        texFillLineWords = 64,
        texFillCacheSlots = 64,
        texFillWayCount = 2,
        texFillXorIndex = true
      )
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
  val enableTmu = !args.contains("--no-tmu")
  val enableFog = !args.contains("--no-fog")
  val enableDither = !args.contains("--no-dither")
  val enableChromaKey = !args.contains("--no-chroma-key")
  val enableAlphaTest = !args.contains("--no-alpha-test")

  val report = GenSupport
    .simVerilog()
    .generate(
      CoreDe10(
        defaultConfig
          .copy(
            useFbWriteBuffer = useFbWriteBuffer,
            useFbReadCache = useFbReadCache,
            useTexFillCache = useTexFillCache,
            enableTmu = enableTmu,
            enableFog = enableFog,
            enableDither = enableDither,
            enableChromaKey = enableChromaKey,
            enableAlphaTest = enableAlphaTest,
            texFillLineWords =
              argIntValue("--tex-fill-line-words").getOrElse(defaultConfig.texFillLineWords),
            texFillCacheSlots =
              argIntValue("--tex-fill-cache-slots").getOrElse(defaultConfig.texFillCacheSlots),
            texFillWayCount =
              argIntValue("--tex-fill-way-count").getOrElse(defaultConfig.texFillWayCount),
            texFillXorIndex = !args.contains("--no-tex-fill-xor-index"),
            texFillRequestWindow = argIntValue("--tex-fill-request-window")
              .getOrElse(defaultConfig.texFillRequestWindow)
          )
      )
    )
  GenSupport.mirrorRomSidecars("emu/sim/rtl", report.toplevelName)
}
