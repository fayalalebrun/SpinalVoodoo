package voodoo.hdmi

import spinal.core._
import spinal.lib._
import voodoo.Config
import voodoo.GenSupport
import voodoo.framebuffer.{FramebufferAddressMath, FramebufferPlaneReader}

case class VideoTiming(
    activeWidth: Int = 640,
    activeHeight: Int = 480,
    hFrontPorch: Int = 16,
    hSync: Int = 96,
    hBackPorch: Int = 48,
    vFrontPorch: Int = 10,
    vSync: Int = 2,
    vBackPorch: Int = 33,
    hSyncActiveHigh: Boolean = false,
    vSyncActiveHigh: Boolean = false
) {
  val hTotal: Int = activeWidth + hFrontPorch + hSync + hBackPorch
  val vTotal: Int = activeHeight + vFrontPorch + vSync + vBackPorch
}

object VideoTiming {
  val dmt640x480: VideoTiming = VideoTiming()

  val cea720x480p: VideoTiming = VideoTiming(
    activeWidth = 720,
    activeHeight = 480,
    hFrontPorch = 16,
    hSync = 62,
    hBackPorch = 60,
    vFrontPorch = 9,
    vSync = 6,
    vBackPorch = 30,
    hSyncActiveHigh = false,
    vSyncActiveHigh = false
  )
}

case class Rgb888() extends Bundle {
  val r = UInt(8 bits)
  val g = UInt(8 bits)
  val b = UInt(8 bits)

  def asBits24: Bits = r.asBits ## g.asBits ## b.asBits
}

object Rgb888 {
  def black(): Rgb888 = {
    val rgb = Rgb888()
    rgb.r := 0
    rgb.g := 0
    rgb.b := 0
    rgb
  }

