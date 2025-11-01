package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import org.scalatest.funsuite.AnyFunSuite
import scala.math.abs

class TriangleSetupTest extends AnyFunSuite {
  val cfg = Config.voodoo1()
  val fmt = cfg.vertexFormat

  def testTriangle(vertices: Seq[(Double, Double)])(check: TriangleSetup => Unit): Unit = {
    SimConfig.withIVerilog.withWave.compile(new TriangleSetup(cfg)).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)

      dut.i.valid #= false
      dut.o.ready #= true
      dut.clockDomain.waitSampling()

      dut.i.valid #= true
      vertices.zipWithIndex.foreach { case ((x, y), i) =>
        // SQ(12, 4) = 12 bits total, 4 fractional bits
        // Scale factor is 2^4 = 16
        val scale = 16
        val xFixed = (x * scale).toInt
        val yFixed = (y * scale).toInt
        dut.i.payload(i)(0) #= xFixed
        dut.i.payload(i)(1) #= yFixed
      }
      sleep(0) // Let signals settle

      dut.clockDomain.waitSampling()

      check(dut)

      dut.i.valid #= false
    }
  }

  // Helper to convert from fixed-point (SQ(12, 4) = 12 bits, 4 fractional)
  def fromFixed(value: AFix): Double = {
    value.toDouble
  }

  test("Bounding box for simple triangle") {
    testTriangle(Seq((0.0, 0.0), (6.0, 0.0), (3.0, 6.0))) { dut =>
      // Values are in scaled format (multiplied by 16)
      assert(fromFixed(dut.o.xrange(0)) == 0.0)
      assert(fromFixed(dut.o.xrange(1)) == 96.0) // 6.0 * 16
      assert(fromFixed(dut.o.yrange(0)) == 0.0)
      assert(fromFixed(dut.o.yrange(1)) == 96.0) // 6.0 * 16
    }
  }

  test("Bounding box with negative coordinates") {
    testTriangle(Seq((-1.5, -1.5), (1.5, -1.5), (0.0, 1.5))) { dut =>
      // Values are in scaled format (multiplied by 16)
      assert(abs(fromFixed(dut.o.xrange(0)) - (-24.0)) < 0.1) // -1.5 * 16
      assert(abs(fromFixed(dut.o.xrange(1)) - 24.0) < 0.1) // 1.5 * 16
      assert(abs(fromFixed(dut.o.yrange(0)) - (-24.0)) < 0.1) // -1.5 * 16
      assert(abs(fromFixed(dut.o.yrange(1)) - 24.0) < 0.1) // 1.5 * 16
    }
  }

  test("Edge equations for right triangle") {
    testTriangle(Seq((0.0, 0.0), (4.0, 0.0), (0.0, 4.0))) { dut =>
      // Debug: print all coefficient values
      println(
        s"Edge 0->1: a=${fromFixed(dut.o.coeffs(0).a)} (raw=${dut.o.coeffs(0).a.raw.toInt & 0xffff}), b=${fromFixed(dut.o.coeffs(0).b)} (raw=${dut.o.coeffs(0).b.raw.toInt & 0xffff})"
      )
      println(
        s"Edge 1->2: a=${fromFixed(dut.o.coeffs(1).a)} (raw=${dut.o.coeffs(1).a.raw.toInt & 0xffff}), b=${fromFixed(dut.o.coeffs(1).b)} (raw=${dut.o.coeffs(1).b.raw.toInt & 0xffff})"
      )
      println(
        s"Edge 2->0: a=${fromFixed(dut.o.coeffs(2).a)} (raw=${dut.o.coeffs(2).a.raw.toInt & 0xffff}), b=${fromFixed(dut.o.coeffs(2).b)} (raw=${dut.o.coeffs(2).b.raw.toInt & 0xffff})"
      )

      // Edge 0->1: horizontal (0,0)->(4,0), a = 0-0 = 0, b = 4-0 = 64 (scaled)
      assert(abs(fromFixed(dut.o.coeffs(0).a)) < 0.5)
      assert(abs(fromFixed(dut.o.coeffs(0).b) - 64.0) < 0.5)

      // Edge 1->2: diagonal (4,0)->(0,4), a = 0-4 = -64, b = 0-4 = -64 (scaled)
      assert(abs(fromFixed(dut.o.coeffs(1).a) - (-64.0)) < 0.5)
      assert(abs(fromFixed(dut.o.coeffs(1).b) - (-64.0)) < 0.5)

      // Edge 2->0: vertical (0,4)->(0,0), a = 4-0 = 64, b = 0-0 = 0 (scaled)
      assert(abs(fromFixed(dut.o.coeffs(2).a) - 64.0) < 0.5)
      assert(abs(fromFixed(dut.o.coeffs(2).b)) < 0.5)
    }
  }

  test("Initial edge values at bounding box corner") {
    testTriangle(Seq((0.0, 0.0), (4.0, 0.0), (0.0, 4.0))) { dut =>
      val xmin = fromFixed(dut.o.xrange(0))
      val ymin = fromFixed(dut.o.yrange(0))

      println(s"Bounding box corner: ($xmin, $ymin)")

      for (i <- 0 until 3) {
        val a = fromFixed(dut.o.coeffs(i).a)
        val b = fromFixed(dut.o.coeffs(i).b)
        val c = fromFixed(dut.o.coeffs(i).c)
        val edgeStart = fromFixed(dut.o.edgeStart(i))
        val expected = a * xmin + b * ymin + c

        println(s"Edge $i: a=$a, b=$b, c=$c")
        println(s"  edgeStart=$edgeStart, expected=$expected")

        assert(
          abs(edgeStart - expected) < 1.0,
          s"Edge $i: edgeStart=$edgeStart should equal a*xmin + b*ymin + c = $expected"
        )
      }
    }
  }
}
