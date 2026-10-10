package voodoo.de10

import spinal.core._
import spinal.lib._

case class XlsAvalonResponseMeta() extends Bundle {
  val kind = UInt(2 bits)
  val count = UInt(7 bits)
  val writeData = Bits(32 bits)
}

case class XlsAvalonReadMeta() extends Bundle {
  val count = UInt(7 bits)
}

case class XlsAvalonTransferMasterCore(
    requestDepth: Int = 32,
    responseDepth: Int = 32,
    readDepth: Int = 32
) extends Component {
  require(requestDepth > 0)
  require(responseDepth > 0)
  require(readDepth > 0)

  setDefinitionName("XlsAvalonTransferMasterCore")

  val request = in Bits (66 bits)
  val request_valid = in Bool ()
  val request_ready = out Bool ()
  val response = out Bits (33 bits)
  val response_valid = out Bool ()
  val response_ready = in Bool ()
  val mem_read = out Bool ()
  val mem_write = out Bool ()
  val mem_waitRequestn = in Bool ()
  val mem_burstCount = out UInt (11 bits)
  val mem_address = out UInt (32 bits)
  val mem_byteEnable = out Bits (4 bits)
  val mem_writeData = out Bits (32 bits)
  val mem_readDataValid = in Bool ()
  val mem_readData = in Bits (32 bits)
  val base_address = in UInt (32 bits)
  val span_words = in UInt (23 bits)
  val wrap_burst64 = in Bool ()

  val failureKind = U(0, 2 bits)
  val writeKind = U(1, 2 bits)
  val scalarReadKind = U(2, 2 bits)
  val burstReadKind = U(3, 2 bits)

  val requestFifo = StreamFifo(Bits(66 bits), requestDepth)
  requestFifo.io.push.valid := request_valid
  requestFifo.io.push.payload := request
  request_ready := requestFifo.io.push.ready

  val responseMetaFifo = StreamFifo(XlsAvalonResponseMeta(), responseDepth)
  val readMetaFifo = new StreamFifo(XlsAvalonReadMeta(), readDepth, withAsyncRead = true)
  val scalarDataFifo = StreamFifo(Bits(32 bits), readDepth)

  val command = requestFifo.io.pop.payload
  val commandCount = command(65 downto 59).asUInt
  val commandWrite = command(58)
  val commandAddress = command(57 downto 36).asUInt
  val commandData = command(35 downto 4)
  val commandMask = command(3 downto 0)
  val commandOffset = commandAddress(5 downto 0)
  val commandWrap = wrap_burst64 && !commandWrite && commandCount === 64 && commandOffset =/= 0
  val commandEnd = Mux(
    commandWrap,
    (commandAddress & U(0x3fffc0, 22 bits)).resize(23) + U(64, 23 bits),
    commandAddress.resize(23) + commandCount.resize(23)
  )
  val commandInRange =
    commandCount =/= 0 && commandCount <= 64 &&
      commandAddress.resize(23) < span_words && commandEnd <= span_words &&
      (!commandWrite || commandCount === 1)
  val commandIsRead = commandInRange && !commandWrite
  val commandIsScalarRead = commandIsRead && commandCount === 1
  val commandIsBurstRead = commandIsRead && commandCount =/= 1

  val scalarReservations = Reg(UInt(log2Up(readDepth + 1) bits)) init (0)
  val burstReservations = Reg(UInt(8 bits)) init (0)
  val burstDataFifo = StreamFifo(Bits(32 bits), 128)
  val responseBeat = Reg(UInt(7 bits)) init (0)

  val responseHead = responseMetaFifo.io.pop.payload
  val responseIsFailure = responseHead.kind === failureKind
  val responseIsWrite = responseHead.kind === writeKind
  val responseIsScalarRead = responseHead.kind === scalarReadKind
  val responseIsBurstRead = responseHead.kind === burstReadKind
  val responsePayloadReady =
    responseIsFailure || responseIsWrite ||
      (responseIsScalarRead && scalarDataFifo.io.pop.valid) ||
      (responseIsBurstRead && burstDataFifo.io.pop.valid)
  val responseLast = responseBeat + 1 === responseHead.count

  response_valid := responseMetaFifo.io.pop.valid && responsePayloadReady
  response := B(0, 33 bits)
  response(0) := !responseIsFailure
  when(responseIsWrite) {
    response(32 downto 1) := responseHead.writeData
  }
  when(responseIsScalarRead) {
    response(32 downto 1) := scalarDataFifo.io.pop.payload
  }
  when(responseIsBurstRead) {
    response(32 downto 1) := burstDataFifo.io.pop.payload
  }

  val responseFire = response_ready && response_valid
  responseMetaFifo.io.pop.ready := responseFire && responseLast
  scalarDataFifo.io.pop.ready := responseFire && responseIsScalarRead
  burstDataFifo.io.pop.ready := responseFire && responseIsBurstRead
  val scalarResponseFire = scalarDataFifo.io.pop.fire
  val burstResponseFire = burstDataFifo.io.pop.fire
  when(responseFire) {
    responseBeat := Mux(responseLast, U(0, 7 bits), responseBeat + 1)
  }

  val scalarCreditAvailable = scalarReservations =/= readDepth || scalarResponseFire
  val burstSlotAvailable =
    burstReservations.resize(9) + commandCount.resize(9) <=
      U(128, 9 bits) + burstResponseFire.asUInt.resize(9)
  val readResourcesAvailable =
    readMetaFifo.io.push.ready &&
      (!commandIsScalarRead || scalarCreditAvailable) &&
      (!commandIsBurstRead || burstSlotAvailable)
  val issueResourcesAvailable =
    responseMetaFifo.io.push.ready && (!commandIsRead || readResourcesAvailable)
  val wrapPending = Reg(Bool()) init (False)
  val wrapAddress = Reg(UInt(22 bits)) init (0)
  val commandNeedsAvalon = commandInRange
  val commandCanIssue =
    !wrapPending && requestFifo.io.pop.valid && issueResourcesAvailable &&
      (!commandNeedsAvalon || mem_waitRequestn)
  val issueRead = commandCanIssue && commandIsRead
  val issueScalarRead = issueRead && commandIsScalarRead
  val issueBurstRead = issueRead && commandIsBurstRead

  requestFifo.io.pop.ready := commandCanIssue
  responseMetaFifo.io.push.valid := commandCanIssue
  responseMetaFifo.io.push.payload.kind := failureKind
  responseMetaFifo.io.push.payload.count := Mux(
    commandWrite || commandCount === 0,
    U(1, 7 bits),
    commandCount
  )
  responseMetaFifo.io.push.payload.writeData := commandData
  when(commandInRange) {
    responseMetaFifo.io.push.payload.kind := writeKind
    when(!commandWrite) {
      responseMetaFifo.io.push.payload.kind := scalarReadKind
      when(commandCount =/= 1) {
        responseMetaFifo.io.push.payload.kind := burstReadKind
      }
    }
  }

  readMetaFifo.io.push.valid := issueRead
  readMetaFifo.io.push.payload.count := commandCount

  mem_read := wrapPending ||
    (requestFifo.io.pop.valid && commandIsRead && issueResourcesAvailable)
  mem_write := !wrapPending && requestFifo.io.pop.valid && commandInRange &&
    commandWrite && issueResourcesAvailable
  mem_burstCount := Mux(
    wrapPending,
    wrapAddress(5 downto 0).resize(11),
    Mux(commandWrap, (U(64, 7 bits) - commandOffset.resize(7)).resize(11), commandCount.resize(11))
  )
  val avalonAddress = Mux(
    wrapPending,
    wrapAddress & U(0x3fffc0, 22 bits),
    commandAddress
  )
  mem_address := base_address + (avalonAddress << 2).resize(32)
  mem_byteEnable := Mux(commandWrite, commandMask, B(15, 4 bits))
  mem_writeData := commandData

  val returnActive = Reg(Bool()) init (False)
  val returnRemaining = Reg(UInt(7 bits)) init (0)
  val returningRead = mem_readDataValid && readMetaFifo.io.pop.valid
  val returningScalar = returningRead && readMetaFifo.io.pop.payload.count === 1
  val currentRemaining = Mux(returnActive, returnRemaining, readMetaFifo.io.pop.payload.count)
  val returningLast = currentRemaining === 1

  readMetaFifo.io.pop.ready := returningRead && returningLast
  scalarDataFifo.io.push.valid := returningScalar
  scalarDataFifo.io.push.payload := mem_readData
  burstDataFifo.io.push.valid := returningRead && !returningScalar
  burstDataFifo.io.push.payload := mem_readData

  when(returningRead) {
    when(returningLast) {
      returnActive := False
    } otherwise {
      returnActive := True
      returnRemaining := currentRemaining - 1
    }
  }

  when(issueBurstRead) {
    burstReservations := burstReservations + commandCount.resized
    when(commandWrap) {
      wrapPending := True
      wrapAddress := commandAddress
    }
  }
  when(wrapPending && mem_waitRequestn) {
    wrapPending := False
  }
  when(burstResponseFire) {
    burstReservations := burstReservations - 1
  }
  when(issueBurstRead && burstResponseFire) {
    burstReservations := burstReservations + commandCount.resized - 1
  }

  when(issueScalarRead =/= scalarResponseFire) {
    when(issueScalarRead) {
      scalarReservations := scalarReservations + 1
    } otherwise {
      scalarReservations := scalarReservations - 1
    }
  }
}

object XlsAvalonTransferMasterGen extends App {
  SpinalConfig(
    mode = Verilog,
    targetDirectory = "build/generated/scala/de10",
    privateNamespace = true,
    defaultConfigForClockDomains = ClockDomainConfig(
      resetKind = SYNC,
      resetActiveLevel = HIGH
    )
  ).generate(XlsAvalonTransferMasterCore())
}
