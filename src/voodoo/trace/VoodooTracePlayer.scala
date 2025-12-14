package voodoo.trace

import spinal.core._
import spinal.core.sim._
import spinal.lib.bus.bmb.sim.BmbDriver
import spinal.lib.sim.StreamMonitor
import voodoo._
import java.io.File

/** Main application for replaying Voodoo trace files
  *
  * Parses binary trace files and drives the Core hardware accelerator through register writes,
  * displaying results in real-time.
  */
object VoodooTracePlayer {

  case class Config(
      traceFile: File = null,
      indexFile: Option[File] = None,
      frameNum: Option[Int] = None,
      timingMode: TimingMode = TimingMode.FreeRun,
      resolution: (Int, Int) = (640, 480)
  )

  sealed trait TimingMode
  object TimingMode {
    case object Accurate extends TimingMode
    case object FreeRun extends TimingMode
  }

  def main(args: Array[String]): Unit = {
    val parser = new scopt.OptionParser[Config]("voodoo-trace-player") {
      head("Voodoo Trace Player", "1.0")

      opt[File]("trace")
        .required()
        .valueName("<file>")
        .action((x, c) => c.copy(traceFile = x))
        .text("Binary trace file (.bin)")

      opt[File]("index")
        .optional()
        .valueName("<file>")
        .action((x, c) => c.copy(indexFile = Some(x)))
        .text("Frame index file (.idx)")

      opt[Int]("frame")
        .optional()
        .valueName("<n>")
        .action((x, c) => c.copy(frameNum = Some(x)))
        .text("Frame number to render (requires --index)")

      opt[String]("timing")
        .optional()
        .valueName("<mode>")
        .action((x, c) =>
          c.copy(timingMode = x match {
            case "accurate" => TimingMode.Accurate
            case "freerun"  => TimingMode.FreeRun
            case _          => TimingMode.FreeRun
          })
        )
        .text("Timing mode: accurate or freerun (default: freerun)")

      opt[(Int, Int)]("resolution")
        .optional()
        .valueName("<WxH>")
        .action((x, c) => c.copy(resolution = x))
        .text("Display resolution (default: 640x480)")

      help("help").text("Show this help message")
    }

    parser.parse(args, Config()) match {
      case Some(config) => run(config)
      case None         => System.exit(1)
    }
  }

