package voodoo

import spinal.core._

case class Config(
    revision: Config.Revision,
    vertexFormat: QFormat,
    vColorFormat: QFormat,
    vDepthFormat: QFormat,
    texCoordsFormat: QFormat,
    wFormat: QFormat,
    maxFbDims: (Int, Int)
)

object Config {
  def voodoo1() = Config(
    Voodoo1(),
    vertexFormat = SQ(12, 4),
    vColorFormat = SQ(12, 12),
    vDepthFormat = SQ(12, 12),
    texCoordsFormat = SQ(14, 18),
    wFormat = SQ(2, 30),
    maxFbDims = (800, 600)
  )

  sealed trait Revision;

  case class Voodoo1() extends Revision;
}
