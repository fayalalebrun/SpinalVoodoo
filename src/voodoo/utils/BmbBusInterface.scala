package voodoo.utils

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.regif._
import spinal.lib.bus.misc.SizeMapping
import scala.collection.mutable

/** BMB Bus Interface adapter for the RegIf system (pure RegIf adapter)
  *
  * Adapts the BMB (Bus Memory Bus) protocol to the RegIf BusIf trait, enabling use of RegIf's
  * register definition system with BMB buses.
  *
  * All FIFO/drain/float-conversion logic has been extracted to PciFifo. This component now handles
  * only direct register reads and writes — all writes arriving here are immediate (PciFifo has
  * already handled ordering).
  *
  * Extends base functionality with Voodoo-specific register categorization metadata exports.
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

  case class VoodooRegInst(underlying: RegInst) {
    def field[T <: BaseType](that: T, accessType: AccessType, section: BigInt, doc: String)(implicit
        symbol: SymbolName
    ): T = underlying.field(cloneOf(that), accessType, section, doc)

    def field(that: AFix, accessType: AccessType, section: BigInt, doc: String)(implicit
        symbol: SymbolName
    ): AFix = {
      val proto = cloneOf(that)
      val base =
        if (proto.signed) underlying.field(SInt(proto.bitWidth bits), accessType, section, doc)
        else underlying.field(UInt(proto.bitWidth bits), accessType, section, doc)
      val afix = cloneOf(proto)
      afix.raw := base.asBits
      afix
    }

    def fieldAt(bitOffset: Int, that: AFix, accessType: AccessType, section: BigInt, doc: String)(
        implicit symbol: SymbolName
    ): AFix = {
      val proto = cloneOf(that)
      val base =
        if (proto.signed)
          underlying.fieldAt(bitOffset, SInt(proto.bitWidth bits), accessType, section, doc)
        else underlying.fieldAt(bitOffset, UInt(proto.bitWidth bits), accessType, section, doc)
      val afix = cloneOf(proto)
      afix.raw := base.asBits
      afix
    }

    def fieldAt[T <: BaseType](
        bitOffset: Int,
        that: T,
        accessType: AccessType,
        section: BigInt,
        doc: String
    )(implicit
        symbol: SymbolName
    ): T = underlying.fieldAt(bitOffset, cloneOf(that), accessType, section, doc)
  }

  // Register category tracking: maps address -> category
  private val registerCategories = mutable.Map[BigInt, RegisterCategory]()

  // Track command register stream ready signals for automatic drain blocking
  // Maps address -> pulseStream.ready (from s2mPipe, safe because output valid is registered)
  private val commandStreamReady = mutable.Map[BigInt, Bool]()

  // Float alias support: maps float address -> config (target addr, format)
  // Metadata only — hardware generation is in PciFifo
  private val floatAliases = mutable.Map[BigInt, PciFifo.FloatAliasConfig]()
  private val FLOAT_ALIAS_OFFSET: BigInt = 0x080
  private var lastCreatedRegAddr: BigInt = -1

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

  // BMB transaction signals — all writes are now immediate (PciFifo has handled ordering)
  val askWrite = (bus.cmd.valid && bus.cmd.opcode === Bmb.Cmd.Opcode.WRITE).allowPruning()
  val askRead = (bus.cmd.valid && bus.cmd.opcode === Bmb.Cmd.Opcode.READ).allowPruning()
  val doWrite = (bus.cmd.fire && bus.cmd.opcode === Bmb.Cmd.Opcode.WRITE).allowPruning()
  val doRead = (bus.cmd.fire && bus.cmd.opcode === Bmb.Cmd.Opcode.READ).allowPruning()

  // Write data directly from BMB
  val writeData = bus.cmd.data

  // Write byte strobes from BMB mask field
  wstrb := bus.cmd.mask

  // Connect read data to response
  bus.rsp.data := bus_rdata

  // Simple response: one cycle after any command
  bus.rsp.valid := RegNext(bus.cmd.fire) init (False)
  bus.rsp.last := True
  bus.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  bus.rsp.source := RegNext(bus.cmd.source)
  if (bus.p.access.contextWidth > 0) bus.rsp.context := RegNext(bus.cmd.context)

  // Always ready — PciFifo handles stall logic
  bus.cmd.ready := True

  // Clock gating enable - active when transaction is pending or completing
  override lazy val cg_en: Bool = bus.cmd.valid || RegNext(bus.cmd.valid) init (False)

  // Bus-level signals
  override lazy val bus_nsbit: Bool = False // No non-secure bit in BMB

  def readAddress(): UInt = bus.cmd.address
  def writeAddress(): UInt = bus.cmd.address

  def readHalt(): Unit = bus.cmd.ready := False
  def writeHalt(): Unit = bus.cmd.ready := False

  /** Create a register with category metadata */
  def newRegAtWithCategory(
      addr: BigInt,
      name: String,
      category: RegisterCategory,
      sec: Secure = null
  ): VoodooRegInst = {
    registerCategories(addr) = category
    lastCreatedRegAddr = addr
    VoodooRegInst(newRegAt(addr, name, sec))
  }

  /** Create a command register that produces a Stream
    *
    * The stream pulses when the register is written. Returns RegInst so additional fields can be
    * added.
    */
  def newCommandReg(
      addr: BigInt,
      name: String,
      category: RegisterCategory,
      sec: Secure = null
  ): (VoodooRegInst, Stream[NoData]) = {
    registerCategories(addr) = category
    val reg = newRegAt(addr, name, sec)

    val pulseStream = Stream(NoData())
    pulseStream.valid := reg.hitDoWrite

    // s2mPipe() breaks the ready-path combinatorial loop (its ready is registered).
    // m2sPipe() then registers the valid path, ensuring the stream fires ONE CYCLE
    // AFTER the register write.
    val s2m = pulseStream.s2mPipe()
    val bufferedStream = s2m.m2sPipe()

    // For blocking: check pulseStream.ready (from s2mPipe, registered — no comb loop)
    commandStreamReady(addr) = pulseStream.ready

    (VoodooRegInst(reg), bufferedStream)
  }

  /** Implicit enrichment for field-level .withFloatAlias()
    *
    * Registers a float alias address (integer addr + 0x080). Hardware generation is in PciFifo;
    * this only tracks metadata.
    */
  implicit class FieldFloatAlias[T <: Data](field: T) {
    def withFloatAlias(intBits: Int, fracBits: Int): T = {
      require(lastCreatedRegAddr >= 0, "withFloatAlias must be called after newRegAtWithCategory")
      val floatAddr = lastCreatedRegAddr + FLOAT_ALIAS_OFFSET
      floatAliases(floatAddr) = PciFifo.FloatAliasConfig(lastCreatedRegAddr, intBits, fracBits)
      registerCategories(floatAddr) = RegisterCategory.fifoNoSync
      field
    }

    def withFloatAlias(): T = field match {
      case afix: AFix => withFloatAlias(afix.intWidth, afix.fracWidth)
      case _          =>
        throw new IllegalArgumentException(
          s"withFloatAlias() without arguments requires an AFix field, got ${field.getClass.getSimpleName}"
        )
    }
  }

  // ========================================================================
  // Metadata exports for PciFifo
  // ========================================================================

  /** Get all registered categories (addr -> RegisterCategory) */
  def getCategories: Map[BigInt, RegisterCategory] = registerCategories.toMap

  /** Get all float alias configurations (float addr -> FloatAliasConfig) */
  def getFloatAliases: Map[BigInt, PciFifo.FloatAliasConfig] = floatAliases.toMap

  /** Get command stream ready signals (addr -> Bool) */
  def getCommandStreamReady: Map[BigInt, Bool] = commandStreamReady.toMap
}

/** Register write metadata bundle */
class RegisterWriteMetadata extends Bundle {
  val fifoBypass = Bool()
  val syncRequired = Bool()
}

object BmbBusInterface {

  /** Factory method matching RegIf BusInterface pattern */
  def apply(bus: Bmb, sizeMap: SizeMapping, regPre: String)(implicit
      moduleName: ClassName
  ): BmbBusInterface = {
    new BmbBusInterface(bus, sizeMap, regPre, false)
  }
}
