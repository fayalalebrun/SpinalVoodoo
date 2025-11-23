import spinal.core._

package voodoo {
  case class Coefficients(config: Config) extends Bundle {
    // Edge coefficients need much wider range than vertices
    // Width is calculated from vertex format to handle cross products
    private val fmt = config.coefficientFormat
    val a = AFix(fmt)
    val b = AFix(fmt)
    val c = AFix(fmt)
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

  case class TriangleWithSign(fmt: QFormat) extends Bundle {
    val tri = triangle(fmt)
    val signBit = Bool() // Bit 31 from triangleCMD: 0=CCW (positive area), 1=CW (negative area)
  }

  def rgb565() = Color(UInt(5 bits), UInt(6 bits), UInt(5 bits))
}
