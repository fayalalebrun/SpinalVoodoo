package voodoo.audit

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo._
import voodoo.core.FramebufferMemSubsystem
import voodoo.framebuffer.{FramebufferPlaneBuffer, FramebufferPlaneReader}

object FramebufferCacheSynthGen {
  private def baseConfig =
    Config
      .voodoo1()
      .copy(
        addressWidth = 26 bits,
        memBurstLengthWidth = 6,
        maxFbDims = (800, 600),
        fbWriteBufferCount = 2,
        texFillLineWords = 8,
        useTexFillCache = false,
        trace = TraceConfig(enabled = false)
      )

  val cache16 = baseConfig.copy(
    fbWriteBufferLineWords = 16,
    useFbWriteBuffer = true
  )

  val cache64 = baseConfig.copy(
    fbWriteBufferLineWords = 64,
    useFbWriteBuffer = true
  )

  val direct = baseConfig.copy(
    fbWriteBufferLineWords = 16,
    useFbWriteBuffer = false
  )
}

case class FbWriteBufferSynthTop(c: Config) extends Component {
  val io = new Bundle {
    val readReq = slave(Stream(FramebufferPlaneBuffer.ReadReq(c)))
    val readRsp = master(Stream(FramebufferPlaneBuffer.ReadRsp()))
    val writeReq = slave(Stream(FramebufferPlaneBuffer.WriteReq(c)))
    val flush = in Bool ()
    val mem = master(Bmb(FramebufferPlaneBuffer.bmbParams(c)))
    val busy = out Bool ()
    val writeStallCycles = out UInt (32 bits)
    val writeDrainCount = out UInt (32 bits)
    val writeFullDrainCount = out UInt (32 bits)
    val writePartialDrainCount = out UInt (32 bits)
    val writeRotateBlockedCycles = out UInt (32 bits)
  }

  val dut = FramebufferPlaneBuffer(c)
  dut.io.readReq <> io.readReq
  io.readRsp <> dut.io.readRsp
  dut.io.writeReq <> io.writeReq
  dut.io.flush := io.flush
  io.mem <> dut.io.mem
  io.busy := dut.io.busy
  io.writeStallCycles := dut.io.writeStallCycles
  io.writeDrainCount := dut.io.writeDrainCount
  io.writeFullDrainCount := dut.io.writeFullDrainCount
  io.writePartialDrainCount := dut.io.writePartialDrainCount
  io.writeRotateBlockedCycles := dut.io.writeRotateBlockedCycles
}

case class FbReadCacheSynthTop(c: Config) extends Component {
  val io = new Bundle {
    val prefetchReq = slave(Stream(FramebufferPlaneReader.PrefetchReq(c)))
    val readReq = slave(Stream(FramebufferPlaneBuffer.ReadReq(c)))
    val readRsp = master(Stream(FramebufferPlaneBuffer.ReadRsp()))
    val mem = master(Bmb(FramebufferPlaneReader.bmbParams(c)))
    val busy = out Bool ()
    val fillHits = out UInt (32 bits)
    val fillMisses = out UInt (32 bits)
    val fillBurstCount = out UInt (32 bits)
    val fillBurstBeats = out UInt (32 bits)
    val fillStallCycles = out UInt (32 bits)
    val maxOccupancy = out UInt (8 bits)
  }

  val dut = FramebufferPlaneReader(c)
  dut.io.prefetchReq <> io.prefetchReq
  dut.io.readReq <> io.readReq
  io.readRsp <> dut.io.readRsp
  io.mem <> dut.io.mem
  io.busy := dut.io.busy
  io.fillHits := dut.io.fillHits
  io.fillMisses := dut.io.fillMisses
  io.fillBurstCount := dut.io.fillBurstCount
  io.fillBurstBeats := dut.io.fillBurstBeats
  io.fillStallCycles := dut.io.fillStallCycles
  io.maxOccupancy := dut.io.maxOccupancy
}

