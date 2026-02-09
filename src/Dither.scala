package voodoo

import spinal.core._

/** Combinational ordered-dither module.
  *
  * Converts 8-bit R/G/B to 5-6-5 bit using ordered dithering with either a 4x4 or 2x2 Bayer
  * matrix, matching the 86Box vid_voodoo_dither.h tables exactly.
  */
case class Dither() extends Component {
  val io = new Bundle {
    val r = in UInt (8 bits)
    val g = in UInt (8 bits)
    val b = in UInt (8 bits)
    val x = in UInt (2 bits)
    val y = in UInt (2 bits)
    val enable = in Bool ()
    val use2x2 = in Bool ()
    val ditR = out UInt (5 bits)
    val ditG = out UInt (6 bits)
    val ditB = out UInt (5 bits)
  }

  // 4x4 Bayer matrix (vertically-flipped, matching 86Box)
  val bayer4x4 = Vec(
    Vec(U(15, 4 bits), U(7, 4 bits), U(13, 4 bits), U(5, 4 bits)),
    Vec(U(3, 4 bits), U(11, 4 bits), U(1, 4 bits), U(9, 4 bits)),
    Vec(U(12, 4 bits), U(4, 4 bits), U(14, 4 bits), U(6, 4 bits)),
    Vec(U(0, 4 bits), U(8, 4 bits), U(2, 4 bits), U(10, 4 bits))
  )

  // 2x2 Bayer matrix
  val bayer2x2 = Vec(
    Vec(U(3, 4 bits), U(1, 4 bits)),
    Vec(U(0, 4 bits), U(2, 4 bits))
  )

  val threshold4x4 = bayer4x4(io.y)(io.x)
  val threshold2x2 = bayer2x2(io.y(0).asUInt)(io.x(0).asUInt)

  /** Dither a single channel.
    *
    * @param v
    *   8-bit input value
    * @param outBits
    *   output width (5 for R/B, 6 for G)
    * @param fracBits
    *   number of fractional bits (3 for 8→5, 2 for 8→6)
    * @param fracMul4x4
    *   multiplier for 4x4 mode (2 for R/B, 4 for G)
    * @return
    *   dithered output
    */
  def ditherChannel(v: UInt, outBits: Int, fracBits: Int, fracMul4x4: Int): UInt = {
    val base = v >> fracBits
    val frac = v(fracBits - 1 downto 0)
    val maxVal = (1 << outBits) - 1

    // 4x4: add = (frac * fracMul4x4 > threshold) ? 1 : 0
    val scaledFrac4x4 = (frac * fracMul4x4).resize(7 bits)
    val add4x4 = (scaledFrac4x4 > threshold4x4.resize(7 bits)).asUInt.resize(1 bits)

    // 2x2: add = (frac > threshold) ? 1 : 0
    val add2x2 = (frac.resize(7 bits) > threshold2x2.resize(7 bits)).asUInt.resize(1 bits)

    val add = io.use2x2 ? add2x2 | add4x4

    // base + add, clamped to maxVal
    val result = (base +^ add).resize(outBits + 1 bits)
    val clamped = (result > maxVal) ? U(maxVal, outBits bits) | result(outBits - 1 downto 0)

    io.enable ? clamped | (v >> fracBits).resize(outBits bits)
  }

  io.ditR := ditherChannel(io.r, 5, 3, 2)
  io.ditG := ditherChannel(io.g, 6, 2, 4)
  io.ditB := ditherChannel(io.b, 5, 3, 2)
}
