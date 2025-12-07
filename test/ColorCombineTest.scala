package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import org.scalatest.funsuite.AnyFunSuite

class ColorCombineTest extends AnyFunSuite {

  val config = Config.voodoo1()

  // Helper to setup DUT for a test
  def setupDut(dut: ColorCombine): Unit = {
    dut.clockDomain.forkStimulus(period = 10)
    dut.io.input.valid #= false
    dut.io.output.ready #= true
    dut.clockDomain.waitSampling()
  }

  // Helper to set default input values
  def setDefaultInput(dut: ColorCombine): Unit = {
    // Coordinates
    dut.io.input.payload.coords(0) #= 100
    dut.io.input.payload.coords(1) #= 200

    // Iterated colors (Gouraud shading values)
    dut.io.input.payload.iterated.r #= 128
    dut.io.input.payload.iterated.g #= 64
    dut.io.input.payload.iterated.b #= 32
    dut.io.input.payload.iteratedAlpha #= 255
    dut.io.input.payload.iteratedZ #= 100

    // Texture (stubbed as zero)
    dut.io.input.payload.texture.r #= 0
    dut.io.input.payload.texture.g #= 0
    dut.io.input.payload.texture.b #= 0
    dut.io.input.payload.textureAlpha #= 0

    // Constant colors
    dut.io.input.payload.color0.r #= 200
    dut.io.input.payload.color0.g #= 150
    dut.io.input.payload.color0.b #= 100
    dut.io.input.payload.color1.r #= 50
    dut.io.input.payload.color1.g #= 75
    dut.io.input.payload.color1.b #= 100

    // Depth
    dut.io.input.payload.depth #= 0.5

    // Default config: simple passthrough of iterated color
    setPassthroughConfig(dut)
  }

  // Helper to set passthrough config (iterated color, no modification)
  def setPassthroughConfig(dut: ColorCombine): Unit = {
    dut.io.input.payload.config.rgbSel #= ColorCombine.RgbSel.ITERATED
    dut.io.input.payload.config.alphaSel #= ColorCombine.AlphaSel.ITERATED
    dut.io.input.payload.config.localSelect #= ColorCombine.LocalSel.ITERATED
    dut.io.input.payload.config.alphaLocalSelect #= ColorCombine.AlphaLocalSel.ITERATED
    dut.io.input.payload.config.localSelectOverride #= false
    dut.io.input.payload.config.zeroOther #= false
    dut.io.input.payload.config.subClocal #= false
    dut.io.input.payload.config.mselect #= ColorCombine.MSelect.ZERO
    dut.io.input.payload.config.reverseBlend #= false
    dut.io.input.payload.config.add #= ColorCombine.AddMode.NONE
    dut.io.input.payload.config.invertOutput #= false
    dut.io.input.payload.config.alphaZeroOther #= false
    dut.io.input.payload.config.alphaSubClocal #= false
    dut.io.input.payload.config.alphaMselect #= ColorCombine.MSelect.ZERO
    dut.io.input.payload.config.alphaReverseBlend #= false
    dut.io.input.payload.config.alphaAdd #= ColorCombine.AddMode.NONE
    dut.io.input.payload.config.alphaInvertOutput #= false
    dut.io.input.payload.config.textureEnable #= false
  }

  // Helper to send input and get output
  // The pipeline has 9 nodes (n0-n8) connected by 8 StageLinks
  // Each StageLink adds one cycle of latency
  val maxPipelineLatency = 20

  def sendAndReceive(dut: ColorCombine): (Int, Int, Int, Int) = {
    dut.io.input.valid #= true
    dut.io.output.ready #= true

    // Wait for data to propagate through the pipeline
    var cycles = 0
    while (!dut.io.output.valid.toBoolean && cycles < maxPipelineLatency) {
      dut.clockDomain.waitSampling()
      cycles += 1
    }

    // Now output should be valid
    assert(
      dut.io.output.valid.toBoolean,
      s"Expected output to be valid after pipeline latency (waited $cycles cycles)"
    )

    // Read output
    val r = dut.io.output.payload.color.r.toInt
    val g = dut.io.output.payload.color.g.toInt
    val b = dut.io.output.payload.color.b.toInt
    val a = dut.io.output.payload.alpha.toInt

    // Deassert valid
    dut.io.input.valid #= false
    dut.clockDomain.waitSampling()

    (r, g, b, a)
  }

