//> using target.scope test

package voodoo.utils

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim.BmbDriver
import spinal.lib.bus.regif._
import spinal.lib.bus.misc.SizeMapping
import org.scalatest.funsuite.AnyFunSuite

class PciFifoTest extends AnyFunSuite {

  /** Test component wiring PciFifo between a CPU-side BMB bus and a RegisterBank-side
    * BmbBusInterface. Mirrors the real Core wiring.
    */
  case class PciFifoTestHarness() extends Component {
    val busParams = BmbParameter(
      addressWidth = 10,
      dataWidth = 32,
      sourceWidth = 4,
      contextWidth = 0,
      lengthWidth = 2,
      canRead = true,
      canWrite = true,
      alignment = BmbParameter.BurstAlignement.WORD
    )

    val io = new Bundle {
      val cpuBus = slave(Bmb(busParams))
      val cmdStream = master(Stream(NoData()))
      val pipelineBusy = in Bool ()
      val testReg = out(Bits(32 bits))
      val directReg = out(Bits(32 bits))
      val syncDrained = out Bool ()
      val fifoEmpty = out Bool ()
      val texWrite = slave Stream (TexWritePayload())
      val texDrain = master Stream (TexWritePayload())
      val floatShadowValid = out Bool ()
      val floatShadowAddr = out UInt (busParams.access.addressWidth bits)
      val floatShadowRaw = out SInt (50 bits)
    }

    // RegisterBank side: pure RegIf adapter
    val regBus = Bmb(busParams)
    implicit val moduleName: spinal.lib.bus.regif.ClassName =
      spinal.lib.bus.regif.ClassName("PciFifoTestHarness")
    val busif = BmbBusInterface(regBus, SizeMapping(0x000, 256 Byte), "TST")
    import busif.FieldFloatAlias

    // Command register at 0x010 (FIFO=Yes, Sync=No)
    val (cmdReg, cmdStream) = busif.newCommandReg(0x010, "cmdReg", RegisterCategory.fifoNoSync)
    cmdReg.field(Bits(32 bits), AccessType.RW, 0, "Command data")
    io.cmdStream <> cmdStream

    // Regular FIFO register at 0x014 (FIFO=Yes, Sync=No)
    val fifoReg = busif.newRegAtWithCategory(0x014, "fifoReg", RegisterCategory.fifoNoSync)
    val fifoField = fifoReg.field(Bits(32 bits), AccessType.RW, 0, "FIFO test field")
    fifoField.withFloatAlias(12, 4)
    io.testReg := fifoField

    // Direct register at 0x020 (FIFO=No - bypass)
    val directReg = busif.newRegAtWithCategory(0x020, "directReg", RegisterCategory.bypassFifo)
    val directField = directReg.field(Bits(32 bits), AccessType.RW, 0, "Direct field")
    io.directReg := directField

    // Sync=Yes register at 0x030
    val syncReg = busif.newRegAtWithCategory(0x030, "syncReg", RegisterCategory.fifoWithSync)
    val syncField = syncReg.field(Bits(32 bits), AccessType.RW, 0, "Sync field")

    // Instantiate PciFifo using metadata from busif
    val pciFifo = PciFifo(
      busParams = busParams,
      categories = busif.getCategories,
      floatAliases = busif.getFloatAliases,
      commandAddresses = busif.getCommandStreamReady.keys.toSeq
    )

    // Wire PciFifo between CPU bus and register bus
    io.cpuBus <> pciFifo.io.cpuSide
    pciFifo.io.regSide <> regBus

    // Control signals
    pciFifo.io.pipelineBusy := io.pipelineBusy
    pciFifo.io.commandReady := Vec(busif.getCommandStreamReady.values.toSeq)
    pciFifo.io.wasEnqueuedAddr := 0

    // Texture write pass-through
    pciFifo.io.texWrite <> io.texWrite
    pciFifo.io.texDrain <> io.texDrain

    // Status outputs
    io.syncDrained := pciFifo.io.syncDrained
    io.fifoEmpty := pciFifo.io.fifoEmpty
    io.floatShadowValid := pciFifo.io.floatShadow.valid
    io.floatShadowAddr := pciFifo.io.floatShadow.address
    io.floatShadowRaw := pciFifo.io.floatShadow.raw
  }

