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

  val laneQueue = StreamFifo(Bool(), 8)
  val rspInput = Stream(FramebufferPlaneBuffer.ReadRsp())
  val rspQueue = rspInput.queue(2)

  io.readReq.ready := io.mem.cmd.ready && laneQueue.io.push.ready

  io.mem.cmd.valid := io.readReq.valid && laneQueue.io.push.ready
  io.mem.cmd.fragment.address := alignedWordAddress(io.readReq.address)
  io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
  io.mem.cmd.fragment.length := 3
  io.mem.cmd.fragment.source := 0
  io.mem.cmd.fragment.data := 0
  io.mem.cmd.fragment.mask := 0
  io.mem.cmd.last := True

  laneQueue.io.push.valid := io.mem.cmd.fire
  laneQueue.io.push.payload := io.readReq.address(1)

  rspInput.valid := io.mem.rsp.valid && laneQueue.io.pop.valid
  rspInput.data := laneQueue.io.pop.payload ? io.mem.rsp.fragment
    .data(31 downto 16) | io.mem.rsp.fragment
    .data(15 downto 0)
  io.mem.rsp.ready := rspInput.ready && laneQueue.io.pop.valid
  laneQueue.io.pop.ready := io.mem.rsp.fire && rspInput.ready
  io.readRsp << rspQueue

  io.busy := laneQueue.io.occupancy =/= 0 || rspQueue.valid
}