case class FbMemSubsystemSynthTop(c: Config) extends Component {
  val io = new Bundle {
    val colorWrite = slave(Stream(FramebufferPlaneBuffer.WriteReq(c)))
    val auxWrite = slave(Stream(FramebufferPlaneBuffer.WriteReq(c)))
    val colorReadReq = slave(Stream(FramebufferPlaneBuffer.ReadReq(c)))
    val colorReadRsp = master(Stream(FramebufferPlaneBuffer.ReadRsp()))
    val auxReadReq = slave(Stream(FramebufferPlaneBuffer.ReadReq(c)))
    val auxReadRsp = master(Stream(FramebufferPlaneBuffer.ReadRsp()))
    val prefetchColor = slave(Stream(FramebufferPlaneReader.PrefetchReq(c)))
    val prefetchAux = slave(Stream(FramebufferPlaneReader.PrefetchReq(c)))
    val lfbReadBus = slave(Bmb(Lfb.fbReadBmbParams(c)))
    val flush = in Bool ()
    val fbMemWrite = master(Bmb(Core.fbMemBmbParams(c)))
    val fbColorReadMem = master(Bmb(Core.fbMemBmbParams(c)))
    val fbAuxReadMem = master(Bmb(Core.fbMemBmbParams(c)))
    val status = out(voodoo.core.FramebufferMemStatus())
    val stats = out(voodoo.core.FramebufferMemStats())
  }

  val dut = FramebufferMemSubsystem(c)
  dut.io.colorWrite <> io.colorWrite
  dut.io.auxWrite <> io.auxWrite
  dut.io.colorReadReq <> io.colorReadReq
  io.colorReadRsp <> dut.io.colorReadRsp
  dut.io.auxReadReq <> io.auxReadReq
  io.auxReadRsp <> dut.io.auxReadRsp
  dut.io.prefetchColor <> io.prefetchColor
  dut.io.prefetchAux <> io.prefetchAux
  dut.io.lfbReadBus <> io.lfbReadBus
  dut.io.flush := io.flush
  io.fbMemWrite <> dut.io.fbMemWrite
  io.fbColorReadMem <> dut.io.fbColorReadMem
  io.fbAuxReadMem <> dut.io.fbAuxReadMem
  io.status := dut.io.status
  io.stats := dut.io.stats
}

object FbWriteBuffer16SynthTopGen extends App {
  GenSupport
    .de10Verilog("output/fb-cache-audit/rtl")
    .generate(
      FbWriteBufferSynthTop(FramebufferCacheSynthGen.cache16)
        .setDefinitionName("FbWriteBuffer16SynthTop")
    )
}

object FbWriteBuffer64SynthTopGen extends App {
  GenSupport
    .de10Verilog("output/fb-cache-audit/rtl")
    .generate(
      FbWriteBufferSynthTop(FramebufferCacheSynthGen.cache64)
        .setDefinitionName("FbWriteBuffer64SynthTop")
    )
}

object FbReadCache64SynthTopGen extends App {
  GenSupport
    .de10Verilog("output/fb-cache-audit/rtl")
    .generate(
      FbReadCacheSynthTop(FramebufferCacheSynthGen.cache64)
        .setDefinitionName("FbReadCache64SynthTop")
    )
}

object FbMemSubsystemDirectSynthTopGen extends App {
  GenSupport
    .de10Verilog("output/fb-cache-audit/rtl")
    .generate(
      FbMemSubsystemSynthTop(FramebufferCacheSynthGen.direct)
        .setDefinitionName("FbMemSubsystemDirectSynthTop")
    )
}

object FbMemSubsystemCached64SynthTopGen extends App {
  GenSupport
    .de10Verilog("output/fb-cache-audit/rtl")
    .generate(
      FbMemSubsystemSynthTop(FramebufferCacheSynthGen.cache64)
        .setDefinitionName("FbMemSubsystemCached64SynthTop")
    )
}
