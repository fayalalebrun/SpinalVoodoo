package voodoo

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
  ): (UInt, UInt, UInt, UInt) = {
    val texelData = selectTexelData(rspData32, addrHalf)
    val texelByte = selectTexelByte(rspData32, addrHalf, addrByte)
    val dr = UInt(8 bits)
    val dg = UInt(8 bits)
    val db = UInt(8 bits)
    val da = UInt(8 bits)

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
        dr := expandTo8(texelByte(7 downto 5).asUInt, 3)
        dg := expandTo8(texelByte(4 downto 2).asUInt, 3)
        db := expandTo8(texelByte(1 downto 0).asUInt, 2)
        da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.YIQ422) {
        val (r, g, b) = nccDecode(texelByte, ncc)
        dr := r; dg := g; db := b; da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.A8) {
        val dat = texelByte.asUInt
        dr := dat; dg := dat; db := dat; da := dat
      }
      is(Tmu.TextureFormat.I8) {
        val intensity = texelByte.asUInt
        dr := intensity; dg := intensity; db := intensity; da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.AI44) {
        val alpha = texelByte(7 downto 4).asUInt
        val intensity = texelByte(3 downto 0).asUInt
        dr := expandTo8(intensity, 4)
        dg := expandTo8(intensity, 4)
        db := expandTo8(intensity, 4)
        da := expandTo8(alpha, 4)
      }
      is(Tmu.TextureFormat.P8) {
        dr := paletteColor(23 downto 16).asUInt
        dg := paletteColor(15 downto 8).asUInt
        db := paletteColor(7 downto 0).asUInt
        da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.ARGB8332) {
        da := texelData(15 downto 8).asUInt
        dr := expandTo8(texelData(7 downto 5).asUInt, 3)
        dg := expandTo8(texelData(4 downto 2).asUInt, 3)
        db := expandTo8(texelData(1 downto 0).asUInt, 2)
      }
      is(Tmu.TextureFormat.AYIQ8422) {
        val (r, g, b) = nccDecode(texelData(7 downto 0), ncc)
        dr := r; dg := g; db := b; da := texelData(15 downto 8).asUInt
      }
      is(Tmu.TextureFormat.RGB565) {
        dr := expandTo8(texelData(15 downto 11).asUInt, 5)
        dg := expandTo8(texelData(10 downto 5).asUInt, 6)
        db := expandTo8(texelData(4 downto 0).asUInt, 5)
        da := U(255, 8 bits)
      }
      is(Tmu.TextureFormat.ARGB1555) {
        da := expandTo8(texelData(15 downto 15).asUInt, 1)
        dr := expandTo8(texelData(14 downto 10).asUInt, 5)
        dg := expandTo8(texelData(9 downto 5).asUInt, 5)
        db := expandTo8(texelData(4 downto 0).asUInt, 5)
      }
      is(Tmu.TextureFormat.ARGB4444) {
        da := expandTo8(texelData(15 downto 12).asUInt, 4)
        dr := expandTo8(texelData(11 downto 8).asUInt, 4)
        dg := expandTo8(texelData(7 downto 4).asUInt, 4)
        db := expandTo8(texelData(3 downto 0).asUInt, 4)
      }
      is(Tmu.TextureFormat.AI88) {
        val alpha = texelData(15 downto 8).asUInt
        val intensity = texelData(7 downto 0).asUInt
        dr := intensity; dg := intensity; db := intensity; da := alpha
      }
      is(Tmu.TextureFormat.AP88) {
        dr := paletteColor(23 downto 16).asUInt
        dg := paletteColor(15 downto 8).asUInt
        db := paletteColor(7 downto 0).asUInt
        da := texelData(15 downto 8).asUInt
      }
      default {
        dr := expandTo8(texelData(15 downto 11).asUInt, 5)
        dg := expandTo8(texelData(10 downto 5).asUInt, 6)
        db := expandTo8(texelData(4 downto 0).asUInt, 5)
        da := U(255, 8 bits)
      }
    }
    (dr, dg, db, da)
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
    val (dr, dg, db, da) =
      decodeTexelWord(
        rspData32,
        queued.addrHalf,
        queued.addrByte,
        pass.format,
        pass.ncc,
        fetchedPaletteColor
      )

    val result = Tmu.DecodedTexel(c)
    result.r := dr
    result.g := dg
    result.b := db
    result.a := da
    when(pass.sendConfig) {
      result.r := 0
      result.g := 0
      result.b := 1
      result.a := U(255, 8 bits)
    }
    result.passthrough := pass
    result
  }

  case class BilinearWeights() extends Bundle {
    val w0 = UInt(10 bits)
    val w1 = UInt(10 bits)
    val w2 = UInt(10 bits)
    val w3 = UInt(10 bits)
  }

  def bilinearWeights(ds: UInt, dt: UInt): BilinearWeights = {
    val ds5 = ds.resize(5 bits)
    val dt5 = dt.resize(5 bits)
    val invDs = U(16, 5 bits) - ds5
    val invDt = U(16, 5 bits) - dt5
    val weights = BilinearWeights()
    weights.w0 := (invDs * invDt).resize(10 bits)
    weights.w1 := (ds5 * invDt).resize(10 bits)
    weights.w2 := (invDs * dt5).resize(10 bits)
    weights.w3 := (ds5 * dt5).resize(10 bits)
    weights
  }

  def blendChannel(weights: BilinearWeights, t0: UInt, t1: UInt, t2: UInt, t3: UInt): UInt = {
    val sum = (t0.resize(18 bits) * weights.w0.resize(10 bits)) +
      (t1.resize(18 bits) * weights.w1.resize(10 bits)) +
      (t2.resize(18 bits) * weights.w2.resize(10 bits)) +
      (t3.resize(18 bits) * weights.w3.resize(10 bits))
    (sum >> 8).resize(8 bits)
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

  io.fastOutput << fastFetchPipe.translateWith {
    val pass = fastFetchPipe.payload.passthrough
    val weights = bilinearWeights(pass.ds, pass.dt)
    val (r0, g0, b0, a0) =
      decodeTexelWord(
        fastFetchPipe.payload.texels(0).rspData32,
        fastFetchPipe.payload.texels(0).addrHalf,
        fastFetchPipe.payload.texels(0).addrByte,
        pass.format,
        pass.ncc,
        fastPaletteColors(0)
      )
    val (r1, g1, b1, a1) =
      decodeTexelWord(
        fastFetchPipe.payload.texels(1).rspData32,
        fastFetchPipe.payload.texels(1).addrHalf,
        fastFetchPipe.payload.texels(1).addrByte,
        pass.format,
        pass.ncc,
        fastPaletteColors(1)
      )
    val (r2, g2, b2, a2) =
      decodeTexelWord(
        fastFetchPipe.payload.texels(2).rspData32,
        fastFetchPipe.payload.texels(2).addrHalf,
        fastFetchPipe.payload.texels(2).addrByte,
        pass.format,
        pass.ncc,
        fastPaletteColors(2)
      )
    val (r3, g3, b3, a3) =
      decodeTexelWord(
        fastFetchPipe.payload.texels(3).rspData32,
        fastFetchPipe.payload.texels(3).addrHalf,
        fastFetchPipe.payload.texels(3).addrByte,
        pass.format,
        pass.ncc,
        fastPaletteColors(3)
      )

    val result = Tmu.Output(c)
    result.texture.r := blendChannel(weights, r0, r1, r2, r3)
    result.texture.g := blendChannel(weights, g0, g1, g2, g3)
    result.texture.b := blendChannel(weights, b0, b1, b2, b3)
    result.textureAlpha := blendChannel(weights, a0, a1, a2, a3)
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
}
