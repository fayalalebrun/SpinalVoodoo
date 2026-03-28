package voodoo.bus

import voodoo._
import spinal.core._
import spinal.core.formal._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo.math.{Fpxx, FpxxConfig, Fpxx2AFix}

/** Texture write payload for PciFifo input/output */
case class TexWritePayload() extends Bundle {
  val pciAddr = UInt(23 bits) // 23-bit PCI offset within texture space
  val data = Bits(32 bits)
  val mask = Bits(4 bits)
}

case class FloatShadowWrite(busAddrWidth: Int) extends Bundle {
  val address = UInt(busAddrWidth bits)
  val raw = SInt(50 bits)
}

/** PCI FIFO — sits on the BMB bus between AddressRemapper and RegisterBank.
  *
  * Handles write ordering, float conversion, FIFO queueing, drain blocking, and texture write
  * routing. All PCI writes (registers and texture data) flow through a single FIFO, matching real
  * Voodoo hardware.
  *
  * @param busParams
  *   BMB bus params (12-bit addr, 32-bit data)
  * @param categories
  *   addr -> RegisterCategory (fifoBypass/syncRequired)
  * @param floatAliases
  *   float addr -> (target addr, intBits, fracBits)
  * @param commandAddresses
  *   addresses with drain blocking
  */
