package voodoo.mister

import spinal.core._
import spinal.lib._
import voodoo.de10.H2fLwMmio

object MisterMailbox {
  val magic = BigInt("4d565331", 16)
  val requestOffset = 0x100
  val resultOffset = 0x2000
  val keysOffset = 0x2800
  val ackOffset = 0x80
  val maxCommands = 256
  val chunkCommands = 32
}

case class MisterMailbox(pollCycles: Int = 1024) extends Component {
  import MisterMailbox._
  require(pollCycles >= 1)
  val io = new Bundle {
    val memory = master(MisterMemoryPort())
    val mmio = master(H2fLwMmio(22))
    val frame = master(Stream(MisterFrame()))
    val frameAck = slave(Stream(MisterFrameAck()))
    val keys = in Bits (512 bits)
    val keySequence = in UInt (32 bits)
  }
  object State extends SpinalEnum {
    val poll, header, count, countWait, fetch, fill, first, second, execute, mmio, mmioRead, result,
        frame, frameWait, keys, acknowledge, delay = newElement()
  }
  val state = RegInit(State.poll)
  val boot = RegInit(True)
  val completed = Reg(UInt(32 bits)) init (0)
  val sequence = Reg(UInt(32 bits)) init (0)
  val count = Reg(UInt(9 bits)) init (0)
  val index = Reg(UInt(9 bits)) init (0)
  val error = Reg(UInt(8 bits)) init (0)
  val chunkLeft = Reg(UInt(6 bits)) init (0)
  val responsesLeft = Reg(UInt(7 bits)) init (0)
  val first = Reg(Bits(64 bits))
  val second = Reg(Bits(64 bits))
  val result = Reg(Bits(32 bits))
  val keys = Reg(Bits(512 bits))
  val keySequence = Reg(UInt(32 bits))
  val keyIndex = Reg(UInt(4 bits)) init (0)
  val delay = Reg(UInt(log2Up(pollCycles + 1) bits)) init (0)
  val fifo = StreamFifo(Bits(64 bits), chunkCommands * 2)
  val chunk =
    Mux(count - index > chunkCommands, U(chunkCommands, 6 bits), (count - index).resize(6))
  val opcode = first(31 downto 0).asUInt
  val address = first(63 downto 32).asUInt
  val value = second(31 downto 0)
  val mask = second(63 downto 32)
  val readMemory = state === State.poll || state === State.count || state === State.fetch

  io.memory.command.valid := readMemory || state === State.result ||
    state === State.keys || state === State.acknowledge
  io.memory.command.read := readMemory
  io.memory.command.address := MisterMemoryMap.hostBase
  io.memory.command.data := 0
  io.memory.command.mask := 255
  io.memory.command.count := 1
  switch(state) {
    is(State.count) { io.memory.command.address := MisterMemoryMap.hostBase + 8 }
    is(State.fetch) {
      io.memory.command.address := U(MisterMemoryMap.hostBase + requestOffset, 32 bits) +
        (index.resize(32) << 4).resized
      io.memory.command.count := (chunk.resize(8) << 1).resized
    }
    is(State.result) {
      io.memory.command.address := U(MisterMemoryMap.hostBase + resultOffset, 32 bits) +
        ((index.resize(32) >> 1) << 3).resized
      io.memory.command.data := Mux(index(0), result ## B(0, 32 bits), B(0, 32 bits) ## result)
      io.memory.command.mask := Mux(index(0), B(240, 8 bits), B(15, 8 bits))
    }
    is(State.keys) {
      io.memory.command.address := U(MisterMemoryMap.hostBase + keysOffset, 32 bits) +
        (keyIndex.resize(32) << 3).resized
      io.memory.command.data := Mux(
        keyIndex === 8,
        B(0, 32 bits) ## keySequence.asBits,
        keys.subdivideIn(64 bits)(keyIndex.resize(3))
      )
    }
    is(State.acknowledge) {
      io.memory.command.address := MisterMemoryMap.hostBase + ackOffset
      io.memory.command.data := B(0, 15 bits) ## index.asBits ## error.asBits ## sequence.asBits
    }
  }
  val incoming = io.memory.response
  fifo.io.push.valid := incoming.valid && (state === State.fill || state === State.fetch)
  fifo.io.push.payload := incoming.payload
  fifo.io.pop.ready := state === State.first || state === State.second
  fifo.io.flush := state === State.acknowledge
  val chunkRead = io.memory.command.fire && state === State.fetch
  when(chunkRead) {
    responsesLeft := (chunk.resize(7) << 1).resize(7) - incoming.valid.asUInt.resize(7)
  } elsewhen (state === State.fill && incoming.valid) {
    responsesLeft := responsesLeft - 1
  }

  io.mmio.address := address(23 downto 2)
  io.mmio.writedata := value
  io.mmio.byteenable := mask(3 downto 0)
  io.mmio.read := state === State.mmio && opcode === 2
  io.mmio.write := state === State.mmio && opcode === 1
  io.frame.valid := state === State.frame
  io.frame.sequence := sequence
  io.frame.base := address
  io.frame.width := value(11 downto 0).asUInt
  io.frame.height := value(23 downto 12).asUInt
  io.frame.format := value(28 downto 24)
  io.frame.enable := value(29)
  io.frame.stride := mask(13 downto 0).asUInt
  io.frameAck.ready := state === State.frameWait

  def nextCommand(): Unit = {
    index := index + 1
    chunkLeft := chunkLeft - 1
    when(index + 1 === count) {
      state := State.acknowledge
    } elsewhen (chunkLeft === 1) {
      state := State.fetch
    } otherwise {
      state := State.first
    }
  }
  def acceptHeader(): Unit = {
    sequence := incoming.payload(63 downto 32).asUInt
    index := 0
    error := 0
    when(boot) {
      boot := False
      error := 4
      state := State.acknowledge
    } elsewhen (incoming.payload(31 downto 0) === B(magic, 32 bits) &&
      incoming.payload(63 downto 32).asUInt =/= completed) {
      state := State.count
    } otherwise {
      delay := pollCycles - 1
      state := State.delay
    }
  }
  def acceptCount(): Unit = {
    when(
      incoming.payload(63 downto 32) =/= 0 || incoming.payload(31 downto 0).asUInt > maxCommands
    ) {
      error := 1
      state := State.acknowledge
    } elsewhen (incoming.payload(31 downto 0) === 0) {
      state := State.acknowledge
    } otherwise {
      count := incoming.payload(8 downto 0).asUInt
      state := State.fetch
    }
  }
  switch(state) {
    is(State.poll) {
      when(io.memory.command.fire) {
        state := State.header
        when(incoming.valid) { acceptHeader() }
      }
    }
    is(State.header) { when(incoming.valid) { acceptHeader() } }
    is(State.count) {
      when(io.memory.command.fire) {
        state := State.countWait
        when(incoming.valid) { acceptCount() }
      }
    }
    is(State.countWait) { when(incoming.valid) { acceptCount() } }
    is(State.fetch) {
      when(chunkRead) {
        chunkLeft := chunk
        state := State.fill
      }
    }
    is(State.fill) {
      when(incoming.valid && responsesLeft === 1) { state := State.first }
    }
    is(State.first) {
      when(fifo.io.pop.fire) { first := fifo.io.pop.payload; state := State.second }
    }
    is(State.second) {
      when(fifo.io.pop.fire) { second := fifo.io.pop.payload; state := State.execute }
    }
    is(State.execute) {
      when(opcode === 0 && address === 0 && second === 0) { nextCommand() } elsewhen (
        (opcode === 1 || opcode === 2) && address(31 downto 24) === 0 &&
          address(1 downto 0) === 0 && mask(31 downto 4) === 0 && mask(3 downto 0) =/= 0
      ) {
        state := State.mmio
      } elsewhen (opcode === 3 && value(31 downto 30) === 0 && mask(31 downto 14) === 0) {
        state := State.frame
      } elsewhen (opcode === 4 && address === 0 && second === 0) {
        keys := io.keys
        keySequence := io.keySequence
        keyIndex := 0
        state := State.keys
      } otherwise {
        error := 2
        state := State.acknowledge
      }
    }
    is(State.mmio) {
      when(!io.mmio.waitrequest) {
        when(opcode === 1) { nextCommand() } otherwise {
          state := State.mmioRead
          when(io.mmio.readdatavalid) { result := io.mmio.readdata; state := State.result }
        }
      }
    }
    is(State.mmioRead) {
      when(io.mmio.readdatavalid) { result := io.mmio.readdata; state := State.result }
    }
    is(State.result) { when(io.memory.command.fire) { nextCommand() } }
    is(State.frame) { when(io.frame.fire) { state := State.frameWait } }
    is(State.frameWait) {
      when(io.frameAck.fire) {
        when(io.frameAck.error || io.frameAck.sequence =/= sequence) {
          error := 3
          state := State.acknowledge
        } otherwise { nextCommand() }
      }
    }
    is(State.keys) {
      when(io.memory.command.fire) {
        keyIndex := keyIndex + 1
        when(keyIndex === 8) { nextCommand() }
      }
    }
    is(State.acknowledge) {
      when(io.memory.command.fire) {
        completed := sequence
        state := State.poll
      }
    }
    is(State.delay) {
      delay := delay - 1
      when(delay === 0) { state := State.poll }
    }
  }
  when(fifo.io.push.valid) { assert(fifo.io.push.ready) }
}
