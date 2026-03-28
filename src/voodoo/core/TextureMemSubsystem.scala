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

  val texArbiter = BmbArbiter(
    inputsParameter =
      Seq(Tmu.bmbParams(c), voodoo.Core.cpuTexBmbParams, voodoo.Core.cpuTexBmbParams),
    outputParameter = voodoo.Core.texMemBmbParams(c),
    lowerFirstPriority = true
  )

  texArbiter.io.inputs(0).cmd << io.tmuTexRead.cmd.s2mPipe()
  texArbiter.io.inputs(0).rsp >> io.tmuTexRead.rsp
  texArbiter.io.inputs(1) <> cpuTexWriteBus
  texArbiter.io.inputs(2) <> io.cpuTexRead
  texArbiter.io.output <> io.texMem
}
