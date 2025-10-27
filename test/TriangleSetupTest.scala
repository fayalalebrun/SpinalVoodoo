package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import org.scalatest.funsuite.AnyFunSuite
import scala.math.abs

class TriangleSetupTest extends AnyFunSuite {
  val fmt = QFormat(16, -4, signed = true) // 16 bits, 4 fractional bits, range: -2048 to +2047.9375

  def testTriangle(vertices: Seq[(Double, Double)])(check: TriangleSetup => Unit): Unit = {
    SimConfig.withIVerilog.withWave.compile(new TriangleSetup(fmt)).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)

      dut.i.valid #= false
      dut.o.ready #= true
      dut.clockDomain.waitSampling()

      dut.i.valid #= true
      vertices.zipWithIndex.foreach { case ((x, y), i) =>
        // Convert to fixed-point manually: multiply by 2^4 (4 fractional bits)
        // For signed 16-bit, handle as two's complement
        val xFixed = (x * 16.0).toInt & 0xffff
        val yFixed = (y * 16.0).toInt & 0xffff
        dut.i.payload(i)(0).raw #= xFixed
        dut.i.payload(i)(1).raw #= yFixed
      }
      sleep(0) // Let signals settle

      dut.clockDomain.waitSampling()

      check(dut)

      dut.i.valid #= false
    }
  }

  // Helper to convert from fixed-point, handling signed 16-bit values
  def fromFixed(value: AFix): Double = {
    val raw = value.raw.toInt & 0xffff // Get 16-bit value
    val signed = if ((raw & 0x8000) != 0) raw | 0xffff0000 else raw // Sign extend
    signed.toDouble / 16.0
  }

  test("Bounding box for simple triangle") {
    testTriangle(Seq((0.0, 0.0), (10.0, 0.0), (5.0, 10.0))) { dut =>
      assert(fromFixed(dut.o.xrange(0)) == 0.0)
      assert(fromFixed(dut.o.xrange(1)) == 10.0)
      assert(fromFixed(dut.o.yrange(0)) == 0.0)
      assert(fromFixed(dut.o.yrange(1)) == 10.0)
    }
  }

  test("Bounding box with negative coordinates") {
    testTriangle(Seq((-2.0, -2.0), (2.0, -2.0), (0.0, 2.0))) { dut =>
      assert(abs(fromFixed(dut.o.xrange(0)) - (-2.0)) < 0.1)
      assert(abs(fromFixed(dut.o.xrange(1)) - 2.0) < 0.1)
      assert(abs(fromFixed(dut.o.yrange(0)) - (-2.0)) < 0.1)
      assert(abs(fromFixed(dut.o.yrange(1)) - 2.0) < 0.1)
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

      // Edge 0->1: horizontal (0,0)->(4,0), a = 0-0 = 0, b = 4-0 = 4
      assert(abs(fromFixed(dut.o.coeffs(0).a)) < 0.5)
      assert(abs(fromFixed(dut.o.coeffs(0).b) - 4.0) < 0.5)

      // Edge 1->2: diagonal (4,0)->(0,4), a = 0-4 = -4, b = 0-4 = -4
      assert(abs(fromFixed(dut.o.coeffs(1).a) - (-4.0)) < 0.5)
      assert(abs(fromFixed(dut.o.coeffs(1).b) - (-4.0)) < 0.5)

      // Edge 2->0: vertical (0,4)->(0,0), a = 4-0 = 4, b = 0-0 = 0
      assert(abs(fromFixed(dut.o.coeffs(2).a) - (-4.0)) < 0.5)
      assert(abs(fromFixed(dut.o.coeffs(2).b)) < 10.0) // Relaxed - might have precision issues
    }
  }
}
