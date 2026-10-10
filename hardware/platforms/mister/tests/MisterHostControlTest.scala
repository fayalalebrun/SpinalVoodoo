package voodoo.mister

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._
import scala.language.reflectiveCalls

class MisterHostControlTest extends AnyFunSuite {
  test("frame publication is atomic at vblank and acknowledgments withstand backpressure") {
    SimConfig.withIVerilog.compile(MisterFramebufferControl()).doSim { dut =>
      SimTimeout(20000)
      dut.clockDomain.forkStimulus(10)
      dut.io.submit.valid #= false
      dut.io.acknowledge.ready #= false
      dut.io.vblank #= false
      dut.clockDomain.waitSampling(5)
      assert(!dut.io.active.enable.toBoolean)
      def submit(
          sequence: Long,
          base: BigInt,
          width: Int = 640,
          height: Int = 480,
          stride: Int = 1280,
          format: Int = 4,
          enable: Boolean = true
      ): Unit = {

        dut.io.submit.sequence #= sequence
        dut.io.submit.base #= base
        dut.io.submit.width #= width
        dut.io.submit.height #= height
        dut.io.submit.stride #= stride
        dut.io.submit.format #= format
        dut.io.submit.enable #= enable
        dut.io.submit.valid #= true
        dut.clockDomain.waitSamplingWhere(dut.io.submit.ready.toBoolean)
        dut.io.submit.valid #= false
        dut.clockDomain.waitSampling()
      }
      def acknowledge(sequence: Long, error: Boolean): Unit = {

        assert(dut.io.acknowledge.valid.toBoolean)
        for (_ <- 0 until 7) {
          assert(dut.io.acknowledge.sequence.toLong == sequence)
          assert(dut.io.acknowledge.error.toBoolean == error)
          assert(!dut.io.submit.ready.toBoolean)
          dut.clockDomain.waitSampling()
        }
        dut.io.acknowledge.ready #= true
        dut.clockDomain.waitSampling()
        dut.io.acknowledge.ready #= false
        dut.clockDomain.waitSampling()
      }
      val base = MisterMemoryMap.framebufferBase
      submit(1, base)
      dut.clockDomain.waitSampling(10)
      assert(!dut.io.active.enable.toBoolean)
      assert(!dut.io.acknowledge.valid.toBoolean)
      assert(!dut.io.submit.ready.toBoolean)
      dut.io.vblank #= true
      dut.clockDomain.waitSampling(2)
      assert(dut.io.active.base.toBigInt == base)
      assert(dut.io.active.width.toInt == 640)
      assert(dut.io.active.height.toInt == 480)
      assert(dut.io.active.stride.toInt == 1280)
      assert(dut.io.active.enable.toBoolean)
      acknowledge(1, false)
      submit(2, base + MisterMemoryMap.framebufferSlotBytes, 800, 600, 3200, 6)
      dut.clockDomain.waitSampling(5)
      assert(dut.io.active.sequence.toLong == 1)
      dut.io.vblank #= false
      dut.clockDomain.waitSampling(2)
      dut.io.vblank #= true
      dut.clockDomain.waitSampling(2)
      assert(dut.io.active.sequence.toLong == 2)
      assert(dut.io.active.format.toInt == 6)
      acknowledge(2, false)
      val invalid = Seq(
        (base - 8, 640, 480, 1280, 4),
        (base + 8, 640, 480, 1280, 4),
        (MisterMemoryMap.framebufferEnd, 640, 480, 1280, 4),
        (BigInt("fffffff8", 16), 640, 480, 1280, 4),
        (base, 0, 480, 1280, 4),
        (base, 1921, 480, 8000, 4),
        (base, 640, 0, 1280, 4),
        (base, 640, 1201, 1280, 4),
        (base, 640, 480, 1276, 4),
        (base, 640, 480, 1281, 4),
        (base, 640, 480, 1280, 3),
        (base, 1920, 1200, 7680, 6),
        (base, 4095, 4095, 16380, 6),
        (base, 1024, 1025, 4096, 6)
      )
      for (((address, width, height, stride, format), index) <- invalid.zipWithIndex) {
        submit(index + 3, address, width, height, stride, format)
        acknowledge(index + 3, true)
        assert(dut.io.active.sequence.toLong == 2)
      }
      submit(100, 0, 0, 0, 0, 0, false)
      dut.io.vblank #= false
      dut.clockDomain.waitSampling(2)
      dut.io.vblank #= true
      dut.clockDomain.waitSampling(2)
      acknowledge(100, false)
      assert(!dut.io.active.enable.toBoolean)
      val rendererBase = BigInt("3f400000", 16)
      for (
        (address, index) <- Seq(
          rendererBase,
          rendererBase + 150 * 4096,
          MisterMemoryMap.rendererEnd - 150 * 4096
        ).zipWithIndex
      ) {
        submit(110 + index, address)
        dut.io.vblank #= false
        dut.clockDomain.waitSampling(2)
        dut.io.vblank #= true
        dut.clockDomain.waitSampling(2)
        acknowledge(110 + index, false)
        assert(dut.io.active.base.toBigInt == address)
      }
      for (
        (address, index) <- Seq(
          rendererBase + 4,
          rendererBase - 4096,
          BigInt("3f800000", 16) - 149 * 4096
        ).zipWithIndex
      ) {
        submit(120 + index, address)
        acknowledge(120 + index, true)
      }
      submit(130, rendererBase, 640, 480, 2560, 6)
      acknowledge(130, true)
      submit(131, base, 1024, 1024, 4096, 6)
      dut.io.vblank #= false
      dut.clockDomain.waitSampling(2)
      dut.io.vblank #= true
      dut.clockDomain.waitSampling(2)
      acknowledge(131, false)
      dut.clockDomain.assertReset()
      sleep(30)
      dut.clockDomain.deassertReset()
      dut.clockDomain.waitSampling(2)
      assert(!dut.io.active.enable.toBoolean)
      assert(!dut.io.acknowledge.valid.toBoolean)
    }
  }

