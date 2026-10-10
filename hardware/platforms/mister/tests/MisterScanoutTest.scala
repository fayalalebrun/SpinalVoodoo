package voodoo.mister

import org.scalatest.funsuite.AnyFunSuite
import spinal.core._
import spinal.core.sim._
import voodoo.hdmi.VideoTiming
import scala.collection.mutable
import scala.language.reflectiveCalls

class MisterScanoutTest extends AnyFunSuite {
  test("DDR response register retains the outstanding reservation until FIFO acceptance") {
    SimConfig.withIVerilog
      .compile {
        val dut = MisterScanout(8, 3, VideoTiming(8, 3, 2, 2, 2, 1, 1, 1))
        dut.words.io.push.simPublic()
        dut.outstanding.simPublic()
        dut
      }
      .doSim { dut =>
        SimTimeout(10000)
        dut.clockDomain.forkStimulus(10)
        dut.io.pixelClock #= false
        dut.io.pixelReset #= true
        dut.io.frame.sequence #= 1
        dut.io.frame.base #= MisterMemoryMap.framebufferBase
        dut.io.frame.width #= 8
        dut.io.frame.height #= 3
        dut.io.frame.stride #= 16
        dut.io.frame.format #= 4
        dut.io.frame.enable #= true
        dut.io.memory.command.ready #= false
        dut.io.memory.response.valid #= false
        dut.io.memory.response.payload #= 0
        dut.clockDomain.waitSampling(12)
        dut.clockDomain.waitFallingEdge()
        dut.io.memory.command.ready #= true
        dut.clockDomain.waitSamplingWhere(dut.io.memory.command.valid.toBoolean)
        dut.clockDomain.waitFallingEdge()
        dut.io.memory.command.ready #= false
        assert(dut.outstanding.toInt == 2)
        dut.io.memory.response.valid #= true
        dut.io.memory.response.payload #= BigInt("123456789abcdef0", 16)
        sleep(1)
        assert(!dut.words.io.push.valid.toBoolean)
        dut.clockDomain.waitSampling()
        dut.clockDomain.waitFallingEdge()
        dut.io.memory.response.valid #= false
        sleep(1)
        assert(dut.words.io.push.valid.toBoolean)
        assert(dut.words.io.push.payload.toBigInt == BigInt("123456789abcdef0", 16))
        assert(dut.outstanding.toInt == 2)
        dut.clockDomain.waitSampling()
        dut.clockDomain.waitFallingEdge()
        assert(!dut.words.io.push.valid.toBoolean && dut.outstanding.toInt == 1)
      }
  }
  for (format <- Seq(4, 6)) {
    test(s"local scanout format $format pixels, padding and underflow recovery") {
      val timing = VideoTiming(8, 3, 2, 2, 2, 1, 1, 1)
      SimConfig.withIVerilog
        .compile {
          val dut = MisterScanout(8, 3, timing)
          dut.pixel.scan.io.x.simPublic()
          dut.pixel.scan.io.y.simPublic()
          dut
        }
        .doSim { dut =>
          SimTimeout(2000000)
          dut.clockDomain.forkStimulus(10)
          dut.io.pixelClock #= false
          dut.io.pixelReset #= true
          val pixelClock = dut.pixelDomain
          pixelClock.forkStimulus(100)
          val bytes = if (format == 4) 2 else 4
          val stride = if (format == 4) 12 else 20
          val base = MisterMemoryMap.framebufferBase
          val image = Array.fill[Byte](stride * 3 + 8)(0xa5.toByte)
          val expected = (0 until 24).map { index =>
            val x = index % 8
            val y = index / 8
            if (x >= 5) 0
            else {
              val raw = if (format == 4) 0x0841 * (1 + x + y * 5) else 0x102030 * (1 + x + y * 5)
              for (b <- 0 until bytes) image(y * stride + x * bytes + b) = (raw >> (b * 8)).toByte
              val rgb =
                if (format == 4) raw
                else ((raw >> 8) & 0xf800) | ((raw >> 5) & 0x7e0) | ((raw >> 3) & 31)
              val r = (rgb >> 11) & 31
              val g = (rgb >> 5) & 63
              val b = rgb & 31
              (((r << 3) | (r >> 2)) << 16) | (((g << 2) | (g >> 4)) << 8) | (b << 3) | (b >> 2)
            }
          }
          dut.io.frame.sequence #= 1
          dut.io.frame.base #= base
          dut.io.frame.width #= 5
          dut.io.frame.height #= 3
          dut.io.frame.stride #= stride
          dut.io.frame.format #= format
          dut.io.frame.enable #= true
          dut.io.memory.command.ready #= false
          dut.io.memory.response.valid #= false
          dut.io.memory.response.payload #= 0
          var stalled = false
          fork {
            val pending = mutable.Queue.empty[BigInt]
            var cycles = 0
            while (true) {
              dut.clockDomain.waitFallingEdge()
              val ready = !stalled && cycles % 7 != 0
              val response = !stalled && pending.nonEmpty && cycles % 3 != 0
              dut.io.memory.command.ready #= ready
              dut.io.memory.response.valid #= response
              if (response) dut.io.memory.response.payload #= pending.dequeue()
              sleep(1)
              if (dut.io.memory.command.valid.toBoolean && ready) {
                assert(pending.isEmpty)
                val offset = (dut.io.memory.command.address.toBigInt - base).toInt
                val count = dut.io.memory.command.count.toInt
                assert(offset >= 0 && offset + count * 8 <= ((stride * 2 + 5 * bytes + 7) & ~7))
                for (word <- 0 until count) pending.enqueue((0 until 8).foldLeft(BigInt(0)) {
                  (value, b) => value | (BigInt(image(offset + word * 8 + b) & 255) << (8 * b))
                })
              }
              dut.clockDomain.waitSampling()
              cycles += 1
            }
          }
          var goodFrames = 0
          var frame = Vector.empty[Int]
          fork {
            while (true) {
              pixelClock.waitFallingEdge()
              val x = dut.pixel.scan.io.x.toInt
              val y = dut.pixel.scan.io.y.toInt
              if (x == 0 && y == 0) frame = Vector.empty
              if (dut.io.video.de.toBoolean) {
                val rgb = dut.io.video.rgb
                frame :+= (rgb.r.toInt << 16) | (rgb.g.toInt << 8) | rgb.b.toInt
                if (x == 7 && y == 2 && frame == expected) goodFrames += 1
              }
            }
          }
          sleep(80000)
          assert(goodFrames >= 2 && !dut.io.underflow.toBoolean)
          stalled = true
          sleep(450000)
          assert(dut.io.underflow.toBoolean)
          val before = goodFrames
          stalled = false
          sleep(80000)
          assert(goodFrames >= before + 2)
        }
    }
  }
  test("local scanout retires DDR reads before frame handoff under stalls") {
    val timing = VideoTiming(8, 3, 2, 2, 2, 1, 1, 1)
    SimConfig.withIVerilog.compile(MisterScanout(8, 3, timing)).doSim { dut =>
      SimTimeout(500000)
      dut.clockDomain.forkStimulus(10)
      dut.io.pixelClock #= false
      dut.io.pixelReset #= true
      fork { while (true) { sleep(9); dut.io.pixelClock #= !dut.io.pixelClock.toBoolean } }
      dut.io.frame.sequence #= 1
      dut.io.frame.base #= MisterMemoryMap.framebufferBase
      dut.io.frame.width #= 5
      dut.io.frame.height #= 3
      dut.io.frame.stride #= 12
      dut.io.frame.format #= 4
      dut.io.frame.enable #= true
      dut.io.memory.command.ready #= false
      dut.io.memory.response.valid #= false
      dut.io.memory.response.payload #= 0
      sleep(80)
      dut.io.pixelReset #= false
      var outstanding = 0
      var responseAddress = BigInt(0)
      var cycle = 0
      var requests = 0
      var boundaries = 0
      var oldAfterSwitch = 0
      var switched = false
      var pendingSwitch = false
      val bases = mutable.Set.empty[BigInt]
      val base = MisterMemoryMap.framebufferBase
      while (boundaries < 8) {
        dut.clockDomain.waitFallingEdge()
        val ready = cycle % 7 != 0
        val respond = outstanding > 0 && cycle % 5 == 0
        dut.io.memory.command.ready #= ready
        dut.io.memory.response.valid #= respond
        dut.io.memory.response.payload #= BigInt("f800f800f800f800", 16)
        sleep(1)
        if (dut.io.boundary.toBoolean) {
          assert(outstanding == 0)
          boundaries += 1
          if (pendingSwitch && !switched) {
            dut.io.frame.base #= base + MisterMemoryMap.framebufferSlotBytes
            dut.io.frame.sequence #= 2
            dut.io.frame.format #= 6
            dut.io.frame.stride #= 20
            switched = true
          }
        }
        if (dut.io.memory.command.valid.toBoolean && ready) {
          assert(outstanding == 0)
          assert(dut.io.memory.command.read.toBoolean)
          val address = dut.io.memory.command.address.toBigInt
          val count = dut.io.memory.command.count.toInt
          assert((address & 7) == 0 && count > 0 && count <= 16)
          assert(address >= base && address < MisterMemoryMap.framebufferEnd)
          if (switched && address < base + MisterMemoryMap.framebufferSlotBytes) oldAfterSwitch += 1
          responseAddress = address
          bases += address & ~BigInt(0x3fffff)
          outstanding = count
          requests += 1
          if (requests >= 4) pendingSwitch = true
        }
        if (respond) {
          assert(responseAddress >= base)
          responseAddress += 8
          outstanding -= 1
        }
        dut.clockDomain.waitSampling()
        cycle += 1
      }
      assert(requests > 10 && switched && oldAfterSwitch == 0 && bases.size == 2)
      dut.io.frame.enable #= false
      var boundarySeen = false
      for (_ <- 0 until 8000) {
        dut.clockDomain.waitFallingEdge()
        dut.io.memory.command.ready #= true
        val respond = outstanding > 0
        dut.io.memory.response.valid #= respond
        sleep(1)
        if (dut.io.boundary.toBoolean) { assert(outstanding == 0); boundarySeen = true }
        if (dut.io.memory.command.valid.toBoolean) {
          assert(!boundarySeen)
          outstanding = dut.io.memory.command.count.toInt
        }
        if (respond) outstanding -= 1
        dut.clockDomain.waitSampling()
      }
      assert(boundarySeen && outstanding == 0)
    }
  }
}
