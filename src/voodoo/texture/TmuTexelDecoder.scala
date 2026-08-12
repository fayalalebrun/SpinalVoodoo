package voodoo.texture

import voodoo._
import voodoo.xls.{XlsTexelDecode, XlsTextureFilter}
import spinal.core._
import spinal.lib._

case class TmuTexelDecoder(c: voodoo.Config) extends Component {
  val io = new Bundle {
    val sampleFetch = slave Stream (Tmu.SampleFetch(c))
    val output = master Stream (Tmu.Output(c))
    val paletteWrite = slave Flow (Tmu.PaletteWrite())
    val sendConfig = in Bool ()
    val nccTables = in(Tmu.NccTables())
  }

  // Quartus trips over the inferred 1W/4R synchronous RAM shape here during DE10 builds.
  // Replicate the small palette RAM per tap so each copy is a simple 1W/1R memory.
  val paletteRam = Seq.fill(4)(Mem(Bits(24 bits), 256))
  when(io.paletteWrite.valid) {
    for (ram <- paletteRam) {
      ram.write(io.paletteWrite.payload.address, io.paletteWrite.payload.data)
    }
  }

  case class DecodedRgba() extends Bundle {
    val r, g, b, a = UInt(8 bits)
  }

  case class DecodedFetch() extends Bundle {
    val passthrough = Tmu.TmuPassthrough(c)
    val bilinear = Bool()
    val texels = Vec.fill(4)(DecodedRgba())
  }

  val paletteColors = Seq.tabulate(4) { idx =>
    paletteRam(idx).readSync(
      address = io.sampleFetch.payload.texels(idx)(7 downto 0).asUInt,
      enable = io.sampleFetch.fire
    )
  }
  val fetchPipe = io.sampleFetch.m2sPipe()
  val decodedPipe = fetchPipe
    .translateWith {
      val pass = fetchPipe.payload.passthrough
      val ncc = pass.nccTableSelect ? io.nccTables.table1 | io.nccTables.table0
      val decodedFetch = DecodedFetch()
      decodedFetch.passthrough := pass
      decodedFetch.bilinear := fetchPipe.payload.bilinear
      for (idx <- 0 until 4) {
        val texel = fetchPipe.payload.texels(idx)
        val xls = new XlsTexelDecode
        xls.io.texel := texel
        xls.io.format := pass.format
        xls.io.palette := paletteColors(idx)
        xls.io.ncc_y := ncc.y(texel(7 downto 4).asUInt)
        xls.io.ncc_i := ncc.i(texel(3 downto 2).asUInt)
        xls.io.ncc_q := ncc.q(texel(1 downto 0).asUInt)
        decodedFetch.texels(idx).r := xls.io.out(31 downto 24).asUInt
        decodedFetch.texels(idx).g := xls.io.out(23 downto 16).asUInt
        decodedFetch.texels(idx).b := xls.io.out(15 downto 8).asUInt
        decodedFetch.texels(idx).a := xls.io.out(7 downto 0).asUInt
      }
      decodedFetch
    }
    .m2sPipe()

  io.output << decodedPipe
    .translateWith {
      val pass = decodedPipe.payload.passthrough
      val filter = new XlsTextureFilter
      val packed = decodedPipe.payload.texels.map(t => (t.r ## t.g ## t.b ## t.a).asBits)
      filter.io.t0 := packed(0)
      filter.io.t1 := packed(1)
      filter.io.t2 := packed(2)
      filter.io.t3 := packed(3)
      filter.io.ds := pass.ds
      filter.io.dt := pass.dt
      filter.io.bilinear := decodedPipe.payload.bilinear

      val result = Tmu.Output(c)
      result.texture.r := filter.io.out(31 downto 24).asUInt
      result.texture.g := filter.io.out(23 downto 16).asUInt
      result.texture.b := filter.io.out(15 downto 8).asUInt
      result.textureAlpha := filter.io.out(7 downto 0).asUInt
      when(io.sendConfig) {
        result.texture.r := 0
        result.texture.g := 0
        result.texture.b := 1
        result.textureAlpha := U(255, 8 bits)
      }
      if (c.trace.enabled) {
        result.trace := pass.trace
      }
      result
    }
    .m2sPipe()
}
