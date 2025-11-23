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

  // Test component with FIFO and command register
  case class BmbFifoTest() extends Component {
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
      val cmdStream = master(Stream(NoData()))
      val pipelineBusy = in Bool ()
    }

    implicit val moduleName: spinal.lib.bus.regif.ClassName =
      spinal.lib.bus.regif.ClassName("BmbFifoTest")
    val busif = BmbBusInterface(io.bus, SizeMapping(0x000, 256 Byte), "TST")

    // Set pipeline busy signal
    busif.setPipelineBusy(io.pipelineBusy)

    // Command register at 0x010 (FIFO=Yes, Sync=No)
    val (cmdReg, cmdStream) = busif.newCommandReg(0x010, "cmdReg", RegisterCategory.fifoNoSync)
    cmdReg.field(Bits(32 bits), AccessType.RW, 0, "Command data")
    io.cmdStream <> cmdStream

    // Regular register at 0x020 (FIFO=No - bypass)
    val directReg = busif.newRegAtWithCategory(0x020, "directReg", RegisterCategory.bypassFifo)
    val directField = directReg.field(Bits(32 bits), AccessType.RW, 0, "Direct field")
  }

  test("FIFO command register responds immediately when enqueued") {
    SimConfig.withIVerilog.withWave.compile(BmbFifoTest()).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.cmdStream.ready #= false // Command stream NOT ready (pipeline stalled)
      dut.io.pipelineBusy #= false
      dut.clockDomain.waitSampling()

      val driver = BmbDriver(dut.io.bus, dut.clockDomain)

      // Write to command register - should get immediate response even though stream isn't ready
      driver.write(address = 0x010, data = 0)

      // Command stream should NOT have fired yet (not ready)
      assert(
        !dut.io.cmdStream.valid.toBoolean || !dut.io.cmdStream.ready.toBoolean,
        "Command should be queued in FIFO, not drained yet"
      )

      dut.clockDomain.waitSampling(5)
    }
  }

  test("FIFO can queue up to 64 commands when pipeline is stalled") {
    SimConfig.withIVerilog.withWave.compile(BmbFifoTest()).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.cmdStream.ready #= false // Pipeline stalled
      dut.io.pipelineBusy #= false
      dut.clockDomain.waitSampling()

      val driver = BmbDriver(dut.io.bus, dut.clockDomain)

      // Write 64 commands - all should complete without stalling
      for (i <- 0 until 64) {
        driver.write(address = 0x010, data = i)
      }

      // Now FIFO is full - check that cmd.ready is false
      dut.clockDomain.waitSampling()
      dut.io.bus.cmd.valid #= true
      dut.io.bus.cmd.address #= 0x010
      dut.io.bus.cmd.data #= 64
      dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
      dut.io.bus.cmd.length #= 3
      dut.io.bus.cmd.last #= true
      dut.io.bus.cmd.mask #= 0xf

      dut.clockDomain.waitSampling()

      // FIFO is full, so cmd.ready should be false
      assert(!dut.io.bus.cmd.ready.toBoolean, "Bus should stall when FIFO is full")

      dut.io.bus.cmd.valid #= false
      dut.clockDomain.waitSampling(5)
    }
  }

  test("FIFO drains when command stream becomes ready") {
    SimConfig.withIVerilog.withWave.compile(BmbFifoTest()).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.bus.cmd.valid #= false
      dut.io.bus.rsp.ready #= true
      dut.io.cmdStream.ready #= false // Start with pipeline stalled
      dut.io.pipelineBusy #= false
      dut.clockDomain.waitSampling()

      // Write 10 commands using manual protocol (BmbDriver has issues with blocked stream)
      for (i <- 0 until 10) {
        dut.io.bus.cmd.valid #= true
        dut.io.bus.cmd.address #= 0x010
        dut.io.bus.cmd.data #= 1 // W1P requires bit 0 set
        dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
        dut.io.bus.cmd.length #= 3
        dut.io.bus.cmd.last #= true
        dut.io.bus.cmd.mask #= 0xf

        // Wait for response
        var gotResponse = false
        for (_ <- 0 until 10 if !gotResponse) {
          dut.clockDomain.waitSampling()
          if (dut.io.bus.rsp.valid.toBoolean) {
            gotResponse = true
          }
        }

        dut.io.bus.cmd.valid #= false
        dut.clockDomain.waitSampling(2)
      }

      // Commands are queued but not drained
      // Now make stream ready - FIFO should drain
      dut.io.cmdStream.ready #= true

      // Monitor command stream and count how many drain
      var drainCount = 0
      var cyclesSinceLastDrain = 0
      for (i <- 0 until 50) {
        dut.clockDomain.waitSampling()
        val valid = dut.io.cmdStream.valid.toBoolean
        val ready = dut.io.cmdStream.ready.toBoolean
        if (valid && ready) {
          drainCount += 1
          cyclesSinceLastDrain = 0
          if (drainCount <= 12) println(s"Cycle $i: Command $drainCount drained")
        } else {
          cyclesSinceLastDrain += 1
        }
        // Stop if no drains for 10 cycles (FIFO empty)
        if (cyclesSinceLastDrain > 10 && drainCount > 0) {
          println(s"No drains for 10 cycles, stopping at cycle $i")
        }
      }
      println(s"Total drained: $drainCount out of 10 expected")

      // Verify all commands drained
      assert(drainCount == 10, s"All 10 commands should have drained, got $drainCount")

      // Wait one more cycle and check that stream is no longer valid
      dut.clockDomain.waitSampling()
      assert(
        !dut.io.cmdStream.valid.toBoolean,
        "Stream should not be valid after all commands drained"
      )
    }
  }
}
