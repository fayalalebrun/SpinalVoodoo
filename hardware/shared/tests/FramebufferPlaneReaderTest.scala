//> using target.scope test

package voodoo

import org.scalatest.funsuite.AnyFunSuite
import spinal.core._
import spinal.core.sim._
import voodoo.framebuffer.FramebufferPlaneReader

class FramebufferPlaneReaderTest extends AnyFunSuite {
  private def mkConfig =
    Config(
      addressWidth = 12 bits,
      memBurstLengthWidth = 6,
      maxFbDims = (16, 16)
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

  private def sendRspBeat(dut: FramebufferPlaneReader, data: Long, last: Boolean): Unit = {
    dut.io.mem.rsp.valid #= true
    dut.io.mem.rsp.fragment.data #= data
    dut.io.mem.rsp.last #= last
    sleep(1)
    while (!dut.io.mem.rsp.ready.toBoolean) {
      dut.clockDomain.waitSampling()
    }
    dut.clockDomain.waitSampling()
  }

  test("unused same-line statistic generates a constant without storage") {
    val directory = "output/mister/same-line-statistic-regression"
    SpinalConfig(targetDirectory = directory).generateVerilog(FramebufferPlaneReader(mkConfig))
    val path = java.nio.file.Paths.get(directory, "FramebufferPlaneReader.v")
    val rtl =
      new String(java.nio.file.Files.readAllBytes(path), java.nio.charset.StandardCharsets.UTF_8)
    assert(rtl.matches("(?s).*assign io_reqSameLineCount = 32'h0+;.*"))
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
        sendRspBeat(dut, if (beat == 0) 0x56781234L else 0xdef09abcL, beat == 1)
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

      sendRspBeat(dut, 0x56781234L, last = false)
      sendRspBeat(dut, 0xdef09abcL, last = true)
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

  test("prefetch acceptance is backpressured by consume tracking") {
    SimConfig.withIVerilog.compile(FramebufferPlaneReader(mkConfig)).doSim { dut =>
      initDut(dut)

      dut.io.mem.cmd.ready #= true
      dut.io.prefetchReq.valid #= true
      for (idx <- 0 until 64) {
        dut.io.prefetchReq.startAddress #= (idx * 2)
        dut.io.prefetchReq.endAddress #= (idx * 2)
        assert(dut.io.prefetchReq.ready.toBoolean, s"prefetch $idx should be accepted")
        dut.clockDomain.waitSampling()
      }

      dut.io.prefetchReq.startAddress #= 0x200
      dut.io.prefetchReq.endAddress #= 0x200
      dut.clockDomain.waitSampling()
      assert(!dut.io.prefetchReq.ready.toBoolean, "prefetch must stall when consume queue is full")
      dut.io.prefetchReq.valid #= false
    }
  }

  test("direct miss does not consume cached lane") {
    SimConfig.withIVerilog.compile(FramebufferPlaneReader(mkConfig)).doSim { dut =>
      initDut(dut)

      dut.io.prefetchReq.valid #= true
      dut.io.prefetchReq.startAddress #= 0x000
      dut.io.prefetchReq.endAddress #= 0x006
      dut.clockDomain.waitSampling()
      dut.io.prefetchReq.valid #= false

      dut.io.mem.cmd.ready #= true
      while (!dut.io.mem.cmd.valid.toBoolean) dut.clockDomain.waitSampling()
      dut.clockDomain.waitSampling()
      dut.io.mem.cmd.ready #= false

      sendRspBeat(dut, 0x56781234L, last = false)
      sendRspBeat(dut, 0xdef09abcL, last = true)
      dut.io.mem.rsp.valid #= false

      for ((addr, expected) <- Seq(0x000 -> 0x1234L, 0x002 -> 0x5678L)) {
        dut.io.readReq.valid #= true
        dut.io.readReq.address #= addr
        dut.clockDomain.waitSampling()
        assert(dut.io.readReq.ready.toBoolean, f"address 0x$addr%x should be ready")
        dut.io.readReq.valid #= false
        while (!dut.io.readRsp.valid.toBoolean) dut.clockDomain.waitSampling()
        assert(dut.io.readRsp.data.toLong == expected)
        dut.clockDomain.waitSampling()
      }

      dut.io.readReq.valid #= true
      dut.io.readReq.address #= 0x000
      dut.io.mem.cmd.ready #= true
      while (!dut.io.readReq.ready.toBoolean) dut.clockDomain.waitSampling()
      assert(dut.io.mem.cmd.valid.toBoolean, "backward direct miss should issue a memory command")
      dut.clockDomain.waitSampling()
      dut.io.readReq.valid #= false
      dut.io.mem.cmd.ready #= false

      dut.io.mem.rsp.valid #= true
      dut.io.mem.rsp.fragment.source #= 1
      dut.io.mem.rsp.fragment.data #= 0xcafebabeL
      dut.io.mem.rsp.last #= true
      while (!dut.io.mem.rsp.ready.toBoolean) dut.clockDomain.waitSampling()
      dut.clockDomain.waitSampling()
      dut.io.mem.rsp.valid #= false
      dut.io.mem.rsp.fragment.source #= 0

      while (!dut.io.readRsp.valid.toBoolean) dut.clockDomain.waitSampling()
      assert(dut.io.readRsp.data.toLong == 0xbabeL)
      dut.clockDomain.waitSampling()

      dut.io.readReq.valid #= true
      dut.io.readReq.address #= 0x004
      dut.clockDomain.waitSampling()
      assert(dut.io.readReq.ready.toBoolean, "cached lane at 0x004 must survive the direct miss")
      dut.io.readReq.valid #= false
      while (!dut.io.readRsp.valid.toBoolean) dut.clockDomain.waitSampling()
      assert(dut.io.readRsp.data.toLong == 0x9abcL)
    }
  }
}
