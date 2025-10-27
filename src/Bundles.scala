import spinal.core._

package voodoo {
  case class Coefficients(format: QFormat) extends Bundle {
    val a = AFix(format)
    val b = AFix(format)
    val c = AFix(format)
  }
}

package object voodoo {
  import spinal.core._

  def vertex2d(fmt: QFormat) = Vec.fill(2)(AFix(fmt))
  def triangle(fmt: QFormat) = Vec.fill(3)(vertex2d(fmt))
}
