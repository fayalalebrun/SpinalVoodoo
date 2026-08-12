package voodoo.pixel

import voodoo._
import voodoo.core.PixelPipeline
import voodoo.xls.XlsColorCombine
import spinal.core._
import spinal.lib._

/** Color Combine Unit types */
object ColorCombine {
  // Re-export enums from package level so ColorCombine.RgbSel etc. still work
  val RgbSel = voodoo.RgbSel
  val AlphaSel = voodoo.AlphaSel
  val LocalSel = voodoo.LocalSel
  val AlphaLocalSel = voodoo.AlphaLocalSel
  val MSelect = voodoo.MSelect
  val AddMode = voodoo.AddMode

  /** Configuration bundle for Color Combine Unit — subset of FbzColorPath fields. All fields share
    * names and types with FbzColorPath, enabling assignSomeByName.
    */
  case class Config() extends Bundle {
    // RGB channel controls
    val rgbSel = voodoo.RgbSel()
    val localSelect = voodoo.LocalSel()
    val localSelectOverride = Bool()
    val zeroOther = Bool()
    val subClocal = Bool()
    val mselect = voodoo.MSelect()
    val reverseBlend = Bool()
    val add = voodoo.AddMode()
    val invertOutput = Bool()

    // Alpha channel controls
    val alphaSel = voodoo.AlphaSel()
    val alphaLocalSelect = voodoo.AlphaLocalSel()
    val alphaZeroOther = Bool()
    val alphaSubClocal = Bool()
    val alphaMselect = voodoo.MSelect()
    val alphaReverseBlend = Bool()
    val alphaAdd = voodoo.AddMode()
    val alphaInvertOutput = Bool()

    // Texture enable
    val textureEnable = Bool()
  }

  object Config {
    def fromFbzColorPath(fcp: FbzColorPath): Config = {
      val cfg = Config()
      cfg.assignSomeByName(fcp)
      cfg
    }
  }

