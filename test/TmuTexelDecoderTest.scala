//> using target.scope test

package voodoo

import org.scalatest.funsuite.AnyFunSuite
import spinal.core._
import spinal.core.sim._
import spinal.lib.sim.{StreamDriver, StreamMonitor, StreamReadyRandomizer}

import scala.collection.mutable

class TmuTexelDecoderTest extends AnyFunSuite {
  case class NccRef(y: Vector[Int], i: Vector[(Int, Int, Int)], q: Vector[(Int, Int, Int)])

  case class PassRef(
      format: Int,
      bilinear: Boolean = false,
      sendConfig: Boolean = false,
      ds: Int = 0,
      dt: Int = 0,
      readIdx: Int = 0,
      ncc: NccRef = zeroNcc
  )

  case class FetchTxn(rspData32: Long, addrHalf: Boolean, addrByte: Boolean, pass: PassRef)
  case class FastWord(rspData32: Long, addrHalf: Boolean, addrByte: Boolean)
  case class FastTxn(texels: Seq[FastWord], pass: PassRef)
  case class DecodedExpected(r: Int, g: Int, b: Int, a: Int, pass: PassRef)
  case class OutputExpected(r: Int, g: Int, b: Int, a: Int)

  val config = Config.voodoo1(TraceConfig()).copy(packedTexLayout = false)
  val zeroNcc = NccRef(Vector.fill(16)(0), Vector.fill(4)((0, 0, 0)), Vector.fill(4)((0, 0, 0)))
  val sampleNcc = NccRef(
    y = Vector.tabulate(16)(i => 8 + i * 10),
    i = Vector((20, -10, 5), (-12, 18, -6), (7, 9, -14), (-8, -5, 21)),
    q = Vector((3, 11, -7), (-9, 4, 16), (14, -13, 2), (-6, 8, 12))
  )

  def expandTo8(v: Int, srcWidth: Int): Int = {
    require(srcWidth >= 1 && srcWidth <= 8)
    if (srcWidth >= 8) v & 0xff
    else if (srcWidth == 1) if ((v & 1) != 0) 0xff else 0x00
    else {
      val reps = (8 + srcWidth - 1) / srcWidth
      var wide = 0
      var width = 0
      for (_ <- 0 until reps) {
        wide = (wide << srcWidth) | (v & ((1 << srcWidth) - 1))
        width += srcWidth
      }
      (wide >> (width - 8)) & 0xff
    }
  }

  def clampToU8(v: Int): Int = v.max(0).min(255)

  def selectedHalf(data: Long, addrHalf: Boolean): Int =
    if (addrHalf) ((data >> 16) & 0xffff).toInt else (data & 0xffff).toInt
  def selectedByte(data: Long, addrHalf: Boolean, addrByte: Boolean): Int = {
    val half = selectedHalf(data, addrHalf)
    if (addrByte) (half >> 8) & 0xff else half & 0xff
  }

  def decodeNcc(texelByte: Int, ncc: NccRef): (Int, Int, Int) = {
    val yVal = ncc.y((texelByte >> 4) & 0xf)
    val (iR, iG, iB) = ncc.i((texelByte >> 2) & 0x3)
    val (qR, qG, qB) = ncc.q(texelByte & 0x3)
    (
      clampToU8(yVal + iR + qR),
      clampToU8(yVal + iG + qG),
      clampToU8(yVal + iB + qB)
    )
  }

