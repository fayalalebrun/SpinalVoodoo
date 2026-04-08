package voodoo.de10

import voodoo.{Config, GenSupport}

object De10TopTextureCacheGen extends App {
  val config = Config
    .voodoo1()
    .copy(
      useTexFillCache = true,
      texFillLineWords = 8,
      texFillCacheSlots = 16,
      texFillRequestWindow = 16
    )

  GenSupport.de10Verilog().generate(De10Top(config))
}
