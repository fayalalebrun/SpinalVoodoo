package voodoo

import scala.io.Source
import scala.util.matching.Regex

/** Parses the 86Box vid_voodoo_dither.h lookup tables at elaboration/test time.
  *
  * Provides the ground-truth dithering data as flat arrays indexed by (v, y, x) for ROM
  * initialization (HDL) and direct lookup (reference model).
  */
object DitherTables {

  /** Path to the 86Box dither header relative to the project root. */
  private val HeaderPath = "emu/86Box/src/include/86box/vid_voodoo_dither.h"

  private lazy val headerContent: String = {
    val path = new java.io.File(HeaderPath)
    val absPath =
      if (path.isAbsolute) path
      else {
        // Try common working directories
        val candidates = Seq(
          new java.io.File(HeaderPath),
          new java.io.File(System.getProperty("user.dir"), HeaderPath)
        )
        candidates
          .find(_.exists())
          .getOrElse(
            sys.error(s"Cannot find $HeaderPath from ${System.getProperty("user.dir")}")
          )
      }
    Source.fromFile(absPath).mkString
  }

  /** Parse a C array declaration like `dither_rb[256][4][4]` into a flat array.
    *
    * For a `[256][R][C]` table, returns a flat `Array[Int]` of length `256 * R * C`, indexed as
    * `table(v * R * C + y * C + x)`.
    */
  private def parseTable(name: String, rows: Int, cols: Int): Array[Int] = {
    val pattern =
      s"""static const uint8_t ${Regex.quote(name)}\\[256\\]\\[$rows\\]\\[$cols\\] = \\{"""
    val regex = pattern.r
    val m = regex
      .findFirstMatchIn(headerContent)
      .getOrElse(
        sys.error(s"Table $name not found in header")
      )

    val result = new Array[Int](256 * rows * cols)
    var pos = m.end
    for (v <- 0 until 256) {
      pos = headerContent.indexOf('{', pos) + 1 // outer { for this entry
      for (row <- 0 until rows) {
        pos = headerContent.indexOf('{', pos) + 1
        val end = headerContent.indexOf('}', pos)
        val vals = headerContent.substring(pos, end).split(',').map(_.trim.toInt)
        for (col <- 0 until cols) {
          result(v * rows * cols + row * cols + col) = vals(col)
        }
        pos = end + 1
      }
      pos = headerContent.indexOf('}', pos) + 1 // closing } of entry
    }
    result
  }

  /** 4x4 R/B dither table: 256 × 4 × 4 = 4096 entries. Index: v*16 + y*4 + x. Values 0-31. */
  lazy val rb4x4: Array[Int] = parseTable("dither_rb", 4, 4)

  /** 4x4 G dither table: 256 × 4 × 4 = 4096 entries. Index: v*16 + y*4 + x. Values 0-63. */
  lazy val g4x4: Array[Int] = parseTable("dither_g", 4, 4)

  /** 2x2 R/B dither table: 256 × 2 × 2 = 1024 entries. Index: v*4 + y*2 + x. Values 0-31. */
  lazy val rb2x2: Array[Int] = parseTable("dither_rb2x2", 2, 2)

  /** 2x2 G dither table: 256 × 2 × 2 = 1024 entries. Index: v*4 + y*2 + x. Values 0-63. */
  lazy val g2x2: Array[Int] = parseTable("dither_g2x2", 2, 2)

  /** Lookup a 4x4 R/B dither value. */
  def lookupRb(v: Int, y: Int, x: Int): Int = rb4x4(v * 16 + y * 4 + x)

  /** Lookup a 4x4 G dither value. */
  def lookupG(v: Int, y: Int, x: Int): Int = g4x4(v * 16 + y * 4 + x)

  /** Lookup a 2x2 R/B dither value. */
  def lookupRb2x2(v: Int, y: Int, x: Int): Int = rb2x2(v * 4 + y * 2 + x)

  /** Lookup a 2x2 G dither value. */
  def lookupG2x2(v: Int, y: Int, x: Int): Int = g2x2(v * 4 + y * 2 + x)

  /** Packed ROM init: 4x4 table (4096 entries) followed by 2x2 table (1024 entries) = 5120 total.
    */
  lazy val rbPacked: Array[Int] = rb4x4 ++ rb2x2
  lazy val gPacked: Array[Int] = g4x4 ++ g2x2
}
