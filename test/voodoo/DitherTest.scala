package voodoo

import spinal.core._
import spinal.core.sim._
import org.scalatest.funsuite.AnyFunSuite
import voodoo.ref.VoodooReference

class DitherTest extends AnyFunSuite {

  lazy val compiled = SimConfig.withIVerilog.compile(Dither())

  val testValues = Seq(0, 1, 7, 8, 127, 128, 248, 255)

  test("4x4 dither mode") {
    compiled.doSim("dither_4x4") { dut =>
      dut.io.enable #= true
      dut.io.use2x2 #= false

      sleep(1)

      var mismatches = 0
      for (y <- 0 until 4; x <- 0 until 4; v <- testValues) {
        dut.io.x #= x
        dut.io.y #= y
        dut.io.r #= v
        dut.io.g #= v
        dut.io.b #= v
        sleep(1)

        val simR = dut.io.ditR.toInt
        val simG = dut.io.ditG.toInt
        val simB = dut.io.ditB.toInt

        val refR = VoodooReference.dither_rb(v, y, x)
        val refG = VoodooReference.dither_g(v, y, x)
        val refB = VoodooReference.dither_rb(v, y, x)

        if (simR != refR || simG != refG || simB != refB) {
          if (mismatches < 20)
            println(f"4x4 MISMATCH v=$v x=$x y=$y: simR=$simR refR=$refR simG=$simG refG=$refG simB=$simB refB=$refB")
          mismatches += 1
        }
      }
      println(s"[4x4] Total mismatches: $mismatches / ${4 * 4 * testValues.size}")
      assert(mismatches == 0, s"4x4 dither: $mismatches mismatches")
    }
  }

  test("2x2 dither mode") {
    compiled.doSim("dither_2x2") { dut =>
      dut.io.enable #= true
      dut.io.use2x2 #= true

      sleep(1)

      var mismatches = 0
      for (y <- 0 until 2; x <- 0 until 2; v <- testValues) {
        dut.io.x #= x
        dut.io.y #= y
        dut.io.r #= v
        dut.io.g #= v
        dut.io.b #= v
        sleep(1)

        val simR = dut.io.ditR.toInt
        val simG = dut.io.ditG.toInt
        val simB = dut.io.ditB.toInt

        val refR = VoodooReference.dither_rb2x2(v, y, x)
        val refG = VoodooReference.dither_g2x2(v, y, x)
        val refB = VoodooReference.dither_rb2x2(v, y, x)

        if (simR != refR || simG != refG || simB != refB) {
          if (mismatches < 20)
            println(f"2x2 MISMATCH v=$v x=$x y=$y: simR=$simR refR=$refR simG=$simG refG=$refG simB=$simB refB=$refB")
          mismatches += 1
        }
      }
      println(s"[2x2] Total mismatches: $mismatches / ${2 * 2 * testValues.size}")
      assert(mismatches == 0, s"2x2 dither: $mismatches mismatches")
    }
  }

  test("disabled bypass") {
    compiled.doSim("dither_bypass") { dut =>
      dut.io.enable #= false
      dut.io.use2x2 #= false

      sleep(1)

      var mismatches = 0
      for (v <- testValues) {
        dut.io.x #= 0
        dut.io.y #= 0
        dut.io.r #= v
        dut.io.g #= v
        dut.io.b #= v
        sleep(1)

        val simR = dut.io.ditR.toInt
        val simG = dut.io.ditG.toInt
        val simB = dut.io.ditB.toInt

        val expR = v >> 3
        val expG = v >> 2
        val expB = v >> 3

        if (simR != expR || simG != expG || simB != expB) {
          println(f"bypass MISMATCH v=$v: simR=$simR expR=$expR simG=$simG expG=$expG simB=$simB expB=$expB")
          mismatches += 1
        }
      }
      println(s"[bypass] Total mismatches: $mismatches / ${testValues.size}")
      assert(mismatches == 0, s"bypass: $mismatches mismatches")
    }
  }
}
