//> using target.scope test

package voodoo

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._

class TmuTexelDecoderTest extends AnyFunSuite {
  val config = Config.voodoo1(TraceConfig())

  def zeroNcc(dut: TmuTexelDecoder): Unit = {
    for (i <- 0 until 16) {
      dut.io.nccTables.table0.y(i) #= 0
      dut.io.nccTables.table1.y(i) #= 0
    }
    for (i <- 0 until 4) {
      dut.io.nccTables.table0.i(i) #= 0
      dut.io.nccTables.table0.q(i) #= 0
      dut.io.nccTables.table1.i(i) #= 0
      dut.io.nccTables.table1.q(i) #= 0
    }
  }

  def init(dut: TmuTexelDecoder): Unit = {
    dut.clockDomain.forkStimulus(10)
    dut.io.sampleFetch.valid #= false
    dut.io.output.ready #= true
    dut.io.paletteWrite.valid #= false
    dut.io.paletteWrite.payload.address #= 0
    dut.io.paletteWrite.payload.data #= 0
    dut.clockDomain.waitSampling()
  }

  def drivePass(
      dut: TmuTexelDecoder,
      format: Int,
      bilinear: Boolean,
      ds: Int = 0,
      dt: Int = 0
  ): Unit = {
    dut.io.sampleFetch.payload.bilinear #= bilinear
    dut.io.sampleFetch.payload.passthrough.format #= format
    dut.io.sampleFetch.payload.passthrough.bilinear #= bilinear
    dut.io.sampleFetch.payload.passthrough.nccTableSelect #= false
    dut.io.sendConfig #= false
    dut.io.sampleFetch.payload.passthrough.ds #= ds
    dut.io.sampleFetch.payload.passthrough.dt #= dt

    zeroNcc(dut)
  }

  test("TmuTexelDecoder decodes a point RGB565 sample") {
    SimConfig.withIVerilog.compile(TmuTexelDecoder(config)).doSim { dut =>
      init(dut)
      drivePass(dut, Tmu.TextureFormat.RGB565, bilinear = false)
      dut.io.sampleFetch.payload.texels(0) #= 0xf81f
      dut.io.sampleFetch.payload.texels(1) #= 0
      dut.io.sampleFetch.payload.texels(2) #= 0
      dut.io.sampleFetch.payload.texels(3) #= 0
      dut.io.sampleFetch.valid #= true
      dut.clockDomain.waitSamplingWhere(dut.io.sampleFetch.ready.toBoolean)
      dut.io.sampleFetch.valid #= false
      dut.clockDomain.waitSamplingWhere(dut.io.output.valid.toBoolean)
      assert(dut.io.output.payload.texture.r.toInt == 255)
      assert(dut.io.output.payload.texture.g.toInt == 0)
      assert(dut.io.output.payload.texture.b.toInt == 255)
      assert(dut.io.output.payload.textureAlpha.toInt == 255)

    }
  }

  def sample(
      dut: TmuTexelDecoder,
      format: Int,
      texel: Int,
      nccY: Option[Int] = None
  ): (Int, Int, Int, Int) = {
    drivePass(dut, format, bilinear = false)
    nccY.foreach(value => dut.io.nccTables.table0.y((texel >> 4) & 0xf) #= value)
    dut.io.sampleFetch.payload.texels(0) #= texel
    dut.io.sampleFetch.payload.texels(1) #= 0
    dut.io.sampleFetch.payload.texels(2) #= 0
    dut.io.sampleFetch.payload.texels(3) #= 0
    dut.io.sampleFetch.valid #= true
    dut.clockDomain.waitSamplingWhere(dut.io.sampleFetch.ready.toBoolean)
    dut.io.sampleFetch.valid #= false
    dut.clockDomain.waitSamplingWhere(dut.io.output.valid.toBoolean)
    (
      dut.io.output.payload.texture.r.toInt,
      dut.io.output.payload.texture.g.toInt,
      dut.io.output.payload.texture.b.toInt,
      dut.io.output.payload.textureAlpha.toInt
    )
  }

  test("TmuTexelDecoder blends a bilinear RGB565 sample") {
    SimConfig.withIVerilog.compile(TmuTexelDecoder(config)).doSim { dut =>
      init(dut)
      drivePass(dut, Tmu.TextureFormat.RGB565, bilinear = true, ds = 8, dt = 8)
      dut.io.sampleFetch.payload.texels(0) #= 0xf800
      dut.io.sampleFetch.payload.texels(1) #= 0x07e0
      dut.io.sampleFetch.payload.texels(2) #= 0x001f
      dut.io.sampleFetch.payload.texels(3) #= 0xffff
      dut.io.sampleFetch.valid #= true
      dut.clockDomain.waitSamplingWhere(dut.io.sampleFetch.ready.toBoolean)
      dut.io.sampleFetch.valid #= false
      dut.clockDomain.waitSamplingWhere(dut.io.output.valid.toBoolean)
      assert(dut.io.output.payload.texture.r.toInt >= 120)
      assert(dut.io.output.payload.texture.g.toInt >= 120)
      assert(dut.io.output.payload.texture.b.toInt >= 120)
      assert(dut.io.output.payload.textureAlpha.toInt == 255)
    }
  }

  test("TmuTexelDecoder covers alpha, intensity, palette, NCC, and packed formats") {
    SimConfig.withIVerilog.compile(TmuTexelDecoder(config)).doSim { dut =>
      init(dut)
      assert(sample(dut, Tmu.TextureFormat.RGB332, 0xff) == (255, 255, 255, 255))
      assert(sample(dut, Tmu.TextureFormat.A8, 0x80) == (128, 128, 128, 128))
      assert(sample(dut, Tmu.TextureFormat.I8, 0x40) == (64, 64, 64, 255))
      assert(sample(dut, Tmu.TextureFormat.AI44, 0xa5) == (85, 85, 85, 170))
      assert(sample(dut, Tmu.TextureFormat.ARGB8332, 0x80ff) == (255, 255, 255, 128))
      assert(sample(dut, Tmu.TextureFormat.ARGB1555, 0xffff) == (255, 255, 255, 255))
      assert(sample(dut, Tmu.TextureFormat.ARGB4444, 0xabcd) == (187, 204, 221, 170))
      assert(sample(dut, Tmu.TextureFormat.AI88, 0x8040) == (64, 64, 64, 128))

      dut.io.paletteWrite.payload.address #= 0x42
      dut.io.paletteWrite.payload.data #= 0x112233
      dut.io.paletteWrite.valid #= true
      dut.clockDomain.waitSampling()
      dut.io.paletteWrite.valid #= false
      assert(sample(dut, Tmu.TextureFormat.P8, 0x42) == (17, 34, 51, 255))
      assert(sample(dut, Tmu.TextureFormat.AP88, 0x8042) == (17, 34, 51, 128))

      assert(sample(dut, Tmu.TextureFormat.YIQ422, 0, Some(100)) == (100, 100, 100, 255))
      assert(sample(dut, Tmu.TextureFormat.AYIQ8422, 0x8000, Some(100)) == (100, 100, 100, 128))
    }
  }
}
