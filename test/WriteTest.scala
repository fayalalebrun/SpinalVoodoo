package voodoo

import spinal.core._
import spinal.core.sim._

import org.scalatest.funsuite.AnyFunSuite

class WriteTest extends AnyFunSuite {

  // Default stride: fbiInit1[7:4]=5 → 5*128 bytes/row → 640 pixels/row
  val defaultStride = 640

  def setupDut(dut: Write): Unit = {
    dut.clockDomain.forkStimulus(period = 10)
    dut.i.fromPipeline.valid #= false
    dut.i.fromPipeline.fbBaseAddr #= 0
    dut.i.fromPipeline.fbPixelStride #= defaultStride
    dut.o.fbWrite.ready #= true
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

  test("Write converts pixel coordinates to aligned command address") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)

      val fbBaseAddr = 0x1000000L
      val pixel = PixelWrite(x = 11, y = 5, data16 = 0xabcd, fbBaseAddr = fbBaseAddr)
      sendPixel(dut, pixel)

      assert(dut.o.fbWrite.valid.toBoolean, "write request should be valid")

      val planeAddr = calculateExpectedAddress(fbBaseAddr, pixel.x, pixel.y)
      val expectedAddr = alignToWord(planeAddr)
      val actualAddr = dut.o.fbWrite.address.toLong

      assert(
        actualAddr == expectedAddr,
        f"Address mismatch: expected 0x$expectedAddr%08X, got 0x$actualAddr%08X"
      )
      assert(
        dut.o.fbWrite.mask.toInt == expectedMask(planeAddr),
        f"Mask mismatch at lane=${if (laneHi(planeAddr)) "hi" else "lo"}"
      )

      clearPixel(dut)
      dut.clockDomain.waitSampling(5)
    }
  }

  test("Write handles Stream handshaking correctly") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Write(config)).doSim { dut =>
      setupDut(dut)

      assert(!dut.o.fbWrite.valid.toBoolean, "write request should be invalid when input invalid")

      sendPixel(dut, PixelWrite(0, 0, 0x1234))
      assert(dut.o.fbWrite.valid.toBoolean, "write request should be valid when input is valid")

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
        val actualData = dut.o.fbWrite.data.toLong
        val expectedData = expectedDataWord(pixel.data16, planeAddr)
        assert(
          actualData == expectedData,
          f"Data mismatch at (${pixel.x},${pixel.y}): expected 0x$expectedData%08X, got 0x$actualData%08X"
        )
        assert(
          dut.o.fbWrite.mask.toInt == expectedMask(planeAddr),
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
      dut.o.fbWrite.ready #= false
      sendPixel(dut, pixel)

      assert(dut.o.fbWrite.valid.toBoolean, "write request should be valid")
      assert(!dut.i.fromPipeline.ready.toBoolean, "input should be backpressured")

      dut.o.fbWrite.ready #= true
      dut.clockDomain.waitSampling()
      assert(dut.i.fromPipeline.ready.toBoolean, "input should recover when cmd accepted")

      clearPixel(dut)
      dut.clockDomain.waitSampling(3)
    }
  }

}