  /** Input to Color Combine Unit */
  case class Input(c: voodoo.Config) extends Bundle {
    val coords = PixelCoords(c)
    val iterated = Color.u8() // Iterated (Gouraud) colors, 0-255
    val iteratedAlpha = UInt(8 bits) // Iterated alpha, 0-255
    val iteratedZ = UInt(8 bits) // Upper bits of Z for alpha local select
    val depth = AFix(c.vDepthFormat)
    val rawW = SInt(32 bits) // Raw W value for fog wDepth calculation

    // Texture colors from TMU chain
    val texture = Color.u8()
    val textureAlpha = UInt(8 bits)

    // Constant colors from registers
    val color0 = Color.u8()
    val color1 = Color.u8()
    val color0Alpha = UInt(8 bits) // Alpha byte from color0 register (bits 31:24)
    val color1Alpha = UInt(8 bits) // Alpha byte from color1 register (bits 31:24)

    // Fog color (per-triangle, passed through to Fog stage)
    val fogColor = Bits(32 bits)
    val chromaKey = Bits(32 bits)
    val zaColor = Bits(32 bits)

    // Configuration (decoded enums)
    val config = ColorCombine.Config()

    // Per-triangle FIFO registers (captured at triangle command time, must travel with pixels)
    val alphaMode = AlphaMode()
    val fogMode = FogMode()
    val fbzMode = FbzMode()
    val routing = FbRouting(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null

    def drawColorBufferBase: UInt = routing.colorBaseAddr
    def drawAuxBufferBase: UInt = routing.auxBaseAddr
    def fbPixelStride: UInt = routing.pixelStride
  }

  object Input {
    private def u8FromSat(v: AFix): UInt = (v.asBits >> 12).asUInt.resize(8 bits)

    def fromTmuAndRaster(
        c: voodoo.Config,
        tmuOut: Tmu.Output,
        rasterOut: Rasterizer.PostTmu,
        sync: PixelPipeline.SyncRender
    ): Input = {
      val out = Input(c)
      val redSat = rasterOut.grads.redGrad.sat(satMax = 255, satMin = 0, exp = 0 exp)
      val greenSat = rasterOut.grads.greenGrad.sat(satMax = 255, satMin = 0, exp = 0 exp)
      val blueSat = rasterOut.grads.blueGrad.sat(satMax = 255, satMin = 0, exp = 0 exp)
      val alphaSat = rasterOut.alphaGradHi.sat(satMax = 255, satMin = 0, exp = 0 exp)
      val depthSat = rasterOut.grads.depthGrad.sat(satMax = 255, satMin = 0, exp = 12 exp)

      out.coords.assignFromVec(rasterOut.coords)
      out.iterated.r := u8FromSat(redSat)
      out.iterated.g := u8FromSat(greenSat)
      out.iterated.b := u8FromSat(blueSat)
      out.iteratedAlpha := (alphaSat.asBits >> 30).asUInt.resize(8 bits)
      out.iteratedZ := u8FromSat(depthSat)
      out.depth := rasterOut.grads.depthGrad
      out.rawW := rasterOut.grads.wGrad.asSInt.resize(32 bits)
      out.texture := tmuOut.texture
      out.textureAlpha := tmuOut.textureAlpha
      assignRgb888(out.color0, sync.color0)
      out.color0Alpha := sync.color0(31 downto 24).asUInt
      assignRgb888(out.color1, sync.color1)
      out.color1Alpha := sync.color1(31 downto 24).asUInt
      out.fogColor := sync.fogColor
      out.chromaKey := sync.chromaKey
      out.zaColor := sync.zaColor
      out.config := Config.fromFbzColorPath(rasterOut.config.fbzColorPath)
      out.alphaMode := rasterOut.config.alphaMode
      out.fogMode := rasterOut.config.fogMode
      out.fbzMode := sync.fbzMode
      out.routing := sync.routing
      if (c.trace.enabled) out.trace := rasterOut.trace
      out
    }
  }

  /** Output from Color Combine Unit */
  case class Output(c: voodoo.Config) extends Bundle {
    val coords = PixelCoords(c)
    val color = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val alpha = UInt(8 bits)
    val depth = AFix(c.vDepthFormat)
    val iteratedAlpha = UInt(8 bits) // Pass-through for fog alpha mode
    val rawW = SInt(32 bits) // Pass-through for fog wDepth calculation

    // Fog color (per-triangle, pass-through for Fog stage)
    val fogColor = Bits(32 bits)
    val chromaKey = Bits(32 bits)
    val zaColor = Bits(32 bits)

    // Per-triangle FIFO registers (pass-through for downstream stages)
    val alphaMode = AlphaMode()
    val fogMode = FogMode()
    val fbzMode = FbzMode()
    val routing = FbRouting(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null

    def drawColorBufferBase: UInt = routing.colorBaseAddr
    def drawAuxBufferBase: UInt = routing.auxBaseAddr
    def fbPixelStride: UInt = routing.pixelStride
  }
}

/** Color Combine Unit - combines iterated colors with texture and constants */
case class ColorCombine(c: voodoo.Config) extends Component {
  val io = new Bundle {
    val input = slave Stream (ColorCombine.Input(c))
    val output = master Stream (ColorCombine.Output(c))
  }

  io.output.translateFrom(io.input) { (out, in) =>
    val xls = new XlsColorCombine
    xls.io.iterated_rgb := (in.iterated.r ## in.iterated.g ## in.iterated.b).asBits
    xls.io.iterated_alpha := in.iteratedAlpha
    xls.io.iterated_z := in.iteratedZ
    xls.io.texture_rgb := (in.texture.r ## in.texture.g ## in.texture.b).asBits
    xls.io.texture_alpha := in.textureAlpha
    xls.io.color0_rgb := (in.color0.r ## in.color0.g ## in.color0.b).asBits
    xls.io.color0_alpha := in.color0Alpha
    xls.io.color1_rgb := (in.color1.r ## in.color1.g ## in.color1.b).asBits
    xls.io.color1_alpha := in.color1Alpha
    xls.io.rgb_sel := in.config.rgbSel.asBits
    xls.io.local_sel := in.config.localSelect.asBits
    xls.io.local_override := in.config.localSelectOverride
    xls.io.zero_other := in.config.zeroOther
    xls.io.sub_local := in.config.subClocal
    xls.io.factor_sel := in.config.mselect.asBits
    xls.io.reverse_blend := in.config.reverseBlend
    xls.io.add_mode := in.config.add.asBits
    xls.io.invert_rgb := in.config.invertOutput
    xls.io.alpha_sel := in.config.alphaSel.asBits
    xls.io.alpha_local_sel := in.config.alphaLocalSelect.asBits
    xls.io.alpha_zero_other := in.config.alphaZeroOther
    xls.io.alpha_sub_local := in.config.alphaSubClocal
    xls.io.alpha_factor_sel := in.config.alphaMselect.asBits
    xls.io.alpha_reverse_blend := in.config.alphaReverseBlend
    xls.io.alpha_add_mode := in.config.alphaAdd.asBits
    xls.io.invert_alpha := in.config.alphaInvertOutput
    out.color.r := xls.io.out(31 downto 24).asUInt
    out.color.g := xls.io.out(23 downto 16).asUInt
    out.color.b := xls.io.out(15 downto 8).asUInt
    out.alpha := xls.io.out(7 downto 0).asUInt
    out.coords := in.coords
    out.depth := in.depth
    out.iteratedAlpha := in.iteratedAlpha
    out.rawW := in.rawW
    out.fogColor := in.fogColor
    out.chromaKey := in.chromaKey
    out.zaColor := in.zaColor
    out.alphaMode := in.alphaMode
    out.fogMode := in.fogMode
    out.fbzMode := in.fbzMode
    out.routing := in.routing
    if (c.trace.enabled) out.trace := in.trace
  }
}
