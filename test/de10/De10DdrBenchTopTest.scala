//> using target.scope test

package voodoo.de10

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._
import scala.language.reflectiveCalls
import scala.collection.mutable

class De10DdrBenchTopTest extends AnyFunSuite {
  import De10DdrBenchAddressMap._

  private def initMmio(dut: De10DdrBenchTop): Unit = {
    dut.io.h2fLw.address #= 0
    dut.io.h2fLw.read #= false
    dut.io.h2fLw.write #= false
    dut.io.h2fLw.byteenable #= 0xf
    dut.io.h2fLw.writedata #= 0
  }

  private def mmioWrite(dut: De10DdrBenchTop, byteOffset: Int, value: Long): Unit = {
    dut.io.h2fLw.address #= (byteOffset >> 2)
    dut.io.h2fLw.writedata #= value
    dut.io.h2fLw.byteenable #= 0xf
    dut.io.h2fLw.write #= true
    while (dut.io.h2fLw.waitrequest.toBoolean) dut.clockDomain.waitSampling()
    dut.clockDomain.waitSampling()
    dut.io.h2fLw.write #= false
    dut.clockDomain.waitSampling()
  }

  private def mmioRead(dut: De10DdrBenchTop, byteOffset: Int): Long = {
    dut.io.h2fLw.address #= (byteOffset >> 2)
    dut.io.h2fLw.byteenable #= 0xf
    dut.io.h2fLw.read #= true
    while (dut.io.h2fLw.waitrequest.toBoolean) dut.clockDomain.waitSampling()
    dut.clockDomain.waitSampling()
    dut.io.h2fLw.read #= false
    while (!dut.io.h2fLw.readdatavalid.toBoolean) dut.clockDomain.waitSampling()
    val value = dut.io.h2fLw.readdata.toLong & 0xffffffffL
    dut.clockDomain.waitSampling()
    value
  }

  private def mmioRead64(dut: De10DdrBenchTop, loOffset: Int): Long = {
    val lo = mmioRead(dut, loOffset)
    val hi = mmioRead(dut, loOffset + 4)
    lo | (hi << 32)
  }

  private def attachMemory(dut: De10DdrBenchTop): Unit = {
    val fbMem = mutable.Map.empty[Long, Long]
    val texMem = mutable.Map.empty[Long, Long]

    fork {
      var cycle = 0
      var fbPending = List.empty[(Int, Long)]
      var texPending = List.empty[(Int, Long)]

      dut.io.memFb.waitRequestn #= true
      dut.io.memFb.readDataValid #= false
      dut.io.memFb.readData #= 0
      dut.io.memTex.waitRequestn #= true
      dut.io.memTex.readDataValid #= false
      dut.io.memTex.readData #= 0

      while (true) {
        val fbReady = (cycle % 3) != 1
        dut.io.memFb.waitRequestn #= fbReady
        dut.io.memTex.waitRequestn #= true

        fbPending = fbPending.map { case (delay, data) => (delay - 1, data) }
        texPending = texPending.map { case (delay, data) => (delay - 1, data) }

        fbPending.find(_._1 <= 0) match {
          case Some((_, data)) =>
            dut.io.memFb.readDataValid #= true
            dut.io.memFb.readData #= data
            fbPending = fbPending.filterNot(_._1 <= 0)
          case None =>
            dut.io.memFb.readDataValid #= false
            dut.io.memFb.readData #= 0
        }

        texPending.find(_._1 <= 0) match {
          case Some((_, data)) =>
            dut.io.memTex.readDataValid #= true
            dut.io.memTex.readData #= data
            texPending = texPending.filterNot(_._1 <= 0)
          case None =>
            dut.io.memTex.readDataValid #= false
            dut.io.memTex.readData #= 0
        }

        dut.clockDomain.waitSampling()

        if (dut.io.memFb.write.toBoolean && dut.io.memFb.waitRequestn.toBoolean) {
          fbMem.update(dut.io.memFb.address.toLong, dut.io.memFb.writeData.toLong & 0xffffffffL)
        }
        if (dut.io.memTex.write.toBoolean && dut.io.memTex.waitRequestn.toBoolean) {
          texMem.update(dut.io.memTex.address.toLong, dut.io.memTex.writeData.toLong & 0xffffffffL)
        }

        if (dut.io.memFb.read.toBoolean && dut.io.memFb.waitRequestn.toBoolean) {
          val addr = dut.io.memFb.address.toLong
          val beats = math.max(1, dut.io.memFb.burstCount.toInt)
          for (beat <- 0 until beats) {
            val beatAddr = addr + beat * 4L
            val data = fbMem.getOrElse(beatAddr, beatAddr ^ 0x55aa00ffL)
            fbPending = fbPending :+ ((2 + beat, data))
          }
        }
        if (dut.io.memTex.read.toBoolean && dut.io.memTex.waitRequestn.toBoolean) {
          val addr = dut.io.memTex.address.toLong
          val beats = math.max(1, dut.io.memTex.burstCount.toInt)
          for (beat <- 0 until beats) {
            val beatAddr = addr + beat * 4L
            val data = texMem.getOrElse(beatAddr, beatAddr ^ 0xaa5500ffL)
            texPending = texPending :+ ((1 + beat, data))
          }
        }

        cycle += 1
      }
    }
  }