  test("USB-derived PS2 key state handles extended keys, releases and OSD focus") {
    SimConfig.withIVerilog.compile(MisterKeyboardState()).doSim { dut =>
      SimTimeout(20000)
      dut.clockDomain.forkStimulus(10)
      dut.io.ps2Key #= 0
      dut.io.osdOpen #= false
      dut.clockDomain.waitSampling(5)
      var toggle = 0
      def event(key: Int, pressed: Boolean): Unit = {
        toggle ^= 1024
        dut.io.ps2Key #= toggle | (if (pressed) 512 else 0) | key
        dut.clockDomain.waitSampling(3)
      }
      event(0x1c, true)
      event(0x175, true)
      assert(dut.io.keys.toBigInt == (BigInt(1) << 0x1c) + (BigInt(1) << 0x175))
      assert(dut.io.sequence.toLong == 2)
      dut.clockDomain.waitSampling(20)
      assert(dut.io.sequence.toLong == 2)
      event(0x1c, false)
      assert(dut.io.keys.toBigInt == BigInt(1) << 0x175)
      dut.io.osdOpen #= true
      dut.clockDomain.waitSampling(3)
      assert(dut.io.keys.toBigInt == 0)
      assert(dut.io.sequence.toLong == 4)
      event(0x1c, true)
      assert(dut.io.keys.toBigInt == 0)
      assert(dut.io.sequence.toLong == 4)
      dut.io.osdOpen #= false
      dut.clockDomain.waitSampling(3)
      assert(dut.io.keys.toBigInt == 0)
      event(0x175, false)
      event(0x1c, true)
      assert(dut.io.keys.toBigInt == BigInt(1) << 0x1c)
      dut.clockDomain.assertReset()
      sleep(30)
      assert(dut.io.keys.toBigInt == 0)
    }
  }
}
