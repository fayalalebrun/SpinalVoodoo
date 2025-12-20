package voodoo.trace

import spinal.core._
import spinal.core.sim._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim.BmbMemoryAgent
import spinal.lib.sim.{SparseMemory, StreamDriver, StreamMonitor, StreamReadyRandomizer}

/** Framebuffer memory model using BmbMemoryAgent
  *
  * Wraps BmbMemoryAgent to provide real-time display updates as framebuffer writes occur.
  *
  * @param size
  *   Framebuffer size in bytes
  * @param display
  *   Optional display window for real-time rendering
  */
class FramebufferModel(size: Long, display: Option[DisplayWindow] = None) {

  // Use BmbMemoryAgent for memory simulation
  private val memoryAgent = new BmbMemoryAgent(size)

  /** Add a BMB port to the framebuffer memory
    *
    * @param bus
    *   BMB bus to connect
    * @param busAddress
    *   Base address offset
    * @param clockDomain
    *   Clock domain for the bus
    */
  def addPort(bus: Bmb, busAddress: Long, clockDomain: ClockDomain): Unit = {
    memoryAgent.addPort(
      bus = bus,
      busAddress = busAddress,
      clockDomain = clockDomain,
      withDriver = true
    )

    // Override write notification to update display
    display.foreach { disp =>
      val originalAgent = memoryAgent

      // Monitor write transactions and update display
      StreamMonitor(bus.cmd, clockDomain) { payload =>
        if (payload.opcode.toInt == Bmb.Cmd.Opcode.WRITE) {
          val addr = payload.address.toLong - busAddress
          val data = payload.data.toInt
          val mask = payload.mask.toInt

          // Update display for each byte written
          for (i <- 0 until 4) {
            if ((mask & (1 << i)) != 0) {
              val byteAddr = addr + i
              val byteValue = ((data >> (i * 8)) & 0xff).toByte
              disp.updatePixel(byteAddr, byteValue)
            }
          }
        }
      }
    }
  }

  /** Read a byte from framebuffer memory
    *
    * @param address
    *   Byte address
    * @return
    *   Byte value
    */
  def getByte(address: Long): Byte = {
    memoryAgent.getByte(address)
  }

  /** Write a byte to framebuffer memory
    *
    * @param address
    *   Byte address
    * @param value
    *   Byte value
    */
  def setByte(address: Long, value: Byte): Unit = {
    memoryAgent.setByte(address, value)
    display.foreach(_.updatePixel(address, value))
  }

  /** Load framebuffer data from a byte array (raw, no conversion)
    *
    * @param address
    *   Starting byte address
    * @param data
    *   Byte array to load
    */
  def loadData(address: Long, data: Array[Byte]): Unit = {
    for (i <- data.indices) {
      memoryAgent.setByte(address + i, data(i))
    }
    // Update display with all loaded pixels
    display.foreach { disp =>
      // RGB565 format - 2 bytes per pixel
      for (i <- 0 until (data.length / 2)) {
        val byteAddr = address + i * 2
        disp.updatePixel(byteAddr, data(i * 2))
        disp.updatePixel(byteAddr + 1, data(i * 2 + 1))
      }
    }
  }

  /** Load framebuffer from 86Box state dump format with conversion
    *
    * Converts from 86Box format (2 bytes/pixel, configurable row width) to our format (4
    * bytes/pixel, fixed stride).
    *
    * 86Box format:
    *   - Color buffer at drawOffset: 2 bytes per pixel (RGB565)
    *   - Depth buffer at auxOffset: 2 bytes per pixel (16-bit depth)
    *   - Row stride: rowWidth bytes
    *
    * Our format:
    *   - 4 bytes per pixel: low 16 bits = RGB565, high 16 bits = depth
    *   - Row stride: targetStride * 4 bytes
    *
    * @param fbMem
    *   Raw framebuffer memory from 86Box
    * @param drawOffset
    *   Color buffer offset in fbMem
    * @param auxOffset
    *   Depth buffer offset in fbMem
    * @param rowWidth
    *   Row stride in bytes (86Box format)
    * @param hDisp
    *   Horizontal display resolution
    * @param vDisp
    *   Vertical display resolution
    * @param targetStride
    *   Our target stride in pixels (e.g., 1024)
    */
  def loadFrom86BoxFormat(
      fbMem: Array[Byte],
      drawOffset: Long,
      auxOffset: Long,
      rowWidth: Long,
      hDisp: Int,
      vDisp: Int,
      targetStride: Int
  ): Unit = {
    println(
      s"[DEBUG] Converting 86Box FB: ${hDisp}x${vDisp}, rowWidth=$rowWidth, " +
        s"drawOffset=$drawOffset, auxOffset=$auxOffset, targetStride=$targetStride"
    )

    for (y <- 0 until vDisp) {
      for (x <- 0 until hDisp) {
        // Read color from 86Box format (2 bytes, little-endian)
        val colorAddr = (drawOffset + y * rowWidth + x * 2).toInt
        if (colorAddr + 1 < fbMem.length) {
          val colorLo = fbMem(colorAddr) & 0xff
          val colorHi = fbMem(colorAddr + 1) & 0xff
          val rgb565 = colorLo | (colorHi << 8)

          // Read depth from 86Box format (2 bytes, little-endian)
          val depthAddr = (auxOffset + y * rowWidth + x * 2).toInt
          val depth = if (depthAddr + 1 < fbMem.length) {
            val depthLo = fbMem(depthAddr) & 0xff
            val depthHi = fbMem(depthAddr + 1) & 0xff
            depthLo | (depthHi << 8)
          } else 0

          // Write to our format: (y * stride + x) * 4
          val ourAddr = (y * targetStride + x) * 4L

          // Write as 4 bytes: color in low 16 bits, depth in high 16 bits
          memoryAgent.setByte(ourAddr, (rgb565 & 0xff).toByte)
          memoryAgent.setByte(ourAddr + 1, ((rgb565 >> 8) & 0xff).toByte)
          memoryAgent.setByte(ourAddr + 2, (depth & 0xff).toByte)
          memoryAgent.setByte(ourAddr + 3, ((depth >> 8) & 0xff).toByte)

          // Update display
          display.foreach { disp =>
            disp.updatePixel(ourAddr, (rgb565 & 0xff).toByte)
            disp.updatePixel(ourAddr + 1, ((rgb565 >> 8) & 0xff).toByte)
          }
        }
      }
    }
    println(s"[DEBUG] Converted ${hDisp * vDisp} pixels from 86Box format")
  }

  /** Get the underlying sparse memory */
  def memory: SparseMemory = memoryAgent.memory
}

object FramebufferModel {
  def apply(size: Long, display: Option[DisplayWindow] = None): FramebufferModel = {
    new FramebufferModel(size, display)
  }
}
