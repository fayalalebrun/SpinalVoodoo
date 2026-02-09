package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim._
import spinal.lib.sim.StreamMonitor
import org.scalatest.funsuite.AnyFunSuite

/** Debug test to compare manual BMB protocol vs BmbDriver timing Uses StreamMonitor to observe bus
  * transactions
  */
class BmbDriverDebugTest extends AnyFunSuite {

  test("Compare manual BMB vs BmbDriver timing") {
    val config = Config.voodoo1()

    SimConfig.withIVerilog.withWave.compile(RegisterBank(config)).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)

      // Setup (pciFifoFree now comes from actual FIFO)
      dut.io.statusInputs.vRetrace #= true
      dut.io.statusInputs.fbiBusy #= false
      dut.io.statusInputs.trexBusy #= false
      dut.io.statusInputs.sstBusy #= false
      dut.io.statusInputs.memFifoFree #= 0xffff
      dut.io.statusInputs.pciInterrupt #= false
      dut.io.swapDisplayedBuffer #= 0
      dut.io.swapsPending #= 0

      dut.io.statisticsIn.pixelsIn #= 0
      dut.io.statisticsIn.chromaFail #= 0
      dut.io.statisticsIn.zFuncFail #= 0
      dut.io.statisticsIn.aFuncFail #= 0
      dut.io.statisticsIn.pixelsOut #= 0

      dut.io.pipelineBusy #= false

      dut.commands.triangleCmd.ready #= true
      dut.commands.fastfillCmd.ready #= true
      dut.commands.nopCmd.ready #= true
      dut.commands.swapbufferCmd.ready #= true
      dut.commands.ftriangleCmd.ready #= true

      dut.clockDomain.waitSampling()

      // Monitor BMB command and response streams
      var cmdCount = 0
      var rspCount = 0

      StreamMonitor(dut.io.bus.cmd, dut.clockDomain) { payload =>
        val cycle = simTime() / 10 // assuming 10ns period
        println(
          f"[Cycle $cycle%3d] BMB CMD: opcode=${payload.opcode.toInt} addr=0x${payload.address.toLong}%03x data=0x${payload.data.toLong}%08x valid=${dut.io.bus.cmd.valid.toBoolean} ready=${dut.io.bus.cmd.ready.toBoolean}"
        )
        cmdCount += 1
      }

      StreamMonitor(dut.io.bus.rsp, dut.clockDomain) { payload =>
        val cycle = simTime() / 10
        println(
          f"[Cycle $cycle%3d] BMB RSP: data=0x${payload.data.toLong}%08x valid=${dut.io.bus.rsp.valid.toBoolean}"
        )
        rspCount += 1
      }

      println("\n=== TEST 1: Manual BMB Protocol ===")
      val testAddr = 0x0c0
      val testValue = 0xdeadbeefL

      // Manual write
      println(
        s"Writing 0x${testValue.toHexString} to address 0x${testAddr.toHexString} using MANUAL protocol"
      )
      dut.io.bus.cmd.valid #= true
      dut.io.bus.cmd.address #= testAddr
      dut.io.bus.cmd.data #= testValue
      dut.io.bus.cmd.opcode #= Bmb.Cmd.Opcode.WRITE
      dut.io.bus.cmd.length #= 3
      dut.io.bus.cmd.last #= true
      dut.io.bus.cmd.mask #= 0xf
      dut.io.bus.rsp.ready #= true

      dut.clockDomain.waitSampling()
      println(
        s"[Manual] After cycle 1: fbzColorPath = 0x${dut.renderConfig.fbzColorPath.toLong.toHexString}"
      )

      dut.io.bus.cmd.valid #= false
      dut.clockDomain.waitSampling()
      println(
        s"[Manual] After cycle 2: fbzColorPath = 0x${dut.renderConfig.fbzColorPath.toLong.toHexString}"
      )

      // Wait for FIFO drain
      dut.clockDomain.waitSampling(2)
      println(
        s"[Manual] After FIFO drain (cycle 4): fbzColorPath = 0x${dut.renderConfig.fbzColorPath.toLong.toHexString}"
      )

      dut.clockDomain.waitSampling()
      println(
        s"[Manual] After extra cycle (cycle 5): fbzColorPath = 0x${dut.renderConfig.fbzColorPath.toLong.toHexString}"
      )

      val manualResult = dut.renderConfig.fbzColorPath.toLong
      println(
        s"\n[Manual] Final value: 0x${manualResult.toHexString}, expected: 0x${testValue.toHexString}"
      )

      // Reset register
      dut.renderConfig.fbzColorPath #= 0
      dut.clockDomain.waitSampling(5)

      println("\n\n=== TEST 2: BmbDriver ===")
      val testAddr2 = 0x0c4
      val testValue2 = 0xcafebabeL

      val driver = BmbDriver(dut.io.bus, dut.clockDomain)

      println(
        s"Writing 0x${testValue2.toHexString} to address 0x${testAddr2.toHexString} using BmbDriver"
      )
      driver.write(address = testAddr2, data = testValue2)
      println(
        s"[BmbDriver] After driver.write() returns: fogColor = 0x${dut.renderConfig.fogColor.toLong.toHexString}"
      )

      dut.clockDomain.waitSampling()
      println(
        s"[BmbDriver] After 1 cycle: fogColor = 0x${dut.renderConfig.fogColor.toLong.toHexString}"
      )

      dut.clockDomain.waitSampling()
      println(
        s"[BmbDriver] After 2 cycles: fogColor = 0x${dut.renderConfig.fogColor.toLong.toHexString}"
      )

      dut.clockDomain.waitSampling()
      println(
        s"[BmbDriver] After 3 cycles: fogColor = 0x${dut.renderConfig.fogColor.toLong.toHexString}"
      )

      val driverResult = dut.renderConfig.fogColor.toLong
      println(
        s"\n[BmbDriver] Final value: 0x${driverResult.toHexString}, expected: 0x${testValue2.toHexString}"
      )

      dut.clockDomain.waitSampling(10)

      println(s"\nTotal BMB commands: $cmdCount")
      println(s"Total BMB responses: $rspCount")
    }
  }
}
