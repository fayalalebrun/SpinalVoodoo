package voodoo.pixel

import voodoo._
import voodoo.xls.{XlsFogBlend, XlsFogTableFactor, XlsFogWDepth}
import spinal.core._
import spinal.lib._

/** Fog output bundle — same as CC Output but without rawW/iteratedAlpha (consumed by fog) */
object Fog {
  case class WStage(c: Config) extends Bundle {
    val payload = Output(c)
    val fogRgb = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val fogEnable = Bool()
    val fogAdd = Bool()
    val fogMult = Bool()
    val fogConstant = Bool()
    val fogModeSelect = UInt(2 bits)
    val zFogFactor = UInt(8 bits)
    val alphaFogFactor = UInt(8 bits)
  }

  case class Stage1(c: Config) extends Bundle {
    val payload = Output(c)
    val fogA = UInt(8 bits)
    val fogRgb = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val fogEnable = Bool()
    val fogAdd = Bool()
    val fogMult = Bool()
    val fogConstant = Bool()
  }

  case class Output(c: Config) extends Bundle {
    val coords = PixelCoords(c)
    val color = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val alpha = UInt(8 bits)
    val depth = AFix(c.vDepthFormat)
    val wDepth = UInt(16 bits) // precomputed wDepth for W-buffer depth mode
    val colorBeforeFog = Color(
      UInt(8 bits),
      UInt(8 bits),
      UInt(8 bits)
    ) // pre-fog color for ACOLORBEFOREFOG blend factor
    val chromaKey = Bits(32 bits)
    val zaColor = Bits(32 bits)

    // Per-triangle FIFO registers (pass-through for alpha test and framebuffer access)
    val alphaMode = AlphaMode()
    val fbzMode = FbzMode()
    val routing = FbRouting(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null

    def drawColorBufferBase: UInt = routing.colorBaseAddr
    def drawAuxBufferBase: UInt = routing.auxBaseAddr
    def fbPixelStride: UInt = routing.pixelStride
  }
}

/** Fog stage — applies fog blending between color combine and alpha test.
  *
  * Combinational transform (no pipeline stages). Fog blends pixel colors toward a fog color based
  * on a fog factor derived from depth, alpha, or W.
  */
case class Fog(c: Config) extends Component {
  val io = new Bundle {
    val input = slave Stream (ColorCombine.Output(c))
    val output = master Stream (Fog.Output(c))
    val fogTable = in Vec (Bits(16 bits), 64)
    val busy = out Bool ()
  }

  val wStage = io.input
    .translateWith {
      val in = io.input.payload
      val wDepth = new XlsFogWDepth
      wDepth.io.raw_w := in.rawW
      val out = Fog.WStage(c)
      out.payload.coords := in.coords
      out.payload.color := in.color
      out.payload.alpha := in.alpha
      out.payload.depth := in.depth
      out.payload.wDepth := wDepth.io.out
      out.payload.colorBeforeFog := in.color
      out.payload.chromaKey := in.chromaKey
      out.payload.zaColor := in.zaColor
      out.payload.alphaMode := in.alphaMode
      out.payload.fbzMode := in.fbzMode
      out.payload.routing := in.routing
      if (c.trace.enabled) out.payload.trace := in.trace
      out.fogRgb := rgb888Color(in.fogColor)
      out.fogEnable := in.fogMode.fogEnable
      out.fogAdd := in.fogMode.fogAdd
      out.fogMult := in.fogMode.fogMult
      out.fogConstant := in.fogMode.fogConstant
      out.fogModeSelect := in.fogMode.fogModeSelect
      out.zFogFactor := (in.depth.raw.asSInt >> 20).asUInt.resize(8 bits)
      out.alphaFogFactor := in.iteratedAlpha
      out
    }
    .m2sPipe()

  val stage1 = wStage
    .translateWith {
      val in = wStage.payload
      val tableFactor = new XlsFogTableFactor
      tableFactor.io.w_depth := in.payload.wDepth
      tableFactor.io.table_entry := io.fogTable(in.payload.wDepth(15 downto 10))
      val out = Fog.Stage1(c)
      out.payload := in.payload
      out.fogA := in.fogModeSelect.mux(
        U(0) -> tableFactor.io.out,
        U(1) -> in.alphaFogFactor,
        U(2) -> in.zFogFactor,
        default -> U(0, 8 bits)
      )
      out.fogRgb := in.fogRgb
      out.fogEnable := in.fogEnable
      out.fogAdd := in.fogAdd
      out.fogMult := in.fogMult
      out.fogConstant := in.fogConstant
      out
    }
    .m2sPipe()

  val stage2 = stage1
    .translateWith {
      val in = stage1.payload
      val blend = new XlsFogBlend
      blend.io.src_rgb := (in.payload.colorBeforeFog.r ## in.payload.colorBeforeFog.g ## in.payload.colorBeforeFog.b).asBits
      blend.io.fog_rgb := (in.fogRgb.r ## in.fogRgb.g ## in.fogRgb.b).asBits
      blend.io.factor := in.fogA
      blend.io.enable := in.fogEnable
      blend.io.add := in.fogAdd
      blend.io.mult := in.fogMult
      blend.io.constant := in.fogConstant
      val out = Fog.Output(c)
      out.coords := in.payload.coords
      out.alpha := in.payload.alpha
      out.depth := in.payload.depth
      out.wDepth := in.payload.wDepth
      out.colorBeforeFog := in.payload.colorBeforeFog
      out.chromaKey := in.payload.chromaKey
      out.zaColor := in.payload.zaColor
      out.alphaMode := in.payload.alphaMode
      out.fbzMode := in.payload.fbzMode
      out.routing := in.payload.routing
      if (c.trace.enabled) out.trace := in.payload.trace
      out.color.r := blend.io.out(23 downto 16).asUInt
      out.color.g := blend.io.out(15 downto 8).asUInt
      out.color.b := blend.io.out(7 downto 0).asUInt
      out
    }
    .m2sPipe()

  io.output << stage2
  io.busy := wStage.valid || stage1.valid || stage2.valid
}
