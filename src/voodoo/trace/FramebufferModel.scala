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

  /** Get the underlying sparse memory */
  def memory: SparseMemory = memoryAgent.memory
}

object FramebufferModel {
  def apply(size: Long, display: Option[DisplayWindow] = None): FramebufferModel = {
    new FramebufferModel(size, display)
  }
}
