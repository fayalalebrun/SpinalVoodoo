package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim._
import org.scalatest.funsuite.AnyFunSuite

class WriteTest extends AnyFunSuite {

  // Default stride: fbiInit1[7:4]=5 → 5*128=640 pixels
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
      r: Int,
      g: Int,
      b: Int,
      depthAlpha: Int = 0xffff,
      fbBaseAddr: Long = 0
  )

  def sendPixel(dut: Write, pixel: PixelWrite): Unit = {
    dut.i.fromPipeline.valid #= true
    dut.i.fromPipeline.coords(0) #= pixel.x
    dut.i.fromPipeline.coords(1) #= pixel.y
    dut.i.fromPipeline.toFb.color.r #= pixel.r
    dut.i.fromPipeline.toFb.color.g #= pixel.g
    dut.i.fromPipeline.toFb.color.b #= pixel.b
    dut.i.fromPipeline.toFb.depthAlpha #= pixel.depthAlpha
    dut.i.fromPipeline.rgbWrite #= true
    dut.i.fromPipeline.auxWrite #= true
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

  def sendAndClearPixel(dut: Write, pixel: PixelWrite): Unit = {
    sendPixel(dut, pixel)
    clearPixel(dut)
  }

  def calculateExpectedAddress(
      fbBaseAddr: Long,
      x: Int,
      y: Int,
      stride: Int = defaultStride
  ): Long = {
    val pixelOffset = y * stride + x
    fbBaseAddr + pixelOffset * 4
  }

  def calculateExpectedData(r: Int, g: Int, b: Int, depthAlpha: Int): Long = {
    val rgb565 = (r << 11) | (g << 5) | b
    (depthAlpha.toLong << 16) | rgb565
  }

  def readMemoryWord(memory: BmbMemoryAgent, addr: Int): Long = {
    val byte0 = memory.getByte(addr + 0) & 0xff
    val byte1 = memory.getByte(addr + 1) & 0xff
    val byte2 = memory.getByte(addr + 2) & 0xff
    val byte3 = memory.getByte(addr + 3) & 0xff
    ((byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0) & 0xffffffffL
  }

  test("Write converts pixel coordinates to framebuffer address") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)

      val fbBaseAddr = 0x1000000L

      val pixel = PixelWrite(x = 10, y = 5, r = 0x1f, g = 0x3f, b = 0x1f, fbBaseAddr = fbBaseAddr)
      sendPixel(dut, pixel)

      assert(dut.o.fbWrite.cmd.valid.toBoolean, "BMB cmd should be valid")

      val expectedAddr = calculateExpectedAddress(fbBaseAddr, pixel.x, pixel.y)
      val actualAddr = dut.o.fbWrite.cmd.address.toInt

      assert(
        actualAddr == expectedAddr,
        f"Address mismatch: expected 0x$expectedAddr%08X, got 0x$actualAddr%08X"
      )
      assert(dut.o.fbWrite.cmd.opcode.toInt == 1, "Opcode should be WRITE")
      assert(dut.o.fbWrite.cmd.length.toInt == 3, "Length should be 3 (4 bytes)")
      assert(dut.o.fbWrite.cmd.last.toBoolean, "Last flag should be set")

      clearPixel(dut)
      dut.clockDomain.waitSampling(5)
    }
  }

  test("Write handles Stream handshaking correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)
      // cmd should not be valid when input is invalid
      assert(
        !dut.o.fbWrite.cmd.valid.toBoolean,
        "BMB cmd should not be valid when input is invalid"
      )

      // cmd should become valid when input is valid
      sendPixel(dut, PixelWrite(0, 0, 0, 0, 0, 0))
      assert(dut.o.fbWrite.cmd.valid.toBoolean, "BMB cmd should be valid when input is valid")

      clearPixel(dut)
      dut.clockDomain.waitSampling(3)
    }
  }

  test("Write encodes RGB565 and depth/alpha correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)
      val pixel = PixelWrite(x = 100, y = 50, r = 0x15, g = 0x2a, b = 0x0f, depthAlpha = 0xabcd)
      sendPixel(dut, pixel)

      val expectedData = calculateExpectedData(pixel.r, pixel.g, pixel.b, pixel.depthAlpha)
      val actualData = dut.o.fbWrite.cmd.data.toLong

      assert(
        actualData == expectedData,
        f"Data mismatch: expected 0x$expectedData%08X, got 0x$actualData%08X"
      )

      clearPixel(dut)
      dut.clockDomain.waitSampling(3)
    }
  }

  test("Write handles multiple pixels in sequence") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)
      val pixels = Seq(
        PixelWrite(0, 0, 0x1f, 0x00, 0x00),
        PixelWrite(1, 0, 0x00, 0x3f, 0x00),
        PixelWrite(2, 0, 0x00, 0x00, 0x1f),
        PixelWrite(0, 1, 0x1f, 0x3f, 0x1f)
      )

      var sentCount = 0
      for (pixel <- pixels) {
        sendPixel(dut, pixel)
        if (dut.o.fbWrite.cmd.valid.toBoolean) {
          sentCount += 1
        }
        clearPixel(dut)
      }

      assert(
        sentCount == pixels.length,
        s"Should have sent ${pixels.length} pixels, but sent $sentCount"
      )
    }
  }

  test("Write handles backpressure from BMB") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)
      val pixel = PixelWrite(50, 50, 0x10, 0x20, 0x10, 0x8000, fbBaseAddr = 0x300000)
      dut.o.fbWrite.cmd.ready #= false
      sendPixel(dut, pixel)

      // cmd should be valid but not transferred due to backpressure
      assert(dut.o.fbWrite.cmd.valid.toBoolean, "cmd should be valid")
      assert(
        !dut.i.fromPipeline.ready.toBoolean,
        "input should not be ready when BMB is backpressured"
      )

      // Release backpressure
      dut.o.fbWrite.cmd.ready #= true
      dut.clockDomain.waitSampling()

      assert(dut.i.fromPipeline.ready.toBoolean, "input should be ready when BMB accepts")

      clearPixel(dut)
      dut.clockDomain.waitSampling(3)
    }
  }

  test("Write calculates address for various pixel coordinates") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)

      val fbBaseAddr = 0x1000000L

      val testCoords = Seq((0, 0), (127, 0), (0, 127), (127, 127), (64, 64))

      for ((x, y) <- testCoords) {
        sendPixel(dut, PixelWrite(x, y, 0, 0, 0, 0, fbBaseAddr = fbBaseAddr))

        val expectedAddr = calculateExpectedAddress(fbBaseAddr, x, y)
        val actualAddr = dut.o.fbWrite.cmd.address.toInt

        assert(
          actualAddr == expectedAddr,
          f"Address for ($x,$y): expected 0x$expectedAddr%08X, got 0x$actualAddr%08X"
        )

        clearPixel(dut)
      }
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
        PixelWrite(10, 20, 0x1f, 0x00, 0x00, 0x1234, fbBaseAddr = fbBaseAddr),
        PixelWrite(11, 20, 0x00, 0x3f, 0x00, 0x5678, fbBaseAddr = fbBaseAddr),
        PixelWrite(12, 20, 0x00, 0x00, 0x1f, 0x9abc, fbBaseAddr = fbBaseAddr)
      )

      for (pixel <- testPixels) {
        sendPixel(dut, pixel)
        waitForReady(dut)
        clearPixel(dut)
        dut.clockDomain.waitSampling(10)

        val memAddr = (pixel.y * defaultStride + pixel.x) * 4
        val expectedData = calculateExpectedData(pixel.r, pixel.g, pixel.b, pixel.depthAlpha)
        val actualData = readMemoryWord(memory, memAddr)

        assert(
          actualData == expectedData,
          f"Memory data mismatch at (${pixel.x},${pixel.y}): expected 0x$expectedData%08X, got 0x$actualData%08X"
        )
      }

      dut.clockDomain.waitSampling(10)
    }
  }
}
