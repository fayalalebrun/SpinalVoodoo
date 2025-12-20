package voodoo.trace

import java.io.{File, RandomAccessFile}
import scodec._
import scodec.bits._
import scodec.codecs._
import shapeless.{::, HNil}

/** Voodoo trace file header (68 bytes) */
case class TraceHeader(
    magic: Long, // 0x564F4F44 "VOOD"
    version: Long, // Format version (1)
    voodooType: Long, // VOODOO_1, VOODOO_2, etc
    fbSizeMb: Long, // Framebuffer size in MB
    texSizeMb: Long, // Texture size in MB (per TMU)
    numTmus: Long, // Number of TMUs (1 or 2)
    pciBaseAddr: Long, // PCI BAR base address
    flags: Long, // Reserved flags
    cpuSpeedHz: Long, // CPU speed in Hz
    pciSpeedHz: Long, // PCI bus speed in Hz
    entryCount: Long // Total entries
)

/** Voodoo trace entry (24 bytes) */
case class TraceEntry(
    timestamp: Long, // CPU cycles from start
    timestampEnd: Long, // CPU cycles for last occurrence (if count > 1)
    cmdType: Int, // Command type (8 bits)
    addr: Long, // Address within 16MB space (32 bits, unsigned)
    data: Long, // Value written/read (32 bits, unsigned)
    count: Int // Number of times this command was repeated
)

/** Frame index entry from CSV */
case class FrameIndexEntry(
    frameNum: Long, // Frame number
    startCycle: Long, // CPU cycle when frame started
    endCycle: Long, // CPU cycle at end of frame
    startOffset: Long, // Byte offset in trace file where frame starts
    endOffset: Long, // Byte offset in trace file where frame ends
    numCommands: Long, // Number of commands in this frame
    numTriangles: Long, // Triangle count for this frame
    hDisp: Int, // Horizontal resolution
    vDisp: Int, // Vertical resolution
    duration: Long // Frame duration in cycles
)

object TraceCommandType {
  val WRITE_REG_L = 0 // 32-bit register write
  val WRITE_REG_W = 1 // 16-bit register write
  val WRITE_FB_L = 2 // 32-bit framebuffer write
  val WRITE_FB_W = 3 // 16-bit framebuffer write
  val WRITE_TEX_L = 4 // 32-bit texture write
  val WRITE_CMDFIFO = 5 // Write to CMDFIFO region
  val READ_REG_L = 6 // 32-bit register read
  val READ_REG_W = 7 // 16-bit register read
  val READ_FB_L = 8 // 32-bit framebuffer read
  val READ_FB_W = 9 // 16-bit framebuffer read
  val VSYNC = 10 // VSync event (addr=frame_num, data=resolution)
  val SWAP = 11 // Buffer swap (addr=offset)
  val CONFIG = 12 // Display config change (extended entry)
}

object TraceCodecs {
  // Define codec for TraceHeader (64 bytes, little-endian, packed)
  val traceHeaderCodec: Codec[TraceHeader] = {
    ("magic" | uint32L) :: // 4 bytes
      ("version" | uint32L) :: // 4 bytes
      ("voodooType" | uint32L) :: // 4 bytes
      ("fbSizeMb" | uint32L) :: // 4 bytes
      ("texSizeMb" | uint32L) :: // 4 bytes
      ("numTmus" | uint32L) :: // 4 bytes
      ("pciBaseAddr" | uint32L) :: // 4 bytes
      ("flags" | uint32L) :: // 4 bytes (total: 32)
      ("cpuSpeedHz" | longL(64)) :: // 8 bytes (total: 40)
      ("pciSpeedHz" | uint32L) :: // 4 bytes (total: 44)
      ("entryCount" | uint32L) :: // 4 bytes (total: 48)
      ignore(16 * 8) // 16 bytes reserved[4] (total: 64)
  }.as[TraceHeader]

  // Define codec for TraceEntry (24 bytes, little-endian)
  // Format: timestamp(4) | timestamp_end(4) | cmd_type(1) | reserved[3](3) | addr(4) | data(4) | count(4)
  // Note: ignore() returns Unit, so it doesn't appear in the tuple
  private case class TraceEntryRaw(
      timestamp: Long,
      timestampEnd: Long,
      cmdType: Int,
      addr: Long,
      data: Long,
      count: Long
  )

