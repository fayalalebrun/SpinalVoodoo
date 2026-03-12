package voodoo.de10

import spinal.core._
import spinal.lib._
import spinal.lib.bus.avalon._
import spinal.lib.bus.bmb._
import voodoo.{Config, Core}

/** Initial DE10-oriented wrapper around Core.
  *
  * This is a bring-up scaffold, not the final board integration. It keeps the HPS
  * lightweight-facing MMIO interface and exports Avalon-MM style memory ports for framebuffer and
  * texture traffic so we can generate standalone RTL for the Quartus flow while full Platform
  * Designer integration is being built.
  */
case class H2fLwMmio(addressWidth: Int) extends Bundle with IMasterSlave {
  val address = UInt(addressWidth bits)
  val read = Bool()
  val write = Bool()
  val byteenable = Bits(4 bits)
  val writedata = Bits(32 bits)
  val waitrequest = Bool()
  val readdata = Bits(32 bits)
  val readdatavalid = Bool()

  override def asMaster(): Unit = {
    out(address, read, write, byteenable, writedata)
    in(waitrequest, readdata, readdatavalid)
  }
}

case class CoreDe10(c: Config) extends Component {
  private val cpuAddressWidth = Core.cpuBmbParams.access.addressWidth

  val io = new Bundle {
    val h2fLw = slave(H2fLwMmio(cpuAddressWidth - 2))
    val memFb = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
    val memTex = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
  }

  val core = Core(c)
  val memBackend = De10MemBackend(c)

  core.io.cpuBus.cmd.valid := False
  core.io.cpuBus.cmd.last := True
  core.io.cpuBus.cmd.source := 0
  core.io.cpuBus.cmd.opcode := Bmb.Cmd.Opcode.READ
  core.io.cpuBus.cmd.address := (io.h2fLw.address.resize(cpuAddressWidth) |<< 2)
  core.io.cpuBus.cmd.length := 3
  core.io.cpuBus.cmd.data := io.h2fLw.writedata
  core.io.cpuBus.cmd.mask := io.h2fLw.byteenable
  core.io.cpuBus.rsp.ready := True

  val cmdInFlight = RegInit(False)
  val readRspPending = RegInit(False)
  val readData = Reg(Bits(32 bits)) init (0)
  val readDataValid = RegInit(False)
  val readDataPending = RegInit(False)
  val reqSeen = RegInit(False)

  val writeReq = io.h2fLw.write
  val readReq = io.h2fLw.read
  val selectedReadReq = !writeReq && readReq
  val hasReq = writeReq || readReq
  val cmdBlocked = cmdInFlight || readRspPending || readDataPending || readDataValid
  val acceptWindow = hasReq && !reqSeen && !cmdBlocked

  io.h2fLw.waitrequest := hasReq && (!acceptWindow || !core.io.cpuBus.cmd.ready)
  io.h2fLw.readdata := readData
  io.h2fLw.readdatavalid := readDataValid

  readDataValid := False
  when(!hasReq) {
    reqSeen := False
  }

  when(acceptWindow) {
    core.io.cpuBus.cmd.valid := True
    when(writeReq) {
      core.io.cpuBus.cmd.opcode := Bmb.Cmd.Opcode.WRITE
    } otherwise {
      core.io.cpuBus.cmd.opcode := Bmb.Cmd.Opcode.READ
    }
  }

  when(core.io.cpuBus.cmd.fire) {
    cmdInFlight := True
    readRspPending := selectedReadReq
    reqSeen := True
  }

  when(cmdInFlight && core.io.cpuBus.rsp.valid) {
    cmdInFlight := False
    when(readRspPending) {
      readData := core.io.cpuBus.rsp.data
      readDataPending := True
    }
    readRspPending := False
  }

  when(readDataPending) {
    readDataPending := False
    readDataValid := True
  }

  core.io.fbMem <> memBackend.io.fbMem
  core.io.texMem <> memBackend.io.texMem
  io.memFb <> memBackend.io.memFb
  io.memTex <> memBackend.io.memTex

  // The DE10 wrapper does not yet model display timing, but swapbufferCMD and
  // retrace polling need forward progress. Provide a simple periodic retrace pulse
  // until the real video path drives this input.
  val vRetraceCounter = Reg(UInt(12 bits)) init (0)
  vRetraceCounter := vRetraceCounter + 1
  core.io.statusInputs.vRetrace := vRetraceCounter === 0
  core.io.statusInputs.memFifoFree := 0xffff
  core.io.statusInputs.pciInterrupt := False

  core.io.fbBaseAddr := 0
}
