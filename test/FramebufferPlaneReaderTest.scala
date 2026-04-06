//> using target.scope test

package voodoo

import org.scalatest.funsuite.AnyFunSuite
import spinal.core._
import spinal.core.sim._

class FramebufferPlaneReaderTest extends AnyFunSuite {
  private def mkConfig =
    Config
      .voodoo1()
      .copy(
        addressWidth = 12 bits,
        memBurstLengthWidth = 6,
        fbWriteBufferLineWords = 4,
        fbWriteBufferCount = 2,
        useFbWriteBuffer = true,
        useTexFillCache = false,
        maxFbDims = (16, 16),
        trace = TraceConfig(enabled = true)
      )

  private def mkLengthFixerConfig =
    mkConfig.copy(
      addressWidth = 20 bits,
      maxFbDims = (128, 16)
    )

  private def initDut(dut: FramebufferPlaneReader): Unit = {
    dut.clockDomain.forkStimulus(10)
    dut.io.prefetchReq.valid #= false
    dut.io.prefetchReq.startAddress #= 0
    dut.io.prefetchReq.endAddress #= 0
    dut.io.readReq.valid #= false
    dut.io.readReq.address #= 0
    dut.io.readRsp.ready #= true
    dut.io.mem.cmd.ready #= false
    dut.io.mem.rsp.valid #= false
    dut.io.mem.rsp.last #= true
    dut.io.mem.rsp.fragment.data #= 0
    dut.io.mem.rsp.fragment.source #= 0
    dut.io.mem.rsp.fragment.context #= 0
    dut.io.mem.rsp.fragment.opcode #= 0
    dut.clockDomain.waitSampling()
  }

  test("span prefetch issues exact-length burst") {
    SimConfig.withIVerilog.compile(FramebufferPlaneReader(mkConfig)).doSim { dut =>
      initDut(dut)

      dut.io.prefetchReq.valid #= true
      dut.io.prefetchReq.startAddress #= 0x000
      dut.io.prefetchReq.endAddress #= 0x006
      dut.clockDomain.waitSampling()
      dut.io.prefetchReq.valid #= false

      dut.io.mem.cmd.ready #= true
      while (!dut.io.mem.cmd.valid.toBoolean) {
        dut.clockDomain.waitSampling()
      }
      assert(dut.io.mem.cmd.valid.toBoolean)
      assert(dut.io.mem.cmd.fragment.address.toLong == 0x000)
      assert(dut.io.mem.cmd.fragment.length.toInt == 7)
      dut.clockDomain.waitSampling()

      dut.io.readRsp.ready #= false
      for (beat <- 0 until 2) {
        dut.io.mem.rsp.valid #= true
        dut.io.mem.rsp.fragment.data #= (if (beat == 0) 0x56781234L else 0xdef09abcL)
        dut.io.mem.rsp.last #= (beat == 1)
        dut.clockDomain.waitSampling()
      }
      dut.io.mem.rsp.valid #= false
      dut.io.mem.rsp.last #= true
      dut.clockDomain.waitSampling()
      dut.io.readReq.valid #= true
      dut.io.readReq.address #= 0x000
      dut.clockDomain.waitSampling()
      assert(dut.io.readReq.ready.toBoolean)
      dut.io.readReq.valid #= false
      dut.io.readRsp.ready #= true

      while (!dut.io.readRsp.valid.toBoolean) dut.clockDomain.waitSampling()
      assert(dut.io.readRsp.data.toLong == 0x1234L)
      dut.clockDomain.waitSampling()

      dut.io.readReq.valid #= true
      dut.io.readReq.address #= 0x002
      dut.clockDomain.waitSampling()
      assert(dut.io.readReq.ready.toBoolean)
      dut.io.readReq.valid #= false
      while (!dut.io.readRsp.valid.toBoolean) dut.clockDomain.waitSampling()
      assert(dut.io.readRsp.data.toLong == 0x5678L)

      dut.clockDomain.waitSampling()
      dut.io.readReq.valid #= true
      dut.io.readReq.address #= 0x004
      dut.clockDomain.waitSampling()
      assert(dut.io.readReq.ready.toBoolean)
      dut.io.readReq.valid #= false
      while (!dut.io.readRsp.valid.toBoolean) dut.clockDomain.waitSampling()
      assert(dut.io.readRsp.data.toLong == 0x9abcL)

      dut.clockDomain.waitSampling()
      dut.io.readReq.valid #= true
      dut.io.readReq.address #= 0x006
      dut.clockDomain.waitSampling()
      assert(dut.io.readReq.ready.toBoolean)
      dut.io.readReq.valid #= false
      while (!dut.io.readRsp.valid.toBoolean) dut.clockDomain.waitSampling()
      assert(dut.io.readRsp.data.toLong == 0xdef0L)
    }
  }

  test("odd-lane span only enqueues requested halfwords") {
    SimConfig.withIVerilog.compile(FramebufferPlaneReader(mkConfig)).doSim { dut =>
      initDut(dut)

      dut.io.prefetchReq.valid #= true
      dut.io.prefetchReq.startAddress #= 0x002
      dut.io.prefetchReq.endAddress #= 0x006
      dut.clockDomain.waitSampling()
      dut.io.prefetchReq.valid #= false

      dut.io.mem.cmd.ready #= true
      while (!dut.io.mem.cmd.valid.toBoolean) dut.clockDomain.waitSampling()
      assert(dut.io.mem.cmd.fragment.address.toLong == 0x000)
      assert(dut.io.mem.cmd.fragment.length.toInt == 7)
      dut.clockDomain.waitSampling()

      dut.io.mem.rsp.valid #= true
      dut.io.mem.rsp.fragment.data #= 0x56781234L
      dut.io.mem.rsp.last #= false
      dut.clockDomain.waitSampling()
      dut.io.mem.rsp.fragment.data #= 0xdef09abcL
      dut.io.mem.rsp.last #= true
      dut.clockDomain.waitSampling()
      dut.io.mem.rsp.valid #= false

      for ((addr, expected) <- Seq(0x002 -> 0x5678L, 0x004 -> 0x9abcL, 0x006 -> 0xdef0L)) {
        dut.io.readReq.valid #= true
        dut.io.readReq.address #= addr
        dut.clockDomain.waitSampling()
        assert(dut.io.readReq.ready.toBoolean, f"address 0x$addr%x should be ready")
        dut.io.readReq.valid #= false
        while (!dut.io.readRsp.valid.toBoolean) dut.clockDomain.waitSampling()
        assert(dut.io.readRsp.data.toLong == expected)
        dut.clockDomain.waitSampling()
      }
    }
  }

  test("length fixer splits long span bursts") {
    SimConfig.withIVerilog.compile(FramebufferPlaneReader(mkLengthFixerConfig)).doSim { dut =>
      initDut(dut)

      dut.io.prefetchReq.valid #= true
      dut.io.prefetchReq.startAddress #= 0x040
      dut.io.prefetchReq.endAddress #= 0x0a0
      dut.clockDomain.waitSampling()
      dut.io.prefetchReq.valid #= false

      dut.io.mem.cmd.ready #= true
      val seen = scala.collection.mutable.ArrayBuffer.empty[(Long, Int)]
      while (seen.length < 2) {
        dut.clockDomain.waitSampling()
        if (dut.io.mem.cmd.valid.toBoolean && dut.io.mem.cmd.ready.toBoolean) {
          seen += ((dut.io.mem.cmd.fragment.address.toLong, dut.io.mem.cmd.fragment.length.toInt))
        }
      }
      assert(seen == Seq((0x040L, 63), (0x080L, 35)))
    }
  }
}
