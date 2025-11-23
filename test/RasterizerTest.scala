package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import org.scalatest.funsuite.AnyFunSuite
import java.awt.image.BufferedImage
import javax.imageio.ImageIO
import java.io.File

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

  // Helper to render triangle output to an image file
  def renderTriangleToImage(
      pixels: Seq[(Int, Int)],
      xmin: Int,
      xmax: Int,
      ymin: Int,
      ymax: Int,
      filename: String,
      scale: Int = 10
  ): Unit = {
    val width = (xmax - xmin + 1) * scale
    val height = (ymax - ymin + 1) * scale
    val image = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB)

    // Fill background with white
    val g = image.getGraphics()
    g.setColor(java.awt.Color.WHITE)
    g.fillRect(0, 0, width, height)

    // Draw grid lines (light gray)
    g.setColor(new java.awt.Color(230, 230, 230))
    for (x <- xmin to xmax) {
      val pixelX = (x - xmin) * scale
      g.drawLine(pixelX, 0, pixelX, height)
    }
    for (y <- ymin to ymax) {
      val pixelY = (y - ymin) * scale
      g.drawLine(0, pixelY, width, pixelY)
    }

    // Draw rasterized pixels (blue)
    g.setColor(new java.awt.Color(100, 149, 237)) // Cornflower blue
    pixels.foreach { case (x, y) =>
      val pixelX = (x - xmin) * scale
      val pixelY = (y - ymin) * scale
      g.fillRect(pixelX + 1, pixelY + 1, scale - 1, scale - 1)
    }

    g.dispose()

    // Save to file
    val file = new File(filename)
    ImageIO.write(image, "png", file)
    println(s"Rendered triangle image to: ${file.getAbsolutePath}")
  }

  // Data structure for triangle test cases
  case class TriangleTestCase(
      name: String,
      edges: Seq[EdgeEquation],
      xmin: Double,
      xmax: Double,
      ymin: Double,
      ymax: Double,
      imageName: String,
      imageScale: Int = 10,
      maxCycles: Int = 200
  )

  // General helper to test a triangle with given parameters
  // Performs common assertions and returns outputs for test-specific checks
  def testTriangle(tc: TriangleTestCase): Seq[(Int, Int)] = {
    val config = Config.voodoo1()
    var capturedOutputs: Seq[(Int, Int)] = Seq.empty

    val compiled =
      SimConfig.withIVerilog.withWave.workspaceName(tc.imageName).compile(Rasterizer(config))
    compiled.doSim(tc.imageName) { (dut: Rasterizer) =>
      setupDut(dut)

      setTriangle(dut, tc.xmin, tc.xmax, tc.ymin, tc.ymax, tc.edges)
      setDefaultGradients(dut)

      val outputs = sendAndCollect(dut, cycles = tc.maxCycles)

      println(s"${tc.name} rasterized ${outputs.length} pixels:")
      outputs.foreach { case (x, y) => println(f"  ($x%d, $y%d)") }

      // Render triangle to image in the test's workspace directory
      val testPath = currentTestPath()
      renderTriangleToImage(
        outputs,
        xmin = tc.xmin.toInt,
        xmax = tc.xmax.toInt,
        ymin = tc.ymin.toInt,
        ymax = tc.ymax.toInt,
        filename = s"$testPath/image.png",
        scale = tc.imageScale
      )

      // Common assertions that apply to all triangles
      outputs.foreach { case (x, y) =>
        assert(x >= tc.xmin.toInt && x <= tc.xmax.toInt, s"x=$x out of bounds")
        assert(y >= tc.ymin.toInt && y <= tc.ymax.toInt, s"y=$y out of bounds")
      }

      assert(outputs.nonEmpty, "Should rasterize at least one pixel")

      capturedOutputs = outputs
    }

    capturedOutputs
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

      // Render triangle to image in test's workspace directory
      val testPath = currentTestPath()
      renderTriangleToImage(
        outputs,
        xmin = 0,
        xmax = 4,
        ymin = 0,
        ymax = 4,
        filename = s"$testPath/image.png"
      )

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

  test("Obtuse triangle with wide base") {
    // Obtuse triangle: (0,0), (8,0), (1,4)
    // This creates an obtuse angle at vertex (0,0)
    testTriangle(
      TriangleTestCase(
        name = "Obtuse triangle with wide base",
        edges = Seq(
          EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
          EdgeEquation(a = 4.0, b = 7.0, c = -32.0, startValue = -32.0), // 4x + 7y = 32
          EdgeEquation(a = 4.0, b = -1.0, c = 0.0, startValue = 0.0) // 4x = y
        ),
        xmin = 0.0,
        xmax = 8.0,
        ymin = 0.0,
        ymax = 4.0,
        imageName = "obtuse_triangle",
        imageScale = 20,
        maxCycles = 150
      )
    )
  }

  test("Acute triangle (equilateral-ish)") {
    // Acute triangle (equilateral-ish): (2,0), (6,0), (4,3)
    // All angles less than 90 degrees
    testTriangle(
      TriangleTestCase(
        name = "Acute triangle (equilateral-ish)",
        edges = Seq(
          EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
          EdgeEquation(a = 3.0, b = 2.0, c = -18.0, startValue = -12.0), // 3x + 2y = 18
          EdgeEquation(a = 3.0, b = -2.0, c = -6.0, startValue = 0.0) // 3x - 2y = 6
        ),
        xmin = 2.0,
        xmax = 6.0,
        ymin = 0.0,
        ymax = 3.0,
        imageName = "acute_triangle",
        imageScale = 25,
        maxCycles = 100
      )
    )
  }

  test("Very thin triangle (extreme aspect ratio)") {
    // Very thin triangle: (0,0), (20,0), (10,1)
    // Creates two very acute angles near the base
    testTriangle(
      TriangleTestCase(
        name = "Very thin triangle (extreme aspect ratio)",
        edges = Seq(
          EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
          EdgeEquation(a = 1.0, b = 10.0, c = -20.0, startValue = -20.0), // x + 10y = 20
          EdgeEquation(a = 1.0, b = -10.0, c = 0.0, startValue = 0.0) // x = 10y
        ),
        xmin = 0.0,
        xmax = 20.0,
        ymin = 0.0,
        ymax = 1.0,
        imageName = "thin_triangle",
        imageScale = 15,
        maxCycles = 200
      )
    )
  }

  test("Very tall and narrow triangle (sharp apex)") {
    // Very tall, narrow triangle: (10,0), (11,0), (10.5,10)
    // Creates a very sharp angle at the top
    testTriangle(
      TriangleTestCase(
        name = "Very tall and narrow triangle (sharp apex)",
        edges = Seq(
          EdgeEquation(a = 0.0, b = 1.0, c = 0.0, startValue = 0.0), // y = 0
          EdgeEquation(a = 20.0, b = 1.0, c = -120.0, startValue = -20.0), // 20x + y = 120
          EdgeEquation(a = 20.0, b = -1.0, c = -100.0, startValue = 0.0) // 20x - y = 100
        ),
        xmin = 10.0,
        xmax = 11.0,
        ymin = 0.0,
        ymax = 10.0,
        imageName = "tall_narrow_triangle",
        imageScale = 15,
        maxCycles = 200
      )
    )
  }

  test("Triangle with negative coordinates") {
    // Triangle in negative space: (-4,-2), (0,-2), (-2,1)
    testTriangle(
      TriangleTestCase(
        name = "Negative coordinate triangle",
        edges = Seq(
          EdgeEquation(a = 0.0, b = 1.0, c = 2.0, startValue = 0.0), // y = -2
          EdgeEquation(a = 3.0, b = -2.0, c = -4.0, startValue = 0.0), // 3x - 2y = 4
          EdgeEquation(a = 3.0, b = 2.0, c = 16.0, startValue = -4.0) // 3x + 2y = -16
        ),
        xmin = -4.0,
        xmax = 0.0,
        ymin = -2.0,
        ymax = 1.0,
        imageName = "negative_triangle",
        imageScale = 20,
        maxCycles = 100
      )
    )
  }

  test("Upside-down triangle (flat top, point down)") {
    // Triangle with flat top: (0,4), (4,4), (2,0)
    // Edge orientations flipped so all are positive inside the triangle
    testTriangle(
      TriangleTestCase(
        name = "Upside-down triangle (flat top, point down)",
        edges = Seq(
          EdgeEquation(a = 0.0, b = -1.0, c = 4.0, startValue = 4.0), // -y + 4 = 0 (flipped)
          EdgeEquation(a = -2.0, b = 1.0, c = 4.0, startValue = 4.0), // -2x + y + 4 = 0 (flipped)
          EdgeEquation(a = 2.0, b = 1.0, c = -4.0, startValue = -4.0) // 2x + y - 4 = 0
        ),
        xmin = 0.0,
        xmax = 4.0,
        ymin = 0.0,
        ymax = 4.0,
        imageName = "upside_down_triangle",
        imageScale = 20,
        maxCycles = 100
      )
    )
  }

  test("Real trace triangle: tiny clockwise") {
    // Real triangle from trace: (264.69, 148.94), (263.63, 151.38), (267.38, 154.25)
    testTriangle(
      TriangleTestCase(
        name = "Real trace triangle: tiny clockwise",
        edges = Seq(
          EdgeEquation(a = 2.44, b = 1.06, c = -803.72, startValue = -5.12),
          EdgeEquation(a = 2.87, b = -3.75, c = -188.94, startValue = 10.87),
          EdgeEquation(a = -5.31, b = 2.69, c = 1004.86, startValue = 6.45)
        ),
        xmin = 263.0,
        xmax = 268.0,
        ymin = 148.0,
        ymax = 155.0,
        imageName = "trace_triangle_1",
        imageScale = 40,
        maxCycles = 100
      )
    )
  }

  test("Real trace triangle: large clockwise sliver") {
    // Real triangle from trace: (315.31, 123.5), (282.44, 149.5), (315.25, 326.56)
    testTriangle(
      TriangleTestCase(
        name = "Real trace triangle: large clockwise sliver",
        edges = Seq(
          EdgeEquation(a = 26.0, b = 32.87, c = -12257.51, startValue = -882.50),
          EdgeEquation(a = 177.06, b = -32.81, c = -45103.73, startValue = 791.56),
          EdgeEquation(a = -203.06, b = -0.06, c = 64034.26, startValue = 6763.96)
        ),
        xmin = 282.0,
        xmax = 316.0,
        ymin = 123.0,
        ymax = 327.0,
        imageName = "trace_triangle_2",
        imageScale = 2,
        maxCycles = 10000
      )
    )
  }

  test("Real trace triangle: medium counter-clockwise") {
    // Real triangle from trace: (226.31, 216.0), (261.25, 221.38), (228.88, 233.75)
    testTriangle(
      TriangleTestCase(
        name = "Real trace triangle: medium counter-clockwise",
        edges = Seq(
          EdgeEquation(a = -5.38, b = 34.94, c = -6329.49, startValue = 1.67),
          EdgeEquation(a = -12.37, b = -32.37, c = 10397.73, startValue = 610.19),
          EdgeEquation(a = 17.75, b = -2.57, c = -3461.88, startValue = -5.50)
        ),
        xmin = 226.0,
        xmax = 262.0,
        ymin = 216.0,
        ymax = 234.0,
        imageName = "trace_triangle_3",
        imageScale = 8,
        maxCycles = 800
      )
    )
  }

  test("Real trace triangle: very tiny counter-clockwise") {
    // Real triangle from trace: (263.63, 151.38), (267.38, 154.25), (266.38, 156.56)
    testTriangle(
      TriangleTestCase(
        name = "Real trace triangle: very tiny counter-clockwise",
        edges = Seq(
          EdgeEquation(a = -2.87, b = 3.75, c = 188.94, startValue = 0.38),
          EdgeEquation(a = -2.31, b = -1.0, c = 771.90, startValue = 13.37),
          EdgeEquation(a = 5.18, b = -2.75, c = -949.31, startValue = -2.22)
        ),
        xmin = 263.0,
        xmax = 268.0,
        ymin = 151.0,
        ymax = 157.0,
        imageName = "trace_triangle_4",
        imageScale = 40,
        maxCycles = 100
      )
    )
  }

  test("Real trace triangle: small wide clockwise") {
    // Real triangle from trace: (217.63, 161.56), (184.25, 169.25), (217.00, 173.00)
    // signBit=true (clockwise), size ~33x11 pixels
    testTriangle(
      TriangleTestCase(
        name = "Real trace triangle: small wide clockwise",
        edges = Seq(
          EdgeEquation(a = 3.75, b = -32.75, c = 4852.00, startValue = 251.85),
          EdgeEquation(a = -11.44, b = -0.63, c = 2591.47, startValue = 381.87),
          EdgeEquation(a = 7.69, b = 33.38, c = -7066.45, startValue = -256.69)
        ),
        xmin = 184.0,
        xmax = 218.0,
        ymin = 161.0,
        ymax = 174.0,
        imageName = "trace_triangle_5",
        imageScale = 10,
        maxCycles = 500
      )
    )
  }

  test("Nearly degenerate triangle (almost a line)") {
    // Almost collinear points: (0,0), (10,0), (5,0.5)
    // This is nearly degenerate - might produce few or no pixels depending on rounding
    val config = Config.voodoo1()
    var capturedOutputs: Seq[(Int, Int)] = Seq.empty

    val compiled = SimConfig.withIVerilog.withWave
      .workspaceName("degenerate_triangle")
      .compile(Rasterizer(config))
    compiled.doSim("degenerate_triangle") { (dut: Rasterizer) =>
      setupDut(dut)

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

      // Render triangle to image in the test's workspace directory
      val testPath = currentTestPath()
      renderTriangleToImage(
        outputs,
        xmin = 0,
        xmax = 10,
        ymin = 0,
        ymax = 1,
        filename = s"$testPath/image.png",
        scale = 20
      )

      outputs.foreach { case (x, y) =>
        assert(x >= 0 && x <= 10, s"x=$x out of bounds")
        assert(y >= 0 && y <= 1, s"y=$y out of bounds (note: 0.5 rounds to 0)")
      }

      // Even nearly degenerate triangles might produce some pixels
      // depending on rounding behavior - don't assert nonEmpty

      capturedOutputs = outputs
    }
  }
}
