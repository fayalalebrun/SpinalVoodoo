package voodoo.mister

import spinal.core._
import spinal.lib._

object MisterMemoryMap {
  val hostBase = BigInt("3e000000", 16)
  val framebufferBase = BigInt("3e400000", 16)
  val framebufferEnd = BigInt("3f000000", 16)
  val framebufferSlotBytes = 4 * 1024 * 1024
  val framebufferSlots = 3
  val rendererBase = BigInt("3f400000", 16)
  val rendererEnd = BigInt("3f800000", 16)
}

case class MisterFrame() extends Bundle {
  val sequence = UInt(32 bits)
  val base = UInt(32 bits)
  val width = UInt(12 bits)
  val height = UInt(12 bits)
  val stride = UInt(14 bits)
  val format = Bits(5 bits)
  val enable = Bool()
}

case class MisterFrameAck() extends Bundle {
  val sequence = UInt(32 bits)
  val error = Bool()
}

case class MisterFramebufferControl() extends Component {
  val io = new Bundle {
    val submit = slave(Stream(MisterFrame()))
    val acknowledge = master(Stream(MisterFrameAck()))
    val vblank = in Bool ()
    val active = out(MisterFrame())
  }

  val active = Reg(MisterFrame()) init (MisterFrame().getZero)
  val pending = Reg(MisterFrame())
  val pendingValid = RegInit(False)
  val ack = Reg(MisterFrameAck())
  val ackValid = RegInit(False)
  val vblankRise = io.vblank && !RegNext(io.vblank).init(False)
  val frame = io.submit.payload
  val rgb565 = frame.format === B(4, 5 bits)
  val xrgb8888 = frame.format === B(6, 5 bits)
  val rowBytes = Mux(rgb565, (frame.width << 1).resize(16), (frame.width << 2).resize(16))
  val extent = ((frame.height - 1) * frame.stride).resize(26) + rowBytes.resize(26)
  val slotBase = frame.base === MisterMemoryMap.framebufferBase ||
    frame.base === MisterMemoryMap.framebufferBase + MisterMemoryMap.framebufferSlotBytes ||
    frame.base === MisterMemoryMap.framebufferBase + 2 * MisterMemoryMap.framebufferSlotBytes
  val hostRange = slotBase && extent <= MisterMemoryMap.framebufferSlotBytes
  val rendererEnd = frame.base(21 downto 0).resize(27) + extent.resize(27)
  val rendererRange = frame.base >= MisterMemoryMap.rendererBase &&
    frame.base < MisterMemoryMap.rendererEnd && frame.base(11 downto 0) === 0 &&
    rendererEnd <= MisterMemoryMap.framebufferSlotBytes && rgb565 && frame.width === 640 && frame.height === 480
  val valid = !frame.enable || ((hostRange || rendererRange) && (rgb565 || xrgb8888) &&
    frame.width =/= 0 && frame.width <= 1920 && frame.height =/= 0 && frame.height <= 1200 &&
    frame.stride.resize(16) >= rowBytes && frame.stride(1 downto 0) === 0)

  io.active := active
  io.submit.ready := !pendingValid && !ackValid
  io.acknowledge.valid := ackValid
  io.acknowledge.payload := ack
  when(io.acknowledge.fire) {
    ackValid := False
  }
  when(io.submit.fire) {
    when(valid) {
      pending := frame
      pendingValid := True
    } otherwise {
      ack.sequence := frame.sequence
      ack.error := True
      ackValid := True
    }
  }
  when(pendingValid && vblankRise) {
    active := pending
    pendingValid := False
    ack.sequence := pending.sequence
    ack.error := False
    ackValid := True
  }
}

case class MisterKeyboardState() extends Component {
  val io = new Bundle {
    val ps2Key = in Bits (11 bits)
    val osdOpen = in Bool ()
    val keys = out Bits (512 bits)
    val sequence = out UInt (32 bits)
  }

  val keys = Reg(Bits(512 bits)) init (0)
  val sequence = Reg(UInt(32 bits)) init (0)
  val lastToggle = RegNext(io.ps2Key(10)) init (False)
  val lastOsd = RegNext(io.osdOpen) init (False)
  val changed = io.ps2Key(10) =/= lastToggle
  when(io.osdOpen) {
    keys := 0
    when(!lastOsd) {
      sequence := sequence + 1
    }
  } elsewhen (changed) {
    keys(io.ps2Key(8 downto 0).asUInt) := io.ps2Key(9)
    sequence := sequence + 1
  }
  io.keys := keys
  io.sequence := sequence
}
