package voodoo.utils

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.regif._
import spinal.lib.bus.misc.SizeMapping
import org.scalatest.funsuite.AnyFunSuite

class BmbBusInterfaceTest extends AnyFunSuite {

  // Simple test component using BmbBusInterface
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

  def bmbWrite(dut: BmbRegIfTest, address: Long, data: Long): Unit = {
    dut.io.bus.cmd.valid #= true
    dut.io.bus.cmd.address #= address
    dut.io.bus.cmd.data #= data
    dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
    dut.io.bus.cmd.length #= 3 // 4 bytes
    dut.io.bus.cmd.last #= true
    dut.io.bus.cmd.mask #= 0xf
    dut.io.bus.rsp.ready #= true

    dut.clockDomain.waitSampling()

    dut.io.bus.cmd.valid #= false
    dut.clockDomain.waitSampling()
  }

  def bmbRead(dut: BmbRegIfTest, address: Long): Long = {
    dut.io.bus.cmd.valid #= true
    dut.io.bus.cmd.address #= address
    dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.READ
    dut.io.bus.cmd.length #= 3 // 4 bytes
    dut.io.bus.cmd.last #= true
    dut.io.bus.rsp.ready #= true

    dut.clockDomain.waitSampling()

    // Wait for response
    while (!dut.io.bus.rsp.valid.toBoolean) {
      dut.clockDomain.waitSampling()
    }

    val data = dut.io.bus.rsp.data.toLong
    dut.io.bus.cmd.valid #= false
    dut.clockDomain.waitSampling()

    data & 0xffffffffL
  }

  test("BmbBusInterface read/write register works") {
    SimConfig.withIVerilog.withWave.compile(BmbRegIfTest()).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.statusIn #= 0
      dut.io.bus.cmd.valid #= false
      dut.io.bus.rsp.ready #= false
      dut.clockDomain.waitSampling()

      // Write a value to testReg
      bmbWrite(dut, 0x000, 0xdeadbeefL)
      dut.clockDomain.waitSampling(2)

      // Verify it was written
      assert(
        dut.io.testReg.toLong == 0xdeadbeefL,
        f"testReg should be 0xDEADBEEF, got 0x${dut.io.testReg.toLong}%08X"
      )

      // Read it back
      val readValue = bmbRead(dut, 0x000)
      assert(readValue == 0xdeadbeefL, f"Read back should be 0xDEADBEEF, got 0x$readValue%08X")

      dut.clockDomain.waitSampling(5)
    }
  }

  test("BmbBusInterface read-only register works") {
    SimConfig.withIVerilog.withWave.compile(BmbRegIfTest()).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.statusIn #= 0x5a
      dut.io.bus.cmd.valid #= false
      dut.io.bus.rsp.ready #= false
      dut.clockDomain.waitSampling()

      // Read status register
      val statusValue = bmbRead(dut, 0x004)
      assert(
        (statusValue & 0xff) == 0x5a,
        f"Status should be 0x5A, got 0x${statusValue & 0xff}%02X"
      )

      // Try to write (should have no effect)
      bmbWrite(dut, 0x004, 0xff)
      dut.clockDomain.waitSampling(2)

      // Verify status is still from hardware input
      val stillSame = bmbRead(dut, 0x004)
      assert((stillSame & 0xff) == 0x5a, "Read-only register should not change on write")

      dut.clockDomain.waitSampling(5)
    }
  }

  test("BmbBusInterface handles multiple transactions") {
    SimConfig.withIVerilog.withWave.compile(BmbRegIfTest()).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.statusIn #= 0
      dut.io.bus.cmd.valid #= false
      dut.io.bus.rsp.ready #= false
      dut.clockDomain.waitSampling()

      val testValues = Seq(0x11111111L, 0x22222222L, 0x33333333L, 0xaaaaaaaaL)

      for (value <- testValues) {
        bmbWrite(dut, 0x000, value)
        dut.clockDomain.waitSampling(2)

        val readBack = bmbRead(dut, 0x000)
        assert(readBack == value, f"Expected 0x$value%08X, got 0x$readBack%08X")
      }

      dut.clockDomain.waitSampling(5)
    }
  }
}
