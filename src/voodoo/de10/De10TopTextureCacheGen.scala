package voodoo.de10

import voodoo.{Config, GenSupport}

object De10TopTextureCacheGen extends App {
  val config = Config
    .voodoo1()
    .copy(
      useFbWriteBuffer = true,
      useTexFillCache = true,
      texFillLineWords = 32,
      texFillCacheSlots = 64,
      texFillWayCount = 2,
      texFillXorIndex = true,
      texFillRequestWindow = 16
    )

  GenSupport.de10Verilog().generate(De10Top(config))
}
