package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._

case class SimMemoryTiming(
    mode: String = "onchip",
    fixedReadLatency: Int = 0,
    burstSetupLatency: Int = 12,
    burstBeatLatency: Int = 1,
    writeLatency: Int = 0
) {
  require(Set("onchip", "ideal", "fixed", "burst").contains(mode))
  require(fixedReadLatency >= 0)
  require(burstSetupLatency >= 0)
  require(burstBeatLatency >= 0)
  require(writeLatency >= 0)
}

case class SimBmbMemory(p: BmbParameter, size: Int, timing: SimMemoryTiming) extends Component {
  require(size > 0 && (size % 4) == 0)

  val io = new Bundle {
    val bus = slave(Bmb(p))
  }

  private val wordCount = size / 4
  private val wordAddrWidth = log2Up(wordCount)
  private val burstCountWidth = p.access.lengthWidth
  private val maxLatency = List(
    timing.fixedReadLatency,
    timing.burstSetupLatency,
    timing.burstBeatLatency,
    timing.writeLatency
  ).max
  private val latencyWidth = log2Up(maxLatency + 1)

  val ram_symbol0 = Mem(Bits(8 bits), wordCount)
  val ram_symbol1 = Mem(Bits(8 bits), wordCount)
  val ram_symbol2 = Mem(Bits(8 bits), wordCount)
  val ram_symbol3 = Mem(Bits(8 bits), wordCount)
  private val byteLanes = Seq(ram_symbol0, ram_symbol1, ram_symbol2, ram_symbol3)

  private val cmdWordAddr = io.bus.cmd.address(wordAddrWidth + 1 downto 2)
  private val cmdByteCount =
    (io.bus.cmd.length.resize(p.access.lengthWidth + 1 bits) + U(1, p.access.lengthWidth + 1 bits))
  private val cmdWordCount = (cmdByteCount >> 2).resize(burstCountWidth bits)

  object OpState extends SpinalEnum {
    val Idle, ReadRsp, WriteCmd, WriteRsp = newElement()
  }
  private val state = RegInit(OpState.Idle)
  private val currentWordAddr = Reg(UInt(wordAddrWidth bits)) init (0)
  private val wordsRemaining = Reg(UInt(burstCountWidth bits)) init (0)
  private val rspDelay = Reg(UInt(latencyWidth bits)) init (0)
  private val rspSource = Reg(UInt(p.access.sourceWidth bits)) init (0)
  private val rspContext =
    if (p.access.contextWidth > 0) Reg(Bits(p.access.contextWidth bits)) init (0) else null

  private def latencyValue(v: Int) = U(v, latencyWidth bits)

  private val readDataBytes = byteLanes.map(_.readAsync(currentWordAddr))
  private val readData =
    readDataBytes(3) ## readDataBytes(2) ## readDataBytes(1) ## readDataBytes(0)

  private val readActive = state === OpState.ReadRsp
  private val rspValid = (state === OpState.ReadRsp || state === OpState.WriteRsp) && rspDelay === 0

  io.bus.cmd.ready := state === OpState.Idle || state === OpState.WriteCmd
  io.bus.rsp.valid := rspValid
  io.bus.rsp.data := readActive ? readData | B(0, 32 bits)
  io.bus.rsp.last := !readActive || wordsRemaining === 1
  io.bus.rsp.opcode := Bmb.Rsp.Opcode.SUCCESS
  io.bus.rsp.source := rspSource
  if (p.access.contextWidth > 0) {
    io.bus.rsp.context := rspContext
  }

  private val doCmd = io.bus.cmd.fire
  private val doReadCmd = doCmd && io.bus.cmd.opcode === Bmb.Cmd.Opcode.READ
  private val doWriteCmd = doCmd && io.bus.cmd.opcode === Bmb.Cmd.Opcode.WRITE
  private val doRsp = io.bus.rsp.fire

  when(state === OpState.Idle && io.bus.cmd.valid) {
    rspSource := io.bus.cmd.source
    if (p.access.contextWidth > 0) {
      rspContext := io.bus.cmd.context
    }
  }

  for (lane <- 0 until 4) {
    byteLanes(lane).write(
      address = cmdWordAddr,
      data = io.bus.cmd.data(8 * lane + 7 downto 8 * lane),
      enable = doWriteCmd && io.bus.cmd.mask(lane)
    )
  }

  when(doReadCmd) {
    state := OpState.ReadRsp
    currentWordAddr := cmdWordAddr
    wordsRemaining := cmdWordCount
    if (timing.mode == "fixed") {
      rspDelay := latencyValue(timing.fixedReadLatency)
    } else if (timing.mode == "burst") {
      rspDelay := latencyValue(timing.burstSetupLatency)
    } else {
      rspDelay := 0
    }
  }

  when(doWriteCmd) {
    when(state === OpState.Idle) {
      rspSource := io.bus.cmd.source
      if (p.access.contextWidth > 0) {
        rspContext := io.bus.cmd.context
      }
    }
    wordsRemaining := 1
    when(io.bus.cmd.last) {
      state := OpState.WriteRsp
      rspDelay := latencyValue(timing.writeLatency)
    }.otherwise {
      state := OpState.WriteCmd
      rspDelay := 0
    }
  }

  when((state === OpState.ReadRsp || state === OpState.WriteRsp) && rspDelay =/= 0) {
    rspDelay := rspDelay - 1
  }

  when(doRsp) {
    when(state === OpState.ReadRsp && wordsRemaining > 1) {
      currentWordAddr := currentWordAddr + 1
      wordsRemaining := wordsRemaining - 1
      if (timing.mode == "fixed") {
        rspDelay := latencyValue(timing.fixedReadLatency)
      } else if (timing.mode == "burst") {
        rspDelay := latencyValue(timing.burstBeatLatency)
      } else {
        rspDelay := 0
      }
    }.otherwise {
      state := OpState.Idle
      wordsRemaining := 0
      rspDelay := 0
    }
  }
}
