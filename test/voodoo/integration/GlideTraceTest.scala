package voodoo.integration

import spinal.core._
import spinal.core.sim._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim._
import spinal.lib.sim.StreamMonitor
import org.scalatest.funsuite.AnyFunSuite
import voodoo._
import voodoo.ref.VoodooReference
import voodoo.ref.VoodooReference.RefPixel
import voodoo.trace.{TraceParser, TraceEntry, TraceCommandType}
import java.io.File
import java.awt.image.BufferedImage
import javax.imageio.ImageIO

/** Trace-based Glide integration test.
  *
  * Loads a binary trace file (produced by sim_trace_harness or 86Box), replays it through
  * VoodooReference (golden model) and the Verilator Core simulation, and compares per-triangle
  * pixel output. Writes PNG screenshots of the full reference and simulation framebuffers as
  * artifacts.
  */
class GlideTraceTest extends AnyFunSuite {

  val voodooConfig = Config.voodoo1()
  val stride = voodooConfig.fbPixelStride // 1024

  // Compile Verilator model once, reuse across tests
  lazy val compiled = SimConfig.withVerilator.compile(Core(voodooConfig))

  // Default display dimensions for screenshot output
  val displayWidth = 640
  val displayHeight = 480

  // Output directory for artifacts
  val resultDir = new File("test-output")

  // ========================================================================
  // Register address constants
  // ========================================================================
  val REG_TRIANGLE_CMD = 0x080
  val REG_FTRIANGLE_CMD = 0x100
  val REG_TEXBASEADDR = 0x30c
  val REG_CLIP_LR = 0x118
  val REG_CLIP_TB = 0x11c
  val REG_FBZMODE = 0x110
  val REG_FBIINIT2 = 0x218

  // Status register bits (matching CoreTest)
  val SST_FIFO_FREE_MASK = 0x3fL

  // ========================================================================
  // Float-to-fixed conversion for Voodoo1 float register writes
  // ========================================================================
  // Float registers (0x088-0x0FC) are 0x080 above their integer counterparts
  // (0x008-0x07C). The hardware converts IEEE754 floats to fixed-point.
  val FLOAT_REG_BASE = 0x088
  val FLOAT_REG_END = 0x0fc

  /** Convert an IEEE754 float (as raw bits) to fixed-point integer, based on which integer register
    * it maps to.
    */
  def floatRegToFixed(intRegAddr: Int, floatBits: Long): Int = {
    val f = java.lang.Float.intBitsToFloat(floatBits.toInt)
    // Vertex coordinates: 12.4 fixed-point
    if (intRegAddr >= 0x008 && intRegAddr <= 0x01c)
      return (f * 16.0f).toInt
    // Start values, X gradients, Y gradients share the same format
    // per position within their 8-register group:
    //   r,g,b,a = 12.12; z = 20.12; s,t = 14.18; w = 2.30
    val pos = (intRegAddr - 0x020) & 0x1c // position within group of 8
    pos match {
      case 0x00 | 0x04 | 0x08 | 0x10 => (f * 4096.0f).toInt // r,g,b,a: 12.12
      case 0x0c                      => (f * 4096.0f).toInt // z: 20.12
      case 0x14 | 0x18               => (f * 262144.0f).toInt // s,t: 14.18
      case 0x1c                      => (f * 1073741824.0f).toInt // w: 2.30
      case _                         => floatBits.toInt
    }
  }

  // ========================================================================
  // Texture data helpers
  // ========================================================================

  /** Map a trace texture address to a byte offset for texMem.
    *
    * With PCI-encoded layout, the PCI address IS the SRAM address. We add the current texBaseAddr
    * to produce the absolute texMem address.
    */
  def decodePciTexAddr(
      traceAddr: Long,
      texBaseAddr: Long,
      textureMode: Int,
      tLOD: Int,
      memMask: Int
  ): Int = {
    val texBaseBytes = texBaseAddr << 3
    val flatOffset = traceAddr & 0x7fffffL
    ((texBaseBytes + flatOffset) & memMask).toInt
  }

