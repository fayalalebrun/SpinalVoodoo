package voodoo.mister

import org.scalatest.funsuite.AnyFunSuite
import spinal.core._
import spinal.core.sim._
import scala.language.reflectiveCalls

class MisterResetBoundaryTest extends AnyFunSuite {
  test("reset drains accepted reads and stalled commands before resetting the client") {
    val config = SpinalConfig(defaultConfigForClockDomains = ClockDomainConfig(resetKind = BOOT))
    SimConfig.withConfig(config).withIVerilog.compile(MisterResetBoundary()).doSim { dut =>
      SimTimeout(50000)
      dut.io.request #= true
      dut.io.core.address #= 0x7c00000
      dut.io.core.burstCount #= 1
      dut.io.core.read #= false
      dut.io.core.write #= false
      dut.io.core.writeData #= 0x12345678
      dut.io.core.byteEnable #= 255
      dut.io.ddr.busy #= false
      dut.io.ddr.readData #= 0
      dut.io.ddr.readDataValid #= false
      dut.clockDomain.forkStimulus(10)
      def tick(): Unit = { dut.clockDomain.waitSampling(); sleep(1) }
      def idle(): Unit = {
        dut.io.core.read #= false
        dut.io.core.write #= false
        dut.io.ddr.readDataValid #= false
      }
      def release(): Unit = {
        dut.io.request #= false
        for (_ <- 0 until 24) tick()
        assert(!dut.io.coreReset.toBoolean)
        assert(!dut.io.core.busy.toBoolean)
      }
      for (_ <- 0 until 5) tick()
      assert(dut.io.coreReset.toBoolean && dut.io.core.busy.toBoolean)
      release()
      for (count <- Seq(1, 128); delay <- Seq(0, 4)) {
        dut.io.core.burstCount #= count
        dut.io.core.read #= true
        dut.io.ddr.busy #= true
        tick()
        assert(dut.io.ddr.read.toBoolean)
        dut.io.request #= true
        for (_ <- 0 until 6) {
          tick()
          assert(!dut.io.coreReset.toBoolean)
          assert(dut.io.ddr.read.toBoolean)
          assert(dut.io.ddr.address.toLong == 0x7c00000)
          assert(dut.io.ddr.burstCount.toInt == count)
        }
        dut.io.ddr.busy #= false
        tick()
        dut.io.core.read #= false
        dut.io.request #= false
        for (i <- 0 until count) {
          dut.io.ddr.readDataValid #= false
          for (_ <- 0 until delay) { tick(); assert(!dut.io.coreReset.toBoolean) }
          dut.io.ddr.readDataValid #= true
          dut.io.ddr.readData #= i + 100
          sleep(1)
          assert(!dut.io.coreReset.toBoolean)
          assert(dut.io.core.readDataValid.toBoolean)
          assert(dut.io.core.readData.toInt == i + 100)
          tick()
        }
        idle()
        tick()
        assert(dut.io.coreReset.toBoolean)
        for (_ <- 0 until 12) { tick(); assert(dut.io.coreReset.toBoolean) }
        release()
      }
      dut.io.core.burstCount #= 128
      dut.io.core.read #= true
      for (_ <- 0 until 8) tick()
      dut.io.core.read #= false
      dut.io.request #= true
      for (_ <- 0 until 6) { tick(); assert(!dut.io.coreReset.toBoolean) }
      dut.io.ddr.readDataValid #= true
      for (beat <- 0 until 1024) {
        dut.io.ddr.readData #= beat
        sleep(1)
        assert(!dut.io.coreReset.toBoolean && dut.io.core.readDataValid.toBoolean)
        tick()
      }
      idle()
      tick()
      assert(dut.io.coreReset.toBoolean)
      release()
      dut.io.core.write #= true
      dut.io.core.burstCount #= 1
      dut.io.ddr.busy #= true
      tick()
      dut.io.request #= true
      for (_ <- 0 until 6) {
        tick()
        assert(!dut.io.coreReset.toBoolean && dut.io.ddr.write.toBoolean)
        assert(dut.io.ddr.writeData.toLong == 0x12345678)
        assert(dut.io.ddr.byteEnable.toInt == 255)
      }
      dut.io.ddr.busy #= false
      tick()
      idle()
      tick()
      assert(dut.io.coreReset.toBoolean)
      for (_ <- 0 until 5) { tick(); assert(dut.io.coreReset.toBoolean) }
      release()
      dut.io.core.read #= true
      dut.io.ddr.readDataValid #= true
      tick()
      idle()
      dut.io.request #= true
      for (_ <- 0 until 6) tick()
      assert(dut.io.coreReset.toBoolean)
      assert(!dut.io.ddr.read.toBoolean && !dut.io.ddr.write.toBoolean)
      release()
    }
  }
}
