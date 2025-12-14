package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import org.scalatest.funsuite.AnyFunSuite

class TmuTest extends AnyFunSuite {

  val config = Config.voodoo1()

  // Helper to setup DUT for a test
  def setupDut(dut: Tmu): Unit = {
    dut.clockDomain.forkStimulus(period = 10)
    dut.io.input.valid #= false
    dut.io.output.ready #= true
    dut.clockDomain.waitSampling()
  }

  // Helper to set default config (now part of input stream)
  def setDefaultConfig(dut: Tmu): Unit = {
    dut.io.input.payload.config.textureMode #= 0
    dut.io.input.payload.config.texBaseAddr #= 0
  }

  // Helper to set default gradient values (all zeros)
  def setDefaultGrads(dut: Tmu): Unit = {
    dut.io.input.payload.grads.redGrad #= 0.0
    dut.io.input.payload.grads.greenGrad #= 0.0
    dut.io.input.payload.grads.blueGrad #= 0.0
    dut.io.input.payload.grads.depthGrad #= 0.0
    dut.io.input.payload.grads.alphaGrad #= 0.0
    dut.io.input.payload.grads.wGrad #= 1.0
    dut.io.input.payload.grads.s0Grad #= 0.0
    dut.io.input.payload.grads.t0Grad #= 0.0
    dut.io.input.payload.grads.s1Grad #= 0.0
    dut.io.input.payload.grads.t1Grad #= 0.0
  }

  // Helper to set default input values
  def setDefaultInput(dut: Tmu): Unit = {
    // Coordinates
    dut.io.input.payload.coords(0) #= 100
    dut.io.input.payload.coords(1) #= 200

    // Texture coordinates (0,0)
    dut.io.input.payload.s #= 0.0
    dut.io.input.payload.t #= 0.0
    dut.io.input.payload.w #= 1.0

    // No upstream texture (for TMU0)
    dut.io.input.payload.cOther.r #= 0
    dut.io.input.payload.cOther.g #= 0
    dut.io.input.payload.cOther.b #= 0
    dut.io.input.payload.aOther #= 0

    // Set default gradients
    setDefaultGrads(dut)

    // Set default config
    setDefaultConfig(dut)
  }

  // Helper to simulate BMB memory response
  def respondToTexRead(dut: Tmu, texelData: Int): Unit = {
    // Wait for command request
    if (dut.io.texRead.cmd.valid.toBoolean) {
      dut.io.texRead.cmd.ready #= true
      dut.clockDomain.waitSampling()
      dut.io.texRead.cmd.ready #= false

      // Send response with texel data
      dut.io.texRead.rsp.valid #= true
      dut.io.texRead.rsp.fragment.data #= texelData
      dut.io.texRead.rsp.last #= true
      dut.clockDomain.waitSampling()
      dut.io.texRead.rsp.valid #= false
    }
  }

  // Pipeline has 5 nodes (n0-n4), so 4 stages of latency
  val maxPipelineLatency = 20

