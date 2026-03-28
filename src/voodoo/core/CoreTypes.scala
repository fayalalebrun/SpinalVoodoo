package voodoo.core

import spinal.core._
import spinal.core.sim._
import voodoo._
import voodoo.frontend.RegisterBank

case class FramebufferLayout(c: Config) extends Bundle {
  val front = UInt(c.addressWidth.value bits)
  val back = UInt(c.addressWidth.value bits)
  val aux = UInt(c.addressWidth.value bits)
  val draw = FbRouting(c)
  val lfbWrite = FbRouting(c)
  val lfbRead = FbRouting(c)
  val displayedBuffer = UInt(2 bits)
  val swapsPending = UInt(3 bits)
}

object FramebufferLayout {
  def fromRegisterBank(
      c: Config,
      fbBaseAddr: UInt,
      regBank: RegisterBank,
      swapBuffer: SwapBuffer
  ): FramebufferLayout = {
    val layout = FramebufferLayout(c)
    val fbPixelStride = (regBank.init.fbiInit1_videoTilesX << 6).resize(11 bits)
    val bufferOffsetBytes =
      regBank.init.fbiInit2_bufferOffset.resize(c.addressWidth.value bits) |<< 12
    val buffer0Base = fbBaseAddr
    val buffer1Base = fbBaseAddr + bufferOffsetBytes
    val auxBufferBase = fbBaseAddr + (bufferOffsetBytes << 1).resized
    val displayed = swapBuffer.io.swapCount(0)
    val frontBufferBase = displayed ? buffer1Base | buffer0Base
    val backBufferBase = displayed ? buffer0Base | buffer1Base

    layout.front := frontBufferBase
    layout.back := backBufferBase
    layout.aux := auxBufferBase
    layout.draw.colorBaseAddr := regBank.renderConfig.fbzMode.drawBuffer(
      0
    ) ? backBufferBase | frontBufferBase
    layout.draw.auxBaseAddr := auxBufferBase
    layout.draw.pixelStride := fbPixelStride
    layout.lfbWrite.colorBaseAddr := regBank.renderConfig.lfbMode.writeBufferSelect(
      0
    ) ? backBufferBase | frontBufferBase
    layout.lfbWrite.auxBaseAddr := auxBufferBase
    layout.lfbWrite.pixelStride := fbPixelStride
    layout.lfbRead.colorBaseAddr := regBank.renderConfig.lfbMode.readBufferSelect(
      0
    ) ? backBufferBase | frontBufferBase
    layout.lfbRead.auxBaseAddr := auxBufferBase
    layout.lfbRead.pixelStride := fbPixelStride
    layout.displayedBuffer := displayed.asUInt.resize(2 bits)
    layout.swapsPending := swapBuffer.io.swapsPending
    layout
  }
}

case class CoreStats() extends Bundle {
  val pixelsIn = UInt(24 bits)
  val chromaFail = UInt(24 bits)
  val zFuncFail = UInt(24 bits)
  val aFuncFail = UInt(24 bits)
  val pixelsOut = UInt(24 bits)

  def exposeToSim(): Unit = {
    pixelsIn.simPublic()
    chromaFail.simPublic()
    zFuncFail.simPublic()
    aFuncFail.simPublic()
    pixelsOut.simPublic()
  }
}

case class FramebufferMemStatus() extends Bundle {
  val colorBusy = Bool()
  val auxBusy = Bool()

  def exposeToSim(): Unit = {
    colorBusy.simPublic()
    auxBusy.simPublic()
  }
}

