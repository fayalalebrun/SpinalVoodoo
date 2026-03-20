//> using target.scope test

package voodoo

import org.scalatest.funsuite.AnyFunSuite
import spinal.core._
import spinal.core.sim._

class TmuTextureCacheReuseTest extends AnyFunSuite {
  val c = Config.voodoo1().copy(texFillCacheSlots = 2, texFillRequestWindow = 4)

  def zeroNcc(dut: TmuTextureCache): Unit = {
    for (i <- 0 until 16) dut.io.sampleRequest.payload.passthrough.ncc.y(i) #= 0
    for (i <- 0 until 4) {
      dut.io.sampleRequest.payload.passthrough.ncc.i(i) #= 0
      dut.io.sampleRequest.payload.passthrough.ncc.q(i) #= 0
    }
  }

  def setReq(dut: TmuTextureCache, addr: Int, bank: Int, bilinear: Boolean = false): Unit = {
    dut.io.sampleRequest.payload.pointAddr #= addr
    dut.io.sampleRequest.payload.biAddr0 #= addr
    dut.io.sampleRequest.payload.biAddr1 #= addr + 2
    dut.io.sampleRequest.payload.biAddr2 #= addr + 0x200
    dut.io.sampleRequest.payload.biAddr3 #= addr + 0x202
    dut.io.sampleRequest.payload.pointBankSel #= bank
    dut.io.sampleRequest.payload.biBankSel0 #= 0
    dut.io.sampleRequest.payload.biBankSel1 #= 1
    dut.io.sampleRequest.payload.biBankSel2 #= 2
    dut.io.sampleRequest.payload.biBankSel3 #= 3
    dut.io.sampleRequest.payload.lodBase #= 0
    dut.io.sampleRequest.payload.lodEnd #= 0x20000
    dut.io.sampleRequest.payload.lodShift #= 8
    dut.io.sampleRequest.payload.is16Bit #= true
    for (lod <- 0 until 9) {
      val size = 1 << ((8 - lod).max(0) + (8 - lod).max(0) + 1)
      val base = (0 until lod).map(i => 1 << ((8 - i).max(0) + (8 - i).max(0) + 1)).sum
      dut.io.sampleRequest.payload.texTables.texBase(lod) #= base
      dut.io.sampleRequest.payload.texTables.texEnd(lod) #= base + size
      dut.io.sampleRequest.payload.texTables.texShift(lod) #= (8 - lod).max(0)
    }
    dut.io.sampleRequest.payload.bilinear #= bilinear
    dut.io.sampleRequest.payload.passthrough.format #= Tmu.TextureFormat.RGB565
    dut.io.sampleRequest.payload.passthrough.bilinear #= bilinear
    dut.io.sampleRequest.payload.passthrough.sendConfig #= false
    dut.io.sampleRequest.payload.passthrough.ds #= 0
    dut.io.sampleRequest.payload.passthrough.dt #= 0
    dut.io.sampleRequest.payload.passthrough.readIdx #= 0
    dut.io.sampleRequest.payload.passthrough.requestId #= 0
    zeroNcc(dut)
  }

