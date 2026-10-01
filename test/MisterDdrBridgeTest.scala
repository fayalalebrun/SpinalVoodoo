package voodoo.mister

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._
import scala.collection.mutable
import scala.language.reflectiveCalls
import scala.util.Random

class MisterDdrBridgeTest extends AnyFunSuite {
  for (readSlots <- Seq(2, 8)) {
    test(s"$readSlots pending read slots preserve all ports and allow writes while full") {
      SimConfig.withIVerilog.compile(MisterMemoryArbiter(8, readSlots)).doSim { dut =>
        SimTimeout(30000)
        dut.io.ddr.busy #= false
        dut.io.ddr.readDataValid #= false
        dut.io.ddr.readData #= 0
        for ((port, index) <- dut.io.ports.zipWithIndex) {
          port.command.valid #= false
          port.command.address #= 0x3f400000L + index * 128
          port.command.read #= true
          port.command.data #= BigInt("1122334455667788", 16)
          port.command.mask #= 0x5a
          port.command.count #= 2
        }
        dut.clockDomain.forkStimulus(10)
        dut.clockDomain.waitSampling(5)
        for (index <- 0 until readSlots) {
          val port = dut.io.ports(index)
          port.command.valid #= true
          sleep(1)
          assert(port.command.ready.toBoolean)
          dut.clockDomain.waitSampling()
          port.command.valid #= false
        }
        val writer = dut.io.ports.last
        writer.command.valid #= true
        writer.command.read #= false
        writer.command.count #= 1
        dut.io.ddr.busy #= true
        for (_ <- 0 until 3) {
          sleep(1)
          assert(dut.io.ddr.write.toBoolean && !dut.io.ddr.read.toBoolean)
          assert(!writer.command.ready.toBoolean)
          assert(dut.io.ddr.writeData.toBigInt == BigInt("1122334455667788", 16))
          assert(dut.io.ddr.byteEnable.toInt == 0x5a)
          dut.clockDomain.waitSampling()
        }
        dut.io.ddr.busy #= false
        sleep(1)
        assert(writer.command.ready.toBoolean)
        dut.clockDomain.waitSampling()
        writer.command.valid #= false
        for (beat <- 0 until readSlots * 2) {
          dut.io.ddr.readDataValid #= true
          dut.io.ddr.readData #= beat
          sleep(1)
          for ((port, index) <- dut.io.ports.zipWithIndex) {
            assert(port.response.valid.toBoolean == (index == beat / 2))
            if (port.response.valid.toBoolean) assert(port.response.payload.toInt == beat)
          }
          dut.clockDomain.waitSampling()
        }
        dut.io.ddr.readDataValid #= false
        dut.clockDomain.waitSampling(3)
        for ((port, index) <- dut.io.ports.zipWithIndex) {
          port.command.valid #= true
          port.command.read #= true
          port.command.count #= 1
          dut.io.ddr.readDataValid #= true
          dut.io.ddr.readData #= index
          sleep(1)
          assert(port.command.ready.toBoolean && port.response.valid.toBoolean)
          assert(port.response.payload.toInt == index)
          assert(dut.io.ports.count(_.response.valid.toBoolean) == 1)
          dut.clockDomain.waitSampling()
          port.command.valid #= false
          dut.io.ddr.readDataValid #= false
          dut.clockDomain.waitSampling(2)
        }
      }
    }
  }

