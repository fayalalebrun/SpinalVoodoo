package voodoo.mister

import spinal.core._
import spinal.lib._
import spinal.lib.bus.avalon._
import voodoo.de10.De10MemBackend

case class MisterMemoryCommand() extends Bundle {
  val address = UInt(32 bits)
  val read = Bool()
  val data = Bits(64 bits)
  val mask = Bits(8 bits)
  val count = UInt(8 bits)
}

case class MisterMemoryPort() extends Bundle with IMasterSlave {
  val command = Stream(MisterMemoryCommand())
  val response = Flow(Bits(64 bits))
  override def asMaster(): Unit = {
    master(command)
    slave(response)
  }
}

case class MisterDdrPort() extends Bundle with IMasterSlave {
  val busy = Bool()
  val address = UInt(29 bits)
  val burstCount = UInt(8 bits)
  val read = Bool()
  val write = Bool()
  val writeData = Bits(64 bits)
  val byteEnable = Bits(8 bits)
  val readData = Bits(64 bits)
  val readDataValid = Bool()
  override def asMaster(): Unit = {
    out(address, burstCount, read, write, writeData, byteEnable)
    in(busy, readData, readDataValid)
  }
}

case class MisterMemoryArbiter(portCount: Int, readSlots: Int = 8) extends Component {
  require(portCount >= 2)
  require(readSlots >= 1 && readSlots <= 8)
  val io = new Bundle {
    val ports = Vec(slave(MisterMemoryPort()), portCount)
    val ddr = master(MisterDdrPort())
  }
  val routeWidth = log2Up(portCount)
  val requests = io.ports.zipWithIndex.map { case (port, index) =>
    port.command.translateWith(U(index, routeWidth bits) ## port.command.payload.asBits)
  }
  val request = StreamArbiterFactory.roundRobin.transactionLock.on(requests)
  val command = MisterMemoryCommand()
  command.assignFromBits(request.payload(command.getBitsWidth - 1 downto 0))
  val route = request.payload(request.payload.getWidth - 1 downto command.getBitsWidth).asUInt
  val routes = StreamFifo(Bits(routeWidth + 8 bits), readSlots, latency = 1)
  val remaining = Reg(UInt(8 bits)) init (0)
  val launchRead = request.fire && command.read
  val immediate = !routes.io.pop.valid && io.ddr.readDataValid
  val beats = Mux(remaining === 0, routes.io.pop.payload(7 downto 0).asUInt, remaining)
  val responseOwner =
    Mux(routes.io.pop.valid, routes.io.pop.payload(routeWidth + 7 downto 8).asUInt, route)
  routes.io.push.valid := launchRead && !(immediate && command.count === 1)
  routes.io.push.payload := route.asBits ## (command.count - immediate.asUInt).asBits
  routes.io.pop.ready := io.ddr.readDataValid && beats === 1

  request.ready := (!command.read || routes.io.push.ready) && !io.ddr.busy
  io.ddr.address := command.address(31 downto 3)
  io.ddr.burstCount := command.count
  io.ddr.read := request.valid && routes.io.push.ready && command.read
  io.ddr.write := request.valid && !command.read
  io.ddr.writeData := command.data
  io.ddr.byteEnable := command.mask
  for ((port, index) <- io.ports.zipWithIndex) {
    port.response.valid := io.ddr.readDataValid && (routes.io.pop.valid || launchRead) &&
      responseOwner === index
    port.response.payload := io.ddr.readData
  }
  when(io.ddr.readDataValid && routes.io.pop.valid) {
    remaining := Mux(beats === 1, U(0, 8 bits), beats - 1)
  }
  when(request.fire) {
    assert(command.address(2 downto 0) === 0)
    assert(command.count =/= 0 && command.count <= 128)
    assert(command.read || command.count === 1)
  }
  when(io.ddr.readDataValid) {
    assert(routes.io.pop.valid || launchRead)
  }
}

case class MisterAvalonToDdr() extends Component {
  val io = new Bundle {
    val avalon = slave(AvalonMM(De10MemBackend.avalonConfig(32)))
    val memory = master(MisterMemoryPort())
  }
  object State extends SpinalEnum {
    val idle, write, issue, drain = newElement()
  }
  val state = RegInit(State.idle)
  val address = Reg(UInt(32 bits)) init (0)
  val remaining = Reg(UInt(11 bits)) init (0)
  val chunkRemaining = Reg(UInt(9 bits)) init (0)
  val highHalf = RegInit(False)
  val data = StreamFifo(Bits(64 bits), 128)
  data.io.push.valid := io.memory.response.valid
  data.io.push.payload := io.memory.response.payload
  val maxChunk = U(256, 12 bits) - address(2).asUInt.resize(12)
  val chunk = Mux(remaining.resize(12) > maxChunk, maxChunk, remaining.resize(12))
  val words64 = ((chunk + address(2).asUInt.resize(12) + 1) >> 1).resize(8)
  val aligned = io.avalon.address(1 downto 0) === 0
  val legal = aligned && io.avalon.burstCount =/= 0 && io.avalon.burstCount <= 1024 &&
    (io.avalon.read ^ io.avalon.write)

  io.memory.command.valid := state === State.issue ||
    (state === State.write && io.avalon.write && !io.avalon.read)
  io.memory.command.address := address & U(BigInt("fffffff8", 16), 32 bits)
  io.memory.command.read := state === State.issue
  io.memory.command.count := Mux(state === State.issue, words64, U(1, 8 bits))
  io.memory.command.data := Mux(
    address(2),
    io.avalon.writeData ## B(0, 32 bits),
    B(0, 32 bits) ## io.avalon.writeData
  )
  io.memory.command.mask := Mux(
    address(2),
    io.avalon.byteEnable ## B(0, 4 bits),
    B(0, 4 bits) ## io.avalon.byteEnable
  )
  io.avalon.waitRequestn := (state === State.idle && legal && io.avalon.read) ||
    (state === State.write && io.memory.command.ready && io.avalon.write && !io.avalon.read)
  io.avalon.readDataValid := state === State.drain && data.io.pop.valid
  io.avalon.readData := Mux(
    highHalf,
    data.io.pop.payload(63 downto 32),
    data.io.pop.payload(31 downto 0)
  )
  data.io.pop.ready := state === State.drain && (highHalf || chunkRemaining === 1)

  when(state === State.idle && legal) {
    address := io.avalon.address
    remaining := io.avalon.burstCount
    state := Mux(io.avalon.read, State.issue, State.write)
  }
  when(state === State.issue && io.memory.command.fire) {
    chunkRemaining := chunk.resized
    highHalf := address(2)
    state := State.drain
  }
  when(state === State.drain && data.io.pop.valid) {
    address := address + 4
    remaining := remaining - 1
    chunkRemaining := chunkRemaining - 1
    highHalf := !highHalf
    when(chunkRemaining === 1) {
      state := Mux(remaining === 1, State.idle, State.issue)
    }
  }
  when(state === State.write && io.memory.command.fire) {
    address := address + 4
    remaining := remaining - 1
    when(remaining === 1) {
      state := State.idle
    }
  }
  when(io.memory.response.valid) {
    assert(data.io.push.ready)
  }
}
