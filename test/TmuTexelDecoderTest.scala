//> using target.scope test

package voodoo

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._

class TmuTexelDecoderTest extends AnyFunSuite {
  val config = Config.voodoo1(TraceConfig())

  def zeroNcc(dut: TmuTexelDecoder): Unit = {
    for (i <- 0 until 16) dut.io.sampleFetch.payload.passthrough.ncc.y(i) #= 0
    for (i <- 0 until 4) {
      dut.io.sampleFetch.payload.passthrough.ncc.i(i) #= 0
      dut.io.sampleFetch.payload.passthrough.ncc.q(i) #= 0
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
    dut.io.sampleFetch.payload.passthrough.sendConfig #= false
    dut.io.sampleFetch.payload.passthrough.ds #= ds
    dut.io.sampleFetch.payload.passthrough.dt #= dt
    dut.io.sampleFetch.payload.passthrough.readIdx #= 0
    dut.io.sampleFetch.payload.passthrough.requestId #= 3
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
      assert(dut.io.output.payload.requestId.toInt == 3)
    }
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
}
