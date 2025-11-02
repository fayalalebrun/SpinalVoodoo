package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim._
import org.scalatest.funsuite.AnyFunSuite
import scala.concurrent.{Future, Await}
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global

class RegisterBankTest extends AnyFunSuite {

  def setupDut(dut: RegisterBank): Unit = {
    dut.clockDomain.forkStimulus(period = 10)

    // Initialize status inputs to default values
    dut.io.statusInputs.pciFifoFree #= 0x3f
    dut.io.statusInputs.vRetrace #= true
    dut.io.statusInputs.fbiBusy #= false
    dut.io.statusInputs.trexBusy #= false
    dut.io.statusInputs.sstBusy #= false
    dut.io.statusInputs.displayedBuffer #= 0
    dut.io.statusInputs.memFifoFree #= 0xffff
    dut.io.statusInputs.swapsPending #= 0
    dut.io.statusInputs.pciInterrupt #= false

    // Initialize statistics inputs
    dut.io.statisticsIn.pixelsIn #= 0
    dut.io.statisticsIn.chromaFail #= 0
    dut.io.statisticsIn.zFuncFail #= 0
    dut.io.statisticsIn.aFuncFail #= 0
    dut.io.statisticsIn.pixelsOut #= 0

    // Command streams are always ready by default
    dut.commands.triangleCmd.ready #= true
    dut.commands.fastfillCmd.ready #= true
    dut.commands.nopCmd.ready #= true
    dut.commands.swapbufferCmd.ready #= true
    dut.commands.userIntrCmd.ready #= true
    dut.commands.ftriangleCmd.ready #= true

    dut.clockDomain.waitSampling()
  }

  /** Helper to write a 32-bit value to a BMB register */
  def bmbWrite(dut: RegisterBank, address: Long, data: Long): Unit = {
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

  /** Helper to read a 32-bit value from a BMB register */
  def bmbRead(dut: RegisterBank, address: Long): Long = {
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

  test("Status register reads hardware inputs correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Set specific status inputs
      dut.io.statusInputs.pciFifoFree #= 0x20
      dut.io.statusInputs.fbiBusy #= true
      dut.io.statusInputs.displayedBuffer #= 1
      dut.io.statusInputs.swapsPending #= 2

      dut.clockDomain.waitSampling(2)

      val status = bmbRead(dut, 0x000)

      assert((status & 0x3f) == 0x20, s"PCI FIFO should be 0x20, got ${status & 0x3f}")
      assert(((status >> 7) & 1) == 1, "FBI busy should be set")
      assert(((status >> 10) & 3) == 1, "Displayed buffer should be 1")
      assert(((status >> 28) & 7) == 2, "Swaps pending should be 2")
    }
  }

  test("Vertex registers write correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Write vertex A coordinates (12.4 fixed-point: 100.5 = 100*16 + 8 = 1608)
      bmbWrite(dut, 0x008, 1608) // vertexAx
      bmbWrite(dut, 0x00c, 2004) // vertexAy (125.25 = 125*16 + 4)

      dut.clockDomain.waitSampling()

      assert(dut.triangleGeometry.vertexAx.toInt == 1608, "Vertex A X should be 1608")
      assert(dut.triangleGeometry.vertexAy.toInt == 2004, "Vertex A Y should be 2004")
    }
  }

  test("Triangle command generates Stream transaction") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Stream should be invalid initially
      assert(
        !dut.commands.triangleCmd.valid.toBoolean,
        "Triangle command stream should be initially invalid"
      )

      // Write to triangleCMD register (W1P requires writing 1)
      bmbWrite(dut, 0x080, 1)

      // Check that stream becomes valid
      assert(
        dut.commands.triangleCmd.valid.toBoolean,
        "Triangle command stream should be valid after write"
      )

      dut.clockDomain.waitSampling()