  test("DDR accepts independent reads and writes before earlier read data returns") {
    SimConfig.withIVerilog.compile(MisterMemoryArbiter(3)).doSim { dut =>
      SimTimeout(20000)
      dut.io.ddr.busy #= false
      dut.io.ddr.readDataValid #= false
      dut.io.ddr.readData #= 0
      for (port <- dut.io.ports) {
        port.command.valid #= false
        port.command.address #= 0x3f400000L
        port.command.read #= true
        port.command.data #= 0
        port.command.mask #= 255
        port.command.count #= 128
      }
      dut.clockDomain.forkStimulus(10)
      dut.clockDomain.waitSampling(5)
      for (index <- 0 until 3) {
        dut.io.ports(index).command.valid #= true
        dut.io.ports(index).command.read #= (index != 2)
        dut.io.ports(index).command.count #= (if (index == 2) 1 else 128)
        sleep(1)
        assert(dut.io.ports(index).command.ready.toBoolean)
        dut.clockDomain.waitSampling()
        dut.io.ports(index).command.valid #= false
      }
      for (beat <- 0 until 256) {
        dut.io.ddr.readDataValid #= true
        dut.io.ddr.readData #= beat
        sleep(1)
        for ((port, index) <- dut.io.ports.zipWithIndex) {
          assert(port.response.valid.toBoolean == (index == beat / 128))
          if (port.response.valid.toBoolean) assert(port.response.payload.toInt == beat)
        }
        dut.clockDomain.waitSampling()
      }
      dut.io.ddr.readDataValid #= false
      dut.clockDomain.waitSampling(3)
      for (index <- 0 until 8) {
        val port = dut.io.ports(index % 3)
        port.command.valid #= true
        port.command.read #= true
        port.command.count #= 128
        sleep(1)
        assert(port.command.ready.toBoolean)
        dut.clockDomain.waitSampling()
        port.command.valid #= false
      }
      dut.io.ports(0).command.valid #= true
      sleep(1)
      assert(!dut.io.ports(0).command.ready.toBoolean && !dut.io.ddr.read.toBoolean)
      dut.io.ddr.busy #= true
      for (beat <- 0 until 1024) {
        dut.io.ddr.readDataValid #= true
        dut.io.ddr.readData #= beat
        sleep(1)
        for ((port, index) <- dut.io.ports.zipWithIndex) {
          assert(port.response.valid.toBoolean == (index == (beat / 128) % 3))
          if (port.response.valid.toBoolean) assert(port.response.payload.toInt == beat)
        }
        dut.clockDomain.waitSampling()
      }
      dut.io.ddr.readDataValid #= false
      dut.io.ddr.busy #= false
      sleep(1)
      assert(dut.io.ports(0).command.ready.toBoolean)
      dut.clockDomain.waitSampling()
      dut.io.ports(0).command.valid #= false
      dut.io.ddr.readDataValid #= true
      for (_ <- 0 until 128) dut.clockDomain.waitSampling()
      dut.io.ddr.readDataValid #= false
      dut.clockDomain.waitSampling(3)
    }
  }

