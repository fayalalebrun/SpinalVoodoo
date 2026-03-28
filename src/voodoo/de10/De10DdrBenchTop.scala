package voodoo.de10

import spinal.core._
import spinal.lib._
import spinal.lib.bus.avalon._
import voodoo.GenSupport

object De10DdrBenchAddressMap {
  val mmioAddressWidth = 24
  val mmioWordAddressWidth = mmioAddressWidth - 2

  val ident = 0x000
  val version = 0x004
  val capability = 0x008
  val control = 0x00c
  val status = 0x010
  val uptimeCyclesLo = 0x014
  val uptimeCyclesHi = 0x018

  val portStride = 0x80
  val fbPortBase = 0x100
  val texPortBase = fbPortBase + portStride

  val portControl = 0x00
  val portBaseAddr = 0x04
  val portAddressMask = 0x08
  val portStrideBytes = 0x0c
  val portTransferCount = 0x10
  val portSeed = 0x14
  val portStatus = 0x18
  val portCurrentAddress = 0x1c
  val portIssuedCount = 0x20
  val portCompletedCount = 0x24
  val portBytesLo = 0x28
  val portBytesHi = 0x2c
  val portWaitCyclesLo = 0x30
  val portWaitCyclesHi = 0x34
  val portActiveCyclesLo = 0x38
  val portActiveCyclesHi = 0x3c
  val portReadLatencyMin = 0x40
  val portReadLatencyMax = 0x44
  val portReadLatencySumLo = 0x48
  val portReadLatencySumHi = 0x4c
  val portReadSamples = 0x50
  val portLastReadData = 0x54
  val portBurstWords = 0x58

  val identValue = 0x44444231L
  val versionValue = 0x00010000L
  val capabilityValue =
    (50 << 24) | (De10MemBackend.avalonBurstCountWidth << 16) | (De10MemBackend.physicalAddressWidth << 8) | 32
}

object De10DdrBenchPortControl {
  val enable = 0
  val readEnable = 1
  val writeEnable = 2
  val randomAddress = 3
  val continuous = 4
}

object De10DdrBenchGlobalControl {
  val start = 0
  val stop = 1
  val clear = 2
}

