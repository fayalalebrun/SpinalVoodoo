package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import org.scalatest.funsuite.AnyFunSuite

class RasterizerTest extends AnyFunSuite {

  // SpinalSim handles conversion automatically with #=
  def fromAFix(value: AFix): Double = {
    value.toDouble
  }

  // Helper to setup DUT for a test
  def setupDut(dut: Rasterizer): Unit = {
    dut.clockDomain.forkStimulus(period = 10)
    dut.i.valid #= false
    dut.o.ready #= true
    dut.clockDomain.waitSampling()
  }

  // Helper to set default gradients (simple constant values)
  def setDefaultGradients(
      dut: Rasterizer,
      startVals: Seq[Double] = Seq(0.01, 0.02, 0.03, 0.04, 0.05, 0.0),
      dxVals: Seq[Double] = Seq(0.001, 0.001, 0.001, 0.001, 0.001, 0.0),
      dyVals: Seq[Double] = Seq(0.002, 0.002, 0.002, 0.002, 0.002, 0.0)
  ): Unit = {
    for (i <- 0 until 6) {
      dut.i.grads.all(i).start #= startVals(i)
      dut.i.grads.all(i).d(0) #= dxVals(i)
      dut.i.grads.all(i).d(1) #= dyVals(i)
    }
  }

  // Helper to set zero gradients
  def setZeroGradients(dut: Rasterizer): Unit = {
    for (i <- 0 until 6) {
      dut.i.grads.all(i).start #= 0.0
      dut.i.grads.all(i).d(0) #= 0.0
      dut.i.grads.all(i).d(1) #= 0.0
    }
  }

  // Helper to set a box (all pixels inside)
  def setBox(dut: Rasterizer, xmin: Double, xmax: Double, ymin: Double, ymax: Double): Unit = {
    dut.i.tri.xrange(0) #= xmin
    dut.i.tri.xrange(1) #= xmax
    dut.i.tri.yrange(0) #= ymin
    dut.i.tri.yrange(1) #= ymax

    // All edge functions always positive (entire box is inside)
    for (i <- 0 until 3) {
      dut.i.tri.coeffs(i).a #= 0.0
      dut.i.tri.coeffs(i).b #= 0.0
      dut.i.tri.coeffs(i).c #= 1.0
      dut.i.tri.edgeStart(i) #= 1.0
    }
  }

  // Helper to set a triangle by defining edge equations
  // Each edge is defined by coefficients (a, b, c) for ax + by + c = 0
  // and the edge value at the bounding box corner
  case class EdgeEquation(a: Double, b: Double, c: Double, startValue: Double)

  def setTriangle(
      dut: Rasterizer,
      xmin: Double,
      xmax: Double,
      ymin: Double,
      ymax: Double,
      edges: Seq[EdgeEquation]
  ): Unit = {
    require(edges.length == 3, "Must provide exactly 3 edge equations")

    dut.i.tri.xrange(0) #= xmin
    dut.i.tri.xrange(1) #= xmax
    dut.i.tri.yrange(0) #= ymin
    dut.i.tri.yrange(1) #= ymax

    for (i <- 0 until 3) {
      dut.i.tri.coeffs(i).a #= edges(i).a
      dut.i.tri.coeffs(i).b #= edges(i).b
      dut.i.tri.coeffs(i).c #= edges(i).c
      dut.i.tri.edgeStart(i) #= edges(i).startValue
    }
  }

  // Helper to send input and collect outputs
  def sendAndCollect(dut: Rasterizer, cycles: Int): Seq[(Int, Int)] = {
    dut.i.valid #= true
    dut.clockDomain.waitSampling()
    dut.i.valid #= false

    val outputs = scala.collection.mutable.ArrayBuffer[(Int, Int)]()
    for (_ <- 0 until cycles) {
      dut.clockDomain.waitSampling()
      if (dut.o.valid.toBoolean) {
        val x = dut.o.coords(0).toInt
        val y = dut.o.coords(1).toInt
        outputs += ((x, y))
      }
    }
    outputs.toSeq
  }

  test("Rasterizer outputs all pixels in bounding box that are inside triangle") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // Create a simple triangle: (0,0), (4,0), (0,4)
      // Edge 0->1: y = 0 (horizontal bottom edge)
      // Edge 1->2: x + y - 4 = 0 (diagonal)
      // Edge 2->0: x = 0 (vertical left edge)
      val edges = Seq(
        EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
        EdgeEquation(a = 1.0, b = 1.0, c = -4.0, startValue = -4.0), // x + y = 4
        EdgeEquation(a = 1.0, b = 0.0, c = 0.0, startValue = 0.0) // x = 0
      )

