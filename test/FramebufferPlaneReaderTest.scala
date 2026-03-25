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
        trace = TraceConfig(enabled = true)
      )

  private def initDut(dut: FramebufferPlaneReader): Unit = {
    dut.clockDomain.forkStimulus(10)
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

  test("adjacent reads collapse into one burst") {
    SimConfig.withIVerilog.compile(FramebufferPlaneReader(mkConfig)).doSim { dut =>
      initDut(dut)

      val addrs = Seq(0x000, 0x004, 0x008)
      for (addr <- addrs) {
        dut.io.readReq.valid #= true
        dut.io.readReq.address #= addr
        dut.clockDomain.waitSampling()
        assert(dut.io.readReq.ready.toBoolean)
      }
      dut.io.readReq.valid #= false

      dut.io.mem.cmd.ready #= true
      dut.clockDomain.waitSampling()
      assert(dut.io.mem.cmd.valid.toBoolean)
      assert(dut.io.mem.cmd.fragment.address.toLong == 0x000)
      assert(dut.io.mem.cmd.fragment.length.toInt == 11)

      dut.io.readRsp.ready #= false
      for (beat <- 0 until 3) {
        dut.io.mem.rsp.valid #= true
        dut.io.mem.rsp.fragment.data #= (0x12340000L + beat)
        dut.clockDomain.waitSampling()
      }
      dut.io.mem.rsp.valid #= false
      dut.io.readRsp.ready #= true

      for (beat <- 0 until 3) {
        dut.clockDomain.waitSampling()
        assert(dut.io.readRsp.valid.toBoolean)
        assert(dut.io.readRsp.data.toLong == (0x12340000L + beat))
      }
    }
  }
}