case class De10DdrBenchPort(
    defaultBase: BigInt,
    defaultMask: BigInt,
    mem: AvalonMM,
    globalCycles: UInt
) extends Area {
  private val fullByteEnable = B"4'xF"
  private val addressAlignMask = U(0xfffffffcL, 32 bits)

  val control = Reg(Bits(32 bits)) init (0)
  val baseAddr = Reg(UInt(32 bits)) init (U(defaultBase, 32 bits))
  val addressMask = Reg(UInt(32 bits)) init (U(defaultMask, 32 bits))
  val strideBytes = Reg(UInt(32 bits)) init (4)
  val transferCount = Reg(UInt(32 bits)) init (1024)
  val seed = Reg(UInt(32 bits)) init (U((defaultBase ^ 0x13579bdfL) & 0xffffffffL, 32 bits))
  val burstWords = Reg(UInt(32 bits)) init (1)

  val running = RegInit(False)
  val done = RegInit(False)
  val readPending = RegInit(False)
  val readLatencyPending = RegInit(False)
  val currentOffset = Reg(UInt(32 bits)) init (0)
  val issuedRemaining = Reg(UInt(32 bits)) init (0)
  val lfsr = Reg(UInt(32 bits)) init (1)
  val nextWrite = RegInit(False)
  val readIssueCycle = Reg(UInt(64 bits)) init (0)
  val readBurstBeatsRemaining = Reg(UInt(32 bits)) init (0)
  val currentAddress = Reg(UInt(32 bits)) init (U(defaultBase, 32 bits))

  val issuedCount = Reg(UInt(32 bits)) init (0)
  val completedCount = Reg(UInt(32 bits)) init (0)
  val bytesTransferred = Reg(UInt(64 bits)) init (0)
  val waitCycles = Reg(UInt(64 bits)) init (0)
  val activeCycles = Reg(UInt(64 bits)) init (0)
  val readLatencyMinRaw = Reg(UInt(32 bits)) init (U(0xffffffffL, 32 bits))
  val readLatencyMax = Reg(UInt(32 bits)) init (0)
  val readLatencySum = Reg(UInt(64 bits)) init (0)
  val readSamples = Reg(UInt(32 bits)) init (0)
  val lastReadData = Reg(Bits(32 bits)) init (0)

  private val enable = control(De10DdrBenchPortControl.enable)
  private val readEnable = control(De10DdrBenchPortControl.readEnable)
  private val writeEnable = control(De10DdrBenchPortControl.writeEnable)
  private val randomAddress = control(De10DdrBenchPortControl.randomAddress)
  private val continuous = control(De10DdrBenchPortControl.continuous)
  private val operationValid = enable && (readEnable || writeEnable)
  private val alignedMask = addressMask & addressAlignMask
  private val alignedStride =
    Mux(strideBytes(31 downto 2) === 0, U(4, 32 bits), strideBytes & addressAlignMask)
  private val clampedBurstWords = Mux(
    burstWords === 0,
    U(1, 32 bits),
    Mux(
      burstWords > U((1 << De10MemBackend.avalonBurstCountWidth) - 1, 32 bits),
      U((1 << De10MemBackend.avalonBurstCountWidth) - 1, 32 bits),
      burstWords
    )
  )
  private val maskedOffset = currentOffset & alignedMask
  private val addressNow = (baseAddr + maskedOffset).resized
  private val writeData = (seed ^ addressNow ^ issuedCount.resize(32)).asBits

  private def nextLfsrValue(value: UInt): UInt = {
    val feedback = value(31) ^ value(21) ^ value(1) ^ value(0)
    ((value(30 downto 0) ## feedback).asUInt | U(1, 32 bits))
  }

  def clearStats(): Unit = {
    issuedCount := 0
    completedCount := 0
    bytesTransferred := 0
    waitCycles := 0
    activeCycles := 0
    readLatencyMinRaw := U(0xffffffffL, 32 bits)
    readLatencyMax := 0
    readLatencySum := 0
    readSamples := 0
    lastReadData := 0
  }

  def resetRunState(): Unit = {
    running := False
    done := False
    readPending := False
    readLatencyPending := False
    currentOffset := 0
    issuedRemaining := 0
    lfsr := seed | U(1, 32 bits)
    nextWrite := False
    readIssueCycle := 0
    readBurstBeatsRemaining := 0
    currentAddress := baseAddr
  }

  def startRun(): Unit = {
    val shouldRun = operationValid && (continuous || transferCount =/= 0)
    running := shouldRun
    done := !shouldRun
    readPending := False
    readLatencyPending := False
    currentOffset := 0
    issuedRemaining := transferCount
    lfsr := seed | U(1, 32 bits)
    nextWrite := False
    readIssueCycle := 0
    readBurstBeatsRemaining := 0
    currentAddress := baseAddr
  }

  def stopRun(): Unit = {
    running := False
    done := operationValid
    readPending := False
    readLatencyPending := False
    issuedRemaining := 0
    readBurstBeatsRemaining := 0
  }

  mem.address := addressNow
  mem.read := False
  mem.write := False
  mem.burstCount := 1
  mem.byteEnable := fullByteEnable
  mem.writeData := writeData

  when(running) {
    activeCycles := activeCycles + 1
  }

  val hasCommandsLeft = continuous || (issuedRemaining =/= 0)
  val issueWrite = writeEnable && (!readEnable || nextWrite)
  val readRequestWords = Mux(
    continuous,
    clampedBurstWords,
    Mux(issuedRemaining < clampedBurstWords, issuedRemaining, clampedBurstWords)
  )
  val requestWords = Mux(issueWrite, U(1, 32 bits), readRequestWords)
  val requestFire = running && !readPending && hasCommandsLeft
  val requestAccepted = requestFire && mem.waitRequestn

  when(requestFire) {
    currentAddress := addressNow
    when(issueWrite) {
      mem.write := True
      mem.burstCount := 1
    } otherwise {
      mem.read := True
      mem.burstCount := readRequestWords.resize(De10MemBackend.avalonBurstCountWidth)
    }

    when(!mem.waitRequestn) {
      waitCycles := waitCycles + 1
    }
  }

  when(requestAccepted) {
    issuedCount := issuedCount + 1
    when(randomAddress) {
      val advanced = nextLfsrValue(lfsr)
      lfsr := advanced
      currentOffset := ((advanced |<< 2).resize(32)) & alignedMask
    } otherwise {
      val requestStride = (alignedStride * requestWords).resize(32)
      val nextOffset = (currentOffset + requestStride) & alignedMask
      currentOffset := nextOffset
    }

    when(!continuous && issuedRemaining =/= 0) {
      issuedRemaining := issuedRemaining - requestWords
    }

    when(readEnable && writeEnable) {
      nextWrite := !nextWrite
    }

    when(issueWrite) {
      completedCount := completedCount + 1
      bytesTransferred := bytesTransferred + U(4, 64 bits)
      when(!continuous && issuedRemaining === 1) {
        running := False
        done := True
      }
    } otherwise {
      readPending := True
      readLatencyPending := True
      readIssueCycle := globalCycles
      readBurstBeatsRemaining := readRequestWords
    }
  }

  when(readPending && mem.readDataValid) {
    val latency = (globalCycles - readIssueCycle).resized
    bytesTransferred := bytesTransferred + U(4, 64 bits)
    lastReadData := mem.readData

    when(readLatencyPending) {
      readLatencyPending := False
      readLatencySum := readLatencySum + latency.resize(64)
      readSamples := readSamples + 1
      when(readSamples === 0 || latency.resize(32) < readLatencyMinRaw) {
        readLatencyMinRaw := latency.resize(32)
      }
      when(latency.resize(32) > readLatencyMax) {
        readLatencyMax := latency.resize(32)
      }
    }

    when(readBurstBeatsRemaining === 1) {
      readPending := False
      completedCount := completedCount + 1
      when(!continuous && issuedRemaining === 0) {
        running := False
        done := True
      }
    }.otherwise {
      readBurstBeatsRemaining := readBurstBeatsRemaining - 1
    }
  }

  def statusWord: Bits = {
    val bits = Bits(32 bits)
    bits := 0
    bits(0) := running
    bits(1) := done
    bits(2) := readPending
    bits(3) := enable
    bits(4) := readEnable
    bits(5) := writeEnable
    bits(6) := randomAddress
    bits(7) := continuous
    bits
  }

  def readLatencyMinVisible: Bits = Mux(readSamples === 0, B(0, 32 bits), readLatencyMinRaw.asBits)
}

case class De10DdrBenchTop() extends Component {
  import De10DdrBenchAddressMap._

  val io = new Bundle {
    val h2fLw = slave(H2fLwMmio(mmioWordAddressWidth))
    val memFb = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
    val memTex = master(AvalonMM(De10MemBackend.avalonConfig(De10MemBackend.physicalAddressWidth)))
  }

  val uptimeCycles = Reg(UInt(64 bits)) init (0)
  uptimeCycles := uptimeCycles + 1

  val startPulse = Bool()
  val stopPulse = Bool()
  val clearPulse = Bool()
  startPulse := False
  stopPulse := False
  clearPulse := False

  val fb =
    De10DdrBenchPort(De10AddressMap.fbMemBase, De10AddressMap.fbMemMask, io.memFb, uptimeCycles)
  val tex =
    De10DdrBenchPort(De10AddressMap.texMemBase, De10AddressMap.texMemMask, io.memTex, uptimeCycles)

  when(clearPulse) {
    fb.clearStats()
    fb.resetRunState()
    tex.clearStats()
    tex.resetRunState()
  }

  when(stopPulse) {
    fb.stopRun()
    tex.stopRun()
  }

  when(startPulse) {
    fb.startRun()
    tex.startRun()
  }

  val readPending = RegInit(False)
  val readData = Reg(Bits(32 bits)) init (0)
  val reqSeen = RegInit(False)
  val hasReq = io.h2fLw.read || io.h2fLw.write
  val acceptReq = hasReq && !reqSeen && !readPending

  io.h2fLw.waitrequest := hasReq && !acceptReq
  io.h2fLw.readdatavalid := readPending
  io.h2fLw.readdata := readData

  readPending := False
  when(!hasReq) {
    reqSeen := False
  }

  def writeReg(reg: UInt): Unit = {
    reg := io.h2fLw.writedata.asUInt
  }

  def writeBits(reg: Bits): Unit = {
    reg := io.h2fLw.writedata
  }

  when(acceptReq) {
    reqSeen := True
    val wordAddress = io.h2fLw.address.resize(32)
    val fbLocalWord = wordAddress - U(fbPortBase >> 2, 32 bits)
    val texLocalWord = wordAddress - U(texPortBase >> 2, 32 bits)

    when(io.h2fLw.write) {
      switch(wordAddress) {
        is(U(control >> 2, 32 bits)) {
          startPulse := io.h2fLw.writedata(De10DdrBenchGlobalControl.start)
          stopPulse := io.h2fLw.writedata(De10DdrBenchGlobalControl.stop)
          clearPulse := io.h2fLw.writedata(De10DdrBenchGlobalControl.clear)
        }
        default {
          when(
            wordAddress >= U(fbPortBase >> 2, 32 bits) && wordAddress < U(
              (fbPortBase + portStride) >> 2,
              32 bits
            )
          ) {
            switch(fbLocalWord) {
              is(U(portControl >> 2, 32 bits)) { writeBits(fb.control) }
              is(U(portBaseAddr >> 2, 32 bits)) { writeReg(fb.baseAddr) }
              is(U(portAddressMask >> 2, 32 bits)) { writeReg(fb.addressMask) }
              is(U(portStrideBytes >> 2, 32 bits)) { writeReg(fb.strideBytes) }
              is(U(portTransferCount >> 2, 32 bits)) { writeReg(fb.transferCount) }
              is(U(portSeed >> 2, 32 bits)) { writeReg(fb.seed) }
              is(U(portBurstWords >> 2, 32 bits)) { writeReg(fb.burstWords) }
            }
          } elsewhen (wordAddress >= U(texPortBase >> 2, 32 bits) && wordAddress < U(
            (texPortBase + portStride) >> 2,
            32 bits
          )) {
            switch(texLocalWord) {
              is(U(portControl >> 2, 32 bits)) { writeBits(tex.control) }
              is(U(portBaseAddr >> 2, 32 bits)) { writeReg(tex.baseAddr) }
              is(U(portAddressMask >> 2, 32 bits)) { writeReg(tex.addressMask) }
              is(U(portStrideBytes >> 2, 32 bits)) { writeReg(tex.strideBytes) }
              is(U(portTransferCount >> 2, 32 bits)) { writeReg(tex.transferCount) }
              is(U(portSeed >> 2, 32 bits)) { writeReg(tex.seed) }
              is(U(portBurstWords >> 2, 32 bits)) { writeReg(tex.burstWords) }
            }
          }
        }
      }
    } otherwise {
      readPending := True
      switch(wordAddress) {
        is(U(ident >> 2, 32 bits)) {
          readData := B(identValue, 32 bits)
        }
        is(U(version >> 2, 32 bits)) {
          readData := B(versionValue, 32 bits)
        }
        is(U(capability >> 2, 32 bits)) {
          readData := B(capabilityValue, 32 bits)
        }
        is(U(status >> 2, 32 bits)) {
          val bits = Bits(32 bits)
          bits := 0
          bits(0) := fb.running || tex.running
          bits(1) := !fb.running && !tex.running && (fb.done || !fb.control(
            De10DdrBenchPortControl.enable
          )) && (tex.done || !tex.control(De10DdrBenchPortControl.enable))
          bits(8) := fb.running
          bits(9) := fb.done
          bits(10) := tex.running
          bits(11) := tex.done
          readData := bits
        }
        is(U(uptimeCyclesLo >> 2, 32 bits)) {
          readData := uptimeCycles(31 downto 0).asBits
        }
        is(U(uptimeCyclesHi >> 2, 32 bits)) {
          readData := uptimeCycles(63 downto 32).asBits
        }
        default {
          when(
            wordAddress >= U(fbPortBase >> 2, 32 bits) && wordAddress < U(
              (fbPortBase + portStride) >> 2,
              32 bits
            )
          ) {
            switch(fbLocalWord) {
              is(U(portControl >> 2, 32 bits)) { readData := fb.control }
              is(U(portBaseAddr >> 2, 32 bits)) { readData := fb.baseAddr.asBits }
              is(U(portAddressMask >> 2, 32 bits)) { readData := fb.addressMask.asBits }
              is(U(portStrideBytes >> 2, 32 bits)) { readData := fb.strideBytes.asBits }
              is(U(portTransferCount >> 2, 32 bits)) { readData := fb.transferCount.asBits }
              is(U(portSeed >> 2, 32 bits)) { readData := fb.seed.asBits }
              is(U(portBurstWords >> 2, 32 bits)) { readData := fb.burstWords.asBits }
              is(U(portStatus >> 2, 32 bits)) { readData := fb.statusWord }
              is(U(portCurrentAddress >> 2, 32 bits)) { readData := fb.currentAddress.asBits }
              is(U(portIssuedCount >> 2, 32 bits)) { readData := fb.issuedCount.asBits }
              is(U(portCompletedCount >> 2, 32 bits)) { readData := fb.completedCount.asBits }
              is(U(portBytesLo >> 2, 32 bits)) {
                readData := fb.bytesTransferred(31 downto 0).asBits
              }
              is(U(portBytesHi >> 2, 32 bits)) {
                readData := fb.bytesTransferred(63 downto 32).asBits
              }
              is(U(portWaitCyclesLo >> 2, 32 bits)) {
                readData := fb.waitCycles(31 downto 0).asBits
              }
              is(U(portWaitCyclesHi >> 2, 32 bits)) {
                readData := fb.waitCycles(63 downto 32).asBits
              }
              is(U(portActiveCyclesLo >> 2, 32 bits)) {
                readData := fb.activeCycles(31 downto 0).asBits
              }
              is(U(portActiveCyclesHi >> 2, 32 bits)) {
                readData := fb.activeCycles(63 downto 32).asBits
              }
              is(U(portReadLatencyMin >> 2, 32 bits)) { readData := fb.readLatencyMinVisible }
              is(U(portReadLatencyMax >> 2, 32 bits)) { readData := fb.readLatencyMax.asBits }
              is(U(portReadLatencySumLo >> 2, 32 bits)) {
                readData := fb.readLatencySum(31 downto 0).asBits
              }
              is(U(portReadLatencySumHi >> 2, 32 bits)) {
                readData := fb.readLatencySum(63 downto 32).asBits
              }
              is(U(portReadSamples >> 2, 32 bits)) { readData := fb.readSamples.asBits }
              is(U(portLastReadData >> 2, 32 bits)) { readData := fb.lastReadData }
              default { readData := 0 }
            }
          } elsewhen (wordAddress >= U(texPortBase >> 2, 32 bits) && wordAddress < U(
            (texPortBase + portStride) >> 2,
            32 bits
          )) {
            switch(texLocalWord) {
              is(U(portControl >> 2, 32 bits)) { readData := tex.control }
              is(U(portBaseAddr >> 2, 32 bits)) { readData := tex.baseAddr.asBits }
              is(U(portAddressMask >> 2, 32 bits)) { readData := tex.addressMask.asBits }
              is(U(portStrideBytes >> 2, 32 bits)) { readData := tex.strideBytes.asBits }
              is(U(portTransferCount >> 2, 32 bits)) { readData := tex.transferCount.asBits }
              is(U(portSeed >> 2, 32 bits)) { readData := tex.seed.asBits }
              is(U(portBurstWords >> 2, 32 bits)) { readData := tex.burstWords.asBits }
              is(U(portStatus >> 2, 32 bits)) { readData := tex.statusWord }
              is(U(portCurrentAddress >> 2, 32 bits)) { readData := tex.currentAddress.asBits }
              is(U(portIssuedCount >> 2, 32 bits)) { readData := tex.issuedCount.asBits }
              is(U(portCompletedCount >> 2, 32 bits)) { readData := tex.completedCount.asBits }
              is(U(portBytesLo >> 2, 32 bits)) {
                readData := tex.bytesTransferred(31 downto 0).asBits
              }
              is(U(portBytesHi >> 2, 32 bits)) {
                readData := tex.bytesTransferred(63 downto 32).asBits
              }
              is(U(portWaitCyclesLo >> 2, 32 bits)) {
                readData := tex.waitCycles(31 downto 0).asBits
              }
              is(U(portWaitCyclesHi >> 2, 32 bits)) {
                readData := tex.waitCycles(63 downto 32).asBits
              }
              is(U(portActiveCyclesLo >> 2, 32 bits)) {
                readData := tex.activeCycles(31 downto 0).asBits
              }
              is(U(portActiveCyclesHi >> 2, 32 bits)) {
                readData := tex.activeCycles(63 downto 32).asBits
              }
              is(U(portReadLatencyMin >> 2, 32 bits)) { readData := tex.readLatencyMinVisible }
              is(U(portReadLatencyMax >> 2, 32 bits)) { readData := tex.readLatencyMax.asBits }
              is(U(portReadLatencySumLo >> 2, 32 bits)) {
                readData := tex.readLatencySum(31 downto 0).asBits
              }
              is(U(portReadLatencySumHi >> 2, 32 bits)) {
                readData := tex.readLatencySum(63 downto 32).asBits
              }
              is(U(portReadSamples >> 2, 32 bits)) { readData := tex.readSamples.asBits }
              is(U(portLastReadData >> 2, 32 bits)) { readData := tex.lastReadData }
              default { readData := 0 }
            }
          } otherwise {
            readData := 0
          }
        }
      }
    }
  }
}

object De10DdrBenchTopGen extends App {
  // Reuse the existing DE10 board wrapper/Qsys wiring by emitting the bench as De10Top.
  GenSupport.de10Verilog().generate(De10DdrBenchTop().setDefinitionName("De10Top"))
}
