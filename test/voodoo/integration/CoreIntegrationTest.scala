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
  val REG_COLOR0 = 0x144
  val REG_COLOR1 = 0x148
  val REG_TEXTUREMODE = 0x300
  val REG_TLOD = 0x304
  val REG_TEXBASEADDR = 0x30c

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
  // Helper: compare reference pixels with simulation output
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

      // Compare
      comparePixels(refPixels, simPixels, "flat_shaded")
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

      comparePixels(refPixels, simPixels, "gouraud_shaded")
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

      // textureMode: format=ARGB4444 (0xc << 8), clampS (bit 6), clampT (bit 7), no perspective (bit 0 = 0)
      val textureMode = (0xc << 8) | (1 << 6) | (1 << 7)

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

      comparePixels(refPixels, simPixels, "textured")
    }
  }
}
