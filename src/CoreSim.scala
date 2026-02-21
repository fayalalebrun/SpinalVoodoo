package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._

/** CoreSim: Simulation wrapper for Core with embedded BRAM and unified CPU bus.
  *
  * Wraps Core(Config.voodoo1()) with:
  *   - Embedded BRAM for framebuffer (4MB) and texture memory (8MB)
  *   - Unified 24-bit CPU bus matching PCI BAR layout: 0x000000-0x3FFFFF = registers (22-bit
  *     sub-addr) 0x400000-0x7FFFFF = LFB (22-bit sub-addr) 0x800000-0xFFFFFF = texture BRAM (23-bit
  *     sub-addr)
  *   - Status outputs for fast idle polling
  */
case class CoreSim(c: Config) extends Component {
  val io = new Bundle {
    // Unified CPU bus (24-bit address covers 16MB PCI BAR)
    val cpuBus = slave(Bmb(CoreSim.cpuBmbParams))

    // Status outputs for fast polling
    val pipelineBusy = out Bool ()
    val fifoEmpty = out Bool ()

    // Vsync input (driven by C++ harness)
    val vRetrace = in Bool ()
  }

  // ========================================================================
  // Instantiate Core
  // ========================================================================
  val core = Core(c)

  // ========================================================================
  // Framebuffer: BmbArbiter + BmbOnChipRam (4MB)
  // ========================================================================
  val fbArbiterOutputParam = BmbParameter(
    addressWidth = 26,
    dataWidth = 32,
    sourceWidth = 1 + log2Up(3), // 3 inputs: 2 route bits + 1 source bit = 3
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  val fbArbiter = BmbArbiter(
    inputsParameter = Seq(
      Write.baseBmbParams(c),
      FramebufferAccess.bmbParams(c),
      Lfb.fbReadBmbParams(c)
    ),
    outputParameter = fbArbiterOutputParam,
    lowerFirstPriority = true // fbWrite gets priority
  )

  val fbRam = BmbOnChipRam(p = fbArbiterOutputParam, size = 4 * 1024 * 1024)

  // Pipe fbWrite cmd.ready to break combinational loop: Core's fbAccess has
  // a combinational path from fbWrite.cmd.ready -> fbRead.rsp.ready, which
  // loops back through the arbiter and BmbOnChipRam's !rsp.isStall gating.
  fbArbiter.io.inputs(0).cmd << core.io.fbWrite.cmd.s2mPipe()
  fbArbiter.io.inputs(0).rsp >> core.io.fbWrite.rsp

  // Buffer fbRead response to prevent deadlock: FramebufferAccess does
  // read-then-write (fbRead -> pipeline -> fbWrite). With single-ported RAM,
  // the read response blocks the RAM until consumed, but consumption requires
  // the write to complete (backpressure through the pipeline). The s2mPipe
  // decouples the response handshake, freeing the RAM for the write.
  fbArbiter.io.inputs(1).cmd << core.io.fbRead.cmd
  core.io.fbRead.rsp << fbArbiter.io.inputs(1).rsp.s2mPipe()

  fbArbiter.io.inputs(2) <> core.io.lfbFbRead
  fbArbiter.io.output <> fbRam.io.bus

  // ========================================================================
  // Texture: BmbArbiter + BmbOnChipRam (8MB)
  // ========================================================================
  val cpuTexBmbParam = BmbParameter(
    addressWidth = 26,
    dataWidth = 32,
    sourceWidth = 4, // matches cpuBus
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  val texArbiterOutputParam = BmbParameter(
    addressWidth = 26,
    dataWidth = 32,
    sourceWidth = 4 + log2Up(2), // 2 inputs: 1 route bit + 4 source bits = 5
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  val cpuTexBus = Bmb(cpuTexBmbParam)

  val texArbiter = BmbArbiter(
    inputsParameter = Seq(
      Tmu.bmbParams(c),
      cpuTexBmbParam
    ),
    outputParameter = texArbiterOutputParam,
    lowerFirstPriority = true // texRead gets priority over CPU writes
  )

  val texRam = BmbOnChipRam(p = texArbiterOutputParam, size = 8 * 1024 * 1024)

  // Pipe texRead cmd.ready to break combinational loop: TMU's StreamFork has
  // a combinational path from texRead.cmd.ready -> texRead.cmd.valid, which
  // loops through BmbArbiter's StreamArbiter mask logic (valid -> ready).
  texArbiter.io.inputs(0).cmd << core.io.texRead.cmd.s2mPipe()
  texArbiter.io.inputs(0).rsp >> core.io.texRead.rsp

  texArbiter.io.inputs(1) <> cpuTexBus
  texArbiter.io.output <> texRam.io.bus

  // ========================================================================
  // Status wiring
  // ========================================================================
  core.io.statusInputs.sstBusy := core.io.pipelineBusy
  core.io.statusInputs.fbiBusy := core.io.pipelineBusy
  core.io.statusInputs.trexBusy := core.io.pipelineBusy
  core.io.statusInputs.vRetrace := io.vRetrace
  core.io.statusInputs.memFifoFree := 0xffff
  core.io.statusInputs.pciInterrupt := False

  core.io.statisticsIn.pixelsIn := 0
  core.io.statisticsIn.chromaFail := 0
  core.io.statisticsIn.zFuncFail := 0
  core.io.statisticsIn.aFuncFail := 0
  core.io.statisticsIn.pixelsOut := 0

  core.io.fbBaseAddr := 0

  // ========================================================================
  // Status outputs
  // ========================================================================
  io.pipelineBusy := core.io.pipelineBusy
  io.fifoEmpty := core.io.fifoEmpty

  // ========================================================================
  // Unified CPU bus: route to regBus, lfbBus, or cpuTexBus based on address
  // ========================================================================
  // Address decode: bits [23:22]
  //   00 = registers (0x000000-0x3FFFFF)
  //   01 = LFB       (0x400000-0x7FFFFF)
  //   10 = texture    (0x800000-0xBFFFFF)
  //   11 = texture    (0xC00000-0xFFFFFF)

  val cpuCmd = io.cpuBus.cmd
  val cpuRsp = io.cpuBus.rsp

  val target = cpuCmd.address(23 downto 22)

  // Register which target was selected (latched on cmd.fire for response routing)
  val rspTarget = RegNextWhen(target, cpuCmd.fire) init (0)

  // Response mux
  val regRspValid = core.io.regBus.rsp.valid
  val lfbRspValid = core.io.lfbBus.rsp.valid

  // CPU bus cmd routing
  val isReg = target === 0
  val isLfb = target === 1
  val isTex = target >= 2

  // Drive regBus cmd
  core.io.regBus.cmd.valid := cpuCmd.valid && isReg
  core.io.regBus.cmd.opcode := cpuCmd.opcode
  core.io.regBus.cmd.address := cpuCmd.address.resize(22 bits)
  core.io.regBus.cmd.data := cpuCmd.data
  core.io.regBus.cmd.mask := cpuCmd.mask
  core.io.regBus.cmd.length := cpuCmd.length
  core.io.regBus.cmd.last := cpuCmd.last
  core.io.regBus.cmd.source := cpuCmd.source.resize(core.io.regBus.p.access.sourceWidth bits)

  // Drive lfbBus cmd
  core.io.lfbBus.cmd.valid := cpuCmd.valid && isLfb
  core.io.lfbBus.cmd.opcode := cpuCmd.opcode
  core.io.lfbBus.cmd.address := cpuCmd.address.resize(22 bits)
  core.io.lfbBus.cmd.data := cpuCmd.data
  core.io.lfbBus.cmd.mask := cpuCmd.mask
  core.io.lfbBus.cmd.length := cpuCmd.length
  core.io.lfbBus.cmd.last := cpuCmd.last
  core.io.lfbBus.cmd.source := cpuCmd.source.resize(core.io.lfbBus.p.access.sourceWidth bits)

  // Drive cpuTexBus cmd (forwarded through BmbArbiter to texture RAM)
  cpuTexBus.cmd.valid := cpuCmd.valid && isTex
  cpuTexBus.cmd.address := cpuCmd.address.resized // 24-bit -> 26-bit zero-extend
  cpuTexBus.cmd.opcode := cpuCmd.opcode
  cpuTexBus.cmd.data := cpuCmd.data
  cpuTexBus.cmd.mask := cpuCmd.mask
  cpuTexBus.cmd.length := cpuCmd.length
  cpuTexBus.cmd.last := cpuCmd.last
  cpuTexBus.cmd.source := cpuCmd.source

  // Mux ready back from selected target
  cpuCmd.ready := (isReg && core.io.regBus.cmd.ready) ||
    (isLfb && core.io.lfbBus.cmd.ready) ||
    (isTex && cpuTexBus.cmd.ready)

  // Response mux: route response from selected target
  cpuRsp.valid := False
  cpuRsp.data := 0
  cpuRsp.last := True
  cpuRsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  cpuRsp.source := 0

  // Always accept responses from sub-buses
  core.io.regBus.rsp.ready := (rspTarget === 0) && cpuRsp.ready
  core.io.lfbBus.rsp.ready := (rspTarget === 1) && cpuRsp.ready
  cpuTexBus.rsp.ready := (rspTarget >= 2) && cpuRsp.ready

  when(rspTarget === 0) {
    cpuRsp.valid := regRspValid
    cpuRsp.data := core.io.regBus.rsp.data
    cpuRsp.opcode := core.io.regBus.rsp.opcode
    cpuRsp.source := core.io.regBus.rsp.source.resize(cpuRsp.p.access.sourceWidth bits)
  } elsewhen (rspTarget === 1) {
    cpuRsp.valid := lfbRspValid
    cpuRsp.data := core.io.lfbBus.rsp.data
    cpuRsp.opcode := core.io.lfbBus.rsp.opcode
    cpuRsp.source := core.io.lfbBus.rsp.source.resize(cpuRsp.p.access.sourceWidth bits)
  } otherwise {
    cpuRsp.valid := cpuTexBus.rsp.valid
    cpuRsp.data := cpuTexBus.rsp.data
    cpuRsp.opcode := cpuTexBus.rsp.opcode
    cpuRsp.source := cpuTexBus.rsp.source.resize(cpuRsp.p.access.sourceWidth bits)
  }
}

object CoreSim {
  val cpuBmbParams = BmbParameter(
    addressWidth = 24, // 16MB PCI BAR
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )
}
