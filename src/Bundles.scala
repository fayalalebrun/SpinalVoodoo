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

    /** Access channels as a sequence for iteration (r, g, b order) */
    def channels: Seq[T] = Seq(r, g, b)

    /** Apply a function to each channel, returning results as a sequence */
    def map[U](f: T => U): Seq[U] = channels.map(f)

    /** Apply a function to each channel for side effects */
    def foreach(f: T => Unit): Unit = channels.foreach(f)

    /** Zip with another Color and apply a function to corresponding channels */
    def zipWith[U <: Data, V](other: Color[U])(f: (T, U) => V): Seq[V] =
      (channels, other.channels).zipped.map(f)

    /** Assign from a sequence of values (must have exactly 3 elements in r, g, b order) */
    def assignFromSeq(values: Seq[T]): Unit = {
      r := values(0)
      g := values(1)
      b := values(2)
    }
  }

  object Color {

    /** 8-bit unsigned RGB (0-255 per channel) */
    def u8() = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))

    /** 9-bit signed RGB for intermediate math (-256 to 255) */
    def s9() = Color(SInt(9 bits), SInt(9 bits), SInt(9 bits))

    /** 10-bit signed RGB after subtraction */
    def s10() = Color(SInt(10 bits), SInt(10 bits), SInt(10 bits))

    /** 18-bit signed RGB after multiplication */
    def s18() = Color(SInt(18 bits), SInt(18 bits), SInt(18 bits))

    /** Unsigned 0.8 fixed-point RGB for blend factors (0.0 to ~1.0) */
    def ufactor() = Color(AFix.UQ(0 bits, 8 bits), AFix.UQ(0 bits, 8 bits), AFix.UQ(0 bits, 8 bits))
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
