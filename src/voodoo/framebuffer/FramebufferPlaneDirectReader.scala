package voodoo.framebuffer

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo._

case class FramebufferPlaneDirectReader(c: Config) extends Component {
  val io = new Bundle {
    val readReq = slave(Stream(FramebufferPlaneBuffer.ReadReq(c)))
    val readRsp = master(Stream(FramebufferPlaneBuffer.ReadRsp()))
    val mem = master(Bmb(FramebufferPlaneReader.bmbParams(c)))
    val busy = out Bool ()
  }

  val outstanding = RegInit(False)
  val rspInput = Stream(FramebufferPlaneBuffer.ReadRsp())
  val rspQueue = rspInput.queue(1)
  val busyNow = outstanding || rspQueue.valid

  io.readReq.ready := !busyNow && io.mem.cmd.ready

  io.mem.cmd.valid := io.readReq.valid && !busyNow
  io.mem.cmd.fragment.address := alignedWordAddress(io.readReq.address)
  io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
  io.mem.cmd.fragment.length := 3
  io.mem.cmd.fragment.source := 0
  io.mem.cmd.fragment.data := 0
  io.mem.cmd.fragment.mask := 0
  io.mem.cmd.last := True

  when(io.mem.cmd.fire) {
    outstanding := True
  }

  rspInput.valid := io.mem.rsp.valid && outstanding
  rspInput.data := io.mem.rsp.fragment.data
  io.mem.rsp.ready := rspInput.ready && outstanding
  io.readRsp << rspQueue

  when(io.mem.rsp.fire) {
    outstanding := False
  }

  io.busy := busyNow
}
