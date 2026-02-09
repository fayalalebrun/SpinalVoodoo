package voodoo.integration

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim._
import spinal.lib.sim.StreamMonitor
import org.scalatest.funsuite.AnyFunSuite
import voodoo._
import voodoo.ref.VoodooReference
import voodoo.ref.VoodooReference.{VoodooParams, RefPixel}

class CoreIntegrationTest extends AnyFunSuite {

  val voodooConfig = Config.voodoo1()

  // Compile once, reuse across tests
  lazy val compiled = SimConfig.withVerilator.compile(Core(voodooConfig))

  // ========================================================================
  // Register addresses (from RegisterBank.scala)
  // ========================================================================
  val REG_VERTEX_AX = 0x008
  val REG_VERTEX_AY = 0x00c
  val REG_VERTEX_BX = 0x010
  val REG_VERTEX_BY = 0x014
  val REG_VERTEX_CX = 0x018
  val REG_VERTEX_CY = 0x01c
  val REG_START_R = 0x020
  val REG_START_G = 0x024
  val REG_START_B = 0x028
  val REG_START_Z = 0x02c
  val REG_START_A = 0x030
  val REG_START_S = 0x034
  val REG_START_T = 0x038
  val REG_START_W = 0x03c
  val REG_DRDX = 0x040
  val REG_DGDX = 0x044
  val REG_DBDX = 0x048
  val REG_DZDX = 0x04c
  val REG_DADX = 0x050
  val REG_DSDX = 0x054
  val REG_DTDX = 0x058
  val REG_DWDX = 0x05c
  val REG_DRDY = 0x060
  val REG_DGDY = 0x064
  val REG_DBDY = 0x068
  val REG_DZDY = 0x06c
  val REG_DADY = 0x070
  val REG_DSDY = 0x074
  val REG_DTDY = 0x078
  val REG_DWDY = 0x07c
  val REG_TRIANGLE_CMD = 0x080
  val REG_FBZCOLORPATH = 0x104
  val REG_FOGMODE = 0x108
  val REG_ALPHAMODE = 0x10c
  val REG_FBZMODE = 0x110
  val REG_CLIP_LR = 0x118
  val REG_CLIP_TB = 0x11c
  val REG_ZACOLOR = 0x130
  val REG_CHROMAKEY = 0x134
  val REG_COLOR0 = 0x144
  val REG_COLOR1 = 0x148
  val REG_TEXTUREMODE = 0x300
  val REG_TLOD = 0x304
  val REG_TEXBASEADDR = 0x30c

  // ========================================================================
  // Texture format mapping table
  // ========================================================================
  case class TexFormatInfo(
      name: String,
      hwFormat: Int,   // textureMode[11:8] for SpinalVoodoo hardware
      refFormat: Int,  // decodeTexel() format code (86Box convention)
      bpp: Int,        // bytes per texel (1 or 2)
      encode: (Int, Int, Int, Int) => Int  // (R8, G8, B8, A8) => raw texel
  )