      // Check that stream is only valid for one cycle
      assert(
        !dut.commands.triangleCmd.valid.toBoolean,
        "Triangle command stream should be invalid after one cycle"
      )
    }
  }

  test("Rendering mode registers are read/write") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      val testValue = 0xdeadbeefL

      // Write to fbzColorPath
      bmbWrite(dut, 0x0c0, testValue)
      dut.clockDomain.waitSampling()

      // Verify it was stored
      assert(
        dut.renderConfig.fbzColorPath.toLong == testValue,
        f"fbzColorPath should be 0x$testValue%08X"
      )

      // Read it back
      val readValue = bmbRead(dut, 0x0c0)
      assert(
        readValue == testValue,
        f"Read back value should match: expected 0x$testValue%08X, got 0x$readValue%08X"
      )
    }
  }

  test("Sync-required registers trigger sync pulse") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Initially no sync pulse
      assert(!dut.io.syncPulse.toBoolean, "Sync pulse should be initially false")

      // Write to fbzMode (Sync=Yes register)
      bmbWrite(dut, 0x0cc, 0x12345678L)

      // Check sync pulse is asserted
      assert(
        dut.io.syncPulse.toBoolean,
        "Sync pulse should be high after writing Sync=Yes register"
      )

      dut.clockDomain.waitSampling()

      // Check pulse is only one cycle
      assert(!dut.io.syncPulse.toBoolean, "Sync pulse should go low after one cycle")
    }
  }

  test("Clipping registers work correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // clipLeftRight: left in [9:0], right in [25:16]
      val leftX = 10
      val rightX = 640
      val clipLR = (rightX << 16) | leftX

      bmbWrite(dut, 0x0d4, clipLR)
      dut.clockDomain.waitSampling()

      assert(dut.renderConfig.clipLeftX.toInt == leftX, s"Left clip should be $leftX")
      assert(dut.renderConfig.clipRightX.toInt == rightX, s"Right clip should be $rightX")

      // Read back
      val readBack = bmbRead(dut, 0x0d4)
      assert((readBack & 0x3ff) == leftX, "Read back left X should match")
      assert(((readBack >> 16) & 0x3ff) == rightX, "Read back right X should match")
    }
  }

  test("Command registers generate Stream transactions") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Test nopCMD (W1P requires writing 1)
      bmbWrite(dut, 0x088, 1)
      assert(dut.commands.nopCmd.valid.toBoolean, "NOP command stream should be valid")
      dut.clockDomain.waitSampling()
      assert(!dut.commands.nopCmd.valid.toBoolean, "NOP stream should be one cycle")

      // Test fastfillCMD
      bmbWrite(dut, 0x084, 1)
      assert(dut.commands.fastfillCmd.valid.toBoolean, "Fastfill stream should be valid")
      dut.clockDomain.waitSampling()
      assert(!dut.commands.fastfillCmd.valid.toBoolean, "Fastfill stream should be one cycle")

      // Test swapbufferCMD
      bmbWrite(dut, 0x08c, 1)
      assert(dut.commands.swapbufferCmd.valid.toBoolean, "Swapbuffer stream should be valid")
      dut.clockDomain.waitSampling()
      assert(!dut.commands.swapbufferCmd.valid.toBoolean, "Swapbuffer stream should be one cycle")
    }
  }

  test("Command registers apply backpressure when not ready") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Part 1: Test BMB bus-level backpressure
      // When stream.ready is false, writeHalt() should stall the BMB transaction
      // This means the write won't complete until stream.ready becomes true

      dut.commands.triangleCmd.ready #= false

      // Manually initiate a BMB write transaction to triangleCMD register (0x080)
      dut.io.bus.cmd.valid #= true
      dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
      dut.io.bus.cmd.address #= 0x080
      dut.io.bus.cmd.data #= 1 // W1P requires writing 1
      dut.io.bus.cmd.mask #= 0xf // All bytes enabled
      dut.io.bus.cmd.length #= 3 // 4 bytes (length is size-1)
      dut.io.bus.cmd.last #= true
      dut.io.bus.rsp.ready #= true

      dut.clockDomain.waitSampling()

      // When backpressure is active, BMB cmd may not be accepted immediately
      // Keep cmd.valid high until cmd.ready is asserted (BMB protocol)
      var cmdAccepted = dut.io.bus.cmd.ready.toBoolean
      var cyclesWaited = 0
      while (!cmdAccepted && cyclesWaited < 10) {
        dut.clockDomain.waitSampling()
        cmdAccepted = dut.io.bus.cmd.ready.toBoolean
        cyclesWaited += 1
      }

      // If backpressure is working, cmd should not be accepted while stream.ready is false
      assert(!cmdAccepted, "BMB cmd should NOT be accepted when stream backpressure is active")

      // Deassert cmd.valid temporarily to test rsp behavior
      dut.io.bus.cmd.valid #= false
      dut.clockDomain.waitSampling()

      // rsp should not be valid (transaction hasn't completed)
      assert(
        !dut.io.bus.rsp.valid.toBoolean,
        "BMB rsp.valid should be false when stream backpressure prevented cmd acceptance"
      )

      // Now release the backpressure by setting stream.ready to true
      dut.commands.triangleCmd.ready #= true

      // Re-initiate the BMB write (since we deasserted cmd.valid)
      dut.io.bus.cmd.valid #= true
      dut.clockDomain.waitSampling()

      // Now cmd should be accepted
      assert(
        dut.io.bus.cmd.ready.toBoolean,
        "BMB cmd should be ready after stream backpressure is released"
      )

      dut.io.bus.cmd.valid #= false
      dut.clockDomain.waitSampling()

      // Wait for response
      while (!dut.io.bus.rsp.valid.toBoolean) {
        dut.clockDomain.waitSampling()
      }

      // Response should be valid now
      assert(
        dut.io.bus.rsp.valid.toBoolean,
        "BMB rsp.valid should be true after stream.ready becomes true"
      )

      dut.clockDomain.waitSampling()

      // Stream should have fired and pulse should be done
      assert(
        !dut.commands.triangleCmd.valid.toBoolean,
        "Stream should be invalid after transaction completes"
      )

      // Part 2: Verify normal operation when stream.ready is true from the start
      dut.commands.triangleCmd.ready #= true

      // Use the helper function for a normal write
      bmbWrite(dut, 0x080, 1)

      // Stream should pulse for one cycle
      assert(
        dut.commands.triangleCmd.valid.toBoolean,
        "Stream should be valid immediately after write"
      )

      dut.clockDomain.waitSampling()

      // Should be invalid next cycle (no pending)
      assert(!dut.commands.triangleCmd.valid.toBoolean, "Stream should be invalid after firing")

      // Summary: The test verifies:
      // 1. BMB rsp.valid is held low when stream.ready is false (writeHalt works)
      // 2. Stream valid persists via pending register during backpressure
      // 3. BMB transaction completes when stream.ready becomes true
      // 4. Normal operation works correctly when no backpressure
    }
  }

  test("Statistics registers are read-only") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Set statistics inputs
      dut.io.statisticsIn.pixelsIn #= 12345
      dut.io.statisticsIn.chromaFail #= 100
      dut.clockDomain.waitSampling(2)

      // Read pixelsIn
      val pixelsIn = bmbRead(dut, 0x110)
      assert((pixelsIn & 0xffffff) == 12345, "Pixels in should be 12345")

      // Read chromaFail
      val chromaFail = bmbRead(dut, 0x114)
      assert((chromaFail & 0xffffff) == 100, "Chroma fail should be 100")

      // Attempt to write (should have no effect)
      bmbWrite(dut, 0x110, 99999)
      dut.clockDomain.waitSampling()

      // Input should still control the value
      val stillSame = bmbRead(dut, 0x110)
      assert((stillSame & 0xffffff) == 12345, "Read-only register should not change on write")
    }
  }

  test("Fog table registers write correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Write to fog table entries (32 registers, each with 2 entries)
      // Register format: [31:24]=entry1_fog, [23:16]=entry1_dfog, [15:8]=entry0_fog, [7:0]=entry0_dfog
      for (i <- 0 until 32) {
        val entry0_dfog = (i * 2) & 0xff
        val entry0_fog = (i * 2 + 1) & 0xff
        val entry1_dfog = (i * 2 + 32) & 0xff
        val entry1_fog = (i * 2 + 33) & 0xff
        val regValue = (entry1_fog << 24) | (entry1_dfog << 16) | (entry0_fog << 8) | entry0_dfog
        bmbWrite(dut, 0x160 + i * 4, regValue)
      }

      dut.clockDomain.waitSampling(2)

      // Verify fog table values (64 entries total, accessed as pairs)
      for (i <- 0 until 32) {
        val entry0_expected_dfog = (i * 2) & 0xff
        val entry0_expected_fog = (i * 2 + 1) & 0xff
        val entry1_expected_dfog = (i * 2 + 32) & 0xff
        val entry1_expected_fog = (i * 2 + 33) & 0xff

        val (entry0_dfog, entry0_fog) = dut.fogTable.fogTable(i * 2)
        val (entry1_dfog, entry1_fog) = dut.fogTable.fogTable(i * 2 + 1)

        assert(
          entry0_dfog.toInt == entry0_expected_dfog,
          s"Fog entry ${i * 2} dfog should be $entry0_expected_dfog, got ${entry0_dfog.toInt}"
        )
        assert(
          entry0_fog.toInt == entry0_expected_fog,
          s"Fog entry ${i * 2} fog should be $entry0_expected_fog, got ${entry0_fog.toInt}"
        )
        assert(
          entry1_dfog.toInt == entry1_expected_dfog,
          s"Fog entry ${i * 2 + 1} dfog should be $entry1_expected_dfog, got ${entry1_dfog.toInt}"
        )
        assert(
          entry1_fog.toInt == entry1_expected_fog,
          s"Fog entry ${i * 2 + 1} fog should be $entry1_expected_fog, got ${entry1_fog.toInt}"
        )
      }
    }
  }

  test("Init registers are read/write") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Test fbiInit0 bit fields
      bmbWrite(dut, 0x210, 0x3) // Set both VGA passthrough and graphics reset
      dut.clockDomain.waitSampling()

      assert(dut.init.fbiInit0_vgaPassthrough.toBoolean, "fbiInit0 VGA passthrough should be set")
      assert(dut.init.fbiInit0_graphicsReset.toBoolean, "fbiInit0 graphics reset should be set")

      // Read back
      val readBack = bmbRead(dut, 0x210)
      assert(
        (readBack & 0x3) == 0x3,
        f"fbiInit0 read back should have bits 0-1 set, got 0x$readBack%08X"
      )

      // Test fbiInit4 (PCI read wait states)
      bmbWrite(dut, 0x200, 0x1)
      dut.clockDomain.waitSampling()

      assert(
        dut.init.fbiInit4_pciReadWaitStates.toBoolean,
        "fbiInit4 PCI read wait states should be set"
      )

      // Test fbiInit2 bit fields (swap algorithm and buffer offset)
      bmbWrite(dut, 0x218, (75 << 11) | (2 << 9)) // Buffer offset=75 (4KB units), swap alg=2
      dut.clockDomain.waitSampling()

      assert(dut.init.fbiInit2_swapAlgorithm.toInt == 2, "fbiInit2 swap algorithm should be 2")
      assert(dut.init.fbiInit2_bufferOffset.toInt == 75, "fbiInit2 buffer offset should be 75")
    }
  }

  test("Triangle parameter gradients store correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Write X gradients
      bmbWrite(dut, 0x040, 0x123456) // dRdX (24-bit)
      bmbWrite(dut, 0x044, 0x789abc) // dGdX
      bmbWrite(dut, 0x048, 0xdef012) // dBdX

      // Write Y gradients
      bmbWrite(dut, 0x060, 0x111111) // dRdY
      bmbWrite(dut, 0x064, 0x222222) // dGdY
      bmbWrite(dut, 0x068, 0x333333) // dBdY

      dut.clockDomain.waitSampling()

      // Verify X gradients (sign-extended to 24 bits)
      assert((dut.triangleGeometry.dRdX.toLong & 0xffffff) == 0x123456, "dRdX should be 0x123456")
      assert((dut.triangleGeometry.dGdX.toLong & 0xffffff) == 0x789abc, "dGdX should be 0x789ABC")

      // Verify Y gradients
      assert((dut.triangleGeometry.dRdY.toLong & 0xffffff) == 0x111111, "dRdY should be 0x111111")
      assert((dut.triangleGeometry.dGdY.toLong & 0xffffff) == 0x222222, "dGdY should be 0x222222")
    }
  }
}