  test("Avalon bursts preserve lanes, masks and ordering across DDR chunks and stalls") {
    SimConfig.withIVerilog.compile(MisterAvalonToDdr()).doSim { dut =>
      SimTimeout(1000000)
      val random = new Random(42)
      val responses = mutable.Queue[BigInt]()
      val returned = mutable.ArrayBuffer[Long]()
      val writes = mutable.ArrayBuffer[(Long, BigInt, Int)]()
      val reads = mutable.ArrayBuffer[(Long, Int)]()
      var stalled: Option[(Long, Boolean, BigInt, Int, Int)] = None
      def word(address: Long): Long = (address ^ 0xa5963c69L) & 0xffffffffL
      def tick(): Boolean = {
        dut.io.memory.command.ready #= random.nextInt(4) != 0
        val respond = responses.nonEmpty && random.nextInt(3) != 0
        dut.io.memory.response.valid #= respond
        dut.io.memory.response.payload #= (if (respond) responses.front else BigInt(0))
        sleep(1)
        if (dut.io.avalon.readDataValid.toBoolean) returned += dut.io.avalon.readData.toLong
        val accepted = dut.io.avalon.waitRequestn.toBoolean
        if (respond) responses.dequeue()
        if (dut.io.memory.command.valid.toBoolean) {
          val cmd = dut.io.memory.command
          val current = (
            cmd.address.toLong,
            cmd.read.toBoolean,
            cmd.data.toBigInt,
            cmd.mask.toInt,
            cmd.count.toInt
          )
          stalled.foreach(previous => assert(current == previous))
          if (cmd.ready.toBoolean) {
            stalled = None
            val (address, read, data, mask, count) = current
            assert((address & 7) == 0)
            assert(count >= 1 && count <= 128)
            if (read) {
              reads += ((address, count))
              for (i <- 0 until count) {
                responses.enqueue(
                  (BigInt(word(address + i * 8 + 4)) << 32) |
                    BigInt(word(address + i * 8))
                )
              }
            } else {
              assert(count == 1)
              writes += ((address, data, mask))
            }
          } else stalled = Some(current)
        } else assert(stalled.isEmpty)
        dut.clockDomain.waitSampling()
        accepted
      }
      dut.io.avalon.read #= false
      dut.io.avalon.write #= false
      dut.io.avalon.address #= 0
      dut.io.avalon.burstCount #= 1
      dut.io.avalon.byteEnable #= 15
      dut.io.avalon.writeData #= 0
      dut.io.memory.command.ready #= false
      dut.io.memory.response.valid #= false
      dut.io.memory.response.payload #= 0
      dut.clockDomain.forkStimulus(10)
      dut.clockDomain.waitSampling(5)
      for (offset <- Seq(0, 4); count <- Seq(1, 2, 3, 255, 256, 257, 1024)) {
        val address = 0x3f400000L + offset
        returned.clear()
        reads.clear()
        dut.io.avalon.address #= address
        dut.io.avalon.burstCount #= count
        dut.io.avalon.read #= true
        while (!tick()) {}
        dut.io.avalon.read #= false
        while (returned.size < count) tick()
        for (_ <- 0 until 5) tick()
        assert(returned.toSeq == (0 until count).map(i => word(address + i * 4)))
        assert(reads.head._1 == (address & ~7L))
        assert(responses.isEmpty)
      }
      for (offset <- Seq(0, 4); count <- Seq(1, 3, 257)) {
        val address = 0x3f800000L + offset
        writes.clear()
        dut.io.avalon.address #= address
        dut.io.avalon.burstCount #= count
        dut.io.avalon.write #= true
        val expected = (0 until count).map { index =>
          val value = word(index)
          val mask = index % 16
          dut.io.avalon.writeData #= value
          dut.io.avalon.byteEnable #= mask
          while (!tick()) {}
          val current = address + index * 4
          val shift = if ((current & 4) != 0) 32 else 0
          (current & ~7L, BigInt(value) << shift, mask << (shift / 8))
        }
        dut.io.avalon.write #= false
        tick()
        assert(writes.toSeq == expected)
      }
      for (
        (address, count, write) <- Seq(
          (1L, 1, false),
          (0L, 0, false),
          (0L, 1025, false),
          (0L, 1, true)
        )
      ) {
        dut.io.avalon.address #= address
        dut.io.avalon.burstCount #= count
        dut.io.avalon.read #= true
        dut.io.avalon.write #= write
        for (_ <- 0 until 5) {
          assert(!tick())
          assert(!dut.io.memory.command.valid.toBoolean)
        }
      }
      dut.io.avalon.read #= false
      dut.io.avalon.write #= false
      dut.clockDomain.assertReset()
      sleep(30)
      dut.clockDomain.deassertReset()
      dut.clockDomain.waitSampling(2)
      assert(!dut.io.avalon.readDataValid.toBoolean)
      assert(!dut.io.memory.command.valid.toBoolean)
    }
  }

