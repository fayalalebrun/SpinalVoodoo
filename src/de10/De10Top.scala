package voodoo.de10

import spinal.core._
import spinal.lib._
import spinal.lib.bus.avalon._
import voodoo.{Config, Core}

case class De10Top(c: Config) extends Component {
  private val cpuAddressWidth = Core.cpuBmbParams.access.addressWidth

  val io = new Bundle {
    val h2fLw = slave(H2fLwMmio(cpuAddressWidth - 2))
    val memFb = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
    val memTex = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
  }

  val core = CoreDe10(c)

  core.io.h2fLw <> io.h2fLw
  io.memFb <> core.io.memFb
  io.memTex <> core.io.memTex
}