  test("packed cache reuses a filled point word") {
    SimConfig.withWave.compile(TmuTextureCache(c)).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      dut.io.sampleRequest.valid #= false
      dut.io.fetched.ready #= true
      dut.io.fastFetch.ready #= true
      dut.io.outputRoute.ready #= true
      dut.io.texRead.cmd.ready #= true
      dut.io.texRead.rsp.valid #= false
      dut.io.texRead.rsp.last #= false
      dut.io.texRead.rsp.fragment.data #= 0
      dut.io.texRead.rsp.fragment.source #= 0
      dut.io.texRead.rsp.fragment.context #= 0
      dut.io.invalidate #= false
      dut.clockDomain.waitSampling()

      var cmdCount = 0
      fork {
        while (true) {
          dut.clockDomain.waitSampling()
          if (dut.io.texRead.cmd.valid.toBoolean && dut.io.texRead.cmd.ready.toBoolean) {
            cmdCount += 1
            for (beat <- 0 until c.texFillLineWords) {
              dut.io.texRead.rsp.valid #= true
              dut.io.texRead.rsp.fragment.data #= (0x1000 + beat)
              dut.io.texRead.rsp.last #= (beat == c.texFillLineWords - 1)
              dut.clockDomain.waitSampling()
            }
            dut.io.texRead.rsp.valid #= false
            dut.io.texRead.rsp.last #= false
          }
        }
      }

      setReq(dut, addr = 0, bank = 0)
      dut.io.sampleRequest.valid #= true
      dut.clockDomain.waitSamplingWhere(dut.io.sampleRequest.ready.toBoolean)
      dut.io.sampleRequest.valid #= false
      dut.clockDomain.waitSamplingWhere(dut.io.fetched.valid.toBoolean)
      assert(cmdCount == 1)

      setReq(dut, addr = 0, bank = 0)
      dut.io.sampleRequest.valid #= true
      dut.clockDomain.waitSamplingWhere(dut.io.sampleRequest.ready.toBoolean)
      dut.io.sampleRequest.valid #= false
      dut.clockDomain.waitSamplingWhere(dut.io.fetched.valid.toBoolean)
      assert(cmdCount == 1, s"expected second request to hit cache, saw $cmdCount fills")
    }
  }

  test("packed cache uses fast bilinear hit path") {
    SimConfig.compile(TmuTextureCache(c)).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      dut.io.sampleRequest.valid #= false
      dut.io.fetched.ready #= true
      dut.io.fastFetch.ready #= true
      dut.io.outputRoute.ready #= true
      dut.io.texRead.cmd.ready #= true
      dut.io.texRead.rsp.valid #= false
      dut.io.texRead.rsp.last #= false
      dut.io.texRead.rsp.fragment.data #= 0
      dut.io.texRead.rsp.fragment.source #= 0
      dut.io.texRead.rsp.fragment.context #= 0
      dut.io.invalidate #= false
      dut.clockDomain.waitSampling()

      var cmdCount = 0
      fork {
        while (true) {
          dut.clockDomain.waitSampling()
          if (dut.io.texRead.cmd.valid.toBoolean && dut.io.texRead.cmd.ready.toBoolean) {
            cmdCount += 1
            for (beat <- 0 until c.texFillLineWords) {
              dut.io.texRead.rsp.valid #= true
              dut.io.texRead.rsp.fragment.data #= (0x2000 + beat)
              dut.io.texRead.rsp.last #= (beat == c.texFillLineWords - 1)
              dut.clockDomain.waitSampling()
            }
            dut.io.texRead.rsp.valid #= false
            dut.io.texRead.rsp.last #= false
          }
        }
      }

      for ((addr, bank) <- Seq((0, 0), (2, 1), (512, 2), (514, 3))) {
        setReq(dut, addr = addr, bank = bank)
        dut.io.sampleRequest.valid #= true
        dut.clockDomain.waitSamplingWhere(dut.io.sampleRequest.ready.toBoolean)
        dut.io.sampleRequest.valid #= false
        dut.clockDomain.waitSamplingWhere(dut.io.fetched.valid.toBoolean)
      }
      assert(cmdCount == 2)

      setReq(dut, addr = 0, bank = 0, bilinear = true)
      dut.io.sampleRequest.valid #= true
      dut.clockDomain.waitSamplingWhere(dut.io.sampleRequest.ready.toBoolean)
      dut.io.sampleRequest.valid #= false
      dut.clockDomain.waitSamplingWhere(dut.io.fastFetch.valid.toBoolean)
      assert(cmdCount == 2, s"expected bilinear hit to avoid extra fill, saw $cmdCount fills")
    }
  }
}