  def fromRgb565(pixel: Bits): Rgb888 = {
    val rgb = Rgb888()
    rgb.r := (pixel(15 downto 11) ## pixel(15 downto 13)).asUInt
    rgb.g := (pixel(10 downto 5) ## pixel(10 downto 9)).asUInt
    rgb.b := (pixel(4 downto 0) ## pixel(4 downto 2)).asUInt
    rgb
  }

  def applyGamma(rgb: Rgb888, lut: Vec[Bits]): Rgb888 = {
    def interpolate(channel: UInt, high: Int, low: Int): UInt = {
      val index = channel(7 downto 3).resize(log2Up(33))
      val fraction = channel(2 downto 0)
      val a = lut(index)(high downto low).asUInt
      val b = lut(index + 1)(high downto low).asUInt
      val invFraction = U(8, 4 bits) - fraction.resize(4)
      val weighted = (a.resize(12) * invFraction) + (b.resize(12) * fraction.resize(4))
      Mux(channel === 255, b, (weighted >> 3).resize(8))
    }

    val out = Rgb888()
    out.r := interpolate(rgb.r, 23, 16)
    out.g := interpolate(rgb.g, 15, 8)
    out.b := interpolate(rgb.b, 7, 0)
    out
  }
}

case class ParallelRgbVideo() extends Bundle {
  val rgb = Rgb888()
  val de = Bool()
  val hSync = Bool()
  val vSync = Bool()
}

case class HdmiScanoutRegs(c: Config) extends Bundle {
  val frontBase = UInt(c.addressWidth)
  val backBase = UInt(c.addressWidth)
  val pixelStride = UInt(11 bits)
  val displayWidth = UInt(10 bits)
  val displayHeight = UInt(10 bits)
  val framebufferEnable = Bool()
  val testPatternEnable = Bool()
  val gammaLut = Vec(Bits(24 bits), 33)
}

case class HdmiScanoutStatus(c: Config) extends Bundle {
  val displayedBuffer = UInt(1 bits)
  val displayedBase = UInt(c.addressWidth)
  val swapPending = Bool()
  val swapCount = UInt(2 bits)
  val newFrame = Bool()
  val active = Bool()
  val x = UInt(10 bits)
  val y = UInt(10 bits)
}

case class HdmiScanoutPort(c: Config, fifoLevelWidth: Int = 13) extends Bundle with IMasterSlave {
  val clock = Bool()
  val reset = Bool()
  val video = ParallelRgbVideo()
  val status = HdmiScanoutStatus(c)
  val underflow = Bool()
  val fifoLevel = UInt(fifoLevelWidth bits)

  override def asMaster(): Unit = {
    in(clock)
    in(reset)
    out(video)
    out(status)
    out(underflow)
    out(fifoLevel)
  }
}

case class VideoScanout(timing: VideoTiming = VideoTiming()) extends Component {
  private val hWidth = log2Up(timing.hTotal)
  private val vWidth = log2Up(timing.vTotal)

  val io = new Bundle {
    val x = out UInt (hWidth bits)
    val y = out UInt (vWidth bits)
    val active = out Bool ()
    val hSync = out Bool ()
    val vSync = out Bool ()
    val newLine = out Bool ()
    val newFrame = out Bool ()
    val enable = in Bool ()
  }

  val h = Reg(UInt(hWidth bits)) init (0)
  val v = Reg(UInt(vWidth bits)) init (0)

  val atEndOfLine = h === timing.hTotal - 1
  val atEndOfFrame = atEndOfLine && v === timing.vTotal - 1

  when(io.enable) {
    h := h + 1
    when(atEndOfLine) {
      h := 0
      v := v + 1
      when(v === timing.vTotal - 1) {
        v := 0
      }
    }
  } otherwise {
    h := 0
    v := 0
  }

  when(!io.enable) {
    when(h =/= 0 || v =/= 0) {
      v := 0
      h := 0
    }
  }

  val hSyncRaw = h >= timing.activeWidth + timing.hFrontPorch &&
    h < timing.activeWidth + timing.hFrontPorch + timing.hSync
  val vSyncRaw = v >= timing.activeHeight + timing.vFrontPorch &&
    v < timing.activeHeight + timing.vFrontPorch + timing.vSync

  io.x := h
  io.y := v
  io.active := h < timing.activeWidth && v < timing.activeHeight
  io.hSync := (if (timing.hSyncActiveHigh) hSyncRaw else !hSyncRaw)
  io.vSync := (if (timing.vSyncActiveHigh) vSyncRaw else !vSyncRaw)
  io.newLine := atEndOfLine
  io.newFrame := atEndOfFrame
}

case class HdmiTestPattern(timing: VideoTiming = VideoTiming()) extends Component {
  private val hWidth = log2Up(timing.hTotal)
  private val vWidth = log2Up(timing.vTotal)

  val io = new Bundle {
    val x = in UInt (hWidth bits)
    val y = in UInt (vWidth bits)
    val active = in Bool ()
    val rgb = out(Rgb888())
  }

  io.rgb.r := 0
  io.rgb.g := 0
  io.rgb.b := 0

  when(io.active) {
    when(io.x < timing.activeWidth / 4) {
      io.rgb.r := 255
      io.rgb.g := 0
      io.rgb.b := 0
    } elsewhen (io.x < timing.activeWidth / 2) {
      io.rgb.r := 0
      io.rgb.g := 255
      io.rgb.b := 0
    } elsewhen (io.x < timing.activeWidth * 3 / 4) {
      io.rgb.r := 0
      io.rgb.g := 0
      io.rgb.b := 255
    } otherwise {
      val x8 = io.x.resize(8)
      val y8 = io.y.resize(8)
      io.rgb.r := x8
      io.rgb.g := y8
      io.rgb.b := x8 ^ y8
    }
  }
}

case class HdmiFramebufferScanout(c: Config, timing: VideoTiming = VideoTiming.cea720x480p)
    extends Component {
  private val hWidth = log2Up(timing.hTotal)
  private val vWidth = log2Up(timing.vTotal)

  val io = new Bundle {
    val regs = in(HdmiScanoutRegs(c))
    val swap = slave Stream (NoData)
    val readReq = master Stream (FramebufferPlaneReader.ReadReq(c))
    val readRsp = slave Stream (FramebufferPlaneReader.ReadRsp())
    val video = out(ParallelRgbVideo())
    val status = out(HdmiScanoutStatus(c))
  }

  val scan = VideoScanout(timing)
  scan.io.enable := True
  val pattern = HdmiTestPattern(timing)
  pattern.io.x := scan.io.x
  pattern.io.y := scan.io.y
  pattern.io.active := scan.io.active

  val contentActive = scan.io.active &&
    scan.io.x < io.regs.displayWidth.resized &&
    scan.io.y < io.regs.displayHeight.resized

  val displayedBuffer = Reg(UInt(1 bits)) init (0)
  val swapPending = RegInit(False)
  val swapCount = Reg(UInt(2 bits)) init (0)

  io.swap.ready := !swapPending
  when(io.swap.fire) {
    swapPending := True
  }

  when(scan.io.newFrame && swapPending) {
    displayedBuffer := ~displayedBuffer
    swapPending := False
    swapCount := swapCount + 1
  }

  val displayedBase = UInt(c.addressWidth)
  displayedBase := Mux(displayedBuffer(0), io.regs.backBase, io.regs.frontBase)

  io.readReq.valid := io.regs.framebufferEnable && contentActive
  io.readReq.address := FramebufferAddressMath.planeAddress(
    displayedBase,
    scan.io.x.resize(10 bits),
    scan.io.y.resize(10 bits),
    io.regs.pixelStride
  )

  val fbRgb = Reg(Rgb888()) init (Rgb888.black())
  val readRspRgb = Rgb888.fromRgb565(io.readRsp.data)
  io.readRsp.ready := True
  when(io.readRsp.fire) {
    fbRgb := readRspRgb
  }

  val selectedRgb = Rgb888()
  selectedRgb := fbRgb
  when(io.readRsp.valid) {
    selectedRgb := readRspRgb
  }
  when(io.regs.testPatternEnable || !io.regs.framebufferEnable) {
    selectedRgb := pattern.io.rgb
  }

  io.video.rgb := Rgb888.applyGamma(selectedRgb, io.regs.gammaLut)
  when(!contentActive) {
    io.video.rgb := Rgb888.black()
  }
  io.video.de := scan.io.active
  io.video.hSync := scan.io.hSync
  io.video.vSync := scan.io.vSync

  io.status.displayedBuffer := displayedBuffer
  io.status.displayedBase := displayedBase
  io.status.swapPending := swapPending
  io.status.swapCount := swapCount
  io.status.newFrame := scan.io.newFrame
  io.status.active := contentActive
  io.status.x := scan.io.x.resize(10 bits)
  io.status.y := scan.io.y.resize(10 bits)
}

case class HdmiCdcFramebufferScanout(
    c: Config,
    timing: VideoTiming = VideoTiming.cea720x480p,
    fifoDepth: Int = 4096,
    prefillLevel: Int = 2048,
    refillLowLevel: Int = 1024,
    refillHighLevel: Int = 3072
) extends Component {
  require(isPow2(fifoDepth), "HDMI CDC FIFO depth must be a power of two")
  require(prefillLevel > 0 && prefillLevel < fifoDepth)
  require(refillLowLevel > 0 && refillLowLevel < refillHighLevel)
  require(refillHighLevel < fifoDepth)

  private val coreClockDomain = ClockDomain.current
  private val hWidth = log2Up(timing.hTotal)
  private val vWidth = log2Up(timing.vTotal)

  val io = new Bundle {
    val hdmiClock = in Bool ()
    val hdmiReset = in Bool ()
    val regs = in(HdmiScanoutRegs(c))
    val prefetchReq = master Stream (FramebufferPlaneReader.PrefetchReq(c))
    val readReq = master Stream (FramebufferPlaneReader.ReadReq(c))
    val readRsp = slave Stream (FramebufferPlaneReader.ReadRsp())
    val video = out(ParallelRgbVideo())
    val status = out(HdmiScanoutStatus(c))
    val fifoPushOccupancy = out UInt (log2Up(fifoDepth + 1) bits)
    val fifoPopOccupancy = out UInt (log2Up(fifoDepth + 1) bits)
    val underflow = out Bool ()
  }

  val hdmiClockDomain = ClockDomain(
    clock = io.hdmiClock,
    reset = io.hdmiReset,
    config = ClockDomain.current.config
  )

  val pixelFifo = StreamFifoCC(Bits(16 bits), fifoDepth, coreClockDomain, hdmiClockDomain)

  val prefetchX = Reg(UInt(hWidth bits)) init (0)
  val prefetchY = Reg(UInt(vWidth bits)) init (0)
  val prefetchBase = Reg(UInt(c.addressWidth)) init (0)
  val outstanding = Reg(UInt(log2Up(fifoDepth + 1) bits)) init (0)
  val scanoutRun = RegInit(False)
  val refillActive = RegInit(True)
  val linePrefetched = RegInit(False)

  val projected = UInt((log2Up(fifoDepth + 1) + 1) bits)
  projected := (pixelFifo.io.pushOccupancy.resize(projected.getWidth) + outstanding.resize(
    projected.getWidth
  ))

  val displayWidth = io.regs.displayWidth.resize(hWidth)
  val displayHeight = io.regs.displayHeight.resize(vWidth)
  val displayEnabled =
    io.regs.framebufferEnable && io.regs.displayWidth =/= 0 && io.regs.displayHeight =/= 0
  val canPrefetch =
    displayEnabled && refillActive && (projected < U(fifoDepth - 4, projected.getWidth bits))
  val atEndOfDisplayLine = prefetchX === (displayWidth - 1).resized
  val atEndOfDisplayFrame = atEndOfDisplayLine && prefetchY === (displayHeight - 1).resized
  val lineStartAddress = FramebufferAddressMath.planeAddress(
    prefetchBase,
    U(0, 10 bits),
    prefetchY.resize(10 bits),
    io.regs.pixelStride
  )
  val lineEndAddress = FramebufferAddressMath.planeAddress(
    prefetchBase,
    (displayWidth - 1).resize(10 bits),
    prefetchY.resize(10 bits),
    io.regs.pixelStride
  )

  when(!displayEnabled) {
    prefetchX := 0
    prefetchY := 0
    prefetchBase := io.regs.frontBase
    scanoutRun := False
    refillActive := True
    linePrefetched := False
  } otherwise {
    when(!scanoutRun) {
      prefetchBase := io.regs.frontBase
      refillActive := True
      when(pixelFifo.io.pushOccupancy >= prefillLevel) {
        scanoutRun := True
        when(projected >= refillHighLevel) {
          refillActive := False
        }
      }
    } otherwise {
      when(pixelFifo.io.pushOccupancy <= refillLowLevel) {
        refillActive := True
      }
      when(projected >= refillHighLevel) {
        refillActive := False
      }
    }
  }

  io.prefetchReq.valid := displayEnabled && refillActive && !linePrefetched
  io.prefetchReq.startAddress := lineStartAddress
  io.prefetchReq.endAddress := lineEndAddress

  io.readReq.valid := canPrefetch && linePrefetched
  io.readReq.address := FramebufferAddressMath.planeAddress(
    prefetchBase,
    prefetchX.resize(10 bits),
    prefetchY.resize(10 bits),
    io.regs.pixelStride
  )

  when(io.prefetchReq.fire) {
    linePrefetched := True
  }

  when(io.readReq.fire) {
    when(atEndOfDisplayLine) {
      prefetchX := 0
      when(atEndOfDisplayFrame) {
        prefetchY := 0
        prefetchBase := io.regs.frontBase
        linePrefetched := False
      } otherwise {
        prefetchY := prefetchY + 1
        linePrefetched := False
      }
    } otherwise {
      prefetchX := prefetchX + 1
    }
  }

  pixelFifo.io.push.valid := io.readRsp.valid && displayEnabled
  pixelFifo.io.push.payload := io.readRsp.data
  io.readRsp.ready := pixelFifo.io.push.ready

  switch((io.readReq.fire.asUInt ## pixelFifo.io.push.fire.asUInt).asBits) {
    is(B"10") { outstanding := outstanding + 1 }
    is(B"01") { outstanding := outstanding - 1 }
    default {}
  }

  val hdmi = new ClockingArea(hdmiClockDomain) {
    val testPatternEnable = BufferCC(io.regs.testPatternEnable, False)
    val gammaLut = Vec(Bits(24 bits), 33)
    for (i <- 0 until 33) {
      gammaLut(i) := BufferCC(io.regs.gammaLut(i), B(0, 24 bits))
    }
    val contentWidth = BufferCC(io.regs.displayWidth, U(0, 10 bits)).resize(hWidth)
    val contentHeight = BufferCC(io.regs.displayHeight, U(0, 10 bits)).resize(vWidth)
    val scan = VideoScanout(timing)
    scan.io.enable := True
    val pattern = HdmiTestPattern(timing)
    pattern.io.x := scan.io.x
    pattern.io.y := scan.io.y
    pattern.io.active := scan.io.active
    val underflowReg = RegInit(False)
    val contentActive = scan.io.active && scan.io.x < contentWidth && scan.io.y < contentHeight

    val useFramebuffer = !testPatternEnable
    val startArmed = BufferCC(scanoutRun, False)
    val frameAligned = RegInit(False)
    when(!useFramebuffer || !startArmed) {
      frameAligned := False
    } elsewhen (!frameAligned && scan.io.newFrame) {
      frameAligned := True
    }

    pixelFifo.io.pop.ready := useFramebuffer && frameAligned && contentActive
    when(useFramebuffer && frameAligned && contentActive && !pixelFifo.io.pop.valid) {
      underflowReg := True
    }

    val rgb = Rgb888.applyGamma(Rgb888.fromRgb565(pixelFifo.io.pop.payload), gammaLut)
    io.video.rgb := rgb
    when(testPatternEnable) {
      io.video.rgb := pattern.io.rgb
    } elsewhen (!frameAligned || !contentActive || !pixelFifo.io.pop.valid) {
      io.video.rgb := Rgb888.black()
    }
    io.video.de := scan.io.active
    io.video.hSync := scan.io.hSync
    io.video.vSync := scan.io.vSync
  }

  val newFrameCore = PulseCCByToggle(hdmi.scan.io.newFrame, hdmiClockDomain, coreClockDomain)

  io.status.displayedBuffer := 0
  io.status.displayedBase := prefetchBase
  io.status.swapPending := False
  io.status.swapCount := 0
  io.status.newFrame := newFrameCore
  io.status.active := BufferCC(hdmi.contentActive, False)
  io.status.x := BufferCC(hdmi.scan.io.x.resize(10 bits), U(0, 10 bits))
  io.status.y := BufferCC(hdmi.scan.io.y.resize(10 bits), U(0, 10 bits))
  io.fifoPushOccupancy := pixelFifo.io.pushOccupancy
  io.fifoPopOccupancy := BufferCC(pixelFifo.io.popOccupancy, U(0, log2Up(fifoDepth + 1) bits))
  io.underflow := BufferCC(hdmi.underflowReg, False)
}

case class TmdsEncoder() extends Component {
  val io = new Bundle {
    val data = in Bits (8 bits)
    val control = in Bits (2 bits)
    val dataEnable = in Bool ()
    val encoded = out Bits (10 bits)
  }

  val disparity = Reg(SInt(5 bits)) init (0)

  val ones = CountOne(io.data)
  val useXnor = ones > 4 || (ones === 4 && !io.data(0))
  val q = Vec(Bool(), 9)
  q(0) := io.data(0)
  for (i <- 1 until 8) {
    q(i) := Mux(useXnor, !(q(i - 1) ^ io.data(i)), q(i - 1) ^ io.data(i))
  }
  q(8) := !useXnor
  val qBits = q.asBits

  val qOnes = CountOne(qBits(7 downto 0))
  val qBalance = (qOnes.resize(5).asSInt |<< 1) - S(8, 5 bits)
  val nextEncoded = Bits(10 bits)
  val nextDisparity = SInt(5 bits)

  nextEncoded := 0
  nextDisparity := disparity

  when(disparity === 0 || qBalance === 0) {
    nextEncoded(9) := !q(8)
    nextEncoded(8) := q(8)
    nextEncoded(7 downto 0) := Mux(q(8), qBits(7 downto 0), ~qBits(7 downto 0))
    nextDisparity := Mux(q(8), qBalance, -qBalance)
  } elsewhen ((disparity > 0 && qBalance > 0) || (disparity < 0 && qBalance < 0)) {
    nextEncoded(9) := True
    nextEncoded(8) := q(8)
    nextEncoded(7 downto 0) := ~qBits(7 downto 0)
    nextDisparity := disparity + Mux(q(8), S(2, 5 bits), S(0, 5 bits)) - qBalance
  } otherwise {
    nextEncoded(9) := False
    nextEncoded(8) := q(8)
    nextEncoded(7 downto 0) := qBits(7 downto 0)
    nextDisparity := disparity - Mux(q(8), S(0, 5 bits), S(2, 5 bits)) + qBalance
  }

  when(io.dataEnable) {
    io.encoded := nextEncoded
    disparity := nextDisparity
  } otherwise {
    disparity := 0
    switch(io.control) {
      is(B"00") { io.encoded := B"10'b1101010100" }
      is(B"01") { io.encoded := B"10'b0010101011" }
      is(B"10") { io.encoded := B"10'b0101010100" }
      default { io.encoded := B"10'b1010101011" }
    }
  }
}

case class HdmiTestPatternScanout(timing: VideoTiming = VideoTiming()) extends Component {
  val io = new Bundle {
    val active = out Bool ()
    val hSync = out Bool ()
    val vSync = out Bool ()
    val rgb = out(Rgb888())
    val tmds = out(Vec(Bits(10 bits), 3))
  }

  val scan = VideoScanout(timing)
  scan.io.enable := True
  val pattern = HdmiTestPattern(timing)
  pattern.io.x := scan.io.x
  pattern.io.y := scan.io.y
  pattern.io.active := scan.io.active

  val encB = TmdsEncoder()
  val encG = TmdsEncoder()
  val encR = TmdsEncoder()

  encB.io.data := pattern.io.rgb.b.asBits
  encB.io.control := scan.io.vSync.asBits ## scan.io.hSync.asBits
  encB.io.dataEnable := scan.io.active
  encG.io.data := pattern.io.rgb.g.asBits
  encG.io.control := 0
  encG.io.dataEnable := scan.io.active
  encR.io.data := pattern.io.rgb.r.asBits
  encR.io.control := 0
  encR.io.dataEnable := scan.io.active

  io.active := scan.io.active
  io.hSync := scan.io.hSync
  io.vSync := scan.io.vSync
  io.rgb := pattern.io.rgb
  io.tmds(0) := encB.io.encoded
  io.tmds(1) := encG.io.encoded
  io.tmds(2) := encR.io.encoded
}

case class TmdsSerializer() extends Component {
  val io = new Bundle {
    val load = in Bool ()
    val symbols = in(Vec(Bits(10 bits), 3))
    val data = out Bits (3 bits)
    val clock = out Bool ()
  }

  val dataShift = Vec(Reg(Bits(10 bits)) init (0), 3)
  val clockShift = Reg(Bits(10 bits)) init (B"10'b0000011111")

  when(io.load) {
    for (lane <- 0 until 3) {
      dataShift(lane) := io.symbols(lane)
    }
    clockShift := B"10'b0000011111"
  } otherwise {
    for (lane <- 0 until 3) {
      dataShift(lane) := B"0" ## dataShift(lane)(9 downto 1)
    }
    clockShift := B"0" ## clockShift(9 downto 1)
  }

  for (lane <- 0 until 3) {
    io.data(lane) := dataShift(lane)(0)
  }
  io.clock := clockShift(0)
}

object HdmiTestPatternScanoutGen extends App {
  GenSupport.de10Verilog().generate(HdmiTestPatternScanout()).printPruned()
}

object TmdsSerializerGen extends App {
  GenSupport.de10Verilog().generate(TmdsSerializer()).printPruned()
}

object HdmiFramebufferScanoutGen extends App {
  GenSupport.de10Verilog().generate(HdmiFramebufferScanout(Config())).printPruned()
}

object HdmiCdcFramebufferScanoutGen extends App {
  GenSupport.de10Verilog().generate(HdmiCdcFramebufferScanout(Config())).printPruned()
}

object HdmiRtlGen extends App {
  val config = GenSupport.de10Verilog()
  config.generate(HdmiTestPatternScanout()).printPruned()
  config.generate(TmdsSerializer()).printPruned()
  config.generate(HdmiFramebufferScanout(Config())).printPruned()
  config.generate(HdmiCdcFramebufferScanout(Config())).printPruned()
}
