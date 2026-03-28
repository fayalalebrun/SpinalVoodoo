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
case class CoreSim(c: Config, memTiming: SimMemoryTiming = SimMemoryTiming()) extends Component {
  val io = new Bundle {
    // Unified CPU bus (24-bit address covers 16MB PCI BAR)
    val cpuBus = slave(Bmb(Core.cpuBmbParams))

    // Vsync input (driven by C++ harness)
    val vRetrace = in Bool ()

    // Simulation-only framebuffer cache flush request
    val flushFbCaches = in Bool ()
  }

  val core = Core(c)
  core.io.cpuBus <> io.cpuBus

  def simRam(p: BmbParameter, size: Int) = SimBmbMemory(p = p, size = size, timing = memTiming)

  val fbWriteRam = simRam(Core.fbMemBmbParams(c), 4 * 1024 * 1024)
  val texRam = simRam(Core.texMemBmbParams(c), 8 * 1024 * 1024)

  val fbArbiter = BmbArbiter(
    inputsParameter = Seq.fill(3)(Core.fbMemBmbParams(c)),
    outputParameter = Core.fbMemBmbParams(c),
    lowerFirstPriority = true
  )

  fbArbiter.io.inputs(0) <> core.io.fbMemWrite
  fbArbiter.io.inputs(1) <> core.io.fbColorReadMem
  fbArbiter.io.inputs(2) <> core.io.fbAuxReadMem
  fbArbiter.io.output <> fbWriteRam.io.bus
  core.io.texMem <> texRam.io.bus

  core.io.statusInputs.vRetrace := io.vRetrace
  core.io.statusInputs.memFifoFree := 0xffff
  core.io.statusInputs.pciInterrupt := False

  core.io.fbBaseAddr := 0
  core.io.flushFbCaches := io.flushFbCaches
}

object CoreSim {
  val cpuBmbParams = Core.cpuBmbParams
}
