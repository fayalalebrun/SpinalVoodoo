package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import org.scalatest.funsuite.AnyFunSuite

class StreamWhileTest extends AnyFunSuite {
  test("StreamWhile decrements value to zero") {
    SimConfig.withIVerilog.withWave
      .compile(new Component {
        val start = slave Stream (UInt(8 bits))
        val output = master Stream (UInt(8 bits))

        output << StreamWhile(
          start,
          stateType = HardType(UInt(8 bits)),
          outputType = HardType(UInt(8 bits)),
          maxIterations = 16,
          init = (value: UInt) => value,
          step = (idx: UInt, value: UInt, output: UInt) => {
            output := value
            val nextValue = value - 1
            val isLast = value === 1 // Last output when current value is 1 (next would be 0)
            (nextValue, isLast)
          }
        )
      })
      .doSim { dut =>
        dut.clockDomain.forkStimulus(period = 10)

        dut.start.valid #= false
        dut.output.ready #= false
        dut.clockDomain.waitSampling()

        // Test: Start with value 5, should produce outputs 5, 4, 3, 2, 1
        dut.start.valid #= true
        dut.start.payload #= 5
        dut.output.ready #= true
        dut.clockDomain.waitSampling()

        dut.start.valid #= false

        // Should get 5 outputs
        val outputs = scala.collection.mutable.ArrayBuffer[Int]()
        for (_ <- 0 until 20) {
          dut.clockDomain.waitSampling()
          if (dut.output.valid.toBoolean) {
            outputs += dut.output.payload.toInt
          }
        }

        assert(outputs == Seq(5, 4, 3, 2, 1))

        // After completion, start should be ready again
        assert(dut.start.ready.toBoolean)
      }
  }

  test("StreamWhile respects maxIterations") {
    SimConfig.withIVerilog.withWave
      .compile(new Component {
        val start = slave Stream (UInt(8 bits))
        val output = master Stream (UInt(8 bits))

        output << StreamWhile(
          start,
          stateType = HardType(UInt(8 bits)),
          outputType = HardType(UInt(8 bits)),
          maxIterations = 3,
          init = (value: UInt) => value,
          step = (idx: UInt, value: UInt, output: UInt) => {
            output := value
            val nextValue = value - 1
            val isLast = value === 1 // Last output when current value is 1 (next would be 0)
            (nextValue, isLast)
          }
        )
      })
      .doSim { dut =>
        dut.clockDomain.forkStimulus(period = 10)

        dut.start.valid #= false
        dut.output.ready #= true
        dut.clockDomain.waitSampling()

        // Test: Start with value 10, but maxIterations is 3
        dut.start.valid #= true
        dut.start.payload #= 10
        dut.clockDomain.waitSampling()

        dut.start.valid #= false

        // Should get only 3 outputs
        val outputs = scala.collection.mutable.ArrayBuffer[Int]()
        for (_ <- 0 until 20) {
          dut.clockDomain.waitSampling()
          if (dut.output.valid.toBoolean) {
            outputs += dut.output.payload.toInt
          }
        }

        assert(outputs.size == 3)
        assert(outputs == Seq(10, 9, 8))
      }
  }

  test("StreamWhile handles backpressure") {
    SimConfig.withIVerilog.withWave
      .compile(new Component {
        val start = slave Stream (UInt(8 bits))
        val output = master Stream (UInt(8 bits))

        output << StreamWhile(
          start,
          stateType = HardType(UInt(8 bits)),
          outputType = HardType(UInt(8 bits)),
          maxIterations = 16,
          init = (value: UInt) => value,
          step = (idx: UInt, value: UInt, output: UInt) => {
            output := value
            val nextValue = value - 1
            val isLast = value === 1 // Last output when current value is 1 (next would be 0)
            (nextValue, isLast)
          }
        )
      })
      .doSim { dut =>
        dut.clockDomain.forkStimulus(period = 10)

        dut.start.valid #= false
        dut.output.ready #= false
        dut.clockDomain.waitSampling()

        // Test: Start with value 3
        dut.start.valid #= true
        dut.start.payload #= 3
        dut.clockDomain.waitSampling()

        dut.start.valid #= false

        // Apply backpressure - only accept every other cycle
        val outputs = scala.collection.mutable.ArrayBuffer[Int]()
        for (i <- 0 until 20) {
          dut.output.ready #= (i % 2 == 0)
          dut.clockDomain.waitSampling()
          if (dut.output.valid.toBoolean && dut.output.ready.toBoolean) {
            outputs += dut.output.payload.toInt
          }
        }

        assert(outputs == Seq(3, 2, 1))
      }
  }
}