  def run(config: Config): Unit = {
    // Parse trace file
    val indexFile = config.indexFile.orElse {
      val idxPath = config.traceFile.getAbsolutePath + ".idx"
      val f = new File(idxPath)
      if (f.exists()) Some(f) else None
    }

    val parser = new TraceParser(config.traceFile, indexFile)

    // Get entries to replay
    val entries = config.frameNum match {
      case Some(frameNum) =>
        parser.readFrameEntries(frameNum) match {
          case Some(iter) => iter
          case None       =>
            System.err.println(s"ERROR: Frame $frameNum not found in index")
            System.exit(1)
            Iterator.empty
        }
      case None =>
        parser.readAllEntries()
    }

    // Create display window and debug tracker
    val (width, height) = config.resolution
    val voodooConfig = voodoo.Config.voodoo1()
    val stride = voodooConfig.fbPixelStride
    val debugTracker = new DebugTracker(stride = stride)
    val display = DisplayWindow(width, height, stride, Some(debugTracker))

    // Run simulation
    SimConfig.withVerilator.withWave.compile(Core(voodooConfig)).doSim { dut =>
      // Tracked gradient values for creating triangles
      var startR, startG, startB, startZ, startA, startW = 0L
      var dRdX, dGdX, dBdX, dZdX, dAdX, dWdX = 0L
      var dRdY, dGdY, dBdY, dZdY, dAdY, dWdY = 0L

      setupDut(
        dut,
        debugTracker,
        () =>
          TriangleGradients(
            startR = startR.toDouble,
            startG = startG.toDouble,
            startB = startB.toDouble,
            startZ = startZ.toDouble,
            startA = startA.toDouble,
            startW = startW.toDouble,
            dRdX = dRdX.toDouble,
            dGdX = dGdX.toDouble,
            dBdX = dBdX.toDouble,
            dZdX = dZdX.toDouble,
            dAdX = dAdX.toDouble,
            dWdX = dWdX.toDouble,
            dRdY = dRdY.toDouble,
            dGdY = dGdY.toDouble,
            dBdY = dBdY.toDouble,
            dZdY = dZdY.toDouble,
            dAdY = dAdY.toDouble,
            dWdY = dWdY.toDouble
          )
      )

      // Monitor TriangleSetup output - shows computed edge equations
      StreamMonitor(dut.triangleSetup.o, dut.clockDomain) { _ =>
        // Extract edge equations (a, b, c coefficients)
        val edge0a = dut.triangleSetup.o.coeffs(0).a.toDouble
        val edge0b = dut.triangleSetup.o.coeffs(0).b.toDouble
        val edge0c = dut.triangleSetup.o.coeffs(0).c.toDouble

        val edge1a = dut.triangleSetup.o.coeffs(1).a.toDouble
        val edge1b = dut.triangleSetup.o.coeffs(1).b.toDouble
        val edge1c = dut.triangleSetup.o.coeffs(1).c.toDouble

        val edge2a = dut.triangleSetup.o.coeffs(2).a.toDouble
        val edge2b = dut.triangleSetup.o.coeffs(2).b.toDouble
        val edge2c = dut.triangleSetup.o.coeffs(2).c.toDouble

        // Extract bounding box
        val xmin = dut.triangleSetup.o.xrange(0).toDouble
        val xmax = dut.triangleSetup.o.xrange(1).toDouble
        val ymin = dut.triangleSetup.o.yrange(0).toDouble
        val ymax = dut.triangleSetup.o.yrange(1).toDouble

        // Update debug tracker with coefficients and bounding box
        debugTracker.updateTriangleSetup(
          coefficients = Array(
            (edge0a, edge0b, edge0c),
            (edge1a, edge1b, edge1c),
            (edge2a, edge2b, edge2c)
          ),
          boundingBox = (xmin.toInt, ymin.toInt, xmax.toInt, ymax.toInt)
        )

      }

      // Monitor rasterizer input - triangle enters rasterizer
      StreamMonitor(dut.rasterizer.i, dut.clockDomain) { _ =>
        debugTracker.pushToRasterizer()
      }

      val fbSize = parser.header.fbSizeMb * 1024 * 1024
      val framebuffer = FramebufferModel(fbSize, Some(display))
      framebuffer.addPort(dut.io.fbWrite, 0, dut.clockDomain)

      val bmbDriver = new BmbDriver(dut.io.regBus, dut.clockDomain)

      // Set framebuffer base address
      dut.io.fbBaseAddr #= 0

      // Main replay loop
      var currentCycle = 0L
      var lastTimestamp = 0L
      var entryCount = 0

      // Track triangle geometry for logging (vertices only - gradients tracked above)
      var vax, vay, vbx, vby, vcx, vcy = 0L

      for (entry <- entries) {
        entryCount += 1
        // Handle timing if in accurate mode
        config.timingMode match {
          case TimingMode.Accurate =>
            val targetCycle = entry.timestamp
            while (currentCycle < targetCycle) {
              dut.clockDomain.waitSampling()
              currentCycle += 1
            }
          case TimingMode.FreeRun =>
          // Just execute commands as fast as possible
        }

        // Process command based on type
        entry.cmdType match {
          case TraceCommandType.WRITE_REG_L =>
            // Repeat the command 'count' times if coalesced
            // Mask address to 12 bits for register bus
            val regAddr = entry.addr & 0xfff

            // Track triangle geometry values and gradients
            regAddr match {
              case 0x008 => vax = entry.data
              case 0x00c => vay = entry.data
              case 0x010 => vbx = entry.data
              case 0x014 => vby = entry.data
              case 0x018 => vcx = entry.data
              case 0x01c => vcy = entry.data
              // Start values (R, G, B, Z, A, W)
              case 0x020 => startR = entry.data
              case 0x024 => startG = entry.data
              case 0x028 => startB = entry.data
              case 0x02c => startZ = entry.data
              case 0x030 => startA = entry.data
              case 0x034 => startW = entry.data
              // X gradients (dR/dX, dG/dX, dB/dX, dZ/dX, dA/dX, dW/dX)
              case 0x040 => dRdX = entry.data
              case 0x044 => dGdX = entry.data
              case 0x048 => dBdX = entry.data
              case 0x04c => dZdX = entry.data
              case 0x050 => dAdX = entry.data
              case 0x054 => dWdX = entry.data
              // Y gradients (dR/dY, dG/dY, dB/dY, dZ/dY, dA/dY, dW/dY)
              case 0x060 => dRdY = entry.data
              case 0x064 => dGdY = entry.data
              case 0x068 => dBdY = entry.data
              case 0x06c => dZdY = entry.data
              case 0x070 => dAdY = entry.data
              case 0x074 => dWdY = entry.data
              case _     =>
            }

            for (_ <- 0 until entry.count) {
              bmbDriver.write(BigInt(entry.data), BigInt(regAddr))
            }

          case TraceCommandType.WRITE_REG_W =>
            // Mask address to 12 bits for register bus
            val regAddr = entry.addr & 0xfff
            val data16 = entry.data & 0xffff
            for (_ <- 0 until entry.count) {
              bmbDriver.write(BigInt(data16), BigInt(regAddr))
            }

          case TraceCommandType.VSYNC =>
          // Frame sync - no action needed

          case TraceCommandType.SWAP =>
          // Buffer swap - no action needed

          case _ =>
          // Ignore other command types (FB/TEX writes, reads)
        }

        lastTimestamp = entry.timestamp
      }

      // Let simulation run for a bit longer to finish rendering
      for (_ <- 0 until 10000) {
        dut.clockDomain.waitSampling()
      }

      // Keep simulation alive while display is open
      while (true) {
        Thread.sleep(100)
      }
    }

    parser.close()
  }

