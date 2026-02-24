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
    // Note: pciFifoFree now comes from busif.pciFifoFree (FIFO availability)
    dut.io.statusInputs.vRetrace #= true
    dut.io.statusInputs.memFifoFree #= 0xffff
    dut.io.statusInputs.pciInterrupt #= false
    dut.io.swapDisplayedBuffer #= 0
    dut.io.swapsPending #= 0

    // Initialize statistics inputs
    dut.io.statisticsIn.pixelsIn #= 0
    dut.io.statisticsIn.chromaFail #= 0
    dut.io.statisticsIn.zFuncFail #= 0
    dut.io.statisticsIn.aFuncFail #= 0
    dut.io.statisticsIn.pixelsOut #= 0

    // Initialize pipelineBusy to false by default
    dut.io.pipelineBusy #= false

    // Command streams are always ready by default
    dut.commands.triangleCmd.ready #= true
    dut.commands.fastfillCmd.ready #= true
    dut.commands.nopCmd.ready #= true
    dut.commands.swapbufferCmd.ready #= true
    dut.commands.ftriangleCmd.ready #= true

    dut.clockDomain.waitSampling()
  }

  /** Get or create BmbDriver for this DUT (cached per test) */
  private val driverCache = new scala.collection.mutable.WeakHashMap[RegisterBank, BmbDriver]()

  private def getDriver(dut: RegisterBank): BmbDriver = {
    driverCache.getOrElseUpdate(dut, BmbDriver(dut.io.bus, dut.clockDomain))
  }

  /** Helper to write a 32-bit value to a BMB register
    *
    * @param dut
    *   RegisterBank device under test
    * @param address
    *   Register address
    * @param data
    *   Data to write
    * @param waitForFifoDrain
    *   If true, waits additional cycles for FIFO to drain (needed for FIFO=Yes registers)
    */
  def bmbWrite(
      dut: RegisterBank,
      address: Long,
      data: Long,
      waitForFifoDrain: Boolean = true
  ): Unit = {
    val driver = getDriver(dut)
    driver.write(address = address, data = data)

    // Wait for FIFO to drain if requested (FIFO=Yes registers need this)
    // BmbDriver.write() returns when command is acknowledged by bus
    // But register update happens later when FIFO drains
    if (waitForFifoDrain) {
      dut.clockDomain.waitSampling()
      dut.clockDomain.waitSampling()
    }
  }

  /** Helper to read a 32-bit value from a BMB register */
  def bmbRead(dut: RegisterBank, address: Long): Long = {
    val driver = getDriver(dut)
    driver.read(address = address).toLong & 0xffffffffL
  }

  test("Status register reads hardware inputs correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Set specific status inputs (pciFifoFree is now from actual FIFO)
      // fbiBusy/trexBusy/sstBusy are wired from pipelineBusy internally
      dut.io.pipelineBusy #= true
      dut.io.swapDisplayedBuffer #= 1
      dut.io.swapsPending #= 2

      dut.clockDomain.waitSampling(2)

      val status = bmbRead(dut, 0x000)

      // PCI FIFO should report full availability (64 = 0x40) since FIFO is empty
      assert((status & 0x3f) == 0x3f, s"PCI FIFO should be 0x3f (63 free), got ${status & 0x3f}")
      assert(((status >> 7) & 1) == 1, "FBI busy should be set (from pipelineBusy)")
      assert(((status >> 8) & 1) == 1, "TREX busy should be set (from pipelineBusy)")
      assert(((status >> 9) & 1) == 1, "SST busy should be set (from pipelineBusy)")
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

      // Track when stream.valid pulses
      var sawValid = false
      var validCycles = 0

      // Fork a monitor to watch for stream.valid pulse
      val monitor = fork {
        for (_ <- 0 until 10) { // Watch for up to 10 cycles
          if (dut.commands.triangleCmd.valid.toBoolean) {
            sawValid = true
            validCycles += 1
          }
          dut.clockDomain.waitSampling()
        }
      }

      // Write to triangleCMD register (W1P requires writing 1)
      bmbWrite(dut, 0x080, 1)

      // Wait for monitor to complete
      monitor.join()

      // Verify we saw exactly one valid pulse
      assert(sawValid, "Triangle command stream should have pulsed valid")
      assert(
        validCycles == 1,
        s"Stream should be valid for exactly 1 cycle, was valid for $validCycles"
      )
    }
  }

  test("Rendering mode registers are read/write") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      val testValue = 0xdeadbeefL

      // Write to fbzColorPath (address changed from 0x0c0 to 0x104 with FIFO implementation)
      bmbWrite(dut, 0x104, testValue)
      dut.clockDomain.waitSampling()

      // Verify it was stored
      assert(
        dut.renderConfig.fbzColorPath.toLong == testValue,
        f"fbzColorPath should be 0x$testValue%08X"
      )

      // Read it back
      val readValue = bmbRead(dut, 0x104)
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

      // Write to fbzMode (Sync=Yes register) - address changed from 0x0cc to 0x110
      bmbWrite(dut, 0x110, 0x12345678L)

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

      // clipLeftRight: bits[9:0]=right, bits[25:16]=left (per SST-1 datasheet)
      val leftX = 10
      val rightX = 640
      val clipLR = (leftX << 16) | rightX

      bmbWrite(dut, 0x118, clipLR)
      dut.clockDomain.waitSampling()

      assert(dut.renderConfig.clipLeftX.toInt == leftX, s"Left clip should be $leftX")
      assert(dut.renderConfig.clipRightX.toInt == rightX, s"Right clip should be $rightX")

      // Read back
      val readBack = bmbRead(dut, 0x118)
      assert((readBack & 0x3ff) == rightX, "Read back right X should match")
      assert(((readBack >> 16) & 0x3ff) == leftX, "Read back left X should match")
    }
  }

  test("Command registers generate Stream transactions") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Helper to test a command stream with forking
      def testCommandStream(
          address: Long,
          stream: spinal.lib.Stream[spinal.lib.NoData],
          name: String
      ): Unit = {
        var sawValid = false
        var validCycles = 0

        val monitor = fork {
          for (_ <- 0 until 10) {
            if (stream.valid.toBoolean) {
              sawValid = true
              validCycles += 1
            }
            dut.clockDomain.waitSampling()
          }
        }

        bmbWrite(dut, address, 1)
        monitor.join()

        assert(sawValid, s"$name command stream should have pulsed valid")
        assert(
          validCycles == 1,
          s"$name stream should be valid for exactly 1 cycle, was $validCycles"
        )
      }

      // Test nopCMD
      testCommandStream(0x120, dut.commands.nopCmd, "NOP")

      // Test fastfillCMD
      testCommandStream(0x124, dut.commands.fastfillCmd, "Fastfill")

      // Test swapbufferCMD
      testCommandStream(0x128, dut.commands.swapbufferCmd, "Swapbuffer")
    }
  }

  test("Command registers apply backpressure when not ready") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Test command stream backpressure with 1-entry buffer:
      // 1. Write IS accepted into FIFO
      // 2. First command drains into buffer, valid becomes true (buffer can hold 1)
      // 3. Second command is BLOCKED until first is consumed
      // 4. When ready becomes true, buffered command fires

      dut.commands.triangleCmd.ready #= false

      var sawFireWhileReady = false

      // Write first command - goes into buffer
      bmbWrite(dut, 0x080, 1)
      dut.clockDomain.waitSamplingWhere(dut.commands.triangleCmd.valid.toBoolean)

      // Verify buffer has the command (valid should be true)
      assert(
        dut.commands.triangleCmd.valid.toBoolean,
        "First command should be in buffer (valid=true)"
      )

      // Write second command - should be blocked (buffer full)
      // Fork the write so we can check bus stalling
      val writer = fork {
        bmbWrite(dut, 0x080, 1)
      }

      // Give time for FIFO to potentially drain (should NOT drain)
      dut.clockDomain.waitSampling(5)

      // Set ready=true to consume first command
      dut.commands.triangleCmd.ready #= true
      for (_ <- 0 until 5) {
        if (dut.commands.triangleCmd.valid.toBoolean && dut.commands.triangleCmd.ready.toBoolean) {
          sawFireWhileReady = true
        }
        dut.clockDomain.waitSampling()
      }

      writer.join()

      // Verify first command was consumed
      assert(sawFireWhileReady, "Command should fire when ready becomes true")

      // Part 2: Verify normal operation when stream.ready is true from the start
      dut.commands.triangleCmd.ready #= true

      var sawValid = false
      var validCycles = 0

      val monitor2 = fork {
        for (_ <- 0 until 10) {
          if (dut.commands.triangleCmd.valid.toBoolean) {
            sawValid = true
            validCycles += 1
          }
          dut.clockDomain.waitSampling()
        }
      }

      bmbWrite(dut, 0x080, 1)
      monitor2.join()

      assert(sawValid, "Stream should pulse valid when ready is true")
      assert(validCycles == 1, s"Stream should be valid for exactly 1 cycle, was $validCycles")
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

      // Read pixelsIn (address changed from 0x110 to 0x14c)
      val pixelsIn = bmbRead(dut, 0x14c)
      assert((pixelsIn & 0xffffff) == 12345, "Pixels in should be 12345")

      // Read chromaFail (address changed from 0x114 to 0x150)
      val chromaFail = bmbRead(dut, 0x150)
      assert((chromaFail & 0xffffff) == 100, "Chroma fail should be 100")

      // Attempt to write (should have no effect)
      bmbWrite(dut, 0x14c, 99999)
      dut.clockDomain.waitSampling()

      // Input should still control the value
      val stillSame = bmbRead(dut, 0x14c)
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

  test("FIFO registers stall when command streams are not ready") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // NEW FIFO BEHAVIOR: Writes are NOT stalled - they're queued in FIFO
      // FIFO drain is blocked, but CPU doesn't stall

      // Initialize pipelineBusy to false
      dut.io.pipelineBusy #= false

      // Test 1: FIFO=Yes register (vertexAx at 0x008) is accepted into FIFO even when triangleCmd is not ready
      dut.commands.triangleCmd.ready #= false

      // Write to vertexAx - should be accepted into FIFO immediately
      bmbWrite(dut, 0x008, 0x12345678, waitForFifoDrain = false)

      // Register should NOT have updated yet (FIFO hasn't drained)
      // Note: vertexAx is SInt(16 bits) so only lower 16 bits matter
      assert(
        dut.triangleGeometry.vertexAx.toInt == 0,
        "Register should not update until FIFO drains"
      )

      // Make stream ready - FIFO should drain
      dut.commands.triangleCmd.ready #= true
      dut.clockDomain.waitSampling(2)

      // Now register should have updated
      assert(
        (dut.triangleGeometry.vertexAx.toLong & 0xffffL) == (0x12345678L & 0xffffL),
        "Register should update after stream becomes ready"
      )

      // Test 2: FIFO=Yes register succeeds immediately when all streams are ready
      dut.commands.triangleCmd.ready #= true
      dut.commands.fastfillCmd.ready #= true
      dut.commands.nopCmd.ready #= true
      dut.commands.swapbufferCmd.ready #= true
      dut.commands.ftriangleCmd.ready #= true

      bmbWrite(dut, 0x010, 0xaabbccddL) // vertexBx - FIFO=Yes register

      // Should have written successfully (only lower 16 bits stored in SInt(16 bits) register)
      assert(
        (dut.triangleGeometry.vertexBx.toLong & 0xffffL) == (0xaabbccddL & 0xffffL),
        "FIFO register should write when streams ready"
      )
    }
  }

  test("FIFO=No registers bypass stream backpressure") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Initialize pipelineBusy to false
      dut.io.pipelineBusy #= false

      // Stall all command streams
      dut.commands.triangleCmd.ready #= false
      dut.commands.fastfillCmd.ready #= false
      dut.commands.nopCmd.ready #= false
      dut.commands.swapbufferCmd.ready #= false
      dut.commands.ftriangleCmd.ready #= false

      // FIFO=No register (fbiInit0 at 0x210) should NOT stall even when streams are not ready
      bmbWrite(dut, 0x210, 0x3) // fbiInit0 - FIFO=No register
      dut.clockDomain.waitSampling()

      // Should have written successfully despite stream backpressure
      assert(
        dut.init.fbiInit0_vgaPassthrough.toBoolean,
        "FIFO=No register should write despite stream backpressure"
      )
      assert(
        dut.init.fbiInit0_graphicsReset.toBoolean,
        "FIFO=No register should write despite stream backpressure"
      )
    }
  }

  test("Sync=Yes registers stall when pipeline is busy") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Test that Sync=Yes register drain is blocked when pipeline is busy
      // 1. Write IS accepted into FIFO even when pipeline is busy
      // 2. FIFO drain is blocked until pipeline becomes not busy
      // 3. Register updates after pipeline becomes not busy

      dut.io.pipelineBusy #= true

      var sawUpdateWhileBusy = false
      var sawUpdateAfterNotBusy = false

      // Fork a monitor to track register updates
      val monitor = fork {
        // Phase 1: Watch while pipeline is busy (should not see update)
        for (_ <- 0 until 5) {
          if (dut.renderConfig.fbzMode.enableClipping.toBoolean) {
            sawUpdateWhileBusy = true
          }
          dut.clockDomain.waitSampling()
        }

        // Phase 2: Set pipelineBusy=false and watch for update
        dut.io.pipelineBusy #= false
        for (_ <- 0 until 5) {
          if (dut.renderConfig.fbzMode.enableClipping.toBoolean) {
            sawUpdateAfterNotBusy = true
          }
          dut.clockDomain.waitSampling()
        }
      }

      // Write to fbzMode - Sync=Yes register at 0x110
      // Use value with enableClipping bit set (bit 0)
      bmbWrite(dut, 0x110, 0x00000001)

      monitor.join()

      // Verify behavior
      assert(
        !sawUpdateWhileBusy,
        "Register should NOT update while pipeline is busy (FIFO drain blocked)"
      )
      assert(
        sawUpdateAfterNotBusy,
        "Register should update after pipeline becomes not busy"
      )

      // Test 2: Sync=No register succeeds immediately regardless of pipeline
      dut.io.pipelineBusy #= false

      bmbWrite(dut, 0x10c, 0x22222222) // alphaMode - Sync=No register

      // Should have written successfully
      assert(
        (dut.renderConfig.alphaMode.toLong & 0xffffffffL) == 0x22222222L,
        "Sync=No register should write when pipeline not busy"
      )
    }
  }

  test("Sync=No registers write immediately regardless of pipeline") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Pipeline is busy but Sync=No register should still write
      dut.io.pipelineBusy #= true

      // fbzColorPath (0x104) is Sync=No register (address changed from fogColor at 0x0e4)
      bmbWrite(dut, 0x104, 0x12345678)
      dut.clockDomain.waitSampling()

      // Should have written successfully despite pipeline being busy
      assert(
        (dut.renderConfig.fbzColorPath.toLong & 0xffffffffL) == 0x12345678L,
        "Sync=No register should write despite pipeline busy"
      )
    }
  }

  test("CPU stalls when FIFO is full (64 entries) - Sync=Yes registers") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Block pipeline to prevent Sync=Yes registers from draining
      dut.io.pipelineBusy #= true
      dut.clockDomain.waitSampling(2)

      // Write 64 FIFO=Yes,Sync=Yes registers to fill the FIFO
      // Use fbzMode (0x110) - Sync=Yes register (address changed from 0x0cc)
      for (i <- 0 until 64) {
        dut.io.bus.cmd.valid #= true
        dut.io.bus.cmd.address #= 0x110
        dut.io.bus.cmd.data #= i
        dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
        dut.io.bus.cmd.length #= 3
        dut.io.bus.cmd.last #= true
        dut.io.bus.cmd.mask #= 0xf
        dut.io.bus.rsp.ready #= true

        dut.clockDomain.waitSampling()

        assert(dut.io.bus.cmd.ready.toBoolean, s"Bus should accept write $i")

        dut.io.bus.cmd.valid #= false
        dut.clockDomain.waitSampling()
      }

      // Now try to write one more (65th entry)
      dut.io.bus.cmd.valid #= true
      dut.io.bus.cmd.address #= 0x110
      dut.io.bus.cmd.data #= 999
      dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
      dut.io.bus.cmd.length #= 3
      dut.io.bus.cmd.last #= true
      dut.io.bus.cmd.mask #= 0xf
      dut.io.bus.rsp.ready #= true

      dut.clockDomain.waitSampling()

      // Bus should stall (FIFO full)
      assert(!dut.io.bus.cmd.ready.toBoolean, "Bus should stall when FIFO is full")

      dut.io.bus.cmd.valid #= false
      dut.clockDomain.waitSampling()
    }
  }

  test("CPU stalls when FIFO is full (64 entries) - command register backpressure") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      setupDut(dut)

      // Block command register stream by not consuming it
      // triangleCMD (0x080) is Sync=No, so won't be blocked by pipelineBusy
      // But it WILL be blocked by stream backpressure
      dut.commands.triangleCmd.ready #= false
      dut.clockDomain.waitSampling(2)

      // Write 65 FIFO=Yes,Sync=No command registers to fill the FIFO + buffer
      // Use triangleCMD (0x080) - command register, Sync=No
      // With 1-entry buffer: first cmd drains to buffer, FIFO holds 64 more
      for (i <- 0 until 65) {
        dut.io.bus.cmd.valid #= true
        dut.io.bus.cmd.address #= 0x080
        dut.io.bus.cmd.data #= i
        dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
        dut.io.bus.cmd.length #= 3
        dut.io.bus.cmd.last #= true
        dut.io.bus.cmd.mask #= 0xf
        dut.io.bus.rsp.ready #= true

        dut.clockDomain.waitSampling()

        assert(dut.io.bus.cmd.ready.toBoolean, s"Bus should accept write $i")

        dut.io.bus.cmd.valid #= false
        dut.clockDomain.waitSampling()
      }

      // Now try to write one more (66th entry)
      dut.io.bus.cmd.valid #= true
      dut.io.bus.cmd.address #= 0x080
      dut.io.bus.cmd.data #= 999
      dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
      dut.io.bus.cmd.length #= 3
      dut.io.bus.cmd.last #= true
      dut.io.bus.cmd.mask #= 0xf
      dut.io.bus.rsp.ready #= true

      dut.clockDomain.waitSampling()

      // Bus should stall (FIFO + buffer full)
      assert(!dut.io.bus.cmd.ready.toBoolean, "Bus should stall when FIFO + buffer are full")

      dut.io.bus.cmd.valid #= false
      dut.clockDomain.waitSampling()
    }
  }
}