  test("TMU passes coordinates through") {
    SimConfig.withIVerilog.withWave.compile(Tmu(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // Set specific coordinates
      dut.io.input.payload.coords(0) #= 123
      dut.io.input.payload.coords(1) #= 456

      dut.io.input.valid #= true

      // Handle memory transactions and wait for output
      var cycles = 0
      while (!dut.io.output.valid.toBoolean && cycles < maxPipelineLatency) {
        // Respond to texture read requests
        if (dut.io.texRead.cmd.valid.toBoolean) {
          dut.io.texRead.cmd.ready #= true
          dut.io.texRead.rsp.valid #= true
          dut.io.texRead.rsp.fragment.data #= 0xffff // White texel (RGB565)
          dut.io.texRead.rsp.last #= true
        } else {
          dut.io.texRead.cmd.ready #= false
          dut.io.texRead.rsp.valid #= false
        }
        dut.clockDomain.waitSampling()
        cycles += 1
      }

      assert(dut.io.output.valid.toBoolean, s"Expected output to be valid (waited $cycles cycles)")
      assert(dut.io.output.payload.coords(0).toInt == 123, "X coordinate should pass through")
      assert(dut.io.output.payload.coords(1).toInt == 456, "Y coordinate should pass through")

      println(
        s"Coordinates: x=${dut.io.output.payload.coords(0).toInt}, y=${dut.io.output.payload.coords(1).toInt}"
      )
    }
  }

  test("TMU converts RGB565 white to RGB888") {
    SimConfig.withIVerilog.withWave.compile(Tmu(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      dut.io.input.valid #= true

      // Handle memory transactions
      var cycles = 0
      while (!dut.io.output.valid.toBoolean && cycles < maxPipelineLatency) {
        if (dut.io.texRead.cmd.valid.toBoolean) {
          dut.io.texRead.cmd.ready #= true
          dut.io.texRead.rsp.valid #= true
          // RGB565 white: RRRRR_GGGGGG_BBBBB = 11111_111111_11111 = 0xFFFF
          dut.io.texRead.rsp.fragment.data #= 0xffff
          dut.io.texRead.rsp.last #= true
        } else {
          dut.io.texRead.cmd.ready #= false
          dut.io.texRead.rsp.valid #= false
        }
        dut.clockDomain.waitSampling()
        cycles += 1
      }

      assert(dut.io.output.valid.toBoolean, s"Expected output to be valid (waited $cycles cycles)")

      val r = dut.io.output.payload.texture.r.toInt
      val g = dut.io.output.payload.texture.g.toInt
      val b = dut.io.output.payload.texture.b.toInt
      val a = dut.io.output.payload.textureAlpha.toInt

      println(s"RGB565 white (0xFFFF) -> RGB888: R=$r, G=$g, B=$b, A=$a")

      // RGB565 white: R=31, G=63, B=31
      // Expanded: R = 31 << 3 | 31 >> 2 = 248 | 7 = 255
      //           G = 63 << 2 | 63 >> 4 = 252 | 3 = 255
      //           B = 31 << 3 | 31 >> 2 = 248 | 7 = 255
      assert(r == 255, s"Expected R=255 for white, got $r")
      assert(g == 255, s"Expected G=255 for white, got $g")
      assert(b == 255, s"Expected B=255 for white, got $b")
      assert(a == 255, s"Expected A=255 (full opacity), got $a")
    }
  }

  test("TMU converts RGB565 black to RGB888") {
    SimConfig.withIVerilog.withWave.compile(Tmu(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      dut.io.input.valid #= true

      var cycles = 0
      while (!dut.io.output.valid.toBoolean && cycles < maxPipelineLatency) {
        if (dut.io.texRead.cmd.valid.toBoolean) {
          dut.io.texRead.cmd.ready #= true
          dut.io.texRead.rsp.valid #= true
          // RGB565 black: 0x0000
          dut.io.texRead.rsp.fragment.data #= 0x0000
          dut.io.texRead.rsp.last #= true
        } else {
          dut.io.texRead.cmd.ready #= false
          dut.io.texRead.rsp.valid #= false
        }
        dut.clockDomain.waitSampling()
        cycles += 1
      }

      assert(dut.io.output.valid.toBoolean, s"Expected output to be valid (waited $cycles cycles)")

      val r = dut.io.output.payload.texture.r.toInt
      val g = dut.io.output.payload.texture.g.toInt
      val b = dut.io.output.payload.texture.b.toInt

      println(s"RGB565 black (0x0000) -> RGB888: R=$r, G=$g, B=$b")

      assert(r == 0, s"Expected R=0 for black, got $r")
      assert(g == 0, s"Expected G=0 for black, got $g")
      assert(b == 0, s"Expected B=0 for black, got $b")
    }
  }

  test("TMU converts RGB565 red to RGB888") {
    SimConfig.withIVerilog.withWave.compile(Tmu(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      dut.io.input.valid #= true

      var cycles = 0
      while (!dut.io.output.valid.toBoolean && cycles < maxPipelineLatency) {
        if (dut.io.texRead.cmd.valid.toBoolean) {
          dut.io.texRead.cmd.ready #= true
          dut.io.texRead.rsp.valid #= true
          // RGB565 pure red: R=31, G=0, B=0 = 11111_000000_00000 = 0xF800
          dut.io.texRead.rsp.fragment.data #= 0xf800
          dut.io.texRead.rsp.last #= true
        } else {
          dut.io.texRead.cmd.ready #= false
          dut.io.texRead.rsp.valid #= false
        }
        dut.clockDomain.waitSampling()
        cycles += 1
      }

      assert(dut.io.output.valid.toBoolean, s"Expected output to be valid (waited $cycles cycles)")

      val r = dut.io.output.payload.texture.r.toInt
      val g = dut.io.output.payload.texture.g.toInt
      val b = dut.io.output.payload.texture.b.toInt

      println(s"RGB565 red (0xF800) -> RGB888: R=$r, G=$g, B=$b")

      // R=31: 31 << 3 | 31 >> 2 = 248 | 7 = 255
      assert(r == 255, s"Expected R=255 for red, got $r")
      assert(g == 0, s"Expected G=0 for red, got $g")
      assert(b == 0, s"Expected B=0 for red, got $b")
    }
  }

  test("TMU converts RGB565 green to RGB888") {
    SimConfig.withIVerilog.withWave.compile(Tmu(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      dut.io.input.valid #= true

      var cycles = 0
      while (!dut.io.output.valid.toBoolean && cycles < maxPipelineLatency) {
        if (dut.io.texRead.cmd.valid.toBoolean) {
          dut.io.texRead.cmd.ready #= true
          dut.io.texRead.rsp.valid #= true
          // RGB565 pure green: R=0, G=63, B=0 = 00000_111111_00000 = 0x07E0
          dut.io.texRead.rsp.fragment.data #= 0x07e0
          dut.io.texRead.rsp.last #= true
        } else {
          dut.io.texRead.cmd.ready #= false
          dut.io.texRead.rsp.valid #= false
        }
        dut.clockDomain.waitSampling()
        cycles += 1
      }

      assert(dut.io.output.valid.toBoolean, s"Expected output to be valid (waited $cycles cycles)")

      val r = dut.io.output.payload.texture.r.toInt
      val g = dut.io.output.payload.texture.g.toInt
      val b = dut.io.output.payload.texture.b.toInt

      println(s"RGB565 green (0x07E0) -> RGB888: R=$r, G=$g, B=$b")

      // G=63: 63 << 2 | 63 >> 4 = 252 | 3 = 255
      assert(r == 0, s"Expected R=0 for green, got $r")
      assert(g == 255, s"Expected G=255 for green, got $g")
      assert(b == 0, s"Expected B=0 for green, got $b")
    }
  }

  test("TMU converts RGB565 blue to RGB888") {
    SimConfig.withIVerilog.withWave.compile(Tmu(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      dut.io.input.valid #= true

      var cycles = 0
      while (!dut.io.output.valid.toBoolean && cycles < maxPipelineLatency) {
        if (dut.io.texRead.cmd.valid.toBoolean) {
          dut.io.texRead.cmd.ready #= true
          dut.io.texRead.rsp.valid #= true
          // RGB565 pure blue: R=0, G=0, B=31 = 00000_000000_11111 = 0x001F
          dut.io.texRead.rsp.fragment.data #= 0x001f
          dut.io.texRead.rsp.last #= true
        } else {
          dut.io.texRead.cmd.ready #= false
          dut.io.texRead.rsp.valid #= false
        }
        dut.clockDomain.waitSampling()
        cycles += 1
      }

      assert(dut.io.output.valid.toBoolean, s"Expected output to be valid (waited $cycles cycles)")

      val r = dut.io.output.payload.texture.r.toInt
      val g = dut.io.output.payload.texture.g.toInt
      val b = dut.io.output.payload.texture.b.toInt

      println(s"RGB565 blue (0x001F) -> RGB888: R=$r, G=$g, B=$b")

      // B=31: 31 << 3 | 31 >> 2 = 248 | 7 = 255
      assert(r == 0, s"Expected R=0 for blue, got $r")
      assert(g == 0, s"Expected G=0 for blue, got $g")
      assert(b == 255, s"Expected B=255 for blue, got $b")
    }
  }

  test("TMU computes correct texel address") {
    SimConfig.withIVerilog.withWave.compile(Tmu(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // Set texture base address (now part of input stream config)
      val baseAddr = 0x100000
      dut.io.input.payload.config.texBaseAddr #= baseAddr

      // Set S=10, T=20 (in integer coordinates)
      dut.io.input.payload.s #= 10.0
      dut.io.input.payload.t #= 20.0

      dut.io.input.valid #= true

      // Wait for command and check address
      var cycles = 0
      var addressCaptured = false
      var capturedAddress = 0L
      while (!addressCaptured && cycles < maxPipelineLatency) {
        if (dut.io.texRead.cmd.valid.toBoolean) {
          capturedAddress = dut.io.texRead.cmd.fragment.address.toLong
          addressCaptured = true
          dut.io.texRead.cmd.ready #= true
          dut.io.texRead.rsp.valid #= true
          dut.io.texRead.rsp.fragment.data #= 0xffff
          dut.io.texRead.rsp.last #= true
        } else {
          dut.io.texRead.cmd.ready #= false
          dut.io.texRead.rsp.valid #= false
        }
        dut.clockDomain.waitSampling()
        cycles += 1
      }

      assert(addressCaptured, "Expected texture read command")

      // Expected address: base + (y * stride + x) * 2
      // With stride=256, x=10, y=20: base + (20 * 256 + 10) * 2 = base + 10260
      val expectedAddress = baseAddr + (20 * 256 + 10) * 2
      println(
        s"Texture address: expected=0x${expectedAddress.toHexString}, actual=0x${capturedAddress.toHexString}"
      )

      assert(
        capturedAddress == expectedAddress,
        s"Expected address 0x${expectedAddress.toHexString}, got 0x${capturedAddress.toHexString}"
      )
    }
  }

  test("TMU gradients pass through") {
    SimConfig.withIVerilog.withWave.compile(Tmu(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // Set specific gradient values
      dut.io.input.payload.grads.redGrad #= 128.0
      dut.io.input.payload.grads.greenGrad #= 64.0
      dut.io.input.payload.grads.blueGrad #= 32.0

      dut.io.input.valid #= true

      var cycles = 0
      while (!dut.io.output.valid.toBoolean && cycles < maxPipelineLatency) {
        if (dut.io.texRead.cmd.valid.toBoolean) {
          dut.io.texRead.cmd.ready #= true
          dut.io.texRead.rsp.valid #= true
          dut.io.texRead.rsp.fragment.data #= 0xffff
          dut.io.texRead.rsp.last #= true
        } else {
          dut.io.texRead.cmd.ready #= false
          dut.io.texRead.rsp.valid #= false
        }
        dut.clockDomain.waitSampling()
        cycles += 1
      }

      assert(dut.io.output.valid.toBoolean, s"Expected output to be valid (waited $cycles cycles)")

      // Check gradients pass through (with some tolerance for fixed-point conversion)
      val outR = dut.io.output.payload.grads.redGrad.toDouble
      val outG = dut.io.output.payload.grads.greenGrad.toDouble
      val outB = dut.io.output.payload.grads.blueGrad.toDouble

      println(s"Gradients: R=$outR, G=$outG, B=$outB")

      val tolerance = 0.1
      assert(
        scala.math.abs(outR - 128.0) < tolerance,
        s"Red gradient should pass through (got $outR)"
      )
      assert(
        scala.math.abs(outG - 64.0) < tolerance,
        s"Green gradient should pass through (got $outG)"
      )
      assert(
        scala.math.abs(outB - 32.0) < tolerance,
        s"Blue gradient should pass through (got $outB)"
      )
    }
  }
}