  def decodeTexelWord(txn: FetchTxn, palette: Map[Int, Int]): OutputExpected = {
    val texelData = selectedHalf(txn.rspData32, txn.addrHalf)
    val texelByte = selectedByte(txn.rspData32, txn.addrHalf, txn.addrByte)
    if (txn.pass.sendConfig) return OutputExpected(0, 0, 1, 255)

    txn.pass.format match {
      case Tmu.TextureFormat.RGB332 =>
        OutputExpected(
          expandTo8((texelByte >> 5) & 0x7, 3),
          expandTo8((texelByte >> 2) & 0x7, 3),
          expandTo8(texelByte & 0x3, 2),
          255
        )
      case Tmu.TextureFormat.YIQ422 =>
        val (r, g, b) = decodeNcc(texelByte, txn.pass.ncc)
        OutputExpected(r, g, b, 255)
      case Tmu.TextureFormat.A8   => OutputExpected(texelByte, texelByte, texelByte, texelByte)
      case Tmu.TextureFormat.I8   => OutputExpected(texelByte, texelByte, texelByte, 255)
      case Tmu.TextureFormat.AI44 =>
        OutputExpected(
          expandTo8(texelByte & 0xf, 4),
          expandTo8(texelByte & 0xf, 4),
          expandTo8(texelByte & 0xf, 4),
          expandTo8((texelByte >> 4) & 0xf, 4)
        )
      case Tmu.TextureFormat.P8 =>
        val color = palette(texelByte)
        OutputExpected((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff, 255)
      case Tmu.TextureFormat.ARGB8332 =>
        OutputExpected(
          expandTo8((texelData >> 5) & 0x7, 3),
          expandTo8((texelData >> 2) & 0x7, 3),
          expandTo8(texelData & 0x3, 2),
          (texelData >> 8) & 0xff
        )
      case Tmu.TextureFormat.AYIQ8422 =>
        val (r, g, b) = decodeNcc(texelData & 0xff, txn.pass.ncc)
        OutputExpected(r, g, b, (texelData >> 8) & 0xff)
      case Tmu.TextureFormat.RGB565 =>
        OutputExpected(
          expandTo8((texelData >> 11) & 0x1f, 5),
          expandTo8((texelData >> 5) & 0x3f, 6),
          expandTo8(texelData & 0x1f, 5),
          255
        )
      case Tmu.TextureFormat.ARGB1555 =>
        OutputExpected(
          expandTo8((texelData >> 10) & 0x1f, 5),
          expandTo8((texelData >> 5) & 0x1f, 5),
          expandTo8(texelData & 0x1f, 5),
          expandTo8((texelData >> 15) & 0x1, 1)
        )
      case Tmu.TextureFormat.ARGB4444 =>
        OutputExpected(
          expandTo8((texelData >> 8) & 0xf, 4),
          expandTo8((texelData >> 4) & 0xf, 4),
          expandTo8(texelData & 0xf, 4),
          expandTo8((texelData >> 12) & 0xf, 4)
        )
      case Tmu.TextureFormat.AI88 =>
        OutputExpected(
          texelData & 0xff,
          texelData & 0xff,
          texelData & 0xff,
          (texelData >> 8) & 0xff
        )
      case Tmu.TextureFormat.AP88 =>
        val color = palette(texelData & 0xff)
        OutputExpected(
          (color >> 16) & 0xff,
          (color >> 8) & 0xff,
          color & 0xff,
          (texelData >> 8) & 0xff
        )
      case other => throw new IllegalArgumentException(s"unexpected format $other")
    }
  }

  def blendChannel(ds: Int, dt: Int, values: Seq[Int]): Int = {
    val w0 = (16 - ds) * (16 - dt)
    val w1 = ds * (16 - dt)
    val w2 = (16 - ds) * dt
    val w3 = ds * dt
    ((values(0) * w0 + values(1) * w1 + values(2) * w2 + values(3) * w3) >> 8) & 0xff
  }

  def expectedFast(txn: FastTxn, palette: Map[Int, Int]): OutputExpected = {
    if (txn.pass.sendConfig) return OutputExpected(0, 0, 1, 255)
    val decoded = txn.texels.map { word =>
      decodeTexelWord(FetchTxn(word.rspData32, word.addrHalf, word.addrByte, txn.pass), palette)
    }
    OutputExpected(
      blendChannel(txn.pass.ds, txn.pass.dt, decoded.map(_.r)),
      blendChannel(txn.pass.ds, txn.pass.dt, decoded.map(_.g)),
      blendChannel(txn.pass.ds, txn.pass.dt, decoded.map(_.b)),
      blendChannel(txn.pass.ds, txn.pass.dt, decoded.map(_.a))
    )
  }

  def drivePass(payload: Tmu.TmuPassthrough, pass: PassRef): Unit = {
    payload.format #= pass.format
    payload.bilinear #= pass.bilinear
    payload.sendConfig #= pass.sendConfig
    payload.ds #= pass.ds
    payload.dt #= pass.dt
    payload.readIdx #= pass.readIdx
    for (idx <- 0 until 16) payload.ncc.y(idx) #= pass.ncc.y(idx)
    for (idx <- 0 until 4) {
      val (ir, ig, ib) = pass.ncc.i(idx)
      val (qr, qg, qb) = pass.ncc.q(idx)
      payload.ncc.i(idx) #= BigInt(packNccEntry(ir, ig, ib))
      payload.ncc.q(idx) #= BigInt(packNccEntry(qr, qg, qb))
    }
  }

  def packNccEntry(r: Int, g: Int, b: Int): Int = {
    def enc(v: Int) = v & 0x1ff
    (enc(r) << 18) | (enc(g) << 9) | enc(b)
  }

  def checkPass(actual: Tmu.TmuPassthrough, expected: PassRef): Unit = {
    assert(actual.format.toInt == expected.format)
    assert(actual.bilinear.toBoolean == expected.bilinear)
    assert(actual.sendConfig.toBoolean == expected.sendConfig)
    assert(actual.ds.toInt == expected.ds)
    assert(actual.dt.toInt == expected.dt)
    assert(actual.readIdx.toInt == expected.readIdx)
    for (idx <- 0 until 16) assert(actual.ncc.y(idx).toInt == expected.ncc.y(idx))
    for (idx <- 0 until 4) {
      val iEntry = actual.ncc.i(idx).toBigInt.intValue
      val qEntry = actual.ncc.q(idx).toBigInt.intValue
      val expI =
        packNccEntry(expected.ncc.i(idx)._1, expected.ncc.i(idx)._2, expected.ncc.i(idx)._3)
      val expQ =
        packNccEntry(expected.ncc.q(idx)._1, expected.ncc.q(idx)._2, expected.ncc.q(idx)._3)
      assert(iEntry == expI)
      assert(qEntry == expQ)
    }
  }

  def initDut(dut: TmuTexelDecoder): Unit = {
    dut.clockDomain.forkStimulus(10)
    dut.io.paletteWrite.valid #= false
    dut.io.paletteWrite.payload.address #= 0
    dut.io.paletteWrite.payload.data #= 0
    dut.io.fetched.valid #= false
    dut.io.fastFetch.valid #= false
    dut.io.decoded.ready #= true
    dut.io.fastOutput.ready #= true
    dut.clockDomain.waitSampling()
  }

  def writePalette(dut: TmuTexelDecoder, address: Int, data: Int): Unit = {
    dut.io.paletteWrite.valid #= true
    dut.io.paletteWrite.payload.address #= address
    dut.io.paletteWrite.payload.data #= data
    dut.clockDomain.waitSampling()
    dut.io.paletteWrite.valid #= false
  }

  test("TmuTexelDecoder decodes all fetched formats and preserves passthrough") {
    SimConfig.withIVerilog.compile(TmuTexelDecoder(config)).doSim { dut =>
      initDut(dut)

      val palette = Map(0x12 -> 0x345678, 0x56 -> 0x89abcd)
      palette.foreach { case (address, data) => writePalette(dut, address, data) }

      val cases = Seq(
        FetchTxn(
          0x000000e5L,
          addrHalf = false,
          addrByte = false,
          PassRef(Tmu.TextureFormat.RGB332, readIdx = 1)
        ),
        FetchTxn(
          0x000000b2L,
          addrHalf = false,
          addrByte = false,
          PassRef(Tmu.TextureFormat.YIQ422, ncc = sampleNcc, readIdx = 2)
        ),
        FetchTxn(
          0x0000007cL,
          addrHalf = false,
          addrByte = false,
          PassRef(Tmu.TextureFormat.A8, readIdx = 3)
        ),
        FetchTxn(0x0000003aL, addrHalf = false, addrByte = false, PassRef(Tmu.TextureFormat.I8)),
        FetchTxn(
          0x000000c7L,
          addrHalf = false,
          addrByte = false,
          PassRef(Tmu.TextureFormat.AI44, readIdx = 1)
        ),
        FetchTxn(0x00000012L, addrHalf = false, addrByte = false, PassRef(Tmu.TextureFormat.P8)),
        FetchTxn(
          0x5ac60000L,
          addrHalf = true,
          addrByte = false,
          PassRef(Tmu.TextureFormat.ARGB8332, readIdx = 2)
        ),
        FetchTxn(
          0x6db20000L,
          addrHalf = true,
          addrByte = false,
          PassRef(Tmu.TextureFormat.AYIQ8422, ncc = sampleNcc, readIdx = 3)
        ),
        FetchTxn(0xbeef0000L, addrHalf = true, addrByte = false, PassRef(Tmu.TextureFormat.RGB565)),
        FetchTxn(
          0x9c110000L,
          addrHalf = true,
          addrByte = false,
          PassRef(Tmu.TextureFormat.ARGB1555, readIdx = 1)
        ),
        FetchTxn(
          0xfa730000L,
          addrHalf = true,
          addrByte = false,
          PassRef(Tmu.TextureFormat.ARGB4444, readIdx = 2)
        ),
        FetchTxn(
          0x7f2c0000L,
          addrHalf = true,
          addrByte = false,
          PassRef(Tmu.TextureFormat.AI88, readIdx = 3)
        ),
        FetchTxn(
          0x44560000L,
          addrHalf = true,
          addrByte = false,
          PassRef(Tmu.TextureFormat.AP88, readIdx = 1)
        ),
        FetchTxn(
          0x00000000L,
          addrHalf = false,
          addrByte = false,
          PassRef(Tmu.TextureFormat.RGB565, sendConfig = true, readIdx = 2)
        )
      )

      val expected = mutable.Queue(cases.map { txn =>
        val decoded = decodeTexelWord(txn, palette)
        DecodedExpected(decoded.r, decoded.g, decoded.b, decoded.a, txn.pass)
      }: _*)
      val pending = mutable.Queue(cases: _*)

      StreamDriver(dut.io.fetched, dut.clockDomain) { payload =>
        if (pending.nonEmpty) {
          val txn = pending.dequeue()
          payload.rspData32 #= txn.rspData32
          payload.queued.addrHalf #= txn.addrHalf
          payload.queued.addrByte #= txn.addrByte
          drivePass(payload.queued.passthrough, txn.pass)
          true
        } else {
          false
        }
      }
      StreamDriver(dut.io.fastFetch, dut.clockDomain)(_ => false)
      StreamReadyRandomizer(dut.io.decoded, dut.clockDomain)

      StreamMonitor(dut.io.decoded, dut.clockDomain) { payload =>
        assert(expected.nonEmpty)
        val exp = expected.dequeue()
        assert(payload.r.toInt == exp.r)
        assert(payload.g.toInt == exp.g)
        assert(payload.b.toInt == exp.b)
        assert(payload.a.toInt == exp.a)
        checkPass(payload.passthrough, exp.pass)
      }

      var cycles = 0
      while (expected.nonEmpty && cycles < 200) {
        dut.clockDomain.waitSampling()
        cycles += 1
      }
      assert(expected.isEmpty, s"fetched cases still pending after $cycles cycles")
    }
  }

  test("TmuTexelDecoder fast path blends texels across simple, palette, and YIQ formats") {
    SimConfig.withIVerilog.compile(TmuTexelDecoder(config)).doSim { dut =>
      initDut(dut)

      val palette = Map(0x10 -> 0x102030, 0x20 -> 0x405060, 0x30 -> 0x708090, 0x40 -> 0xa0b0c0)
      palette.foreach { case (address, data) => writePalette(dut, address, data) }

      val cases = Seq(
        FastTxn(
          Seq(
            FastWord(0xf800L, addrHalf = false, addrByte = false),
            FastWord(0x07e0L, addrHalf = false, addrByte = false),
            FastWord(0x001fL, addrHalf = false, addrByte = false),
            FastWord(0xffffL, addrHalf = false, addrByte = false)
          ),
          PassRef(Tmu.TextureFormat.RGB565, bilinear = true, ds = 5, dt = 9)
        ),
        FastTxn(
          Seq(
            FastWord(0x10L, addrHalf = false, addrByte = false),
            FastWord(0x20L, addrHalf = false, addrByte = false),
            FastWord(0x30L, addrHalf = false, addrByte = false),
            FastWord(0x40L, addrHalf = false, addrByte = false)
          ),
          PassRef(Tmu.TextureFormat.P8, bilinear = true, ds = 8, dt = 4)
        ),
        FastTxn(
          Seq(
            FastWord(0x81L, addrHalf = false, addrByte = false),
            FastWord(0x92L, addrHalf = false, addrByte = false),
            FastWord(0xa3L, addrHalf = false, addrByte = false),
            FastWord(0xb0L, addrHalf = false, addrByte = false)
          ),
          PassRef(Tmu.TextureFormat.YIQ422, bilinear = true, ds = 3, dt = 12, ncc = sampleNcc)
        ),
        FastTxn(
          Seq.fill(4)(FastWord(0L, addrHalf = false, addrByte = false)),
          PassRef(Tmu.TextureFormat.RGB565, bilinear = true, ds = 7, dt = 6, sendConfig = true)
        )
      )

      val expected = mutable.Queue(cases.map(expectedFast(_, palette)): _*)
      val pending = mutable.Queue(cases: _*)

      StreamDriver(dut.io.fetched, dut.clockDomain)(_ => false)
      StreamDriver(dut.io.fastFetch, dut.clockDomain) { payload =>
        if (pending.nonEmpty) {
          val txn = pending.dequeue()
          txn.texels.zipWithIndex.foreach { case (texel, idx) =>
            payload.texels(idx).rspData32 #= texel.rspData32
            payload.texels(idx).addrHalf #= texel.addrHalf
            payload.texels(idx).addrByte #= texel.addrByte
          }
          drivePass(payload.passthrough, txn.pass)
          true
        } else {
          false
        }
      }
      StreamReadyRandomizer(dut.io.fastOutput, dut.clockDomain)

      StreamMonitor(dut.io.fastOutput, dut.clockDomain) { payload =>
        assert(expected.nonEmpty)
        val exp = expected.dequeue()
        assert(payload.texture.r.toInt == exp.r)
        assert(payload.texture.g.toInt == exp.g)
        assert(payload.texture.b.toInt == exp.b)
        assert(payload.textureAlpha.toInt == exp.a)
      }

      var cycles = 0
      while (expected.nonEmpty && cycles < 200) {
        dut.clockDomain.waitSampling()
        cycles += 1
      }
      assert(expected.isEmpty, s"fast cases still pending after $cycles cycles")
    }
  }
}
