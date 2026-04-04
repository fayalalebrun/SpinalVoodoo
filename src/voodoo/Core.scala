package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo.core.{
  CoreCpuFrontdoor,
  FramebufferLayout,
  FramebufferMemSubsystem,
  PixelPipeline,
  TextureMemSubsystem
}
import voodoo.texture.TextureMem

object Core {
  val cpuBmbParams = BmbParameter(
    addressWidth = 24,
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )

  def fbMemBmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1 + log2Up(5), // 5 arbiter inputs
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  def texMemBmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 4 + log2Up(3), // max(srcW=1,4) + 2 route bits for 3 inputs = 6
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

  // Internal CPU texture write bus params
  val cpuTexBmbParams = BmbParameter(
    addressWidth = 26,
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 6,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}

case class Core(c: Config) extends Component {
  val io = new Bundle {
    // Unified CPU bus (24-bit address covers 16MB PCI BAR)
    val cpuBus = slave(Bmb(Core.cpuBmbParams))

    // Framebuffer memory buses
    val fbMemWrite = master(Bmb(Core.fbMemBmbParams(c)))
    val fbColorReadMem = master(Bmb(Core.fbMemBmbParams(c)))
    val fbAuxReadMem = master(Bmb(Core.fbMemBmbParams(c)))

    // Texture memory bus (R/W, internal 2-port arbitration with texBaseAddr relocation)
    val texMem = master(Bmb(Core.texMemBmbParams(c)))

    // Status inputs (hardware state)
    val statusInputs = in(new Bundle {
      val vRetrace = Bool()
      val memFifoFree = UInt(16 bits)
      val pciInterrupt = Bool()
    })

    // SwapBuffer outputs (active buffer index and pending swap count)
    val swapDisplayedBuffer = out UInt (2 bits)
    val swapsPending = out UInt (3 bits)

    // Framebuffer base address
    val fbBaseAddr = in UInt (c.addressWidth)

    // Simulation-only framebuffer cache flush request
    val flushFbCaches = in Bool ()

  }
  val addressRemapper =
    AddressRemapper(RegisterBank.externalBmbParams(c), RegisterBank.bmbParams(c))
  val regBank = RegisterBank(c)
  val pciFifo = PciFifo(
    busParams = RegisterBank.bmbParams(c),
    categories = regBank.busif.getCategories,
    floatAliases = regBank.busif.getFloatAliases,
    commandAddresses = regBank.busif.getCommandStreamReady.keys.toSeq.sorted
  )

  val frontdoor = CoreCpuFrontdoor(c)
  val pixelPipeline = PixelPipeline(c)
  val framebufferMem = FramebufferMemSubsystem(c)
  val textureMem = TextureMemSubsystem(c)

  val controlPlane = new Area {
    val swapBuffer = SwapBuffer()
    val framebufferLayout =
      FramebufferLayout.fromRegisterBank(c, io.fbBaseAddr, regBank, swapBuffer)

    frontdoor.io.cpuBus <> io.cpuBus
    frontdoor.io.regBus <> addressRemapper.io.input
    frontdoor.io.lfbBus <> pixelPipeline.io.lfbBus
    frontdoor.io.texReadBus <> textureMem.io.cpuTexRead
    pciFifo.io.texWrite << frontdoor.io.texWrite
    frontdoor.io.texBaseAddr := regBank.tmuConfig.texBaseAddr

    addressRemapper.io.output <> pciFifo.io.cpuSide
    pciFifo.io.regSide <> regBank.io.bus
    regBank.io.floatShadow <> pciFifo.io.floatShadow
    regBank.io.statusInputs <> io.statusInputs
    regBank.commands.nopCmd.ready := True

    swapBuffer.io.cmd << regBank.commands.swapbufferCmd
    swapBuffer.io.vRetrace := io.statusInputs.vRetrace
    swapBuffer.io.vsyncEnable := regBank.commands.swapVsyncEnable
    swapBuffer.io.swapInterval := regBank.commands.swapInterval
    swapBuffer.io.swapCmdEnqueued := pciFifo.io.wasEnqueued

    regBank.io.swapDisplayedBuffer := framebufferLayout.displayedBuffer
    regBank.io.swapsPending := framebufferLayout.swapsPending
    regBank.io.drawRouting := framebufferLayout.draw
    io.swapDisplayedBuffer := framebufferLayout.displayedBuffer
    io.swapsPending := framebufferLayout.swapsPending

    pixelPipeline.io.controls := PixelPipeline.Controls.fromRegisterBank(
      c,
      regBank,
      framebufferLayout
    )
    pixelPipeline.io.externalBusy := PixelPipeline.ExternalBusy.fromCore(regBank, swapBuffer)
    textureMem.io.downloadConfig := TextureMem.DownloadConfig.fromRegisterBank(c, regBank)
  }

  pixelPipeline.io.triangleCmd << regBank.commands.triangleCmd
  pixelPipeline.io.ftriangleCmd << regBank.commands.ftriangleCmd
  pixelPipeline.io.fastfillCmd << regBank.commands.fastfillCmd
  pixelPipeline.io.paletteWrite << regBank.io.paletteWrite
  pixelPipeline.io.tmuInvalidate := frontdoor.io.invalidate
  pixelPipeline.io.pciFifoEmpty := pciFifo.io.fifoEmpty
  pixelPipeline.io.fbStatus := framebufferMem.io.status
  pixelPipeline.io.fbStats := framebufferMem.io.stats

  framebufferMem.io.colorWrite << pixelPipeline.io.colorWrite
  framebufferMem.io.auxWrite << pixelPipeline.io.auxWrite
  framebufferMem.io.colorReadReq << pixelPipeline.io.colorReadReq
  pixelPipeline.io.colorReadRsp << framebufferMem.io.colorReadRsp
  framebufferMem.io.auxReadReq << pixelPipeline.io.auxReadReq
  pixelPipeline.io.auxReadRsp << framebufferMem.io.auxReadRsp
  framebufferMem.io.prefetchColor <> pixelPipeline.io.prefetchColor
  framebufferMem.io.prefetchAux <> pixelPipeline.io.prefetchAux
  framebufferMem.io.lfbReadBus <> pixelPipeline.io.lfbReadBus
  framebufferMem.io.flush := io.flushFbCaches
  framebufferMem.io.fbMemWrite <> io.fbMemWrite
  framebufferMem.io.fbColorReadMem <> io.fbColorReadMem
  framebufferMem.io.fbAuxReadMem <> io.fbAuxReadMem

  textureMem.io.cpuTexDrain << pciFifo.io.texDrain
  textureMem.io.tmuTexRead <> pixelPipeline.io.texRead
  textureMem.io.texMem <> io.texMem

  regBank.io.statistics := pixelPipeline.io.stats
  regBank.io.debug := pixelPipeline.io.debug

  pciFifo.io.pipelineBusy := pixelPipeline.io.debug.pipelineBusy
  pciFifo.io.commandReady := regBank.io.commandReady
  pciFifo.io.wasEnqueuedAddr := U(0x128, pciFifo.busAddrWidth bits)
  regBank.io.pciFifoEmpty := pciFifo.io.fifoEmpty
  regBank.io.pciFifoFree := pciFifo.io.pciFifoFree
  regBank.io.swapCmdEnqueued := pciFifo.io.wasEnqueued
  regBank.io.syncDrained := pciFifo.io.syncDrained
}
