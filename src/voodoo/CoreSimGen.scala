package voodoo

import spinal.core._

object CoreSimGen extends App {
  val enableTrace = args.contains("--trace-pipeline")
  def argValue(name: String): Option[String] = {
    val idx = args.indexOf(name)
    if (idx >= 0 && idx + 1 < args.length) Some(args(idx + 1)) else None
  }
  def argIntValue(name: String): Option[Int] = argValue(name).map(_.toInt)

  val defaultConfig = Config.voodoo1(trace = TraceConfig(enabled = enableTrace))
  val useFbWriteBuffer =
    if (args.contains("--fb-write-buffer") || args.contains("--fb-fill-cache")) true
    else if (args.contains("--no-fb-write-buffer") || args.contains("--no-fb-fill-cache")) false
    else defaultConfig.useFbWriteBuffer
  val useTexFillCache =
    if (args.contains("--tex-fill-cache")) true
    else if (args.contains("--no-tex-fill-cache")) false
    else defaultConfig.useTexFillCache

  val config = Config
    .voodoo1(trace = TraceConfig(enabled = enableTrace))
    .copy(
      texFillLineWords =
        argIntValue("--tex-fill-line-words").getOrElse(Config.voodoo1().texFillLineWords),
      useFbWriteBuffer = useFbWriteBuffer,
      useTexFillCache = useTexFillCache,
      texFillCacheSlots =
        argIntValue("--tex-fill-cache-slots").getOrElse(Config.voodoo1().texFillCacheSlots),
      texFillRequestWindow =
        argIntValue("--tex-fill-request-window").getOrElse(Config.voodoo1().texFillRequestWindow)
    )

  val memTiming = SimMemoryTiming(
    mode = argValue("--sim-mem-mode").getOrElse("onchip"),
    fixedReadLatency = argIntValue("--sim-mem-read-latency").getOrElse(0),
    burstSetupLatency = argIntValue("--sim-mem-burst-setup-latency").getOrElse(12),
    burstBeatLatency = argIntValue("--sim-mem-burst-beat-latency").getOrElse(1),
    writeLatency = argIntValue("--sim-mem-write-latency").getOrElse(0)
  )

  GenSupport.simVerilog().generate(CoreSim(config, memTiming))
}
