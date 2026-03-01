package voodoo

import spinal.core._
import spinal.core.sim._
import org.scalatest.funsuite.AnyFunSuite

class DitherTest extends AnyFunSuite {

  lazy val compiled = SimConfig.withIVerilog.compile(Dither())

  /** Drive a dither transaction: present input, wait for acceptance, then wait for output.
    *
    * Deasserts input.valid after acceptance to prevent re-fire via streamReadSync's isFree.
    */
  def ditherTransaction(
      dut: Dither,
      r: Int,
      g: Int,
      b: Int,
      x: Int,
      y: Int,
      enable: Boolean,
      use2x2: Boolean
  ): (Int, Int, Int) = {
    // Phase 1: Present input, wait for it to be accepted
    dut.io.input.valid #= true
    dut.io.input.payload.r #= r
    dut.io.input.payload.g #= g
    dut.io.input.payload.b #= b
    dut.io.input.payload.x #= x
    dut.io.input.payload.y #= y
    dut.io.input.payload.enable #= enable
    dut.io.input.payload.use2x2 #= use2x2
    dut.io.output.ready #= false

    dut.clockDomain.waitSamplingWhere(dut.io.input.ready.toBoolean)
    // Input accepted at this edge; deassert to prevent re-fire
    dut.io.input.valid #= false

    // Phase 2: Wait for output
    dut.io.output.ready #= true
    dut.clockDomain.waitSamplingWhere(dut.io.output.valid.toBoolean)

    val result = (
      dut.io.output.payload.ditR.toInt,
      dut.io.output.payload.ditG.toInt,
      dut.io.output.payload.ditB.toInt
    )
    dut.io.output.ready #= false
    result
  }

  test("4x4 dither mode - exhaustive") {
    compiled.doSim("dither_4x4") { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.input.valid #= false
      dut.io.output.ready #= false
      dut.clockDomain.waitSampling(3) // let reset deassert

      var mismatches = 0
      for (y <- 0 until 4; x <- 0 until 4; v <- 0 until 256) {
        val (simR, simG, simB) =
          ditherTransaction(dut, r = v, g = v, b = v, x = x, y = y, enable = true, use2x2 = false)

        val refR = DitherTables.lookupRb(v, y, x)
        val refG = DitherTables.lookupG(v, y, x)
        val refB = DitherTables.lookupRb(v, y, x)

        if (simR != refR || simG != refG || simB != refB) {
          if (mismatches < 20)
            println(
              f"4x4 MISMATCH v=$v x=$x y=$y: simR=$simR refR=$refR simG=$simG refG=$refG simB=$simB refB=$refB"
            )
          mismatches += 1
        }
      }
      println(s"[4x4] Total mismatches: $mismatches / ${4 * 4 * 256}")
      assert(mismatches == 0, s"4x4 dither: $mismatches mismatches")
    }
  }

  test("2x2 dither mode - exhaustive") {
    compiled.doSim("dither_2x2") { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.input.valid #= false
      dut.io.output.ready #= false
      dut.clockDomain.waitSampling(3) // let reset deassert

      var mismatches = 0
      for (y <- 0 until 2; x <- 0 until 2; v <- 0 until 256) {
        val (simR, simG, simB) =
          ditherTransaction(dut, r = v, g = v, b = v, x = x, y = y, enable = true, use2x2 = true)

        val refR = DitherTables.lookupRb2x2(v, y, x)
        val refG = DitherTables.lookupG2x2(v, y, x)
        val refB = DitherTables.lookupRb2x2(v, y, x)

        if (simR != refR || simG != refG || simB != refB) {
          if (mismatches < 20)
            println(
              f"2x2 MISMATCH v=$v x=$x y=$y: simR=$simR refR=$refR simG=$simG refG=$refG simB=$simB refB=$refB"
            )
          mismatches += 1
        }
      }
      println(s"[2x2] Total mismatches: $mismatches / ${2 * 2 * 256}")
      assert(mismatches == 0, s"2x2 dither: $mismatches mismatches")
    }
  }

  test("disabled bypass") {
    compiled.doSim("dither_bypass") { dut =>
      dut.clockDomain.forkStimulus(period = 10)
      dut.io.input.valid #= false
      dut.io.output.ready #= false
      dut.clockDomain.waitSampling(3) // let reset deassert

      val testValues = Seq(0, 1, 7, 8, 127, 128, 248, 255)
      var mismatches = 0
      for (v <- testValues) {
        val (simR, simG, simB) =
          ditherTransaction(dut, r = v, g = v, b = v, x = 0, y = 0, enable = false, use2x2 = false)

        val expR = v >> 3
        val expG = v >> 2
        val expB = v >> 3

        if (simR != expR || simG != expG || simB != expB) {
          println(
            f"bypass MISMATCH v=$v: simR=$simR expR=$expR simG=$simG expG=$expG simB=$simB expB=$expB"
          )
          mismatches += 1
        }
      }
      println(s"[bypass] Total mismatches: $mismatches / ${testValues.size}")
      assert(mismatches == 0, s"bypass: $mismatches mismatches")
    }
  }
}
