//> using target.scope test

package voodoo.de10

import org.scalatest.funsuite.AnyFunSuite
import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo.bus.PciFifo
import voodoo.core.CoreCpuFrontdoor
import voodoo.frontend.{AddressRemapper, RegisterBank}
import voodoo.{Config, Core}

import scala.language.reflectiveCalls

class H2fRegisterPathTest extends AnyFunSuite {
  case class H2fRegisterPathHarness() extends Component {
    val c = Config.voodoo1()

    val io = new Bundle {
      val h2fLw = slave(H2fLwMmio(Core.cpuBmbParams.access.addressWidth - 2))
      val pipelineBusy = in Bool ()
      val pciFifoFree = out UInt (7 bits)
      val pciFifoEmpty = out Bool ()
    }

    val h2fBridge =
      H2fLwToBmbBridge(Core.cpuBmbParams.access.addressWidth, Core.cpuBmbParams, timeoutLog2 = 8)
    val frontdoor = CoreCpuFrontdoor(c)
    val addressRemapper =
      AddressRemapper(RegisterBank.externalBmbParams(c), RegisterBank.bmbParams(c))
    val regBank = RegisterBank(c)
    val pciFifo = PciFifo(
      busParams = RegisterBank.bmbParams(c),
      categories = regBank.busif.getCategories,
      floatAliases = regBank.busif.getFloatAliases,
      commandAddresses = regBank.busif.getCommandStreamReady.keys.toSeq.sorted
    )

    io.h2fLw <> h2fBridge.io.h2fLw
    h2fBridge.io.cpuBus <> frontdoor.io.cpuBus
    frontdoor.io.regBus <> addressRemapper.io.input
    addressRemapper.io.output <> pciFifo.io.cpuSide
    pciFifo.io.regSide <> regBank.io.bus
    pciFifo.io.texWrite << frontdoor.io.texWrite
    regBank.io.floatShadow <> pciFifo.io.floatShadow

    frontdoor.io.texBaseAddr := regBank.tmuConfig.texBaseAddr
    frontdoor.io.lfbBus.cmd.ready := False
    frontdoor.io.lfbBus.rsp.valid := False
    frontdoor.io.lfbBus.rsp.last := True
    frontdoor.io.lfbBus.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
    frontdoor.io.lfbBus.rsp.source := 0
    frontdoor.io.lfbBus.rsp.data := 0
    frontdoor.io.texReadBus.cmd.ready := False
    frontdoor.io.texReadBus.rsp.valid := False
    frontdoor.io.texReadBus.rsp.last := True
    frontdoor.io.texReadBus.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
    frontdoor.io.texReadBus.rsp.source := 0
    frontdoor.io.texReadBus.rsp.data := 0

    pciFifo.io.texDrain.ready := True
    pciFifo.io.pipelineBusy := io.pipelineBusy
    pciFifo.io.commandReady := regBank.io.commandReady
    pciFifo.io.wasEnqueuedAddr := U(0x128, pciFifo.busAddrWidth bits)

    regBank.io.statusInputs.vRetrace := True
    regBank.io.statusInputs.memFifoFree := 0xffff
    regBank.io.statusInputs.pciInterrupt := False
    regBank.io.swapDisplayedBuffer := 0
    regBank.io.swapsPending := 0
    regBank.io.statistics.assignFromBits(B(0, regBank.io.statistics.asBits.getWidth bits))
    regBank.io.debug.assignFromBits(B(0, regBank.io.debug.asBits.getWidth bits))
    regBank.io.pciFifoEmpty := pciFifo.io.fifoEmpty
    regBank.io.pciFifoFree := pciFifo.io.pciFifoFree
    regBank.io.swapCmdEnqueued := pciFifo.io.wasEnqueued
    regBank.io.syncDrained := pciFifo.io.syncDrained
    regBank.io.drawRouting.assignFromBits(B(0, regBank.io.drawRouting.asBits.getWidth bits))

    regBank.commands.triangleCmd.ready := True
    regBank.commands.ftriangleCmd.ready := True
    regBank.commands.nopCmd.ready := True
    regBank.commands.fastfillCmd.ready := True
    regBank.commands.swapbufferCmd.ready := True

    io.pciFifoFree := pciFifo.io.pciFifoFree
    io.pciFifoEmpty := pciFifo.io.fifoEmpty
  }

