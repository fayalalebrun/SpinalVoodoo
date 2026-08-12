package voodoo

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._

class FogTest extends AnyFunSuite {
  private val config = Config.voodoo1()

  private def init(dut: Fog): Unit = {
    dut.clockDomain.forkStimulus(10)
    dut.io.input.valid #= false
    dut.io.output.ready #= true
    dut.io.input.payload.coords(0) #= 0
    dut.io.input.payload.coords(1) #= 0
    dut.io.input.payload.alpha #= 255
    dut.io.input.payload.depth #= 0
    dut.io.input.payload.iteratedAlpha #= 0
    dut.io.input.payload.rawW #= 0
    dut.io.input.payload.chromaKey #= 0
    dut.io.input.payload.zaColor #= 0
    dut.io.input.payload.routing.colorBaseAddr #= 0
    dut.io.input.payload.routing.auxBaseAddr #= 0
    dut.io.input.payload.routing.pixelStride #= 640
    for (i <- 0 until 64) dut.io.fogTable(i) #= 0
    dut.clockDomain.waitSampling()
  }

  private def drive(
      dut: Fog,
      color: (Int, Int, Int),
      fog: (Int, Int, Int),
      enable: Boolean,
      add: Boolean = false,
      mult: Boolean = false,
      constant: Boolean = false,
      mode: Int = 1,
      factor: Int = 0
  ): (Int, Int, Int) = {
    dut.io.input.payload.color.r #= color._1
    dut.io.input.payload.color.g #= color._2
    dut.io.input.payload.color.b #= color._3
    dut.io.input.payload.fogColor #= ((fog._1 << 16) | (fog._2 << 8) | fog._3)
    dut.io.input.payload.fogMode.fogEnable #= enable
    dut.io.input.payload.fogMode.fogAdd #= add
    dut.io.input.payload.fogMode.fogMult #= mult
    dut.io.input.payload.fogMode.fogConstant #= constant
    dut.io.input.payload.fogMode.fogModeSelect #= mode
    dut.io.input.payload.iteratedAlpha #= factor
    dut.io.input.valid #= true
    dut.clockDomain.waitSamplingWhere(dut.io.input.ready.toBoolean)
    dut.io.input.valid #= false
    dut.clockDomain.waitSamplingWhere(dut.io.output.valid.toBoolean)
    (
      dut.io.output.payload.color.r.toInt,
      dut.io.output.payload.color.g.toInt,
      dut.io.output.payload.color.b.toInt
    )
  }

  test("fog bypass preserves color") {
    SimConfig.withIVerilog.compile(Fog(config)).doSim { dut =>
      init(dut)
      assert(drive(dut, (12, 34, 56), (200, 210, 220), enable = false) == (12, 34, 56))
    }
  }

  test("constant fog saturating-adds fog color") {
    SimConfig.withIVerilog.compile(Fog(config)).doSim { dut =>
      init(dut)
      assert(
        drive(dut, (100, 240, 10), (20, 30, 250), enable = true, constant = true) ==
          (120, 255, 255)
      )
    }
  }

  test("alpha-selected fog blends toward fog color") {
    SimConfig.withIVerilog.compile(Fog(config)).doSim { dut =>
      init(dut)
      val result = drive(dut, (0, 0, 0), (255, 128, 64), enable = true, factor = 127)
      assert(result == (127, 64, 32))
    }
  }

  test("fog add and multiply modes select attenuation products") {
    SimConfig.withIVerilog.compile(Fog(config)).doSim { dut =>
      init(dut)
      assert(
        drive(dut, (100, 100, 100), (200, 200, 200), enable = true, add = true, factor = 127) == (
          50,
          50,
          50
        )
      )
      assert(
        drive(dut, (100, 100, 100), (100, 100, 100), enable = true, mult = true, factor = 127) == (
          50,
          50,
          50
        )
      )
      assert(
        drive(
          dut,
          (100, 100, 100),
          (200, 200, 200),
          enable = true,
          add = true,
          mult = true,
          factor = 127
        ) == (0, 0, 0)
      )
    }
  }

  test("fog selects Z and W-table factors") {
    SimConfig.withIVerilog.compile(Fog(config)).doSim { dut =>
      init(dut)
      dut.io.input.payload.depth.raw #= (BigInt(128) << 20)
      assert(
        drive(dut, (0, 0, 0), (100, 100, 100), enable = true, mode = 2) ==
          (50, 50, 50)
      )

      dut.io.input.payload.rawW #= 0
      dut.io.fogTable(60) #= 0x0080
      assert(
        drive(dut, (0, 0, 0), (100, 100, 100), enable = true, mode = 0) ==
          (50, 50, 50)
      )
      assert(dut.io.output.payload.wDepth.toInt == 0xf001)
    }
  }
}
