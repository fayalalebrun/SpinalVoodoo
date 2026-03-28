package voodoo.texture

import voodoo._
import spinal.core._
import spinal.core.sim._
import spinal.lib._

case class TmuTexelDecoder(c: voodoo.Config) extends Component {
  val io = new Bundle {
    val fetched = slave Stream (Tmu.FetchResult(c))
    val fastFetch = slave Stream (Tmu.FastFetch(c))
    val decoded = master Stream (Tmu.DecodedTexel(c))
    val fastOutput = master Stream (Tmu.Output(c))
    val paletteWrite = slave Flow (Tmu.PaletteWrite())
  }

  val paletteRam = Mem(Bits(24 bits), 256)
  when(io.paletteWrite.valid) {
    paletteRam.write(io.paletteWrite.payload.address, io.paletteWrite.payload.data)
  }

  case class DecodedRgba() extends Bundle {
    val r, g, b, a = UInt(8 bits)
  }

  def selectTexelData(rspData32: Bits, addrHalf: Bool): Bits = {
    Mux(addrHalf, rspData32(31 downto 16), rspData32(15 downto 0))
  }

  def selectTexelByte(rspData32: Bits, addrHalf: Bool, addrByte: Bool): Bits = {
    val texelData = selectTexelData(rspData32, addrHalf)
    Mux(addrByte, texelData(15 downto 8), texelData(7 downto 0))
  }

  def decodeTexelWord(
      rspData32: Bits,
      addrHalf: Bool,
      addrByte: Bool,
      format: UInt,
      ncc: Tmu.NccTableData,
      paletteColor: Bits
  ): DecodedRgba = {
    val texelData = selectTexelData(rspData32, addrHalf)
    val texelByte = selectTexelByte(rspData32, addrHalf, addrByte)
    val decoded = DecodedRgba()

    def nccDecode(texByte: Bits, nccData: Tmu.NccTableData): (UInt, UInt, UInt) = {
      val yVal = nccData.y(texByte(7 downto 4).asUInt)
      val iEntry = nccData.i(texByte(3 downto 2).asUInt)
      val qEntry = nccData.q(texByte(1 downto 0).asUInt)
      val iR = iEntry(26 downto 18).asSInt; val iG = iEntry(17 downto 9).asSInt;
      val iB = iEntry(8 downto 0).asSInt
      val qR = qEntry(26 downto 18).asSInt; val qG = qEntry(17 downto 9).asSInt;
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

  val fetchedPaletteColor = paletteRam.readSync(
    address = selectTexelByte(
      io.fetched.payload.rspData32,
      io.fetched.payload.queued.addrHalf,
      io.fetched.payload.queued.addrByte
    ).asUInt,
    enable = io.fetched.fire
  )
  val fetchedPipe = io.fetched.m2sPipe()

  io.decoded << fetchedPipe.translateWith {
    val rspData32 = fetchedPipe.payload.rspData32
    val queued = fetchedPipe.payload.queued
    val pass = queued.passthrough
    val decoded = decodeTexelWord(
      rspData32,
      queued.addrHalf,
      queued.addrByte,
      pass.format,
      pass.ncc,
      fetchedPaletteColor
    )

    val result = Tmu.DecodedTexel(c)
    result.r := decoded.r
    result.g := decoded.g
    result.b := decoded.b
    result.a := decoded.a
    when(pass.sendConfig) {
      result.r := 0
      result.g := 0
      result.b := 1
      result.a := U(255, 8 bits)
    }
    result.passthrough := pass
    result
  }

  case class FastBilinearPrep(c: voodoo.Config) extends Bundle {
    val passthrough = Tmu.TmuPassthrough(c)
    val weights = Tmu.BilinearWeights()
    val texels = Vec.fill(4)(Bits(32 bits))
  }

  val fastPaletteColors = Seq.tabulate(4) { idx =>
    paletteRam.readSync(
      address = selectTexelByte(
        io.fastFetch.payload.texels(idx).rspData32,
        io.fastFetch.payload.texels(idx).addrHalf,
        io.fastFetch.payload.texels(idx).addrByte
      ).asUInt,
      enable = io.fastFetch.fire
    )
  }
  val fastFetchPipe = io.fastFetch.m2sPipe()

  val fastBlendPrep = fastFetchPipe
    .translateWith {
      val pass = fastFetchPipe.payload.passthrough
      val out = FastBilinearPrep(c)
      out.passthrough := pass
      out.weights := Tmu.bilinearWeights(pass.ds, pass.dt)
      def decodeFastTexel(idx: Int) =
        decodeTexelWord(
          fastFetchPipe.payload.texels(idx).rspData32,
          fastFetchPipe.payload.texels(idx).addrHalf,
          fastFetchPipe.payload.texels(idx).addrByte,
          pass.format,
          pass.ncc,
          fastPaletteColors(idx)
        )
      val decoded = Seq.tabulate(4)(decodeFastTexel)
      for ((texel, idx) <- decoded.zipWithIndex) {
        out.texels(idx) := texel.r ## texel.g ## texel.b ## texel.a
      }
      out
    }
    .m2sPipe()

  io.fastOutput << fastBlendPrep
    .translateWith {
      val pass = fastBlendPrep.payload.passthrough
      def channel(texel: Bits, hi: Int, lo: Int) = texel(hi downto lo).asUInt

      val result = Tmu.Output(c)
      result.texture.r := Tmu.blendChannel(
        fastBlendPrep.payload.weights,
        channel(fastBlendPrep.payload.texels(0), 31, 24),
        channel(fastBlendPrep.payload.texels(1), 31, 24),
        channel(fastBlendPrep.payload.texels(2), 31, 24),
        channel(fastBlendPrep.payload.texels(3), 31, 24)
      )
      result.texture.g := Tmu.blendChannel(
        fastBlendPrep.payload.weights,
        channel(fastBlendPrep.payload.texels(0), 23, 16),
        channel(fastBlendPrep.payload.texels(1), 23, 16),
        channel(fastBlendPrep.payload.texels(2), 23, 16),
        channel(fastBlendPrep.payload.texels(3), 23, 16)
      )
      result.texture.b := Tmu.blendChannel(
        fastBlendPrep.payload.weights,
        channel(fastBlendPrep.payload.texels(0), 15, 8),
        channel(fastBlendPrep.payload.texels(1), 15, 8),
        channel(fastBlendPrep.payload.texels(2), 15, 8),
        channel(fastBlendPrep.payload.texels(3), 15, 8)
      )
      result.textureAlpha := Tmu.blendChannel(
        fastBlendPrep.payload.weights,
        channel(fastBlendPrep.payload.texels(0), 7, 0),
        channel(fastBlendPrep.payload.texels(1), 7, 0),
        channel(fastBlendPrep.payload.texels(2), 7, 0),
        channel(fastBlendPrep.payload.texels(3), 7, 0)
      )
      result.requestId := pass.requestId
      when(pass.sendConfig) {
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