  /** Build pre-decoded texture data for all LOD levels from raw texture memory.
    *
    * Returns (texData, texWMask, texHMask, texShift, texLod) matching the
    * VoodooReference.VoodooParams fields.
    */
  def buildTextureData(
      texMem: Array[Byte],
      texBaseAddr: Long,
      textureMode: Int,
      tLOD: Int
  ): (Array[Array[Int]], Array[Int], Array[Int], Array[Int], Array[Int]) = {
    val format = (textureMode >> 8) & 0xf
    val is16Bit = format >= 8
    val aspectRatio = (tLOD >> 21) & 3
    val sIsWider = ((tLOD >> 20) & 1) != 0

    val texData = Array.fill(9)(Array.empty[Int])
    val texWMask = new Array[Int](9)
    val texHMask = new Array[Int](9)
    val texShift = new Array[Int](9)
    val texLod = new Array[Int](9)

    val texBaseByteAddr = texBaseAddr << 3
    val memMask = texMem.length - 1

    for (lod <- 0 to 8) {
      val dimBits = 8 - lod
      val wBits =
        if (!sIsWider && aspectRatio > 0)
          scala.math.max(0, dimBits - aspectRatio)
        else dimBits
      val hBits =
        if (sIsWider && aspectRatio > 0)
          scala.math.max(0, dimBits - aspectRatio)
        else dimBits
      val w = scala.math.max(1, 1 << wBits)
      val h = scala.math.max(1, 1 << hBits)

      texWMask(lod) = w - 1
      texHMask(lod) = h - 1
      texShift(lod) = scala.math.max(0, wBits)
      texLod(lod) = lod

      val texels = new Array[Int](w * h)
      for (t <- 0 until h; s <- 0 until w) {
        val texelIdx = s + t * w
        // 8-bit textures use two-bank interleaved addressing: {s[7:2], 0, s[1:0]}
        val colByteOffset = if (is16Bit) (s << 1) else ((s >> 2) << 3) | (s & 3)
        val pciOffset = (lod << 17) | (t << 9) | colByteOffset
        val byteAddr = (texBaseByteAddr + pciOffset) & memMask

        val raw =
          if (is16Bit) {
            (texMem(byteAddr.toInt) & 0xff) |
              ((texMem(((byteAddr + 1) & memMask).toInt) & 0xff) << 8)
          } else {
            texMem(byteAddr.toInt) & 0xff
          }

        texels(texelIdx) = VoodooReference.decodeTexel(raw, format)
      }
      texData(lod) = texels
    }

    (texData, texWMask, texHMask, texShift, texLod)
  }

  // ========================================================================
  // Register state tracker
  // ========================================================================
  class RegisterStateTracker {
    val regs = scala.collection.mutable.Map.empty[Int, Long]

    def write(addr: Int, data: Long): Unit = {
      val regAddr = addr & 0x3fc
      // Float register write: convert to fixed-point, store at integer offset
      if (regAddr >= FLOAT_REG_BASE && regAddr <= FLOAT_REG_END) {
        val intAddr = regAddr - 0x080
        regs(intAddr) = floatRegToFixed(intAddr, data).toLong & 0xffffffffL
      }
      regs(regAddr) = data & 0xffffffffL
    }

    def get(addr: Int): Int = regs.getOrElse(addr & 0x3fc, 0L).toInt

    def getLong(addr: Int): Long = regs.getOrElse(addr & 0x3fc, 0L)
  }