  private def setupDut(
      dut: Core,
      debugTracker: DebugTracker,
      captureGradients: () => TriangleGradients
  ): Unit = {
    dut.clockDomain.forkStimulus(10)

    // Random color for visual debugging - each triangle gets a unique color
    var currentTriangleColor = 0 // RGB565 format

    // Track rasterizer running state for edge detection
    var prevRunning = false

    // Override framebuffer write data with random colors for visual debugging
    // Use fork to continuously monitor and override data before it's sampled
    fork {
      while (true) {
        dut.clockDomain.waitSampling()
        if (dut.io.fbWrite.cmd.valid.toBoolean) {
          // Extract current data for logging
          val originalData = dut.io.fbWrite.cmd.fragment.data.toLong

          // Override data with random color (only bottom 16 bits for RGB565)
          dut.io.fbWrite.cmd.fragment.data #= BigInt(currentTriangleColor)

          // Extract RGB565 components for logging
          val r5 = (currentTriangleColor >> 11) & 0x1f
          val g6 = (currentTriangleColor >> 5) & 0x3f
          val b5 = currentTriangleColor & 0x1f
          val origR5 = (originalData >> 11) & 0x1f
          val origG6 = (originalData >> 5) & 0x3f
          val origB5 = originalData & 0x1f

          val addr = dut.io.fbWrite.cmd.fragment.address.toLong
          // println(
          //   f"[FBWrite] 0x${addr}%08x: RGB565=0x${currentTriangleColor}%04x (R=$r5%2d G=$g6%2d B=$b5%2d) [was 0x${originalData}%04x (R=$origR5%2d G=$origG6%2d B=$origB5%2d)]"
          // )
        }
      }
    }

    // Initialize status inputs (pciFifoFree now comes from internal FIFO)
    dut.io.statusInputs.vRetrace #= false
    dut.io.statusInputs.fbiBusy #= false
    dut.io.statusInputs.trexBusy #= false
    dut.io.statusInputs.sstBusy #= false
    dut.io.statusInputs.displayedBuffer #= 0
    dut.io.statusInputs.memFifoFree #= 0xffff
    dut.io.statusInputs.swapsPending #= 0
    dut.io.statusInputs.pciInterrupt #= false

    // Initialize statistics inputs
    dut.io.statisticsIn.pixelsIn #= 0
    dut.io.statisticsIn.chromaFail #= 0
    dut.io.statisticsIn.zFuncFail #= 0
    dut.io.statisticsIn.aFuncFail #= 0
    dut.io.statisticsIn.pixelsOut #= 0

    // Monitor triangles passing through pipeline (after arbiter)
    StreamMonitor(dut.triangleSetup.i, dut.clockDomain) { payload =>
      val tri = payload.triWithSign.tri
      val sign = payload.triWithSign.signBit.toBoolean

      // Generate random RGB565 color for this triangle
      val r5 = scala.util.Random.nextInt(32) // 5 bits (0-31)
      val g6 = scala.util.Random.nextInt(64) // 6 bits (0-63)
      val b5 = scala.util.Random.nextInt(32) // 5 bits (0-31)
      currentTriangleColor = (r5 << 11) | (g6 << 5) | b5

      // Extract vertices for debug tracker
      val vertices = Array(
        (tri(0)(0).toDouble, tri(0)(1).toDouble),
        (tri(1)(0).toDouble, tri(1)(1).toDouble),
        (tri(2)(0).toDouble, tri(2)(1).toDouble)
      )

      // Create triangle in debug tracker
      debugTracker.createTriangle(
        vertices = vertices,
        signBit = sign,
        gradients = captureGradients(),
        timestamp = simTime()
      )
    }

    // Monitor rasterizer output (pixels)
    StreamMonitor(dut.rasterizer.o, dut.clockDomain) { pixel =>
      val x = pixel.coords(0).toInt
      val y = pixel.coords(1).toInt
      val r = pixel.grads.redGrad.toDouble
      val g = pixel.grads.greenGrad.toDouble
      val b = pixel.grads.blueGrad.toDouble
      val z = pixel.grads.depthGrad.toDouble
      val a = pixel.grads.alphaGrad.toDouble

      // Add pixel to debug tracker
      debugTracker.addPixel(
        x = x,
        y = y,
        r = r,
        g = g,
        b = b,
        depth = z,
        alpha = a,
        timestamp = simTime()
      )
    }

    // Monitor rasterizer running signal for triangle completion
    dut.clockDomain.onSamplings {
      val running = dut.rasterizer.running.toBoolean
      if (prevRunning && !running) {
        // Falling edge: triangle finished rasterizing
        debugTracker.rasterizerFinished()
      }
      prevRunning = running
    }

    dut.clockDomain.waitSampling()
  }
}