case class FramebufferMemStats() extends Bundle {
  val fillHits = UInt(32 bits)
  val fillMisses = UInt(32 bits)
  val fillBurstCount = UInt(32 bits)
  val fillBurstBeats = UInt(32 bits)
  val fillStallCycles = UInt(32 bits)
  val writeStallCycles = UInt(32 bits)
  val writeDrainCount = UInt(32 bits)
  val writeFullDrainCount = UInt(32 bits)
  val writePartialDrainCount = UInt(32 bits)
  val writeDrainReasonFullCount = UInt(32 bits)
  val writeDrainReasonRotateCount = UInt(32 bits)
  val writeDrainReasonFlushCount = UInt(32 bits)
  val writeDrainDirtyWordTotal = UInt(32 bits)
  val writeRotateBlockedCycles = UInt(32 bits)
  val writeSingleWordDrainCount = UInt(32 bits)
  val writeSingleWordDrainStartAtZeroCount = UInt(32 bits)
  val writeSingleWordDrainStartAtLastCount = UInt(32 bits)
  val writeRotateAdjacentLineCount = UInt(32 bits)
  val writeRotateSameLineGapCount = UInt(32 bits)
  val writeRotateOtherLineCount = UInt(32 bits)
  val memColorWriteCmdCount = UInt(32 bits)
  val memAuxWriteCmdCount = UInt(32 bits)
  val memColorReadCmdCount = UInt(32 bits)
  val memAuxReadCmdCount = UInt(32 bits)
  val memLfbReadCmdCount = UInt(32 bits)
  val memColorWriteBlockedCycles = UInt(32 bits)
  val memAuxWriteBlockedCycles = UInt(32 bits)
  val memColorReadBlockedCycles = UInt(32 bits)
  val memAuxReadBlockedCycles = UInt(32 bits)
  val memLfbReadBlockedCycles = UInt(32 bits)
  val readReqCount = UInt(32 bits)
  val readReqForwardStepCount = UInt(32 bits)
  val readReqBackwardStepCount = UInt(32 bits)
  val readReqSameWordCount = UInt(32 bits)
  val readReqSameLineCount = UInt(32 bits)
  val readReqOtherCount = UInt(32 bits)
  val readSingleBeatBurstCount = UInt(32 bits)
  val readMultiBeatBurstCount = UInt(32 bits)
  val readMaxQueueOccupancy = UInt(8 bits)

  def exposeToSim(): Unit = {
    fillHits.simPublic()
    fillMisses.simPublic()
    fillBurstCount.simPublic()
    fillBurstBeats.simPublic()
    fillStallCycles.simPublic()
    writeStallCycles.simPublic()
    writeDrainCount.simPublic()
    writeFullDrainCount.simPublic()
    writePartialDrainCount.simPublic()
    writeDrainReasonFullCount.simPublic()
    writeDrainReasonRotateCount.simPublic()
    writeDrainReasonFlushCount.simPublic()
    writeDrainDirtyWordTotal.simPublic()
    writeRotateBlockedCycles.simPublic()
    writeSingleWordDrainCount.simPublic()
    writeSingleWordDrainStartAtZeroCount.simPublic()
    writeSingleWordDrainStartAtLastCount.simPublic()
    writeRotateAdjacentLineCount.simPublic()
    writeRotateSameLineGapCount.simPublic()
    writeRotateOtherLineCount.simPublic()
    memColorWriteCmdCount.simPublic()
    memAuxWriteCmdCount.simPublic()
    memColorReadCmdCount.simPublic()
    memAuxReadCmdCount.simPublic()
    memLfbReadCmdCount.simPublic()
    memColorWriteBlockedCycles.simPublic()
    memAuxWriteBlockedCycles.simPublic()
    memColorReadBlockedCycles.simPublic()
    memAuxReadBlockedCycles.simPublic()
    memLfbReadBlockedCycles.simPublic()
    readReqCount.simPublic()
    readReqForwardStepCount.simPublic()
    readReqBackwardStepCount.simPublic()
    readReqSameWordCount.simPublic()
    readReqSameLineCount.simPublic()
    readReqOtherCount.simPublic()
    readSingleBeatBurstCount.simPublic()
    readMultiBeatBurstCount.simPublic()
    readMaxQueueOccupancy.simPublic()
  }
}

