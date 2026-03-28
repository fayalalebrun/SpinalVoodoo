package voodoo

import spinal.core._

// Color combine path enums (shared between FbzColorPath bundle and ColorCombine pipeline)

/** RGB source selection (fbzColorPath bits 1:0) */
object RgbSel extends SpinalEnum {
  val ITERATED, TEXTURE, COLOR1, LFB = newElement()
}

/** Alpha source selection (fbzColorPath bits 3:2) */
object AlphaSel extends SpinalEnum {
  val ITERATED, TEXTURE, COLOR1, LFB = newElement()
}

/** Local color selection for CCU (fbzColorPath bit 4) */
object LocalSel extends SpinalEnum {
  val ITERATED, COLOR0 = newElement()
}

/** Local alpha selection for ACU (fbzColorPath bits 6:5) */
object AlphaLocalSel extends SpinalEnum {
  val ITERATED, COLOR0, ITERATED_Z = newElement()
}

/** Multiply factor selection (fbzColorPath bits 12:10 for CCU, bits 21:19 for ACU) */
object MSelect extends SpinalEnum {
  val ZERO, CLOCAL, AOTHER, ALOCAL, TEXTURE_ALPHA, TEXTURE_RGB = newElement()
}

/** Add mode (fbzColorPath bits 15:14 for CCU, bits 24:23 for ACU) */
object AddMode extends SpinalEnum {
  val NONE, CLOCAL, ALOCAL = newElement()
}

/** SST-1 register 0x10C — Alpha test and blend control */
case class AlphaMode() extends Bundle {
  val alphaTestEnable = Bool() // [0]
  val alphaFunc = UInt(3 bits) // [3:1]
  val alphaBlendEnable = Bool() // [4]
  val enableAntiAlias = Bool() // [5]
  val reserved1 = Bits(2 bits) // [7:6]
  val rgbSrcFact = UInt(4 bits) // [11:8]
  val rgbDstFact = UInt(4 bits) // [15:12]
  val aSrcFact = UInt(4 bits) // [19:16]
  val aDstFact = UInt(4 bits) // [23:20]
  val alphaRef = UInt(8 bits) // [31:24]
}

/** SST-1 register 0x110 — Framebuffer/depth mode control */
case class FbzMode() extends Bundle {
  val enableClipping = Bool() // [0]
  val enableChromaKey = Bool() // [1]
  val enableStipple = Bool() // [2]
  val wBufferSelect = Bool() // [3]
  val enableDepthBuffer = Bool() // [4]
  val depthFunction = UInt(3 bits) // [7:5]
  val enableDithering = Bool() // [8]
  val rgbBufferMask = Bool() // [9]
  val auxBufferMask = Bool() // [10]
  val ditherAlgorithm = Bool() // [11]
  val enableStipplePattern = Bool() // [12]
  val enableAlphaMask = Bool() // [13]
  val drawBuffer = UInt(2 bits) // [15:14]
  val enableDepthBias = Bool() // [16]
  val yOrigin = Bool() // [17]
  val enableAlphaPlanes = Bool() // [18]
  val enableDitherSubtract = Bool() // [19]
  val depthSourceSelect = Bool() // [20]
}

/** SST-1 register 0x104 — Color combine path control */
case class FbzColorPath() extends Bundle {
  val rgbSel = RgbSel() // [1:0]
  val alphaSel = AlphaSel() // [3:2]
  val localSelect = LocalSel() // [4]
  val alphaLocalSelect = AlphaLocalSel() // [6:5]
  val localSelectOverride = Bool() // [7]
  val zeroOther = Bool() // [8]
  val subClocal = Bool() // [9]
  val mselect = MSelect() // [12:10]
  val reverseBlend = Bool() // [13]
  val add = AddMode() // [15:14]
  val invertOutput = Bool() // [16]
  val alphaZeroOther = Bool() // [17]
  val alphaSubClocal = Bool() // [18]
  val alphaMselect = MSelect() // [21:19]
  val alphaReverseBlend = Bool() // [22]
  val alphaAdd = AddMode() // [24:23]
  val alphaInvertOutput = Bool() // [25]
  val paramAdjust = Bool() // [26]
  val textureEnable = Bool() // [27]
}

/** SST-1 register 0x114 — Linear frame buffer mode control */
case class LfbMode() extends Bundle {
  val writeFormat = UInt(4 bits) // [3:0]
  val writeBufferSelect = UInt(2 bits) // [5:4]
  val readBufferSelect = UInt(2 bits) // [7:6]
  val pixelPipelineEnable = Bool() // [8]
  val rgbaLanes = UInt(2 bits) // [10:9]
  val wordSwapWrites = Bool() // [11]
  val byteSwizzleWrites = Bool() // [12]
  val yOrigin = Bool() // [13]
  val wSelect = Bool() // [14]
  val wordSwapReads = Bool() // [15]
  val byteSwizzleReads = Bool() // [16]
}

/** SST-1 register 0x108 — Fog mode control */
case class FogMode() extends Bundle {
  val fogEnable = Bool() // [0]
  val fogAdd = Bool() // [1]
  val fogMult = Bool() // [2]
  val fogModeSelect = UInt(2 bits) // [4:3]
  val fogConstant = Bool() // [5]
}
