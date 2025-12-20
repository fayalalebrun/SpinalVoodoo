package voodoo.trace

import spinal.core._
import spinal.core.sim._
import spinal.lib.bus.bmb._
import spinal.lib.bus.bmb.sim.BmbMemoryAgent
import spinal.lib.sim.SparseMemory

/** Texture memory model for simulation
  *
  * Wraps BmbMemoryAgent to provide read/write access for texture data. Supports multiple read ports
  * (one per TMU) and write access for texture uploads.
  *
  * @param size
  *   Texture memory size in bytes
  */
class TextureMemoryModel(size: Long) {

  // Use BmbMemoryAgent for memory simulation
  private val memoryAgent = new BmbMemoryAgent(size)

  /** Add a BMB read port for texture fetches (used by TMUs)
    *
    * @param bus
    *   BMB bus to connect
    * @param busAddress
    *   Base address offset
    * @param clockDomain
    *   Clock domain for the bus
    */
  def addReadPort(bus: Bmb, busAddress: Long, clockDomain: ClockDomain): Unit = {
    memoryAgent.addPort(
      bus = bus,
      busAddress = busAddress,
      clockDomain = clockDomain,
      withDriver = true
    )
  }

  /** Read a byte from texture memory
    *
    * @param address
    *   Byte address
    * @return
    *   Byte value
    */
  def getByte(address: Long): Byte = {
    memoryAgent.getByte(address)
  }

  /** Write a byte to texture memory
    *
    * @param address
    *   Byte address
    * @param value
    *   Byte value
    */
  def setByte(address: Long, value: Byte): Unit = {
    memoryAgent.setByte(address, value)
  }

  /** Write a 16-bit value to texture memory (little-endian)
    *
    * @param address
    *   Byte address (must be 2-byte aligned for correct behavior)
    * @param value
    *   16-bit value
    */
  def setShort(address: Long, value: Int): Unit = {
    memoryAgent.setByte(address, (value & 0xff).toByte)
    memoryAgent.setByte(address + 1, ((value >> 8) & 0xff).toByte)
  }

  /** Write a 32-bit value to texture memory (little-endian)
    *
    * @param address
    *   Byte address
    * @param value
    *   32-bit value
    */
  def setInt(address: Long, value: Int): Unit = {
    memoryAgent.setByte(address, (value & 0xff).toByte)
    memoryAgent.setByte(address + 1, ((value >> 8) & 0xff).toByte)
    memoryAgent.setByte(address + 2, ((value >> 16) & 0xff).toByte)
    memoryAgent.setByte(address + 3, ((value >> 24) & 0xff).toByte)
  }

  /** Load texture data from a byte array
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
  }

  /** Get the underlying sparse memory */
  def memory: SparseMemory = memoryAgent.memory
}

object TextureMemoryModel {
  def apply(size: Long): TextureMemoryModel = {
    new TextureMemoryModel(size)
  }
}
