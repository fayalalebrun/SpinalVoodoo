package voodoo.core

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo._
import voodoo.bus.TexWritePayload
import voodoo.texture.TextureMem

case class TextureMemSubsystem(c: Config) extends Component {
  val io = new Bundle {
    val cpuTexDrain = slave(Stream(TexWritePayload()))
    val cpuTexRead = slave(Bmb(voodoo.Core.cpuTexBmbParams))
    val tmuTexRead = slave(Bmb(Tmu.bmbParams(c)))
    val downloadConfig = in(TextureMem.DownloadConfig(c))
    val texMem = master(Bmb(voodoo.Core.texMemBmbParams(c)))
  }

  val cpuTexWriteCmd = TextureMem.WriteCmd.fromPciDrain(c, io.cpuTexDrain, io.downloadConfig)
  val cpuTexWriteBus =
    TextureMem.WriteCmd.toBmb(cpuTexWriteCmd, voodoo.Core.cpuTexBmbParams, source = 0)

  private val tmuTexCmd = io.tmuTexRead.cmd.s2mPipe()
  private val cmdRouteWidth = log2Up(3)
  private val texMemSourceWidth = voodoo.Core.texMemBmbParams(c).access.sourceWidth

  private def routedSource(cmd: Stream[Fragment[BmbCmd]], route: Int): UInt =
    (cmd.source
      .resize(texMemSourceWidth - cmdRouteWidth bits) ## U(route, cmdRouteWidth bits)).asUInt

  private val useTmuTex = tmuTexCmd.valid
  private val useCpuTexWrite = !useTmuTex && cpuTexWriteBus.cmd.valid
  private val useCpuTexRead = !useTmuTex && !useCpuTexWrite && io.cpuTexRead.cmd.valid

  io.texMem.cmd.valid := False
  io.texMem.cmd.opcode := tmuTexCmd.opcode
  io.texMem.cmd.address := tmuTexCmd.address.resize(io.texMem.p.access.addressWidth bits)
  io.texMem.cmd.data := 0
  io.texMem.cmd.mask := 0
  io.texMem.cmd.length := tmuTexCmd.length.resize(io.texMem.p.access.lengthWidth bits)
  io.texMem.cmd.last := tmuTexCmd.last
  io.texMem.cmd.source := routedSource(tmuTexCmd, 0)
  if (io.texMem.p.access.contextWidth > 0) {
    io.texMem.cmd.context := 0
  }

  when(useTmuTex) {
    io.texMem.cmd.valid := True
  }
  when(useCpuTexWrite) {
    io.texMem.cmd.valid := True
    io.texMem.cmd.opcode := cpuTexWriteBus.cmd.opcode
    io.texMem.cmd.address := cpuTexWriteBus.cmd.address.resize(io.texMem.p.access.addressWidth bits)
    io.texMem.cmd.data := cpuTexWriteBus.cmd.data
    io.texMem.cmd.mask := cpuTexWriteBus.cmd.mask
    io.texMem.cmd.length := cpuTexWriteBus.cmd.length.resize(io.texMem.p.access.lengthWidth bits)
    io.texMem.cmd.last := cpuTexWriteBus.cmd.last
    io.texMem.cmd.source := routedSource(cpuTexWriteBus.cmd, 1)
    if (io.texMem.p.access.contextWidth > 0) {
      io.texMem.cmd.context := cpuTexWriteBus.cmd.context.resize(
        io.texMem.p.access.contextWidth bits
      )
    }
  }
  when(useCpuTexRead) {
    io.texMem.cmd.valid := True
    io.texMem.cmd.opcode := io.cpuTexRead.cmd.opcode
    io.texMem.cmd.address := io.cpuTexRead.cmd.address.resize(io.texMem.p.access.addressWidth bits)
    io.texMem.cmd.data := 0
    io.texMem.cmd.mask := 0
    io.texMem.cmd.length := io.cpuTexRead.cmd.length.resize(io.texMem.p.access.lengthWidth bits)
    io.texMem.cmd.last := io.cpuTexRead.cmd.last
    io.texMem.cmd.source := routedSource(io.cpuTexRead.cmd, 2)
    if (io.texMem.p.access.contextWidth > 0) {
      io.texMem.cmd.context := io.cpuTexRead.cmd.context
        .resize(io.texMem.p.access.contextWidth bits)
    }
  }

  tmuTexCmd.ready := useTmuTex && io.texMem.cmd.ready
  cpuTexWriteBus.cmd.ready := useCpuTexWrite && io.texMem.cmd.ready
  io.cpuTexRead.cmd.ready := useCpuTexRead && io.texMem.cmd.ready

  val rspRoute = io.texMem.rsp.source(cmdRouteWidth - 1 downto 0)
  val rspSource = (io.texMem.rsp.source >> cmdRouteWidth).resized

  io.tmuTexRead.rsp.valid := False
  io.tmuTexRead.rsp.last := io.texMem.rsp.last
  io.tmuTexRead.rsp.opcode := io.texMem.rsp.opcode
  io.tmuTexRead.rsp.data := io.texMem.rsp.data
  io.tmuTexRead.rsp.source := rspSource.resize(io.tmuTexRead.p.access.sourceWidth bits)

  cpuTexWriteBus.rsp.valid := False
  cpuTexWriteBus.rsp.last := io.texMem.rsp.last
  cpuTexWriteBus.rsp.opcode := io.texMem.rsp.opcode
  cpuTexWriteBus.rsp.data := io.texMem.rsp.data
  cpuTexWriteBus.rsp.source := rspSource.resize(cpuTexWriteBus.p.access.sourceWidth bits)

  io.cpuTexRead.rsp.valid := False
  io.cpuTexRead.rsp.last := io.texMem.rsp.last
  io.cpuTexRead.rsp.opcode := io.texMem.rsp.opcode
  io.cpuTexRead.rsp.data := io.texMem.rsp.data
  io.cpuTexRead.rsp.source := rspSource.resize(io.cpuTexRead.p.access.sourceWidth bits)

  io.texMem.rsp.ready := False
  when(io.texMem.rsp.valid && rspRoute === U(0, cmdRouteWidth bits)) {
    io.tmuTexRead.rsp.valid := True
    io.texMem.rsp.ready := io.tmuTexRead.rsp.ready
  } elsewhen (io.texMem.rsp.valid && rspRoute === U(1, cmdRouteWidth bits)) {
    cpuTexWriteBus.rsp.valid := True
    io.texMem.rsp.ready := cpuTexWriteBus.rsp.ready
  } elsewhen (io.texMem.rsp.valid && rspRoute === U(2, cmdRouteWidth bits)) {
    io.cpuTexRead.rsp.valid := True
    io.texMem.rsp.ready := io.cpuTexRead.rsp.ready
  }
}
