package voodoo.utils

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.regif._
import spinal.lib.bus.misc.SizeMapping

/** BMB Bus Interface adapter for the RegIf system
  *
  * Adapts the BMB (Bus Memory Bus) protocol to the RegIf BusIf trait, enabling use of RegIf's
  * register definition system with BMB buses.
  *
  * Based on Apb3BusInterface implementation pattern.
  *
  * @param bus
  *   BMB bus to adapt
  * @param sizeMap
  *   Address range mapping
  * @param regPre
  *   Register name prefix
  * @param withSecFireWall
  *   Security firewall enable
  * @param moduleName
  *   Implicit module name for documentation
  */
case class BmbBusInterface(
    bus: Bmb,
    sizeMap: SizeMapping,
    regPre: String = "",
    withSecFireWall: Boolean = false
)(implicit moduleName: ClassName)
    extends BusIf {

  val busDataWidth: Int = bus.p.access.dataWidth
  val busAddrWidth: Int = bus.p.access.addressWidth

  // Error and data registers
  lazy val reg_wrerr: Bool = Reg(Bool()) init (False)
  val bus_rdata: Bits = Bits(busDataWidth bits)
  val reg_rderr: Bool = Reg(Bool()) init (False)
  val reg_rdata: Bits = Reg(Bits(busDataWidth bits)) init (defaultReadBits)

  // BMB supports byte strobes via mask field
  override val withStrb: Boolean = true
  val wstrb: Bits = Bits(strbWidth bits)
  val wmask: Bits = Bits(busDataWidth bits)
  val wmaskn: Bits = Bits(busDataWidth bits)
  initStrbMasks()

  override def getModuleName = moduleName.name

  // BMB transaction signals
  // askWrite: Write request is pending (cmd.valid and WRITE opcode)
  val askWrite = (bus.cmd.valid && bus.cmd.opcode === Bmb.Cmd.Opcode.WRITE).allowPruning()

  // askRead: Read request is pending (cmd.valid and READ opcode)
  val askRead = (bus.cmd.valid && bus.cmd.opcode === Bmb.Cmd.Opcode.READ).allowPruning()

  // doWrite: Write transaction is completing (cmd fires with WRITE)
  val doWrite = (bus.cmd.fire && bus.cmd.opcode === Bmb.Cmd.Opcode.WRITE).allowPruning()

  // doRead: Read transaction is completing (cmd fires with READ)
  val doRead = (bus.cmd.fire && bus.cmd.opcode === Bmb.Cmd.Opcode.READ).allowPruning()

  // Write data from BMB
  val writeData = bus.cmd.data

  // Write byte strobes from BMB mask field
  wstrb := bus.cmd.mask

  // Connect read data to response
  bus.rsp.data := bus_rdata

  // Response is valid when we have a read/write completion
  // For writes: respond immediately on doWrite
  // For reads: respond with reg_rdata on doRead
  bus.rsp.valid := RegNext(doWrite || doRead) init (False)
  bus.rsp.last := True
  bus.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  bus.rsp.source := RegNext(bus.cmd.source)
  if (bus.p.access.contextWidth > 0) bus.rsp.context := RegNext(bus.cmd.context)

  // Set ready when we can accept transactions
  bus.cmd.ready := True

  // Clock gating enable - active when transaction is pending or completing
  override lazy val cg_en: Bool = bus.cmd.valid || RegNext(bus.cmd.valid) init (False)

  // Bus-level signals
  override lazy val bus_nsbit: Bool = False // No non-secure bit in BMB

  def readAddress(): UInt = bus.cmd.address
  def writeAddress(): UInt = bus.cmd.address

  def readHalt(): Unit = bus.cmd.ready := False
  def writeHalt(): Unit = bus.cmd.ready := False
}

object BmbBusInterface {

  /** Factory method matching RegIf BusInterface pattern */
  def apply(bus: Bmb, sizeMap: SizeMapping, regPre: String)(implicit
      moduleName: ClassName
  ): BmbBusInterface = {
    new BmbBusInterface(bus, sizeMap, regPre, false)
  }
}
