package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.regif._
import spinal.lib.bus.misc.SizeMapping
import voodoo.utils.{BmbBusInterface, RegisterCategory}

/** Voodoo Graphics Register Bank
  *
  * Register organization:
  *   - Status Area: Status register with hardware inputs
  *   - Triangle Geometry Area: Vertex coordinates, start values, and gradients
  *   - Command Area: Command streams with backpressure
  *   - Render Config Area: Rendering modes, clipping, and color constants
  *   - Statistics Area: Read-only performance counters
  *   - Fog Table Area: 64-entry fog lookup table
  *   - Initialization Area: Display timing and hardware configuration
  *
  * @param config
  *   Voodoo hardware configuration
  */
case class RegisterBank(config: Config) extends Component {
  val io = new Bundle {
    // BMB bus interface for register access
    val bus = slave(Bmb(RegisterBank.bmbParams(config)))

    // Hardware inputs for status register (read-only fields)
    val statusInputs = in(new Bundle {
      val pciFifoFree = UInt(6 bits)
      val vRetrace = Bool()
      val fbiBusy = Bool()
      val trexBusy = Bool()
      val sstBusy = Bool()
      val displayedBuffer = UInt(2 bits)
      val memFifoFree = UInt(16 bits)
      val swapsPending = UInt(3 bits)
      val pciInterrupt = Bool()
    })

    // Statistics counter inputs (hardware → register bank, read-only)
    val statisticsIn = in(new Bundle {
      val pixelsIn = UInt(24 bits)
      val chromaFail = UInt(24 bits)
      val zFuncFail = UInt(24 bits)
      val aFuncFail = UInt(24 bits)
      val pixelsOut = UInt(24 bits)
    })

    // Pipeline sync pulse - asserted when Sync=Yes register written
    val syncPulse = out Bool ()

    // Pipeline busy signal - used to stall Sync=Yes register writes
    val pipelineBusy = in Bool ()
  }

  // Create BMB bus interface for RegIf - shared across all Areas
  implicit val moduleName: spinal.lib.bus.regif.ClassName =
    spinal.lib.bus.regif.ClassName("RegisterBank")
  val busif = BmbBusInterface(io.bus, SizeMapping(0x000, 1 KiB), "VDO")

  // Connect pipeline busy signal for Sync=Yes register FIFO drain blocking
  busif.setPipelineBusy(io.pipelineBusy)

  // Track sync pulse - OR of all Sync=Yes register writes
  val syncRequired = Reg(Bool()) init (False)
  io.syncPulse := syncRequired
  syncRequired := False // Pulse for one cycle

  // ========================================================================
  // Status Register (0x000)
  // ========================================================================
  val status = new Area {
    val reg = busif.newRegAt(0x000, "status")
    val pciFifoFree = reg.field(UInt(6 bits), AccessType.RO, 0x3f, "PCI FIFO freespace")
    pciFifoFree := io.statusInputs.pciFifoFree

    val vRetrace = reg.fieldAt(6, Bool(), AccessType.RO, 0, "Vertical retrace")
    vRetrace := io.statusInputs.vRetrace

    val fbiBusy = reg.fieldAt(7, Bool(), AccessType.RO, 0, "FBI graphics engine busy")
    fbiBusy := io.statusInputs.fbiBusy

    val trexBusy = reg.fieldAt(8, Bool(), AccessType.RO, 0, "TREX busy")
    trexBusy := io.statusInputs.trexBusy

    val sstBusy = reg.fieldAt(9, Bool(), AccessType.RO, 0, "SST-1 busy")
    sstBusy := io.statusInputs.sstBusy

    val displayedBuffer = reg.fieldAt(10, UInt(2 bits), AccessType.RO, 0, "Displayed buffer")
    displayedBuffer := io.statusInputs.displayedBuffer

    val memFifoFree = reg.fieldAt(12, UInt(16 bits), AccessType.RO, 0xffff, "Memory FIFO freespace")
    memFifoFree := io.statusInputs.memFifoFree

    val swapsPending = reg.fieldAt(28, UInt(3 bits), AccessType.RO, 0, "Swap buffers pending")
    swapsPending := io.statusInputs.swapsPending

    val pciInterrupt = reg.fieldAt(31, Bool(), AccessType.RO, 0, "PCI interrupt generated")
    pciInterrupt := io.statusInputs.pciInterrupt
  }

  // ========================================================================
  // Triangle Geometry (0x008-0x07C)
  // ========================================================================
  val triangleGeometry = new Area {
    // Triangle geometry registers are FIFO=Yes, Sync=No
    // Vertex Registers (0x008-0x01C)
    val vertexAx = busif
      .newRegAtWithCategory(0x008, "vertexAx", RegisterCategory.fifoNoSync)
      .field(SInt(16 bits), AccessType.WO, 0, "Vertex A X coordinate")
      .asOutput()
    val vertexAy = busif
      .newRegAtWithCategory(0x00c, "vertexAy", RegisterCategory.fifoNoSync)
      .field(SInt(16 bits), AccessType.WO, 0, "Vertex A Y coordinate")
      .asOutput()
    val vertexBx = busif
      .newRegAtWithCategory(0x010, "vertexBx", RegisterCategory.fifoNoSync)
      .field(SInt(16 bits), AccessType.WO, 0, "Vertex B X coordinate")
      .asOutput()
    val vertexBy = busif
      .newRegAtWithCategory(0x014, "vertexBy", RegisterCategory.fifoNoSync)
      .field(SInt(16 bits), AccessType.WO, 0, "Vertex B Y coordinate")
      .asOutput()
    val vertexCx = busif
      .newRegAtWithCategory(0x018, "vertexCx", RegisterCategory.fifoNoSync)
      .field(SInt(16 bits), AccessType.WO, 0, "Vertex C X coordinate")
      .asOutput()
    val vertexCy = busif
      .newRegAtWithCategory(0x01c, "vertexCy", RegisterCategory.fifoNoSync)
      .field(SInt(16 bits), AccessType.WO, 0, "Vertex C Y coordinate")
      .asOutput()

    // Start Value Registers (0x020-0x03C)
    val startR = busif
      .newRegAt(0x020, "startR")
      .field(SInt(24 bits), AccessType.WO, 0, "Starting red value (12.12 fixed)")
      .asOutput()
    val startG = busif
      .newRegAt(0x024, "startG")
      .field(SInt(24 bits), AccessType.WO, 0, "Starting green value (12.12 fixed)")
      .asOutput()
    val startB = busif
      .newRegAt(0x028, "startB")
      .field(SInt(24 bits), AccessType.WO, 0, "Starting blue value (12.12 fixed)")
      .asOutput()
    val startZ = busif
      .newRegAt(0x02c, "startZ")
      .field(SInt(32 bits), AccessType.WO, 0, "Starting Z depth (20.12 fixed)")
      .asOutput()
    val startA = busif
      .newRegAt(0x030, "startA")
      .field(SInt(24 bits), AccessType.WO, 0, "Starting alpha value (12.12 fixed)")
      .asOutput()
    val startS = busif
      .newRegAt(0x034, "startS")
      .field(SInt(32 bits), AccessType.WO, 0, "Starting S texture coord (14.18 fixed)")
      .asOutput()
    val startT = busif
      .newRegAt(0x038, "startT")
      .field(SInt(32 bits), AccessType.WO, 0, "Starting T texture coord (14.18 fixed)")
      .asOutput()
    val startW = busif
      .newRegAt(0x03c, "startW")
      .field(SInt(32 bits), AccessType.WO, 0, "Starting W value (2.30 fixed)")
      .asOutput()

    // X Gradient Registers (0x040-0x05C)
    val dRdX = busif
      .newRegAt(0x040, "dRdX")
      .field(SInt(24 bits), AccessType.WO, 0, "Red gradient dR/dX (12.12 fixed)")
      .asOutput()
    val dGdX = busif
      .newRegAt(0x044, "dGdX")
      .field(SInt(24 bits), AccessType.WO, 0, "Green gradient dG/dX (12.12 fixed)")
      .asOutput()
    val dBdX = busif
      .newRegAt(0x048, "dBdX")
      .field(SInt(24 bits), AccessType.WO, 0, "Blue gradient dB/dX (12.12 fixed)")
      .asOutput()
    val dZdX = busif
      .newRegAt(0x04c, "dZdX")
      .field(SInt(32 bits), AccessType.WO, 0, "Z gradient dZ/dX (20.12 fixed)")
      .asOutput()
    val dAdX = busif
      .newRegAt(0x050, "dAdX")
      .field(SInt(24 bits), AccessType.WO, 0, "Alpha gradient dA/dX (12.12 fixed)")
      .asOutput()
    val dSdX = busif
      .newRegAt(0x054, "dSdX")
      .field(SInt(32 bits), AccessType.WO, 0, "S texture gradient dS/dX (14.18 fixed)")
      .asOutput()
    val dTdX = busif
      .newRegAt(0x058, "dTdX")
      .field(SInt(32 bits), AccessType.WO, 0, "T texture gradient dT/dX (14.18 fixed)")
      .asOutput()
    val dWdX = busif
      .newRegAt(0x05c, "dWdX")
      .field(SInt(32 bits), AccessType.WO, 0, "W gradient dW/dX (2.30 fixed)")
      .asOutput()

    // Y Gradient Registers (0x060-0x07C)
    val dRdY = busif
      .newRegAt(0x060, "dRdY")
      .field(SInt(24 bits), AccessType.WO, 0, "Red gradient dR/dY (12.12 fixed)")
      .asOutput()
    val dGdY = busif
      .newRegAt(0x064, "dGdY")
      .field(SInt(24 bits), AccessType.WO, 0, "Green gradient dG/dY (12.12 fixed)")
      .asOutput()
    val dBdY = busif
      .newRegAt(0x068, "dBdY")
      .field(SInt(24 bits), AccessType.WO, 0, "Blue gradient dB/dY (12.12 fixed)")
      .asOutput()
    val dZdY = busif
      .newRegAt(0x06c, "dZdY")
      .field(SInt(32 bits), AccessType.WO, 0, "Z gradient dZ/dY (20.12 fixed)")
      .asOutput()
    val dAdY = busif
      .newRegAt(0x070, "dAdY")
      .field(SInt(24 bits), AccessType.WO, 0, "Alpha gradient dA/dY (12.12 fixed)")
      .asOutput()
    val dSdY = busif
      .newRegAt(0x074, "dSdY")
      .field(SInt(32 bits), AccessType.WO, 0, "S texture gradient dS/dY (14.18 fixed)")
      .asOutput()
    val dTdY = busif
      .newRegAt(0x078, "dTdY")
      .field(SInt(32 bits), AccessType.WO, 0, "T texture gradient dT/dY (14.18 fixed)")
      .asOutput()
    val dWdY = busif
      .newRegAt(0x07c, "dWdY")
      .field(SInt(32 bits), AccessType.WO, 0, "W gradient dW/dY (2.30 fixed)")
      .asOutput()
  }

  // ========================================================================
  // Command Area (0x080-0x100)
  // Command registers with Stream outputs - FIFO queueing and backpressure handled by BusIf
  // ========================================================================
  val commands = new Area {
    val triangleCmd = master(busif.newCommandReg(0x080, "triangleCMD", RegisterCategory.fifoNoSync))
    val fastfillCmd = master(
      busif.newCommandReg(0x084, "fastfillCMD", RegisterCategory.fifoWithSync)
    )
    val nopCmd = master(busif.newCommandReg(0x088, "nopCMD", RegisterCategory.fifoWithSync))
    val swapbufferCmd = master(
      busif.newCommandReg(0x08c, "swapbufferCMD", RegisterCategory.fifoWithSync)
    )
    val userIntrCmd = master(busif.newCommandReg(0x0fc, "userIntrCMD", RegisterCategory.fifoNoSync))
    val ftriangleCmd = master(
      busif.newCommandReg(0x100, "ftriangleCMD", RegisterCategory.fifoNoSync)
    )
  }

  // ========================================================================
  // Render Configuration Area (0x0C0-0x0F8)
  // Rendering modes, clipping, and color constants
  // ========================================================================
  val renderConfig = new Area {
    // Rendering Mode Registers (0x0C0-0x0C8) - Sync=No, FIFO=Yes
    val fbzColorPath = busif
      .newRegAtWithCategory(0x0c0, "fbzColorPath", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Color combine path control")
      .asOutput()

    val fogMode = busif
      .newRegAtWithCategory(0x0c4, "fogMode", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Fog mode control")
      .asOutput()

    val alphaMode = busif
      .newRegAtWithCategory(0x0c8, "alphaMode", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Alpha test and blend control")
      .asOutput()

    // Rendering Mode Registers (0x0CC-0x0D0) - Sync=Yes, FIFO=Yes
    val fbzMode = busif
      .newRegAtWithCategory(0x0cc, "fbzMode", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Framebuffer and Z-buffer mode")
      .asOutput()
    when(busif.doWrite && busif.writeAddress() === 0x0cc) { syncRequired := True }

    val lfbMode = busif
      .newRegAtWithCategory(0x0d0, "lfbMode", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Linear framebuffer mode")
      .asOutput()
    when(busif.doWrite && busif.writeAddress() === 0x0d0) { syncRequired := True }

    // Clipping Registers (0x0D4-0x0D8) - Sync=Yes, FIFO=Yes
    val clipLeftRight =
      busif.newRegAtWithCategory(0x0d4, "clipLeftRight", RegisterCategory.fifoWithSync)
    val clipLeftX =
      clipLeftRight.field(UInt(10 bits), AccessType.RW, 0, "Left clip boundary").asOutput()
    val clipRightX = clipLeftRight
      .fieldAt(16, UInt(10 bits), AccessType.RW, 0x3ff, "Right clip boundary")
      .asOutput()
    when(busif.doWrite && busif.writeAddress() === 0x0d4) { syncRequired := True }

    val clipLowYHighY =
      busif.newRegAtWithCategory(0x0d8, "clipLowYHighY", RegisterCategory.fifoWithSync)
    val clipLowY =
      clipLowYHighY.field(UInt(10 bits), AccessType.RW, 0, "Top clip boundary").asOutput()
    val clipHighY = clipLowYHighY
      .fieldAt(16, UInt(10 bits), AccessType.RW, 0x3ff, "Bottom clip boundary")
      .asOutput()
    when(busif.doWrite && busif.writeAddress() === 0x0d8) { syncRequired := True }

    // Color and Constant Registers (0x0E4-0x0F8) - Sync=No, FIFO=Yes
    val fogColor = busif
      .newRegAtWithCategory(0x0e4, "fogColor", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "RGBA fog color")
      .asOutput()

    val zaColor = busif
      .newRegAt(0x0e8, "zaColor")
      .field(Bits(32 bits), AccessType.WO, 0, "Z/alpha constant for fills")
      .asOutput()

    val chromaKey = busif
      .newRegAt(0x0ec, "chromaKey")
      .field(Bits(32 bits), AccessType.WO, 0, "Chroma key color")
      .asOutput()

    val color0 = busif
      .newRegAt(0x0f0, "color0")
      .field(Bits(32 bits), AccessType.RW, 0, "Constant color 0")
      .asOutput()

    val color1 = busif
      .newRegAt(0x0f4, "color1")
      .field(Bits(32 bits), AccessType.RW, 0, "Constant color 1")
      .asOutput()

    val stipple = busif
      .newRegAt(0x0f8, "stipple")
      .field(Bits(32 bits), AccessType.RW, 0, "Stipple pattern")
      .asOutput()
  }

  // ========================================================================
  // Statistics Area (0x110-0x124)
  // Read-only performance counters
  // ========================================================================
  val statistics = new Area {
    val statsPixelsIn = busif
      .newRegAt(0x110, "statsPixelsIn")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels entering pipeline")
    statsPixelsIn := io.statisticsIn.pixelsIn

    val statsChromaFail = busif
      .newRegAt(0x114, "statsChromaFail")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels failed chroma key test")
    statsChromaFail := io.statisticsIn.chromaFail

    val statsZFuncFail = busif
      .newRegAt(0x118, "statsZFuncFail")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels failed Z test")
    statsZFuncFail := io.statisticsIn.zFuncFail

    val statsAFuncFail = busif
      .newRegAt(0x11c, "statsAFuncFail")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels failed alpha test")
    statsAFuncFail := io.statisticsIn.aFuncFail

    val statsPixelsOut = busif
      .newRegAt(0x120, "statsPixelsOut")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels written to framebuffer")
    statsPixelsOut := io.statisticsIn.pixelsOut
  }

  // ========================================================================
  // Fog Table Area (0x160-0x1DC)
  // 64-entry fog lookup table (32 registers, 2 entries per register)
  // ========================================================================
  val fogTable = new Area {
    val fogTable = (0 until 32).flatMap { i =>
      val reg = busif.newRegAt(0x160 + i * 4, s"fogTable${i * 2}")
      val entry0_dfog =
        reg.field(Bits(8 bits), AccessType.WO, 0, s"Fog entry ${i * 2} delta").asOutput()
      val entry0_fog =
        reg.fieldAt(8, Bits(8 bits), AccessType.WO, 0, s"Fog entry ${i * 2} value").asOutput()
      val entry1_dfog =
        reg.fieldAt(16, Bits(8 bits), AccessType.WO, 0, s"Fog entry ${i * 2 + 1} delta").asOutput()
      val entry1_fog =
        reg.fieldAt(24, Bits(8 bits), AccessType.WO, 0, s"Fog entry ${i * 2 + 1} value").asOutput()
      Seq((entry0_dfog, entry0_fog), (entry1_dfog, entry1_fog))
    }
  }

  // ========================================================================
  // Initialization Area (0x200-0x24C)
  // Display timing and hardware configuration registers
  // ========================================================================
  val init = new Area {
    // All init registers bypass FIFO (FIFO=No)
    // fbiInit4 (0x200) - PCI read timing
    val fbiInit4Reg = busif.newRegAtWithCategory(0x200, "fbiInit4", RegisterCategory.bypassFifo)
    val fbiInit4_pciReadWaitStates = fbiInit4Reg
      .field(Bool(), AccessType.RW, 0, "PCI read wait states: 0=1 wait state, 1=2 wait states")
      .asOutput()

    // backPorch (0x208) - Display back porch timing
    val backPorch = busif
      .newRegAtWithCategory(0x208, "backPorch", RegisterCategory.bypassFifo)
      .field(Bits(32 bits), AccessType.WO, 0, "Display back porch timing")
      .asOutput()

    // videoDimensions (0x20C) - Display resolution
    val videoDimensionsReg =
      busif.newRegAtWithCategory(0x20c, "videoDimensions", RegisterCategory.bypassFifo)
    val hDisp = videoDimensionsReg
      .field(UInt(12 bits), AccessType.WO, 0, "Horizontal display width - 1")
      .asOutput()
    val vDisp = videoDimensionsReg
      .fieldAt(16, UInt(12 bits), AccessType.WO, 0, "Vertical display height")
      .asOutput()

    // fbiInit0 (0x210) - VGA passthrough and graphics reset
    val fbiInit0Reg = busif.newRegAtWithCategory(0x210, "fbiInit0", RegisterCategory.bypassFifo)
    val fbiInit0_vgaPassthrough = fbiInit0Reg
      .field(Bool(), AccessType.RW, 0, "VGA passthrough: 0=VGA blocked, 1=VGA passed")
      .asOutput()
    val fbiInit0_graphicsReset =
      fbiInit0Reg.fieldAt(1, Bool(), AccessType.RW, 0, "Graphics reset").asOutput()

    // fbiInit1 (0x214) - PCI timing and SLI enable
    val fbiInit1Reg = busif.newRegAt(0x214, "fbiInit1")
    val fbiInit1_pciWriteWaitStates = fbiInit1Reg
      .fieldAt(1, Bool(), AccessType.RW, 0, "PCI write wait states: 0=fast, 1=slow")
      .asOutput()
    val fbiInit1_multiSst =
      fbiInit1Reg.fieldAt(2, Bool(), AccessType.RW, 0, "Multi-SST (SLI) mode [V1]").asOutput()
    val fbiInit1_videoReset =
      fbiInit1Reg.fieldAt(8, Bool(), AccessType.RW, 0, "Video timing reset").asOutput()
    val fbiInit1_sliEnable =
      fbiInit1Reg.fieldAt(23, Bool(), AccessType.RW, 0, "SLI enable [V2+]").asOutput()

    // fbiInit2 (0x218) - Buffer config and swap algorithm
    val fbiInit2Reg = busif.newRegAt(0x218, "fbiInit2")
    val fbiInit2_swapAlgorithm = fbiInit2Reg
      .fieldAt(
        9,
        UInt(2 bits),
        AccessType.RW,
        0,
        "Swap algorithm: 0=DAC vsync, 1=DAC data, 2=PCI FIFO stall, 3=SLI sync"
      )
      .asOutput()
    val fbiInit2_bufferOffset = fbiInit2Reg
      .fieldAt(11, UInt(10 bits), AccessType.RW, 0, "Buffer offset in 4KB units")
      .asOutput()

    // fbiInit3 (0x21C) - Register remapping
    val fbiInit3Reg = busif.newRegAt(0x21c, "fbiInit3")
    val fbiInit3_remapEnable =
      fbiInit3Reg.field(Bool(), AccessType.RW, 0, "Enable register address remapping").asOutput()

    // hSync (0x220) - Horizontal sync timing
    val hSyncReg = busif.newRegAt(0x220, "hSync")
    val hSyncOn = hSyncReg.field(UInt(8 bits), AccessType.WO, 0, "Horizontal sync start").asOutput()
    val hSyncOff =
      hSyncReg.fieldAt(16, UInt(10 bits), AccessType.WO, 0, "Horizontal sync end").asOutput()

    // vSync (0x224) - Vertical sync timing
    val vSyncReg = busif.newRegAt(0x224, "vSync")
    val vSyncOn =
      vSyncReg.field(UInt(16 bits), AccessType.WO, 0, "Vertical sync start (lines)").asOutput()
    val vSyncOff =
      vSyncReg.fieldAt(16, UInt(16 bits), AccessType.WO, 0, "Vertical sync end (lines)").asOutput()

    // maxRgbDelta (0x230) - Max RGB difference for video filtering
    val maxRgbDelta = busif
      .newRegAt(0x230, "maxRgbDelta")
      .field(Bits(32 bits), AccessType.WO, 0, "Max RGB difference for video filtering")
      .asOutput()

    // fbiInit5 (0x244) - Multi-chip config
    val fbiInit5Reg = busif.newRegAt(0x244, "fbiInit5")
    val fbiInit5_multiCvg = fbiInit5Reg
      .fieldAt(14, Bool(), AccessType.RW, 0, "Multi-chip coverage (SLI for V2)")
      .asOutput()

    // fbiInit6 (0x248) - Extended init
    val fbiInit6Reg = busif.newRegAt(0x248, "fbiInit6")
    val fbiInit6_blockWidthExtend = fbiInit6Reg
      .fieldAt(30, Bool(), AccessType.RW, 0, "Extends block width calculation")
      .asOutput()

    // fbiInit7 (0x24C) - Command FIFO enable [V2+]
    val fbiInit7Reg = busif.newRegAt(0x24c, "fbiInit7")
    val fbiInit7_cmdFifoEnable =
      fbiInit7Reg.fieldAt(8, Bool(), AccessType.RW, 0, "Enable command FIFO mode [V2+]").asOutput()
  }
}

object RegisterBank {
  def bmbParams(c: Config) = BmbParameter(
    addressWidth = 12, // 4KB register space
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )
}
