package voodoo.mister

import spinal.core._
import spinal.lib._
import voodoo.hdmi.{ParallelRgbVideo, Rgb888, VideoScanout, VideoTiming}

case class MisterScanout(
    width: Int = 640,
    height: Int = 480,
    timing: VideoTiming = VideoTiming.cea720x480p.copy(activeWidth = 640, hBackPorch = 140)
) extends Component {
  require(width > 0 && height > 0 && width <= timing.activeWidth && height <= timing.activeHeight)
  val io = new Bundle {
    val frame = in(MisterFrame())
    val memory = master(MisterMemoryPort())
    val pixelClock = in Bool ()
    val pixelReset = in Bool ()
    val boundary = out Bool ()
    val newFrame = out Bool ()
    val video = out(ParallelRgbVideo())
    val underflow = out Bool ()
  }
  val coreDomain = ClockDomain.current
  val pixelDomain =
    ClockDomain(io.pixelClock, io.pixelReset, config = ClockDomainConfig(resetKind = SYNC))
  val pixels = StreamFifoCC(Bits(17 bits), 1024, coreDomain, pixelDomain)
  val words = StreamFifo(Bits(64 bits), 16)
  val frame = Reg(MisterFrame()) init (MisterFrame().getZero)
  val x = Reg(UInt(log2Up(width + 1) bits)) init (0)
  val y = Reg(UInt(log2Up(height + 1) bits)) init (0)
  val row = Reg(UInt(32 bits)) init (0)
  val address = Reg(UInt(32 bits)) init (0)
  val outstanding = Reg(UInt(5 bits)) init (0)
  object State extends SpinalEnum { val boundary, latch, pixels = newElement() }
  val state = RegInit(State.boundary)
  val wide = frame.format === B(6, 5 bits)
  val contentWidth = Mux(frame.width > width, U(width, 12 bits), frame.width)
  val inContent = frame.enable && x.resize(12) < contentWidth && y.resize(12) < frame.height
  val endContent = x.resize(12) === contentWidth - 1
  val laneEnd = Mux(wide, address(2), address(2 downto 1) === 3)
  val bytesLeft = ((contentWidth - x.resize(12)).resize(15) << 1).resize(16)
  val spanBytes = Mux(wide, (bytesLeft << 1).resize(16), bytesLeft) + address(2 downto 0).resize(16)
  val neededWords = ((spanBytes + 7) >> 3).resize(13)
  val burst = Mux(neededWords > 16, U(16, 8 bits), neededWords.resize(8))
  io.boundary := state === State.boundary
  io.memory.command.valid := state === State.pixels && inContent &&
    outstanding === 0 && words.io.occupancy === 0
  io.memory.command.address := address & U(BigInt("fffffff8", 16), 32 bits)
  io.memory.command.read := True
  io.memory.command.data := 0
  io.memory.command.mask := 255
  io.memory.command.count := burst
  val response = io.memory.response.stage()
  words.io.push.valid := response.valid
  words.io.push.payload := response.payload
  when(io.memory.command.fire) {
    outstanding := burst.resize(5) - response.valid.asUInt
  } elsewhen (response.valid) { outstanding := outstanding - 1 }
  when(response.valid) {
    assert(words.io.push.ready)
    assert(outstanding =/= 0 || io.memory.command.fire)
  }
  val raw565 = words.io.pop.payload.subdivideIn(16 bits)(address(2 downto 1))
  val raw888 = words.io.pop.payload.subdivideIn(32 bits)(address(2).asUInt)
  val rgb565 = Mux(wide, raw888(23 downto 19) ## raw888(15 downto 10) ## raw888(7 downto 3), raw565)
  pixels.io.push.valid := state === State.pixels && (!inContent || words.io.pop.valid)
  pixels.io.push.payload := (x === 0 && y === 0).asBits ## Mux(inContent, rgb565, B(0, 16 bits))
  words.io.pop.ready := pixels.io.push.fire && inContent && (laneEnd || endContent)
  when(state === State.boundary) {
    assert(outstanding === 0 && words.io.occupancy === 0)
    state := State.latch
  }
  when(state === State.latch) {
    frame := io.frame
    row := io.frame.base
    address := io.frame.base
    x := 0
    y := 0
    state := State.pixels
  }
  when(pixels.io.push.fire) {
    when(inContent) { address := address + Mux(wide, U(4, 32 bits), U(2, 32 bits)) }
    when(x === width - 1) {
      x := 0
      row := row + frame.stride.resize(32)
      address := row + frame.stride.resize(32)
      when(y === height - 1) { state := State.boundary } otherwise { y := y + 1 }
    } otherwise { x := x + 1 }
  }
  val pixel = new ClockingArea(pixelDomain) {
    val scan = VideoScanout(timing)
    scan.io.enable := True
    val aligned = RegInit(False)
    val underflow = RegInit(False)
    val content = scan.io.active && scan.io.x < width && scan.io.y < height
    val first = scan.io.x === 0 && scan.io.y === 0
    val unexpectedStart = pixels.io.pop.payload(16) && !first
    val available = pixels.io.pop.valid && !unexpectedStart
    when(scan.io.newFrame) { aligned := pixels.io.pop.valid && pixels.io.pop.payload(16) }
    when(aligned && content && !available) {
      aligned := False
      underflow := True
    }
    pixels.io.pop.ready := Mux(aligned, content && available, !pixels.io.pop.payload(16))
    io.video.rgb := Rgb888.fromRgb565(pixels.io.pop.payload(15 downto 0))
    when(!aligned || !content || !available) { io.video.rgb := Rgb888.black() }
    io.video.de := scan.io.active
    io.video.hSync := scan.io.hSync
    io.video.vSync := scan.io.vSync
  }
  io.newFrame := PulseCCByToggle(pixel.scan.io.newFrame, pixelDomain, coreDomain)
  io.underflow := BufferCC(pixel.underflow, False)
}
