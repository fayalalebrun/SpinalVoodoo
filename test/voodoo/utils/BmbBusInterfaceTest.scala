package voodoo.utils

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim.BmbDriver
import spinal.lib.bus.regif._
import spinal.lib.bus.misc.SizeMapping
import org.scalatest.funsuite.AnyFunSuite

class BmbBusInterfaceTest extends AnyFunSuite {

  // Simple test component using BmbBusInterface (pure RegIf adapter, no FIFO)
  case class BmbRegIfTest() extends Component {
    val io = new Bundle {
      val bus = slave(
        Bmb(
          BmbParameter(
            addressWidth = 10,
            dataWidth = 32,
            sourceWidth = 4,
            contextWidth = 0,
            lengthWidth = 2,
            canRead = true,
            canWrite = true,
            alignment = BmbParameter.BurstAlignement.WORD
          )
        )
      )
      val testReg = out(Bits(32 bits))
      val statusIn = in(UInt(8 bits))
    }

    // Create BMB bus interface using BusInterface factory
    val busif = BmbBusInterface(io.bus, SizeMapping(0x000, 256 Byte), "TST")

    // Test read/write register at 0x000
    val testReg = busif.newRegAt(0x000, "testReg", Secure.NS())
    val testField = testReg.field(Bits(32 bits), AccessType.RW, 0, "Test field")
    io.testReg := testField

    // Test read-only register at 0x004
    val statusReg = busif.newRegAt(0x004, "statusReg", Secure.NS())
    val statusField = statusReg.field(UInt(8 bits), AccessType.RO, 0, "Status field")
    statusField := io.statusIn
  }

  // Note: Using BmbDriver from spinal.lib.bus.bmb.sim for all BMB transactions
  // This abstracts the protocol handshaking and makes tests cleaner

  test("BmbBusInterface read/write register works") {
    SimConfig.withIVerilog.withWave.compile(BmbRegIfTest()).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.statusIn #= 0
      dut.clockDomain.waitSampling()

      val driver = BmbDriver(dut.io.bus, dut.clockDomain)

      // Write a value to testReg
      driver.write(address = 0x000, data = 0xdeadbeefL)
      dut.clockDomain.waitSampling(2)

      // Verify it was written
      assert(
        dut.io.testReg.toLong == 0xdeadbeefL,
        f"testReg should be 0xDEADBEEF, got 0x${dut.io.testReg.toLong}%08X"
      )

      // Read it back
      val readValue = driver.read(address = 0x000)
      assert(readValue == 0xdeadbeefL, f"Read back should be 0xDEADBEEF, got 0x$readValue%08X")

      dut.clockDomain.waitSampling(5)
    }
  }

  test("BmbBusInterface read-only register works") {
    SimConfig.withIVerilog.withWave.compile(BmbRegIfTest()).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.statusIn #= 0x5a
      dut.clockDomain.waitSampling()

      val driver = BmbDriver(dut.io.bus, dut.clockDomain)

      // Read status register
      val statusValue = driver.read(address = 0x004)
      assert(
        (statusValue & 0xff) == 0x5a,
        f"Status should be 0x5A, got 0x${statusValue & 0xff}%02X"
      )

      // Try to write (should have no effect)
      driver.write(address = 0x004, data = 0xff)
      dut.clockDomain.waitSampling(2)

      // Verify status is still from hardware input
      val stillSame = driver.read(address = 0x004)
      assert((stillSame & 0xff) == 0x5a, "Read-only register should not change on write")

      dut.clockDomain.waitSampling(5)
    }
  }

  test("BmbBusInterface handles multiple transactions") {
    SimConfig.withIVerilog.withWave.compile(BmbRegIfTest()).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.statusIn #= 0
      dut.clockDomain.waitSampling()

      val driver = BmbDriver(dut.io.bus, dut.clockDomain)
      val testValues = Seq(0x11111111L, 0x22222222L, 0x33333333L, 0xaaaaaaaaL)

      for (value <- testValues) {
        driver.write(address = 0x000, data = value)
        dut.clockDomain.waitSampling(2)

        val readBack = driver.read(address = 0x000)
        assert(readBack == value, f"Expected 0x$value%08X, got 0x$readBack%08X")
      }

      dut.clockDomain.waitSampling(5)
    }
  }
}
