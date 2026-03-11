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

  def decodeTexelWord(
      rspData32: Bits,
      addrHalf: Bool,
      addrByte: Bool,
      format: UInt,
      ncc: Tmu.NccTableData
  ): (UInt, UInt, UInt, UInt) = {
    val texelData = Mux(addrHalf, rspData32(31 downto 16), rspData32(15 downto 0))
    val texelByte = Mux(addrByte, texelData(15 downto 8), texelData(7 downto 0))
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

    val paletteColor = paletteRam.readAsync(texelByte.asUInt)

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

  io.decoded << io.fetched.translateWith {
    val rspData32 = io.fetched.payload.rspData32
    val queued = io.fetched.payload.queued
    val pass = queued.passthrough
    val (dr, dg, db, da) =
      decodeTexelWord(rspData32, queued.addrHalf, queued.addrByte, pass.format, pass.ncc)

    val result = Tmu.DecodedTexel(c)
    result.r := dr
    result.g := dg
    result.b := db
    result.a := da
    result.passthrough := pass
    result
  }

  def blendChannel(ds: UInt, dt: UInt, t0: UInt, t1: UInt, t2: UInt, t3: UInt): UInt = {
    val w0 = (U(16, 5 bits) - ds.resize(5 bits)) * (U(16, 5 bits) - dt.resize(5 bits))
    val w1 = ds.resize(5 bits) * (U(16, 5 bits) - dt.resize(5 bits))
    val w2 = (U(16, 5 bits) - ds.resize(5 bits)) * dt.resize(5 bits)
    val w3 = ds.resize(5 bits) * dt.resize(5 bits)
    val sum = (t0.resize(18 bits) * w0.resize(10 bits)) +
      (t1.resize(18 bits) * w1.resize(10 bits)) +
      (t2.resize(18 bits) * w2.resize(10 bits)) +
      (t3.resize(18 bits) * w3.resize(10 bits))
    (sum >> 8).resize(8 bits)
  }

  io.fastOutput << io.fastFetch.translateWith {
    val pass = io.fastFetch.payload.passthrough
    val (r0, g0, b0, a0) =
      decodeTexelWord(
        io.fastFetch.payload.texels(0).rspData32,
        io.fastFetch.payload.texels(0).addrHalf,
        io.fastFetch.payload.texels(0).addrByte,
        pass.format,
        pass.ncc
      )
    val (r1, g1, b1, a1) =
      decodeTexelWord(
        io.fastFetch.payload.texels(1).rspData32,
        io.fastFetch.payload.texels(1).addrHalf,
        io.fastFetch.payload.texels(1).addrByte,
        pass.format,
        pass.ncc
      )
    val (r2, g2, b2, a2) =
      decodeTexelWord(
        io.fastFetch.payload.texels(2).rspData32,
        io.fastFetch.payload.texels(2).addrHalf,
        io.fastFetch.payload.texels(2).addrByte,
        pass.format,
        pass.ncc
      )
    val (r3, g3, b3, a3) =
      decodeTexelWord(
        io.fastFetch.payload.texels(3).rspData32,
        io.fastFetch.payload.texels(3).addrHalf,
        io.fastFetch.payload.texels(3).addrByte,
        pass.format,
        pass.ncc
      )

    val result = Tmu.Output(c)
    result.texture.r := blendChannel(pass.ds, pass.dt, r0, r1, r2, r3)
    result.texture.g := blendChannel(pass.ds, pass.dt, g0, g1, g2, g3)
    result.texture.b := blendChannel(pass.ds, pass.dt, b0, b1, b2, b3)
    result.textureAlpha := blendChannel(pass.ds, pass.dt, a0, a1, a2, a3)
    if (c.trace.enabled) {
      result.trace := pass.trace
    }
    result
  }
}
