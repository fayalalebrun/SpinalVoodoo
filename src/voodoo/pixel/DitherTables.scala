package voodoo.pixel

object DitherTables {
  lazy val rb4x4: Array[Int] = DitherTableData.rb4x4
  lazy val g4x4: Array[Int] = DitherTableData.g4x4
  lazy val rb2x2: Array[Int] = DitherTableData.rb2x2
  lazy val g2x2: Array[Int] = DitherTableData.g2x2

  lazy val dsubRb4x4: Array[Int] = DitherTableData.dsubRb4x4
  lazy val dsubG4x4: Array[Int] = DitherTableData.dsubG4x4
  lazy val dsubRb2x2: Array[Int] = DitherTableData.dsubRb2x2
  lazy val dsubG2x2: Array[Int] = DitherTableData.dsubG2x2

  def lookupRb(v: Int, y: Int, x: Int): Int = rb4x4(v * 16 + y * 4 + x)
  def lookupG(v: Int, y: Int, x: Int): Int = g4x4(v * 16 + y * 4 + x)
  def lookupRb2x2(v: Int, y: Int, x: Int): Int = rb2x2(v * 4 + y * 2 + x)
  def lookupG2x2(v: Int, y: Int, x: Int): Int = g2x2(v * 4 + y * 2 + x)

  def lookupDsubRb(v: Int, y: Int, x: Int): Int = dsubRb4x4(v * 16 + y * 4 + x)
  def lookupDsubG(v: Int, y: Int, x: Int): Int = dsubG4x4(v * 16 + y * 4 + x)
  def lookupDsubRb2x2(v: Int, y: Int, x: Int): Int = dsubRb2x2(v * 4 + y * 2 + x)
  def lookupDsubG2x2(v: Int, y: Int, x: Int): Int = dsubG2x2(v * 4 + y * 2 + x)

  lazy val rbPacked: Array[Int] = DitherTableData.rbPacked
  lazy val gPacked: Array[Int] = DitherTableData.gPacked
  lazy val dsubRbPacked: Array[Int] = DitherTableData.dsubRbPacked
  lazy val dsubGPacked: Array[Int] = DitherTableData.dsubGPacked
}
