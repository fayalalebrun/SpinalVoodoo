package voodoo.mister

import spinal.core._
import spinal.lib._

case class MisterResetBoundary() extends Component {
  val io = new Bundle {
    val request = in Bool ()
    val core = slave(MisterDdrPort())
    val ddr = master(MisterDdrPort())
    val coreReset = out Bool ()
  }
  val request = BufferCC(io.request, True)
  val pending = RegInit(True)
  val resetting = RegInit(True)
  val hold = Reg(UInt(4 bits)) init (15)
  val stalled = RegInit(False)
  val remaining = Reg(UInt(11 bits)) init (0)
  val allow = !resetting && (!(pending || request) || stalled)
  io.ddr.address := io.core.address
  io.ddr.burstCount := io.core.burstCount
  io.ddr.writeData := io.core.writeData
  io.ddr.byteEnable := io.core.byteEnable
  io.ddr.read := io.core.read && allow
  io.ddr.write := io.core.write && allow
  io.core.busy := io.ddr.busy || !allow
  io.core.readData := io.ddr.readData
  io.core.readDataValid := io.ddr.readDataValid && !resetting
  io.coreReset := resetting
  val read = io.ddr.read && !io.ddr.busy
  stalled := (io.ddr.read || io.ddr.write) && io.ddr.busy
  when(read || io.ddr.readDataValid) {
    remaining := remaining + Mux(read, io.ddr.burstCount.resize(11), U(0, 11 bits)) -
      io.ddr.readDataValid.asUInt
  }
  when(resetting) {
    when(hold =/= 0) { hold := hold - 1 }
    when(!request && hold === 0) {
      resetting := False
      pending := False
    }
  } otherwise {
    when(request) { pending := True }
    when((pending || request) && remaining === 0 && !stalled) {
      resetting := True
      hold := 15
    }
  }
  when(read) {
    assert(remaining <= 1024 - 128)
    assert(io.ddr.burstCount =/= 0 && io.ddr.burstCount <= 128)
  }
  when(io.ddr.write && !io.ddr.busy) { assert(io.ddr.burstCount === 1) }
  when(io.ddr.readDataValid) { assert(remaining =/= 0 || read) }
}