case class CoreBusyMap() extends Bundle {
  val triangleSetupValid = Bool()
  val rasterizerRunning = Bool()
  val tmuInputValid = Bool()
  val tmuBusy = Bool()
  val fbAccessBusy = Bool()
  val colorCombineInputValid = Bool()
  val fogBusy = Bool()
  val fbAccessInputValid = Bool()
  val writeColorInputValid = Bool()
  val writeAuxInputValid = Bool()
  val fastfillRunning = Bool()
  val swapWaiting = Bool()
  val lfbBusy = Bool()
  val fastfillOutputValid = Bool()
  val fastfillOutputReady = Bool()
  val fastfillWriteValid = Bool()
  val fastfillWriteReady = Bool()
  val preDitherMergedValid = Bool()
  val preDitherMergedReady = Bool()
  val preDitherPipedValid = Bool()
  val preDitherPipedReady = Bool()
  val ditherOutputValid = Bool()
  val ditherOutputReady = Bool()
  val ditherJoinedValid = Bool()
  val ditherJoinedReady = Bool()
  val colorForkValid = Bool()
  val colorForkReady = Bool()
  val auxForkValid = Bool()
  val auxForkReady = Bool()
  val writeColorReady = Bool()
  val writeAuxReady = Bool()
  val fastfillWriteAuxWrite = Bool()

  def bitsValue: Bits = {
    val bits = Bits(32 bits)
    bits := 0
    bits(0) := triangleSetupValid
    bits(1) := rasterizerRunning
    bits(2) := tmuInputValid
    bits(3) := tmuBusy
    bits(4) := fbAccessBusy
    bits(5) := colorCombineInputValid
    bits(6) := fogBusy
    bits(7) := fbAccessInputValid
    bits(8) := writeColorInputValid
    bits(9) := writeAuxInputValid
    bits(10) := fastfillRunning
    bits(11) := swapWaiting
    bits(12) := lfbBusy
    bits(13) := fastfillOutputValid
    bits(14) := fastfillOutputReady
    bits(15) := fastfillWriteValid
    bits(16) := fastfillWriteReady
    bits(17) := preDitherMergedValid
    bits(18) := preDitherMergedReady
    bits(19) := preDitherPipedValid
    bits(20) := preDitherPipedReady
    bits(21) := ditherOutputValid
    bits(22) := ditherOutputReady
    bits(23) := ditherJoinedValid
    bits(24) := ditherJoinedReady
    bits(25) := colorForkValid
    bits(26) := colorForkReady
    bits(27) := auxForkValid
    bits(28) := auxForkReady
    bits(29) := writeColorReady
    bits(30) := writeAuxReady
    bits(31) := fastfillWriteAuxWrite
    bits
  }

  def exposeToSim(): Unit = {
    triangleSetupValid.simPublic()
    rasterizerRunning.simPublic()
    tmuInputValid.simPublic()
    tmuBusy.simPublic()
    fbAccessBusy.simPublic()
    colorCombineInputValid.simPublic()
    fogBusy.simPublic()
    fbAccessInputValid.simPublic()
    writeColorInputValid.simPublic()
    writeAuxInputValid.simPublic()
    fastfillRunning.simPublic()
    swapWaiting.simPublic()
    lfbBusy.simPublic()
    fastfillOutputValid.simPublic()
    fastfillOutputReady.simPublic()
    fastfillWriteValid.simPublic()
    fastfillWriteReady.simPublic()
    preDitherMergedValid.simPublic()
    preDitherMergedReady.simPublic()
    preDitherPipedValid.simPublic()
    preDitherPipedReady.simPublic()
    ditherOutputValid.simPublic()
    ditherOutputReady.simPublic()
    ditherJoinedValid.simPublic()
    ditherJoinedReady.simPublic()
    colorForkValid.simPublic()
    colorForkReady.simPublic()
    auxForkValid.simPublic()
    auxForkReady.simPublic()
    writeColorReady.simPublic()
    writeAuxReady.simPublic()
    fastfillWriteAuxWrite.simPublic()
  }
}