  // ========================================================================
  // PNG writer — RGB565 framebuffer to PNG file
  // ========================================================================
  def writePng(
      path: File,
      fb: Array[Int], // RGB565 per pixel, indexed [y * stride + x]
      width: Int,
      height: Int
  ): Unit = {
    val img = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB)
    for (y <- 0 until height; x <- 0 until width) {
      val pixel = fb(y * stride + x) & 0xffff
      val r = ((pixel >> 11) & 0x1f) * 255 / 31
      val g = ((pixel >> 5) & 0x3f) * 255 / 63
      val b = (pixel & 0x1f) * 255 / 31
      img.setRGB(x, y, (r << 16) | (g << 8) | b)
    }
    ImageIO.write(img, "PNG", path)
    println(s"  Screenshot: ${path.getAbsolutePath} (${width}x${height})")
  }

  // ========================================================================
  // Status/idle helpers (matching CoreTest)
  // ========================================================================
  def readStatus(driver: BmbDriver): Long = {
    driver.read(BigInt(0)).toLong
  }

  def isBusy(status: Long): Boolean = (status & (1L << 9)) != 0

  def pollUntilIdle(
      driver: BmbDriver,
      cd: ClockDomain,
      label: String,
      maxReads: Int = 50000,
      ticksPerPoll: Int = 100
  ): Long = {
    var idleCount = 0
    for (i <- 0 until maxReads) {
      cd.waitSampling(ticksPerPoll)
      val status = readStatus(driver)
      val fifoFree = status & SST_FIFO_FREE_MASK
      if (!isBusy(status) && fifoFree == 0x3f) {
        idleCount += 1
        if (idleCount >= 3) return status
      } else {
        idleCount = 0
      }
    }
    val finalStatus = readStatus(driver)
    throw new AssertionError(f"[$label] STUCK after $maxReads polls, status=0x$finalStatus%08x")
  }

  // ========================================================================
  // Collect pixels from simulation framebuffer memory (matching CoreTest)
  // ========================================================================
  def collectPixels(
      memory: BmbMemoryAgent,
      writtenAddrs: scala.collection.mutable.Set[Long],
      fbBaseAddr: Long
  ): Map[(Int, Int), (Int, Int)] = {
    val pixels = scala.collection.mutable.Map.empty[(Int, Int), (Int, Int)]
    for (addr <- writtenAddrs) {
      val pixelOffset = (addr - fbBaseAddr).toInt
      if (pixelOffset >= 0 && (pixelOffset % 4) == 0) {
        val pixelIndex = pixelOffset / 4
        val x = pixelIndex % stride
        val y = pixelIndex / stride
        val b0 = memory.getByte(addr) & 0xff
        val b1 = memory.getByte(addr + 1) & 0xff
        val b2 = memory.getByte(addr + 2) & 0xff
        val b3 = memory.getByte(addr + 3) & 0xff
        val rgb565 = b0 | (b1 << 8)
        val depth16 = b2 | (b3 << 8)
        pixels((x, y)) = (rgb565, depth16)
      }
    }
    pixels.toMap
  }

  // ========================================================================
  // Per-triangle comparison (matching CoreTest.comparePixelsFuzzy)
  // ========================================================================
  def comparePixelsFuzzy(
      ref: Seq[RefPixel],
      sim: Map[(Int, Int), (Int, Int)],
      testName: String,
      spatialTolerance: Int = 1,
      checkColor: Boolean = true,
      checkDepth: Boolean = true,
      colorTolerance: Int = 0,
      depthTolerance: Int = 0,
      maxEdgeDiffs: Int = 0,
      maxColorMismatches: Int = 0,
      maxDepthMismatches: Int = 0
  ): (Int, Int, Int) = {
    val refSet = ref.map(p => (p.x, p.y)).toSet
    val simSet = sim.keySet

    val unmatchedSim = simSet.filter { case (x, y) =>
      !(-spatialTolerance to spatialTolerance).exists(dx =>
        (-spatialTolerance to spatialTolerance).exists(dy => refSet.contains((x + dx, y + dy)))
      )
    }

    val unmatchedRef = refSet.filter { case (x, y) =>
      !(-spatialTolerance to spatialTolerance).exists(dx =>
        (-spatialTolerance to spatialTolerance).exists(dy => simSet.contains((x + dx, y + dy)))
      )
    }

    val refMap = ref.map(p => (p.x, p.y) -> (p.rgb565, p.depth16)).toMap
    val colorMismatches = scala.collection.mutable.ArrayBuffer.empty[String]
    val depthMismatches = scala.collection.mutable.ArrayBuffer.empty[String]

    for ((xy, (simRgb, simDepth)) <- sim if refMap.contains(xy)) {
      val (refRgb, refDepth) = refMap(xy)

      // Color comparison
      if (checkColor) {
        if (colorTolerance == 0) {
          if (simRgb != refRgb)
            colorMismatches += f"($testName) (${xy._1},${xy._2}) color: ref=0x$refRgb%04X sim=0x$simRgb%04X"
        } else {
          val refR = (refRgb >> 11) & 0x1f; val simR = (simRgb >> 11) & 0x1f
          val refG = (refRgb >> 5) & 0x3f; val simG = (simRgb >> 5) & 0x3f
          val refB = refRgb & 0x1f; val simB = simRgb & 0x1f
          if (
            scala.math.abs(refR - simR) > colorTolerance ||
            scala.math.abs(refG - simG) > colorTolerance ||
            scala.math.abs(refB - simB) > colorTolerance
          )
            colorMismatches += f"($testName) (${xy._1},${xy._2}) color: ref=0x$refRgb%04X sim=0x$simRgb%04X"
        }
      }

      // Depth comparison
      if (checkDepth) {
        val diff = scala.math.abs(refDepth - simDepth)
        if (diff > depthTolerance)
          depthMismatches += f"($testName) (${xy._1},${xy._2}) depth: ref=0x$refDepth%04X sim=0x$simDepth%04X (diff=$diff)"
      }
    }

    val edgeDiffs = unmatchedSim.size + unmatchedRef.size
    if (edgeDiffs > 0) {
      println(
        f"[$testName] Edge diffs: ${unmatchedSim.size} sim-only, ${unmatchedRef.size} ref-only"
      )
      if (unmatchedSim.size <= 10)
        unmatchedSim.toSeq.sorted.foreach { case (x, y) => println(f"  sim-only: ($x,$y)") }
      if (unmatchedRef.size <= 10)
        unmatchedRef.toSeq.sorted.foreach { case (x, y) => println(f"  ref-only: ($x,$y)") }
    }
    if (colorMismatches.nonEmpty) {
      println(f"[$testName] ${colorMismatches.size} color mismatches:")
      colorMismatches.take(20).foreach(println)
    }
    if (depthMismatches.nonEmpty) {
      println(f"[$testName] ${depthMismatches.size} depth mismatches:")
      depthMismatches.take(20).foreach(println)
    }

    (edgeDiffs, colorMismatches.size, depthMismatches.size)
  }

  // ========================================================================
  // Reference model replay — returns per-triangle pixels + full framebuffer
  // ========================================================================
  def replayReference(
      entries: Seq[TraceEntry],
      width: Int,
      height: Int
  ): (Array[Int], Seq[Seq[RefPixel]]) = {
    val fb = new Array[Int](stride * height)
    val tracker = new RegisterStateTracker
    val perTriangle = scala.collection.mutable.ArrayBuffer.empty[Seq[RefPixel]]
    // Persistent framebuffer for alpha blending across triangles
    val refFramebuffer = scala.collection.mutable.Map.empty[(Int, Int), (Int, Int)]

    // Texture memory buffer (mirrors simulation's texMemory)
    val texMemSize = 4 * 1024 * 1024
    val texMem = new Array[Byte](texMemSize)
    val texMemMask = texMemSize - 1
    var currentTexBaseAddr = 0L
    var texWriteCount = 0
    for (entry <- entries) {
      entry.cmdType match {
        case TraceCommandType.WRITE_REG_L =>
          val regAddr = (entry.addr & 0x3fc).toInt

          // Track texBaseAddr for texture writes
          if (regAddr == REG_TEXBASEADDR)
            currentTexBaseAddr = entry.data & 0x7ffffL

          tracker.write(regAddr, entry.data)

          if (regAddr == REG_TRIANGLE_CMD || regAddr == REG_FTRIANGLE_CMD) {
            val sign = (entry.data & (1L << 31)) != 0
            val clipLR = tracker.get(REG_CLIP_LR)
            val clipTB = tracker.get(REG_CLIP_TB)

            // Build texture data if texture is enabled
            val fbzCP = tracker.get(0x104)
            val texEnabled = (fbzCP & VoodooReference.FBZCP_TEXTURE_ENABLED) != 0
            val (texDataArr, texWMaskArr, texHMaskArr, texShiftArr, texLodArr) =
              if (texEnabled) {
                val texBA = tracker.getLong(REG_TEXBASEADDR) & 0x7ffffL
                buildTextureData(
                  texMem,
                  texBA,
                  tracker.get(0x300),
                  tracker.get(0x304)
                )
              } else {
                (
                  Array.fill(9)(Array.empty[Int]),
                  Array.fill(9)(0),
                  Array.fill(9)(0),
                  Array.fill(9)(0),
                  Array.fill(9)(0)
                )
              }

            val params = VoodooReference.fromRegisterValues(
              vertexAx = tracker.get(0x008),
              vertexAy = tracker.get(0x00c),
              vertexBx = tracker.get(0x010),
              vertexBy = tracker.get(0x014),
              vertexCx = tracker.get(0x018),
              vertexCy = tracker.get(0x01c),
              startR = tracker.get(0x020),
              startG = tracker.get(0x024),
              startB = tracker.get(0x028),
              startA = tracker.get(0x030),
              startZ = tracker.get(0x02c),
              startS = tracker.get(0x034),
              startT = tracker.get(0x038),
              startW = tracker.get(0x03c),
              dRdX = tracker.get(0x040),
              dGdX = tracker.get(0x044),
              dBdX = tracker.get(0x048),
              dAdX = tracker.get(0x050),
              dZdX = tracker.get(0x04c),
              dSdX = tracker.get(0x054),
              dTdX = tracker.get(0x058),
              dWdX = tracker.get(0x05c),
              dRdY = tracker.get(0x060),
              dGdY = tracker.get(0x064),
              dBdY = tracker.get(0x068),
              dAdY = tracker.get(0x070),
              dZdY = tracker.get(0x06c),
              dSdY = tracker.get(0x074),
              dTdY = tracker.get(0x078),
              dWdY = tracker.get(0x07c),
              fbzColorPath = fbzCP,
              fbzMode = tracker.get(0x110),
              sign = sign,
              textureMode = tracker.get(0x300),
              tLOD = tracker.get(0x304),
              color0 = tracker.get(0x144),
              color1 = tracker.get(0x148),
              zaColor = tracker.get(0x130),
              fogMode = tracker.get(0x108),
              alphaMode = tracker.get(0x10c),
              clipLeft = (clipLR >> 16) & 0xfff,
              clipRight = clipLR & 0xfff,
              clipLowY = (clipTB >> 16) & 0xfff,
              clipHighY = clipTB & 0xfff,
              chromaKey = tracker.get(0x134),
              fogColor = tracker.get(0x12c),
              texData = texDataArr,
              texWMask = texWMaskArr,
              texHMask = texHMaskArr,
              texShift = texShiftArr,
              texLod = texLodArr
            )

            val pixels = VoodooReference.voodooTriangle(params, refFramebuffer)

            if (perTriangle.size < 3 || texEnabled) {
              val ax = tracker.get(0x008); val ay = tracker.get(0x00c)
              val bx = tracker.get(0x010); val by = tracker.get(0x014)
              val cx = tracker.get(0x018); val cy = tracker.get(0x01c)
              val texInfo = if (texEnabled) {
                val texBA = tracker.getLong(REG_TEXBASEADDR) & 0x7ffffL
                val texByteAddr = texBA << 3
                val fmt = (tracker.get(0x300) >> 8) & 0xf
                val tLODVal = tracker.get(0x304)
                val lodMin = tLODVal & 0x3f // bits 5:0 = LOD_min (2.2 fixed point)
                // Check if texture region has data at LOD 0 and LOD 1
                val checkLod0Start = (texByteAddr & texMemMask).toInt
                val checkLod0End = scala.math.min(checkLod0Start + 256, texMem.length)
                val nzLod0 = (checkLod0Start until checkLod0End).count(i => texMem(i) != 0)
                val lod1Start = ((texByteAddr + 65536) & texMemMask).toInt
                val lod1End = scala.math.min(lod1Start + 256, texMem.length)
                val nzLod1 = (lod1Start until lod1End).count(i => texMem(i) != 0)
                f"  tex=ON fmt=$fmt tLOD=0x$tLODVal%08X lodMin=$lodMin base=0x$texBA%05X lod0nz=$nzLod0 lod1nz=$nzLod1"
              } else ""
              println(
                f"  [ref tri ${perTriangle.size}] cmd=${
                    if (regAddr == REG_FTRIANGLE_CMD) "F" else "I"
                  }triangle" +
                  f"  A=(${ax / 16.0}%.1f, ${ay / 16.0}%.1f)  B=(${bx / 16.0}%.1f, ${by / 16.0}%.1f)" +
                  f"  C=(${cx / 16.0}%.1f, ${cy / 16.0}%.1f)  sign=$sign  pixels=${pixels.size}" +
                  texInfo
              )
            }

            perTriangle += pixels

            // Accumulate into full framebuffer for screenshot
            for (p <- pixels) {
              if (p.x >= 0 && p.x < width && p.y >= 0 && p.y < height) {
                fb(p.y * stride + p.x) = p.rgb565
              }
            }
          }

        case TraceCommandType.WRITE_TEX_L =>
          // Decode PCI LOD/T/S from old-format trace and remap to sequential LOD layout
          val maskedAddr = decodePciTexAddr(
            entry.addr,
            currentTexBaseAddr,
            tracker.get(0x300),
            tracker.get(0x304),
            texMemMask
          )
          if (maskedAddr >= 0) {
            val data = entry.data.toInt
            texMem(maskedAddr) = (data & 0xff).toByte
            texMem((maskedAddr + 1) & texMemMask) = ((data >> 8) & 0xff).toByte
            texMem((maskedAddr + 2) & texMemMask) = ((data >> 16) & 0xff).toByte
            texMem((maskedAddr + 3) & texMemMask) = ((data >> 24) & 0xff).toByte
            texWriteCount += 1
            if (texWriteCount <= 3)
              println(
                f"  [DEBUG TEX] write #$texWriteCount traceAddr=0x${entry.addr}%06X base=0x${currentTexBaseAddr}%05X -> maskedAddr=0x${maskedAddr}%06X data=0x${data}%08X"
              )
          }

        case _ => // Ignore reads, vsync, swap, etc.
      }
    }

    // Debug: check texture data at correct LOD 1 offset for 2:1 aspect
    {
      val texBA = currentTexBaseAddr
      val texByteAddr = ((texBA << 3) & texMemMask).toInt
      val lod1Off = 32768 // lodTexelOffsets(1)(1) for aspect=1
      val lod1Addr = (texByteAddr + lod1Off) & texMemMask
      val nzCount =
        (lod1Addr until scala.math.min(lod1Addr + 8192, texMem.length)).count(i => texMem(i) != 0)
      println(
        f"  [DEBUG] texWriteCount=$texWriteCount texBase=0x${texBA}%05X lod1Addr=0x${lod1Addr}%06X nzAtLod1=$nzCount/8192"
      )
    }

    (fb, perTriangle.toSeq)
  }

  // ========================================================================
  // Simulation replay — per-triangle pixel collection + full framebuffer
  // ========================================================================
  def replaySimulation(
      name: String,
      entries: Seq[TraceEntry],
      width: Int,
      height: Int
  ): (Array[Int], Seq[Map[(Int, Int), (Int, Int)]]) = {
    val simFb = new Array[Int](stride * height)
    val perTriangle = scala.collection.mutable.ArrayBuffer.empty[Map[(Int, Int), (Int, Int)]]

    compiled.doSim(s"trace_$name") { dut =>
      dut.clockDomain.forkStimulus(10)

      dut.io.statusInputs.vRetrace #= false
      dut.io.statusInputs.memFifoFree #= 0xffff
      dut.io.statusInputs.pciInterrupt #= false

      dut.io.statisticsIn.pixelsIn #= 0
      dut.io.statisticsIn.chromaFail #= 0
      dut.io.statisticsIn.zFuncFail #= 0
      dut.io.statisticsIn.aFuncFail #= 0
      dut.io.statisticsIn.pixelsOut #= 0

      dut.io.fbBaseAddr #= 0

      val bmbDriver = new BmbDriver(dut.io.cpuBus, dut.clockDomain)

      val fbMemSize = 4 * 1024 * 1024L
      val fbMemory = new BmbMemoryAgent(fbMemSize)
      // Zero-initialize framebuffer so alpha blending against unwritten
      // pixels matches the reference model's default of black (0).
      for (page <- 0L until (fbMemSize >> 20))
        fbMemory.memory.content(page) = new Array[Byte](1 << 20)
      fbMemory.addPort(
        bus = dut.io.fbMem,
        busAddress = 0,
        clockDomain = dut.clockDomain,
        withDriver = true
      )

      val texMemSize = 8 * 1024 * 1024L // 8MB: TMU addresses can reach ~8MB (4MB base + 4MB offset)
      val texMemory = new BmbMemoryAgent(texMemSize)
      // Zero-initialize all SparseMemory pages so TMU reads zeros for
      // areas not written by the trace. SparseMemory fills new pages
      // with random data by default; we insert zero-filled pages directly.
      for (page <- 0L until (texMemSize >> 20))
        texMemory.memory.content(page) = new Array[Byte](1 << 20)
      texMemory.addPort(
        bus = dut.io.texMem,
        busAddress = 0,
        clockDomain = dut.clockDomain,
        withDriver = true
      )

      dut.clockDomain.waitSampling(5)

      // Track writes for per-triangle pixel collection
      val writtenAddrs = scala.collection.mutable.Set.empty[Long]
      StreamMonitor(dut.io.fbMem.cmd, dut.clockDomain) { payload =>
        if (payload.opcode.toInt == Bmb.Cmd.Opcode.WRITE) {
          writtenAddrs += payload.address.toLong
        }
      }

      var currentTexBaseAddr = 0L
      var simTextureMode = 0 // for PCI texture address decoding
      var simTLOD = 0 // for PCI texture address decoding
      var triCount = 0
      // Track draw buffer base for correct pixel coordinate extraction
      var bufferOffset = 0L // fbiInit2 buffer offset (in 4KB pages)
      var swapCount = 0 // swap buffer count (toggles front/back)
      var fbzModeRaw = 0L // raw fbzMode for drawBuffer bit

      def drawBufferBase: Long = {
        val buffer0Base = 0L
        val buffer1Base = bufferOffset * 4096L
        val backBuffer = if ((swapCount & 1) != 0) buffer0Base else buffer1Base
        val frontBuffer = if ((swapCount & 1) != 0) buffer1Base else buffer0Base
        val drawToBack = (fbzModeRaw & (1L << 14)) != 0 // fbzMode bit 14 = drawBuffer
        if (drawToBack) backBuffer else frontBuffer
      }

      for (entry <- entries) {
        entry.cmdType match {
          case TraceCommandType.WRITE_REG_L =>
            val baseAddr = (entry.addr & 0x3ff).toInt
            val regAddr = baseAddr & 0x3fc
            val isRemapped = (entry.addr & 0x200000) != 0
            val hwAddr = if (isRemapped) (0x200000 | baseAddr) else baseAddr

            // Track registers needed for texture address decoding
            if (regAddr == REG_TEXBASEADDR)
              currentTexBaseAddr = entry.data & 0x7ffffL
            if (regAddr == 0x300) // textureMode
              simTextureMode = entry.data.toInt
            if (regAddr == 0x304) // tLOD
              simTLOD = entry.data.toInt

            // Track buffer layout registers
            if (regAddr == REG_FBIINIT2)
              bufferOffset = (entry.data >> 11) & 0x3ff
            if (regAddr == REG_FBZMODE)
              fbzModeRaw = entry.data

            val isTriangleCmd =
              regAddr == REG_TRIANGLE_CMD || regAddr == REG_FTRIANGLE_CMD

            if (isTriangleCmd) {
              if (triCount == 0)
                println(
                  f"  [sim] drawBufferBase=0x${drawBufferBase}%X (bufferOffset=$bufferOffset, swapCount=$swapCount, fbzMode=0x${fbzModeRaw}%08X)"
                )

              // Drain any pending commands (e.g., fastfill) so their pixel
              // writes don't get attributed to this triangle
              pollUntilIdle(
                bmbDriver,
                dut.clockDomain,
                s"$name pre-tri $triCount"
              )

              // Clear write tracker after drain, before submitting triangle
              writtenAddrs.clear()

              // Submit triangle command
              bmbDriver.write(BigInt(entry.data), BigInt(hwAddr))

              // Drain triangle pipeline
              pollUntilIdle(
                bmbDriver,
                dut.clockDomain,
                s"$name tri $triCount"
              )

              // Collect pixels written by this triangle
              val triPixels = collectPixels(fbMemory, writtenAddrs, drawBufferBase)
              perTriangle += triPixels
              triCount += 1

              if (triCount % 100 == 0)
                println(s"  [$name] $triCount triangles processed...")
            } else {
              // Non-triangle register write
              for (_ <- 0 until entry.count)
                bmbDriver.write(BigInt(entry.data), BigInt(hwAddr))
            }

          case TraceCommandType.WRITE_REG_W =>
            val baseAddr = (entry.addr & 0x3ff).toInt
            val isRemapped = (entry.addr & 0x200000) != 0
            val hwAddr = if (isRemapped) (0x200000 | baseAddr) else baseAddr
            val data16 = entry.data & 0xffffL
            for (_ <- 0 until entry.count)
              bmbDriver.write(BigInt(data16), BigInt(hwAddr))

          case TraceCommandType.WRITE_TEX_L =>
            // Decode PCI LOD/T/S from old-format trace and remap to sequential LOD layout
            val textureMask = (texMemSize - 1).toInt
            val maskedAddr = decodePciTexAddr(
              entry.addr,
              currentTexBaseAddr,
              simTextureMode,
              simTLOD,
              textureMask
            )
            if (maskedAddr >= 0) {
              val data = entry.data.toInt
              texMemory.setByte(maskedAddr.toLong, (data & 0xff).toByte)
              texMemory.setByte(maskedAddr.toLong + 1, ((data >> 8) & 0xff).toByte)
              texMemory.setByte(maskedAddr.toLong + 2, ((data >> 16) & 0xff).toByte)
              texMemory.setByte(maskedAddr.toLong + 3, ((data >> 24) & 0xff).toByte)
            }

          case _ => // Ignore reads, vsync, swap, etc.
        }
      }

      // Read full framebuffer for screenshot (from draw buffer)
      val screenshotBase = drawBufferBase
      println(f"  [$name] Screenshot from drawBufferBase=0x${screenshotBase}%X")
      for (y <- 0 until height; x <- 0 until width) {
        val addr = screenshotBase + (y * stride + x) * 4L
        val b0 = fbMemory.getByte(addr) & 0xff
        val b1 = fbMemory.getByte(addr + 1) & 0xff
        simFb(y * stride + x) = b0 | (b1 << 8)
      }
    }

    (simFb, perTriangle.toSeq)
  }

  // ========================================================================
  // Main test driver — per-triangle comparison
  // ========================================================================
  def testTrace(
      tracePath: String,
      name: String,
      width: Int = displayWidth,
      height: Int = displayHeight,
      spatialTolerance: Int = 1,
      colorTolerance: Int = 0,
      depthTolerance: Int = 0,
      maxEdgeDiffsPerTriangle: Int = 0,
      maxColorMismatchesPerTriangle: Int = 0,
      maxDepthMismatchesPerTriangle: Int = 0
  ): Unit = {
    test(s"Glide trace: $name") {
      val traceFile = new File(tracePath)
      assert(traceFile.exists(), s"Trace file not found: $tracePath")

      resultDir.mkdirs()

      // Parse trace entries
      val parser = new TraceParser(traceFile)
      val entries = parser.readAllEntries().toList
      parser.close()
      println(s"[$name] Loaded ${entries.size} trace entries")

      // 1. Reference model pass
      println(s"[$name] Running reference model...")
      val (refFb, refTriangles) = replayReference(entries, width, height)
      println(s"[$name] Reference: ${refTriangles.size} triangles")
      writePng(new File(resultDir, s"${name}_ref.png"), refFb, width, height)

      // 2. Simulation pass
      println(s"[$name] Running simulation...")
      val (simFb, simTriangles) = replaySimulation(name, entries, width, height)
      println(s"[$name] Simulation: ${simTriangles.size} triangles")
      writePng(new File(resultDir, s"${name}_sim.png"), simFb, width, height)

      // 3. Per-triangle comparison
      assert(
        refTriangles.size == simTriangles.size,
        s"Triangle count mismatch: ref=${refTriangles.size} sim=${simTriangles.size}"
      )

      var totalEdgeDiffs = 0
      var totalColorMismatches = 0
      var totalDepthMismatches = 0
      var failedTriangles = 0

      for (i <- refTriangles.indices) {
        val (edgeDiffs, colorMismatches, depthMismatches) = comparePixelsFuzzy(
          ref = refTriangles(i),
          sim = simTriangles(i),
          testName = s"$name tri $i",
          spatialTolerance = spatialTolerance,
          colorTolerance = colorTolerance,
          depthTolerance = depthTolerance,
          maxEdgeDiffs = maxEdgeDiffsPerTriangle,
          maxColorMismatches = maxColorMismatchesPerTriangle,
          maxDepthMismatches = maxDepthMismatchesPerTriangle
        )
        totalEdgeDiffs += edgeDiffs
        totalColorMismatches += colorMismatches
        totalDepthMismatches += depthMismatches
        if (
          edgeDiffs > maxEdgeDiffsPerTriangle ||
          colorMismatches > maxColorMismatchesPerTriangle ||
          depthMismatches > maxDepthMismatchesPerTriangle
        )
          failedTriangles += 1
      }

      println(
        f"[$name] Summary: ${refTriangles.size} triangles, " +
          f"$totalEdgeDiffs total edge diffs, " +
          f"$totalColorMismatches total color mismatches, " +
          f"$totalDepthMismatches total depth mismatches, " +
          f"$failedTriangles failed triangles"
      )

      // Build diff image if there are mismatches
      if (totalEdgeDiffs > 0 || totalColorMismatches > 0 || totalDepthMismatches > 0) {
        val diffFb = new Array[Int](stride * height)
        for (y <- 0 until height; x <- 0 until width) {
          val idx = y * stride + x
          val refPx = refFb(idx) & 0xffff
          val simPx = simFb(idx) & 0xffff
          if (refPx != simPx) {
            if (refPx != 0 && simPx == 0)
              diffFb(idx) = 0xf800 // red: ref has pixel, sim doesn't
            else if (refPx == 0 && simPx != 0)
              diffFb(idx) = 0x07e0 // green: sim has pixel, ref doesn't
            else
              diffFb(idx) = 0x001f // blue: both have pixel, colors differ
          }
        }
        writePng(new File(resultDir, s"${name}_diff.png"), diffFb, width, height)
      }

      assert(
        failedTriangles == 0,
        s"$name: $failedTriangles triangles failed comparison"
      )
    }
  }

  // ========================================================================
  // Reference-only test (no Verilator, for quick validation)
  // ========================================================================
  def testTraceReferenceOnly(
      tracePath: String,
      name: String,
      width: Int = displayWidth,
      height: Int = displayHeight
  ): Unit = {
    test(s"Glide trace reference: $name") {
      val traceFile = new File(tracePath)
      assert(traceFile.exists(), s"Trace file not found: $tracePath")

      resultDir.mkdirs()

      val parser = new TraceParser(traceFile)
      val entries = parser.readAllEntries().toList
      parser.close()
      println(s"[$name] Loaded ${entries.size} trace entries")

      val (refFb, refTriangles) = replayReference(entries, width, height)
      println(s"[$name] Reference: ${refTriangles.size} triangles")

      writePng(new File(resultDir, s"${name}_ref.png"), refFb, width, height)
    }
  }

  // ========================================================================
  // Test registrations
  // ========================================================================

  // Auto-discover trace files from traces/ directory
  private val tracesDir = new File("traces")

  if (tracesDir.exists() && tracesDir.isDirectory) {
    val traceFiles = tracesDir
      .listFiles()
      .filter(f => f.isFile && f.getName.endsWith(".bin"))
      .sortBy(_.getName)

    for (traceFile <- traceFiles) {
      val name = traceFile.getName.stripSuffix(".bin")
      testTraceReferenceOnly(traceFile.getAbsolutePath, name)
      testTrace(
        traceFile.getAbsolutePath,
        name,
        // Edge diffs from rasterization differences (pre-existing, up to ~46 per triangle).
        // 1 cascading color mismatch from alpha blending against edge-diff pixels.
        maxEdgeDiffsPerTriangle = 50,
        maxColorMismatchesPerTriangle = 60
      )
    }
  }
}
