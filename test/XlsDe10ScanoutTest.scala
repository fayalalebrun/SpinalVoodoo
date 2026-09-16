package voodoo.de10

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._
import spinal.lib.sim._
import voodoo.Config
import scala.collection.mutable
import scala.language.reflectiveCalls

class XlsDe10ScanoutTest extends AnyFunSuite {
  test("scanout timing cuts preserve addresses and spans under memory stalls") {
    SimConfig.withIVerilog
      .compile {
        val dut = XlsDe10Scanout(Config.voodoo1())
        dut.scanout.io.readReq.simPublic()
        dut.reader.io.readReq.simPublic()
        dut.scanout.io.prefetchReq.simPublic()
        dut.reader.io.prefetchReq.simPublic()
        dut.scanout.io.readRsp.simPublic()
        dut
      }
      .doSim { dut =>
        dut.clockDomain.forkStimulus(10)
        dut.io.base #= 0x1000
        dut.io.pixelStride #= 640
        dut.io.enable #= false
        dut.io.hdmi.clock #= false
        dut.io.hdmi.reset #= true
        dut.io.mem.waitRequestn #= false
        dut.io.mem.readDataValid #= false
        dut.io.mem.readData #= 0
        val reads = mutable.Queue.empty[Long]
        val spans = mutable.Queue.empty[(Long, Long)]
        var issued = 0
        var consumed = 0
        var consumedSpans = 0
        var pixels = 0
        StreamMonitor(dut.scanout.io.readRsp, dut.clockDomain) { payload =>
          assert(payload.data.toInt == (if (pixels % 2 == 0) 0x5678 else 0x1234))
          pixels += 1
        }
        StreamMonitor(dut.scanout.io.readReq, dut.clockDomain) { payload =>
          val expected = 0x1000L + 2L * issued
          assert(payload.address.toLong == expected)
          reads.enqueue(expected)
          issued += 1
        }
        StreamMonitor(dut.reader.io.readReq, dut.clockDomain) { payload =>
          assert(reads.nonEmpty)
          assert(payload.address.toLong == reads.dequeue())
          consumed += 1
        }
        StreamMonitor(dut.scanout.io.prefetchReq, dut.clockDomain) { payload =>
          spans.enqueue((payload.startAddress.toLong, payload.endAddress.toLong))
        }
        StreamMonitor(dut.reader.io.prefetchReq, dut.clockDomain) { payload =>
          assert(spans.nonEmpty)
          assert((payload.startAddress.toLong, payload.endAddress.toLong) == spans.dequeue())
          consumedSpans += 1
        }
        dut.clockDomain.waitSampling(10)
        for (_ <- 0 until 2) {
          dut.clockDomain.assertReset()
          dut.io.enable #= false
          dut.io.mem.waitRequestn #= false
          dut.io.mem.readDataValid #= false
          sleep(100)
          reads.clear()
          spans.clear()
          issued = 0
          consumed = 0
          consumedSpans = 0
          pixels = 0
          dut.clockDomain.deassertReset()
          dut.clockDomain.waitSampling(10)
          dut.io.enable #= true
          var pending = 0
          var replies = 0
          for (cycle <- 0 until 20000) {
            val ready = cycle % 11 >= 4
            val responding = pending > 0 && cycle % 7 != 0
            dut.io.mem.waitRequestn #= ready
            dut.io.mem.readDataValid #= responding
            dut.io.mem.readData #= 0x12345678L
            sleep(1)
            if (responding) {
              pending -= 1
              replies += 1
            }
            if (dut.io.mem.read.toBoolean && ready) {
              assert(dut.io.mem.address.toLong >= De10AddressMap.fbMemBase + 0x1000L)
              pending += dut.io.mem.burstCount.toInt
            }
            assert(!dut.io.mem.write.toBoolean)
            dut.clockDomain.waitSampling()
          }
          assert(consumed >= 1280)
          assert(consumedSpans >= 2)
          assert(replies >= 640)
          assert(reads.size <= 2 && spans.size <= 2)
          assert(pixels >= 1280)
        }
      }
  }
}