  def setupDut(dut: PciFifoTestHarness): Unit = {
    dut.clockDomain.forkStimulus(period = 10)
    dut.io.cmdStream.ready #= true
    dut.io.pipelineBusy #= false
    dut.io.texWrite.valid #= false
    dut.io.texWrite.pciAddr #= 0
    dut.io.texWrite.data #= 0
    dut.io.texWrite.mask #= 0
    dut.io.texDrain.ready #= true
    dut.clockDomain.waitSampling()
  }

  test("CPU read passthrough returns regSide data") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      val driver = BmbDriver(dut.io.cpuBus, dut.clockDomain)
      driver.write(address = 0x020, data = 0x13579bdfL)
      dut.clockDomain.waitSampling()

      val readBack = driver.read(address = 0x020)
      assert(
        readBack == 0x13579bdfL,
        f"Read should return direct register data, got 0x$readBack%08X"
      )
    }
  }

  test("Float alias write enqueues converted integer write and shadow") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      val driver = BmbDriver(dut.io.cpuBus, dut.clockDomain)
      val floatBits = java.lang.Float.floatToRawIntBits(1.5f).toLong & 0xffffffffL

      driver.write(address = 0x094, data = floatBits)

      dut.clockDomain.waitSamplingWhere(dut.io.floatShadowValid.toBoolean)
      assert(
        dut.io.floatShadowAddr.toLong == 0x014,
        f"Shadow addr should target 0x014, got 0x${dut.io.floatShadowAddr.toLong}%03X"
      )
      assert(
        dut.io.floatShadowRaw.toBigInt == BigInt(1610612736L),
        s"Unexpected shadow raw ${dut.io.floatShadowRaw.toBigInt}"
      )

      dut.clockDomain.waitSampling(2)
      assert(
        dut.io.testReg.toLong == 24,
        s"Converted 12.4 fixed value should be 24, got ${dut.io.testReg.toLong}"
      )
    }
  }

  test("FIFO command register responds immediately when enqueued") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      dut.io.cmdStream.ready #= false // Command stream NOT ready (pipeline stalled)
      dut.clockDomain.waitSampling()

      val driver = BmbDriver(dut.io.cpuBus, dut.clockDomain)

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
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      dut.io.cmdStream.ready #= false // Pipeline stalled
      dut.clockDomain.waitSampling()

      val driver = BmbDriver(dut.io.cpuBus, dut.clockDomain)

      // With 1-entry buffer per command stream, total capacity is 65:
      // - 64 in PCI FIFO
      // - 1 in command stream buffer (first cmd drains into it immediately)
      // Write 65 commands - all should complete without stalling
      for (i <- 0 until 65) {
        driver.write(address = 0x010, data = i)
      }

      // Now FIFO + buffer are full - check that cmd.ready is false
      dut.clockDomain.waitSampling()
      dut.io.cpuBus.cmd.valid #= true
      dut.io.cpuBus.cmd.address #= 0x010
      dut.io.cpuBus.cmd.data #= 65
      dut.io.cpuBus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
      dut.io.cpuBus.cmd.length #= 3
      dut.io.cpuBus.cmd.last #= true
      dut.io.cpuBus.cmd.mask #= 0xf

      dut.clockDomain.waitSampling()

      // FIFO + buffer are full, so cmd.ready should be false
      assert(!dut.io.cpuBus.cmd.ready.toBoolean, "Bus should stall when FIFO + buffer are full")

      dut.io.cpuBus.cmd.valid #= false
      dut.clockDomain.waitSampling(5)
    }
  }

  test("FIFO drains when command stream becomes ready") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      dut.io.cpuBus.cmd.valid #= false
      dut.io.cpuBus.rsp.ready #= true
      dut.io.cmdStream.ready #= false // Start with pipeline stalled
      dut.clockDomain.waitSampling()

      // Write 10 commands using manual protocol
      for (i <- 0 until 10) {
        dut.io.cpuBus.cmd.valid #= true
        dut.io.cpuBus.cmd.address #= 0x010
        dut.io.cpuBus.cmd.data #= 1 // W1P requires bit 0 set
        dut.io.cpuBus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
        dut.io.cpuBus.cmd.length #= 3
        dut.io.cpuBus.cmd.last #= true
        dut.io.cpuBus.cmd.mask #= 0xf

        // Wait for cmd.ready (transaction fires on this edge)
        while (!dut.io.cpuBus.cmd.ready.toBoolean) {
          dut.clockDomain.waitSampling()
        }
        dut.clockDomain.waitSampling() // Fire happens here
        dut.io.cpuBus.cmd.valid #= false // De-assert immediately after fire

        // Wait for response
        while (!dut.io.cpuBus.rsp.valid.toBoolean) {
          dut.clockDomain.waitSampling()
        }
      }

      // Commands are queued but not drained
      // Now make stream ready - FIFO should drain
      dut.io.cmdStream.ready #= true

      // Monitor command stream and count how many drain
      var drainCount = 0
      for (_ <- 0 until 50) {
        dut.clockDomain.waitSampling()
        if (dut.io.cmdStream.valid.toBoolean && dut.io.cmdStream.ready.toBoolean) {
          drainCount += 1
        }
      }

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

  test("Command drains do not insert artificial bubbles") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      dut.io.cpuBus.cmd.valid #= false
      dut.io.cpuBus.rsp.ready #= true
      dut.io.cmdStream.ready #= false
      dut.clockDomain.waitSampling()

      for (_ <- 0 until 6) {
        dut.io.cpuBus.cmd.valid #= true
        dut.io.cpuBus.cmd.address #= 0x010
        dut.io.cpuBus.cmd.data #= 1
        dut.io.cpuBus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
        dut.io.cpuBus.cmd.length #= 3
        dut.io.cpuBus.cmd.last #= true
        dut.io.cpuBus.cmd.mask #= 0xf

        while (!dut.io.cpuBus.cmd.ready.toBoolean) {
          dut.clockDomain.waitSampling()
        }
        dut.clockDomain.waitSampling()
        dut.io.cpuBus.cmd.valid #= false
        while (!dut.io.cpuBus.rsp.valid.toBoolean) {
          dut.clockDomain.waitSampling()
        }
      }

      dut.io.cmdStream.ready #= true

      var seen = 0
      var gap = 0
      var maxGap = 0
      for (_ <- 0 until 20) {
        dut.clockDomain.waitSampling()
        if (dut.io.cmdStream.valid.toBoolean && dut.io.cmdStream.ready.toBoolean) {
          seen += 1
          gap = 0
        } else if (seen > 0 && seen < 6) {
          gap += 1
          maxGap = maxGap.max(gap)
        }
      }

      assert(seen == 6, s"Expected 6 command drains, got $seen")
      assert(maxGap == 0, s"Expected no bubble between command drains, saw gap $maxGap")
    }
  }

  test("FIFO=No registers bypass FIFO and write immediately") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      // Stall command stream
      dut.io.cmdStream.ready #= false
      dut.clockDomain.waitSampling()

      val driver = BmbDriver(dut.io.cpuBus, dut.clockDomain)

      // Direct (FIFO=No) register at 0x020 should write immediately
      driver.write(address = 0x020, data = 0xdeadbeefL)
      dut.clockDomain.waitSampling()

      assert(
        dut.io.directReg.toLong == 0xdeadbeefL,
        f"FIFO=No register should write immediately, got 0x${dut.io.directReg.toLong}%08X"
      )
    }
  }

  test("Sync=Yes registers stall drain when pipeline is busy") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      dut.io.pipelineBusy #= true
      dut.clockDomain.waitSampling()

      val driver = BmbDriver(dut.io.cpuBus, dut.clockDomain)

      // Write to Sync=Yes register (0x030) - should be accepted into FIFO
      driver.write(address = 0x030, data = 0x12345678L)

      // Wait - register should NOT update while pipeline is busy
      dut.clockDomain.waitSampling(5)

      // syncDrained should not have fired
      assert(
        !dut.io.syncDrained.toBoolean,
        "Sync register should not drain while pipeline is busy"
      )

      // Release pipeline
      dut.io.pipelineBusy #= false
      dut.clockDomain.waitSampling(5)

      // Now syncDrained should have pulsed
      // (checked indirectly: FIFO should be empty after drain)
      assert(
        dut.io.fifoEmpty.toBoolean,
        "FIFO should be empty after pipeline releases"
      )
    }
  }

  test("Texture writes flow through FIFO alongside register writes") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      val driver = BmbDriver(dut.io.cpuBus, dut.clockDomain)

      // Write a texture entry
      dut.io.texWrite.valid #= true
      dut.io.texWrite.pciAddr #= 0x12345
      dut.io.texWrite.data #= 0xaabbccddL
      dut.io.texWrite.mask #= 0xf

      // Wait for it to be accepted
      dut.clockDomain.waitSamplingWhere(dut.io.texWrite.ready.toBoolean)
      dut.io.texWrite.valid #= false
      dut.clockDomain.waitSampling()

      // Should drain out on texDrain
      var sawDrain = false
      var drainAddr = 0L
      var drainData = 0L
      for (_ <- 0 until 10) {
        if (dut.io.texDrain.valid.toBoolean && dut.io.texDrain.ready.toBoolean) {
          sawDrain = true
          drainAddr = dut.io.texDrain.pciAddr.toLong
          drainData = dut.io.texDrain.data.toLong
        }
        dut.clockDomain.waitSampling()
      }

      assert(sawDrain, "Texture write should drain from FIFO")
      assert(drainAddr == 0x12345, f"Drain address should be 0x12345, got 0x$drainAddr%05X")
      assert(
        drainData == 0xaabbccddL,
        f"Drain data should be 0xAABBCCDD, got 0x$drainData%08X"
      )
    }
  }

  test("CPU stalls when FIFO is full (64 entries) - Sync=Yes registers") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      // Block pipeline to prevent Sync=Yes registers from draining
      dut.io.pipelineBusy #= true
      dut.clockDomain.waitSampling(2)

      // Write 64 Sync=Yes registers to fill the FIFO
      for (i <- 0 until 64) {
        dut.io.cpuBus.cmd.valid #= true
        dut.io.cpuBus.cmd.address #= 0x030
        dut.io.cpuBus.cmd.data #= i
        dut.io.cpuBus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
        dut.io.cpuBus.cmd.length #= 3
        dut.io.cpuBus.cmd.last #= true
        dut.io.cpuBus.cmd.mask #= 0xf
        dut.io.cpuBus.rsp.ready #= true

        dut.clockDomain.waitSampling()

        assert(dut.io.cpuBus.cmd.ready.toBoolean, s"Bus should accept write $i")

        dut.io.cpuBus.cmd.valid #= false
        dut.clockDomain.waitSampling()
      }

      // Now try to write one more (65th entry)
      dut.io.cpuBus.cmd.valid #= true
      dut.io.cpuBus.cmd.address #= 0x030
      dut.io.cpuBus.cmd.data #= 999
      dut.io.cpuBus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
      dut.io.cpuBus.cmd.length #= 3
      dut.io.cpuBus.cmd.last #= true
      dut.io.cpuBus.cmd.mask #= 0xf
      dut.io.cpuBus.rsp.ready #= true

      dut.clockDomain.waitSampling()

      // Bus should stall (FIFO full)
      assert(!dut.io.cpuBus.cmd.ready.toBoolean, "Bus should stall when FIFO is full")

      dut.io.cpuBus.cmd.valid #= false
      dut.clockDomain.waitSampling()
    }
  }

  test("CPU stalls when FIFO is full (64 entries) - command register backpressure") {
    SimConfig.withIVerilog.withWave.compile(PciFifoTestHarness()).doSim { dut =>
      setupDut(dut)

      // Block command register stream by not consuming it
      dut.io.cmdStream.ready #= false
      dut.clockDomain.waitSampling(2)

      // Write 65 command registers to fill the FIFO + buffer
      // With 1-entry buffer: first cmd drains to buffer, FIFO holds 64 more
      for (i <- 0 until 65) {
        dut.io.cpuBus.cmd.valid #= true
        dut.io.cpuBus.cmd.address #= 0x010
        dut.io.cpuBus.cmd.data #= i
        dut.io.cpuBus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
        dut.io.cpuBus.cmd.length #= 3
        dut.io.cpuBus.cmd.last #= true
        dut.io.cpuBus.cmd.mask #= 0xf
        dut.io.cpuBus.rsp.ready #= true

        dut.clockDomain.waitSampling()

        assert(dut.io.cpuBus.cmd.ready.toBoolean, s"Bus should accept write $i")

        dut.io.cpuBus.cmd.valid #= false
        dut.clockDomain.waitSampling()
      }

      // Now try to write one more (66th entry)
      dut.io.cpuBus.cmd.valid #= true
      dut.io.cpuBus.cmd.address #= 0x010
      dut.io.cpuBus.cmd.data #= 999
      dut.io.cpuBus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
      dut.io.cpuBus.cmd.length #= 3
      dut.io.cpuBus.cmd.last #= true
      dut.io.cpuBus.cmd.mask #= 0xf
      dut.io.cpuBus.rsp.ready #= true

      dut.clockDomain.waitSampling()

      // Bus should stall (FIFO + buffer full)
      assert(!dut.io.cpuBus.cmd.ready.toBoolean, "Bus should stall when FIFO + buffer are full")

      dut.io.cpuBus.cmd.valid #= false
      dut.clockDomain.waitSampling()
    }
  }
}
