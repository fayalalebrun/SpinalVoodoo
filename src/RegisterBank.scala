package voodoo

import spinal.core._
import spinal.core.sim._
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
    // Note: pciFifoFree comes from busif.pciFifoFree, not from external inputs
    // Note: fbiBusy/trexBusy/sstBusy are wired internally from pipelineBusy
    val statusInputs = in(new Bundle {
      val vRetrace = Bool()
      val memFifoFree = UInt(16 bits)
      val pciInterrupt = Bool()
    })

    // SwapBuffer-driven status fields (outputs from SwapBuffer component)
    val swapDisplayedBuffer = in UInt (2 bits)
    val swapsPending = in UInt (3 bits)

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

    // Pulses when swapbufferCMD enters the FIFO (for swapsPending per SST-1 spec)
    val swapCmdEnqueued = out Bool ()

    // FIFO empty signal - True when internal PCI FIFO has no pending commands
    val fifoEmpty = out Bool ()
  }

  // Create BMB bus interface for RegIf - shared across all Areas
  // Address remapping (for bit 21 set) is handled by AddressRemapper before this
  implicit val moduleName: spinal.lib.bus.regif.ClassName =
    spinal.lib.bus.regif.ClassName("RegisterBank")
  val busif = BmbBusInterface(io.bus, SizeMapping(0x000, 4 KiB), "VDO")

  // Connect pipeline busy signal for Sync=Yes register FIFO drain blocking
  busif.setPipelineBusy(io.pipelineBusy)

  // Track sync pulse - asserted when any Sync=Yes register is written
  val syncPulse = Reg(Bool()) init (False)
  io.syncPulse := syncPulse
  syncPulse := busif.isWriteSyncRequired() // Pulse for one cycle when Sync=Yes write completes

  // Wire FIFO empty status
  io.fifoEmpty := busif.fifoEmpty

  // ========================================================================
  // Status Register (0x000)
  // ========================================================================
  val status = new Area {
    val reg = busif.newRegAt(0x000, "status")
    val pciFifoFree = reg.field(UInt(6 bits), AccessType.RO, 0x3f, "PCI FIFO freespace")
    pciFifoFree := busif.pciFifoFree.sat(1) // Saturate 7-bit (0-64) to 6-bit (0-63)

    val vRetrace = reg.fieldAt(6, Bool(), AccessType.RO, 0, "Vertical retrace")
    vRetrace := io.statusInputs.vRetrace

    val fbiBusy = reg.fieldAt(7, Bool(), AccessType.RO, 0, "FBI graphics engine busy")
    fbiBusy := io.pipelineBusy

    val trexBusy = reg.fieldAt(8, Bool(), AccessType.RO, 0, "TREX busy")
    trexBusy := io.pipelineBusy

    val sstBusy = reg.fieldAt(9, Bool(), AccessType.RO, 0, "SST-1 busy")
    sstBusy := io.pipelineBusy

    val displayedBuffer = reg.fieldAt(10, UInt(2 bits), AccessType.RO, 0, "Displayed buffer")
    displayedBuffer := io.swapDisplayedBuffer

    val memFifoFree = reg.fieldAt(12, UInt(16 bits), AccessType.RO, 0xffff, "Memory FIFO freespace")
    memFifoFree := io.statusInputs.memFifoFree

    val swapsPending = reg.fieldAt(28, UInt(3 bits), AccessType.RO, 0, "Swap buffers pending")
    swapsPending := io.swapsPending

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
      .field(SInt(24 bits), AccessType.WO, 255 << 12, "Starting red value (12.12 fixed)")
      .asOutput()
    val startG = busif
      .newRegAt(0x024, "startG")
      .field(SInt(24 bits), AccessType.WO, 255 << 12, "Starting green value (12.12 fixed)")
      .asOutput()
    val startB = busif
      .newRegAt(0x028, "startB")
      .field(SInt(24 bits), AccessType.WO, 255 << 12, "Starting blue value (12.12 fixed)")
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
  // Float Triangle Geometry Area (0x088-0x0fc)
  // Float registers for ftriangleCMD - IEEE 754 single-precision float32
  // ========================================================================
  val floatTriangleGeometry = new Area {
    // Float Vertex Registers (0x088-0x09c)
    val fvertexAx = busif
      .newRegAtWithCategory(0x088, "fvertexAx", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Vertex A X coordinate (IEEE 754 float32)")
      .asOutput()
    val fvertexAy = busif
      .newRegAtWithCategory(0x08c, "fvertexAy", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Vertex A Y coordinate (IEEE 754 float32)")
      .asOutput()
    val fvertexBx = busif
      .newRegAtWithCategory(0x090, "fvertexBx", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Vertex B X coordinate (IEEE 754 float32)")
      .asOutput()
    val fvertexBy = busif
      .newRegAtWithCategory(0x094, "fvertexBy", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Vertex B Y coordinate (IEEE 754 float32)")
      .asOutput()
    val fvertexCx = busif
      .newRegAtWithCategory(0x098, "fvertexCx", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Vertex C X coordinate (IEEE 754 float32)")
      .asOutput()
    val fvertexCy = busif
      .newRegAtWithCategory(0x09c, "fvertexCy", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Vertex C Y coordinate (IEEE 754 float32)")
      .asOutput()

    // Float Start Value Registers (0x0a0-0x0bc) - all need fifoNoSync for proper triangle command capture
    val fstartR = busif
      .newRegAtWithCategory(0x0a0, "fstartR", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Starting red value (IEEE 754 float32)")
      .asOutput()
    val fstartG = busif
      .newRegAtWithCategory(0x0a4, "fstartG", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Starting green value (IEEE 754 float32)")
      .asOutput()
    val fstartB = busif
      .newRegAtWithCategory(0x0a8, "fstartB", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Starting blue value (IEEE 754 float32)")
      .asOutput()
    val fstartZ = busif
      .newRegAtWithCategory(0x0ac, "fstartZ", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Starting Z depth (IEEE 754 float32)")
      .asOutput()
    val fstartA = busif
      .newRegAtWithCategory(0x0b0, "fstartA", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Starting alpha value (IEEE 754 float32)")
      .asOutput()
    val fstartS = busif
      .newRegAtWithCategory(0x0b4, "fstartS", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Starting S texture coord (IEEE 754 float32)")
      .asOutput()
    val fstartT = busif
      .newRegAtWithCategory(0x0b8, "fstartT", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Starting T texture coord (IEEE 754 float32)")
      .asOutput()
    val fstartW = busif
      .newRegAtWithCategory(0x0bc, "fstartW", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Starting W value (IEEE 754 float32)")
      .asOutput()

    // Float X Gradient Registers (0x0c0-0x0dc)
    val fdRdX = busif
      .newRegAtWithCategory(0x0c0, "fdRdX", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Red gradient dR/dX (IEEE 754 float32)")
      .asOutput()
    val fdGdX = busif
      .newRegAtWithCategory(0x0c4, "fdGdX", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Green gradient dG/dX (IEEE 754 float32)")
      .asOutput()
    val fdBdX = busif
      .newRegAtWithCategory(0x0c8, "fdBdX", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Blue gradient dB/dX (IEEE 754 float32)")
      .asOutput()
    val fdZdX = busif
      .newRegAtWithCategory(0x0cc, "fdZdX", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Z gradient dZ/dX (IEEE 754 float32)")
      .asOutput()
    val fdAdX = busif
      .newRegAtWithCategory(0x0d0, "fdAdX", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Alpha gradient dA/dX (IEEE 754 float32)")
      .asOutput()
    val fdSdX = busif
      .newRegAtWithCategory(0x0d4, "fdSdX", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "S texture gradient dS/dX (IEEE 754 float32)")
      .asOutput()
    val fdTdX = busif
      .newRegAtWithCategory(0x0d8, "fdTdX", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "T texture gradient dT/dX (IEEE 754 float32)")
      .asOutput()
    val fdWdX = busif
      .newRegAtWithCategory(0x0dc, "fdWdX", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "W gradient dW/dX (IEEE 754 float32)")
      .asOutput()

    // Float Y Gradient Registers (0x0e0-0x0fc)
    val fdRdY = busif
      .newRegAtWithCategory(0x0e0, "fdRdY", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Red gradient dR/dY (IEEE 754 float32)")
      .asOutput()
    val fdGdY = busif
      .newRegAtWithCategory(0x0e4, "fdGdY", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Green gradient dG/dY (IEEE 754 float32)")
      .asOutput()
    val fdBdY = busif
      .newRegAtWithCategory(0x0e8, "fdBdY", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Blue gradient dB/dY (IEEE 754 float32)")
      .asOutput()
    val fdZdY = busif
      .newRegAtWithCategory(0x0ec, "fdZdY", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Z gradient dZ/dY (IEEE 754 float32)")
      .asOutput()
    val fdAdY = busif
      .newRegAtWithCategory(0x0f0, "fdAdY", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Alpha gradient dA/dY (IEEE 754 float32)")
      .asOutput()
    val fdSdY = busif
      .newRegAtWithCategory(0x0f4, "fdSdY", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "S texture gradient dS/dY (IEEE 754 float32)")
      .asOutput()
    val fdTdY = busif
      .newRegAtWithCategory(0x0f8, "fdTdY", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "T texture gradient dT/dY (IEEE 754 float32)")
      .asOutput()
    val fdWdY = busif
      .newRegAtWithCategory(0x0fc, "fdWdY", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.WO, 0, "W gradient dW/dY (IEEE 754 float32)")
      .asOutput()
  }

  // ========================================================================
  // Command Area (0x080-0x100)
  // Command registers with Stream outputs - FIFO queueing and backpressure handled by BusIf
  // ========================================================================
  val commands = new Area {
    val (triangleCmdReg, triangleCmdStream) =
      busif.newCommandReg(0x080, "triangleCMD", RegisterCategory.fifoNoSync)
    val (ftriangleCmdReg, ftriangleCmdStream) =
      busif.newCommandReg(0x100, "ftriangleCMD", RegisterCategory.fifoNoSync)
    val (nopCmdReg, nopCmdStream) =
      busif.newCommandReg(0x120, "nopCMD", RegisterCategory.fifoWithSync)
    val (fastfillCmdReg, fastfillCmdStream) =
      busif.newCommandReg(0x124, "fastfillCMD", RegisterCategory.fifoWithSync)
    val (swapbufferCmdReg, swapbufferCmdStream) =
      busif.newCommandReg(0x128, "swapbufferCMD", RegisterCategory.fifoWithSync)

    // Define fields for triangle command registers
    // Bit 31: sign bit (0=CCW, 1=CW), bits 0-30: reserved/unused in register definition
    // Full register value is accessed through the FIFO for queuing
    val triangleSignBit = triangleCmdReg
      .fieldAt(31, Bool(), AccessType.RW, 0, "Triangle sign bit (0=CCW, 1=CW)")
      .asOutput()

    val ftriangleSignBit = ftriangleCmdReg
      .fieldAt(31, Bool(), AccessType.RW, 0, "Float triangle sign bit (0=CCW, 1=CW)")
      .asOutput()

    // Define full 32-bit fields for other command registers
    nopCmdReg.field(Bits(32 bits), AccessType.RW, 0, "NOP command data")
    fastfillCmdReg.field(Bits(32 bits), AccessType.RW, 0, "Fastfill command data")
    val swapVsyncEnable = swapbufferCmdReg
      .field(Bool(), AccessType.RW, 0, "Vsync synchronization enable")
      .asOutput()
    val swapInterval = swapbufferCmdReg
      .field(UInt(8 bits), AccessType.RW, 0, "Swap interval (vsyncs to wait)")
      .asOutput()

    // Detect when swapbufferCMD enters the FIFO (for swapsPending increment per spec)
    io.swapCmdEnqueued := busif.wasEnqueued(0x128)

    // Expose streams for convenience (backwards compatibility)
    val triangleCmd = master(triangleCmdStream)
    val ftriangleCmd = master(ftriangleCmdStream)
    val nopCmd = master(nopCmdStream)
    val fastfillCmd = master(fastfillCmdStream)
    val swapbufferCmd = master(swapbufferCmdStream)
  }

  // ========================================================================
  // Render Configuration Area (0x104-0x148)
  // Rendering modes, clipping, and color constants
  // ========================================================================
  val renderConfig = new Area {
    // Rendering Mode Registers (0x104-0x10c) - Sync=No, FIFO=Yes
    val fbzColorPath = busif
      .newRegAtWithCategory(0x104, "fbzColorPath", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Color combine path control")
      .asOutput()

    val fogMode = busif
      .newRegAtWithCategory(0x108, "fogMode", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Fog mode control")
      .asOutput()

    val alphaMode = busif
      .newRegAtWithCategory(0x10c, "alphaMode", RegisterCategory.fifoNoSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Alpha test and blend control")
      .asOutput()

    // Rendering Mode Registers (0x110-0x114) - Sync=Yes, FIFO=Yes
    val fbzModeReg = busif.newRegAtWithCategory(0x110, "fbzMode", RegisterCategory.fifoWithSync)
    val fbzMode = new Area {
      val enableClipping =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable clipping rectangle").asOutput()
      val enableChromaKey =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable chroma-keying").asOutput()
      val enableStipple =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable stipple register masking").asOutput()
      val wBufferSelect = fbzModeReg
        .field(Bool(), AccessType.RW, 0, "W-Buffer Select (0=Z-value, 1=W-value)")
        .asOutput()
      val enableDepthBuffer =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable depth-buffering").asOutput()
      val depthFunction =
        fbzModeReg.field(UInt(3 bits), AccessType.RW, 0, "Depth-buffer function").asOutput()
      val enableDithering =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable dithering").asOutput()
      val rgbBufferMask =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "RGB buffer write mask").asOutput()
      val auxBufferMask =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Depth/alpha buffer write mask").asOutput()
      val ditherAlgorithm =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Dither algorithm (0=4x4, 1=2x2)").asOutput()
      val enableStipplePattern =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable Stipple pattern masking").asOutput()
      val enableAlphaMask =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable Alpha-channel mask").asOutput()
      val drawBuffer = fbzModeReg
        .field(UInt(2 bits), AccessType.RW, 0, "Draw buffer (0=Front, 1=Back, 2-3=Reserved)")
        .asOutput()
      val enableDepthBias =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable depth-biasing").asOutput()
      val yOrigin =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Y origin (0=top, 1=bottom)").asOutput()
      val enableAlphaPlanes =
        fbzModeReg.field(Bool(), AccessType.RW, 0, "Enable alpha planes").asOutput()
      val enableDitherSubtract = fbzModeReg
        .field(Bool(), AccessType.RW, 0, "Enable alpha-blending dither subtraction")
        .asOutput()
      val depthSourceSelect = fbzModeReg
        .field(Bool(), AccessType.RW, 0, "Depth source (0=normal, 1=zaColor[15:0])")
        .asOutput()
      val reserved = fbzModeReg.field(Bits(11 bits), AccessType.RW, 0, "Reserved").asOutput()
    }

    val lfbModeReg = busif.newRegAtWithCategory(0x114, "lfbMode", RegisterCategory.fifoWithSync)
    val lfbMode = new Area {
      val writeFormat = lfbModeReg
        .field(UInt(4 bits), AccessType.RW, 0, "Linear frame buffer write format")
        .asOutput()
      val writeBufferSelect = lfbModeReg
        .field(
          UInt(2 bits),
          AccessType.RW,
          0,
          "Write buffer select (0=front, 1=back, 2-3=reserved)"
        )
        .asOutput()
      val readBufferSelect = lfbModeReg
        .field(
          UInt(2 bits),
          AccessType.RW,
          0,
          "Read buffer select (0=front, 1=back, 2=depth/alpha, 3=reserved)"
        )
        .asOutput()
      val pixelPipelineEnable = lfbModeReg
        .field(Bool(), AccessType.RW, 0, "Enable pixel pipeline for LFB writes")
        .asOutput()
      val rgbaLanes = lfbModeReg
        .field(UInt(2 bits), AccessType.RW, 0, "RGBA lanes (0=ARGB, 1=ABGR, 2=RGBA, 3=BGRA)")
        .asOutput()
      val wordSwapWrites =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "16-bit word swap LFB writes").asOutput()
      val byteSwizzleWrites =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "Byte swizzle LFB writes").asOutput()
      val yOrigin =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "LFB Y origin (0=top, 1=bottom)").asOutput()
      val wSelect = lfbModeReg
        .field(Bool(), AccessType.RW, 0, "LFB write W select (0=LFB, 1=zaColor[15:0])")
        .asOutput()
      val wordSwapReads =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "16-bit word swap LFB reads").asOutput()
      val byteSwizzleReads =
        lfbModeReg.field(Bool(), AccessType.RW, 0, "Byte swizzle LFB reads").asOutput()
      val reserved = lfbModeReg.field(Bits(15 bits), AccessType.RW, 0, "Reserved").asOutput()
    }

    // Clipping Registers (0x118-0x11c) - Sync=Yes, FIFO=Yes
    // Datasheet: clipLeftRight bits[9:0]=right, bits[25:16]=left
    val clipLeftRight =
      busif.newRegAtWithCategory(0x118, "clipLeftRight", RegisterCategory.fifoWithSync)
    val clipRightX =
      clipLeftRight.field(UInt(10 bits), AccessType.RW, 0x3ff, "Right clip boundary").asOutput()
    val clipLeftX = clipLeftRight
      .fieldAt(16, UInt(10 bits), AccessType.RW, 0, "Left clip boundary")
      .asOutput()

    // Datasheet: clipLowYHighY bits[9:0]=highY, bits[25:16]=lowY
    val clipLowYHighY =
      busif.newRegAtWithCategory(0x11c, "clipLowYHighY", RegisterCategory.fifoWithSync)
    val clipHighY =
      clipLowYHighY.field(UInt(10 bits), AccessType.RW, 0x3ff, "Bottom clip boundary").asOutput()
    val clipLowY = clipLowYHighY
      .fieldAt(16, UInt(10 bits), AccessType.RW, 0, "Top clip boundary")
      .asOutput()

    // Color and Constant Registers (0x12c-0x148) - Sync=Yes, FIFO=Yes
    val fogColor = busif
      .newRegAtWithCategory(0x12c, "fogColor", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.WO, 0, "RGBA fog color")
      .asOutput()

    val zaColor = busif
      .newRegAtWithCategory(0x130, "zaColor", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Z/alpha constant for fills")
      .asOutput()

    val chromaKey = busif
      .newRegAtWithCategory(0x134, "chromaKey", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.WO, 0, "Chroma key color")
      .asOutput()

    val stipple = busif
      .newRegAtWithCategory(0x140, "stipple", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Stipple pattern")
      .asOutput()

    val color0 = busif
      .newRegAtWithCategory(0x144, "color0", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Constant color 0")
      .asOutput()

    val color1 = busif
      .newRegAtWithCategory(0x148, "color1", RegisterCategory.fifoWithSync)
      .field(Bits(32 bits), AccessType.RW, 0, "Constant color 1")
      .asOutput()

    // Bundle accessors — decode flat register bits into named fields

    def fbzColorPathBundle: FbzColorPath = {
      val b = FbzColorPath()
      b.assignFromBits(fbzColorPath.resized)
      b
    }

    def fogModeBundle: FogMode = {
      val b = FogMode()
      b.assignFromBits(fogMode.resized)
      b
    }

    def alphaModeBundle: AlphaMode = {
      val b = AlphaMode()
      b.assignFromBits(alphaMode)
      b
    }

    def fbzModeBundle: FbzMode = {
      val b = voodoo.FbzMode()
      b.enableClipping := fbzMode.enableClipping
      b.enableChromaKey := fbzMode.enableChromaKey
      b.enableStipple := fbzMode.enableStipple
      b.wBufferSelect := fbzMode.wBufferSelect
      b.enableDepthBuffer := fbzMode.enableDepthBuffer
      b.depthFunction := fbzMode.depthFunction
      b.enableDithering := fbzMode.enableDithering
      b.rgbBufferMask := fbzMode.rgbBufferMask
      b.auxBufferMask := fbzMode.auxBufferMask
      b.ditherAlgorithm := fbzMode.ditherAlgorithm
      b.enableStipplePattern := fbzMode.enableStipplePattern
      b.enableAlphaMask := fbzMode.enableAlphaMask
      b.drawBuffer := fbzMode.drawBuffer
      b.enableDepthBias := fbzMode.enableDepthBias
      b.yOrigin := fbzMode.yOrigin
      b.enableAlphaPlanes := fbzMode.enableAlphaPlanes
      b.enableDitherSubtract := fbzMode.enableDitherSubtract
      b.depthSourceSelect := fbzMode.depthSourceSelect
      b
    }

    def lfbModeBundle: LfbMode = {
      val b = LfbMode()
      b.writeFormat := lfbMode.writeFormat
      b.writeBufferSelect := lfbMode.writeBufferSelect
      b.readBufferSelect := lfbMode.readBufferSelect
      b.pixelPipelineEnable := lfbMode.pixelPipelineEnable
      b.rgbaLanes := lfbMode.rgbaLanes
      b.wordSwapWrites := lfbMode.wordSwapWrites
      b.byteSwizzleWrites := lfbMode.byteSwizzleWrites
      b.yOrigin := lfbMode.yOrigin
      b.wSelect := lfbMode.wSelect
      b.wordSwapReads := lfbMode.wordSwapReads
      b.byteSwizzleReads := lfbMode.byteSwizzleReads
      b
    }
  }

  // ========================================================================
  // Statistics Area (0x14c-0x15c)
  // Read-only performance counters
  // ========================================================================
  val statistics = new Area {
    val statsPixelsIn = busif
      .newRegAt(0x14c, "fbiPixelsIn")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels entering pipeline")
    statsPixelsIn := io.statisticsIn.pixelsIn

    val statsChromaFail = busif
      .newRegAt(0x150, "fbiChromaFail")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels failed chroma key test")
    statsChromaFail := io.statisticsIn.chromaFail

    val statsZFuncFail = busif
      .newRegAt(0x154, "fbiZfuncFail")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels failed Z test")
    statsZFuncFail := io.statisticsIn.zFuncFail

    val statsAFuncFail = busif
      .newRegAt(0x158, "fbiAfuncFail")
      .field(UInt(24 bits), AccessType.RO, 0, "Pixels failed alpha test")
    statsAFuncFail := io.statisticsIn.aFuncFail

    val statsPixelsOut = busif
      .newRegAt(0x15c, "fbiPixelsOut")
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
    val fbiInit1_yOriginSwap = fbiInit1Reg
      .fieldAt(22, UInt(10 bits), AccessType.RW, 0, "Y origin swap subtraction value")
      .asOutput()

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

  // ========================================================================
  // TMU Configuration Registers (0x300-0x320)
  // Single TMU support only (Voodoo 1 level functionality)
  // These control texture format, filtering, and base addresses
  // ========================================================================
  val tmuConfig = new Area {
    // textureMode (0x300) - Texture format, filtering, clamp, combine modes
    val textureModeReg =
      busif.newRegAtWithCategory(0x300, "textureMode", RegisterCategory.fifoNoSync)
    val textureMode =
      textureModeReg.field(Bits(32 bits), AccessType.WO, 0, "Texture mode").asOutput()

    // tLOD (0x304) - LOD configuration
    val tLODReg = busif.newRegAtWithCategory(0x304, "tLOD", RegisterCategory.fifoNoSync)
    val tLOD = tLODReg.field(Bits(32 bits), AccessType.WO, 0, "LOD configuration").asOutput()

    // tDetail (0x308) - Detail texture parameters
    val tDetailReg = busif.newRegAtWithCategory(0x308, "tDetail", RegisterCategory.fifoNoSync)
    val tDetail =
      tDetailReg.field(Bits(32 bits), AccessType.WO, 0, "Detail texture params").asOutput()

    // texBaseAddr (0x30C) - Texture base address
    val texBaseAddrReg =
      busif.newRegAtWithCategory(0x30c, "texBaseAddr", RegisterCategory.fifoNoSync)
    val texBaseAddr =
      texBaseAddrReg.field(UInt(24 bits), AccessType.WO, 0, "Texture base address").asOutput()
  }

  // ========================================================================
  // NCC Table Registers (0x324-0x380)
  // Two NCC tables for YIQ compressed texture decode
  // ========================================================================
  val nccTable = new Area {
    // NCC Table 0: Y[0-3], I[0-3], Q[0-3]
    val ncc0Y0Reg = busif.newRegAtWithCategory(0x324, "nccTable0Y0", RegisterCategory.fifoNoSync)
    val table0Y0 = ncc0Y0Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 Y0").asOutput()
    val ncc0Y1Reg = busif.newRegAtWithCategory(0x328, "nccTable0Y1", RegisterCategory.fifoNoSync)
    val table0Y1 = ncc0Y1Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 Y1").asOutput()
    val ncc0Y2Reg = busif.newRegAtWithCategory(0x32c, "nccTable0Y2", RegisterCategory.fifoNoSync)
    val table0Y2 = ncc0Y2Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 Y2").asOutput()
    val ncc0Y3Reg = busif.newRegAtWithCategory(0x330, "nccTable0Y3", RegisterCategory.fifoNoSync)
    val table0Y3 = ncc0Y3Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 Y3").asOutput()

    val ncc0I0Reg = busif.newRegAtWithCategory(0x334, "nccTable0I0", RegisterCategory.fifoNoSync)
    val table0I0 = ncc0I0Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 I0").asOutput()
    val ncc0I1Reg = busif.newRegAtWithCategory(0x338, "nccTable0I1", RegisterCategory.fifoNoSync)
    val table0I1 = ncc0I1Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 I1").asOutput()
    val ncc0I2Reg = busif.newRegAtWithCategory(0x33c, "nccTable0I2", RegisterCategory.fifoNoSync)
    val table0I2 = ncc0I2Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 I2").asOutput()
    val ncc0I3Reg = busif.newRegAtWithCategory(0x340, "nccTable0I3", RegisterCategory.fifoNoSync)
    val table0I3 = ncc0I3Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 I3").asOutput()

    val ncc0Q0Reg = busif.newRegAtWithCategory(0x344, "nccTable0Q0", RegisterCategory.fifoNoSync)
    val table0Q0 = ncc0Q0Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 Q0").asOutput()
    val ncc0Q1Reg = busif.newRegAtWithCategory(0x348, "nccTable0Q1", RegisterCategory.fifoNoSync)
    val table0Q1 = ncc0Q1Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 Q1").asOutput()
    val ncc0Q2Reg = busif.newRegAtWithCategory(0x34c, "nccTable0Q2", RegisterCategory.fifoNoSync)
    val table0Q2 = ncc0Q2Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 Q2").asOutput()
    val ncc0Q3Reg = busif.newRegAtWithCategory(0x350, "nccTable0Q3", RegisterCategory.fifoNoSync)
    val table0Q3 = ncc0Q3Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 0 Q3").asOutput()

    // NCC Table 1: Y[0-3], I[0-3], Q[0-3]
    val ncc1Y0Reg = busif.newRegAtWithCategory(0x354, "nccTable1Y0", RegisterCategory.fifoNoSync)
    val table1Y0 = ncc1Y0Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 Y0").asOutput()
    val ncc1Y1Reg = busif.newRegAtWithCategory(0x358, "nccTable1Y1", RegisterCategory.fifoNoSync)
    val table1Y1 = ncc1Y1Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 Y1").asOutput()
    val ncc1Y2Reg = busif.newRegAtWithCategory(0x35c, "nccTable1Y2", RegisterCategory.fifoNoSync)
    val table1Y2 = ncc1Y2Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 Y2").asOutput()
    val ncc1Y3Reg = busif.newRegAtWithCategory(0x360, "nccTable1Y3", RegisterCategory.fifoNoSync)
    val table1Y3 = ncc1Y3Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 Y3").asOutput()

    val ncc1I0Reg = busif.newRegAtWithCategory(0x364, "nccTable1I0", RegisterCategory.fifoNoSync)
    val table1I0 = ncc1I0Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 I0").asOutput()
    val ncc1I1Reg = busif.newRegAtWithCategory(0x368, "nccTable1I1", RegisterCategory.fifoNoSync)
    val table1I1 = ncc1I1Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 I1").asOutput()
    val ncc1I2Reg = busif.newRegAtWithCategory(0x36c, "nccTable1I2", RegisterCategory.fifoNoSync)
    val table1I2 = ncc1I2Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 I2").asOutput()
    val ncc1I3Reg = busif.newRegAtWithCategory(0x370, "nccTable1I3", RegisterCategory.fifoNoSync)
    val table1I3 = ncc1I3Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 I3").asOutput()

    val ncc1Q0Reg = busif.newRegAtWithCategory(0x374, "nccTable1Q0", RegisterCategory.fifoNoSync)
    val table1Q0 = ncc1Q0Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 Q0").asOutput()
    val ncc1Q1Reg = busif.newRegAtWithCategory(0x378, "nccTable1Q1", RegisterCategory.fifoNoSync)
    val table1Q1 = ncc1Q1Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 Q1").asOutput()
    val ncc1Q2Reg = busif.newRegAtWithCategory(0x37c, "nccTable1Q2", RegisterCategory.fifoNoSync)
    val table1Q2 = ncc1Q2Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 Q2").asOutput()
    val ncc1Q3Reg = busif.newRegAtWithCategory(0x380, "nccTable1Q3", RegisterCategory.fifoNoSync)
    val table1Q3 = ncc1Q3Reg.field(Bits(32 bits), AccessType.WO, 0, "NCC table 1 Q3").asOutput()

    // Convenience accessors as arrays
    def table0Y(i: Int): Bits = i match {
      case 0 => table0Y0; case 1 => table0Y1; case 2 => table0Y2; case 3 => table0Y3
    }
    def table0I(i: Int): Bits = i match {
      case 0 => table0I0; case 1 => table0I1; case 2 => table0I2; case 3 => table0I3
    }
    def table0Q(i: Int): Bits = i match {
      case 0 => table0Q0; case 1 => table0Q1; case 2 => table0Q2; case 3 => table0Q3
    }
    def table1Y(i: Int): Bits = i match {
      case 0 => table1Y0; case 1 => table1Y1; case 2 => table1Y2; case 3 => table1Y3
    }
    def table1I(i: Int): Bits = i match {
      case 0 => table1I0; case 1 => table1I1; case 2 => table1I2; case 3 => table1I3
    }
    def table1Q(i: Int): Bits = i match {
      case 0 => table1Q0; case 1 => table1Q1; case 2 => table1Q2; case 3 => table1Q3
    }
  }

  // ========================================================================
  // Simulation Support - Make all register fields accessible during simulation
  // ========================================================================
  busif.slices.foreach { slice =>
    val addr = slice.getAddr()
    var fieldIndex = 0
    slice.getFields().foreach { field =>
      // Simple naming: address + field index (no string manipulation needed)
      field.hardbit.setName(f"sim_reg_${addr}%03x_f$fieldIndex")
      field.hardbit.simPublic()
      fieldIndex += 1
    }
  }
}

object RegisterBank {
  def bmbParams(c: Config) = BmbParameter(
    addressWidth = 12, // 4KB address space (remapping handled by AddressRemapper)
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )

  // External bus params include bit 21 for remap detection
  def externalBmbParams(c: Config) = BmbParameter(
    addressWidth = 22, // 4MB address space to include bit 21 for remapped registers
    dataWidth = 32,
    sourceWidth = 4,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.WORD
  )
}