  private def init(dut: H2fRegisterPathHarness): Unit = {
    dut.io.h2fLw.address #= 0
    dut.io.h2fLw.read #= false
    dut.io.h2fLw.write #= false
    dut.io.h2fLw.byteenable #= 0xf
    dut.io.h2fLw.writedata #= 0
    dut.io.pipelineBusy #= false
    dut.clockDomain.waitSampling()
  }

  private def waitUntil(maxCycles: Int, clue: String)(
      cond: => Boolean
  )(implicit dut: H2fRegisterPathHarness): Unit = {
    var cycles = 0
    while (!cond && cycles < maxCycles) {
      dut.clockDomain.waitSampling()
      cycles += 1
    }
    assert(cond, clue)
  }

  private def mmioWrite(byteAddress: Int, value: Long, maxWait: Int = 64)(implicit
      dut: H2fRegisterPathHarness
  ): Unit = {
    dut.io.h2fLw.address #= (byteAddress >> 2)
    dut.io.h2fLw.writedata #= value
    dut.io.h2fLw.byteenable #= 0xf
    dut.io.h2fLw.write #= true
    dut.clockDomain.waitSampling()
    waitUntil(maxWait, f"write 0x$byteAddress%06x did not get accepted") {
      !dut.io.h2fLw.waitrequest.toBoolean
    }
    dut.io.h2fLw.write #= false
    dut.clockDomain.waitSampling(2)
  }

  private def mmioRead(byteAddress: Int, maxWait: Int = 64)(implicit
      dut: H2fRegisterPathHarness
  ): Long = {
    dut.io.h2fLw.address #= (byteAddress >> 2)
    dut.io.h2fLw.byteenable #= 0xf
    dut.io.h2fLw.read #= true
    dut.clockDomain.waitSampling()
    waitUntil(maxWait, f"read 0x$byteAddress%06x did not get accepted") {
      !dut.io.h2fLw.waitrequest.toBoolean
    }
    dut.io.h2fLw.read #= false
    waitUntil(maxWait, f"read 0x$byteAddress%06x did not return data") {
      dut.io.h2fLw.readdatavalid.toBoolean
    }
    val value = dut.io.h2fLw.readdata.toLong & 0xffffffffL
    dut.clockDomain.waitSampling()
    value
  }

  test("exact low register writes complete through the FPGA register path") {
    SimConfig.withIVerilog.compile(H2fRegisterPathHarness()).doSim { dut0 =>
      implicit val dut: H2fRegisterPathHarness = dut0
      dut.clockDomain.forkStimulus(10)
      init(dut)

      mmioWrite(0x104, 0x00000002L)
      mmioWrite(0x110, 0x00000300L)
      mmioWrite(0x210, 0x00000000L)

      assert(mmioRead(0x104) == 0x00000002L)
      assert((mmioRead(0x110) & 0x0000ffffL) == 0x00000300L)
      assert(dut.io.pciFifoEmpty.toBoolean, "FIFO should drain when pipeline is idle")
    }
  }

  test("a sync register write stalls only after the PCI FIFO fills behind pipeline busy") {
    SimConfig.withIVerilog.compile(H2fRegisterPathHarness()).doSim { dut0 =>
      implicit val dut: H2fRegisterPathHarness = dut0
      dut.clockDomain.forkStimulus(10)
      init(dut)
      dut.io.pipelineBusy #= true
      dut.clockDomain.waitSampling()

      for (i <- 0 until 64) {
        mmioWrite(0x110, i, maxWait = 256)
      }

      dut.io.h2fLw.address #= (0x110 >> 2)
      dut.io.h2fLw.writedata #= 0x5555
      dut.io.h2fLw.byteenable #= 0xf
      dut.io.h2fLw.write #= true
      dut.clockDomain.waitSampling()
      assert(
        dut.io.h2fLw.waitrequest.toBoolean,
        "H2F should backpressure only once the FIFO is full"
      )
      dut.io.h2fLw.write #= false
    }
  }

  test("bypass register write still completes when the render FIFO is blocked") {
    SimConfig.withIVerilog.compile(H2fRegisterPathHarness()).doSim { dut0 =>
      implicit val dut: H2fRegisterPathHarness = dut0
      dut.clockDomain.forkStimulus(10)
      init(dut)
      dut.io.pipelineBusy #= true
      dut.clockDomain.waitSampling()

      for (i <- 0 until 64) {
        mmioWrite(0x110, i, maxWait = 256)
      }

      mmioWrite(0x210, 0x00000000L, maxWait = 64)
    }
  }
}
