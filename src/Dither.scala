package voodoo

import spinal.core._

/** Combinational ordered-dither module.
  *
  * Converts 8-bit R/G/B to 5-6-5 bit using ordered dithering with either a 4x4 or 2x2 Bayer matrix,
  * matching the 86Box vid_voodoo_dither.h lookup tables exactly.
  *
  * Uses Vec-based constant lookup tables from the ground-truth 86Box tables rather than an
  * algorithmic approximation.
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

  // Constant lookup tables as Vec (synthesizes to mux/ROM without $readmemb)
  val rb4x4Lut = Vec(DitherTables.rb4x4.map(v => U(v, 5 bits)))
  val g4x4Lut = Vec(DitherTables.g4x4.map(v => U(v, 6 bits)))
  val rb2x2Lut = Vec(DitherTables.rb2x2.map(v => U(v, 5 bits)))
  val g2x2Lut = Vec(DitherTables.g2x2.map(v => U(v, 6 bits)))

  // 4x4 addresses: v(8) ## y(2) ## x(2) = 12 bits
  def addr4x4(v: UInt): UInt = (v ## io.y ## io.x).asUInt

  // 2x2 addresses: v(8) ## y(1) ## x(1) = 10 bits
  def addr2x2(v: UInt): UInt = (v ## io.y(0) ## io.x(0)).asUInt

  // Lookups
  val r4x4 = rb4x4Lut(addr4x4(io.r))
  val g4x4 = g4x4Lut(addr4x4(io.g))
  val b4x4 = rb4x4Lut(addr4x4(io.b))

  val r2x2 = rb2x2Lut(addr2x2(io.r))
  val g2x2 = g2x2Lut(addr2x2(io.g))
  val b2x2 = rb2x2Lut(addr2x2(io.b))

  // Select mode
  val ditR = io.use2x2 ? r2x2 | r4x4
  val ditG = io.use2x2 ? g2x2 | g4x4
  val ditB = io.use2x2 ? b2x2 | b4x4

  // Bypass: simple truncation when dithering is disabled
  val truncR = (io.r >> 3).resize(5 bits)
  val truncG = (io.g >> 2).resize(6 bits)
  val truncB = (io.b >> 3).resize(5 bits)

  io.ditR := io.enable ? ditR | truncR
  io.ditG := io.enable ? ditG | truncG
  io.ditB := io.enable ? ditB | truncB
}
