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

    // Starting dimension bits: 256x256 base (8 bits each)
    // Aspect ratio reduces the narrower dimension
    val wBitsStart = UInt(4 bits)
    val hBitsStart = UInt(4 bits)
    wBitsStart := 8
    hBitsStart := 8
    when(cfg.tLOD_sIsWider) {
      hBitsStart := (U(8, 4 bits) - cfg.tLOD_aspect.resize(4 bits)).resized
    } otherwise {
      wBitsStart := (U(8, 4 bits) - cfg.tLOD_aspect.resize(4 bits)).resized
    }

    // Unrolled loop: compute per-LOD base and shift
    // We track offset as a UInt and accumulate (1 << (wBits + hBits + is16bit))
    var offsetExpr: UInt = U(0, 22 bits)
    var multibaseTailOffset: UInt = U(0, 22 bits)
    for (lod <- 0 until 9) {
      // wBits and hBits at this LOD level (clamped to >= 0)
      val wBits = UInt(4 bits)
      val hBits = UInt(4 bits)
      if (lod == 0) {
        wBits := wBitsStart
        hBits := hBitsStart
      } else {
        when(wBitsStart > U(lod, 4 bits)) {
          wBits := (wBitsStart - U(lod, 4 bits)).resized
        } otherwise {
          wBits := 0
        }
        when(hBitsStart > U(lod, 4 bits)) {
          hBits := (hBitsStart - U(lod, 4 bits)).resized
        } otherwise {
          hBits := 0
        }
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
      tables.texShift(lod) := wBits

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
