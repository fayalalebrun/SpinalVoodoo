package voodoo.mister

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._
import scala.collection.mutable
import scala.language.reflectiveCalls
import scala.util.Random

class MisterMailboxTest extends AnyFunSuite {
  test(
    "mailbox batches are ordered, bounded, immutable until acknowledgment and not replayed after reset"
  ) {
    SimConfig.withIVerilog.compile(MisterMailbox(pollCycles = 3)).doSim { dut =>
      SimTimeout(2000000)
      val random = new Random(91)
      val base = MisterMemoryMap.hostBase.toLong
      val memory = mutable.Map[Long, BigInt]().withDefaultValue(BigInt(0))
      val registers = mutable.Map[Long, Long]().withDefaultValue(0L)
      val responses = mutable.Queue[BigInt]()
      val mmioWrites = mutable.ArrayBuffer[(Long, Long, Int)]()
      val frames = mutable.ArrayBuffer[(Long, Int, Int, Int, Int, Boolean)]()
      val acknowledgments = mutable.ArrayBuffer[(Long, Int, Int)]()
      var readPending: Option[Long] = None
      var framePending: Option[(Long, Boolean)] = None
      var keyBits = BigInt(1) << 0x1c
      var keySequence = 1L
      var stalledMemory: Option[(Long, Boolean, BigInt, Int, Int)] = None
      var stalledMmio: Option[(Long, Boolean, Long, Int)] = None
      def tick(): Unit = {
        dut.io.memory.command.ready #= random.nextInt(3) != 0
        dut.io.memory.response.valid #= false
        dut.io.memory.response.payload #= 0
        dut.io.mmio.waitrequest #= random.nextInt(3) == 0
        dut.io.mmio.readdatavalid #= false
        dut.io.mmio.readdata #= 0
        dut.io.frame.ready #= framePending.isEmpty && random.nextBoolean()
        dut.io.frameAck.valid #= framePending.nonEmpty && random.nextBoolean()
        dut.io.frameAck.sequence #= framePending.map(_._1).getOrElse(0L)
        dut.io.frameAck.error #= framePending.exists(_._2)
        dut.io.keys #= keyBits
        dut.io.keySequence #= keySequence
        sleep(1)
        val command = dut.io.memory.command
        if (command.valid.toBoolean) {
          val current = (
            command.address.toLong,
            command.read.toBoolean,
            command.data.toBigInt,
            command.mask.toInt,
            command.count.toInt
          )
          stalledMemory.foreach(previous => assert(current == previous))
          if (command.ready.toBoolean) {
            stalledMemory = None
            val (address, read, data, mask, count) = current
            assert(address >= base && address < base + 0x3000)
            assert((address & 7) == 0)
            if (read) {
              assert(responses.isEmpty)
              assert(count >= 1 && count <= 64)
              for (beat <- 0 until count) responses.enqueue(memory(address + beat * 8))
            } else {
              assert(count == 1)
              val byteMask = (0 until 8)
                .filter(i => (mask & (1 << i)) != 0)
                .map(i => BigInt(255) << (i * 8))
                .foldLeft(BigInt(0))(_ | _)
              memory(address) = (memory(address) & ~byteMask) | (data & byteMask)
              if (address == base + MisterMailbox.ackOffset) {
                acknowledgments += ((
                  (data & 0xffffffffL).toLong,
                  ((data >> 32) & 255).toInt,
                  ((data >> 40) & 511).toInt
                ))
              }
            }
          } else stalledMemory = Some(current)
        } else assert(stalledMemory.isEmpty)
        val deliver = responses.nonEmpty && random.nextInt(3) != 0
        dut.io.memory.response.valid #= deliver
        dut.io.memory.response.payload #= (if (deliver) responses.front else BigInt(0))
        val mmio = dut.io.mmio
        val active = mmio.read.toBoolean || mmio.write.toBoolean
        if (active) {
          val current = (
            mmio.address.toLong * 4,
            mmio.read.toBoolean,
            mmio.writedata.toLong,
            mmio.byteenable.toInt
          )
          stalledMmio.foreach(previous => assert(current == previous))
          if (!mmio.waitrequest.toBoolean) {
            stalledMmio = None
            val (address, read, data, mask) = current
            if (read) {
              assert(readPending.isEmpty)
              readPending = Some(registers(address))
            } else {
              registers(address) = data
              mmioWrites += ((address, data, mask))
            }
          } else stalledMmio = Some(current)
        } else assert(stalledMmio.isEmpty)
        val deliverRead = readPending.nonEmpty && random.nextBoolean()
        dut.io.mmio.readdatavalid #= deliverRead
        dut.io.mmio.readdata #= readPending.getOrElse(0L)
        val frameAccepted = dut.io.frame.valid.toBoolean && dut.io.frame.ready.toBoolean
        if (frameAccepted) {
          val frame = dut.io.frame
          frames += ((
            frame.base.toLong,
            frame.width.toInt,
            frame.height.toInt,
            frame.stride.toInt,
            frame.format.toInt,
            frame.enable.toBoolean
          ))
        }
        val frameAck = dut.io.frameAck.valid.toBoolean && dut.io.frameAck.ready.toBoolean
        sleep(1)
        dut.clockDomain.waitSampling()
        if (deliver) responses.dequeue()
        if (deliverRead) readPending = None
        if (frameAck) framePending = None
        if (frameAccepted) framePending = Some((dut.io.frame.sequence.toLong, frames.last._2 == 0))
      }
      def awaitAck(seq: Long, error: Int, index: Int): Unit = {
        var cycles = 0
        while (!acknowledgments.lastOption.exists(_._1 == seq) && cycles < 30000) {
          tick()
          cycles += 1
        }
        assert(acknowledgments.lastOption.contains((seq, error, index)))
      }
      def publish(seq: Long, commands: Seq[(Int, Long, Long, Long)], flags: Long = 0): Unit = {
        memory(base + 8) = (BigInt(flags) << 32) | commands.size
        for (((op, address, data, mask), index) <- commands.zipWithIndex) {
          memory(base + MisterMailbox.requestOffset + index * 16) = (BigInt(address) << 32) | op
          memory(base + MisterMailbox.requestOffset + index * 16 + 8) = (BigInt(mask) << 32) | data
        }
        memory(base) = (BigInt(seq) << 32) | MisterMailbox.magic
      }
      dut.io.memory.command.ready #= false
      dut.io.memory.response.valid #= false
      dut.io.memory.response.payload #= 0
      dut.io.mmio.waitrequest #= true
      dut.io.mmio.readdatavalid #= false
      dut.io.mmio.readdata #= 0
      dut.io.frame.ready #= false
      dut.io.frameAck.valid #= false
      dut.io.frameAck.sequence #= 0
      dut.io.frameAck.error #= false
      dut.io.keys #= 0
      dut.io.keySequence #= 0
      publish(17, Seq((1, 0x100L, 99L, 15L)))
      dut.clockDomain.forkStimulus(10)
      dut.clockDomain.waitSampling(5)
      awaitAck(17, 4, 0)
      assert(mmioWrites.isEmpty)
      val commands = (0 until 128).flatMap(i => Seq((1, i * 4L, i + 1L, 15L), (2, i * 4L, 0L, 15L)))
      publish(18, commands)
      awaitAck(18, 0, 256)
      assert(mmioWrites.toSeq == (0 until 128).map(i => (i * 4L, i + 1L, 15)))
      for (i <- 0 until 128)
        assert((memory(base + MisterMailbox.resultOffset + i * 8) >> 32) == i + 1)
      val ackCount = acknowledgments.size
      for (_ <- 0 until 40) tick()
      assert(acknowledgments.size == ackCount)
      assert(mmioWrites.size == 128)
      val framebuffer = MisterMemoryMap.framebufferBase.toLong
      val format = 640L | (480L << 12) | (4L << 24) | (1L << 29)
      publish(19, Seq((3, framebuffer, format, 1280L), (4, 0L, 0L, 0L)))
      while (memory(base + MisterMailbox.keysOffset) == 0) tick()
      keyBits = BigInt(1) << 0x175
      keySequence = 2
      awaitAck(19, 0, 2)
      assert(frames.toSeq == Seq((framebuffer, 640, 480, 1280, 4, true)))
      assert(memory(base + MisterMailbox.keysOffset) == BigInt(1) << 0x1c)
      assert(memory(base + MisterMailbox.keysOffset + 64) == 1)
      publish(20, Seq((3, framebuffer, 0L, 0L)))
      awaitAck(20, 3, 0)
      publish(21, Seq((0, 0L, 0L, 0L)), flags = 1)
      awaitAck(21, 1, 0)
      publish(22, Seq.fill(257)((0, 0L, 0L, 0L)))
      awaitAck(22, 1, 0)
      val invalid = Seq(
        (99, 0L, 0L, 0L),
        (1, 3L, 0L, 15L),
        (2, 0x1000000L, 0L, 15L),
        (1, 0L, 0L, 16L),
        (1, 0L, 0L, 0L),
        (3, framebuffer, 0xc0000000L, 0L),
        (3, framebuffer, format, 0x4000L),
        (4, 1L, 0L, 0L)
      )
      for ((command, i) <- invalid.zipWithIndex) {
        publish(23 + i, Seq((0, 0L, 0L, 0L), command, (1, 0L, 500L, 15L)))
        awaitAck(23 + i, 2, 1)
      }
      assert(mmioWrites.size == 128)
      publish(0xffffffffL, Seq.empty)
      awaitAck(0xffffffffL, 0, 0)
      publish(0, Seq.empty)
      awaitAck(0, 0, 0)
      dut.clockDomain.assertReset()
      responses.clear()
      sleep(30)
      stalledMemory = None
      dut.io.memory.response.valid #= false
      dut.clockDomain.deassertReset()
      acknowledgments.clear()
      awaitAck(0, 4, 0)
      assert(mmioWrites.size == 128)
    }
  }
}