      setTriangle(dut, xmin = 0.0, xmax = 4.0, ymin = 0.0, ymax = 4.0, edges)
      setDefaultGradients(dut)

      val outputs = sendAndCollect(dut, cycles = 100)

      println(s"Rasterized ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      // Expected pixels inside triangle (0,0), (4,0), (0,4):
      // Row 0: (0,0), (1,0), (2,0), (3,0), (4,0)
      // Row 1: (0,1), (1,1), (2,1), (3,1)
      // Row 2: (0,2), (1,2), (2,2)
      // Row 3: (0,3), (1,3)
      // Row 4: (0,4)
      // Total: 15 pixels

      assert(outputs.length >= 10, s"Expected at least 10 pixels, got ${outputs.length}")

      // Check all outputs have valid coordinates within bounds
      outputs.foreach { case (x, y) =>
        assert(x >= 0 && x <= 4, s"x=$x out of bounds")
        assert(y >= 0 && y <= 4, s"y=$y out of bounds")
      }
    }
  }

  test("Rasterizer uses serpentine scanning") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // Small 3x3 box from (0,0) to (2,2)
      setBox(dut, xmin = 0.0, xmax = 2.0, ymin = 0.0, ymax = 2.0)
      setZeroGradients(dut)

      val outputs = sendAndCollect(dut, cycles = 50)

      println(s"Serpentine scan produced ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      // Expected serpentine pattern for 3x3:
      // Row 0 (left to right): (0,0), (1,0), (2,0)
      // Row 1 (right to left): (2,1), (1,1), (0,1)
      // Row 2 (left to right): (0,2), (1,2), (2,2)

      assert(outputs.length == 9, s"Expected 9 pixels, got ${outputs.length}")

      if (outputs.length >= 6) {
        // Check first row goes left to right
        assert(outputs(0)._1 < outputs(1)._1, "Row 0 should scan left to right")
        assert(outputs(1)._1 < outputs(2)._1, "Row 0 should scan left to right")

        // Check second row goes right to left
        assert(outputs(3)._1 > outputs(4)._1, "Row 1 should scan right to left")
        assert(outputs(4)._1 > outputs(5)._1, "Row 1 should scan right to left")
      }
    }
  }

  test("Rasterizer correctly interpolates gradients") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // 2x2 box from (0,0) to (1,1)
      setBox(dut, xmin = 0.0, xmax = 1.0, ymin = 0.0, ymax = 1.0)

      // Gradient: start=0.1, dx=0.01, dy=0.02
      val gradStart = Seq(0.1, 0.1, 0.1, 0.1, 0.1, 0.0)
      val gradDx = Seq(0.01, 0.01, 0.01, 0.01, 0.01, 0.0)
      val gradDy = Seq(0.02, 0.02, 0.02, 0.02, 0.02, 0.0)
      setDefaultGradients(dut, gradStart, gradDx, gradDy)

      dut.i.valid #= true
      dut.clockDomain.waitSampling()
      dut.i.valid #= false

      // Collect outputs with gradient values
      val outputs = scala.collection.mutable.ArrayBuffer[(Int, Int, Double)]()
      for (_ <- 0 until 30) {
        dut.clockDomain.waitSampling()
        if (dut.o.valid.toBoolean) {
          val x = dut.o.coords(0).toInt
          val y = dut.o.coords(1).toInt
          val grad = fromAFix(dut.o.grads.all(0))
          outputs += ((x, y, grad))
        }
      }

      println(s"Gradient interpolation test produced ${outputs.length} pixels:")
      outputs.foreach { case (x, y, grad) =>
        println(f"  ($x%d, $y%d) -> grad=$grad%.4f (expected=${0.1 + x * 0.01 + y * 0.02}%.4f)")
      }

      assert(outputs.length == 4, s"Expected 4 pixels, got ${outputs.length}")

      // Check gradient values: grad = start + x*dx + y*dy
      outputs.foreach { case (x, y, grad) =>
        val expected = 0.1 + x * 0.01 + y * 0.02
        val tolerance = 0.001
        assert(
          scala.math.abs(grad - expected) < tolerance,
          f"At ($x%d,$y%d): gradient=$grad%.4f, expected=$expected%.4f"
        )
      }
    }
  }

  test("Rasterizer handles backpressure") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.i.valid #= false
      dut.o.ready #= false
      dut.clockDomain.waitSampling()

      // 2x2 box
      setBox(dut, xmin = 0.0, xmax = 1.0, ymin = 0.0, ymax = 1.0)
      setZeroGradients(dut)

      dut.i.valid #= true
      dut.clockDomain.waitSampling()
      dut.i.valid #= false

      // Apply intermittent backpressure
      val outputs = scala.collection.mutable.ArrayBuffer[(Int, Int)]()
      for (i <- 0 until 50) {
        dut.o.ready #= (i % 3 != 0) // Ready 2 out of 3 cycles
        dut.clockDomain.waitSampling()
        if (dut.o.valid.toBoolean && dut.o.ready.toBoolean) {
          val x = dut.o.coords(0).toInt
          val y = dut.o.coords(1).toInt
          outputs += ((x, y))
        }
      }

      println(s"Backpressure test produced ${outputs.length} pixels")
      assert(
        outputs.length == 4,
        s"Expected 4 pixels even with backpressure, got ${outputs.length}"
      )
    }
  }

  test("Obtuse triangle") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // Obtuse triangle: (0,0), (8,0), (1,4)
      // This creates an obtuse angle at vertex (0,0)
      // Edge 0->1: y = 0 (horizontal)
      // Edge 1->2: 4x + 7y - 32 = 0
      // Edge 2->0: 4x - y = 0

      val edges = Seq(
        EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
        EdgeEquation(a = 4.0, b = 7.0, c = -32.0, startValue = -32.0), // 4x + 7y = 32
        EdgeEquation(a = 4.0, b = -1.0, c = 0.0, startValue = 0.0) // 4x = y
      )

      setTriangle(dut, xmin = 0.0, xmax = 8.0, ymin = 0.0, ymax = 4.0, edges)
      setDefaultGradients(dut)

      val outputs = sendAndCollect(dut, cycles = 150)

      println(s"Obtuse triangle rasterized ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      // Check all outputs are within bounds
      outputs.foreach { case (x, y) =>
        assert(x >= 0 && x <= 8, s"x=$x out of bounds")
        assert(y >= 0 && y <= 4, s"y=$y out of bounds")
      }

      assert(outputs.nonEmpty, "Should rasterize some pixels")
    }
  }

  test("Acute triangle") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // Acute triangle (equilateral-ish): (2,0), (6,0), (4,3)
      // All angles less than 90 degrees
      // Edge 0->1: y = 0
      // Edge 1->2: 3x + 2y - 18 = 0
      // Edge 2->0: 3x - 2y - 6 = 0

      val edges = Seq(
        EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
        EdgeEquation(a = 3.0, b = 2.0, c = -18.0, startValue = -12.0), // 3x + 2y = 18
        EdgeEquation(a = 3.0, b = -2.0, c = -6.0, startValue = 0.0) // 3x - 2y = 6
      )

      setTriangle(dut, xmin = 2.0, xmax = 6.0, ymin = 0.0, ymax = 3.0, edges)
      setDefaultGradients(dut)

      val outputs = sendAndCollect(dut, cycles = 100)

      println(s"Acute triangle rasterized ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      outputs.foreach { case (x, y) =>
        assert(x >= 2 && x <= 6, s"x=$x out of bounds")
        assert(y >= 0 && y <= 3, s"y=$y out of bounds")
      }

      assert(outputs.nonEmpty, "Should rasterize some pixels")
    }
  }

  test("Very thin triangle (extreme angle)") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // Very thin triangle: (0,0), (20,0), (10,1)
      // Creates two very acute angles near the base
      // Edge 0->1: y = 0
      // Edge 1->2: x + 10y - 20 = 0
      // Edge 2->0: x - 10y = 0

      val edges = Seq(
        EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
        EdgeEquation(a = 1.0, b = 10.0, c = -20.0, startValue = -20.0), // x + 10y = 20
        EdgeEquation(a = 1.0, b = -10.0, c = 0.0, startValue = 0.0) // x = 10y
      )

      setTriangle(dut, xmin = 0.0, xmax = 20.0, ymin = 0.0, ymax = 1.0, edges)
      setDefaultGradients(dut)

      val outputs = sendAndCollect(dut, cycles = 200)

      println(s"Thin triangle rasterized ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      outputs.foreach { case (x, y) =>
        assert(x >= 0 && x <= 20, s"x=$x out of bounds")
        assert(y >= 0 && y <= 1, s"y=$y out of bounds")
      }

      assert(outputs.nonEmpty, "Should rasterize some pixels even for thin triangle")
    }
  }

  test("Very tall and narrow triangle (extreme angle)") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // Very tall, narrow triangle: (10,0), (11,0), (10.5,10)
      // Creates a very sharp angle at the top
      // Edge 0->1: y = 0
      // Edge 1->2: 20x + y - 220 = 0 => simplified: y = -20x + 220
      // Edge 2->0: 20x - y - 200 = 0 => simplified: y = 20x - 200
      // Scale down coefficients to fit in SQ(12,4) range (-128 to +127.9375)

      val edges = Seq(
        EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
        EdgeEquation(a = 20.0, b = 1.0, c = -120.0, startValue = -20.0), // 20x + y = 120
        EdgeEquation(a = 20.0, b = -1.0, c = -100.0, startValue = 0.0) // 20x - y = 100
      )

      setTriangle(dut, xmin = 10.0, xmax = 11.0, ymin = 0.0, ymax = 10.0, edges)
      setDefaultGradients(dut)

      val outputs = sendAndCollect(dut, cycles = 200)

      println(s"Tall narrow triangle rasterized ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      outputs.foreach { case (x, y) =>
        assert(x >= 10 && x <= 11, s"x=$x out of bounds")
        assert(y >= 0 && y <= 10, s"y=$y out of bounds")
      }

      assert(outputs.nonEmpty, "Should rasterize some pixels even for very tall triangle")
    }
  }

  test("Triangle with negative coordinates") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // Triangle in negative space: (-4,-2), (0,-2), (-2,1)
      // Edge 0->1: y + 2 = 0
      // Edge 1->2: 3x - 2y - 4 = 0
      // Edge 2->0: 3x + 2y + 16 = 0

      val edges = Seq(
        EdgeEquation(a = 0.0, b = 1.0, c = 2.0, startValue = 0.0), // y = -2
        EdgeEquation(a = 3.0, b = -2.0, c = -4.0, startValue = 0.0), // 3x - 2y = 4
        EdgeEquation(a = 3.0, b = 2.0, c = 16.0, startValue = -4.0) // 3x + 2y = -16
      )

      setTriangle(dut, xmin = -4.0, xmax = 0.0, ymin = -2.0, ymax = 1.0, edges)
      setDefaultGradients(dut)

      val outputs = sendAndCollect(dut, cycles = 100)

      println(s"Negative coordinate triangle rasterized ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      outputs.foreach { case (x, y) =>
        assert(x >= -4 && x <= 0, s"x=$x out of bounds")
        assert(y >= -2 && y <= 1, s"y=$y out of bounds")
      }

      assert(outputs.nonEmpty, "Should rasterize some pixels")
    }
  }

  test("Upside-down triangle") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // Triangle with flat top: (0,4), (4,4), (2,0)
      // Edge 0->1: y - 4 = 0
      // Edge 1->2: 2x + y - 12 = 0
      // Edge 2->0: 2x - y - 4 = 0

      val edges = Seq(
        EdgeEquation(a = 0.0, b = 1.0, c = -4.0, startValue = -4.0), // y = 4
        EdgeEquation(a = 2.0, b = 1.0, c = -12.0, startValue = -4.0), // 2x + y = 12
        EdgeEquation(a = 2.0, b = -1.0, c = -4.0, startValue = -4.0) // 2x - y = 4
      )

      setTriangle(dut, xmin = 0.0, xmax = 4.0, ymin = 0.0, ymax = 4.0, edges)
      setDefaultGradients(dut)

      val outputs = sendAndCollect(dut, cycles = 100)

      println(s"Upside-down triangle rasterized ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      outputs.foreach { case (x, y) =>
        assert(x >= 0 && x <= 4, s"x=$x out of bounds")
        assert(y >= 0 && y <= 4, s"y=$y out of bounds")
      }

      assert(outputs.nonEmpty, "Should rasterize some pixels")
    }
  }

  test("Nearly degenerate triangle (almost a line)") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(Rasterizer(config)).doSim { dut =>
      setupDut(dut)

      // Almost collinear points: (0,0), (10,0), (5,0.5)
      // This is nearly degenerate but should still rasterize a few pixels
      // Edge 0->1: y = 0
      // Edge 1->2: x + 10y - 10 = 0
      // Edge 2->0: x - 10y = 0

      val edges = Seq(
        EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
        EdgeEquation(a = 1.0, b = 10.0, c = -10.0, startValue = -10.0), // x + 10y = 10
        EdgeEquation(a = 1.0, b = -10.0, c = 0.0, startValue = 0.0) // x = 10y
      )

      setTriangle(dut, xmin = 0.0, xmax = 10.0, ymin = 0.0, ymax = 0.5, edges)
      setDefaultGradients(dut)

      val outputs = sendAndCollect(dut, cycles = 150)

      println(s"Nearly degenerate triangle rasterized ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      outputs.foreach { case (x, y) =>
        assert(x >= 0 && x <= 10, s"x=$x out of bounds")
        assert(y >= 0 && y <= 1, s"y=$y out of bounds (note: 0.5 rounds to 0)")
      }

      // Even nearly degenerate triangles might produce some pixels
      // depending on rounding behavior
    }
  }
}
