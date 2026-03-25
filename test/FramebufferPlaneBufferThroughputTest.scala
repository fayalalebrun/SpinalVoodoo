package voodoo

import org.scalatest.funsuite.AnyFunSuite
import spinal.core._
import spinal.core.sim._
import spinal.lib.bus.bmb.sim._

class FramebufferPlaneBufferThroughputTest extends AnyFunSuite {
  private def mkConfig =
    Config
      .voodoo1()
      .copy(
        addressWidth = 12 bits,
        memBurstLengthWidth = 4,
        fbWriteBufferLineWords = 2,
        fbWriteBufferCount = 2,
        useFbWriteBuffer = true,
        useTexFillCache = false,
        trace = TraceConfig(enabled = true)
      )

  private def initDut(dut: FramebufferPlaneBuffer): Unit = {
    dut.clockDomain.forkStimulus(10)
    dut.io.readReq.valid #= false
    dut.io.readReq.address #= 0
    dut.io.readRsp.ready #= true
    dut.io.writeReq.valid #= false
    dut.io.writeReq.address #= 0
    dut.io.writeReq.data #= 0
    dut.io.writeReq.mask #= 0
    dut.io.flush #= false
    dut.clockDomain.waitSampling()
  }

  private def attachMemory(dut: FramebufferPlaneBuffer): BmbMemoryAgent = {
    val memory = new BmbMemoryAgent(1 << dut.c.addressWidth.value)
    memory.addPort(
      bus = dut.io.mem,
      busAddress = 0,
      clockDomain = dut.clockDomain,
      withDriver = true,
      withStall = false
    )
    memory
  }

  private def driveWrite(
      dut: FramebufferPlaneBuffer,
      address: Int,
      data: Long,
      mask: Int = 0xf
  ): Unit = {
    dut.io.writeReq.valid #= true
    dut.io.writeReq.address #= address
    dut.io.writeReq.data #= data
    dut.io.writeReq.mask #= mask
  }

  private def clearWrite(dut: FramebufferPlaneBuffer): Unit = {
    dut.io.writeReq.valid #= false
  }

  test("sequential line rollover stays one-per-cycle") {
    val config = mkConfig
    SimConfig.withIVerilog.withWave.compile(FramebufferPlaneBuffer(config)).doSim { dut =>
      initDut(dut)
      attachMemory(dut)

      val addrs = Seq(0x000, 0x004, 0x008, 0x00c)
      for ((addr, idx) <- addrs.zipWithIndex) {
        driveWrite(dut, addr, 0x11110000L + idx)
        dut.clockDomain.waitSampling()
        assert(dut.io.writeReq.ready.toBoolean, s"write $idx should be accepted")
      }

      clearWrite(dut)
      dut.clockDomain.waitSampling(20)
    }
  }

  test("drain starts while the next line is being filled") {
    val config = mkConfig
    SimConfig.withIVerilog.withWave.compile(FramebufferPlaneBuffer(config)).doSim { dut =>
      initDut(dut)
      attachMemory(dut)

      val addrs = Seq(0x000, 0x004, 0x008)
      for ((addr, idx) <- addrs.zipWithIndex) {
        driveWrite(dut, addr, 0x22220000L + idx)
        dut.clockDomain.waitSampling()
        assert(dut.io.writeReq.ready.toBoolean, s"write $idx should be accepted")
      }

      var sawDrain = false
      for (_ <- 0 until 10) {
        dut.clockDomain.waitSampling()
        if (dut.slotDraining(0).toBoolean || dut.slotDraining(1).toBoolean) sawDrain = true
      }
      assert(sawDrain, "expected write-combiner drain to start")

      clearWrite(dut)
      dut.clockDomain.waitSampling(10)
    }
  }

}
