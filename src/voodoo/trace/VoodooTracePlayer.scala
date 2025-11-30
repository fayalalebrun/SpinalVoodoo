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
    println(s"Config: indexFile=${config.indexFile}, frameNum=${config.frameNum}")

    val indexFile = config.indexFile.orElse {
      val idxPath = config.traceFile.getAbsolutePath + ".idx"
      val f = new File(idxPath)
      println(s"Looking for auto-detected index file: $idxPath")
      if (f.exists()) {
        println(s"  Found auto-detected index file")
        Some(f)
      } else {
        println(s"  No auto-detected index file found")
        None
      }
    }

    println(s"Using index file: $indexFile")
    val parser = new TraceParser(config.traceFile, indexFile)
    println(s"Loaded trace file: ${config.traceFile}")
    println(s"  Magic: 0x${parser.header.magic.toHexString}")
    println(s"  Version: ${parser.header.version}")
    println(s"  Voodoo Type: ${parser.header.voodooType}")
    println(s"  FB Size: ${parser.header.fbSizeMb} MB")
    println(s"  Entry Count: ${parser.header.entryCount}")

    // Get entries to replay
    val entries = config.frameNum match {
      case Some(frameNum) =>
        println(s"Attempting to read frame $frameNum from index")
        parser.readFrameEntries(frameNum) match {
          case Some(iter) =>
            println(s"Successfully loaded frame $frameNum from index")
            iter
          case None =>
            println(s"ERROR: Frame $frameNum not found in index")
            System.exit(1)
            Iterator.empty
        }
      case None =>
        println("Rendering all frames (no frame number specified)")
        parser.readAllEntries()
    }

    // Create display window
    val (width, height) = config.resolution
    val voodooConfig = voodoo.Config.voodoo1()
    val stride = voodooConfig.fbPixelStride
    val display = DisplayWindow(width, height, stride)

    // Run simulation
    SimConfig.withVerilator.withWave.compile(Core(voodooConfig)).doSim { dut =>
      setupDut(dut)

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

        println(s"\n===== TRIANGLE SETUP OUTPUT =====")
        println(f"Bounding box: x=[$xmin%.2f, $xmax%.2f] y=[$ymin%.2f, $ymax%.2f]")
        println(f"Edge 0: ${edge0a}%9.2f*x + ${edge0b}%9.2f*y + ${edge0c}%11.2f = 0")
        println(f"Edge 1: ${edge1a}%9.2f*x + ${edge1b}%9.2f*y + ${edge1c}%11.2f = 0")
        println(f"Edge 2: ${edge2a}%9.2f*x + ${edge2b}%9.2f*y + ${edge2c}%11.2f = 0")
        println(s"=================================\n")
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

      // Track triangle geometry for logging
      var vax, vay, vbx, vby, vcx, vcy = 0L
      var startR, startG, startB, startZ = 0L
      var dRdX, dGdX, dBdX, dZdX = 0L
      var dRdY, dGdY, dBdY, dZdY = 0L

      println("\nFirst 10 entries:")
      for (entry <- entries) {
        // Print first 10 entries for debugging
        if (entryCount < 10) {
          println(
            f"  ts=${entry.timestamp}%16d ts_end=${entry.timestampEnd}%16d cmdType=${entry.cmdType}%2d addr=0x${entry.addr}%06x data=0x${entry.data}%08x count=${entry.count}"
          )
        } else if (entryCount == 10) {
          println() // Empty line after debug output
        }
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

            // Track triangle geometry values
            regAddr match {
              case 0x008 => vax = entry.data
              case 0x00c => vay = entry.data
              case 0x010 => vbx = entry.data
              case 0x014 => vby = entry.data
              case 0x018 => vcx = entry.data
              case 0x01c => vcy = entry.data
              case 0x020 => startR = entry.data
              case 0x024 => startG = entry.data
              case 0x028 => startB = entry.data
              case 0x02c => startZ = entry.data
              case 0x040 => dRdX = entry.data
              case 0x044 => dGdX = entry.data
              case 0x048 => dBdX = entry.data
              case 0x04c => dZdX = entry.data
              case 0x060 => dRdY = entry.data
              case 0x064 => dGdY = entry.data
              case 0x068 => dBdY = entry.data
              case 0x06c => dZdY = entry.data
              case 0x080 =>
                // Triangle command - print all geometry
                println(s"\n========== TRIANGLE COMMAND ==========")
                println(f"Vertices (fixed S15.4):")
                println(
                  f"  A: (0x${vax}%08x, 0x${vay}%08x) = (${vax / 16.0}%.2f, ${vay / 16.0}%.2f)"
                )
                println(
                  f"  B: (0x${vbx}%08x, 0x${vby}%08x) = (${vbx / 16.0}%.2f, ${vby / 16.0}%.2f)"
                )
                println(
                  f"  C: (0x${vcx}%08x, 0x${vcy}%08x) = (${vcx / 16.0}%.2f, ${vcy / 16.0}%.2f)"
                )
                println(f"Start values:")
                println(
                  f"  R=0x${startR}%08x G=0x${startG}%08x B=0x${startB}%08x Z=0x${startZ}%08x"
                )
                println(f"X Gradients:")
                println(
                  f"  dR/dX=0x${dRdX}%08x dG/dX=0x${dGdX}%08x dB/dX=0x${dBdX}%08x dZ/dX=0x${dZdX}%08x"
                )
                println(f"Y Gradients:")
                println(
                  f"  dR/dY=0x${dRdY}%08x dG/dY=0x${dGdY}%08x dB/dY=0x${dBdY}%08x dZ/dY=0x${dZdY}%08x"
                )
                println(s"======================================\n")
              case _ =>
            }

            println(
              f"WRITE_REG_L: addr=0x${regAddr}%03x data=0x${entry.data}%08x count=${entry.count}"
            )
            for (_ <- 0 until entry.count) {
              bmbDriver.write(BigInt(entry.data), BigInt(regAddr))
            }

          case TraceCommandType.WRITE_REG_W =>
            // Mask address to 12 bits for register bus
            val regAddr = entry.addr & 0xfff
            val data16 = entry.data & 0xffff
            println(f"WRITE_REG_W: addr=0x${regAddr}%03x data=0x${data16}%04x count=${entry.count}")
            for (_ <- 0 until entry.count) {
              bmbDriver.write(BigInt(data16), BigInt(regAddr))
            }

          case TraceCommandType.VSYNC =>
            val frameNum = entry.addr
            val resolution = entry.data
            val h = (resolution >> 16) & 0xffff
            val v = resolution & 0xffff
            println(s"VSYNC: Frame $frameNum, Resolution ${h}x${v}")

          case TraceCommandType.SWAP =>
            val offset = entry.addr
            println(s"SWAP: Buffer offset 0x${offset.toHexString}")

          case _ =>
          // Ignore other command types (FB/TEX writes, reads)
        }

        lastTimestamp = entry.timestamp
      }

      // Let simulation run for a bit longer to finish rendering
      println("Finishing rendering...")
      for (_ <- 0 until 10000) {
        dut.clockDomain.waitSampling()
      }

      println("Simulation complete. Press Ctrl+C to exit.")

      // Keep simulation alive while display is open
      while (true) {
        Thread.sleep(100)
      }
    }

    parser.close()
  }

  private def setupDut(dut: Core): Unit = {
    dut.clockDomain.forkStimulus(10)

    // Random color for visual debugging - each triangle gets a unique color
    var currentTriangleColor = 0 // RGB565 format

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

    // Monitor float converter results
    StreamMonitor(dut.joinedResults, dut.clockDomain) { results =>
      println(f"[JoinedResults] Float conversions:")
      println(
        f"  vax=(${results(0).number.toDouble}%7.2f) raw=0x${results(0).number.raw.toLong.toHexString}"
      )
      println(
        f"  vay=(${results(1).number.toDouble}%7.2f) raw=0x${results(1).number.raw.toLong.toHexString}"
      )
      println(
        f"  vbx=(${results(2).number.toDouble}%7.2f) raw=0x${results(2).number.raw.toLong.toHexString}"
      )
      println(
        f"  vby=(${results(3).number.toDouble}%7.2f) raw=0x${results(3).number.raw.toLong.toHexString}"
      )
      println(
        f"  vcx=(${results(4).number.toDouble}%7.2f) raw=0x${results(4).number.raw.toLong.toHexString}"
      )
      println(
        f"  vcy=(${results(5).number.toDouble}%7.2f) raw=0x${results(5).number.raw.toLong.toHexString}"
      )
    }

    // Monitor float triangle path (after conversion)
    StreamMonitor(dut.ftriangleConverted, dut.clockDomain) { payload =>
      val tri = payload.tri
      val sign = payload.signBit.toBoolean
      println(f"[FloatTrianglePath] Triangle from float conversion (signBit=$sign):")
      println(
        f"  v0=(${tri(0)(0).toDouble}%7.2f, ${tri(0)(1).toDouble}%7.2f) raw=(0x${tri(0)(0).raw.toLong.toHexString}, 0x${tri(0)(1).raw.toLong.toHexString})"
      )
      println(
        f"  v1=(${tri(1)(0).toDouble}%7.2f, ${tri(1)(1).toDouble}%7.2f) raw=(0x${tri(1)(0).raw.toLong.toHexString}, 0x${tri(1)(1).raw.toLong.toHexString})"
      )
      println(
        f"  v2=(${tri(2)(0).toDouble}%7.2f, ${tri(2)(1).toDouble}%7.2f) raw=(0x${tri(2)(0).raw.toLong.toHexString}, 0x${tri(2)(1).raw.toLong.toHexString})"
      )
    }

    // Monitor integer triangle path
    StreamMonitor(dut.triangleIntPath, dut.clockDomain) { payload =>
      val tri = payload.tri
      val sign = payload.signBit.toBoolean
      println(f"[IntegerTrianglePath] Triangle from registers (signBit=$sign):")
      println(
        f"  v0=(${tri(0)(0).toDouble}%7.2f, ${tri(0)(1).toDouble}%7.2f) raw=(0x${tri(0)(0).raw.toLong.toHexString}, 0x${tri(0)(1).raw.toLong.toHexString})"
      )
      println(
        f"  v1=(${tri(1)(0).toDouble}%7.2f, ${tri(1)(1).toDouble}%7.2f) raw=(0x${tri(1)(0).raw.toLong.toHexString}, 0x${tri(1)(1).raw.toLong.toHexString})"
      )
      println(
        f"  v2=(${tri(2)(0).toDouble}%7.2f, ${tri(2)(1).toDouble}%7.2f) raw=(0x${tri(2)(0).raw.toLong.toHexString}, 0x${tri(2)(1).raw.toLong.toHexString})"
      )
    }

    // Monitor triangles passing through pipeline (after arbiter)
    StreamMonitor(dut.triangleSetup.i, dut.clockDomain) { payload =>
      val tri = payload.tri
      val sign = payload.signBit.toBoolean

      // Generate random RGB565 color for this triangle
      val r5 = scala.util.Random.nextInt(32) // 5 bits (0-31)
      val g6 = scala.util.Random.nextInt(64) // 6 bits (0-63)
      val b5 = scala.util.Random.nextInt(32) // 5 bits (0-31)
      currentTriangleColor = (r5 << 11) | (g6 << 5) | b5

      println(f"[TriangleSetup.Input] Triangle (after arbiter, signBit=$sign):")
      println(
        f"  v0=(${tri(0)(0).toDouble}%7.2f, ${tri(0)(1).toDouble}%7.2f) raw=(0x${tri(0)(0).raw.toLong.toHexString}, 0x${tri(0)(1).raw.toLong.toHexString})"
      )
      println(
        f"  v1=(${tri(1)(0).toDouble}%7.2f, ${tri(1)(1).toDouble}%7.2f) raw=(0x${tri(1)(0).raw.toLong.toHexString}, 0x${tri(1)(1).raw.toLong.toHexString})"
      )
      println(
        f"  v2=(${tri(2)(0).toDouble}%7.2f, ${tri(2)(1).toDouble}%7.2f) raw=(0x${tri(2)(0).raw.toLong.toHexString}, 0x${tri(2)(1).raw.toLong.toHexString})"
      )
      println(f"  Random color: RGB565=0x${currentTriangleColor}%04x (R=$r5 G=$g6 B=$b5)")
    }

    // Monitor rasterizer output (pixels)
    // StreamMonitor(dut.rasterizer.o, dut.clockDomain) { pixel =>
    //   val x = pixel.coords(0).toInt
    //   val y = pixel.coords(1).toInt
    //   val r = pixel.grads.redGrad.toDouble
    //   val g = pixel.grads.greenGrad.toDouble
    //   val b = pixel.grads.blueGrad.toDouble
    //   val z = pixel.grads.depthGrad.toDouble
    //   println(
    //     f"[Rasterizer.Output] Pixel at ($x%3d, $y%3d): R=$r%6.2f G=$g%6.2f B=$b%6.2f Z=$z%8.2f"
    //   )
    // }

    dut.clockDomain.waitSampling()
  }
}
