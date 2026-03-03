package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib.bus.bmb.sim._
import org.scalatest.funsuite.AnyFunSuite

class WriteTest extends AnyFunSuite {

  // Default stride: fbiInit1[7:4]=5 → 5*128 bytes/row → 640 pixels/row
  val defaultStride = 640

  def setupDut(dut: Write): Unit = {
    dut.clockDomain.forkStimulus(period = 10)
    dut.i.fromPipeline.valid #= false
    dut.i.fromPipeline.fbBaseAddr #= 0
    dut.i.fromPipeline.fbPixelStride #= defaultStride
    dut.o.fbWrite.rsp.valid #= false
    dut.clockDomain.waitSampling()
  }

  case class PixelWrite(
      x: Int,
      y: Int,
      data16: Int,
      fbBaseAddr: Long = 0
  )

  def sendPixel(dut: Write, pixel: PixelWrite): Unit = {
    dut.i.fromPipeline.valid #= true
    dut.i.fromPipeline.coords(0) #= pixel.x
    dut.i.fromPipeline.coords(1) #= pixel.y
    dut.i.fromPipeline.data #= pixel.data16
    dut.i.fromPipeline.fbBaseAddr #= pixel.fbBaseAddr
    dut.i.fromPipeline.fbPixelStride #= defaultStride
    dut.clockDomain.waitSampling()
  }

  def clearPixel(dut: Write): Unit = {
    dut.i.fromPipeline.valid #= false
    dut.clockDomain.waitSampling()
  }

  def waitForReady(dut: Write): Unit = {
    while (!dut.i.fromPipeline.ready.toBoolean) {
      dut.clockDomain.waitSampling()
    }
  }

  def calculateExpectedAddress(
      fbBaseAddr: Long,
      x: Int,
      y: Int,
      stride: Int = defaultStride
  ): Long = {
    val pixelOffset = y * stride + x
    fbBaseAddr + pixelOffset * 2
  }

  def alignToWord(addr: Long): Long = addr & ~0x3L

  def laneHi(addr: Long): Boolean = (addr & 0x2L) != 0

  def expectedMask(addr: Long): Int = if (laneHi(addr)) 0xc else 0x3

  def expectedDataWord(data16: Int, addr: Long): Long =
    if (laneHi(addr)) (data16.toLong << 16) else (data16.toLong & 0xffffL)

  def readMemoryHalf(memory: BmbMemoryAgent, addr: Int): Int = {
    val byte0 = memory.getByte(addr + 0) & 0xff
    val byte1 = memory.getByte(addr + 1) & 0xff
    (byte1 << 8) | byte0
  }

  test("Write converts pixel coordinates to aligned command address") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)

      val fbBaseAddr = 0x1000000L
      val pixel = PixelWrite(x = 11, y = 5, data16 = 0xabcd, fbBaseAddr = fbBaseAddr)
      sendPixel(dut, pixel)

      assert(dut.o.fbWrite.cmd.valid.toBoolean, "BMB cmd should be valid")

      val planeAddr = calculateExpectedAddress(fbBaseAddr, pixel.x, pixel.y)
      val expectedAddr = alignToWord(planeAddr)
      val actualAddr = dut.o.fbWrite.cmd.address.toLong

      assert(
        actualAddr == expectedAddr,
        f"Address mismatch: expected 0x$expectedAddr%08X, got 0x$actualAddr%08X"
      )
      assert(dut.o.fbWrite.cmd.opcode.toInt == 1, "Opcode should be WRITE")
      assert(dut.o.fbWrite.cmd.length.toInt == 3, "Length should be 3 (4 bytes)")
      assert(
        dut.o.fbWrite.cmd.mask.toInt == expectedMask(planeAddr),
        f"Mask mismatch at lane=${if (laneHi(planeAddr)) "hi" else "lo"}"
      )
      assert(dut.o.fbWrite.cmd.last.toBoolean, "Last flag should be set")

      clearPixel(dut)
      dut.clockDomain.waitSampling(5)
    }
  }

  test("Write handles Stream handshaking correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)

      assert(!dut.o.fbWrite.cmd.valid.toBoolean, "cmd should be invalid when input invalid")

      sendPixel(dut, PixelWrite(0, 0, 0x1234))
      assert(dut.o.fbWrite.cmd.valid.toBoolean, "cmd should be valid when input is valid")

      clearPixel(dut)
      dut.clockDomain.waitSampling(3)
    }
  }

  test("Write places 16-bit payload in the correct BMB lane") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)
      val pixels = Seq(
        PixelWrite(x = 100, y = 50, data16 = 0xbeef), // low lane
        PixelWrite(x = 101, y = 50, data16 = 0xcafe) // high lane
      )

      for (pixel <- pixels) {
        sendPixel(dut, pixel)
        val planeAddr = calculateExpectedAddress(pixel.fbBaseAddr, pixel.x, pixel.y)
        val actualData = dut.o.fbWrite.cmd.data.toLong
        val expectedData = expectedDataWord(pixel.data16, planeAddr)
        assert(
          actualData == expectedData,
          f"Data mismatch at (${pixel.x},${pixel.y}): expected 0x$expectedData%08X, got 0x$actualData%08X"
        )
        assert(
          dut.o.fbWrite.cmd.mask.toInt == expectedMask(planeAddr),
          f"Mask mismatch at (${pixel.x},${pixel.y})"
        )

        clearPixel(dut)
        dut.clockDomain.waitSampling(3)
      }
    }
  }

  test("Write handles backpressure from BMB") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)
      val pixel = PixelWrite(50, 50, data16 = 0x8001, fbBaseAddr = 0x300000)
      dut.o.fbWrite.cmd.ready #= false
      sendPixel(dut, pixel)

      assert(dut.o.fbWrite.cmd.valid.toBoolean, "cmd should be valid")
      assert(!dut.i.fromPipeline.ready.toBoolean, "input should be backpressured")

      dut.o.fbWrite.cmd.ready #= true
      dut.clockDomain.waitSampling()
      assert(dut.i.fromPipeline.ready.toBoolean, "input should recover when cmd accepted")

      clearPixel(dut)
      dut.clockDomain.waitSampling(3)
    }
  }

  test("Write with BmbMemoryAgent integration") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)

      val fbBaseAddr = 0x2000000L
      val memorySize = 4 * 1024 * 1024

      val memory = new BmbMemoryAgent(memorySize)
      memory.addPort(
        bus = dut.o.fbWrite,
        busAddress = -fbBaseAddr,
        clockDomain = dut.clockDomain,
        withDriver = true,
        withStall = false
      )

      val testPixels = Seq(
        PixelWrite(10, 20, 0x1234, fbBaseAddr),
        PixelWrite(11, 20, 0x5678, fbBaseAddr),
        PixelWrite(12, 20, 0x9abc, fbBaseAddr)
      )

      for (pixel <- testPixels) {
        sendPixel(dut, pixel)
        waitForReady(dut)
        clearPixel(dut)
        dut.clockDomain.waitSampling(10)

        val memAddr = (pixel.y * defaultStride + pixel.x) * 2
        val actualData = readMemoryHalf(memory, memAddr)

        assert(
          actualData == pixel.data16,
          f"Memory data mismatch at (${pixel.x},${pixel.y}): expected 0x${pixel.data16}%04X, got 0x$actualData%04X"
        )
      }

      dut.clockDomain.waitSampling(10)
    }
  }
}
