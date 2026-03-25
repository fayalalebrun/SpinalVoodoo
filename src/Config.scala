package voodoo

import spinal.core._

case class Config(
    revision: Config.Revision,
    vertexFormat: QFormat,
    coefficientFormat: QFormat,
    vColorFormat: QFormat,
    vDepthFormat: QFormat,
    texCoordsFormat: QFormat,
    texCoordsHiFormat: QFormat,
    wFormat: QFormat,
    texCoordsAccumFormat: QFormat,
    wAccumFormat: QFormat,
    maxFbDims: (Int, Int),
    addressWidth: BitCount,
    memBurstLengthWidth: Int,
    fbWriteBufferLineWords: Int,
    fbWriteBufferCount: Int,
    texFillLineWords: Int,
    useFbWriteBuffer: Boolean,
    useTexFillCache: Boolean,
    texFillCacheSlots: Int = 16,
    texFillRequestWindow: Int = 16,
    packedTexLayout: Boolean = true,
    trace: TraceConfig = TraceConfig()
)

object Config {
  def voodoo1(trace: TraceConfig = TraceConfig()) = Config(
    Voodoo1(),
    vertexFormat = SQ(16, 4), // Datasheet: 12.4 format = 12 integer + 4 frac = SQ(16, 4)
    coefficientFormat =
      SQ(34, 8), // Edge coefficients: c = v0.x*v1.y - v1.x*v0.y needs 2x vertex product range
    vColorFormat = SQ(24, 12), // Datasheet: 12.12 format = 12 integer + 12 frac = SQ(24, 12)
    vDepthFormat = SQ(32, 12), // Datasheet: 20.12 format = 20 integer + 12 frac = SQ(32, 12)
    texCoordsFormat = SQ(32, 18), // Datasheet: 14.18 format = 14 integer + 18 frac = SQ(32, 18)
    texCoordsHiFormat = SQ(60, 30),
    wFormat = SQ(32, 30), // Datasheet: 2.30 format = 2 integer + 30 frac = SQ(32, 30)
    // Internal interpolation accumulators use a wider range than register width to avoid wrap
    // in long spans. Fractional precision remains unchanged.
    texCoordsAccumFormat = SQ(48, 18),
    wAccumFormat = SQ(48, 30),
    maxFbDims = (800, 600),
    addressWidth = 26 bits,
    memBurstLengthWidth = 6,
    fbWriteBufferLineWords = 16,
    fbWriteBufferCount = 2,
    texFillLineWords = 8,
    useFbWriteBuffer = true,
    useTexFillCache = true,
    texFillCacheSlots = 16,
    texFillRequestWindow = 16,
    trace = trace
  )

  sealed trait Revision;

  case class Voodoo1() extends Revision;
}