  test("Gouraud shading passthrough") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // Config: rgb_sel=ITERATED, mselect=ZERO, add=NONE
      // This should pass through iterated color as-is
      // With mselect=ZERO, multiply by 0, so result should be 0
      // But we want passthrough, so actually we need to use the add path

      // For true passthrough: zero_other=true, sub=false, mselect=zero, add=clocal
      dut.io.input.payload.config.zeroOther #= true
      dut.io.input.payload.config.add #= ColorCombine.AddMode.CLOCAL
      dut.io.input.payload.config.localSelect #= ColorCombine.LocalSel.ITERATED

      dut.io.input.payload.config.alphaZeroOther #= true
      dut.io.input.payload.config.alphaAdd #= ColorCombine.AddMode.CLOCAL

      val (r, g, b, a) = sendAndReceive(dut)

      println(s"Gouraud passthrough: R=$r, G=$g, B=$b, A=$a")

      assert(r == 128, s"Expected R=128, got $r")
      assert(g == 64, s"Expected G=64, got $g")
      assert(b == 32, s"Expected B=32, got $b")
      assert(a == 255, s"Expected A=255, got $a")
    }
  }

  test("Color1 selection") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // Use color1 as c_other with passthrough
      dut.io.input.payload.config.rgbSel #= ColorCombine.RgbSel.COLOR1
      dut.io.input.payload.config.zeroOther #= false
      dut.io.input.payload.config.subClocal #= false
      dut.io.input.payload.config.mselect #= ColorCombine.MSelect.ZERO
      dut.io.input.payload.config.add #= ColorCombine.AddMode.NONE

      // With mselect=ZERO, result = 0 * c_other = 0, then add nothing = 0
      // So this test won't work as expected - need different approach

      // Better: zero_other=true, add=NONE should give 0
      // To get color1 out: rgb_sel=COLOR1, then multiply by 1 (via reverse blend)
      // Actually simplest: rgb_sel=COLOR1, zero_other=false, mselect=CLOCAL, reverse=true
      // This gives: color1 * (255 - clocal) >> 8
      // That's complex. Let's use: rgb_sel=COLOR1, zero=true, add nothing, then result is 0

      // Actually the issue is mselect=ZERO always gives 0.
      // To pass color1 through, need: c_other = color1, then result = c_other * factor
      // If factor = 255 (via ALOCAL with alpha=255), result ≈ c_other

      dut.io.input.payload.config.rgbSel #= ColorCombine.RgbSel.COLOR1
      dut.io.input.payload.config.zeroOther #= false
      dut.io.input.payload.config.mselect #= ColorCombine.MSelect.ALOCAL
      dut.io.input.payload.config.localSelect #= ColorCombine.LocalSel.ITERATED
      // aLocal = iteratedAlpha = 255, so factor = 255
      // result = color1 * 255 >> 8 ≈ color1 * 0.996 ≈ color1

      val (r, g, b, a) = sendAndReceive(dut)

      println(s"Color1 selection: R=$r, G=$g, B=$b, A=$a")

      // color1 = (50, 75, 100), multiplied by 255/256 ≈ same
      assert(r >= 49 && r <= 50, s"Expected R≈50, got $r")
      assert(g >= 74 && g <= 75, s"Expected G≈75, got $g")
      assert(b >= 99 && b <= 100, s"Expected B≈100, got $b")
    }
  }

  test("Invert output") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // Passthrough iterated, then invert
      dut.io.input.payload.config.zeroOther #= true
      dut.io.input.payload.config.add #= ColorCombine.AddMode.CLOCAL
      dut.io.input.payload.config.localSelect #= ColorCombine.LocalSel.ITERATED
      dut.io.input.payload.config.invertOutput #= true

      dut.io.input.payload.config.alphaZeroOther #= true
      dut.io.input.payload.config.alphaAdd #= ColorCombine.AddMode.CLOCAL
      dut.io.input.payload.config.alphaInvertOutput #= true

      val (r, g, b, a) = sendAndReceive(dut)

      println(s"Invert output: R=$r, G=$g, B=$b, A=$a")

      // Inverted: 255 - value
      // iterated = (128, 64, 32, 255) → inverted = (127, 191, 223, 0)
      assert(r == 127, s"Expected R=127, got $r")
      assert(g == 191, s"Expected G=191, got $g")
      assert(b == 223, s"Expected B=223, got $b")
      assert(a == 0, s"Expected A=0, got $a")
    }
  }

  test("Multiply by local color") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // c_other = iterated, c_local = color0
      // result = c_other * c_local >> 8
      dut.io.input.payload.config.rgbSel #= ColorCombine.RgbSel.ITERATED
      dut.io.input.payload.config.localSelect #= ColorCombine.LocalSel.COLOR0
      dut.io.input.payload.config.zeroOther #= false
      dut.io.input.payload.config.mselect #= ColorCombine.MSelect.CLOCAL
      dut.io.input.payload.config.add #= ColorCombine.AddMode.NONE

      val (r, g, b, a) = sendAndReceive(dut)

      println(s"Multiply by local: R=$r, G=$g, B=$b, A=$a")

      // iterated = (128, 64, 32), color0 = (200, 150, 100)
      // result = (128 * 200, 64 * 150, 32 * 100) >> 8
      //        = (25600, 9600, 3200) >> 8
      //        = (100, 37, 12)
      assert(r == 100, s"Expected R=100, got $r")
      assert(g == 37, s"Expected G=37, got $g")
      assert(b == 12, s"Expected B=12, got $b")
    }
  }

  test("Subtract and add local") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // Set up: c_other = iterated, c_local = color0
      // With sub_clocal: c_other - c_local
      // With mselect=ZERO, multiply gives 0
      // With add=CLOCAL, add back c_local
      // So result should be c_local (color0)
      dut.io.input.payload.config.rgbSel #= ColorCombine.RgbSel.ITERATED
      dut.io.input.payload.config.localSelect #= ColorCombine.LocalSel.COLOR0
      dut.io.input.payload.config.subClocal #= true
      dut.io.input.payload.config.mselect #= ColorCombine.MSelect.ZERO
      dut.io.input.payload.config.add #= ColorCombine.AddMode.CLOCAL

      val (r, g, b, a) = sendAndReceive(dut)

      println(s"Subtract and add local: R=$r, G=$g, B=$b, A=$a")

      // color0 = (200, 150, 100)
      assert(r == 200, s"Expected R=200, got $r")
      assert(g == 150, s"Expected G=150, got $g")
      assert(b == 100, s"Expected B=100, got $b")
    }
  }

  test("Clamping negative values") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // Set iterated to small values, color0 to larger
      dut.io.input.payload.iterated.r #= 50
      dut.io.input.payload.iterated.g #= 50
      dut.io.input.payload.iterated.b #= 50
      dut.io.input.payload.color0.r #= 200
      dut.io.input.payload.color0.g #= 200
      dut.io.input.payload.color0.b #= 200

      // Subtract: iterated - color0 = negative
      // With mselect=ZERO and add=NONE, result should be 0 (clamped)
      dut.io.input.payload.config.rgbSel #= ColorCombine.RgbSel.ITERATED
      dut.io.input.payload.config.localSelect #= ColorCombine.LocalSel.COLOR0
      dut.io.input.payload.config.subClocal #= true
      dut.io.input.payload.config.mselect #= ColorCombine.MSelect.ZERO
      dut.io.input.payload.config.add #= ColorCombine.AddMode.NONE

      val (r, g, b, a) = sendAndReceive(dut)

      println(s"Clamping negative: R=$r, G=$g, B=$b, A=$a")

      // (50 - 200) * 0 + 0 = 0, clamped to 0
      assert(r == 0, s"Expected R=0, got $r")
      assert(g == 0, s"Expected G=0, got $g")
      assert(b == 0, s"Expected B=0, got $b")
    }
  }

  test("Clamping overflow values") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // Set both iterated and color0 to high values
      dut.io.input.payload.iterated.r #= 200
      dut.io.input.payload.iterated.g #= 200
      dut.io.input.payload.iterated.b #= 200
      dut.io.input.payload.color0.r #= 200
      dut.io.input.payload.color0.g #= 200
      dut.io.input.payload.color0.b #= 200

      // zero_other + add clocal + add clocal again via...
      // Actually: set c_other=iterated, multiply by 1 (ALOCAL with alpha=255), then add clocal
      dut.io.input.payload.config.rgbSel #= ColorCombine.RgbSel.ITERATED
      dut.io.input.payload.config.localSelect #= ColorCombine.LocalSel.COLOR0
      dut.io.input.payload.config.mselect #= ColorCombine.MSelect.ALOCAL
      dut.io.input.payload.config.add #= ColorCombine.AddMode.CLOCAL
      // alocal = 255, so: 200 * 255 >> 8 + 200 = 199 + 200 = 399, clamped to 255

      val (r, g, b, a) = sendAndReceive(dut)

      println(s"Clamping overflow: R=$r, G=$g, B=$b, A=$a")

      assert(r == 255, s"Expected R=255, got $r")
      assert(g == 255, s"Expected G=255, got $g")
      assert(b == 255, s"Expected B=255, got $b")
    }
  }

  test("Reverse blend") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      // c_other = iterated = (128, 64, 32)
      // factor = ALOCAL = 255, reversed = 0
      // result = 128 * 0 >> 8 = 0
      dut.io.input.payload.config.rgbSel #= ColorCombine.RgbSel.ITERATED
      dut.io.input.payload.config.mselect #= ColorCombine.MSelect.ALOCAL
      dut.io.input.payload.config.reverseBlend #= true
      dut.io.input.payload.config.add #= ColorCombine.AddMode.NONE

      val (r, g, b, a) = sendAndReceive(dut)

      println(s"Reverse blend: R=$r, G=$g, B=$b, A=$a")

      // 255 reversed = 0, so multiply by 0 = 0
      assert(r == 0, s"Expected R=0, got $r")
      assert(g == 0, s"Expected G=0, got $g")
      assert(b == 0, s"Expected B=0, got $b")
    }
  }

  test("Coordinates pass through unchanged") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      setupDut(dut)
      setDefaultInput(dut)

      dut.io.input.payload.coords(0) #= 123
      dut.io.input.payload.coords(1) #= 456

      dut.io.input.valid #= true
      dut.clockDomain.waitSampling()
      dut.io.input.valid #= false

      var cycles = 0
      while (!dut.io.output.valid.toBoolean && cycles < 20) {
        dut.clockDomain.waitSampling()
        cycles += 1
      }

      val x = dut.io.output.payload.coords(0).toInt
      val y = dut.io.output.payload.coords(1).toInt

      println(s"Coordinates: x=$x, y=$y")

      assert(x == 123, s"Expected x=123, got $x")
      assert(y == 456, s"Expected y=456, got $y")
    }
  }

  test("Stream backpressure") {
    SimConfig.withIVerilog.withWave.compile(ColorCombine(config)).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.input.valid #= false
      dut.io.output.ready #= false
      dut.clockDomain.waitSampling()

      setDefaultInput(dut)
      dut.io.input.payload.config.zeroOther #= true
      dut.io.input.payload.config.add #= ColorCombine.AddMode.CLOCAL

      // Send input
      dut.io.input.valid #= true
      dut.clockDomain.waitSampling()

      // Keep ready=false, input should stay valid
      for (_ <- 0 until 5) {
        dut.clockDomain.waitSampling()
        // Output may or may not be valid, but shouldn't lose data
      }

      // Now accept the output
      dut.io.output.ready #= true
      var foundValid = false
      for (_ <- 0 until 10) {
        dut.clockDomain.waitSampling()
        if (dut.io.output.valid.toBoolean) {
          foundValid = true
        }
      }

      assert(foundValid, "Should eventually produce valid output after backpressure released")
    }
  }
}