case class PciFifo(
    busParams: BmbParameter,
    categories: Map[BigInt, RegisterCategory],
    floatAliases: Map[BigInt, PciFifo.FloatAliasConfig],
    commandAddresses: Seq[BigInt],
    formalStrong: Boolean = true
) extends Component {

  private case class AddressDecodeInfo() extends Bundle {
    val fifoBypass = Bool()
    val syncRequired = Bool()
    val isFloatAlias = Bool()
    val floatTargetAddr = UInt(busParams.access.addressWidth bits)
    val floatShiftAmount = UInt(5 bits)
  }

  val busAddrWidth: Int = busParams.access.addressWidth
  val busDataWidth: Int = busParams.access.dataWidth

  val io = new Bundle {
    // BMB bus — sits in the middle of the CPU -> RegisterBank path
    val cpuSide = slave(Bmb(busParams))
    val regSide = master(Bmb(busParams))

    // Texture write input (from Core, separate from register bus)
    val texWrite = slave Stream (TexWritePayload())

    // Texture write drain (packed translation happens in Core)
    val texDrain = master Stream (TexWritePayload())

    // Drain blocking inputs
    val pipelineBusy = in Bool ()
    val commandReady = in Vec (Bool(), commandAddresses.size)

    // Status outputs
    val fifoEmpty = out Bool ()
    val pciFifoFree = out UInt (7 bits)

    // Event signals
    val wasEnqueued = out Bool () // swapCmdEnqueued detection
    val wasEnqueuedAddr = in UInt (busAddrWidth bits) // which address to watch
    val syncDrained = out Bool () // Sync=Yes register was drained
    val floatShadow = master Flow (FloatShadowWrite(busAddrWidth))
  }

  // ========================================================================
  // FIFO transaction payload
  // ========================================================================
  private case class QueuedWrite() extends Bundle {
    val address = UInt(busAddrWidth bits)
    val data = Bits(busDataWidth bits)
    val mask = Bits(busDataWidth / 8 bits)
    val syncRequired = Bool()
    val isTexture = Bool()
    val texPciAddr = UInt(23 bits)
    val hasFloatShadow = Bool()
    val floatShadowRaw = SInt(50 bits)
  }

  // ========================================================================
  // Address categorization signals (assigned in addPrePopTask)
  // ========================================================================
  private val decodeInfo = AddressDecodeInfo()
  private val fifoBypass = decodeInfo.fifoBypass.setName("pciFifo_fifoBypass")
  private val syncRequired = decodeInfo.syncRequired.setName("pciFifo_syncRequired")

  // Float alias conversion signals
  private val isFloatAlias = decodeInfo.isFloatAlias
  private val floatTargetAddr = decodeInfo.floatTargetAddr
  private val floatShiftAmount = decodeInfo.floatShiftAmount

  // Detection signals: isWriteCmd uses cmd.valid (no ready dependency) for routing decisions.
  // doWriteImmediate uses cmd.fire for response/push (registered paths, no combinatorial loop).
  private val isWriteCmd = io.cpuSide.cmd.valid && io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.WRITE
  private val doWriteImmediate =
    (io.cpuSide.cmd.fire && io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.WRITE).allowPruning()

  private case class StagedWrite() extends Bundle {
    val address = UInt(busAddrWidth bits)
    val data = Bits(busDataWidth bits)
    val mask = Bits(busDataWidth / 8 bits)
    val syncRequired = Bool()
    val isTexture = Bool()
    val texPciAddr = UInt(23 bits)
    val isFloatAlias = Bool()
    val floatTargetAddr = UInt(busAddrWidth bits)
    val floatShiftAmount = UInt(5 bits)
  }

  private def clearStaged(write: Stream[StagedWrite]): Unit = {
    write.valid := False
    write.address := 0
    write.data := 0
    write.mask := 0
    write.syncRequired := False
    write.isTexture := False
    write.texPciAddr := 0
    write.isFloatAlias := False
    write.floatTargetAddr := 0
    write.floatShiftAmount := 0
  }

  private def clearQueued(write: Stream[QueuedWrite]): Unit = {
    write.valid := False
    write.address := 0
    write.data := 0
    write.mask := 0
    write.syncRequired := False
    write.isTexture := False
    write.texPciAddr := 0
    write.hasFloatShadow := False
    write.floatShadowRaw := 0
  }

  private def floatCorrected(rawSInt: SInt, shift: UInt): SInt = {
    val shifted = rawSInt >> shift
    val hasRemainder = Bool()
    hasRemainder := False
    switch(shift) {
      is(26) { hasRemainder := rawSInt(25 downto 0).orR }
      is(18) { hasRemainder := rawSInt(17 downto 0).orR }
      is(12) { hasRemainder := rawSInt(11 downto 0).orR }
    }
    Mux(rawSInt.msb && hasRemainder, shifted + 1, shifted)
  }

  private val queueableCpuWrite = isWriteCmd && !fifoBypass

  // Small front-end staging FIFO decouples incoming PCI writes from float conversion bubbles.
  private val ingress = new Area {
    val stageFifo = StreamFifo(StagedWrite(), depth = 16)
    val stagedWriteTx = Stream(StagedWrite())
    clearStaged(stagedWriteTx)

    when(queueableCpuWrite) {
      stagedWriteTx.valid := io.cpuSide.cmd.valid
      stagedWriteTx.address := io.cpuSide.cmd.address
      stagedWriteTx.data := io.cpuSide.cmd.data
      stagedWriteTx.mask := io.cpuSide.cmd.mask
      stagedWriteTx.syncRequired := syncRequired
      stagedWriteTx.isFloatAlias := isFloatAlias
      stagedWriteTx.floatTargetAddr := floatTargetAddr
      stagedWriteTx.floatShiftAmount := floatShiftAmount
    } elsewhen (io.texWrite.valid) {
      stagedWriteTx.valid := True
      stagedWriteTx.data := io.texWrite.data
      stagedWriteTx.mask := io.texWrite.mask
      stagedWriteTx.isTexture := True
      stagedWriteTx.texPciAddr := io.texWrite.pciAddr
    }

    stageFifo.io.push << stagedWriteTx
  }

  private val stageFifo = ingress.stageFifo
  private val stagedWriteTx = ingress.stagedWriteTx
  private val stagedWrite = stageFifo.io.pop

  // ========================================================================
  // Float conversion pipeline
  // ========================================================================
  private val floatConvert = new Area {
    val converter = new Fpxx2AFix(20 bits, 30 bits, FpxxConfig.float32(), pipeStages = 1)
    val inFlight = RegInit(False)
    val targetAddr = Reg(UInt(busAddrWidth bits))
    val shift = Reg(UInt(5 bits))

    val writeRequest = stagedWrite.valid && stagedWrite.isFloatAlias
    converter.io.op.valid := writeRequest && !inFlight
    converter.io.op.payload.assignFromBits(stagedWrite.data)

    when(converter.io.op.fire) {
      inFlight := True
      targetAddr := stagedWrite.floatTargetAddr
      shift := stagedWrite.floatShiftAmount
    }
  }
  private val floatConverter = floatConvert.converter
  private val floatConvertInFlight = floatConvert.inFlight
  private val floatConvertTargetAddr = floatConvert.targetAddr
  private val floatConvertShift = floatConvert.shift
  private val floatWriteRequest = floatConvert.writeRequest

  // ========================================================================
  // FIFO push mux
  // ========================================================================
  private val queuePush = new Area {
    val queuedWriteTx = Stream(QueuedWrite())
    clearQueued(queuedWriteTx)

    when(floatConvertInFlight && floatConverter.io.result.valid) {
      // Push converted float data with remapped integer address
      val rawSInt = floatConverter.io.result.number.raw.asSInt
      val corrected = floatCorrected(rawSInt, floatConvertShift)

      queuedWriteTx.valid := True
      queuedWriteTx.address := floatConvertTargetAddr
      queuedWriteTx.data := corrected.resize(32 bits).asBits
      queuedWriteTx.mask := B"1111"
      queuedWriteTx.hasFloatShadow := True
      queuedWriteTx.floatShadowRaw := rawSInt.resize(50 bits)
    } elsewhen (stagedWrite.valid && !stagedWrite.isFloatAlias) {
      queuedWriteTx.valid := True
      queuedWriteTx.address := stagedWrite.address
      queuedWriteTx.data := stagedWrite.data
      queuedWriteTx.mask := stagedWrite.mask
      queuedWriteTx.syncRequired := stagedWrite.syncRequired
      queuedWriteTx.isTexture := stagedWrite.isTexture
      queuedWriteTx.texPciAddr := stagedWrite.texPciAddr
    }
  }
  private val queuedWriteTx = queuePush.queuedWriteTx

  // Converted float writes must be lossless. Hold the result until the FIFO
  // accepts it, which backpressures the host while conversion/output is busy.
  floatConverter.io.result.ready := queuedWriteTx.ready
  when(floatConverter.io.result.fire) {
    floatConvertInFlight := False
  }

  stageFifo.io.pop.ready := False
  when(stagedWrite.valid && !stagedWrite.isFloatAlias) {
    stageFifo.io.pop.ready := queuedWriteTx.fire
  }
  when(floatConvertInFlight && floatConverter.io.result.valid) {
    stageFifo.io.pop.ready := queuedWriteTx.fire
  }

  // Texture writes queue behind CPU writes but no longer stall behind float conversion bubbles.
  io.texWrite.ready := !queueableCpuWrite && stageFifo.io.push.ready

  // wasEnqueued: detect when a specific register write was actually enqueued
  io.wasEnqueued := queuedWriteTx.fire && !queuedWriteTx.isTexture &&
    queuedWriteTx.address === io.wasEnqueuedAddr

  // ========================================================================
  // 64-entry PCI FIFO
  // ========================================================================
  private val pciQueue = new Area {
    val fifo = StreamFifo(QueuedWrite(), depth = 64)
    fifo.io.push << queuedWriteTx
    io.pciFifoFree := fifo.io.availability
  }
  private val pciFifo = pciQueue.fifo

  // ========================================================================
  // Drain blocking logic
  // ========================================================================
  private val drainControl = new Area {
    val canDrain = Bool()
    canDrain := True

    // Registered issue stage directly after the PCI FIFO. This keeps a single clean
    // dequeue boundary while breaking the downstream control loops.
    val issuedWrite = pciFifo.io.pop.haltWhen(!canDrain).m2sPipe()

    when(pciFifo.io.pop.syncRequired && io.pipelineBusy) {
      canDrain := False
    }

    val commandStreamBlocked = Bool()
    commandStreamBlocked := False
    when(commandStreamBlocked) {
      canDrain := False
    }

    val fifoEmpty = Bool().setName("pciFifo_fifoEmpty")
    fifoEmpty.simPublic()
    fifoEmpty := !pciFifo.io.pop.valid && !issuedWrite.valid
    io.fifoEmpty := fifoEmpty
  }
  private val canDrain = drainControl.canDrain
  private val issuedWrite = drainControl.issuedWrite
  private val commandStreamBlocked = drainControl.commandStreamBlocked
  private val drainedWrite = issuedWrite
  private val fifoEmpty = drainControl.fifoEmpty

  // ========================================================================
  // Drain routing: register writes vs texture writes
  // ========================================================================

  private val drainRoute = new Area {
    val isTexDrain = drainedWrite.valid && drainedWrite.isTexture
    val isRegDrain = drainedWrite.valid && !drainedWrite.isTexture

    io.texDrain.valid := isTexDrain
    io.texDrain.pciAddr := drainedWrite.texPciAddr
    io.texDrain.data := drainedWrite.data
    io.texDrain.mask := drainedWrite.mask
    io.floatShadow.valid := isRegDrain && drainedWrite.fire && drainedWrite.hasFloatShadow
    io.floatShadow.address := drainedWrite.address
    io.floatShadow.raw := drainedWrite.floatShadowRaw

    io.syncDrained := isRegDrain && drainedWrite.fire && drainedWrite.syncRequired
  }
  val isTexDrain = drainRoute.isTexDrain
  val isRegDrain = drainRoute.isRegDrain

  // ========================================================================
  // Master bus (regSide) mux
  // ========================================================================
  private val regSideMux = new Area {
    // Direct writes (FIFO=No) — register bypass
    // Uses isWriteCmd (cmd.valid) not doWriteImmediate (cmd.fire) to avoid combinatorial loop
    // through cmd.ready → fire → doWriteDirect → cpuPassthrough → stall → cmd.ready
    val doWriteDirect = isWriteCmd && fifoBypass
    val cpuPassthrough = io.cpuSide.cmd.valid &&
      (io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.READ || doWriteDirect)
    val doRegDrain = isRegDrain && !cpuPassthrough

    io.regSide.cmd.valid := cpuPassthrough || doRegDrain
    io.regSide.cmd.last := True
    io.regSide.cmd.source := io.cpuSide.cmd.source
    if (busParams.access.contextWidth > 0)
      io.regSide.cmd.context := io.cpuSide.cmd.context

    when(doRegDrain) {
      io.regSide.cmd.opcode := Bmb.Cmd.Opcode.WRITE
      io.regSide.cmd.address := drainedWrite.address
      io.regSide.cmd.data := drainedWrite.data
      io.regSide.cmd.mask := drainedWrite.mask
      io.regSide.cmd.length := 3
    } otherwise {
      io.regSide.cmd.opcode := io.cpuSide.cmd.opcode
      io.regSide.cmd.address := io.cpuSide.cmd.address
      io.regSide.cmd.data := io.cpuSide.cmd.data
      io.regSide.cmd.mask := io.cpuSide.cmd.mask
      io.regSide.cmd.length := io.cpuSide.cmd.length
    }

    drainedWrite.ready := (isRegDrain && !cpuPassthrough && io.regSide.cmd.ready) ||
      (isTexDrain && io.texDrain.ready)
  }
  private val doWriteDirect = regSideMux.doWriteDirect
  val cpuPassthrough = regSideMux.cpuPassthrough
  val doRegDrain = regSideMux.doRegDrain

  // ========================================================================
  // Response routing
  // ========================================================================
  // CPU passthrough response: forward from regSide back to cpuSide
  // FIFO'd write: generate immediate response at enqueue time
  // Drain response: consumed/discarded

  private val response = new Area {
    io.regSide.rsp.ready := True

    io.cpuSide.rsp.valid := RegNext(
      doWriteImmediate || (io.cpuSide.cmd.fire && io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.READ)
    ) init (False)
    io.cpuSide.rsp.last := True
    io.cpuSide.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
    io.cpuSide.rsp.source := RegNext(io.cpuSide.cmd.source)
    if (busParams.access.contextWidth > 0)
      io.cpuSide.rsp.context := RegNext(io.cpuSide.cmd.context)

    val pendingRead = RegInit(False)
    when(io.cpuSide.cmd.fire && io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.READ) {
      pendingRead := True
    }
    when(pendingRead && io.regSide.rsp.valid) {
      pendingRead := False
    }

    when(pendingRead) {
      io.cpuSide.rsp.valid := io.regSide.rsp.valid
      io.cpuSide.rsp.data := io.regSide.rsp.data
      io.cpuSide.rsp.opcode := io.regSide.rsp.opcode
      io.cpuSide.rsp.source := io.regSide.rsp.source
    } otherwise {
      io.cpuSide.rsp.data := 0
    }
  }
  val pendingRead = response.pendingRead

  // ========================================================================
  // Stall logic
  // ========================================================================
  private val cpuReady = new Area {
    io.cpuSide.cmd.ready := True
    when(queueableCpuWrite) {
      io.cpuSide.cmd.ready := stageFifo.io.push.ready
    }
    when(io.cpuSide.cmd.valid && io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.READ && pendingRead) {
      io.cpuSide.cmd.ready := False
    }
    when(
      io.cpuSide.cmd.valid && io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.READ && !pendingRead &&
        !io.regSide.cmd.ready
    ) {
      io.cpuSide.cmd.ready := False
    }
  }

  // ========================================================================
  // Address categorization (addPrePopTask — runs after all registers are created)
  // ========================================================================
  Component.current.addPrePopTask(() => {
    // Debug: print registered categories
    println(s"[PciFifo] Registered categories: ${categories.size} entries")
    categories.foreach { case (addr, cat) =>
      println(f"  0x$addr%03x: fifoBypass=${cat.fifoBypass}, syncRequired=${cat.syncRequired}")
    }

    // FIFO queue addresses: only explicitly registered FIFO=Yes addresses go through FIFO
    val fifoQueueCases = categories.collect {
      case (addr, cat) if !cat.fifoBypass => addr
    }.toSeq
    val syncCases = categories.collect {
      case (addr, cat) if cat.syncRequired => addr
    }.toSeq

    println(
      s"[PciFifo] FIFO queue addresses: ${fifoQueueCases.map(a => f"0x$a%03x").mkString(", ")}"
    )
    println(s"[PciFifo] Sync addresses: ${syncCases.map(a => f"0x$a%03x").mkString(", ")}")

    // Default: unregistered addresses bypass FIFO
    fifoBypass := True
    switch(io.cpuSide.cmd.address) {
      for (addr <- fifoQueueCases) {
        is(U(addr, busAddrWidth bits)) {
          fifoBypass := False
        }
      }
    }

    syncRequired := False
    switch(io.cpuSide.cmd.address) {
      for (addr <- syncCases) {
        is(U(addr, busAddrWidth bits)) {
          syncRequired := True
        }
      }
    }

    // Float alias address decode
    isFloatAlias := False
    floatTargetAddr := 0
    floatShiftAmount := 0
    for ((floatAddr, config) <- floatAliases) {
      when(io.cpuSide.cmd.address === U(floatAddr, busAddrWidth bits)) {
        isFloatAlias := True
        floatTargetAddr := U(config.targetAddr, busAddrWidth bits)
        floatShiftAmount := U(30 - config.fracBits, 5 bits)
      }
    }

    // Command stream drain blocking
    println(
      s"[PciFifo] Command stream addresses: ${commandAddresses.map(a => f"0x$a%03x").mkString(", ")}"
    )
    if (commandAddresses.nonEmpty) {
      for ((addr, idx) <- commandAddresses.zipWithIndex) {
        when(pciFifo.io.pop.address === U(addr, busAddrWidth bits)) {
          commandStreamBlocked := !io.commandReady(idx)
        }
      }
    }
  })

  GenerationFlags.formal {
    assert(io.fifoEmpty === (!pciFifo.io.pop.valid && !issuedWrite.valid))
    assert(isTexDrain === (drainedWrite.valid && drainedWrite.isTexture))
    assert(isRegDrain === (drainedWrite.valid && !drainedWrite.isTexture))
    assert(io.texDrain.valid === isTexDrain)
    assert(
      io.floatShadow.valid === (isRegDrain && drainedWrite.fire && drainedWrite.hasFloatShadow)
    )
    assert(io.syncDrained === (isRegDrain && drainedWrite.fire && drainedWrite.syncRequired))
    assert(
      drainedWrite.ready === ((isRegDrain && !cpuPassthrough && io.regSide.cmd.ready) ||
        (isTexDrain && io.texDrain.ready))
    )
    assert(io.regSide.cmd.valid === (cpuPassthrough || doRegDrain))

    when(doWriteDirect) {
      assert(cpuPassthrough)
    }

    when(cpuPassthrough) {
      assert(io.regSide.cmd.valid)
      assert(!doRegDrain)
      assert(io.regSide.cmd.opcode === io.cpuSide.cmd.opcode)
      assert(io.regSide.cmd.address === io.cpuSide.cmd.address)
      assert(io.regSide.cmd.data === io.cpuSide.cmd.data)
      assert(io.regSide.cmd.mask === io.cpuSide.cmd.mask)
      assert(io.regSide.cmd.length === io.cpuSide.cmd.length)
      assert(io.regSide.cmd.last)
    }

    when(doRegDrain) {
      assert(io.regSide.cmd.valid)
      assert(io.regSide.cmd.opcode === Bmb.Cmd.Opcode.WRITE)
      assert(io.regSide.cmd.address === drainedWrite.address)
      assert(io.regSide.cmd.data === drainedWrite.data)
      assert(io.regSide.cmd.mask === drainedWrite.mask)
      assert(io.regSide.cmd.length === 3)
      assert(io.regSide.cmd.last)
    }

    when(pciFifo.io.pop.syncRequired && io.pipelineBusy) {
      assert(!drainedWrite.fire)
    }
    when(commandStreamBlocked) {
      assert(!drainedWrite.fire)
    }

    when(pendingRead) {
      assert(io.cpuSide.rsp.valid === io.regSide.rsp.valid)
      assert(io.cpuSide.rsp.data === io.regSide.rsp.data)
      assert(io.cpuSide.rsp.opcode === io.regSide.rsp.opcode)
      assert(io.cpuSide.rsp.source === io.regSide.rsp.source)
    }

    when(io.cpuSide.cmd.valid && io.cpuSide.cmd.opcode === Bmb.Cmd.Opcode.READ && pendingRead) {
      assert(!io.cpuSide.cmd.ready)
    }

    when(floatWriteRequest && !floatConvertInFlight) {
      assert(floatConverter.io.op.valid)
    }

    when(floatConvertInFlight && floatConverter.io.result.valid) {
      val rawSInt = floatConverter.io.result.number.raw.asSInt
      val shifted = rawSInt >> floatConvertShift
      val hasRemainder = Bool()
      hasRemainder := False
      switch(floatConvertShift) {
        is(26) { hasRemainder := rawSInt(25 downto 0).orR }
        is(18) { hasRemainder := rawSInt(17 downto 0).orR }
        is(12) { hasRemainder := rawSInt(11 downto 0).orR }
      }
      val corrected = Mux(rawSInt.msb && hasRemainder, shifted + 1, shifted)

      assert(queuedWriteTx.valid)
      assert(queuedWriteTx.address === floatConvertTargetAddr)
      assert(queuedWriteTx.data === corrected.resize(32 bits).asBits)
      assert(queuedWriteTx.mask === B"1111")
      assert(!queuedWriteTx.syncRequired)
      assert(!queuedWriteTx.isTexture)
      assert(!queuedWriteTx.texPciAddr.orR)
      assert(queuedWriteTx.hasFloatShadow)
      assert(queuedWriteTx.floatShadowRaw === rawSInt.resize(50 bits))
    }

    when(io.floatShadow.valid) {
      assert(isRegDrain)
      assert(drainedWrite.fire)
      assert(drainedWrite.hasFloatShadow)
      assert(io.floatShadow.address === drainedWrite.address)
      assert(io.floatShadow.raw === drainedWrite.floatShadowRaw)
    }

    if (formalStrong) {
      val fOccupancy = Reg(UInt(7 bits)) init (0)
      when(queuedWriteTx.fire =/= drainedWrite.fire) {
        when(queuedWriteTx.fire) {
          fOccupancy := fOccupancy + 1
        } otherwise {
          fOccupancy := fOccupancy - 1
        }
      }
      assert(io.pciFifoFree + fOccupancy === U(64, 7 bits))

      val fTrackIdx0 = anyconst(UInt(3 bits))
      val fTrackIdx1 = anyconst(UInt(3 bits))
      assume(fTrackIdx0 < 4)
      assume(fTrackIdx1 < 4)
      assume(fTrackIdx0 < fTrackIdx1)

      val fEnqueueCount = Reg(UInt(4 bits)) init (0)
      val fDequeueCount = Reg(UInt(4 bits)) init (0)
      val fSeen0 = Reg(Bool()) init (False)
      val fSeen1 = Reg(Bool()) init (False)
      val fPopped0 = Reg(Bool()) init (False)
      val fPopped1 = Reg(Bool()) init (False)
      val fPayload0 = Reg(QueuedWrite())
      val fPayload1 = Reg(QueuedWrite())

      when(queuedWriteTx.fire) {
        when(fEnqueueCount === fTrackIdx0.resized) {
          fSeen0 := True
          fPayload0 := queuedWriteTx.payload
        }
        when(fEnqueueCount === fTrackIdx1.resized) {
          fSeen1 := True
          fPayload1 := queuedWriteTx.payload
        }
        fEnqueueCount := fEnqueueCount + 1
      }

      when(drainedWrite.fire) {
        when(fSeen0 && fDequeueCount === fTrackIdx0.resized) {
          assert(drainedWrite.payload.asBits === fPayload0.asBits)
          fPopped0 := True
        }
        when(fSeen1 && fDequeueCount === fTrackIdx1.resized) {
          assert(fPopped0)
          assert(drainedWrite.payload.asBits === fPayload1.asBits)
          fPopped1 := True
        }
        fDequeueCount := fDequeueCount + 1
      }

      when(fPopped1) {
        assert(fPopped0)
      }
    }
  }
}

object PciFifo {

  /** Float alias configuration (public for metadata export) */
  case class FloatAliasConfig(targetAddr: BigInt, intBits: Int, fracBits: Int)
}
