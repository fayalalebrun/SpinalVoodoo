package voodoo.texture

import voodoo._
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
    val tables = Tables()

    val is16bit = cfg.tformat >= Tmu.TextureFormat.ARGB8332

    // Base address in bytes: texBaseAddr << 3
    val base0 = (cfg.texBaseAddr << 3).resize(22 bits)
    val base1 = (cfg.texBaseAddr1 << 3).resize(22 bits)
    val base2 = (cfg.texBaseAddr2 << 3).resize(22 bits)
    val base38 = (cfg.texBaseAddr38 << 3).resize(22 bits)

    // Unrolled loop: compute per-LOD base and shift
    // We track offset as a UInt and accumulate (1 << (wBits + hBits + is16bit))
    var offsetExpr: UInt = U(0, 22 bits)
    var multibaseTailOffset: UInt = U(0, 22 bits)
    for (lod <- 0 until 9) {
      // Dimensions are powers of two. Aspect selects the narrower side reduction,
      // while tLOD_sIsWider chooses whether S or T is the wider dimension.
      val wideBits = U(scala.math.max(8 - lod, 0), 4 bits)
      val narrowBits = UInt(4 bits)
      switch(cfg.tLOD_aspect) {
        is(U(0, 2 bits)) {
          narrowBits := wideBits
        }
        is(U(1, 2 bits)) {
          narrowBits := U(scala.math.max(7 - lod, 0), 4 bits)
        }
        is(U(2, 2 bits)) {
          narrowBits := U(scala.math.max(6 - lod, 0), 4 bits)
        }
        default {
          narrowBits := U(scala.math.max(5 - lod, 0), 4 bits)
        }
      }

      val wBits = UInt(4 bits)
      val hBits = UInt(4 bits)
      when(cfg.tLOD_sIsWider) {
        wBits := wideBits
        hBits := narrowBits
      } otherwise {
        wBits := narrowBits
        hBits := wideBits
      }

      val lodBase = UInt(22 bits)
      lodBase := base0 + offsetExpr
      if (lod == 1) {
        when(cfg.tLOD_multibase) {
          lodBase := base1
        }
      }
      if (lod == 2) {
        when(cfg.tLOD_multibase) {
          lodBase := base2
        }
      }
      if (lod >= 3) {
        when(cfg.tLOD_multibase) {
          lodBase := base38 + multibaseTailOffset
        }
      }
      tables.texBase(lod) := lodBase
      tables.texShift(lod) := cfg.tLOD_sIsWider ? wideBits | narrowBits

      // LOD area in bytes = 1 << (wBits + hBits + is16bit)
      // We compute this as a shift amount and add to offset for next LOD
      val shiftAmount = (wBits +^ hBits +^ is16bit.asUInt.resize(5 bits)).resize(5 bits)
      val lodSize = (U(1, 22 bits) |<< shiftAmount).resize(22 bits)
      tables.texEnd(lod) := (lodBase + lodSize).resize(22 bits)
      offsetExpr = (offsetExpr + lodSize).resize(22 bits)
      if (lod >= 3) {
        multibaseTailOffset = (multibaseTailOffset + lodSize).resize(22 bits)
      }
    }

    tables
  }
}
