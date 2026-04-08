package voodoo

import spinal.core._
import spinal.core.sim._

object TmuTextureCacheBenchmark extends App {
  val c = Config
    .voodoo1()
    .copy(
      useTexFillCache = true,
      texFillLineWords = 8,
      texFillCacheSlots = 16,
      texFillRequestWindow = 16,
      packedTexLayout = true
    )

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
    dut.io.sampleRequest.payload.passthrough.nccTableSelect #= false
    dut.io.sampleRequest.payload.passthrough.ds #= 0
    dut.io.sampleRequest.payload.passthrough.dt #= 0
    dut.io.sampleRequest.payload.passthrough.readIdx #= 0
    dut.io.sampleRequest.payload.passthrough.requestId #= 0
  }

  case class Metrics(accepted: Long, produced: Long, cycles: Long) {
    def acceptedPerCycle: Double = accepted.toDouble / cycles.toDouble
    def producedPerCycle: Double = produced.toDouble / cycles.toDouble
  }

  SimConfig.withWave.compile(TmuTextureCache(c)).doSim { dut =>
    dut.clockDomain.forkStimulus(10)
    dut.io.sampleRequest.valid #= false
    dut.io.sampleFetch.ready #= true
    dut.io.texRead.cmd.ready #= true
    dut.io.texRead.rsp.valid #= false
    dut.io.texRead.rsp.last #= false
    dut.io.texRead.rsp.fragment.data #= 0
    dut.io.texRead.rsp.fragment.source #= 0
    dut.io.texRead.rsp.fragment.context #= 0
    dut.io.invalidate #= false
    dut.clockDomain.waitSampling()
    while (dut.io.busy.toBoolean) {
      dut.clockDomain.waitSampling()
    }

    val burstSetupLatency = 12
    val burstBeatLatency = 1

    fork {
      while (true) {
        dut.clockDomain.waitSamplingWhere(
          dut.io.texRead.cmd.valid.toBoolean && dut.io.texRead.cmd.ready.toBoolean
        )
        dut.io.texRead.rsp.valid #= false
        dut.io.texRead.rsp.last #= false
        dut.clockDomain.waitSampling(burstSetupLatency)
        for (beat <- 0 until c.texFillLineWords) {
          dut.io.texRead.rsp.valid #= true
          dut.io.texRead.rsp.fragment.data #= (0x4000 + beat)
          dut.io.texRead.rsp.last #= (beat == c.texFillLineWords - 1)
          dut.clockDomain.waitSampling(burstBeatLatency)
        }
        dut.io.texRead.rsp.valid #= false
        dut.io.texRead.rsp.last #= false
      }
    }

    def warm(addr: Int, bank: Int, bilinear: Boolean = false): Unit = {
      setReq(dut, addr, bank, bilinear)
      dut.io.sampleRequest.valid #= true
      dut.clockDomain.waitSamplingWhere(dut.io.sampleRequest.ready.toBoolean)
      dut.io.sampleRequest.valid #= false
      var seen = false
      var budget = 200
      while (!seen && budget > 0) {
        dut.clockDomain.waitSampling()
        if (dut.io.sampleFetch.valid.toBoolean && dut.io.sampleFetch.ready.toBoolean) seen = true
        budget -= 1
      }
      assert(seen, s"warmup request addr=$addr bank=$bank bilinear=$bilinear did not complete")
    }

    def runWindow(addr: Int, bank: Int, bilinear: Boolean, cycles: Int): Metrics = {
      setReq(dut, addr, bank, bilinear)
      dut.io.sampleRequest.valid #= true
      var accepted = 0L
      var produced = 0L
      for (_ <- 0 until cycles) {
        dut.clockDomain.waitSampling()
        if (dut.io.sampleRequest.valid.toBoolean && dut.io.sampleRequest.ready.toBoolean)
          accepted += 1
        if (dut.io.sampleFetch.valid.toBoolean && dut.io.sampleFetch.ready.toBoolean) produced += 1
      }
      dut.io.sampleRequest.valid #= false
      Metrics(accepted, produced, cycles)
    }

    warm(0, 0, bilinear = false)
    warm(2, 1, bilinear = false)
    warm(512, 2, bilinear = false)
    warm(514, 3, bilinear = false)

    val point = runWindow(0, 0, bilinear = false, cycles = 128)
    val bilinear = runWindow(0, 0, bilinear = true, cycles = 128)

    println(
      f"point_hit_window accepted=${point.accepted}%d produced=${point.produced}%d cycles=${point.cycles}%d accepted_per_cycle=${point.acceptedPerCycle}%.4f produced_per_cycle=${point.producedPerCycle}%.4f"
    )
    println(
      f"bilinear_hit_window accepted=${bilinear.accepted}%d produced=${bilinear.produced}%d cycles=${bilinear.cycles}%d accepted_per_cycle=${bilinear.acceptedPerCycle}%.4f produced_per_cycle=${bilinear.producedPerCycle}%.4f"
    )
  }
}
