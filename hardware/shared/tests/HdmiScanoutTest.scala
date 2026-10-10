//> using target.scope test

package voodoo.hdmi

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._
import spinal.lib.sim._
import voodoo.Config
import scala.collection.mutable
import scala.language.reflectiveCalls

class HdmiScanoutTest extends AnyFunSuite {
  private def identityGammaEntry(index: Int): Int = {
    val value = if (index == 32) 255 else index * 8
    (value << 16) | (value << 8) | value
  }

  test("scanout produces expected active-area count and line/frame cadence") {
    val timing = VideoTiming(
      activeWidth = 8,
      activeHeight = 4,
      hFrontPorch = 2,
      hSync = 2,
      hBackPorch = 2,
      vFrontPorch = 1,
      vSync = 1,
      vBackPorch = 1
    )

    SimConfig.withIVerilog.compile(VideoScanout(timing)).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      dut.io.enable #= true
      dut.clockDomain.waitSampling()

      var activePixels = 0
      var newLines = 0
      var newFrames = 0
      for (_ <- 0 until timing.hTotal * timing.vTotal) {
        if (dut.io.active.toBoolean) activePixels += 1
        if (dut.io.newLine.toBoolean) newLines += 1
        if (dut.io.newFrame.toBoolean) newFrames += 1
        dut.clockDomain.waitSampling()
      }

      assert(activePixels == timing.activeWidth * timing.activeHeight)
      assert(newLines == timing.vTotal)
      assert(newFrames == 1)
    }
  }

  test("test pattern emits color bars inside active area and black during blanking") {
    val timing = VideoTiming(activeWidth = 8, activeHeight = 4)

    SimConfig.withIVerilog.compile(HdmiTestPattern(timing)).doSim { dut =>
      dut.io.active #= true
      dut.io.y #= 1

      dut.io.x #= 0
      sleep(1)
      assert(dut.io.rgb.r.toInt == 255)
      assert(dut.io.rgb.g.toInt == 0)
      assert(dut.io.rgb.b.toInt == 0)

      dut.io.x #= 2
      sleep(1)
      assert(dut.io.rgb.r.toInt == 0)
      assert(dut.io.rgb.g.toInt == 255)
      assert(dut.io.rgb.b.toInt == 0)

      dut.io.x #= 4
      sleep(1)
      assert(dut.io.rgb.r.toInt == 0)
      assert(dut.io.rgb.g.toInt == 0)
      assert(dut.io.rgb.b.toInt == 255)

      dut.io.active #= false
      sleep(1)
      assert(dut.io.rgb.r.toInt == 0)
      assert(dut.io.rgb.g.toInt == 0)
      assert(dut.io.rgb.b.toInt == 0)
    }
  }

  test("TMDS encoder emits DVI control tokens while blanking") {
    SimConfig.withIVerilog.compile(TmdsEncoder()).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      dut.io.data #= 0
      dut.io.dataEnable #= false

      dut.io.control #= 0
      dut.clockDomain.waitSampling()
      assert(dut.io.encoded.toLong == Integer.parseInt("1101010100", 2))

      dut.io.control #= 1
      dut.clockDomain.waitSampling()
      assert(dut.io.encoded.toLong == Integer.parseInt("0010101011", 2))

      dut.io.control #= 2
      dut.clockDomain.waitSampling()
      assert(dut.io.encoded.toLong == Integer.parseInt("0101010100", 2))

      dut.io.control #= 3
      dut.clockDomain.waitSampling()
      assert(dut.io.encoded.toLong == Integer.parseInt("1010101011", 2))
    }
  }

  test("TMDS serializer shifts symbols LSB first") {
    SimConfig.withIVerilog.compile(TmdsSerializer()).doSim { dut =>
      dut.clockDomain.forkStimulus(10)

      dut.io.symbols(0) #= Integer.parseInt("1010101100", 2)
      dut.io.symbols(1) #= Integer.parseInt("0101010011", 2)
      dut.io.symbols(2) #= Integer.parseInt("1111000001", 2)
      dut.io.load #= true
      dut.clockDomain.waitSampling()
      dut.io.load #= false
      sleep(1)

      val expected = Seq(
        Integer.parseInt("1010101100", 2),
        Integer.parseInt("0101010011", 2),
        Integer.parseInt("1111000001", 2)
      )

      for (bit <- 0 until 10) {
        val data = dut.io.data.toInt
        for (lane <- 0 until 3) {
          assert(((data >> lane) & 1) == ((expected(lane) >> bit) & 1))
        }
        dut.clockDomain.waitSampling()
        sleep(1)
      }
    }
  }

  test("CEA 720x480p timing matches the working HDMI mode") {
    val timing = VideoTiming.cea720x480p

    SimConfig.withIVerilog.compile(VideoScanout(timing)).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      dut.io.enable #= true
      dut.clockDomain.waitSampling()

      var activePixels = 0
      var newLines = 0
      var newFrames = 0
      for (_ <- 0 until timing.hTotal * timing.vTotal) {
        if (dut.io.active.toBoolean) activePixels += 1
        if (dut.io.newLine.toBoolean) newLines += 1
        if (dut.io.newFrame.toBoolean) newFrames += 1
        dut.clockDomain.waitSampling()
      }

      assert(activePixels == 720 * 480)
      assert(newLines == 525)
      assert(newFrames == 1)
    }
  }

  test("framebuffer scanout issues front-buffer addresses and flips on vblank swap") {
    val c = Config()
    val timing = VideoTiming(
      activeWidth = 4,
      activeHeight = 2,
      hFrontPorch = 1,
      hSync = 1,
      hBackPorch = 1,
      vFrontPorch = 1,
      vSync = 1,
      vBackPorch = 1
    )

    SimConfig.withIVerilog.compile(HdmiFramebufferScanout(c, timing)).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      dut.io.regs.frontBase #= 0x1000
      dut.io.regs.backBase #= 0x2000
      dut.io.regs.pixelStride #= 64
      dut.io.regs.displayWidth #= timing.activeWidth
      dut.io.regs.displayHeight #= timing.activeHeight
      dut.io.regs.framebufferEnable #= true
      dut.io.regs.testPatternEnable #= false
      for (i <- 0 until 33) dut.io.regs.gammaLut(i) #= identityGammaEntry(i)
      dut.io.swap.valid #= false
      dut.io.readReq.ready #= true
      dut.io.readRsp.valid #= false
      dut.io.readRsp.payload.data #= 0
      dut.clockDomain.waitSampling(20)

      val seenFront = collection.mutable.ArrayBuffer[Long]()
      var frontWait = 0
      while (seenFront.size < 6 && frontWait < timing.hTotal * timing.vTotal * 2) {
        dut.clockDomain.waitSampling()
        sleep(1)
        if (dut.io.readReq.valid.toBoolean) {
          seenFront += dut.io.readReq.payload.address.toLong
        }
        frontWait += 1
      }
      assert(seenFront.size >= 6)

      assert(seenFront.take(4) == Seq(0x1000L, 0x1002L, 0x1004L, 0x1006L))
      assert(seenFront(4) == 0x1080L)
      assert(seenFront(5) == 0x1082L)
      assert(dut.io.status.displayedBuffer.toInt == 0)

      dut.io.swap.valid #= true
      dut.clockDomain.waitSampling()
      dut.io.swap.valid #= false

      var waited = 0
      while (
        dut.io.status.displayedBuffer.toInt == 0 && waited < timing.hTotal * timing.vTotal * 2
      ) {
        dut.clockDomain.waitSampling()
        sleep(1)
        waited += 1
      }
      assert(waited < timing.hTotal * timing.vTotal * 2)
      assert(!dut.io.status.swapPending.toBoolean)
      assert(dut.io.status.swapCount.toInt == 1)

      val seenBack = collection.mutable.ArrayBuffer[Long]()
      var backWait = 0
      while (seenBack.size < 2 && backWait < timing.hTotal * timing.vTotal * 2) {
        dut.clockDomain.waitSampling()
        sleep(1)
        if (dut.io.readReq.valid.toBoolean) {
          seenBack += dut.io.readReq.payload.address.toLong
        }
        backWait += 1
      }

      assert(seenBack.size >= 2)
      assert(seenBack.head >= 0x2000L && seenBack.head < 0x2100L)
    }
  }

  test("framebuffer scanout converts RGB565 read responses to RGB888 output") {
    val c = Config()
    val timing =
      VideoTiming(activeWidth = 4, activeHeight = 2, hFrontPorch = 1, hSync = 1, hBackPorch = 1)

    SimConfig.withIVerilog.compile(HdmiFramebufferScanout(c, timing)).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      dut.io.regs.frontBase #= 0
      dut.io.regs.backBase #= 0
      dut.io.regs.pixelStride #= 64
      dut.io.regs.displayWidth #= timing.activeWidth
      dut.io.regs.displayHeight #= timing.activeHeight
      dut.io.regs.framebufferEnable #= true
      dut.io.regs.testPatternEnable #= false
      for (i <- 0 until 33) dut.io.regs.gammaLut(i) #= identityGammaEntry(i)
      dut.io.swap.valid #= false
      dut.io.readReq.ready #= true
      dut.io.readRsp.valid #= true
      dut.io.readRsp.payload.data #= 0xf800

      dut.clockDomain.waitSampling(2)
      assert(dut.io.video.rgb.r.toInt == 255)
      assert(dut.io.video.rgb.g.toInt == 0)
      assert(dut.io.video.rgb.b.toInt == 0)
    }
  }

  test("framebuffer scanout applies Voodoo gamma CLUT interpolation") {
    val c = Config()
    val timing =
      VideoTiming(activeWidth = 4, activeHeight = 2, hFrontPorch = 1, hSync = 1, hBackPorch = 1)

    SimConfig.withIVerilog.compile(HdmiFramebufferScanout(c, timing)).doSim { dut =>
      dut.clockDomain.forkStimulus(10)
      dut.io.regs.frontBase #= 0
      dut.io.regs.backBase #= 0
      dut.io.regs.pixelStride #= 64
      dut.io.regs.displayWidth #= timing.activeWidth
      dut.io.regs.displayHeight #= timing.activeHeight
      dut.io.regs.framebufferEnable #= true
      dut.io.regs.testPatternEnable #= false
      for (i <- 0 until 33) dut.io.regs.gammaLut(i) #= 0
      dut.io.regs.gammaLut(31) #= 0x100000
      dut.io.regs.gammaLut(32) #= 0x180000
      dut.io.swap.valid #= false
      dut.io.readReq.ready #= true
      dut.io.readRsp.valid #= true
      dut.io.readRsp.payload.data #= 0xf800

      dut.clockDomain.waitSampling(2)
      assert(dut.io.video.rgb.r.toInt == 24)
      assert(dut.io.video.rgb.g.toInt == 0)
      assert(dut.io.video.rgb.b.toInt == 0)
    }
  }

  test("CDC framebuffer scanout prefetches pixels and outputs them in the HDMI clock domain") {
    val c = Config()
    val timing = VideoTiming(
      activeWidth = 4,
      activeHeight = 2,
      hFrontPorch = 1,
      hSync = 1,
      hBackPorch = 1,
      vFrontPorch = 1,
      vSync = 1,
      vBackPorch = 1
    )

    SimConfig.withIVerilog
      .compile(
        HdmiCdcFramebufferScanout(
          c,
          timing,
          fifoDepth = 16,
          prefillLevel = 4,
          refillLowLevel = 4,
          refillHighLevel = 12
        )
      )
      .doSim { dut =>
        dut.io.hdmiClock #= false
        dut.io.hdmiReset #= true
        dut.clockDomain.forkStimulus(10)

        fork {
          while (true) {
            dut.io.hdmiClock #= false
            sleep(5)
            dut.io.hdmiClock #= true
            sleep(5)
          }
        }

        dut.io.regs.frontBase #= 0x1000
        dut.io.regs.backBase #= 0x2000
        dut.io.regs.pixelStride #= 64
        dut.io.regs.displayWidth #= timing.activeWidth
        dut.io.regs.displayHeight #= timing.activeHeight
        dut.io.regs.framebufferEnable #= true
        dut.io.regs.testPatternEnable #= false
        for (i <- 0 until 33) dut.io.regs.gammaLut(i) #= identityGammaEntry(i)
        dut.io.prefetchReq.ready #= true
        dut.io.readReq.ready #= true
        dut.io.readRsp.valid #= false
        dut.io.readRsp.payload.data #= 0

        val pending = mutable.Queue[Int]()
        val seenAddresses = mutable.ArrayBuffer[Long]()
        def pixelFor(address: Long): Int = ((address - 0x1000L) / 2).toInt & 3 match {
          case 0 => 0xf800 // red
          case 1 => 0x07e0 // green
          case 2 => 0x001f // blue
          case _ => 0xffff // white
        }

        fork {
          while (true) {
            dut.clockDomain.waitSampling()
            if (dut.io.readRsp.valid.toBoolean && dut.io.readRsp.ready.toBoolean) {
              dut.io.readRsp.valid #= false
            }
            if (!dut.io.readRsp.valid.toBoolean && pending.nonEmpty) {
              dut.io.readRsp.payload.data #= pending.dequeue()
              dut.io.readRsp.valid #= true
            }
            if (dut.io.readReq.valid.toBoolean && dut.io.readReq.ready.toBoolean) {
              val address = dut.io.readReq.payload.address.toLong
              if (address >= 0x1000L) seenAddresses += address
              pending.enqueue(pixelFor(address))
            }
          }
        }

        dut.clockDomain.waitSampling(4)
        dut.io.hdmiReset #= false
        dut.clockDomain.waitSampling(2)
        dut.io.regs.framebufferEnable #= true

        var addressWait = 0
        while (seenAddresses.size < 4 && addressWait < 200) {
          dut.clockDomain.waitSampling()
          addressWait += 1
        }
        assert(seenAddresses.size >= 4)

        var cycles = 0
        while (dut.io.status.active.toBoolean == false && cycles < 400) {
          dut.clockDomain.waitSampling()
          cycles += 1
        }

        assert(cycles < 400)
        assert(seenAddresses.take(4) == Seq(0x1000L, 0x1002L, 0x1004L, 0x1006L))
        assert(dut.io.fifoPushOccupancy.toInt > 0)
        assert(!dut.io.underflow.toBoolean)

        var sawRed = false
        var sawGreen = false
        var sawBlue = false
        var sampleCycles = 0
        while (!(sawRed && sawGreen && sawBlue) && sampleCycles < 200) {
          if (dut.io.video.de.toBoolean) {
            val rgb = (dut.io.video.rgb.r.toInt, dut.io.video.rgb.g.toInt, dut.io.video.rgb.b.toInt)
            sawRed ||= rgb == (255, 0, 0)
            sawGreen ||= rgb == (0, 255, 0)
            sawBlue ||= rgb == (0, 0, 255)
          }
          dut.clockDomain.waitSampling()
          sampleCycles += 1
        }

        assert(sawRed)
        assert(sawGreen)
        assert(sawBlue)
      }
  }

  test("CDC scanout test-pattern mode drives pattern pixels without FIFO data") {
    val c = Config()
    val timing = VideoTiming(
      activeWidth = 8,
      activeHeight = 2,
      hFrontPorch = 1,
      hSync = 1,
      hBackPorch = 1,
      vFrontPorch = 1,
      vSync = 1,
      vBackPorch = 1
    )

    SimConfig.withIVerilog
      .compile(
        HdmiCdcFramebufferScanout(
          c,
          timing,
          fifoDepth = 16,
          prefillLevel = 4,
          refillLowLevel = 4,
          refillHighLevel = 12
        )
      )
      .doSim { dut =>
        dut.io.hdmiClock #= false
        dut.io.hdmiReset #= true
        dut.clockDomain.forkStimulus(10)

        fork {
          while (true) {
            dut.io.hdmiClock #= false
            sleep(5)
            dut.io.hdmiClock #= true
            sleep(5)
          }
        }

        dut.io.regs.frontBase #= 0
        dut.io.regs.backBase #= 0
        dut.io.regs.pixelStride #= 64
        dut.io.regs.displayWidth #= timing.activeWidth
        dut.io.regs.displayHeight #= timing.activeHeight
        dut.io.regs.framebufferEnable #= true
        dut.io.regs.testPatternEnable #= true
        for (i <- 0 until 33) dut.io.regs.gammaLut(i) #= identityGammaEntry(i)
        dut.io.prefetchReq.ready #= false
        dut.io.readReq.ready #= false
        dut.io.readRsp.valid #= false
        dut.io.readRsp.payload.data #= 0

        dut.clockDomain.waitSampling(4)
        dut.io.hdmiReset #= false

        var sawRed = false
        var sawGreen = false
        var sawBlue = false
        var sawGradient = false
        var cycles = 0
        while (
          !(sawRed && sawGreen && sawBlue && sawGradient) && cycles < timing.hTotal * timing.vTotal * 4
        ) {
          if (dut.io.video.de.toBoolean) {
            val rgb = (dut.io.video.rgb.r.toInt, dut.io.video.rgb.g.toInt, dut.io.video.rgb.b.toInt)
            sawRed ||= rgb == (255, 0, 0)
            sawGreen ||= rgb == (0, 255, 0)
            sawBlue ||= rgb == (0, 0, 255)
            sawGradient ||= rgb._1 != 0 && rgb._2 != 0
          }
          dut.clockDomain.waitSampling()
          cycles += 1
        }

        assert(sawRed)
        assert(sawGreen)
        assert(sawBlue)
        assert(sawGradient)
        assert(dut.io.fifoPushOccupancy.toInt == 0)
        assert(!dut.io.underflow.toBoolean)
      }
  }

  test("CDC framebuffer scanout starts FIFO pixels at HDMI frame origin") {
    val c = Config()
    val timing = VideoTiming(
      activeWidth = 8,
      activeHeight = 4,
      hFrontPorch = 1,
      hSync = 1,
      hBackPorch = 2,
      vFrontPorch = 1,
      vSync = 1,
      vBackPorch = 1
    )

    SimConfig.withIVerilog
      .compile(
        HdmiCdcFramebufferScanout(
          c,
          timing,
          fifoDepth = 64,
          prefillLevel = 8,
          refillLowLevel = 8,
          refillHighLevel = 48
        )
      )
      .doSim { dut =>
        dut.io.hdmiClock #= false
        dut.io.hdmiReset #= true
        dut.clockDomain.forkStimulus(10)

        fork {
          while (true) {
            dut.io.hdmiClock #= false
            sleep(5)
            dut.io.hdmiClock #= true
            sleep(5)
          }
        }

        dut.io.regs.frontBase #= 0x1000
        dut.io.regs.backBase #= 0
        dut.io.regs.pixelStride #= 64
        dut.io.regs.displayWidth #= timing.activeWidth
        dut.io.regs.displayHeight #= timing.activeHeight
        dut.io.regs.framebufferEnable #= true
        dut.io.regs.testPatternEnable #= false
        for (i <- 0 until 33) dut.io.regs.gammaLut(i) #= identityGammaEntry(i)
        dut.io.prefetchReq.ready #= true
        dut.io.readReq.ready #= true
        dut.io.readRsp.valid #= false
        dut.io.readRsp.payload.data #= 0

        val pending = mutable.Queue[Int]()
        def pixelFor(address: Long): Int = ((address - 0x1000L) / 2).toInt & 3 match {
          case 0 => 0xf800 // red at framebuffer origin
          case 1 => 0x07e0
          case 2 => 0x001f
          case _ => 0xffff
        }

        fork {
          var cycles = 0
          while (true) {
            dut.clockDomain.waitSampling()
            cycles += 1
            if (dut.io.readRsp.valid.toBoolean && dut.io.readRsp.ready.toBoolean) {
              dut.io.readRsp.valid #= false
            }
            if (!dut.io.readRsp.valid.toBoolean && pending.nonEmpty && cycles > 30) {
              dut.io.readRsp.payload.data #= pending.dequeue()
              dut.io.readRsp.valid #= true
            }
            if (dut.io.readReq.valid.toBoolean && dut.io.readReq.ready.toBoolean) {
              pending.enqueue(pixelFor(dut.io.readReq.payload.address.toLong))
            }
          }
        }

        dut.clockDomain.waitSampling(4)
        dut.io.hdmiReset #= false

        var firstPixel: Option[(Int, Int, (Int, Int, Int))] = None
        var previousDe = false
        var inactiveCycles = timing.hTotal * timing.vTotal
        var activeX = 0
        var activeY = 0
        val maxCycles = timing.hTotal * timing.vTotal * 4
        for (_ <- 0 until maxCycles if firstPixel.isEmpty) {
          dut.clockDomain.waitSampling()
          sleep(1)
          val de = dut.io.video.de.toBoolean
          if (de && !previousDe) {
            if (inactiveCycles > timing.hTotal) activeY = 0 else activeY += 1
            activeX = 0
            inactiveCycles = 0
          } else if (!de) {
            inactiveCycles += 1
          }

          val rgb = (dut.io.video.rgb.r.toInt, dut.io.video.rgb.g.toInt, dut.io.video.rgb.b.toInt)
          if (de && rgb != (0, 0, 0)) {
            firstPixel = Some((activeX, activeY, rgb))
          }
          if (de) activeX += 1
          previousDe = de
        }

        assert(firstPixel.nonEmpty)
        val (firstX, firstY, firstRgb) = firstPixel.get
        assert(firstRgb == (255, 0, 0))
        assert(
          firstY == 0,
          s"first framebuffer pixel appeared on HDMI row $firstY instead of row 0"
        )
        assert(
          firstX <= 1,
          s"first framebuffer pixel appeared at HDMI x=$firstX instead of frame origin"
        )
        assert(!dut.io.underflow.toBoolean)
      }
  }

  test("CDC scanout pads HDMI pixels outside logical framebuffer width") {
    val c = Config()
    val timing = VideoTiming(
      activeWidth = 4,
      activeHeight = 2,
      hFrontPorch = 1,
      hSync = 1,
      hBackPorch = 1,
      vFrontPorch = 1,
      vSync = 1,
      vBackPorch = 1
    )

    SimConfig.withIVerilog
      .compile(
        HdmiCdcFramebufferScanout(
          c,
          timing,
          fifoDepth = 16,
          prefillLevel = 4,
          refillLowLevel = 4,
          refillHighLevel = 12
        )
      )
      .doSim { dut =>
        dut.io.hdmiClock #= false
        dut.io.hdmiReset #= true
        dut.clockDomain.forkStimulus(10)

        fork {
          while (true) {
            dut.io.hdmiClock #= false
            sleep(5)
            dut.io.hdmiClock #= true
            sleep(5)
          }
        }

        dut.io.regs.frontBase #= 0x1000
        dut.io.regs.backBase #= 0
        dut.io.regs.pixelStride #= 64
        dut.io.regs.displayWidth #= 3
        dut.io.regs.displayHeight #= 2
        dut.io.regs.framebufferEnable #= true
        dut.io.regs.testPatternEnable #= false
        for (i <- 0 until 33) dut.io.regs.gammaLut(i) #= identityGammaEntry(i)
        dut.io.prefetchReq.ready #= true
        dut.io.readReq.ready #= true
        dut.io.readRsp.valid #= false
        dut.io.readRsp.payload.data #= 0xffff

        val seenAddresses = mutable.ArrayBuffer[Long]()
        fork {
          while (true) {
            dut.clockDomain.waitSampling()
            if (dut.io.readRsp.valid.toBoolean && dut.io.readRsp.ready.toBoolean) {
              dut.io.readRsp.valid #= false
            }
            if (!dut.io.readRsp.valid.toBoolean && dut.io.readReq.valid.toBoolean) {
              dut.io.readRsp.payload.data #= 0xffff
              dut.io.readRsp.valid #= true
            }
            if (dut.io.readReq.valid.toBoolean && dut.io.readReq.ready.toBoolean) {
              val address = dut.io.readReq.payload.address.toLong
              if (address >= 0x1000L) seenAddresses += address
            }
          }
        }

        dut.clockDomain.waitSampling(4)
        dut.io.hdmiReset #= false

        var waited = 0
        while (seenAddresses.size < 6 && waited < 200) {
          dut.clockDomain.waitSampling()
          waited += 1
        }

        assert(seenAddresses.size >= 6)
        val expectedFrame = Seq(0x1000L, 0x1002L, 0x1004L, 0x1080L, 0x1082L, 0x1084L)
        assert(seenAddresses.forall(expectedFrame.contains))
        assert(seenAddresses.distinct.size >= 5)
        assert(!seenAddresses.contains(0x1006L))
      }
  }
}
