package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

case class FramebufferPlaneReader(c: Config, queueDepth: Int = 16) extends Component {
  import FramebufferPlaneBuffer._

  val io = new Bundle {
    val readReq = slave Stream (ReadReq(c))
    val readRsp = master Stream (ReadRsp())
    val mem = master(Bmb(bmbParams(c)))
    val busy = out Bool ()
    val fillHits = out UInt (32 bits)
    val fillMisses = out UInt (32 bits)
    val fillBurstCount = out UInt (32 bits)
    val fillBurstBeats = out UInt (32 bits)
    val fillStallCycles = out UInt (32 bits)
  }

  require(queueDepth > 1 && ((queueDepth & (queueDepth - 1)) == 0))

  val ptrWidth = log2Up(queueDepth)
  val occupancyWidth = log2Up(queueDepth + 1)
  val maxBurstWords = Math.min(queueDepth, c.fbWriteBufferLineWords)
  val burstCountWidth = log2Up(maxBurstWords + 1)

  val reqMem = Vec(Reg(UInt(c.addressWidth.value bits)) init (0), queueDepth)
  val head = Reg(UInt(ptrWidth bits)) init (0)
  val tail = Reg(UInt(ptrWidth bits)) init (0)
  val occupancy = Reg(UInt(occupancyWidth bits)) init (0)

  val fillHits = U(0, 32 bits)
  val fillMisses = Reg(UInt(32 bits)) init (0)
  val fillBurstCount = Reg(UInt(32 bits)) init (0)
  val fillBurstBeats = Reg(UInt(32 bits)) init (0)
  val fillStallCycles = Reg(UInt(32 bits)) init (0)
  if (c.trace.enabled) {
    fillMisses.simPublic()
    fillBurstCount.simPublic()
    fillBurstBeats.simPublic()
    fillStallCycles.simPublic()
  }
  io.fillHits := fillHits
  io.fillMisses := fillMisses
  io.fillBurstCount := fillBurstCount
  io.fillBurstBeats := fillBurstBeats
  io.fillStallCycles := fillStallCycles

  val burstActive = Reg(Bool()) init (False)
  val rspWordsLeft = Reg(UInt(burstCountWidth bits)) init (0)

  val rspQueue = io.mem.rsp
    .takeWhen(io.mem.rsp.source === 0)
    .translateWith(io.mem.rsp.fragment.data)
    .queue(queueDepth * 2)
  io.readRsp.valid := rspQueue.valid
  io.readRsp.data := rspQueue.payload
  rspQueue.ready := io.readRsp.ready

  def ptrPlus(base: UInt, offset: Int): UInt = (base + U(offset, ptrWidth bits)).resized

  val queuedAddrs = Vec(UInt(c.addressWidth.value bits), maxBurstWords)
  for (i <- 0 until maxBurstWords) {
    queuedAddrs(i) := reqMem(ptrPlus(head, i))
  }

  val burstWordCount = UInt(burstCountWidth bits)
  burstWordCount := 0
  when(occupancy =/= 0) {
    burstWordCount := 1
    var prefixSequential = True
    for (i <- 1 until maxBurstWords) {
      val expected = (queuedAddrs(0) + U(i * 4, c.addressWidth.value bits)).resized
      val contiguous = occupancy > i && queuedAddrs(i) === expected
      when(prefixSequential && contiguous) {
        burstWordCount := U(i + 1, burstCountWidth bits)
      }
      prefixSequential = prefixSequential && contiguous
    }
  }

  io.readReq.ready := occupancy =/= queueDepth
  when(io.readReq.valid && !io.readReq.ready) {
    fillStallCycles := fillStallCycles + 1
  }
  when(io.readReq.fire) {
    reqMem(tail) := io.readReq.address
    tail := tail + 1
    occupancy := occupancy + 1
  }

  io.mem.cmd.valid := False
  io.mem.cmd.fragment.address := queuedAddrs(0)
  io.mem.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
  io.mem.cmd.fragment.length := (((burstWordCount.resize(
    c.memBurstLengthWidth bits
  )) << 2) - 1).resized
  io.mem.cmd.fragment.source := 0
  io.mem.cmd.fragment.data := 0
  io.mem.cmd.fragment.mask := 0
  io.mem.cmd.last := True

  when(!burstActive && occupancy =/= 0) {
    io.mem.cmd.valid := True
    when(io.mem.cmd.fire) {
      val consumed = burstWordCount.resize(occupancyWidth bits)
      head := (head + burstWordCount.resized).resized
      occupancy := occupancy - consumed
      rspWordsLeft := (burstWordCount - 1).resized
      fillMisses := fillMisses + consumed.resized
      fillBurstCount := fillBurstCount + 1
      fillBurstBeats := fillBurstBeats + consumed.resized
      burstActive := True
    }
  }

  when(io.mem.rsp.fire && burstActive && io.mem.rsp.source === 0) {
    when(rspWordsLeft === 0) {
      burstActive := False
    }.otherwise {
      rspWordsLeft := rspWordsLeft - 1
    }
  }

  io.busy := burstActive || occupancy =/= 0 || rspQueue.valid
}

object FramebufferPlaneReader {
  def bmbParams(c: Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = true,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}
