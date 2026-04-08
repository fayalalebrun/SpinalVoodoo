package voodoo.texture

import voodoo._
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

  val paletteRam = Mem(Bits(24 bits), 256)
  when(io.paletteWrite.valid) {
    paletteRam.write(io.paletteWrite.payload.address, io.paletteWrite.payload.data)
  }

  case class DecodedRgba() extends Bundle {
    val r, g, b, a = UInt(8 bits)
  }

  def decodeTexelWord(
      texelData: Bits,
      format: UInt,
      ncc: Tmu.NccTableData,
      paletteColor: Bits
  ): DecodedRgba = {
    val texelByte = texelData(7 downto 0)
    val decoded = DecodedRgba()

    def nccDecode(texByte: Bits, nccData: Tmu.NccTableData): (UInt, UInt, UInt) = {
      val yVal = nccData.y(texByte(7 downto 4).asUInt)
      val iEntry = nccData.i(texByte(3 downto 2).asUInt)
      val qEntry = nccData.q(texByte(1 downto 0).asUInt)
      val iR = iEntry(26 downto 18).asSInt
      val iG = iEntry(17 downto 9).asSInt
      val iB = iEntry(8 downto 0).asSInt
      val qR = qEntry(26 downto 18).asSInt
      val qG = qEntry(17 downto 9).asSInt
      val qB = qEntry(8 downto 0).asSInt
      val rRaw = (False ## yVal).asSInt.resize(11 bits) + iR.resize(11 bits) + qR.resize(11 bits)
      val gRaw = (False ## yVal).asSInt.resize(11 bits) + iG.resize(11 bits) + qG.resize(11 bits)
      val bRaw = (False ## yVal).asSInt.resize(11 bits) + iB.resize(11 bits) + qB.resize(11 bits)
      (clampToU8(rRaw), clampToU8(gRaw), clampToU8(bRaw))
    }

    switch(format) {
      is(Tmu.TextureFormat.RGB332) {
        decoded.r := expandTo8(texelByte(7 downto 5).asUInt, 3)
        decoded.g := expandTo8(texelByte(4 downto 2).asUInt, 3)
        decoded.b := expandTo8(texelByte(1 downto 0).asUInt, 2)
        decoded.a := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.YIQ422) {
        val (r, g, b) = nccDecode(texelByte, ncc)
        decoded.r := r; decoded.g := g; decoded.b := b; decoded.a := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.A8) {
        val dat = texelByte.asUInt
        decoded.r := dat; decoded.g := dat; decoded.b := dat; decoded.a := dat
      }
      is(Tmu.TextureFormat.I8) {
        val intensity = texelByte.asUInt
        decoded.r := intensity; decoded.g := intensity; decoded.b := intensity;
        decoded.a := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.AI44) {
        val alpha = texelByte(7 downto 4).asUInt
        val intensity = texelByte(3 downto 0).asUInt
        decoded.r := expandTo8(intensity, 4)
        decoded.g := expandTo8(intensity, 4)
        decoded.b := expandTo8(intensity, 4)
        decoded.a := expandTo8(alpha, 4)
      }
      is(Tmu.TextureFormat.P8) {
        decoded.r := paletteColor(23 downto 16).asUInt
        decoded.g := paletteColor(15 downto 8).asUInt
        decoded.b := paletteColor(7 downto 0).asUInt
        decoded.a := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.ARGB8332) {
        decoded.a := texelData(15 downto 8).asUInt
        decoded.r := expandTo8(texelData(7 downto 5).asUInt, 3)
        decoded.g := expandTo8(texelData(4 downto 2).asUInt, 3)
        decoded.b := expandTo8(texelData(1 downto 0).asUInt, 2)
      }
      is(Tmu.TextureFormat.AYIQ8422) {
        val (r, g, b) = nccDecode(texelData(7 downto 0), ncc)
        decoded.r := r; decoded.g := g; decoded.b := b; decoded.a := texelData(15 downto 8).asUInt
      }
      is(Tmu.TextureFormat.RGB565) {
        decoded.r := expandTo8(texelData(15 downto 11).asUInt, 5)
        decoded.g := expandTo8(texelData(10 downto 5).asUInt, 6)
        decoded.b := expandTo8(texelData(4 downto 0).asUInt, 5)
        decoded.a := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.ARGB1555) {
        decoded.a := expandTo8(texelData(15 downto 15).asUInt, 1)
        decoded.r := expandTo8(texelData(14 downto 10).asUInt, 5)
        decoded.g := expandTo8(texelData(9 downto 5).asUInt, 5)
        decoded.b := expandTo8(texelData(4 downto 0).asUInt, 5)
      }
      is(Tmu.TextureFormat.ARGB4444) {
        decoded.a := expandTo8(texelData(15 downto 12).asUInt, 4)
        decoded.r := expandTo8(texelData(11 downto 8).asUInt, 4)
        decoded.g := expandTo8(texelData(7 downto 4).asUInt, 4)
        decoded.b := expandTo8(texelData(3 downto 0).asUInt, 4)
      }
      is(Tmu.TextureFormat.AI88) {
        val alpha = texelData(15 downto 8).asUInt
        val intensity = texelData(7 downto 0).asUInt
        decoded.r := intensity; decoded.g := intensity; decoded.b := intensity; decoded.a := alpha
      }
      is(Tmu.TextureFormat.AP88) {
        decoded.r := paletteColor(23 downto 16).asUInt
        decoded.g := paletteColor(15 downto 8).asUInt
        decoded.b := paletteColor(7 downto 0).asUInt
        decoded.a := texelData(15 downto 8).asUInt
      }
      default {
        decoded.r := expandTo8(texelData(15 downto 11).asUInt, 5)
        decoded.g := expandTo8(texelData(10 downto 5).asUInt, 6)
        decoded.b := expandTo8(texelData(4 downto 0).asUInt, 5)
        decoded.a := U(255, 8 bits)
      }
    }
    decoded
  }

  val paletteColors = Seq.tabulate(4) { idx =>
    paletteRam.readSync(
      address = io.sampleFetch.payload.texels(idx)(7 downto 0).asUInt,
      enable = io.sampleFetch.fire
    )
  }
  val fetchPipe = io.sampleFetch.m2sPipe()

  io.output << fetchPipe
    .translateWith {
      val pass = fetchPipe.payload.passthrough
      val ncc = pass.nccTableSelect ? io.nccTables.table1 | io.nccTables.table0
      val weights = Tmu.bilinearWeights(pass.ds, pass.dt)
      val decoded = Seq.tabulate(4) { idx =>
        decodeTexelWord(fetchPipe.payload.texels(idx), pass.format, ncc, paletteColors(idx))
      }

      def channel(idx: Int, sel: DecodedRgba => UInt): UInt = sel(decoded(idx))

      val result = Tmu.Output(c)
      result.requestId := pass.requestId
      result.texture.r := Mux(
        fetchPipe.payload.bilinear,
        Tmu.blendChannel(
          weights,
          channel(0, _.r),
          channel(1, _.r),
          channel(2, _.r),
          channel(3, _.r)
        ),
        channel(0, _.r)
      )
      result.texture.g := Mux(
        fetchPipe.payload.bilinear,
        Tmu.blendChannel(
          weights,
          channel(0, _.g),
          channel(1, _.g),
          channel(2, _.g),
          channel(3, _.g)
        ),
        channel(0, _.g)
      )
      result.texture.b := Mux(
        fetchPipe.payload.bilinear,
        Tmu.blendChannel(
          weights,
          channel(0, _.b),
          channel(1, _.b),
          channel(2, _.b),
          channel(3, _.b)
        ),
        channel(0, _.b)
      )
      result.textureAlpha := Mux(
        fetchPipe.payload.bilinear,
        Tmu.blendChannel(
          weights,
          channel(0, _.a),
          channel(1, _.a),
          channel(2, _.a),
          channel(3, _.a)
        ),
        channel(0, _.a)
      )
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
