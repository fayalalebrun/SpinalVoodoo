package voodoo.texture

import voodoo._
import voodoo.xls.XlsTextureLayout
import spinal.core._

/** Shared combinational table computation for packed texture memory layout.
  *
  * Used by both the write path (Core drain) and read path (TMU via PerTriangleConfig). Computes
  * per-LOD base addresses and row shifts matching 86Box `voodoo_recalc_tex12`
  * (vid_voodoo_texture.c:59-144).
  */
object TexLayoutTables {

  /** Texture configuration inputs for table computation */
  case class TexConfig() extends Bundle {
    val texBaseAddr = UInt(19 bits) // register 0x30C bits [18:0]
    val texBaseAddr1 = UInt(19 bits)
    val texBaseAddr2 = UInt(19 bits)
    val texBaseAddr38 = UInt(19 bits)
    val tformat = UInt(4 bits) // textureMode bits [11:8]
    val tLOD_aspect = UInt(2 bits) // tLOD bits [22:21]
    val tLOD_sIsWider = Bool() // tLOD bit [20]
    val tLOD_multibase = Bool() // tLOD bit [24]
  }

  /** Computed per-LOD layout tables */
  case class Tables() extends Bundle {
    val texBase = Vec(UInt(22 bits), 9) // packed byte offset per LOD
    val texEnd = Vec(UInt(22 bits), 9) // packed end byte offset per LOD
    val texShift = Vec(UInt(4 bits), 9) // log2(row width in texels) per LOD
  }

  /** Compute packed texture layout tables from configuration.
    *
    * Unrolled 9-stage combinational circuit. Since dimensions are always powers of 2, width *
    * height * bpp = 1 << (wBits + hBits + is16bit), requiring only shifts and adds.
    */
  def compute(cfg: TexConfig): Tables = {
    val xls = new XlsTextureLayout
    xls.io.tex_base := cfg.texBaseAddr
    xls.io.tex_base1 := cfg.texBaseAddr1
    xls.io.tex_base2 := cfg.texBaseAddr2
    xls.io.tex_base38 := cfg.texBaseAddr38
    xls.io.format := cfg.tformat
    xls.io.aspect := cfg.tLOD_aspect
    xls.io.s_wider := cfg.tLOD_sIsWider
    xls.io.multibase := cfg.tLOD_multibase

    val tables = Tables()
    for (lod <- 0 until 9) {
      val low = (8 - lod) * 48
      tables.texBase(lod) := xls.io.out(low + 47 downto low + 26).asUInt
      tables.texEnd(lod) := xls.io.out(low + 25 downto low + 4).asUInt
      tables.texShift(lod) := xls.io.out(low + 3 downto low).asUInt
    }
    tables
  }

}
