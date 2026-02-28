package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._

/** CoreSim: Simulation wrapper for Core with embedded BRAM and unified CPU bus.
  *
  * Wraps Core(Config.voodoo1()) with:
  *   - Embedded BRAM for framebuffer (4MB) and texture memory (8MB)
  *   - Unified 24-bit CPU bus matching PCI BAR layout (address decode inside Core)
  *   - Status outputs for fast idle polling
  */
case class CoreSim(c: Config) extends Component {
  val io = new Bundle {
    // Unified CPU bus (24-bit address covers 16MB PCI BAR)
    val cpuBus = slave(Bmb(Core.cpuBmbParams))

    // Vsync input (driven by C++ harness)
    val vRetrace = in Bool ()
  }

  val core = Core(c)
  core.io.cpuBus <> io.cpuBus

  val fbRam = BmbOnChipRam(p = Core.fbMemBmbParams(c), size = 4 * 1024 * 1024)
  core.io.fbMem <> fbRam.io.bus

  val texRam = BmbOnChipRam(p = Core.texMemBmbParams(c), size = 8 * 1024 * 1024)
  core.io.texMem <> texRam.io.bus

  core.io.statusInputs.vRetrace := io.vRetrace
  core.io.statusInputs.memFifoFree := 0xffff
  core.io.statusInputs.pciInterrupt := False

  core.io.statisticsIn.pixelsIn := 0
  core.io.statisticsIn.chromaFail := 0
  core.io.statisticsIn.zFuncFail := 0
  core.io.statisticsIn.aFuncFail := 0
  core.io.statisticsIn.pixelsOut := 0

  core.io.fbBaseAddr := 0
}

object CoreSim {
  val cpuBmbParams = Core.cpuBmbParams
}
