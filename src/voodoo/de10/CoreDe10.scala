package voodoo.de10

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.bus.avalon._
import spinal.lib.bus.bmb._
import voodoo.{Config, Core}

/** DE10-oriented wrapper around Core.
  *
  * The MMIO bundle name is kept for compatibility with the host-sim harness, but the board path
  * uses the full HPS-to-FPGA aperture while framebuffer and texture traffic still exit through the
  * FPGA-to-SDRAM Avalon-MM ports.
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

case class H2fLwToBmbBridge(addressWidth: Int, bmbParams: BmbParameter) extends Component {
  val io = new Bundle {
    val h2fLw = slave(H2fLwMmio(addressWidth - 2))
    val cpuBus = master(Bmb(bmbParams))
  }

  private val stallLimitLog2 = 20
  private val stalledWriteLimit = 1 << stallLimitLog2
  private val wedgedStatusAddr = U(0x254, addressWidth bits)

  val cmdAddress = (io.h2fLw.address.resize(addressWidth) |<< 2)
  val writeReq = io.h2fLw.write
  val readReq = io.h2fLw.read
  val illegalReq = writeReq && readReq
  val selectedReadReq = !writeReq && readReq
  val hasReq = writeReq || readReq
  val statusReadReq = selectedReadReq && cmdAddress === wedgedStatusAddr

  io.cpuBus.cmd.last := True
  io.cpuBus.cmd.source := 0
  io.cpuBus.cmd.opcode := Bmb.Cmd.Opcode.READ
  io.cpuBus.cmd.address := cmdAddress
  io.cpuBus.cmd.length := 3
  io.cpuBus.cmd.data := io.h2fLw.writedata
  io.cpuBus.cmd.mask := io.h2fLw.byteenable
  io.cpuBus.rsp.ready := True

  val cmdInFlight = RegInit(False)
  val readRspPending = RegInit(False)
  val readData = Reg(Bits(32 bits)) init (0)
  val readDataValid = RegInit(False)
  val readDataPending = RegInit(False)
  val wedged = RegInit(False)
  val stalledWriteCycles = Reg(UInt((stallLimitLog2 + 1) bits)) init (0)
  val wedgedWriteStallCycles = Reg(UInt((stallLimitLog2 + 1) bits)) init (0)
  val droppedWrites = Reg(UInt(8 bits)) init (0)

  val statusReadData = B(32 bits, default -> False)
  statusReadData(0) := wedged
  statusReadData(8, 8 bits) := droppedWrites.asBits
  statusReadData(16, 16 bits) := wedgedWriteStallCycles(15 downto 0).asBits

  val cmdBlocked = cmdInFlight || readRspPending || readDataPending || readDataValid
  val dropWriteReq = writeReq && wedged
  val locallyHandledReq = statusReadReq || dropWriteReq
  val canIssueReq = hasReq && !illegalReq && !cmdBlocked && !locallyHandledReq
  val acceptWindow = canIssueReq && io.cpuBus.cmd.ready
  val localAccept = hasReq && !illegalReq && !cmdBlocked && locallyHandledReq
  val rspCompletesCurrent = io.cpuBus.rsp.valid && (cmdInFlight || acceptWindow)
  val rspIsRead = (cmdInFlight && readRspPending) || acceptWindow && selectedReadReq
  val stalledWrite =
    writeReq && !illegalReq && !wedged && !localAccept && (cmdBlocked || !io.cpuBus.cmd.ready)
  val stallWouldTimeout = stalledWrite && stalledWriteCycles === U(
    stalledWriteLimit - 1,
    stalledWriteCycles.getWidth bits
  )

  io.h2fLw.waitrequest := hasReq && !(localAccept || stallWouldTimeout) && (illegalReq || cmdBlocked || !io.cpuBus.cmd.ready)
  io.h2fLw.readdata := readData
  io.h2fLw.readdatavalid := readDataValid

  readDataValid := False

  when(stalledWrite) {
    when(!stallWouldTimeout) {
      stalledWriteCycles := stalledWriteCycles + 1
    }
  } otherwise {
    stalledWriteCycles := 0
  }

  when(stallWouldTimeout) {
    wedged := True
    wedgedWriteStallCycles := U(stalledWriteLimit, wedgedWriteStallCycles.getWidth bits)
    when(droppedWrites =/= droppedWrites.maxValue) {
      droppedWrites := droppedWrites + 1
    }
  }

  io.cpuBus.cmd.valid := canIssueReq
  when(writeReq) {
    io.cpuBus.cmd.opcode := Bmb.Cmd.Opcode.WRITE
  } otherwise {
    io.cpuBus.cmd.opcode := Bmb.Cmd.Opcode.READ
  }

  when(localAccept && statusReadReq) {
    readData := statusReadData
    readDataPending := True
  }

  when(localAccept && dropWriteReq) {
    when(droppedWrites =/= droppedWrites.maxValue) {
      droppedWrites := droppedWrites + 1
    }
  }

  when(acceptWindow && !rspCompletesCurrent) {
    cmdInFlight := True
    readRspPending := selectedReadReq
  }

  when(rspCompletesCurrent) {
    cmdInFlight := False
    when(rspIsRead) {
      readData := io.cpuBus.rsp.data
      readDataPending := True
    }
    readRspPending := False
  }

  when(readDataPending) {
    readDataPending := False
    readDataValid := True
  }

  GenerationFlags.formal {
    when(acceptWindow) {
      assert(io.cpuBus.cmd.valid)
      assert(io.cpuBus.cmd.address === cmdAddress)
      assert(io.cpuBus.cmd.length === 3)
      assert(io.cpuBus.cmd.mask === io.h2fLw.byteenable)
      assert(io.cpuBus.cmd.data === io.h2fLw.writedata)
      assert(io.cpuBus.cmd.source === 0)
      assert(io.cpuBus.cmd.last)
      when(writeReq) {
        assert(io.cpuBus.cmd.opcode === Bmb.Cmd.Opcode.WRITE)
      } otherwise {
        assert(io.cpuBus.cmd.opcode === Bmb.Cmd.Opcode.READ)
      }
    }

    when(illegalReq) {
      assert(io.h2fLw.waitrequest)
      assert(!io.cpuBus.cmd.valid)
    }

    when(cmdBlocked && hasReq && !locallyHandledReq && !stallWouldTimeout) {
      assert(io.h2fLw.waitrequest)
    }

    when(canIssueReq && !io.cpuBus.cmd.ready && !stallWouldTimeout) {
      assert(io.h2fLw.waitrequest)
      assert(io.cpuBus.cmd.valid)
    }

    when(localAccept && statusReadReq) {
      assert(!io.h2fLw.waitrequest)
      assert(!io.cpuBus.cmd.valid)
    }

    when(localAccept && dropWriteReq) {
      assert(!io.h2fLw.waitrequest)
      assert(!io.cpuBus.cmd.valid)
    }

    when(acceptWindow && selectedReadReq && io.cpuBus.rsp.valid) {
      assert(!cmdInFlight)
      assert(!readRspPending)
    }

    when(rspCompletesCurrent) {
      assert(io.cpuBus.rsp.last)
      assert(io.cpuBus.rsp.source === 0)
    }

    when(readDataValid) {
      assert(io.h2fLw.readdatavalid)
      assert(io.h2fLw.readdata === readData)
    }

    cover(io.cpuBus.cmd.fire && io.cpuBus.cmd.opcode === Bmb.Cmd.Opcode.WRITE)
    cover(io.cpuBus.cmd.fire && io.cpuBus.cmd.opcode === Bmb.Cmd.Opcode.READ)
    cover(acceptWindow && io.cpuBus.rsp.valid && selectedReadReq)
  }
}

case class CoreDe10(c: Config) extends Component {
  private val cpuAddressWidth = Core.cpuBmbParams.access.addressWidth

  val io = new Bundle {
    val h2fLw = slave(H2fLwMmio(cpuAddressWidth - 2))
    val memFbWrite = master(
      AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth))
    )
    val memFbColorWrite = master(
      AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth))
    )
    val memFbAuxWrite = master(
      AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth))
    )
    val memFbColorRead = master(
      AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth))
    )
    val memFbAuxRead = master(
      AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth))
    )
    val memTex = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
  }

  val core = Core(c)
  val memBackend = De10MemBackend(c)
  val h2fBridge = H2fLwToBmbBridge(cpuAddressWidth, Core.cpuBmbParams)

  io.h2fLw <> h2fBridge.io.h2fLw
  core.io.cpuBus <> h2fBridge.io.cpuBus

  Seq(
    core.io.fbMemWrite <> memBackend.io.fbMemWrite,
    core.io.fbColorWriteMem <> memBackend.io.fbColorWriteMem,
    core.io.fbAuxWriteMem <> memBackend.io.fbAuxWriteMem,
    core.io.fbColorReadMem <> memBackend.io.fbColorReadMem,
    core.io.fbAuxReadMem <> memBackend.io.fbAuxReadMem,
    core.io.texMem <> memBackend.io.texMem,
    io.memFbWrite <> memBackend.io.memFbWrite,
    io.memFbColorWrite <> memBackend.io.memFbColorWrite,
    io.memFbAuxWrite <> memBackend.io.memFbAuxWrite,
    io.memFbColorRead <> memBackend.io.memFbColorRead,
    io.memFbAuxRead <> memBackend.io.memFbAuxRead,
    io.memTex <> memBackend.io.memTex
  )

  // The DE10 wrapper does not yet model display timing, but swapbufferCMD and
  // retrace polling need forward progress. Provide a simple periodic retrace pulse
  // until the real video path drives this input.
  val vRetraceCounter = Reg(UInt(12 bits)) init (0)
  vRetraceCounter := vRetraceCounter + 1
  core.io.statusInputs.vRetrace := vRetraceCounter === 0
  core.io.statusInputs.memFifoFree := 0xffff
  core.io.statusInputs.pciInterrupt := False

  core.io.fbBaseAddr := 0
  core.io.flushFbCaches := False
}
