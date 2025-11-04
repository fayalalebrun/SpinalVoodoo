package voodoo.utils

import spinal.core._

/** Register category metadata for Voodoo Graphics registers
  *
  * Tracks two orthogonal categorizations from the Voodoo hardware spec:
  *
  *   1. FIFO routing (fifoBypass):
  *      - false (FIFO=Yes): Register writes go through 64-entry PCI FIFO, processed asynchronously
  *      - true (FIFO=No): Register writes bypass FIFO, cross clock domains directly via
  *        synchronizers
  *   2. Pipeline synchronization (syncRequired):
  *      - false (Sync=No): Changes take effect for next rendering command, no pipeline flush
  *      - true (Sync=Yes): Hardware automatically flushes pipeline before applying write
  *
  * @param fifoBypass
  *   True if register bypasses PCI FIFO (FIFO=No), false if queued in FIFO (FIFO=Yes)
  * @param syncRequired
  *   True if hardware must flush pipeline before applying write (Sync=Yes)
  */
case class RegisterCategory(
    fifoBypass: Boolean = false,
    syncRequired: Boolean = false
)

object RegisterCategory {
  // Common register categories from Voodoo spec

  /** FIFO=Yes, Sync=No: Triangle geometry parameters
    *
    * Most common category. Queued in PCI FIFO, changes take effect for next triangle. Examples:
    * vertexAx, vertexAy, startR, dRdX, etc.
    */
  def fifoNoSync = RegisterCategory(fifoBypass = false, syncRequired = false)

  /** FIFO=Yes, Sync=Yes: Rendering mode changes requiring pipeline flush
    *
    * Hardware automatically flushes pipeline. Examples: fbzMode, clipLeftRight, fogTable, nopCMD
    */
  def fifoWithSync = RegisterCategory(fifoBypass = false, syncRequired = true)

  /** FIFO=No, Sync=N/A: Hardware initialization and video timing
    *
    * Bypass FIFO entirely, write directly to hardware. Examples: fbiInit0-7, backPorch, hSync,
    * vSync
    */
  def bypassFifo = RegisterCategory(fifoBypass = true, syncRequired = false)

  /** Read-only registers (status, statistics)
    *
    * No write behavior, but categorized as FIFO for consistency
    */
  def readOnly = RegisterCategory(fifoBypass = false, syncRequired = false)
}