  test(
    "DDR arbitration is fair and routes immediate and delayed burst responses only to the owner"
  ) {
    SimConfig.withIVerilog.compile(MisterMemoryArbiter(3)).doSim { dut =>
      SimTimeout(200000)
      val random = new Random(19)
      val sent = Array.fill(3)(0)
      val responses = mutable.Queue[(Int, BigInt)]()
      val owners = mutable.ArrayBuffer[Int]()
      var returned = 0
      var expectedResponses = 0
      var stalled: Option[(Long, Boolean, BigInt, Int, Int)] = None
      dut.io.ddr.busy #= false
      dut.io.ddr.readDataValid #= false
      dut.io.ddr.readData #= 0
      for (port <- dut.io.ports) {
        port.command.valid #= false
        port.command.address #= 0
        port.command.read #= false
        port.command.data #= 0
        port.command.mask #= 0
        port.command.count #= 1
      }
      dut.clockDomain.forkStimulus(10)
      dut.clockDomain.waitSampling(5)
      while (sent.exists(_ < 12) || responses.nonEmpty) {
        for ((port, index) <- dut.io.ports.zipWithIndex) {
          port.command.valid #= sent(index) < 12
          port.command.address #= 0x3f400000L + index * 0x10000 + sent(index) * 1024
          port.command.read #= sent(index) % 3 != 0
          port.command.data #= BigInt(index * 100 + sent(index))
          port.command.mask #= (index + 1) * 17
          port.command.count #= (if (sent(index) % 3 == 0 || sent(index) == 1) 1
                                 else if (sent(index) == 2) 128
                                 else 1 + sent(index) * 3)
        }
        val busy = random.nextBoolean()
        dut.io.ddr.busy #= busy
        dut.io.ddr.readDataValid #= false
        sleep(1)
        val ddr = dut.io.ddr
        val active = ddr.read.toBoolean || ddr.write.toBoolean
        val current = (
          ddr.address.toLong,
          ddr.read.toBoolean,
          ddr.writeData.toBigInt,
          ddr.byteEnable.toInt,
          ddr.burstCount.toInt
        )
        stalled.foreach(previous => assert(active && current == previous))
        stalled = if (active && busy) Some(current) else None
        val accepted = dut.io.ports.indices.filter(i =>
          dut.io.ports(i).command.valid.toBoolean && dut.io.ports(i).command.ready.toBoolean
        )
        assert(accepted.size == (if (active && !busy) 1 else 0))
        if (accepted.nonEmpty) {
          val owner = accepted.head
          val command = dut.io.ports(owner).command
          assert(ddr.address.toLong == command.address.toLong / 8)
          assert(ddr.write.toBoolean != command.read.toBoolean)
          assert(ddr.writeData.toBigInt == command.data.toBigInt)
          assert(ddr.byteEnable.toInt == command.mask.toInt)
          assert(responses.size <= 1024)
          owners += owner
          if (ddr.read.toBoolean) {
            for (beat <- 0 until command.count.toInt) {
              responses.enqueue((owner, (BigInt(owner + 1) << 60) + beat))
              expectedResponses += 1
            }
          }
        }
        val respond = responses.nonEmpty && (accepted.nonEmpty || random.nextBoolean())
        dut.io.ddr.readDataValid #= respond
        dut.io.ddr.readData #= (if (respond) responses.front._2 else BigInt(0))
        sleep(1)
        for ((port, index) <- dut.io.ports.zipWithIndex) {
          assert(port.response.valid.toBoolean == (respond && responses.front._1 == index))
          if (port.response.valid.toBoolean)
            assert(port.response.payload.toBigInt == responses.front._2)
        }
        dut.clockDomain.waitSampling()
        accepted.foreach(i => sent(i) += 1)
        if (respond) {
          responses.dequeue()
          returned += 1
        }
      }
      assert(sent.toSeq == Seq(12, 12, 12))
      assert(returned == expectedResponses)
      assert(owners.grouped(3).forall(_.toSet == Set(0, 1, 2)))
      dut.io.ddr.readDataValid #= false
      dut.io.ports.foreach(_.command.valid #= false)
      dut.clockDomain.waitSampling(5)
    }
  }
}