case class CoreWritePathMap() extends Bundle {
  val fastfillRunning = Bool()
  val fastfillOutputValid = Bool()
  val fastfillOutputReady = Bool()
  val fastfillWriteValid = Bool()
  val fastfillWriteReady = Bool()
  val preDitherMergedValid = Bool()
  val preDitherMergedReady = Bool()
  val ditherJoinedValid = Bool()
  val ditherJoinedReady = Bool()
  val colorForkValid = Bool()
  val colorForkReady = Bool()
  val colorWriteInputValid = Bool()
  val colorWriteInputReady = Bool()
  val colorWriteFbValid = Bool()
  val colorWriteFbReady = Bool()
  val fbColorBusy = Bool()
  val fbAuxBusy = Bool()
  val auxForkValid = Bool()
  val auxForkReady = Bool()
  val auxWriteInputValid = Bool()
  val auxWriteInputReady = Bool()
  val auxWriteFbValid = Bool()
  val auxWriteFbReady = Bool()
  val fbFillHitsNonZero = Bool()
  val fbFillMissesNonZero = Bool()
  val fbFillBurstCountNonZero = Bool()
  val fbFillBurstBeatsNonZero = Bool()
  val fbFillStallCyclesNonZero = Bool()
  val pixelsInNonZero = Bool()
  val pixelsOutNonZero = Bool()
  val zFuncFailNonZero = Bool()
  val aFuncFailNonZero = Bool()

  def bitsValue: Bits = {
    val bits = Bits(32 bits)
    bits := 0
    bits(0) := fastfillRunning
    bits(1) := fastfillOutputValid
    bits(2) := fastfillOutputReady
    bits(3) := fastfillWriteValid
    bits(4) := fastfillWriteReady
    bits(5) := preDitherMergedValid
    bits(6) := preDitherMergedReady
    bits(7) := ditherJoinedValid
    bits(8) := ditherJoinedReady
    bits(9) := colorForkValid
    bits(10) := colorForkReady
    bits(11) := colorWriteInputValid
    bits(12) := colorWriteInputReady
    bits(13) := colorWriteFbValid
    bits(14) := colorWriteFbReady
    bits(15) := fbColorBusy
    bits(16) := fbAuxBusy
    bits(17) := auxForkValid
    bits(18) := auxForkReady
    bits(19) := auxWriteInputValid
    bits(20) := auxWriteInputReady
    bits(21) := auxWriteFbValid
    bits(22) := auxWriteFbReady
    bits(23) := fbFillHitsNonZero
    bits(24) := fbFillMissesNonZero
    bits(25) := fbFillBurstCountNonZero
    bits(26) := fbFillBurstBeatsNonZero
    bits(27) := fbFillStallCyclesNonZero
    bits(28) := pixelsInNonZero
    bits(29) := pixelsOutNonZero
    bits(30) := zFuncFailNonZero
    bits(31) := aFuncFailNonZero
    bits
  }

  def exposeToSim(): Unit = {
    fastfillRunning.simPublic()
    fastfillOutputValid.simPublic()
    fastfillOutputReady.simPublic()
    fastfillWriteValid.simPublic()
    fastfillWriteReady.simPublic()
    preDitherMergedValid.simPublic()
    preDitherMergedReady.simPublic()
    ditherJoinedValid.simPublic()
    ditherJoinedReady.simPublic()
    colorForkValid.simPublic()
    colorForkReady.simPublic()
    colorWriteInputValid.simPublic()
    colorWriteInputReady.simPublic()
    colorWriteFbValid.simPublic()
    colorWriteFbReady.simPublic()
    fbColorBusy.simPublic()
    fbAuxBusy.simPublic()
    auxForkValid.simPublic()
    auxForkReady.simPublic()
    auxWriteInputValid.simPublic()
    auxWriteInputReady.simPublic()
    auxWriteFbValid.simPublic()
    auxWriteFbReady.simPublic()
    fbFillHitsNonZero.simPublic()
    fbFillMissesNonZero.simPublic()
    fbFillBurstCountNonZero.simPublic()
    fbFillBurstBeatsNonZero.simPublic()
    fbFillStallCyclesNonZero.simPublic()
    pixelsInNonZero.simPublic()
    pixelsOutNonZero.simPublic()
    zFuncFailNonZero.simPublic()
    aFuncFailNonZero.simPublic()
  }
}

case class CoreDebug() extends Bundle {
  val pipelineBusy = Bool()
  val busy = CoreBusyMap()
  val writePath = CoreWritePathMap()

  def exposeToSim(): Unit = {
    pipelineBusy.simPublic()
    busy.exposeToSim()
    writePath.exposeToSim()
  }
}
