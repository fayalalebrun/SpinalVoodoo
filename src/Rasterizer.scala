package voodoo

import spinal.core._

case class InputGradient(fmt: QFormat) extends Bundle {
  val start = AFix(fmt)
  val d = vertex2d(fmt)
}

case class RasterizerInput() extends Bundle {}

case class Rasterizer(fmt: QFormat) extends Component {}