  val allFormats: Seq[TexFormatInfo] = Seq(
    TexFormatInfo("RGB332", 0, 0x0, 1, (r, g, b, _) =>
      ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6)),
    TexFormatInfo("A8", 2, 0x2, 1, (_, _, _, a) => a),
    TexFormatInfo("I8", 3, 0x3, 1, (r, _, _, _) => r),  // intensity = R channel
    TexFormatInfo("AI44", 4, 0x4, 1, (r, _, _, a) =>
      ((a >> 4) << 4) | (r >> 4)),  // A4:I4
    TexFormatInfo("ARGB8332", 8, 0x8, 2, (r, g, b, a) =>
      (a << 8) | ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6)),
    TexFormatInfo("RGB565", 10, 0xa, 2, (r, g, b, _) =>
      ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)),
    TexFormatInfo("ARGB1555", 11, 0xb, 2, (r, g, b, a) =>
      ((if (a >= 128) 1 else 0) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)),
    TexFormatInfo("ARGB4444", 12, 0xc, 2, (r, g, b, a) =>
      ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4)),
    TexFormatInfo("AI88", 13, 0xd, 2, (r, _, _, a) =>
      (a << 8) | r)  // A8:I8, intensity = R channel
  )

  // ========================================================================
  // Helper: write a single register via BmbDriver
  // ========================================================================
  def writeReg(driver: BmbDriver, addr: Int, data: Long): Unit = {
    driver.write(BigInt(data & 0xffffffffL), BigInt(addr))
  }

  // ========================================================================
  // Helper: submit a triangle by writing all registers then triangleCMD
  // ========================================================================
  def submitTriangle(
      driver: BmbDriver,
      cd: ClockDomain,
      vertexAx: Int,
      vertexAy: Int,
      vertexBx: Int,
      vertexBy: Int,
      vertexCx: Int,
      vertexCy: Int,
      startR: Int,
      startG: Int,
      startB: Int,
      startA: Int,
      startZ: Int,
      startS: Int,
      startT: Int,
      startW: Int,
      dRdX: Int,
      dGdX: Int,
      dBdX: Int,
      dAdX: Int,
      dZdX: Int,
      dSdX: Int,
      dTdX: Int,
      dWdX: Int,
      dRdY: Int,
      dGdY: Int,
      dBdY: Int,
      dAdY: Int,
      dZdY: Int,
      dSdY: Int,
      dTdY: Int,
      dWdY: Int,
      fbzColorPath: Int,
      fbzMode: Int,
      sign: Boolean,
      textureMode: Int = 0,
      tLOD: Int = 0,
      color0: Int = 0,
      color1: Int = 0,
      zaColor: Int = 0,
      fogMode: Int = 0,
      alphaMode: Int = 0,
      chromaKey: Int = 0,
      clipLeft: Int = 0,
      clipRight: Int = 0x3ff,
      clipLowY: Int = 0,
      clipHighY: Int = 0x3ff
  ): Unit = {
    // Config registers - these go through the FIFO
    writeReg(driver, REG_FBZCOLORPATH, fbzColorPath)
    writeReg(driver, REG_FBZMODE, fbzMode)
    writeReg(driver, REG_FOGMODE, fogMode)
    writeReg(driver, REG_ALPHAMODE, alphaMode)
    writeReg(driver, REG_TEXTUREMODE, textureMode)
    writeReg(driver, REG_TLOD, tLOD)
    writeReg(driver, REG_COLOR0, color0)
    writeReg(driver, REG_COLOR1, color1)
    writeReg(driver, REG_ZACOLOR, zaColor)
    writeReg(driver, REG_CHROMAKEY, chromaKey)

    // Clip registers
    writeReg(driver, REG_CLIP_LR, (clipRight.toLong << 16) | clipLeft.toLong)
    writeReg(driver, REG_CLIP_TB, (clipHighY.toLong << 16) | clipLowY.toLong)

    // Vertex coordinates - these go through the FIFO
    writeReg(driver, REG_VERTEX_AX, vertexAx & 0xffff)
    writeReg(driver, REG_VERTEX_AY, vertexAy & 0xffff)
    writeReg(driver, REG_VERTEX_BX, vertexBx & 0xffff)
    writeReg(driver, REG_VERTEX_BY, vertexBy & 0xffff)
    writeReg(driver, REG_VERTEX_CX, vertexCx & 0xffff)
    writeReg(driver, REG_VERTEX_CY, vertexCy & 0xffff)

    // Wait for FIFO to drain before writing direct (non-FIFO) registers
    // The gradient registers (0x020-0x07c) bypass the FIFO and write directly.
    // If a FIFO drain happens simultaneously with a direct write, the direct write
    // is lost because effectiveWriteAddress selects the FIFO's address.
    cd.waitSampling(50)

    // Start values (direct writes - not in FIFO)
    writeReg(driver, REG_START_R, startR)
    writeReg(driver, REG_START_G, startG)
    writeReg(driver, REG_START_B, startB)
    writeReg(driver, REG_START_Z, startZ)
    writeReg(driver, REG_START_A, startA)
    writeReg(driver, REG_START_S, startS)
    writeReg(driver, REG_START_T, startT)
    writeReg(driver, REG_START_W, startW)

    // X gradients (direct writes - not in FIFO)
    writeReg(driver, REG_DRDX, dRdX)
    writeReg(driver, REG_DGDX, dGdX)
    writeReg(driver, REG_DBDX, dBdX)
    writeReg(driver, REG_DZDX, dZdX)
    writeReg(driver, REG_DADX, dAdX)
    writeReg(driver, REG_DSDX, dSdX)
    writeReg(driver, REG_DTDX, dTdX)
    writeReg(driver, REG_DWDX, dWdX)

    // Y gradients (direct writes - not in FIFO)
    writeReg(driver, REG_DRDY, dRdY)
    writeReg(driver, REG_DGDY, dGdY)
    writeReg(driver, REG_DBDY, dBdY)
    writeReg(driver, REG_DZDY, dZdY)
    writeReg(driver, REG_DADY, dAdY)
    writeReg(driver, REG_DSDY, dSdY)
    writeReg(driver, REG_DTDY, dTdY)
    writeReg(driver, REG_DWDY, dWdY)

    // Trigger: triangleCMD with sign bit (goes through FIFO)
    val cmdData = if (sign) 1L << 31 else 0L
    writeReg(driver, REG_TRIANGLE_CMD, cmdData)
  }

  // ========================================================================
  // Helper: collect framebuffer writes from BmbMemoryAgent
  // ========================================================================

  /** Read pixel data written to framebuffer memory. Returns Map[(x, y) -> (rgb565, depth16)]
    *
    * Pixel layout in memory (from Write.scala): address = fbBaseAddr + (y * fbPixelStride + x) * 4
    * bytes 0-1: RGB565 (little-endian) bytes 2-3: depth16 (little-endian)
    */
  def collectPixels(
      memory: BmbMemoryAgent,
      writtenAddrs: scala.collection.mutable.Set[Long],
      fbBaseAddr: Long
  ): Map[(Int, Int), (Int, Int)] = {
    val pixels = scala.collection.mutable.Map.empty[(Int, Int), (Int, Int)]
    val stride = voodooConfig.fbPixelStride

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
  // Helper: compare reference pixels with simulation output (strict)
  // ========================================================================
  def comparePixels(
      ref: Seq[RefPixel],
      sim: Map[(Int, Int), (Int, Int)],
      testName: String
  ): Unit = {
    val mismatches = scala.collection.mutable.ArrayBuffer.empty[String]
    val refMap = ref.map(p => (p.x, p.y) -> (p.rgb565, p.depth16)).toMap

    // Check all ref pixels exist in sim with matching values
    for (p <- ref) {
      sim.get((p.x, p.y)) match {
        case Some((simRgb, simDepth)) =>
          if (simRgb != p.rgb565)
            mismatches += f"($testName) (${p.x},${p.y}) color: ref=0x${p.rgb565}%04X sim=0x$simRgb%04X"
          if (simDepth != p.depth16)
            mismatches += f"($testName) (${p.x},${p.y}) depth: ref=0x${p.depth16}%04X sim=0x$simDepth%04X"
        case None =>
          mismatches += f"($testName) (${p.x},${p.y}) missing in sim (ref=0x${p.rgb565}%04X)"
      }
    }

    // Check for extra pixels in sim not in ref
    for (((x, y), (rgb, depth)) <- sim) {
      if (!refMap.contains((x, y)))
        mismatches += f"($testName) ($x,$y) extra in sim: rgb=0x$rgb%04X depth=0x$depth%04X"
    }

    if (mismatches.nonEmpty) {
      println(s"\n=== $testName: ${mismatches.size} mismatches ===")
      mismatches.foreach(println)
      println(s"Reference pixels: ${ref.size}, Simulation pixels: ${sim.size}")
    }

    assert(mismatches.isEmpty, s"$testName: ${mismatches.size} pixel mismatches (see above)")
  }

  // ========================================================================
  // Helper: fuzzy compare - tolerates edge pixel differences between
  // 86Box DDA and SpinalVoodoo edge-function rasterization
  // ========================================================================
  def comparePixelsFuzzy(
      ref: Seq[RefPixel],
      sim: Map[(Int, Int), (Int, Int)],
      testName: String,
      spatialTolerance: Int = 1,
      checkColor: Boolean = true,
      colorTolerance: Int = 0
  ): Unit = {
    val refSet = ref.map(p => (p.x, p.y)).toSet
    val simSet = sim.keySet

    // Find sim pixels with no nearby ref pixel
    val unmatchedSim = simSet.filter { case (x, y) =>
      !(-spatialTolerance to spatialTolerance).exists(dx =>
        (-spatialTolerance to spatialTolerance).exists(dy =>
          refSet.contains((x + dx, y + dy))))
    }

    // Find ref pixels with no nearby sim pixel
    val unmatchedRef = refSet.filter { case (x, y) =>
      !(-spatialTolerance to spatialTolerance).exists(dx =>
        (-spatialTolerance to spatialTolerance).exists(dy =>
          simSet.contains((x + dx, y + dy))))
    }

    // For exact-match pixels, check color values
    val colorMismatches = scala.collection.mutable.ArrayBuffer.empty[String]
    if (checkColor) {
      val refMap = ref.map(p => (p.x, p.y) -> (p.rgb565, p.depth16)).toMap
      for ((xy, (simRgb, simDepth)) <- sim if refMap.contains(xy)) {
        val (refRgb, refDepth) = refMap(xy)
        if (colorTolerance == 0) {
          if (simRgb != refRgb)
            colorMismatches += f"($testName) (${xy._1},${xy._2}) color: ref=0x$refRgb%04X sim=0x$simRgb%04X"
        }
        // With tolerance > 0, could compare per-channel (future)
      }
    }

    // Report
    val edgeDiffs = unmatchedSim.size + unmatchedRef.size
    if (edgeDiffs > 0) {
      println(f"[$testName] Edge differences: ${unmatchedSim.size} sim-only, ${unmatchedRef.size} ref-only (spatial tolerance=$spatialTolerance)")
      if (unmatchedSim.size <= 10)
        unmatchedSim.toSeq.sorted.foreach { case (x, y) => println(f"  sim-only: ($x,$y)") }
      if (unmatchedRef.size <= 10)
        unmatchedRef.toSeq.sorted.foreach { case (x, y) => println(f"  ref-only: ($x,$y)") }
    }
    if (colorMismatches.nonEmpty) {
      println(f"[$testName] ${colorMismatches.size} color mismatches on shared pixels:")
      colorMismatches.take(20).foreach(println)
    }

    println(f"[$testName] Summary: ref=${ref.size} sim=${sim.size} edgeDiffs=$edgeDiffs colorMismatches=${colorMismatches.size}")

    assert(unmatchedSim.isEmpty && unmatchedRef.isEmpty,
      s"$testName: $edgeDiffs unmatched edge pixels (${unmatchedSim.size} sim-only, ${unmatchedRef.size} ref-only)")
    if (checkColor)
      assert(colorMismatches.isEmpty, s"$testName: ${colorMismatches.size} color mismatches (see above)")
  }

  // ========================================================================
  // DUT setup helper (from VoodooTracePlayer.setupDut pattern)
  // ========================================================================
  def setupDut(
      dut: Core
  ): (BmbDriver, BmbMemoryAgent, BmbMemoryAgent, scala.collection.mutable.Set[Long]) = {
    dut.clockDomain.forkStimulus(10)

    // Init status inputs
    dut.io.statusInputs.vRetrace #= false
    dut.io.statusInputs.fbiBusy #= false
    dut.io.statusInputs.trexBusy #= false
    dut.io.statusInputs.sstBusy #= false
    dut.io.statusInputs.displayedBuffer #= 0
    dut.io.statusInputs.memFifoFree #= 0xffff
    dut.io.statusInputs.swapsPending #= 0
    dut.io.statusInputs.pciInterrupt #= false

    // Init statistics
    dut.io.statisticsIn.pixelsIn #= 0
    dut.io.statisticsIn.chromaFail #= 0
    dut.io.statisticsIn.zFuncFail #= 0
    dut.io.statisticsIn.aFuncFail #= 0
    dut.io.statisticsIn.pixelsOut #= 0

    // Set framebuffer base address
    dut.io.fbBaseAddr #= 0

    // Create BmbDriver for register writes
    val bmbDriver = new BmbDriver(dut.io.regBus, dut.clockDomain)

    // Create BmbMemoryAgent for framebuffer writes
    val fbMemSize = 4 * 1024 * 1024L
    val fbMemory = new BmbMemoryAgent(fbMemSize)
    fbMemory.addPort(
      bus = dut.io.fbWrite,
      busAddress = 0, // fbBaseAddr=0, so no offset needed
      clockDomain = dut.clockDomain,
      withDriver = true
    )

    // Track written addresses for efficient pixel collection
    val writtenAddrs = scala.collection.mutable.Set.empty[Long]
    StreamMonitor(dut.io.fbWrite.cmd, dut.clockDomain) { payload =>
      if (payload.opcode.toInt == Bmb.Cmd.Opcode.WRITE) {
        writtenAddrs += payload.address.toLong
      }
    }

    // Create BmbMemoryAgent for texture reads
    val texMemSize = 4 * 1024 * 1024L
    val texMemory = new BmbMemoryAgent(texMemSize)
    texMemory.addPort(
      bus = dut.io.texRead,
      busAddress = 0,
      clockDomain = dut.clockDomain,
      withDriver = true
    )

    dut.clockDomain.waitSampling(5)

    (bmbDriver, fbMemory, texMemory, writtenAddrs)
  }

  // ========================================================================
  // Test 1: Flat-shaded triangle
  // ========================================================================
  test("Flat-shaded triangle: exact pixel match") {
    compiled.doSim("flat_shaded") { dut =>
      val (driver, fbMemory, _, writtenAddrs) = setupDut(dut)

      // Vertices: A(160,80), B(240,200), C(80,200) in 12.4 format (*16)
      val vAx = 160 * 16
      val vAy = 80 * 16
      val vBx = 240 * 16
      val vBy = 200 * 16
      val vCx = 80 * 16
      val vCy = 200 * 16

      // Constant red: startR = 200<<12 (12.12), all gradients = 0
      val startR = 200 << 12
      val startG = 0
      val startB = 0
      val startA = 255 << 12

      // fbzColorPath: zeroOther=1 (bit 8), add=CLOCAL (bit 14)
      // This means: src = 0 * factor + clocal = CLAMP(ir >> 12) = 200
      val fbzColorPath = (1 << 8) | (1 << 14) // zero_other + add_clocal

      // fbzMode: enable clipping (bit 0), RGB write mask (bit 9)
      val fbzMode = 1 | (1 << 9)

      // sign=false means left-to-right (CCW triangle)
      // For A(160,80) -> B(240,200) -> C(80,200): B is right of AC line
      // So xdir=1 (left-to-right), sign=false
      val sign = false

      // Submit triangle to simulation
      submitTriangle(
        driver,
        dut.clockDomain,
        vertexAx = vAx,
        vertexAy = vAy,
        vertexBx = vBx,
        vertexBy = vBy,
        vertexCx = vCx,
        vertexCy = vCy,
        startR = startR,
        startG = startG,
        startB = startB,
        startA = startA,
        startZ = 0,
        startS = 0,
        startT = 0,
        startW = 0,
        dRdX = 0,
        dGdX = 0,
        dBdX = 0,
        dAdX = 0,
        dZdX = 0,
        dSdX = 0,
        dTdX = 0,
        dWdX = 0,
        dRdY = 0,
        dGdY = 0,
        dBdY = 0,
        dAdY = 0,
        dZdY = 0,
        dSdY = 0,
        dTdY = 0,
        dWdY = 0,
        fbzColorPath = fbzColorPath,
        fbzMode = fbzMode,
        sign = sign,
        clipRight = 640,
        clipHighY = 480
      )

      // Wait for pipeline to drain (9600 pixels at ~12 clocks/pixel ≈ 120k cycles)
      dut.clockDomain.waitSampling(200000)

      // Collect simulation results
      val simPixels = collectPixels(fbMemory, writtenAddrs, 0)
      println(s"[flat_shaded] Simulation produced ${simPixels.size} pixels")

      // Compute reference
      val refParams = VoodooReference.fromRegisterValues(
        vertexAx = vAx,
        vertexAy = vAy,
        vertexBx = vBx,
        vertexBy = vBy,
        vertexCx = vCx,
        vertexCy = vCy,
        startR = startR,
        startG = startG,
        startB = startB,
        startA = startA,
        startZ = 0,
        startS = 0,
        startT = 0,
        startW = 0,
        dRdX = 0,
        dGdX = 0,
        dBdX = 0,
        dAdX = 0,
        dZdX = 0,
        dSdX = 0,
        dTdX = 0,
        dWdX = 0,
        dRdY = 0,
        dGdY = 0,
        dBdY = 0,
        dAdY = 0,
        dZdY = 0,
        dSdY = 0,
        dTdY = 0,
        dWdY = 0,
        fbzColorPath = fbzColorPath,
        fbzMode = fbzMode,
        sign = sign,
        clipRight = 640,
        clipHighY = 480
      )
      val refPixels = VoodooReference.voodooTriangle(refParams)
      println(s"[flat_shaded] Reference produced ${refPixels.size} pixels")

      // Verify reference sanity: all pixels should have the same color
      if (refPixels.nonEmpty) {
        val expectedRgb = VoodooReference.writePixel(200, 0, 0)
        for (p <- refPixels.take(5)) {
          println(
            f"  ref pixel (${p.x},${p.y}) rgb565=0x${p.rgb565}%04X expected=0x$expectedRgb%04X"
          )
        }
      }

      // Compare with fuzzy spatial tolerance for edge pixel differences
      comparePixelsFuzzy(refPixels, simPixels, "flat_shaded")
    }
  }

  // ========================================================================
  // Test 2: Gouraud-shaded triangle
  // ========================================================================
  test("Gouraud-shaded triangle: exact pixel match") {
    compiled.doSim("gouraud_shaded") { dut =>
      val (driver, fbMemory, _, writtenAddrs) = setupDut(dut)

      // Same vertices as flat test
      val vAx = 160 * 16
      val vAy = 80 * 16
      val vBx = 240 * 16
      val vBy = 200 * 16
      val vCx = 80 * 16
      val vCy = 200 * 16

      // Start red=100, with dRdX = 1<<12 (increase by 1.0 per pixel)
      val startR = 100 << 12
      val startG = 50 << 12
      val startB = 0
      val startA = 255 << 12
      val dRdX = 1 << 12 // +1 red per pixel in X
      val dGdX = 0
      val dBdX = 1 << 12 // +1 blue per pixel in X

      // fbzColorPath: zero_other + add_clocal (= iterated color passthrough)
      val fbzColorPath = (1 << 8) | (1 << 14)
      val fbzMode = 1 | (1 << 9)
      val sign = false

      submitTriangle(
        driver,
        dut.clockDomain,
        vertexAx = vAx,
        vertexAy = vAy,
        vertexBx = vBx,
        vertexBy = vBy,
        vertexCx = vCx,
        vertexCy = vCy,
        startR = startR,
        startG = startG,
        startB = startB,
        startA = startA,
        startZ = 0,
        startS = 0,
        startT = 0,
        startW = 0,
        dRdX = dRdX,
        dGdX = dGdX,
        dBdX = dBdX,
        dAdX = 0,
        dZdX = 0,
        dSdX = 0,
        dTdX = 0,
        dWdX = 0,
        dRdY = 0,
        dGdY = 0,
        dBdY = 0,
        dAdY = 0,
        dZdY = 0,
        dSdY = 0,
        dTdY = 0,
        dWdY = 0,
        fbzColorPath = fbzColorPath,
        fbzMode = fbzMode,
        sign = sign,
        clipRight = 640,
        clipHighY = 480
      )

      dut.clockDomain.waitSampling(200000)

      val simPixels = collectPixels(fbMemory, writtenAddrs, 0)
      println(s"[gouraud] Simulation produced ${simPixels.size} pixels")

      val refParams = VoodooReference.fromRegisterValues(
        vertexAx = vAx,
        vertexAy = vAy,
        vertexBx = vBx,
        vertexBy = vBy,
        vertexCx = vCx,
        vertexCy = vCy,
        startR = startR,
        startG = startG,
        startB = startB,
        startA = startA,
        startZ = 0,
        startS = 0,
        startT = 0,
        startW = 0,
        dRdX = dRdX,
        dGdX = dGdX,
        dBdX = dBdX,
        dAdX = 0,
        dZdX = 0,
        dSdX = 0,
        dTdX = 0,
        dWdX = 0,
        dRdY = 0,
        dGdY = 0,
        dBdY = 0,
        dAdY = 0,
        dZdY = 0,
        dSdY = 0,
        dTdY = 0,
        dWdY = 0,
        fbzColorPath = fbzColorPath,
        fbzMode = fbzMode,
        sign = sign,
        clipRight = 640,
        clipHighY = 480
      )
      val refPixels = VoodooReference.voodooTriangle(refParams)
      println(s"[gouraud] Reference produced ${refPixels.size} pixels")

      if (refPixels.nonEmpty) {
        for (p <- refPixels.take(5)) {
          println(f"  ref pixel (${p.x},${p.y}) rgb565=0x${p.rgb565}%04X")
        }
      }

      // Compare with fuzzy tolerance for edge pixel differences
      comparePixelsFuzzy(refPixels, simPixels, "gouraud_shaded")
    }
  }

  // ========================================================================
  // Test 3: Textured triangle (single TMU, point sampling, non-perspective)
  // ========================================================================
  test("Textured triangle: exact pixel match") {
    compiled.doSim("textured") { dut =>
      val (driver, fbMemory, texMemory, writtenAddrs) = setupDut(dut)

      // Load a 4x4 ARGB4444 texture into texture memory
      // Each texel is 2 bytes. ARGB4444 format = 0xc in textureMode bits[11:8]
      // Pattern: red gradient across S, green gradient across T
      val texWidth = 4
      val texHeight = 4
      val texBaseAddr = 0L // texBaseAddr register in units of 8 bytes

      for (t <- 0 until texHeight; s <- 0 until texWidth) {
        val r = (s * 5) & 0xf // 0,5,10,15
        val g = (t * 5) & 0xf
        val b = 0
        val a = 0xf // fully opaque
        val argb4444 = (a << 12) | (r << 8) | (g << 4) | b
        val addr = (t * texWidth + s) * 2
        texMemory.setByte(addr, (argb4444 & 0xff).toByte)
        texMemory.setByte(addr + 1, ((argb4444 >> 8) & 0xff).toByte)
      }

      // Pre-decode the same texture for the reference model
      val refTexData = new Array[Int](texWidth * texHeight)
      for (t <- 0 until texHeight; s <- 0 until texWidth) {
        val r = (s * 5) & 0xf
        val g = (t * 5) & 0xf
        val b = 0
        val a = 0xf
        val argb4444 = (a << 12) | (r << 8) | (g << 4) | b
        refTexData(s + t * texWidth) = VoodooReference.decodeTexel(argb4444, 0xc)
      }

      // Small triangle for textured test
      val vAx = 100 * 16
      val vAy = 100 * 16
      val vBx = 116 * 16
      val vBy = 116 * 16
      val vCx = 84 * 16
      val vCy = 116 * 16

      // Texture coords: non-perspective (bit 0 of textureMode = 0)
      // For non-perspective: tex_s = tmu0_s >> 28 (in 86Box)
      // 86Box shifts S/T by <<14 from register values, so effective shift is >>28 = >>(14+14)
      // To get tex_s=0..3 across 16 pixels, need tmu0_s to go from 0 to 3<<28
      // dSdX in register = (3<<28) / (16 * (1<<14)) = but we need to think in register units
      // Register S is 14.18 fixed. After 86Box shift <<14, it becomes int64.
      // Non-perspective: tex_s = tmu0_s >> 28 = (regS << 14) >> 28 = regS >> 14
      // So to get tex_s ranging 0..3 across ~16 pixels:
      // startS_reg = 0, dSdX_reg such that after 16 pixels: regS + 16*dSdX >> 14 = 3
      // => dSdX_reg = (3 << 14) / 16 = 3072

      val startS_reg = 0 // Will be shifted <<14 by fromRegisterValues
      val startT_reg = 0
      val dSdX_reg = 3072 // (3 << 14) / 16 ≈ increase by 3/16 per pixel
      val dTdX_reg = 0
      val dSdY_reg = 0
      val dTdY_reg = 3072

      // textureMode: format=ARGB4444 (12 << 8), clampS (bit 6), clampT (bit 7), no perspective (bit 0 = 0)
      val textureMode = (12 << 8) | (1 << 6) | (1 << 7)

      // tLOD: lodmin=0, lodmax=0 (single LOD level)
      val tLOD = 0

      // fbzColorPath: rgbSel=TEXTURE(1), textureEnable(bit 27), zero_other(bit 8), add=CLOCAL(bit 14=1)
      // Actually for texture passthrough: rgbSel=1 (texture), zero_other=0, sub_clocal=0, mselect=0, add=0
      // Simplest: zero_other=1, add=0, rgbSel doesn't matter → output = 0 * factor + 0 = 0 (wrong)
      // Better: zero_other=0, rgbSel=1 (tex), sub_clocal=0, mselect=0 (zero), reverse_blend=1 → factor=1
      //   src = tex * 1 = tex, add=0 → output = tex
      // cc_reverse_blend=1 means msel is NOT inverted (keeps as-is), then +1
      // mselect=0 → msel=0, if reverse_blend: stays 0, +1 = 1 → src = tex * 1 / 256 ≈ 0
      // Hmm, the multiply is >>8, so factor=1 means src*1>>8 ≈ 0.
      // The correct way: zero_other=1 (src=0), sub_clocal=0, mselect=0, factor=anything, src=0*factor>>8=0
      //   then add=CLOCAL (cc_add=1), with cc_localselect_override=1 and tex_a bit 7 set → clocal=color0? No.
      // Actually simplest passthrough: cc_localselect=1 (tex output goes to clocal via localselect_override=0, localselect=1)
      //   Wait - localselect=1 means clocal=color0 R/G/B. localselect=0 means clocal=iterated.
      // Let me use: rgbSel=1 (TEX), zero_other=0 → cother=tex
      //   sub_clocal=0, so src=cother=tex
      //   mselect=0 (ZERO), reverse_blend=0 → msel=0, inverted to 0xff, +1 = 0x100=256
      //   src = tex * 256 >> 8 = tex. Perfect!
      val fbzColorPath = 1 | // rgbSel=1 (TREX/texture)
        (0 << 2) | // a_sel=0 (iterated A)
        (0 << 8) | // zero_other=0
        (0 << 9) | // sub_clocal=0
        (0 << 10) | // mselect=0 (ZERO)
        (0 << 13) | // reverse_blend=0 (factor inverted: 0 → 0xff → +1 = 256)
        (0 << 14) | // add=0
        (1 << 27) // texture_enable

      val fbzMode = 1 | (1 << 9) // clipping + RGB write

      val sign = false

      // Write texture config first
      writeReg(driver, REG_TEXBASEADDR, texBaseAddr)

      submitTriangle(
        driver,
        dut.clockDomain,
        vertexAx = vAx,
        vertexAy = vAy,
        vertexBx = vBx,
        vertexBy = vBy,
        vertexCx = vCx,
        vertexCy = vCy,
        startR = 0,
        startG = 0,
        startB = 0,
        startA = 255 << 12,
        startZ = 0,
        startS = startS_reg,
        startT = startT_reg,
        startW = 0,
        dRdX = 0,
        dGdX = 0,
        dBdX = 0,
        dAdX = 0,
        dZdX = 0,
        dSdX = dSdX_reg,
        dTdX = dTdX_reg,
        dWdX = 0,
        dRdY = 0,
        dGdY = 0,
        dBdY = 0,
        dAdY = 0,
        dZdY = 0,
        dSdY = dSdY_reg,
        dTdY = dTdY_reg,
        dWdY = 0,
        fbzColorPath = fbzColorPath,
        fbzMode = fbzMode,
        sign = sign,
        textureMode = textureMode,
        tLOD = tLOD,
        clipRight = 640,
        clipHighY = 480
      )

      dut.clockDomain.waitSampling(20000)

      val simPixels = collectPixels(fbMemory, writtenAddrs, 0)
      println(s"[textured] Simulation produced ${simPixels.size} pixels")

      // Build reference params with pre-decoded texture
      val texDataPerLod = Array.fill(9)(Array.empty[Int])
      texDataPerLod(0) = refTexData
      val texWMaskPerLod = Array.fill(9)(0)
      texWMaskPerLod(0) = texWidth - 1
      val texHMaskPerLod = Array.fill(9)(0)
      texHMaskPerLod(0) = texHeight - 1
      val texShiftPerLod = Array.fill(9)(0)
      texShiftPerLod(0) = 8 - 0 // tex_shift = 8 - tex_lod, lod=0
      val texLodPerLod = Array.fill(9)(0) // tex_lod[0] = 0 for 256x256 base

      val refParams = VoodooReference.fromRegisterValues(
        vertexAx = vAx,
        vertexAy = vAy,
        vertexBx = vBx,
        vertexBy = vBy,
        vertexCx = vCx,
        vertexCy = vCy,
        startR = 0,
        startG = 0,
        startB = 0,
        startA = 255 << 12,
        startZ = 0,
        startS = startS_reg,
        startT = startT_reg,
        startW = 0,
        dRdX = 0,
        dGdX = 0,
        dBdX = 0,
        dAdX = 0,
        dZdX = 0,
        dSdX = dSdX_reg,
        dTdX = dTdX_reg,
        dWdX = 0,
        dRdY = 0,
        dGdY = 0,
        dBdY = 0,
        dAdY = 0,
        dZdY = 0,
        dSdY = dSdY_reg,
        dTdY = dTdY_reg,
        dWdY = 0,
        fbzColorPath = fbzColorPath,
        fbzMode = fbzMode,
        sign = sign,
        textureMode = textureMode,
        tLOD = tLOD,
        clipRight = 640,
        clipHighY = 480,
        texData = texDataPerLod,
        texWMask = texWMaskPerLod,
        texHMask = texHMaskPerLod,
        texShift = texShiftPerLod,
        texLod = texLodPerLod
      )

      val refPixels = VoodooReference.voodooTriangle(refParams)
      println(s"[textured] Reference produced ${refPixels.size} pixels")

      if (refPixels.nonEmpty) {
        for (p <- refPixels.take(5)) {
          println(f"  ref pixel (${p.x},${p.y}) rgb565=0x${p.rgb565}%04X")
        }
      }

      // Compare with fuzzy tolerance for edge pixel differences
      comparePixelsFuzzy(refPixels, simPixels, "textured")
    }
  }

  // ========================================================================
  // Test 4: Texture format coverage (all 9 formats)
  // ========================================================================
  test("Texture format coverage: all formats decode correctly") {
    compiled.doSim("tex_format_coverage") { dut =>
      val (driver, fbMemory, texMemory, writtenAddrs) = setupDut(dut)

      // Same triangle geometry as existing textured test
      val vAx = 100 * 16
      val vAy = 100 * 16
      val vBx = 116 * 16
      val vBy = 116 * 16
      val vCx = 84 * 16
      val vCy = 116 * 16

      // Constant S=0, T=0 with clamp — every pixel reads texel (0,0)
      val startS_reg = 0
      val startT_reg = 0

      // Target color to encode into each texture format
      val targetR = 170
      val targetG = 85
      val targetB = 200
      val targetA = 255

      // fbzColorPath: texture passthrough (same as existing textured test)
      val fbzColorPath = 1 |       // rgbSel=1 (TREX/texture)
        (0 << 2) |                  // a_sel=0 (iterated A)
        (0 << 8) |                  // zero_other=0
        (0 << 9) |                  // sub_clocal=0
        (0 << 10) |                 // mselect=0 (ZERO)
        (0 << 13) |                 // reverse_blend=0 (factor inverted: 0→0xff→+1=256)
        (0 << 14) |                 // add=0
        (1 << 27)                   // texture_enable

      val fbzMode = 1 | (1 << 9)   // clipping + RGB write
      val sign = false

      for (fmt <- allFormats) {
        // Clear tracking
        writtenAddrs.clear()

        // Encode the target color into this format
        val rawTexel = fmt.encode(targetR, targetG, targetB, targetA)

        // Write a single texel at address 0 (texel 0,0)
        // HW reads addr = (t * 256 + s) * bpp; with S=0,T=0 clamp → always reads addr 0
        if (fmt.bpp == 1) {
          texMemory.setByte(0, (rawTexel & 0xff).toByte)
        } else {
          texMemory.setByte(0, (rawTexel & 0xff).toByte)
          texMemory.setByte(1, ((rawTexel >> 8) & 0xff).toByte)
        }

        // textureMode: format | clampS | clampT
        val textureMode = (fmt.hwFormat << 8) | (1 << 6) | (1 << 7)

        writeReg(driver, REG_TEXBASEADDR, 0)

        submitTriangle(
          driver, dut.clockDomain,
          vertexAx = vAx, vertexAy = vAy,
          vertexBx = vBx, vertexBy = vBy,
          vertexCx = vCx, vertexCy = vCy,
          startR = 0, startG = 0, startB = 0, startA = 255 << 12,
          startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
          dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
          dSdX = 0, dTdX = 0, dWdX = 0,
          dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
          dSdY = 0, dTdY = 0, dWdY = 0,
          fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = sign,
          textureMode = textureMode, tLOD = 0,
          clipRight = 640, clipHighY = 480
        )

        dut.clockDomain.waitSampling(20000)

        val simPixels = collectPixels(fbMemory, writtenAddrs, 0)

        // Build reference: single texel at (0,0), all coords clamp to it
        val decoded = VoodooReference.decodeTexel(rawTexel, fmt.refFormat)
        val refTexData = Array(decoded)  // 1-element array for texel (0,0)

        val texDataPerLod = Array.fill(9)(Array.empty[Int])
        texDataPerLod(0) = refTexData
        val texWMaskPerLod = Array.fill(9)(0)
        texWMaskPerLod(0) = 255  // LOD 0: 256x256
        val texHMaskPerLod = Array.fill(9)(0)
        texHMaskPerLod(0) = 255
        val texShiftPerLod = Array.fill(9)(0)
        texShiftPerLod(0) = 8  // tex_shift = 8 - lod, lod=0
        val texLodPerLod = Array.fill(9)(0)

        val refParams = VoodooReference.fromRegisterValues(
          vertexAx = vAx, vertexAy = vAy,
          vertexBx = vBx, vertexBy = vBy,
          vertexCx = vCx, vertexCy = vCy,
          startR = 0, startG = 0, startB = 0, startA = 255 << 12,
          startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
          dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
          dSdX = 0, dTdX = 0, dWdX = 0,
          dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
          dSdY = 0, dTdY = 0, dWdY = 0,
          fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = sign,
          textureMode = textureMode, tLOD = 0,
          clipRight = 640, clipHighY = 480,
          texData = texDataPerLod,
          texWMask = texWMaskPerLod,
          texHMask = texHMaskPerLod,
          texShift = texShiftPerLod,
          texLod = texLodPerLod
        )

        val refPixels = VoodooReference.voodooTriangle(refParams)
        println(f"[format_coverage/${fmt.name}] sim=${simPixels.size} ref=${refPixels.size} rawTexel=0x${rawTexel}%04X")

        comparePixelsFuzzy(refPixels, simPixels, s"format_coverage/${fmt.name}")
      }
    }
  }

  // ========================================================================
  // Test 5: Texture wrap mode (wrap vs clamp)
  // ========================================================================
  test("Texture wrap mode: wrap and clamp at LOD 0") {
    compiled.doSim("tex_wrap_mode") { dut =>
      val (driver, fbMemory, texMemory, writtenAddrs) = setupDut(dut)

      // Use RGB565 format at LOD 0 (256x256)
      // Populate texture: columns 0-127 = red, columns 128-255 = blue
      val red565: Int = 0xF800   // pure red in RGB565
      val blue565: Int = 0x001F  // pure blue in RGB565

      // Write texels for rows 0-3, columns 0-10 and 248-255 (the only ones accessed)
      for (row <- 0 until 4) {
        for (col <- 0 until 11) {
          val addr = (row * 256 + col) * 2
          val texel = red565  // columns 0-127 → red
          texMemory.setByte(addr, (texel & 0xff).toByte)
          texMemory.setByte(addr + 1, ((texel >> 8) & 0xff).toByte)
        }
        for (col <- 248 until 256) {
          val addr = (row * 256 + col) * 2
          val texel = blue565  // columns 128-255 → blue
          texMemory.setByte(addr, (texel & 0xff).toByte)
          texMemory.setByte(addr + 1, ((texel >> 8) & 0xff).toByte)
        }
      }

      // Triangle: A(100,100) B(120,104) C(100,104) — 20px wide strip
      val vAx = 100 * 16
      val vAy = 100 * 16
      val vBx = 120 * 16
      val vBy = 104 * 16
      val vCx = 100 * 16
      val vCy = 104 * 16

      // S starts at texel 250, +1 texel/pixel
      val startS_reg = 250 << 18
      val dSdX_reg = 1 << 18
      val startT_reg = 0
      val dTdX_reg = 0
      val dSdY_reg = 0
      val dTdY_reg = 0

      // fbzColorPath: texture passthrough
      val fbzColorPath = 1 | (1 << 27)  // rgbSel=1 (texture), texture_enable
      val fbzMode = 1 | (1 << 9)        // clipping + RGB write

      // Build reference texture data: pre-decoded 256x256 (only need accessed region)
      val refTexData256 = new Array[Int](256 * 256)
      for (row <- 0 until 256; col <- 0 until 256) {
        val raw = if (col < 128) red565 else blue565
        refTexData256(col + row * 256) = VoodooReference.decodeTexel(raw, 0xa) // 0xa = RGB565 ref code
      }

      val texDataPerLod = Array.fill(9)(Array.empty[Int])
      texDataPerLod(0) = refTexData256
      val texWMaskPerLod = Array.fill(9)(0)
      texWMaskPerLod(0) = 255
      val texHMaskPerLod = Array.fill(9)(0)
      texHMaskPerLod(0) = 255
      val texShiftPerLod = Array.fill(9)(0)
      texShiftPerLod(0) = 8  // tex_shift = 8 - lod, lod=0
      val texLodPerLod = Array.fill(9)(0)

      // --- Sub-test A: wrap mode (clampS=0, clampT=1) ---
      writtenAddrs.clear()

      val textureModeWrap = (10 << 8) | (1 << 7)  // RGB565, clampT only

      writeReg(driver, REG_TEXBASEADDR, 0)

      submitTriangle(
        driver, dut.clockDomain,
        vertexAx = vAx, vertexAy = vAy,
        vertexBx = vBx, vertexBy = vBy,
        vertexCx = vCx, vertexCy = vCy,
        startR = 0, startG = 0, startB = 0, startA = 255 << 12,
        startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
        dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
        dSdX = dSdX_reg, dTdX = dTdX_reg, dWdX = 0,
        dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
        dSdY = dSdY_reg, dTdY = dTdY_reg, dWdY = 0,
        fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
        textureMode = textureModeWrap, tLOD = 0,
        clipRight = 640, clipHighY = 480
      )

      dut.clockDomain.waitSampling(20000)

      val simPixelsWrap = collectPixels(fbMemory, writtenAddrs, 0)

      val refParamsWrap = VoodooReference.fromRegisterValues(
        vertexAx = vAx, vertexAy = vAy,
        vertexBx = vBx, vertexBy = vBy,
        vertexCx = vCx, vertexCy = vCy,
        startR = 0, startG = 0, startB = 0, startA = 255 << 12,
        startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
        dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
        dSdX = dSdX_reg, dTdX = dTdX_reg, dWdX = 0,
        dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
        dSdY = dSdY_reg, dTdY = dTdY_reg, dWdY = 0,
        fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
        textureMode = textureModeWrap, tLOD = 0,
        clipRight = 640, clipHighY = 480,
        texData = texDataPerLod,
        texWMask = texWMaskPerLod,
        texHMask = texHMaskPerLod,
        texShift = texShiftPerLod,
        texLod = texLodPerLod
      )

      val refPixelsWrap = VoodooReference.voodooTriangle(refParamsWrap)
      println(s"[wrap_mode/wrap] sim=${simPixelsWrap.size} ref=${refPixelsWrap.size}")

      comparePixelsFuzzy(refPixelsWrap, simPixelsWrap, "wrap_mode/wrap", spatialTolerance = 2)

      // --- Sub-test B: clamp mode (clampS=1, clampT=1) ---
      writtenAddrs.clear()

      val textureModeClamp = (10 << 8) | (1 << 6) | (1 << 7)  // RGB565, clampS+clampT

      writeReg(driver, REG_TEXBASEADDR, 0)

      submitTriangle(
        driver, dut.clockDomain,
        vertexAx = vAx, vertexAy = vAy,
        vertexBx = vBx, vertexBy = vBy,
        vertexCx = vCx, vertexCy = vCy,
        startR = 0, startG = 0, startB = 0, startA = 255 << 12,
        startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
        dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
        dSdX = dSdX_reg, dTdX = dTdX_reg, dWdX = 0,
        dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
        dSdY = dSdY_reg, dTdY = dTdY_reg, dWdY = 0,
        fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
        textureMode = textureModeClamp, tLOD = 0,
        clipRight = 640, clipHighY = 480
      )

      dut.clockDomain.waitSampling(20000)

      val simPixelsClamp = collectPixels(fbMemory, writtenAddrs, 0)

      val refParamsClamp = VoodooReference.fromRegisterValues(
        vertexAx = vAx, vertexAy = vAy,
        vertexBx = vBx, vertexBy = vBy,
        vertexCx = vCx, vertexCy = vCy,
        startR = 0, startG = 0, startB = 0, startA = 255 << 12,
        startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
        dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
        dSdX = dSdX_reg, dTdX = dTdX_reg, dWdX = 0,
        dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
        dSdY = dSdY_reg, dTdY = dTdY_reg, dWdY = 0,
        fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
        textureMode = textureModeClamp, tLOD = 0,
        clipRight = 640, clipHighY = 480,
        texData = texDataPerLod,
        texWMask = texWMaskPerLod,
        texHMask = texHMaskPerLod,
        texShift = texShiftPerLod,
        texLod = texLodPerLod
      )

      val refPixelsClamp = VoodooReference.voodooTriangle(refParamsClamp)
      println(s"[wrap_mode/clamp] sim=${simPixelsClamp.size} ref=${refPixelsClamp.size}")

      comparePixelsFuzzy(refPixelsClamp, simPixelsClamp, "wrap_mode/clamp", spatialTolerance = 2)
    }
  }

  // ========================================================================
  // Test 6: LOD / mipmap level selection
  // ========================================================================
  test("LOD mipmap level selection: forced LOD 0, 1, 2") {
    compiled.doSim("lod_selection") { dut =>
      val (driver, fbMemory, texMemory, writtenAddrs) = setupDut(dut)

      // ARGB4444 texels for 3 LOD levels: red, green, blue
      val redTexel: Int = 0xFF00    // A=F R=F G=0 B=0
      val greenTexel: Int = 0xF0F0  // A=F R=0 G=F B=0
      val blueTexel: Int = 0xF00F   // A=F R=0 G=0 B=F

      // Memory layout (ARGB4444 = 2 bytes/texel):
      // LOD 0 base: offset 0      (256x256 = 131072 bytes)
      // LOD 1 base: offset 131072 (128x128 = 32768 bytes)
      // LOD 2 base: offset 163840 (64x64 = 16384 bytes)
      val lod0Base = 0
      val lod1Base = 131072
      val lod2Base = lod1Base + 32768  // 163840

      // Write texel (0,0) at each LOD base
      texMemory.setByte(lod0Base, (redTexel & 0xff).toByte)
      texMemory.setByte(lod0Base + 1, ((redTexel >> 8) & 0xff).toByte)
      texMemory.setByte(lod1Base, (greenTexel & 0xff).toByte)
      texMemory.setByte(lod1Base + 1, ((greenTexel >> 8) & 0xff).toByte)
      texMemory.setByte(lod2Base, (blueTexel & 0xff).toByte)
      texMemory.setByte(lod2Base + 1, ((blueTexel >> 8) & 0xff).toByte)

      // Triangle: same as existing test, constant S/T=0 with clamp
      val vAx = 100 * 16
      val vAy = 100 * 16
      val vBx = 116 * 16
      val vBy = 116 * 16
      val vCx = 84 * 16
      val vCy = 116 * 16

      // textureMode: ARGB4444 (12), clampS, clampT, non-perspective
      val textureMode = (12 << 8) | (1 << 6) | (1 << 7)

      // fbzColorPath: texture passthrough
      val fbzColorPath = 1 | (1 << 27)
      val fbzMode = 1 | (1 << 9)

      // LOD configs: lodmin=lodmax=N*4 forces LOD level N
      // tLOD register format: bits[5:0]=lodmin, bits[11:6]=lodmax
      val lodConfigs = Seq(
        ("LOD0", 0, redTexel),       // lodmin=0, lodmax=0
        ("LOD1", (4 << 6) | 4, greenTexel),  // lodmin=4, lodmax=4
        ("LOD2", (8 << 6) | 8, blueTexel)    // lodmin=8, lodmax=8
      )

      for ((lodName, tLOD, expectedTexel) <- lodConfigs) {
        writtenAddrs.clear()

        writeReg(driver, REG_TEXBASEADDR, 0)

        submitTriangle(
          driver, dut.clockDomain,
          vertexAx = vAx, vertexAy = vAy,
          vertexBx = vBx, vertexBy = vBy,
          vertexCx = vCx, vertexCy = vCy,
          startR = 0, startG = 0, startB = 0, startA = 255 << 12,
          startZ = 0, startS = 0, startT = 0, startW = 0,
          dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
          dSdX = 0, dTdX = 0, dWdX = 0,
          dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
          dSdY = 0, dTdY = 0, dWdY = 0,
          fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
          textureMode = textureMode, tLOD = tLOD,
          clipRight = 640, clipHighY = 480
        )

        dut.clockDomain.waitSampling(20000)

        val simPixels = collectPixels(fbMemory, writtenAddrs, 0)

        // Build reference: single texel at LOD level N
        val lodLevel = (tLOD & 0x3f) / 4
        val decoded = VoodooReference.decodeTexel(expectedTexel, 0xc) // 0xc = ARGB4444 ref code

        // Reference texture for each LOD level
        val texDataPerLod = Array.fill(9)(Array.empty[Int])
        val texWMaskPerLod = Array.fill(9)(0)
        val texHMaskPerLod = Array.fill(9)(0)
        val texShiftPerLod = Array.fill(9)(0)
        val texLodPerLod = Array.fill(9)(0)

        // Set up all LOD levels that might be accessed
        for (lod <- 0 to 2) {
          val texel = lod match {
            case 0 => VoodooReference.decodeTexel(redTexel, 0xc)
            case 1 => VoodooReference.decodeTexel(greenTexel, 0xc)
            case 2 => VoodooReference.decodeTexel(blueTexel, 0xc)
          }
          val dim = 256 >> lod
          texDataPerLod(lod) = Array.fill(1)(texel)  // Only texel (0,0)
          texWMaskPerLod(lod) = dim - 1
          texHMaskPerLod(lod) = dim - 1
          texShiftPerLod(lod) = 8 - lod
          texLodPerLod(lod) = lod
        }

        val refParams = VoodooReference.fromRegisterValues(
          vertexAx = vAx, vertexAy = vAy,
          vertexBx = vBx, vertexBy = vBy,
          vertexCx = vCx, vertexCy = vCy,
          startR = 0, startG = 0, startB = 0, startA = 255 << 12,
          startZ = 0, startS = 0, startT = 0, startW = 0,
          dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
          dSdX = 0, dTdX = 0, dWdX = 0,
          dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
          dSdY = 0, dTdY = 0, dWdY = 0,
          fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
          textureMode = textureMode, tLOD = tLOD,
          clipRight = 640, clipHighY = 480,
          texData = texDataPerLod,
          texWMask = texWMaskPerLod,
          texHMask = texHMaskPerLod,
          texShift = texShiftPerLod,
          texLod = texLodPerLod
        )

        val refPixels = VoodooReference.voodooTriangle(refParams)
        println(s"[lod_selection/$lodName] sim=${simPixels.size} ref=${refPixels.size}")

        comparePixelsFuzzy(refPixels, simPixels, s"lod_selection/$lodName")
      }
    }
  }

  // ========================================================================
  // Test 7: Color combine modes (data-driven, 12 sub-cases)
  // ========================================================================
  test("Color combine modes: comprehensive coverage") {
    compiled.doSim("cc_modes") { dut =>
      val (driver, fbMemory, texMemory, writtenAddrs) = setupDut(dut)

      // ---- Shared geometry ----
      // Triangle: A(120,60) B(180,140) C(60,140) in 12.4 format
      val vAx = 120 * 16
      val vAy = 60 * 16
      val vBx = 180 * 16
      val vBy = 140 * 16
      val vCx = 60 * 16
      val vCy = 140 * 16

      // ---- Shared iterated colors ----
      // R=100, G=80, B=60, A=200 in 12.12 format
      val startR = 100 << 12
      val startG = 80 << 12
      val startB = 60 << 12
      val startA = 200 << 12
      // Gradients: +0.5 per pixel for R and G (dRdX = dGdX = 0.5 in 12.12)
      val dRdX = 1 << 11 // 0.5 in 12.12
      val dGdX = 1 << 11

      // ---- Shared constant colors ----
      val color0 = 0xC0304050 // A=0xC0, R=0x30, G=0x40, B=0x50
      val color1 = 0xA0102060 // A=0xA0, R=0x10, G=0x20, B=0x60

      // ---- 4x4 ARGB4444 texture ----
      // Texels with varied colors; some with alpha >= 0x80 for localSelectOverride
      val texWidth = 4
      val texHeight = 4
      // Row 0: (R=0xF,G=0x0,B=0x0,A=0xF), (R=0xA,G=0x5,B=0x3,A=0xE), (R=0x5,G=0xA,B=0x7,A=0x6), (R=0x0,G=0xF,B=0xF,A=0x3)
      // Row 1: same pattern shifted
      val texels = Array(
        // Row 0
        0xFF00, 0xEA53, 0x65A7, 0x30FF,
        // Row 1
        0xF880, 0xD964, 0x74B8, 0x21EE,
        // Row 2
        0xE770, 0xCA75, 0x83C9, 0x12DD,
        // Row 3
        0xD660, 0xBB86, 0x92DA, 0x03CC
      )

      for (t <- 0 until texHeight; s <- 0 until texWidth) {
        val texel = texels(t * texWidth + s)
        val addr = (t * texWidth + s) * 2
        texMemory.setByte(addr, (texel & 0xff).toByte)
        texMemory.setByte(addr + 1, ((texel >> 8) & 0xff).toByte)
      }

      // Pre-decode for reference
      val refTexData = new Array[Int](texWidth * texHeight)
      for (i <- texels.indices)
        refTexData(i) = VoodooReference.decodeTexel(texels(i), 0xc) // 0xc = ARGB4444

      val texDataPerLod = Array.fill(9)(Array.empty[Int])
      texDataPerLod(0) = refTexData
      val texWMaskPerLod = Array.fill(9)(0)
      texWMaskPerLod(0) = texWidth - 1
      val texHMaskPerLod = Array.fill(9)(0)
      texHMaskPerLod(0) = texHeight - 1
      val texShiftPerLod = Array.fill(9)(0)
      texShiftPerLod(0) = 8 // tex_shift = 8 - lod, lod=0
      val texLodPerLod = Array.fill(9)(0)

      // Texture coords: S and T increase by ~3/80 per pixel to span 0..3 across ~80 px
      // dSdX = (3 << 14) / 80 = 614 (approximately)
      val startS_reg = 0
      val startT_reg = 0
      val dSdX_reg = 614
      val dTdX_reg = 0
      val dSdY_reg = 0
      val dTdY_reg = 614

      // textureMode: ARGB4444 (12 << 8), clampS, clampT, non-perspective
      val textureMode = (12 << 8) | (1 << 6) | (1 << 7)

      // ---- fbzColorPath bit-field constructors ----
      def mkFbzCP(
          rgbSel: Int = 0, alphaSel: Int = 0,
          localSelect: Int = 0, alphaLocalSel: Int = 0,
          localSelectOverride: Int = 0,
          zeroOther: Int = 0, subClocal: Int = 0,
          mselect: Int = 0, reverseBlend: Int = 0,
          add: Int = 0, invertOutput: Int = 0,
          alphaZeroOther: Int = 0, alphaSubClocal: Int = 0,
          alphaMselect: Int = 0, alphaReverseBlend: Int = 0,
          alphaAdd: Int = 0, alphaInvertOutput: Int = 0,
          textureEnable: Int = 1
      ): Int = {
        (rgbSel & 3) |
        ((alphaSel & 3) << 2) |
        ((localSelect & 1) << 4) |
        ((alphaLocalSel & 3) << 5) |
        ((localSelectOverride & 1) << 7) |
        ((zeroOther & 1) << 8) |
        ((subClocal & 1) << 9) |
        ((mselect & 7) << 10) |
        ((reverseBlend & 1) << 13) |
        ((add & 3) << 14) |
        ((invertOutput & 1) << 16) |
        ((alphaZeroOther & 1) << 17) |
        ((alphaSubClocal & 1) << 18) |
        ((alphaMselect & 7) << 19) |
        ((alphaReverseBlend & 1) << 22) |
        ((alphaAdd & 3) << 23) |
        ((alphaInvertOutput & 1) << 25) |
        ((textureEnable & 1) << 27)
      }

      // ---- Test case table: (name, fbzColorPath, fbzMode) ----
      val baseFbzMode = 1 | (1 << 9) // clipping + RGB write

      val testCases: Seq[(String, Int, Int)] = Seq(
        // 1. mselect_clocal: tex * clocal/256
        //    rgbSel=TEX(1), mselect=CLOCAL(1), reverseBlend=1 (passthrough)
        ("mselect_clocal",
          mkFbzCP(rgbSel = 1, mselect = 1, reverseBlend = 1),
          baseFbzMode),

        // 2. mselect_alocal: tex * alocal/256
        //    rgbSel=TEX(1), mselect=ALOCAL(3), reverseBlend=1
        ("mselect_alocal",
          mkFbzCP(rgbSel = 1, mselect = 3, reverseBlend = 1),
          baseFbzMode),

        // 3. mselect_texalpha: iter * texA/256 + clocal
        //    rgbSel=ITER(0), mselect=TEX_ALPHA(4), reverseBlend=1, add=CLOCAL(1)
        ("mselect_texalpha",
          mkFbzCP(rgbSel = 0, mselect = 4, reverseBlend = 1, add = 1),
          baseFbzMode),

        // 4. mselect_texrgb: iter * texRGB/256
        //    rgbSel=ITER(0), mselect=TEX_RGB(5), reverseBlend=1
        //    Note: here we use zeroOther=0 so src = cother = iter, then multiply
        //    but actually: src = cother - (subClocal? clocal:0) * factor
        //    With zeroOther=0, subClocal=0: src=cother=iter; src*factor>>8
        ("mselect_texrgb",
          mkFbzCP(rgbSel = 0, mselect = 5, reverseBlend = 1),
          baseFbzMode),

        // 5. sub_clocal: tex - clocal, factor=256 (msel=0, reverseBlend=0 → ~0+1=256)
        //    rgbSel=TEX(1), subClocal=1, mselect=ZERO(0), reverseBlend=0
        ("sub_clocal",
          mkFbzCP(rgbSel = 1, subClocal = 1, mselect = 0, reverseBlend = 0),
          baseFbzMode),

        // 6. add_alocal: tex * 256/256 + alocal
        //    rgbSel=TEX(1), mselect=ZERO(0), reverseBlend=0, add=ALOCAL(2)
        ("add_alocal",
          mkFbzCP(rgbSel = 1, mselect = 0, reverseBlend = 0, add = 2),
          baseFbzMode),

        // 7. invert_output: ~(clocal) = ~(iter clamped)
        //    zeroOther=1, add=CLOCAL(1), invertOutput=1
        ("invert_output",
          mkFbzCP(zeroOther = 1, add = 1, invertOutput = 1),
          baseFbzMode),

        // 8. rgb_sel_color1: color1 * 256/256 (passthrough)
        //    rgbSel=COLOR1(2), mselect=ZERO(0), reverseBlend=0
        ("rgb_sel_color1",
          mkFbzCP(rgbSel = 2, mselect = 0, reverseBlend = 0),
          baseFbzMode),

        // 9. local_sel_color0: output = color0 (zeroOther=1, add=CLOCAL(1), localSelect=1)
        ("local_sel_color0",
          mkFbzCP(zeroOther = 1, add = 1, localSelect = 1),
          baseFbzMode),

        // 10. local_sel_override: localSelectOverride=1 → clocal = color0 when texA>=0x80, else iter
        //     zeroOther=1, add=CLOCAL(1), localSelectOverride=1
        ("local_sel_override",
          mkFbzCP(zeroOther = 1, add = 1, localSelectOverride = 1),
          baseFbzMode),

        // 11. alpha_path: exercise alpha combine separately
        //     alphaSel=TEX(1), alphaLocalSel=COLOR0(1), alphaMselect=ALOCAL(3), alphaReverseBlend=1
        //     RGB path: texture passthrough (rgbSel=1, msel=0, revBlend=0 → factor=256)
        ("alpha_path",
          mkFbzCP(rgbSel = 1, mselect = 0, reverseBlend = 0,
                  alphaSel = 1, alphaLocalSel = 1, alphaMselect = 3, alphaReverseBlend = 1),
          baseFbzMode),

        // 12. alpha_planes: fbzMode bit 18 → alpha written instead of depth
        //     Simple texture passthrough, but fbzMode has bit 18 set and depth write enabled
        ("alpha_planes",
          mkFbzCP(rgbSel = 1, mselect = 0, reverseBlend = 0),
          baseFbzMode | (1 << 10) | (1 << 18)) // aux write mask + alpha planes
      )

      for ((name, fbzColorPath, fbzMode) <- testCases) {
        writtenAddrs.clear()
        writeReg(driver, REG_TEXBASEADDR, 0)

        submitTriangle(
          driver, dut.clockDomain,
          vertexAx = vAx, vertexAy = vAy,
          vertexBx = vBx, vertexBy = vBy,
          vertexCx = vCx, vertexCy = vCy,
          startR = startR, startG = startG, startB = startB, startA = startA,
          startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
          dRdX = dRdX, dGdX = dGdX, dBdX = 0, dAdX = 0, dZdX = 0,
          dSdX = dSdX_reg, dTdX = dTdX_reg, dWdX = 0,
          dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
          dSdY = dSdY_reg, dTdY = dTdY_reg, dWdY = 0,
          fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
          textureMode = textureMode, tLOD = 0,
          color0 = color0, color1 = color1,
          clipRight = 640, clipHighY = 480
        )

        dut.clockDomain.waitSampling(200000)

        val simPixels = collectPixels(fbMemory, writtenAddrs, 0)

        val refParams = VoodooReference.fromRegisterValues(
          vertexAx = vAx, vertexAy = vAy,
          vertexBx = vBx, vertexBy = vBy,
          vertexCx = vCx, vertexCy = vCy,
          startR = startR, startG = startG, startB = startB, startA = startA,
          startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
          dRdX = dRdX, dGdX = dGdX, dBdX = 0, dAdX = 0, dZdX = 0,
          dSdX = dSdX_reg, dTdX = dTdX_reg, dWdX = 0,
          dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
          dSdY = dSdY_reg, dTdY = dTdY_reg, dWdY = 0,
          fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
          textureMode = textureMode, tLOD = 0,
          color0 = color0, color1 = color1,
          clipRight = 640, clipHighY = 480,
          texData = texDataPerLod,
          texWMask = texWMaskPerLod,
          texHMask = texHMaskPerLod,
          texShift = texShiftPerLod,
          texLod = texLodPerLod
        )

        val refPixels = VoodooReference.voodooTriangle(refParams)
        println(s"[cc_modes/$name] sim=${simPixels.size} ref=${refPixels.size}")

        if (name == "alpha_planes") {
          // For alpha_planes: use fuzzy spatial compare for RGB (edge tolerance)
          // but ignore depth16 since the ref writes depth while HW writes alpha.
          comparePixelsFuzzy(refPixels, simPixels, s"cc_modes/$name", checkColor = true)

          // Additionally verify alpha planes: since startZ=0 and dZdX=dZdY=0,
          // without alpha planes the depth16 field would be 0.
          // With alpha planes, it should contain the alpha value (non-zero).
          val nonZeroAlpha = simPixels.values.count(_._2 != 0)
          println(s"[cc_modes/$name] pixels with non-zero alpha field: $nonZeroAlpha / ${simPixels.size}")
          assert(nonZeroAlpha > 0, s"$name: expected non-zero alpha in depth16 field with alpha planes enabled")

          // The CC alpha path is default (iter alpha passthrough): alpha = CLAMP(ia >> 12)
          // startA = 200<<12, dAdX=0 → alpha should be 200=0xC8, zero-extended to 16 bits = 0x00C8
          val alphaValues = simPixels.values.map(_._2).toSet
          println(s"[cc_modes/$name] unique alpha values: ${alphaValues.toSeq.sorted.map(v => f"0x$v%04X").mkString(", ")}")
        } else {
          comparePixelsFuzzy(refPixels, simPixels, s"cc_modes/$name")
        }
      }
    }
  }

  // ========================================================================
  // Test 8: Dithering (4x4 and 2x2 Bayer)
  // ========================================================================
  test("Dithering: 4x4 and 2x2 Bayer") {
    compiled.doSim("dithering") { dut =>
      val (driver, fbMemory, _, writtenAddrs) = setupDut(dut)

      // Reuse Gouraud test geometry: A(160,80) B(240,200) C(80,200)
      val vAx = 160 * 16
      val vAy = 80 * 16
      val vBx = 240 * 16
      val vBy = 200 * 16
      val vCx = 80 * 16
      val vCy = 200 * 16

      // Gouraud shading with gradients to exercise many frac values
      val startR = 100 << 12
      val startG = 50 << 12
      val startB = 0
      val startA = 255 << 12
      val dRdX = 1 << 12  // +1 red per pixel in X
      val dBdX = 1 << 12  // +1 blue per pixel in X

      // fbzColorPath: zero_other + add_clocal (iterated color passthrough)
      val fbzColorPath = (1 << 8) | (1 << 14)
      val sign = false

      // Sub-test configs: (name, fbzMode)
      // fbzMode bits: 0=clip, 8=dithering, 9=rgb write, 11=2x2 algorithm
      val subTests = Seq(
        ("4x4", 1 | (1 << 8) | (1 << 9)),
        ("2x2", 1 | (1 << 8) | (1 << 9) | (1 << 11))
      )

      for ((name, fbzMode) <- subTests) {
        writtenAddrs.clear()

        submitTriangle(
          driver, dut.clockDomain,
          vertexAx = vAx, vertexAy = vAy,
          vertexBx = vBx, vertexBy = vBy,
          vertexCx = vCx, vertexCy = vCy,
          startR = startR, startG = startG, startB = startB, startA = startA,
          startZ = 0, startS = 0, startT = 0, startW = 0,
          dRdX = dRdX, dGdX = 0, dBdX = dBdX, dAdX = 0, dZdX = 0,
          dSdX = 0, dTdX = 0, dWdX = 0,
          dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
          dSdY = 0, dTdY = 0, dWdY = 0,
          fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = sign,
          clipRight = 640, clipHighY = 480
        )

        dut.clockDomain.waitSampling(200000)

        val simPixels = collectPixels(fbMemory, writtenAddrs, 0)
        println(s"[dithering/$name] Simulation produced ${simPixels.size} pixels")

        val refParams = VoodooReference.fromRegisterValues(
          vertexAx = vAx, vertexAy = vAy,
          vertexBx = vBx, vertexBy = vBy,
          vertexCx = vCx, vertexCy = vCy,
          startR = startR, startG = startG, startB = startB, startA = startA,
          startZ = 0, startS = 0, startT = 0, startW = 0,
          dRdX = dRdX, dGdX = 0, dBdX = dBdX, dAdX = 0, dZdX = 0,
          dSdX = 0, dTdX = 0, dWdX = 0,
          dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
          dSdY = 0, dTdY = 0, dWdY = 0,
          fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = sign,
          clipRight = 640, clipHighY = 480
        )
        val refPixels = VoodooReference.voodooTriangle(refParams)
        println(s"[dithering/$name] Reference produced ${refPixels.size} pixels")

        comparePixelsFuzzy(refPixels, simPixels, s"dithering/$name")
      }
    }
  }

  // ========================================================================
  // Test 8: Chroma key - texture pixels matching key are discarded
  // ========================================================================
  test("Chroma key: texture pixels matching key are discarded") {
    compiled.doSim("chroma_key") { dut =>
      val (driver, fbMemory, texMemory, writtenAddrs) = setupDut(dut)

      // LOD 0 = 256x256 texture with RGB565 format
      // Columns 0-127: red (0xF800) — matches chroma key
      // Columns 128-255: green (0x07E0) — does NOT match chroma key
      val red565 = 0xF800   // R=31,G=0,B=0 → decoded R=0xFF,G=0x00,B=0x00
      val green565 = 0x07E0 // R=0,G=63,B=0 → decoded R=0x00,G=0xFF,B=0x00

      // Chroma key = pure red (matches decoded red565)
      val chromaKeyColor = 0x00FF0000

      // Write texture row 0, columns 0-255 (T=0 for all pixels since dTdX=dTdY=0)
      for (row <- 0 until 1; col <- 0 until 256) {
        val texel = if (col < 128) red565 else green565
        val addr = (row * 256 + col) * 2
        texMemory.setByte(addr, (texel & 0xff).toByte)
        texMemory.setByte(addr + 1, ((texel >> 8) & 0xff).toByte)
      }

      // Pre-decode texture for reference model (full 256x256 conceptual, but only populate rows 0-3)
      val refTexData256 = new Array[Int](256 * 256)
      for (row <- 0 until 256; col <- 0 until 256) {
        val raw = if (col < 128) red565 else green565
        refTexData256(col + row * 256) = VoodooReference.decodeTexel(raw, 0xa) // 0xa = RGB565
      }

      // Triangle: A(160,80) B(240,200) C(80,200) — same as flat/gouraud tests
      val vAx = 160 * 16
      val vAy = 80 * 16
      val vBx = 240 * 16
      val vBy = 200 * 16
      val vCx = 80 * 16
      val vCy = 200 * 16

      // S maps texels 120..~260 across the triangle width (~140px at widest)
      // texS = tmu0_s >> 28; point-sample s = texS >> 4
      // Need s to start at ~120 and increase by 1 per pixel
      // texS_start = 120 * 16 = 1920; startS_reg >> 14 = 1920 → startS_reg = 1920 << 14
      val startS_reg = 1920 << 14
      val startT_reg = 0
      // Per pixel: texS += 16 → dSdX_reg = 16 << 14
      val dSdX_reg = 16 << 14
      val dTdX_reg = 0
      val dSdY_reg = 0
      val dTdY_reg = 0

      // textureMode: RGB565 (10<<8), clampS, clampT
      val textureMode = (10 << 8) | (1 << 6) | (1 << 7)
      val tLOD = 0

      // fbzColorPath: texture passthrough + textureEnable
      val fbzColorPath = 1 |       // rgbSel=1 (TREX/texture)
        (0 << 8) |                  // zero_other=0
        (0 << 10) |                 // mselect=0 (ZERO)
        (0 << 13) |                 // reverse_blend=0 (factor=256)
        (0 << 14) |                 // add=0
        (1 << 27)                   // texture_enable

      // fbzMode: clipping + chroma key + RGB write
      val fbzMode = 1 | (1 << 1) | (1 << 9)

      writeReg(driver, REG_TEXBASEADDR, 0)

      submitTriangle(
        driver, dut.clockDomain,
        vertexAx = vAx, vertexAy = vAy,
        vertexBx = vBx, vertexBy = vBy,
        vertexCx = vCx, vertexCy = vCy,
        startR = 0, startG = 0, startB = 0, startA = 255 << 12,
        startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
        dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
        dSdX = dSdX_reg, dTdX = dTdX_reg, dWdX = 0,
        dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
        dSdY = dSdY_reg, dTdY = dTdY_reg, dWdY = 0,
        fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
        textureMode = textureMode, tLOD = tLOD,
        chromaKey = chromaKeyColor,
        clipRight = 640, clipHighY = 480
      )

      dut.clockDomain.waitSampling(200000)

      val simPixels = collectPixels(fbMemory, writtenAddrs, 0)
      println(s"[chroma_key] Simulation produced ${simPixels.size} pixels")

      // Build reference
      val texDataPerLod = Array.fill(9)(Array.empty[Int])
      texDataPerLod(0) = refTexData256
      val texWMaskPerLod = Array.fill(9)(0)
      texWMaskPerLod(0) = 255
      val texHMaskPerLod = Array.fill(9)(0)
      texHMaskPerLod(0) = 255
      val texShiftPerLod = Array.fill(9)(0)
      texShiftPerLod(0) = 8
      val texLodPerLod = Array.fill(9)(0)

      val refParams = VoodooReference.fromRegisterValues(
        vertexAx = vAx, vertexAy = vAy,
        vertexBx = vBx, vertexBy = vBy,
        vertexCx = vCx, vertexCy = vCy,
        startR = 0, startG = 0, startB = 0, startA = 255 << 12,
        startZ = 0, startS = startS_reg, startT = startT_reg, startW = 0,
        dRdX = 0, dGdX = 0, dBdX = 0, dAdX = 0, dZdX = 0,
        dSdX = dSdX_reg, dTdX = dTdX_reg, dWdX = 0,
        dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
        dSdY = dSdY_reg, dTdY = dTdY_reg, dWdY = 0,
        fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
        textureMode = textureMode, tLOD = tLOD,
        chromaKey = chromaKeyColor,
        clipRight = 640, clipHighY = 480,
        texData = texDataPerLod,
        texWMask = texWMaskPerLod,
        texHMask = texHMaskPerLod,
        texShift = texShiftPerLod,
        texLod = texLodPerLod
      )

      val refPixels = VoodooReference.voodooTriangle(refParams)
      println(s"[chroma_key] Reference produced ${refPixels.size} pixels")

      // Verify that some pixels were killed (ref should have fewer than total triangle area)
      assert(refPixels.nonEmpty, "Reference should produce some pixels")

      comparePixelsFuzzy(refPixels, simPixels, "chroma_key")
    }
  }

  // ========================================================================
  // Test 9: Alpha test - pixels failing comparison are discarded
  // ========================================================================
  test("Alpha test: pixels failing comparison are discarded") {
    compiled.doSim("alpha_test") { dut =>
      val (driver, fbMemory, _, writtenAddrs) = setupDut(dut)

      // Gouraud triangle: A(160,80) B(240,200) C(80,200)
      val vAx = 160 * 16
      val vAy = 80 * 16
      val vBx = 240 * 16
      val vBy = 200 * 16
      val vCx = 80 * 16
      val vCy = 200 * 16

      // Color: constant red=200
      val startR = 200 << 12
      val startG = 0
      val startB = 0

      // Alpha: varies across X. startA=0, dAdX=4<<12 (alpha increases 4 per pixel)
      val startA = 0
      val dAdX = 4 << 12

      // fbzColorPath: iterated color passthrough (zero_other + add_clocal)
      // Alpha: iterated alpha passthrough (alphaZeroOther + alphaAdd=ALOCAL)
      val fbzColorPath = (1 << 8) | (1 << 14) | // CCU: zero_other + add_clocal
        (1 << 17) |                               // ACU: alpha_zero_other
        (2 << 23)                                  // ACU: alpha_add=ALOCAL (2)

      // fbzMode: clipping + RGB write
      val fbzMode = 1 | (1 << 9)

      // alphaMode: enable=1, func=GREATER(4), ref=0x80
      // Pixels with combined alpha > 0x80 pass, others killed
      val alphaMode = 1 | (4 << 1) | (0x80 << 24)

      submitTriangle(
        driver, dut.clockDomain,
        vertexAx = vAx, vertexAy = vAy,
        vertexBx = vBx, vertexBy = vBy,
        vertexCx = vCx, vertexCy = vCy,
        startR = startR, startG = startG, startB = startB, startA = startA,
        startZ = 0, startS = 0, startT = 0, startW = 0,
        dRdX = 0, dGdX = 0, dBdX = 0, dAdX = dAdX, dZdX = 0,
        dSdX = 0, dTdX = 0, dWdX = 0,
        dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
        dSdY = 0, dTdY = 0, dWdY = 0,
        fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
        alphaMode = alphaMode,
        clipRight = 640, clipHighY = 480
      )

      dut.clockDomain.waitSampling(200000)

      val simPixels = collectPixels(fbMemory, writtenAddrs, 0)
      println(s"[alpha_test] Simulation produced ${simPixels.size} pixels")

      val refParams = VoodooReference.fromRegisterValues(
        vertexAx = vAx, vertexAy = vAy,
        vertexBx = vBx, vertexBy = vBy,
        vertexCx = vCx, vertexCy = vCy,
        startR = startR, startG = startG, startB = startB, startA = startA,
        startZ = 0, startS = 0, startT = 0, startW = 0,
        dRdX = 0, dGdX = 0, dBdX = 0, dAdX = dAdX, dZdX = 0,
        dSdX = 0, dTdX = 0, dWdX = 0,
        dRdY = 0, dGdY = 0, dBdY = 0, dAdY = 0, dZdY = 0,
        dSdY = 0, dTdY = 0, dWdY = 0,
        fbzColorPath = fbzColorPath, fbzMode = fbzMode, sign = false,
        alphaMode = alphaMode,
        clipRight = 640, clipHighY = 480
      )

      val refPixels = VoodooReference.voodooTriangle(refParams)
      println(s"[alpha_test] Reference produced ${refPixels.size} pixels")

      // Verify that some pixels were killed
      assert(refPixels.nonEmpty, "Reference should produce some pixels")

      comparePixelsFuzzy(refPixels, simPixels, "alpha_test")
    }
  }
}
