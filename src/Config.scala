package voodoo

import spinal.core._

case class Config(
    revision: Config.Revision,
    vertexFormat: QFormat,
    coefficientFormat: QFormat,
    vColorFormat: QFormat,
    vDepthFormat: QFormat,
    texCoordsFormat: QFormat,
    wFormat: QFormat,
    maxFbDims: (Int, Int),
    fbPixelStride: Int,
    addressWidth: BitCount,
    packedTexLayout: Boolean = true
)

object Config {
  def voodoo1() = Config(
    Voodoo1(),
    vertexFormat = SQ(16, 4), // Datasheet: 12.4 format = 12 integer + 4 frac = SQ(16, 4)
    coefficientFormat =
      SQ(34, 8), // Edge coefficients: c = v0.x*v1.y - v1.x*v0.y needs 2x vertex product range
    vColorFormat = SQ(24, 12), // Datasheet: 12.12 format = 12 integer + 12 frac = SQ(24, 12)
    vDepthFormat = SQ(32, 12), // Datasheet: 20.12 format = 20 integer + 12 frac = SQ(32, 12)
    texCoordsFormat = SQ(32, 18), // Datasheet: 14.18 format = 14 integer + 18 frac = SQ(32, 18)
    wFormat = SQ(32, 30), // Datasheet: 2.30 format = 2 integer + 30 frac = SQ(32, 30)
    maxFbDims = (800, 600),
    fbPixelStride = 1024,
    addressWidth = 26 bits
  )

  sealed trait Revision;

  case class Voodoo1() extends Revision;
}