  test("bench exposes ID and collects per-port stats") {
    SimConfig.withIVerilog.compile(De10DdrBenchTop()).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      initMmio(dut)
      attachMemory(dut)
      dut.clockDomain.waitSampling(5)

      assert(mmioRead(dut, ident) == identValue)
      assert(mmioRead(dut, version) == versionValue)
      assert(mmioRead(dut, capability) == capabilityValue)

      val fbControl =
        (1 << De10DdrBenchPortControl.enable) | (1 << De10DdrBenchPortControl.readEnable)
      val texControl =
        (1 << De10DdrBenchPortControl.enable) | (1 << De10DdrBenchPortControl.writeEnable)

      mmioWrite(dut, control, 1 << De10DdrBenchGlobalControl.clear)
      mmioWrite(dut, fbPortBase + portControl, fbControl)
      mmioWrite(dut, fbPortBase + portStrideBytes, 4)
      mmioWrite(dut, fbPortBase + portTransferCount, 8)
      mmioWrite(dut, texPortBase + portControl, texControl)
      mmioWrite(dut, texPortBase + portStrideBytes, 4)
      mmioWrite(dut, texPortBase + portTransferCount, 8)
      mmioWrite(dut, control, 1 << De10DdrBenchGlobalControl.start)

      var polls = 0
      while (((mmioRead(dut, status) & 0x2L) == 0L) && polls < 200) {
        polls += 1
      }
      assert(polls < 200, "benchmark should complete")

      assert(mmioRead(dut, fbPortBase + portIssuedCount) == 8)
      assert(mmioRead(dut, fbPortBase + portCompletedCount) == 8)
      assert(mmioRead64(dut, fbPortBase + portBytesLo) == 32)
      assert(mmioRead64(dut, fbPortBase + portActiveCyclesLo) > 0)
      assert(mmioRead(dut, fbPortBase + portReadSamples) == 8)
      assert(
        mmioRead(dut, fbPortBase + portReadLatencyMax) >= mmioRead(
          dut,
          fbPortBase + portReadLatencyMin
        )
      )

      assert(mmioRead(dut, texPortBase + portIssuedCount) == 8)
      assert(mmioRead(dut, texPortBase + portCompletedCount) == 8)
      assert(mmioRead64(dut, texPortBase + portBytesLo) == 32)
      assert(mmioRead(dut, texPortBase + portReadSamples) == 0)
    }
  }

  test("bench aggregates burst reads into fewer Avalon commands") {
    SimConfig.withIVerilog.compile(De10DdrBenchTop()).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      initMmio(dut)
      attachMemory(dut)
      dut.clockDomain.waitSampling(5)

      val fbControl =
        (1 << De10DdrBenchPortControl.enable) | (1 << De10DdrBenchPortControl.readEnable)

      mmioWrite(dut, control, 1 << De10DdrBenchGlobalControl.clear)
      mmioWrite(dut, fbPortBase + portControl, fbControl)
      mmioWrite(dut, fbPortBase + portStrideBytes, 4)
      mmioWrite(dut, fbPortBase + portTransferCount, 8)
      mmioWrite(dut, fbPortBase + portBurstWords, 4)
      mmioWrite(dut, control, 1 << De10DdrBenchGlobalControl.start)

      var polls = 0
      while (((mmioRead(dut, status) & 0x2L) == 0L) && polls < 200) {
        polls += 1
      }
      assert(polls < 200, "burst benchmark should complete")

      assert(mmioRead(dut, fbPortBase + portIssuedCount) == 2)
      assert(mmioRead(dut, fbPortBase + portCompletedCount) == 2)
      assert(mmioRead64(dut, fbPortBase + portBytesLo) == 32)
      assert(mmioRead(dut, fbPortBase + portReadSamples) == 2)
      assert(mmioRead(dut, fbPortBase + portBurstWords) == 4)
    }
  }
}
