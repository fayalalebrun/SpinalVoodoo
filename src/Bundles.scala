import spinal.core._

package voodoo {
  case class Coefficients(format: QFormat) extends Bundle {
    val a = AFix(format)
    val b = AFix(format)
    val c = AFix(format)
  }

  case class Color[T <: Data](rt: HardType[T], gt: HardType[T], bt: HardType[T]) extends Bundle {
    // Fields are packed in declaration order with later fields at higher bits
    // So declare in reverse order: b, g, r to get standard RGB565 bit layout [R:5][G:6][B:5]
    val b = bt()
    val g = gt()
    val r = rt()
  }
}

package object voodoo {
  import spinal.core._

  def vertex2d(fmt: QFormat) = Vec.fill(2)(AFix(fmt))
  def triangle(fmt: QFormat) = Vec.fill(3)(vertex2d(fmt))
  def rgb565() = Color(UInt(5 bits), UInt(6 bits), UInt(5 bits))
}