  val traceEntryCodec: Codec[TraceEntry] = (
    uint32L :: // 4 bytes (offset 0) - timestamp
      uint32L :: // 4 bytes (offset 4) - timestamp_end
      uint8 :: // 1 byte  (offset 8) - cmd_type
      ignore(3 * 8) :: // 3 bytes (offset 9-11) - reserved padding (returns Unit, not in tuple)
      uint32L :: // 4 bytes (offset 12) - addr
      uint32L :: // 4 bytes (offset 16) - data
      uint32L // 4 bytes (offset 20) - count (total: 24)
  ).as[TraceEntryRaw]
    .xmap(
      raw =>
        TraceEntry(
          raw.timestamp,
          raw.timestampEnd,
          raw.cmdType,
          raw.addr,
          raw.data,
          raw.count.toInt
        ),
      entry =>
        TraceEntryRaw(
          entry.timestamp,
          entry.timestampEnd,
          entry.cmdType,
          entry.addr,
          entry.data,
          entry.count.toLong
        )
    )

}

class TraceParser(traceFile: File, indexFile: Option[File] = None) {
  import TraceCodecs._

  private val raf = new RandomAccessFile(traceFile, "r")

  // Decode header from first 64 bytes (packed)
  val header: TraceHeader = {
    val headerSize = 64 // Header is now packed, exactly 64 bytes

    val headerBytes = new Array[Byte](headerSize)
    raf.seek(0)
    raf.readFully(headerBytes)

    traceHeaderCodec.decode(BitVector(headerBytes)) match {
      case Attempt.Successful(DecodeResult(h, _)) => h
      case Attempt.Failure(err)                   =>
        throw new RuntimeException(s"Failed to parse trace header: $err")
    }
  }

  val frameIndex: Option[Array[FrameIndexEntry]] = indexFile.map(readFrameIndex)

  private def readFrameIndex(file: File): Array[FrameIndexEntry] = {
    import scala.io.Source
    val source = Source.fromFile(file)
    try {
      val lines = source.getLines().toList
      // Skip header line
      val dataLines = lines.drop(1)

      dataLines.map { line =>
        val fields = line.split(",")
        FrameIndexEntry(
          frameNum = fields(0).toLong,
          startCycle = fields(1).toLong,
          endCycle = fields(2).toLong,
          startOffset = fields(3).toLong,
          endOffset = fields(4).toLong,
          numCommands = fields(5).toLong,
          numTriangles = fields(6).toLong,
          hDisp = fields(7).toInt,
          vDisp = fields(8).toInt,
          duration = fields(9).toLong
        )
      }.toArray
    } finally {
      source.close()
    }
  }

  /** Read all entries from the trace file (streaming) */
  def readAllEntries(): Iterator[TraceEntry] = {
    val headerSize = 64 // Header is packed, 64 bytes
    val entrySize = 24

    new Iterator[TraceEntry] {
      private var currentEntry = 0
      private val totalEntries = header.entryCount.toInt

      def hasNext: Boolean = currentEntry < totalEntries

      def next(): TraceEntry = {
        val offset = headerSize + currentEntry * entrySize
        val entryBytes = new Array[Byte](entrySize)

        raf.seek(offset)
        raf.readFully(entryBytes)

        val result = traceEntryCodec.decode(BitVector(entryBytes)) match {
          case Attempt.Successful(DecodeResult(entry, _)) => entry
          case Attempt.Failure(err)                       =>
            throw new RuntimeException(
              s"Failed to parse entry $currentEntry at offset $offset: $err"
            )
        }

        currentEntry += 1
        result
      }
    }
  }

  /** Read entries for a specific frame */
  def readFrameEntries(frameNum: Int): Option[Iterator[TraceEntry]] = {
    val entrySize = 24

    frameIndex.flatMap { frames =>
      // Find the frame by its frameNum field, not by array index
      // (frames in the CSV are 1-indexed and may have gaps)
      frames.find(_.frameNum == frameNum).map { frame =>
        new Iterator[TraceEntry] {
          private var currentOffset = frame.startOffset
          private val endOffset = frame.endOffset

          def hasNext: Boolean = currentOffset < endOffset

          def next(): TraceEntry = {
            val entryBytes = new Array[Byte](entrySize)

            raf.seek(currentOffset)
            raf.readFully(entryBytes)

            val result = traceEntryCodec.decode(BitVector(entryBytes)) match {
              case Attempt.Successful(DecodeResult(entry, _)) => entry
              case Attempt.Failure(err)                       =>
                throw new RuntimeException(s"Failed to parse entry at offset $currentOffset: $err")
            }

            currentOffset += entrySize
            result
          }
        }
      }
    }
  }

  def close(): Unit = {
    raf.close()
  }
}
