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

    // Get entries to replay - collect into array for trace viewer
    val entriesArray = (config.frameNum match {
      case Some(frameNum) =>
        // Print available frames for debugging
        parser.frameIndex match {
          case Some(frames) =>
            println(s"[DEBUG] Index loaded with ${frames.length} frames")
            println(
              s"[DEBUG] Frame numbers available: ${frames.map(_.frameNum).take(10).mkString(", ")}${
                  if (frames.length > 10) "..." else ""
                }"
            )
          case None =>
            println(s"[DEBUG] No frame index loaded!")
        }
        parser.readFrameEntries(frameNum) match {
          case Some(iter) =>
            println(s"[DEBUG] Loading frame $frameNum")
            iter
          case None =>
            System.err.println(s"ERROR: Frame $frameNum not found in index")
            System.exit(1)
            Iterator.empty
        }
      case None =>
        parser.readAllEntries()
    }).toArray

    println(s"[DEBUG] Loaded ${entriesArray.length} trace entries")

    // Create display window and debug tracker
    val (width, height) = config.resolution
    val voodooConfig = voodoo.Config.voodoo1()
    val stride = voodooConfig.fbPixelStride
    val debugTracker = new DebugTracker(stride = stride)
    val display = DisplayWindow(width, height, stride, Some(debugTracker))

    // Populate trace viewer
    println(s"[DEBUG] Populating trace viewer with ${entriesArray.length} entries...")
    for ((entry, idx) <- entriesArray.zipWithIndex) {
      display.addTraceEntry(idx, entry.cmdType, entry.addr, entry.data, entry.count)
    }
    display.finalizeTraceLoading()
    println("[DEBUG] Trace viewer populated")

    // Run simulation - use doSimUntilVoid to allow clean termination via simSuccess()
    SimConfig.withVerilator.withWave.compile(Core(voodooConfig)).doSimUntilVoid { dut =>
      println("[DEBUG] Simulation started, setting up DUT...")

      // Tracked gradient values for creating triangles
      var startR, startG, startB, startZ, startA, startW = 0L
      var dRdX, dGdX, dBdX, dZdX, dAdX, dWdX = 0L
      var dRdY, dGdY, dBdY, dZdY, dAdY, dWdY = 0L

      setupDut(
        dut,
        debugTracker,
        display,
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

        val coefficients = Array(
          (edge0a, edge0b, edge0c),
          (edge1a, edge1b, edge1c),
          (edge2a, edge2b, edge2c)
        )
        val bbox = (xmin.toInt, ymin.toInt, xmax.toInt, ymax.toInt)

        // Update debug tracker with coefficients and bounding box
        debugTracker.updateTriangleSetup(
          coefficients = coefficients,
          boundingBox = bbox
        )

        // Update rasterizer overlay with edge equations and vertices (if tracking enabled)
        if (display.overlayTrackingEnabled) {
          val currentTriId = debugTracker.getStats._1 // Triangle count as ID
          val vertices =
            debugTracker.getTriangle(currentTriId).map(_.vertices).getOrElse(Array.empty)
          display.rasterizerOverlay.startTriangle(currentTriId, coefficients, bbox, vertices)
          display.updateOverlayTriangleList()
        }
      }

      // Monitor rasterizer input - triangle enters rasterizer
      StreamMonitor(dut.rasterizer.i, dut.clockDomain) { _ =>
        debugTracker.pushToRasterizer()
      }

      val fbSize = parser.header.fbSizeMb * 1024 * 1024
      val framebuffer = FramebufferModel(fbSize, Some(display))
      framebuffer.addPort(dut.io.fbWrite, 0, dut.clockDomain)

      // Create texture memory model (4MB for each TMU, typical Voodoo config)
      val texMemSize = 4 * 1024 * 1024L
      val textureMemory = TextureMemoryModel(texMemSize)
      textureMemory.addReadPort(dut.io.texRead0, 0, dut.clockDomain)
      textureMemory.addReadPort(dut.io.texRead1, 0, dut.clockDomain)
      println("[DEBUG] Texture memory model created (4MB)")

      // Load initial state for the specified frame (or frame 0)
      val stateFrameNum = config.frameNum.getOrElse(0)
      val traceDir = StateParser.getTraceDir(config.traceFile)
      StateParser.findStateFile(traceDir, stateFrameNum) match {
        case Some(stateFile) =>
          println(s"[DEBUG] Loading state for frame $stateFrameNum from ${stateFile.getName}")
          val stateParser = new StateParser(stateFile)
          val state = stateParser.loadState()
          stateParser.close()

          // Load framebuffer data with format conversion from 86Box
          println(s"[DEBUG] Loading framebuffer: ${state.framebuffer.length} bytes")
          println(
            s"[DEBUG] FB layout: ${state.header.hDisp}x${state.header.vDisp}, " +
              s"rowWidth=${state.header.rowWidth}, drawOffset=${state.header.drawOffset}, " +
              s"auxOffset=${state.header.auxOffset}"
          )

          // Use conversion if we have valid layout info, otherwise fall back to raw load
          if (state.header.rowWidth > 0 && state.header.hDisp > 0 && state.header.vDisp > 0) {
            framebuffer.loadFrom86BoxFormat(
              fbMem = state.framebuffer,
              drawOffset = state.header.drawOffset,
              auxOffset = state.header.auxOffset,
              rowWidth = state.header.rowWidth,
              hDisp = state.header.hDisp.toInt,
              vDisp = state.header.vDisp.toInt,
              targetStride = stride
            )
          } else {
            println(s"[DEBUG] No layout info, skipping framebuffer load")
          }

          // Load texture memory for TMU 0
          println(s"[DEBUG] Loading TMU0 texture: ${state.texture0.length} bytes")
          textureMemory.loadData(0, state.texture0)

          // Load texture memory for TMU 1 if present
          state.texture1.foreach { tex1 =>
            println(s"[DEBUG] Loading TMU1 texture: ${tex1.length} bytes")
            // TMU1 uses a separate memory space - for now just load to same model
            // In a real implementation, you might have separate texture memories
            textureMemory.loadData(0, tex1)
          }

          // Load registers into display
          for (reg <- state.registers) {
            display.updateRegister(reg.addr, reg.value)
          }
          display.refreshRegisterTable()

          // Log loaded registers
          println(
            s"[DEBUG] State loaded: ${state.registers.length} registers, " +
              f"${state.header.fbSize}%d bytes FB, ${state.header.texSize}%d bytes TEX, " +
              s"${state.header.numTmus} TMUs"
          )

        case None =>
          println(s"[DEBUG] No state file found for frame $stateFrameNum in $traceDir")
      }

      val bmbDriver = new BmbDriver(dut.io.regBus, dut.clockDomain)
      println("[DEBUG] BMB driver created")

      // Set framebuffer base address
      dut.io.fbBaseAddr #= 0
      println("[DEBUG] Framebuffer base address set, ready to replay")

      // Flag to track if replay is complete
      @volatile var replayComplete = false

      // Fork the trace replay loop - this may block on bus writes, but that's OK
      // The main thread will monitor the window and can terminate the simulation
      fork {
        // Main replay loop
        var currentCycle = 0L
        var lastTimestamp = 0L
        var entryCount = 0

        // Track triangle geometry for logging (vertices only - gradients tracked above)
        var vax, vay, vbx, vby, vcx, vcy = 0L

        println("[DEBUG] Starting trace replay loop (forked)...")

        // Counter for register table refresh
        var lastRegisterRefresh = 0

        for ((entry, entryIndex) <- entriesArray.zipWithIndex) {
          // Check pause flag and wait while paused (unless step requested)
          while (display.isPaused && !display.stepRequested && !display.isClosed) {
            dut.clockDomain.waitSampling(10)
          }

          // Track if this was a step (for force update)
          val wasStep = display.stepRequested

          // Clear step request after processing
          if (display.stepRequested) {
            display.stepRequested = false
          }

          // Update trace position in viewer (force update on step for immediate feedback)
          display.updateTracePosition(entryIndex, forceUpdate = wasStep || display.isPaused)

          entryCount += 1
          if (entryCount % 10000 == 0)
            println(s"[DEBUG] Processing entry $entryCount...")

          // Periodically refresh register table (every 1000 entries, or on step)
          if (wasStep || entryCount - lastRegisterRefresh >= 1000) {
            display.refreshRegisterTable()
            lastRegisterRefresh = entryCount
          }

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
              // Skip if address is outside register space (LFB/texture writes mislabeled as register)
              // Registers are in range 0x000-0x3FF (10-bit address space)
              // Addresses >= 0x100000 are LFB/aux writes that should be ignored
              if ((entry.addr & 0xfff000) != 0) {
                // This is a mislabeled LFB write, skip it
              } else {
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
                // Track register state for display
                display.updateRegister(regAddr, entry.data)
              } // end if valid register address

            case TraceCommandType.WRITE_REG_W =>
              // Skip if address is outside register space (mislabeled LFB writes)
              if ((entry.addr & 0xfff000) == 0) {
                // Mask address to 12 bits for register bus
                val regAddr = entry.addr & 0xfff
                val data16 = entry.data & 0xffff
                for (_ <- 0 until entry.count) {
                  bmbDriver.write(BigInt(data16), BigInt(regAddr))
                }
                // Track register state for display
                display.updateRegister(regAddr, data16)
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

        println("[DEBUG] Trace replay complete, letting simulation run for 10000 cycles...")
        for (i <- 0 until 10000) {
          dut.clockDomain.waitSampling()
          if (i % 2000 == 0) println(s"[DEBUG] Cycle $i / 10000")
        }
        println("[DEBUG] Replay thread finished")
        replayComplete = true
      }

      // Main thread: monitor window close and terminate simulation
      // This thread stays responsive because it only does short waitSampling calls
      println("[DEBUG] Main thread monitoring window...")
      while (!display.isClosed && !replayComplete) {
        dut.clockDomain.waitSampling(100) // Short wait to stay responsive
      }

      if (display.isClosed) {
        println("[DEBUG] Window closed, terminating simulation...")
        simSuccess()
      } else {
        // Replay complete, wait for window to close
        println("[DEBUG] Replay complete, waiting for window close...")
        while (!display.isClosed) {
          dut.clockDomain.waitSampling(100)
        }
        println("[DEBUG] Window closed after replay, terminating...")
        simSuccess()
      }
    }

    println("[DEBUG] Simulation block exited")
    parser.close()
    println("Trace player finished.")
  }

  private def setupDut(
      dut: Core,
      debugTracker: DebugTracker,
      display: DisplayWindow,
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

      // Convert iterated color values to 8-bit RGB for display
      // Colors are in S18.14 format, so extract integer part (divide by 2^14)
      // and clamp to 0-255
      val rInt = Math.max(0, Math.min(255, (r / 16384.0).toInt))
      val gInt = Math.max(0, Math.min(255, (g / 16384.0).toInt))
      val bInt = Math.max(0, Math.min(255, (b / 16384.0).toInt))
      display.rasterizerFramebuffer.writePixel(x, y, rInt, gInt, bInt)

      // Add pixel hit to rasterizer overlay (if tracking enabled)
      if (display.overlayTrackingEnabled) {
        display.rasterizerOverlay.addPixelHit(x, y)
      }
    }

    // Monitor TMU0 output - texture color after first TMU
    StreamMonitor(dut.tmu0.io.output, dut.clockDomain) { payload =>
      val x = payload.coords(0).toInt
      val y = payload.coords(1).toInt
      val r = payload.texture.r.toInt
      val g = payload.texture.g.toInt
      val b = payload.texture.b.toInt
      display.tmu0Framebuffer.writePixel(x, y, r, g, b)
    }

    // Monitor TMU1 output - texture color after second TMU
    StreamMonitor(dut.tmu1.io.output, dut.clockDomain) { payload =>
      val x = payload.coords(0).toInt
      val y = payload.coords(1).toInt
      val r = payload.texture.r.toInt
      val g = payload.texture.g.toInt
      val b = payload.texture.b.toInt
      display.tmu1Framebuffer.writePixel(x, y, r, g, b)
    }

    // Monitor ColorCombine output - final color before write stage
    StreamMonitor(dut.colorCombine.io.output, dut.clockDomain) { payload =>
      val x = payload.coords(0).toInt
      val y = payload.coords(1).toInt
      val r = payload.color.r.toInt
      val g = payload.color.g.toInt
      val b = payload.color.b.toInt
      display.colorCombineFramebuffer.writePixel(x, y, r, g, b)
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

    // Stream stall monitoring - detect when streams are stalled for too long
    val stallThreshold = 500
    val streamStallCounters = scala.collection.mutable.Map[String, Long]()
    val streamStallWarned = scala.collection.mutable.Set[String]()

    // Define the pipeline stream path (input to output)
    val streamPath = Seq(
      "triangleSetup.i",
      "triangleSetup.o",
      "rasterizer.o",
      "tmu0.io.input",
      "tmu0.io.output",
      "tmu1.io.input",
      "tmu1.io.output",
      "colorCombine.io.input",
      "colorCombine.io.output",
      "write.i.fromPipeline",
      "write.o.fbWrite.cmd"
    )

    // Print the stream path at startup
    println("[STALL MONITOR] Pipeline stream path:")
    streamPath.zipWithIndex.foreach { case (name, idx) =>
      val arrow = if (idx < streamPath.length - 1) " ->" else ""
      println(f"  [$idx%2d] $name$arrow")
    }

    // Helper to check a stream's stall status
    def checkStreamStall(name: String, valid: Boolean, ready: Boolean): Unit = {
      val isStall = valid && !ready
      if (isStall) {
        val count = streamStallCounters.getOrElse(name, 0L) + 1
        streamStallCounters(name) = count
        if (count >= stallThreshold && !streamStallWarned.contains(name)) {
          println(
            s"[STALL WARNING] $name stalled for $count cycles (valid=$valid, ready=$ready) at time ${simTime()}"
          )
          streamStallWarned += name
        }
      } else {
        // Reset counter when not stalled
        if (streamStallCounters.getOrElse(name, 0L) > 0) {
          streamStallCounters(name) = 0
          streamStallWarned -= name
        }
      }
    }

    // Monitor all pipeline streams for stalls
    dut.clockDomain.onSamplings {
      // Pipeline input: triangleSetup.i
      checkStreamStall(
        "triangleSetup.i",
        dut.triangleSetup.i.valid.toBoolean,
        dut.triangleSetup.i.ready.toBoolean
      )

      // TriangleSetup output / Rasterizer input
      checkStreamStall(
        "triangleSetup.o",
        dut.triangleSetup.o.valid.toBoolean,
        dut.triangleSetup.o.ready.toBoolean
      )

      // Rasterizer output
      checkStreamStall(
        "rasterizer.o",
        dut.rasterizer.o.valid.toBoolean,
        dut.rasterizer.o.ready.toBoolean
      )

      // TMU0 input
      checkStreamStall(
        "tmu0.io.input",
        dut.tmu0.io.input.valid.toBoolean,
        dut.tmu0.io.input.ready.toBoolean
      )

      // TMU0 output
      checkStreamStall(
        "tmu0.io.output",
        dut.tmu0.io.output.valid.toBoolean,
        dut.tmu0.io.output.ready.toBoolean
      )

      // TMU1 input
      checkStreamStall(
        "tmu1.io.input",
        dut.tmu1.io.input.valid.toBoolean,
        dut.tmu1.io.input.ready.toBoolean
      )

      // TMU1 output
      checkStreamStall(
        "tmu1.io.output",
        dut.tmu1.io.output.valid.toBoolean,
        dut.tmu1.io.output.ready.toBoolean
      )

      // ColorCombine input
      checkStreamStall(
        "colorCombine.io.input",
        dut.colorCombine.io.input.valid.toBoolean,
        dut.colorCombine.io.input.ready.toBoolean
      )

      // ColorCombine output
      checkStreamStall(
        "colorCombine.io.output",
        dut.colorCombine.io.output.valid.toBoolean,
        dut.colorCombine.io.output.ready.toBoolean
      )

      // Write stage input
      checkStreamStall(
        "write.i.fromPipeline",
        dut.write.i.fromPipeline.valid.toBoolean,
        dut.write.i.fromPipeline.ready.toBoolean
      )

      // Framebuffer write output (BMB cmd)
      checkStreamStall(
        "write.o.fbWrite.cmd",
        dut.write.o.fbWrite.cmd.valid.toBoolean,
        dut.write.o.fbWrite.cmd.ready.toBoolean
      )
    }

    dut.clockDomain.waitSampling()
  }
}
