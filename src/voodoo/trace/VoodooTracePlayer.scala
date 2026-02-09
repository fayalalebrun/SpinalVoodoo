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
      // Try trace.bin.idx first, then index.csv in the same directory
      val idxPath = config.traceFile.getAbsolutePath + ".idx"
      val f = new File(idxPath)
      if (f.exists()) Some(f)
      else {
        // Look for index.csv in the trace file's parent directory
        val csvPath = new File(config.traceFile.getParentFile, "index.csv")
        if (csvPath.exists()) Some(csvPath) else None
      }
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

      // Load register names from the elaborated RegisterBank's RegIf interface
      RegisterNames.loadFromRegBank(dut.regBank)

      // Set up hardware register reader - reads directly from DUT's regif slices
      // Build a map of address -> (name, fields) for reading during simulation
      // Test accessibility ONCE during setup to avoid repeated error messages
      val regBank = dut.regBank

      // Redirect stderr during accessibility testing to suppress SpinalHDL's warnings
      val originalErr = System.err
      val devNull = new java.io.PrintStream(new java.io.OutputStream {
        override def write(b: Int): Unit = {}
      })

      val regFieldsMap = regBank.busif.slices.map { slice =>
        val addr = slice.getAddr().toLong
        val name = slice.getDoc()
        // Get fields and their bit positions for combining into full register value
        // Only include fields that are accessible (test once during setup)
        val fields = slice.getFields().flatMap { field =>
          val start = field.getSection().start
          val end = field.getSection().end
          val hardbit = field.hardbit
          // Test accessibility by trying to read once (suppress errors)
          System.setErr(devNull)
          val accessible =
            try {
              hardbit.toBigInt // Test read
              true
            } catch {
              case _: Exception => false
            } finally {
              System.setErr(originalErr)
            }
          if (accessible) Some((start, end, hardbit)) else None
        }
        (addr, name, fields)
      }.toSeq

      val totalFields = regFieldsMap.map(_._3.size).sum
      println(
        s"[DEBUG] Hardware register reader: ${regFieldsMap.size} registers, $totalFields accessible fields"
      )

      // TMU config registers need to be read directly from output signals
      // because they are WO (write-only) and regif hardbits may not be readable
      val tmuConfig = regBank.tmuConfig

      display.setHardwareRegisterReader(() => {
        // Read regif fields first
        val regifRegisters = regFieldsMap.map { case (addr, name, fields) =>
          // Combine all field values into a single register value
          var value = 0L
          for ((start, end, hardbit) <- fields) {
            val fieldValue = hardbit.toBigInt.toLong
            val mask = ((1L << (end - start + 1)) - 1) << start
            value = (value & ~mask) | ((fieldValue << start) & mask)
          }
          (addr, name, value)
        }

        // Add TMU registers from output signals directly (override regif values)
        val tmuRegisters = Seq(
          (0x300L, "textureMode", tmuConfig.textureMode.toBigInt.toLong),
          (0x304L, "tLOD", tmuConfig.tLOD.toBigInt.toLong),
          (0x308L, "tDetail", tmuConfig.tDetail.toBigInt.toLong),
          (0x30cL, "texBaseAddr", tmuConfig.texBaseAddr.toBigInt.toLong)
        )

        // Merge: TMU registers override any regif values at same address
        val tmuAddrSet = tmuRegisters.map(_._1).toSet
        regifRegisters.filterNot(r => tmuAddrSet.contains(r._1)) ++ tmuRegisters
      })

      // Tracked gradient values for creating triangles (for debug UI only)
      var startR, startG, startB, startZ, startA, startW = 0L
      var startS_trace, startT_trace, startW_trace = 0L // Track S/T/W separately for debug
      // Track texBaseAddr for texture memory writes (address/8 format, like hardware register)
      var currentTexBaseAddr = 0L
      var dRdX, dGdX, dBdX, dZdX, dAdX, dWdX = 0L
      var dSdX_trace, dTdX_trace, dWdX_trace = 0L
      var dRdY, dGdY, dBdY, dZdY, dAdY, dWdY = 0L
      var dSdY_trace, dTdY_trace, dWdY_trace = 0L

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

      // Load initial state for the specified frame (or frame 0) FIRST
      // to get texBaseAddr before setting up texture memory ports
      val stateFrameNum = config.frameNum.getOrElse(0)
      val traceDir = StateParser.getTraceDir(config.traceFile)
      val stateOpt = StateParser.findStateFile(traceDir, stateFrameNum).map { stateFile =>
        println(s"[DEBUG] Loading state for frame $stateFrameNum from ${stateFile.getName}")
        val stateParser = new StateParser(stateFile)
        val state = stateParser.loadState()
        stateParser.close()
        state
      }

      // Extract texBaseAddr from state registers
      // IMPORTANT: In the 86Box state file:
      // - 0x190 = params.texBaseAddr[0] (already shifted to byte address!)
      // - 0x191 = params.texBaseAddr[1] (already shifted to byte address!)
      // - 0x30c = fb_write_offset (NOT texBaseAddr!)
      // The values at 0x190/0x191 are already byte addresses (shifted by 3)
      val texBaseAddr0 = stateOpt
        .flatMap(_.registers.find(_.addr == 0x190))
        .map(_.value)
        .getOrElse(0L)
      val texBaseAddr1 = stateOpt
        .flatMap(_.registers.find(_.addr == 0x191))
        .map(_.value)
        .getOrElse(0L)

      println(f"[DEBUG] texBaseAddr0 = 0x$texBaseAddr0%06x, texBaseAddr1 = 0x$texBaseAddr1%06x")

      // Create texture memory model (4MB for each TMU, typical Voodoo config)
      // Use busAddress = 0 since we handle texBaseAddr in both writes and reads:
      // - Texture writes: addr = texBaseAddr * 8 + offset (computed in trace player)
      // - TMU reads: addr = texBaseAddr * 8 + LOD_offset + texel_offset (computed in hardware)
      // Both use absolute addresses, so no offset translation needed
      val texMemSize = 4 * 1024 * 1024L
      val textureMemory = TextureMemoryModel(texMemSize)
      textureMemory.addReadPort(dut.io.texRead, 0, dut.clockDomain) // busAddress = 0
      println("[DEBUG] Texture memory model created (4MB)")

      // Wire up texture memory to display for texture viewer tab
      display.setTextureMemory(textureMemory)

      stateOpt match {
        case Some(state) =>
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

          // Load texture memory for TMU 0 at address 0
          // The busAddress offset will translate TMU reads appropriately
          println(s"[DEBUG] Loading TMU0 texture: ${state.texture0.length} bytes")
          // Check if texture data is non-zero
          val nonZeroCount = state.texture0.count(_ != 0)
          val firstNonZeroIdx = state.texture0.indexWhere(_ != 0)
          println(
            f"[DEBUG] Texture data: $nonZeroCount%d non-zero bytes, first at index $firstNonZeroIdx"
          )

          if (firstNonZeroIdx >= 0 && firstNonZeroIdx < state.texture0.length - 8) {
            val sample = state.texture0
              .slice(firstNonZeroIdx, firstNonZeroIdx + 8)
              .map(b => f"${b & 0xff}%02x")
              .mkString(" ")
            println(s"[DEBUG] Texture sample at $firstNonZeroIdx: $sample")
          }
          textureMemory.loadData(0, state.texture0)

          // Load texture memory for TMU 1 if present
          state.texture1.foreach { tex1 =>
            println(s"[DEBUG] Loading TMU1 texture: ${tex1.length} bytes")
            // TMU1 uses a separate memory space - for now just load to same model
            // In a real implementation, you might have separate texture memories
            textureMemory.loadData(0, tex1)
          }

          // Log loaded state (registers read from hardware, not state file)
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
        var texWriteCount = 0

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
              // Check if address is in valid register space:
              // - Standard registers: 0x000-0x3FF (ignore chip field in bits 13:10 for now)
              // - Remapped registers: 0x200000-0x2003FF (bit 21 set for fbiInit3 remap mode)
              // Note: We ignore the chip field and only support single TMU (Voodoo 1 level)
              val baseAddr = entry.addr & 0x3ff // Extract register field only
              val isStandardReg =
                (entry.addr & 0x3fc000) == 0 || (entry.addr & 0x3fc000) != 0 // Accept any chip field
              val isRemappedReg = (entry.addr & 0x200000) != 0
              val regAddr = if (isRemappedReg) (0x200000 | baseAddr) else baseAddr
              if (baseAddr > 0x3ff) {
                // This is a mislabeled LFB write, skip it
              } else {

                // Track triangle geometry values and gradients
                regAddr match {
                  case 0x008 => vax = entry.data
                  case 0x00c => vay = entry.data
                  case 0x010 => vbx = entry.data
                  case 0x014 => vby = entry.data
                  case 0x018 => vcx = entry.data
                  case 0x01c => vcy = entry.data
                  // Start values (R, G, B, Z, A, S, T, W)
                  case 0x020 => startR = entry.data
                  case 0x024 => startG = entry.data
                  case 0x028 => startB = entry.data
                  case 0x02c => startZ = entry.data
                  case 0x030 => startA = entry.data
                  case 0x034 => startS_trace = entry.data // S is at 0x034, not W!
                  case 0x038 => startT_trace = entry.data // T is at 0x038
                  case 0x03c => startW_trace = entry.data; startW = entry.data // W is at 0x03c
                  // X gradients (dR/dX, dG/dX, dB/dX, dZ/dX, dA/dX, dS/dX, dT/dX, dW/dX)
                  case 0x040 => dRdX = entry.data
                  case 0x044 => dGdX = entry.data
                  case 0x048 => dBdX = entry.data
                  case 0x04c => dZdX = entry.data
                  case 0x050 => dAdX = entry.data
                  case 0x054 => dSdX_trace = entry.data // dS/dX is at 0x054
                  case 0x058 => dTdX_trace = entry.data // dT/dX is at 0x058
                  case 0x05c => dWdX_trace = entry.data; dWdX = entry.data // dW/dX is at 0x05c
                  // Y gradients (dR/dY, dG/dY, dB/dY, dZ/dY, dA/dY, dS/dY, dT/dY, dW/dY)
                  case 0x060 => dRdY = entry.data
                  case 0x064 => dGdY = entry.data
                  case 0x068 => dBdY = entry.data
                  case 0x06c => dZdY = entry.data
                  case 0x070 => dAdY = entry.data
                  case 0x074 => dSdY_trace = entry.data // dS/dY is at 0x074
                  case 0x078 => dTdY_trace = entry.data // dT/dY is at 0x078
                  case 0x07c => dWdY_trace = entry.data; dWdY = entry.data // dW/dY is at 0x07c
                  // Triangle command - log captured S/T/W values from trace
                  case 0x080 =>
                    println(
                      f"[TRI CMD] Trace captured: startS=0x${startS_trace}%08X startT=0x${startT_trace}%08X startW=0x${startW_trace}%08X"
                    )
                    println(
                      f"[TRI CMD]   dSdX=0x${dSdX_trace}%08X dTdX=0x${dTdX_trace}%08X dWdX=0x${dWdX_trace}%08X"
                    )
                    println(
                      f"[TRI CMD]   dSdY=0x${dSdY_trace}%08X dTdY=0x${dTdY_trace}%08X dWdY=0x${dWdY_trace}%08X"
                    )
                  case _ =>
                }

                // Track texBaseAddr writes for texture memory address calculation
                if (regAddr == 0x30c) {
                  currentTexBaseAddr = entry.data & 0x7ffffL // 19 bits
                  println(
                    f"[REG WRITE] texBaseAddr (0x30c) = 0x${currentTexBaseAddr}%08X (raw addr=0x${entry.addr}%08X)"
                  )
                }

                for (_ <- 0 until entry.count) {
                  bmbDriver.write(BigInt(entry.data), BigInt(regAddr))
                }
              } // end if valid register address

            case TraceCommandType.WRITE_REG_W =>
              // Single TMU support only - ignore chip field, extract register address
              val baseAddr = entry.addr & 0x3ff
              val isRemapReg = (entry.addr & 0x200000) != 0
              val regAddr = if (isRemapReg) (0x200000 | baseAddr) else baseAddr
              if (baseAddr <= 0x3ff) {
                val data16 = entry.data & 0xffff
                for (_ <- 0 until entry.count) {
                  bmbDriver.write(BigInt(data16), BigInt(regAddr))
                }
              }

            case TraceCommandType.VSYNC =>
            // Frame sync - no action needed

            case TraceCommandType.SWAP =>
            // Buffer swap - no action needed

            case TraceCommandType.WRITE_TEX_L =>
              // Texture memory write - write to texture memory model
              // Per SST-1 spec: texBaseAddr is used for BOTH texture writes AND rendering
              // The actual texture address = texBaseAddr * 8 + offset
              // The entry.addr contains the PCI address; we extract the offset and add texBaseAddr
              val textureMask = (4 * 1024 * 1024L) - 1 // 0x3FFFFF for 4MB texture memory
              val texOffset = entry.addr & textureMask // Extract offset from PCI address
              val texBaseByteAddr = currentTexBaseAddr << 3 // texBaseAddr is in units of 8 bytes
              val texAddr = (texBaseByteAddr + texOffset) & textureMask // Final address with wrap
              val texData = entry.data.toInt
              textureMemory.setInt(texAddr, texData)
              texWriteCount += 1
              if (texWriteCount <= 10 || texWriteCount % 10000 == 0) {
                println(
                  f"[TEX_WRITE] #$texWriteCount offset=0x$texOffset%06X + base=0x$texBaseByteAddr%06X -> addr=0x$texAddr%06X data=0x$texData%08X"
                )
              }

            case _ =>
            // Ignore other command types (FB writes, reads)
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
    dut.io.statusInputs.memFifoFree #= 0xffff
    dut.io.statusInputs.pciInterrupt #= false

    // Initialize statistics inputs
    dut.io.statisticsIn.pixelsIn #= 0
    dut.io.statisticsIn.chromaFail #= 0
    dut.io.statisticsIn.zFuncFail #= 0
    dut.io.statisticsIn.aFuncFail #= 0
    dut.io.statisticsIn.pixelsOut #= 0

    // Monitor triangles passing through pipeline (after arbiter)
    var triInputCount = 0
    StreamMonitor(dut.triangleSetup.i, dut.clockDomain) { payload =>
      val tri = payload.triWithSign.tri
      val sign = payload.triWithSign.signBit.toBoolean

      // Debug: print S/T/W gradient values captured from registers
      if (triInputCount < 10) {
        val sStartRaw = payload.grads.sGrad.start.raw.toBigInt
        val tStartRaw = payload.grads.tGrad.start.raw.toBigInt
        val wStartRaw = payload.grads.wGrad.start.raw.toBigInt
        val sDxRaw = payload.grads.sGrad.d(0).raw.toBigInt
        val tDxRaw = payload.grads.tGrad.d(0).raw.toBigInt
        val wDxRaw = payload.grads.wGrad.d(0).raw.toBigInt
        val sDyRaw = payload.grads.sGrad.d(1).raw.toBigInt
        val tDyRaw = payload.grads.tGrad.d(1).raw.toBigInt
        val wDyRaw = payload.grads.wGrad.d(1).raw.toBigInt
        println(
          f"[TRI GRADS] $triInputCount: startS=0x$sStartRaw%08X startT=0x$tStartRaw%08X startW=0x$wStartRaw%08X"
        )
        println(f"[TRI GRADS]   dSdX=0x$sDxRaw%08X dTdX=0x$tDxRaw%08X dWdX=0x$wDxRaw%08X")
        println(f"[TRI GRADS]   dSdY=0x$sDyRaw%08X dTdY=0x$tDyRaw%08X dWdY=0x$wDyRaw%08X")
        triInputCount += 1
      }

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

      // Add pixel to debug tracker with iterated color values
      debugTracker.addPixel(
        x = x,
        y = y,
        r = pixel.grads.redGrad.toDouble,
        g = pixel.grads.greenGrad.toDouble,
        b = pixel.grads.blueGrad.toDouble,
        depth = pixel.grads.depthGrad.toDouble,
        alpha = pixel.grads.alphaGrad.toDouble,
        timestamp = simTime()
      )

      // Use per-triangle color for rasterizer visualization (same as framebuffer overlay)
      // currentTriangleColor is RGB565 format, convert to 8-bit RGB
      val r5 = (currentTriangleColor >> 11) & 0x1f
      val g6 = (currentTriangleColor >> 5) & 0x3f
      val b5 = currentTriangleColor & 0x1f
      val rInt = (r5 * 255) / 31
      val gInt = (g6 * 255) / 63
      val bInt = (b5 * 255) / 31
      display.rasterizerFramebuffer.writePixel(x, y, rInt, gInt, bInt)

      // Add pixel hit to rasterizer overlay (if tracking enabled)
      if (display.overlayTrackingEnabled) {
        display.rasterizerOverlay.addPixelHit(x, y)
      }
    }

    // Monitor TMU texture read requests - debug what addresses are being read
    var tmuReadCount = 0
    StreamMonitor(dut.io.texRead.cmd, dut.clockDomain) { payload =>
      val addr = payload.fragment.address.toLong
      if (tmuReadCount < 10 || (addr >= 0x6000 && tmuReadCount < 30)) {
        println(f"[TMU READ] request $tmuReadCount: addr=0x$addr%06x")
      }
      tmuReadCount += 1
    }

    // Monitor TMU texture read responses - debug what data is returned
    var tmuRspCount = 0
    StreamMonitor(dut.io.texRead.rsp, dut.clockDomain) { payload =>
      if (tmuRspCount < 10) {
        val data = payload.fragment.data.toLong
        println(f"[TMU RSP] response $tmuRspCount: data=0x$data%08x")
        tmuRspCount += 1
      }
    }

    // Monitor TMU input - S/T coordinates and config
    var tmuInputCount = 0
    StreamMonitor(dut.tmu.io.input, dut.clockDomain) { payload =>
      if (tmuInputCount < 20) {
        val x = payload.coords(0).toInt
        val y = payload.coords(1).toInt
        val sRaw = payload.s.raw.toBigInt
        val tRaw = payload.t.raw.toBigInt
        val wRaw = payload.w.raw.toBigInt
        val s = payload.s.toDouble
        val t = payload.t.toDouble
        val w = payload.w.toDouble
        val texMode = payload.config.textureMode.toBigInt
        val texBase = payload.config.texBaseAddr.toBigInt
        println(
          f"[TMU IN] $tmuInputCount: ($x,$y) S=0x$sRaw%08X ($s%.4f) T=0x$tRaw%08X ($t%.4f) W=0x$wRaw%08X ($w%.6f)"
        )
        println(f"[TMU IN]   mode=0x$texMode%08X base=0x$texBase%06X")
        tmuInputCount += 1
      }
    }

    // Monitor TMU output - texture color from single TMU
    var tmuPixelCount = 0
    StreamMonitor(dut.tmu.io.output, dut.clockDomain) { payload =>
      val x = payload.coords(0).toInt
      val y = payload.coords(1).toInt
      val r = payload.texture.r.toInt
      val g = payload.texture.g.toInt
      val b = payload.texture.b.toInt
      // Debug: print first few TMU outputs
      if (tmuPixelCount < 20) {
        println(f"[TMU OUT] $tmuPixelCount: ($x, $y) RGB=($r, $g, $b)")
        tmuPixelCount += 1
      }
      display.tmuFramebuffer.writePixel(x, y, r, g, b)
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
      "tmu.io.input",
      "tmu.io.output",
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

      // TMU input
      checkStreamStall(
        "tmu.io.input",
        dut.tmu.io.input.valid.toBoolean,
        dut.tmu.io.input.ready.toBoolean
      )

      // TMU output
      checkStreamStall(
        "tmu.io.output",
        dut.tmu.io.output.valid.toBoolean,
        dut.tmu.io.output.ready.toBoolean
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
