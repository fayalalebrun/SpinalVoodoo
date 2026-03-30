package voodoo.core

import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo._
import voodoo.framebuffer.FramebufferPlaneDirectReader

case class FramebufferMemSubsystem(c: Config) extends Component {
  val io = new Bundle {
    val colorWrite = slave(Stream(FramebufferPlaneBuffer.WriteReq(c)))
    val auxWrite = slave(Stream(FramebufferPlaneBuffer.WriteReq(c)))
    val colorReadReq = slave(Stream(FramebufferPlaneBuffer.ReadReq(c)))
    val colorReadRsp = master(Stream(FramebufferPlaneBuffer.ReadRsp()))
    val auxReadReq = slave(Stream(FramebufferPlaneBuffer.ReadReq(c)))
    val auxReadRsp = master(Stream(FramebufferPlaneBuffer.ReadRsp()))
    val prefetchColor = slave(Stream(FramebufferPlaneReader.PrefetchReq(c)))
    val prefetchAux = slave(Stream(FramebufferPlaneReader.PrefetchReq(c)))
    val lfbReadBus = slave(Bmb(Lfb.fbReadBmbParams(c)))
    val flush = in Bool ()

    val fbMemWrite = master(Bmb(voodoo.Core.fbMemBmbParams(c)))
    val fbColorReadMem = master(Bmb(voodoo.Core.fbMemBmbParams(c)))
    val fbAuxReadMem = master(Bmb(voodoo.Core.fbMemBmbParams(c)))

    val status = out(FramebufferMemStatus())
    val stats = out(FramebufferMemStats())
  }

  private def makeFbWriteArbiter() =
    BmbArbiter(
      inputsParameter =
        Seq(FramebufferPlaneBuffer.bmbParams(c), FramebufferPlaneBuffer.bmbParams(c)),
      outputParameter = voodoo.Core.fbMemBmbParams(c),
      lowerFirstPriority = true
    )

  private def makeFbColorReadArbiter() =
    BmbArbiter(
      inputsParameter = Seq(FramebufferPlaneReader.bmbParams(c), Lfb.fbReadBmbParams(c)),
      outputParameter = voodoo.Core.fbMemBmbParams(c),
      lowerFirstPriority = true
    )

  private def makeFbAuxReadArbiter() =
    BmbArbiter(
      inputsParameter = Seq(FramebufferPlaneReader.bmbParams(c)),
      outputParameter = voodoo.Core.fbMemBmbParams(c),
      lowerFirstPriority = true
    )

  private def disableReadPort(port: FramebufferPlaneBuffer): Unit = {
    port.io.readReq.valid := False
    port.io.readReq.address := 0
    port.io.readRsp.ready := True
  }

  val colorReaderCached =
    if (c.useFbWriteBuffer) FramebufferPlaneReader(c).setName("fbColorReader") else null
  val auxReaderCached =
    if (c.useFbWriteBuffer) FramebufferPlaneReader(c).setName("fbAuxReader") else null
  val colorReaderDirect =
    if (!c.useFbWriteBuffer) FramebufferPlaneDirectReader(c).setName("fbColorReader") else null
  val auxReaderDirect =
    if (!c.useFbWriteBuffer) FramebufferPlaneDirectReader(c).setName("fbAuxReader") else null
  val colorWritePort = FramebufferPlaneBuffer(c).setName("fbColorBuffer")
  val auxWritePort = FramebufferPlaneBuffer(c).setName("fbAuxBuffer")

  if (c.useFbWriteBuffer) {
    colorWritePort.io.flush := io.flush
    auxWritePort.io.flush := io.flush
  } else {
    colorWritePort.io.flush := False
    auxWritePort.io.flush := False
  }

  if (c.useFbWriteBuffer) {
    io.colorReadReq.s2mPipe() >> colorReaderCached.io.readReq
    io.colorReadRsp << colorReaderCached.io.readRsp
    io.auxReadReq.s2mPipe() >> auxReaderCached.io.readReq
    io.auxReadRsp << auxReaderCached.io.readRsp
    colorReaderCached.io.prefetchReq <> io.prefetchColor
    auxReaderCached.io.prefetchReq <> io.prefetchAux
  } else {
    io.colorReadReq.s2mPipe() >> colorReaderDirect.io.readReq
    io.colorReadRsp << colorReaderDirect.io.readRsp
    io.auxReadReq.s2mPipe() >> auxReaderDirect.io.readReq
    io.auxReadRsp << auxReaderDirect.io.readRsp
    io.prefetchColor.ready := True
    io.prefetchAux.ready := True
  }

  disableReadPort(colorWritePort)
  disableReadPort(auxWritePort)
  colorWritePort.io.writeReq << io.colorWrite.s2mPipe()
  auxWritePort.io.writeReq << io.auxWrite.s2mPipe()

  val fbWriteArbiter = makeFbWriteArbiter()
  val fbColorReadArbiter = makeFbColorReadArbiter()
  val fbAuxReadArbiter = makeFbAuxReadArbiter()

  fbWriteArbiter.io.inputs(0).cmd << colorWritePort.io.mem.cmd.s2mPipe()
  colorWritePort.io.mem.rsp << fbWriteArbiter.io.inputs(0).rsp.s2mPipe()
  fbWriteArbiter.io.inputs(1).cmd << auxWritePort.io.mem.cmd.s2mPipe()
  auxWritePort.io.mem.rsp << fbWriteArbiter.io.inputs(1).rsp.s2mPipe()

  if (c.useFbWriteBuffer) {
    fbColorReadArbiter.io.inputs(0).cmd << colorReaderCached.io.mem.cmd.s2mPipe()
    colorReaderCached.io.mem.rsp << fbColorReadArbiter.io.inputs(0).rsp.s2mPipe()
    fbAuxReadArbiter.io.inputs(0).cmd << auxReaderCached.io.mem.cmd.s2mPipe()
    auxReaderCached.io.mem.rsp << fbAuxReadArbiter.io.inputs(0).rsp.s2mPipe()
  } else {
    fbColorReadArbiter.io.inputs(0).cmd << colorReaderDirect.io.mem.cmd
    colorReaderDirect.io.mem.rsp << fbColorReadArbiter.io.inputs(0).rsp
    fbAuxReadArbiter.io.inputs(0).cmd << auxReaderDirect.io.mem.cmd
    auxReaderDirect.io.mem.rsp << fbAuxReadArbiter.io.inputs(0).rsp
  }
  fbColorReadArbiter.io.inputs(1).cmd << io.lfbReadBus.cmd
  io.lfbReadBus.rsp << fbColorReadArbiter.io.inputs(1).rsp.s2mPipe()

  fbWriteArbiter.io.output <> io.fbMemWrite
  fbColorReadArbiter.io.output <> io.fbColorReadMem
  fbAuxReadArbiter.io.output <> io.fbAuxReadMem

  val memColorWriteCmdCount = Reg(UInt(32 bits)) init (0)
  val memAuxWriteCmdCount = Reg(UInt(32 bits)) init (0)
  val memColorReadCmdCount = Reg(UInt(32 bits)) init (0)
  val memAuxReadCmdCount = Reg(UInt(32 bits)) init (0)
  val memLfbReadCmdCount = Reg(UInt(32 bits)) init (0)
  val memColorWriteBlockedCycles = Reg(UInt(32 bits)) init (0)
  val memAuxWriteBlockedCycles = Reg(UInt(32 bits)) init (0)
  val memColorReadBlockedCycles = Reg(UInt(32 bits)) init (0)
  val memAuxReadBlockedCycles = Reg(UInt(32 bits)) init (0)
  val memLfbReadBlockedCycles = Reg(UInt(32 bits)) init (0)

  if (c.useFbWriteBuffer) {
    when(colorWritePort.io.mem.cmd.valid && !fbWriteArbiter.io.inputs(0).cmd.ready) {
      memColorWriteBlockedCycles := memColorWriteBlockedCycles + 1
    }
    when(auxWritePort.io.mem.cmd.valid && !fbWriteArbiter.io.inputs(1).cmd.ready) {
      memAuxWriteBlockedCycles := memAuxWriteBlockedCycles + 1
    }
    when(colorReaderCached.io.mem.cmd.valid && !fbColorReadArbiter.io.inputs(0).cmd.ready) {
      memColorReadBlockedCycles := memColorReadBlockedCycles + 1
    }
    when(auxReaderCached.io.mem.cmd.valid && !fbAuxReadArbiter.io.inputs(0).cmd.ready) {
      memAuxReadBlockedCycles := memAuxReadBlockedCycles + 1
    }
    when(io.lfbReadBus.cmd.valid && !fbColorReadArbiter.io.inputs(1).cmd.ready) {
      memLfbReadBlockedCycles := memLfbReadBlockedCycles + 1
    }
    when(colorWritePort.io.mem.cmd.fire) {
      memColorWriteCmdCount := memColorWriteCmdCount + 1
    }
    when(auxWritePort.io.mem.cmd.fire) {
      memAuxWriteCmdCount := memAuxWriteCmdCount + 1
    }
    when(colorReaderCached.io.mem.cmd.fire) {
      memColorReadCmdCount := memColorReadCmdCount + 1
    }
    when(auxReaderCached.io.mem.cmd.fire) {
      memAuxReadCmdCount := memAuxReadCmdCount + 1
    }
    when(io.lfbReadBus.cmd.fire) {
      memLfbReadCmdCount := memLfbReadCmdCount + 1
    }
  }

  io.status.colorBusy := colorWritePort.io.busy || (if (c.useFbWriteBuffer)
                                                      colorReaderCached.io.busy
                                                    else colorReaderDirect.io.busy)
  io.status.auxBusy := auxWritePort.io.busy || (if (c.useFbWriteBuffer) auxReaderCached.io.busy
                                                else auxReaderDirect.io.busy)

  io.stats.fillHits := (if (c.useFbWriteBuffer)
                          (colorReaderCached.io.fillHits + auxReaderCached.io.fillHits).resized
                        else U(0, 32 bits))
  io.stats.fillMisses := (if (c.useFbWriteBuffer)
                            (colorReaderCached.io.fillMisses + auxReaderCached.io.fillMisses).resized
                          else U(0, 32 bits))
  io.stats.fillBurstCount := (if (c.useFbWriteBuffer)
                                (colorReaderCached.io.fillBurstCount + auxReaderCached.io.fillBurstCount).resized
                              else U(0, 32 bits))
  io.stats.fillBurstBeats := (if (c.useFbWriteBuffer)
                                (colorReaderCached.io.fillBurstBeats + auxReaderCached.io.fillBurstBeats).resized
                              else U(0, 32 bits))
  io.stats.fillStallCycles := (if (c.useFbWriteBuffer)
                                 (colorReaderCached.io.fillStallCycles + auxReaderCached.io.fillStallCycles).resized
                               else U(0, 32 bits))
  io.stats.writeStallCycles := (if (c.useFbWriteBuffer)
                                  (colorWritePort.io.writeStallCycles + auxWritePort.io.writeStallCycles).resized
                                else U(0, 32 bits))
  io.stats.writeDrainCount := (if (c.useFbWriteBuffer)
                                 (colorWritePort.io.writeDrainCount + auxWritePort.io.writeDrainCount).resized
                               else U(0, 32 bits))
  io.stats.writeFullDrainCount := (if (c.useFbWriteBuffer)
                                     (colorWritePort.io.writeFullDrainCount + auxWritePort.io.writeFullDrainCount).resized
                                   else U(0, 32 bits))
  io.stats.writePartialDrainCount := (if (c.useFbWriteBuffer)
                                        (colorWritePort.io.writePartialDrainCount + auxWritePort.io.writePartialDrainCount).resized
                                      else U(0, 32 bits))
  io.stats.writeDrainReasonFullCount := (if (c.useFbWriteBuffer)
                                           (colorWritePort.io.writeDrainReasonFullCount + auxWritePort.io.writeDrainReasonFullCount).resized
                                         else U(0, 32 bits))
  io.stats.writeDrainReasonRotateCount := (if (c.useFbWriteBuffer)
                                             (colorWritePort.io.writeDrainReasonRotateCount + auxWritePort.io.writeDrainReasonRotateCount).resized
                                           else U(0, 32 bits))
  io.stats.writeDrainReasonFlushCount := (if (c.useFbWriteBuffer)
                                            (colorWritePort.io.writeDrainReasonFlushCount + auxWritePort.io.writeDrainReasonFlushCount).resized
                                          else U(0, 32 bits))
  io.stats.writeDrainDirtyWordTotal := (if (c.useFbWriteBuffer)
                                          (colorWritePort.io.writeDrainDirtyWordTotal + auxWritePort.io.writeDrainDirtyWordTotal).resized
                                        else U(0, 32 bits))
  io.stats.writeRotateBlockedCycles := (if (c.useFbWriteBuffer)
                                          (colorWritePort.io.writeRotateBlockedCycles + auxWritePort.io.writeRotateBlockedCycles).resized
                                        else U(0, 32 bits))
  io.stats.writeSingleWordDrainCount := (if (c.useFbWriteBuffer)
                                           (colorWritePort.io.writeSingleWordDrainCount + auxWritePort.io.writeSingleWordDrainCount).resized
                                         else U(0, 32 bits))
  io.stats.writeSingleWordDrainStartAtZeroCount := (if (c.useFbWriteBuffer)
                                                      (colorWritePort.io.writeSingleWordDrainStartAtZeroCount + auxWritePort.io.writeSingleWordDrainStartAtZeroCount).resized
                                                    else U(0, 32 bits))
  io.stats.writeSingleWordDrainStartAtLastCount := (if (c.useFbWriteBuffer)
                                                      (colorWritePort.io.writeSingleWordDrainStartAtLastCount + auxWritePort.io.writeSingleWordDrainStartAtLastCount).resized
                                                    else U(0, 32 bits))
  io.stats.writeRotateAdjacentLineCount := (if (c.useFbWriteBuffer)
                                              (colorWritePort.io.writeRotateAdjacentLineCount + auxWritePort.io.writeRotateAdjacentLineCount).resized
                                            else U(0, 32 bits))
  io.stats.writeRotateSameLineGapCount := (if (c.useFbWriteBuffer)
                                             (colorWritePort.io.writeRotateSameLineGapCount + auxWritePort.io.writeRotateSameLineGapCount).resized
                                           else U(0, 32 bits))
  io.stats.writeRotateOtherLineCount := (if (c.useFbWriteBuffer)
                                           (colorWritePort.io.writeRotateOtherLineCount + auxWritePort.io.writeRotateOtherLineCount).resized
                                         else U(0, 32 bits))
  io.stats.memColorWriteCmdCount := (if (c.useFbWriteBuffer) memColorWriteCmdCount
                                     else U(0, 32 bits))
  io.stats.memAuxWriteCmdCount := (if (c.useFbWriteBuffer) memAuxWriteCmdCount else U(0, 32 bits))
  io.stats.memColorReadCmdCount := (if (c.useFbWriteBuffer) memColorReadCmdCount else U(0, 32 bits))
  io.stats.memAuxReadCmdCount := (if (c.useFbWriteBuffer) memAuxReadCmdCount else U(0, 32 bits))
  io.stats.memLfbReadCmdCount := (if (c.useFbWriteBuffer) memLfbReadCmdCount else U(0, 32 bits))
  io.stats.memColorWriteBlockedCycles := (if (c.useFbWriteBuffer) memColorWriteBlockedCycles
                                          else U(0, 32 bits))
  io.stats.memAuxWriteBlockedCycles := (if (c.useFbWriteBuffer) memAuxWriteBlockedCycles
                                        else U(0, 32 bits))
  io.stats.memColorReadBlockedCycles := (if (c.useFbWriteBuffer) memColorReadBlockedCycles
                                         else U(0, 32 bits))
  io.stats.memAuxReadBlockedCycles := (if (c.useFbWriteBuffer) memAuxReadBlockedCycles
                                       else U(0, 32 bits))
  io.stats.memLfbReadBlockedCycles := (if (c.useFbWriteBuffer) memLfbReadBlockedCycles
                                       else U(0, 32 bits))
  io.stats.readReqCount := (if (c.useFbWriteBuffer)
                              (colorReaderCached.io.reqCount + auxReaderCached.io.reqCount).resized
                            else U(0, 32 bits))
  io.stats.readReqForwardStepCount := (if (c.useFbWriteBuffer)
                                         (colorReaderCached.io.reqForwardStepCount + auxReaderCached.io.reqForwardStepCount).resized
                                       else U(0, 32 bits))
  io.stats.readReqBackwardStepCount := (if (c.useFbWriteBuffer)
                                          (colorReaderCached.io.reqBackwardStepCount + auxReaderCached.io.reqBackwardStepCount).resized
                                        else U(0, 32 bits))
  io.stats.readReqSameWordCount := (if (c.useFbWriteBuffer)
                                      (colorReaderCached.io.reqSameWordCount + auxReaderCached.io.reqSameWordCount).resized
                                    else U(0, 32 bits))
  io.stats.readReqSameLineCount := (if (c.useFbWriteBuffer)
                                      (colorReaderCached.io.reqSameLineCount + auxReaderCached.io.reqSameLineCount).resized
                                    else U(0, 32 bits))
  io.stats.readReqOtherCount := (if (c.useFbWriteBuffer)
                                   (colorReaderCached.io.reqOtherCount + auxReaderCached.io.reqOtherCount).resized
                                 else U(0, 32 bits))
  io.stats.readSingleBeatBurstCount := (if (c.useFbWriteBuffer)
                                          (colorReaderCached.io.singleBeatBurstCount + auxReaderCached.io.singleBeatBurstCount).resized
                                        else U(0, 32 bits))
  io.stats.readMultiBeatBurstCount := (if (c.useFbWriteBuffer)
                                         (colorReaderCached.io.multiBeatBurstCount + auxReaderCached.io.multiBeatBurstCount).resized
                                       else U(0, 32 bits))
  io.stats.readMaxQueueOccupancy := (if (c.useFbWriteBuffer)
                                       Mux(
                                         colorReaderCached.io.maxOccupancy > auxReaderCached.io.maxOccupancy,
                                         colorReaderCached.io.maxOccupancy,
                                         auxReaderCached.io.maxOccupancy
                                       ).resize(8 bits)
                                     else U(0, 8 bits))

  io.status.exposeToSim()
  io.stats.exposeToSim()
}
