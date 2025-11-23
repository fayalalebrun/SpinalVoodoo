package voodoo.utils

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import spinal.lib.bus.regif._
import spinal.lib.bus.misc.SizeMapping
import scala.collection.mutable

/** BMB Bus Interface adapter for the RegIf system
  *
  * Adapts the BMB (Bus Memory Bus) protocol to the RegIf BusIf trait, enabling use of RegIf's
  * register definition system with BMB buses.
  *
  * Extends base functionality with Voodoo-specific register categorization (FIFO/Sync metadata).
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

  // Register category tracking: maps address -> category
  private val registerCategories = mutable.Map[BigInt, RegisterCategory]()

  // FIFO transaction payload for queued writes
  private case class QueuedWrite() extends Bundle {
    val address = UInt(busAddrWidth bits)
    val data = Bits(busDataWidth bits)
    val mask = Bits(busDataWidth / 8 bits)
    val syncRequired = Bool()
  }

  // Track command register streams for automatic drain blocking
  private val commandStreams = mutable.Map[BigInt, Stream[NoData]]()

  // Pipeline busy signal (set by user via setPipelineBusy)
  // Use default assignment (can be overridden by :=)
  private val pipelineBusySignal: Bool = Bool()
  pipelineBusySignal.default(False)

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

  // doRead: Read transaction is completing (cmd fires with READ)
  val doRead = (bus.cmd.fire && bus.cmd.opcode === Bmb.Cmd.Opcode.READ).allowPruning()

  // ==== PCI FIFO Implementation ====
  // Original doWrite split into direct (FIFO=No) vs queued (FIFO=Yes) paths

  private val doWriteImmediate =
    (bus.cmd.fire && bus.cmd.opcode === Bmb.Cmd.Opcode.WRITE).allowPruning()

  // Create signals that will be assigned in addPrePopTask
  private val shouldQueue = Bool()
  val fifoBypass = Bool().setName("busif_fifoBypass")
  val syncRequired = Bool().setName("busif_syncRequired")

  // Queued write transaction for FIFO
  private val queuedWriteTx = Stream(QueuedWrite())
  queuedWriteTx.valid := shouldQueue
  queuedWriteTx.address := bus.cmd.address
  queuedWriteTx.data := bus.cmd.data
  queuedWriteTx.mask := bus.cmd.mask
  queuedWriteTx.syncRequired := syncRequired

  // Build address decode logic after all registers are created
  Component.current.addPrePopTask(() => {
    // Debug: print registered categories
    println(s"[BmbBusInterface] Registered categories: ${registerCategories.size} entries")
    registerCategories.foreach { case (addr, cat) =>
      println(f"  0x$addr%03x: fifoBypass=${cat.fifoBypass}, syncRequired=${cat.syncRequired}")
    }

    // Debug: print bypass cases
    val bypassCases = registerCategories.collect {
      case (addr, cat) if cat.fifoBypass => addr
    }.toSeq
    val syncCases = registerCategories.collect {
      case (addr, cat) if cat.syncRequired => addr
    }.toSeq
    println(
      s"[BmbBusInterface] Bypass addresses: ${bypassCases.map(a => f"0x$a%03x").mkString(", ")}"
    )
    println(s"[BmbBusInterface] Sync addresses: ${syncCases.map(a => f"0x$a%03x").mkString(", ")}")

    // Generate routing logic for IMMEDIATE writes (decode bus.cmd.address)
    // IMPORTANT: Must decode bus.cmd.address, not effectiveWriteAddress!
    // Use switch/default pattern for cleaner synthesis
    fifoBypass := False // Default: most registers go through FIFO
    switch(bus.cmd.address) {
      for (addr <- bypassCases) {
        is(U(addr, busAddrWidth bits)) {
          fifoBypass := True
        }
      }
    }

    syncRequired := False // Default: most registers are Sync=No
    switch(bus.cmd.address) {
      for (addr <- syncCases) {
        is(U(addr, busAddrWidth bits)) {
          syncRequired := True
        }
      }
    }

    shouldQueue := doWriteImmediate && !fifoBypass
  })

  // Direct writes (FIFO=No) - register updates immediately
  private val doWriteDirect = doWriteImmediate && fifoBypass

  // 64-entry PCI FIFO for queued writes
  private val pciFifo = StreamFifo(QueuedWrite(), depth = 64)
  pciFifo.io.push << queuedWriteTx

  // Expose FIFO empty status for simulation debugging
  // Accessible in tests via dut.busif.fifoEmpty
  val fifoEmpty = Reg(Bool()) init (True) setName ("busif_fifoEmpty")
  fifoEmpty.simPublic()
  fifoEmpty := !pciFifo.io.pop.valid

  // Expose FIFO availability (free slots) for status register
  // Full 7-bit value (0-64) - saturation done in RegisterBank
  val pciFifoFree = UInt(7 bits)
  pciFifoFree := pciFifo.io.availability

  // Drain blocking logic
  private val canDrain = Bool()
  canDrain := True

  // Register pipelineBusy to break combinatorial loop
  // (pipelineBusy depends on triangle valid, which depends on drain, which depends on pipelineBusy)
  val pipelineBusyReg = RegNext(pipelineBusySignal) init (False)

  // Block drain if Sync=Yes register and pipeline is busy
  when(pciFifo.io.pop.syncRequired && pipelineBusyReg) {
    canDrain := False
  }

  // Note: Command stream backpressure is handled by the command register's pending logic,
  // not by blocking drain. Each command register holds valid high until the stream fires.

  private val drainedWrite = pciFifo.io.pop.haltWhen(!canDrain)

  // Replayed write from FIFO - we always consume immediately when valid
  drainedWrite.ready := True
  private val doWriteReplayed = drainedWrite.fire

  // Final doWrite signal = direct writes OR replayed writes from FIFO
  val doWrite = doWriteDirect || doWriteReplayed

  // Effective write signals (handle both direct and replayed writes)
  private val effectiveWriteAddress = doWriteReplayed ? drainedWrite.address | bus.cmd.address
  private val effectiveWriteData = doWriteReplayed ? drainedWrite.data | bus.cmd.data
  private val effectiveWriteMask = doWriteReplayed ? drainedWrite.mask | bus.cmd.mask

  // Write data from BMB (override to handle replayed writes)
  val writeData = effectiveWriteData

  // Write byte strobes from BMB mask field (override to handle replayed writes)
  wstrb := effectiveWriteMask

  // Connect read data to response
  bus.rsp.data := bus_rdata

  // Response is valid when we have a read/write completion
  // For writes: respond when write is accepted (either direct write or enqueued in FIFO)
  // For reads: respond with reg_rdata on doRead
  bus.rsp.valid := RegNext(doWriteImmediate || doRead) init (False)
  bus.rsp.last := True
  bus.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  bus.rsp.source := RegNext(bus.cmd.source)
  if (bus.p.access.contextWidth > 0) bus.rsp.context := RegNext(bus.cmd.context)

  // Set ready when we can accept transactions
  // Stall bus if FIFO is full (for FIFO=Yes writes only)
  bus.cmd.ready := True
  when(
    bus.cmd.valid && bus.cmd.opcode === Bmb.Cmd.Opcode.WRITE &&
      !fifoBypass && !queuedWriteTx.ready
  ) {
    bus.cmd.ready := False
  }

  // Clock gating enable - active when transaction is pending or completing
  override lazy val cg_en: Bool = bus.cmd.valid || RegNext(bus.cmd.valid) init (False)

  // Bus-level signals
  override lazy val bus_nsbit: Bool = False // No non-secure bit in BMB

  def readAddress(): UInt = bus.cmd.address
  def writeAddress(): UInt = effectiveWriteAddress // Use effective address for replayed writes

  def readHalt(): Unit = bus.cmd.ready := False
  def writeHalt(): Unit = bus.cmd.ready := False

  /** Check if current write operation requires sync (Sync=Yes register)
    *
    * For direct writes: uses decoded syncRequired signal For replayed writes: uses syncRequired
    * from FIFO payload
    *
    * @return
    *   True if current write requires pipeline sync
    */
  def isWriteSyncRequired(): Bool = {
    val replayedSync = doWriteReplayed && drainedWrite.syncRequired
    val directSync = doWriteDirect && syncRequired
    replayedSync || directSync
  }

  /** Create a register with category metadata
    *
    * Extension method that allows specifying register category (FIFO/Sync behavior) when creating
    * registers.
    *
    * @param addr
    *   Register address
    * @param name
    *   Register name
    * @param category
    *   Register category (FIFO routing and sync requirements)
    * @param sec
    *   Security settings
    * @return
    *   RegInst for field definitions
    */
  def newRegAtWithCategory(
      addr: BigInt,
      name: String,
      category: RegisterCategory,
      sec: Secure = null
  ): RegInst = {
    // Store category metadata
    registerCategories(addr) = category

    // Create register normally
    newRegAt(addr, name, sec)
  }

  /** Set the pipeline busy signal for Sync=Yes register drain blocking
    *
    * @param busy
    *   Signal indicating pipeline is busy (blocks Sync=Yes register writes from draining)
    */
  def setPipelineBusy(busy: Bool): Unit = {
    pipelineBusySignal := busy
  }

  /** Create a command register that produces a Stream
    *
    * Automatically handles FIFO queueing and drain blocking based on stream backpressure. The
    * stream pulses when the register is written. Returns RegInst so additional fields can be added.
    *
    * @param addr
    *   Register address
    * @param name
    *   Register name
    * @param category
    *   Register category (FIFO routing and sync requirements)
    * @param sec
    *   Security settings
    * @return
    *   Tuple of (RegInst, Stream[NoData]) - register instance and stream that pulses on write
    */
  def newCommandReg(
      addr: BigInt,
      name: String,
      category: RegisterCategory,
      sec: Secure = null
  ): (RegInst, Stream[NoData]) = {
    // Store category metadata
    registerCategories(addr) = category

    // Create register - caller will define fields
    val reg = newRegAt(addr, name, sec)

    // Use hitDoWrite signal from RegInst to detect writes
    val writeDetect = reg.hitDoWrite

    // Create output stream
    val stream = Stream(NoData())

    // Hold valid until stream fires (backpressure handling)
    val pending = Reg(Bool()) init (False)
    when(writeDetect && !stream.ready) {
      pending := True
    }.elsewhen(stream.fire) {
      pending := False
    }

    stream.valid := writeDetect || pending

    // Register this stream for automatic FIFO drain blocking
    commandStreams(addr) = stream

    (reg, stream)
  }

  /** Get current register write metadata signals
    *
    * Returns Bundle with current write categorization. Call this to get hardware signals that
    * indicate FIFO bypass and sync requirements for the current write transaction.
    *
    * @param address
    *   Address to decode (defaults to bus.cmd.address for immediate writes)
    * @return
    *   Bundle with fifoBypass and syncRequired signals
    */
  def getRegWriteMetadata(address: UInt = bus.cmd.address): RegisterWriteMetadata = {
    val metadata = new RegisterWriteMetadata

    // Generate address decode logic for each registered category
    val fifoBypassCases = registerCategories.collect {
      case (addr, cat) if cat.fifoBypass => addr
    }.toSeq

    val syncRequiredCases = registerCategories.collect {
      case (addr, cat) if cat.syncRequired => addr
    }.toSeq

    // Hardware logic: decode provided address to determine category
    // Create Vec of Bool comparisons, then reduce with OR
    metadata.fifoBypass := {
      if (fifoBypassCases.isEmpty) False
      else fifoBypassCases.map(addr => address === U(addr)).reduce(_ || _)
    }

    metadata.syncRequired := {
      if (syncRequiredCases.isEmpty) False
      else syncRequiredCases.map(addr => address === U(addr)).reduce(_ || _)
    }

    metadata
  }
}

/** Register write metadata bundle
  *
  * Hardware signals indicating register category for current write transaction.
  */
class RegisterWriteMetadata extends Bundle {
  val fifoBypass = Bool() // True = FIFO=No (bypass), False = FIFO=Yes (queued)
  val syncRequired = Bool() // True = Sync=Yes (flush required), False = Sync=No
}

object BmbBusInterface {

  /** Factory method matching RegIf BusInterface pattern */
  def apply(bus: Bmb, sizeMap: SizeMapping, regPre: String)(implicit
      moduleName: ClassName
  ): BmbBusInterface = {
    new BmbBusInterface(bus, sizeMap, regPre, false)
  }
}
