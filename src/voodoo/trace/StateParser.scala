package voodoo.trace

import java.io.{File, RandomAccessFile}
import scodec._
import scodec.bits._
import scodec.codecs._

/** Voodoo state file header (64 bytes) */
case class StateHeader(
    magic: Long, // 0x41545356 "VSTA"
    version: Long, // Format version
    frameNum: Long, // Frame number
    voodooType: Long, // VOODOO_1, VOODOO_2, etc
    fbSize: Long, // Framebuffer size in bytes
    texSize: Long, // Texture size per TMU in bytes
    numTmus: Long, // Number of TMUs (1 or 2)
    regCount: Long, // Number of register entries
    flags: Long, // Reserved flags
    // Framebuffer layout info for conversion
    rowWidth: Long, // Row stride in bytes
    drawOffset: Long, // Color buffer offset in fb_mem
    auxOffset: Long, // Depth buffer offset in fb_mem
    hDisp: Long, // Horizontal display resolution
    vDisp: Long // Vertical display resolution
)

/** Voodoo state register entry (8 bytes) */
case class StateRegister(
    addr: Long, // Register address
    value: Long // Register value
)

/** Loaded state containing all data from a state file */
case class VoodooState(
    header: StateHeader,
    registers: Array[StateRegister],
    framebuffer: Array[Byte],
    texture0: Array[Byte],
    texture1: Option[Array[Byte]]
)

object StateCodecs {
  // Header codec (64 bytes, little-endian)
  val stateHeaderCodec: Codec[StateHeader] = {
    ("magic" | uint32L) ::
      ("version" | uint32L) ::
      ("frameNum" | uint32L) ::
      ("voodooType" | uint32L) ::
      ("fbSize" | uint32L) ::
      ("texSize" | uint32L) ::
      ("numTmus" | uint32L) ::
      ("regCount" | uint32L) ::
      ("flags" | uint32L) ::
      ("rowWidth" | uint32L) ::
      ("drawOffset" | uint32L) ::
      ("auxOffset" | uint32L) ::
      ("hDisp" | uint32L) ::
      ("vDisp" | uint32L) ::
      ignore(2 * 32) // reserved[2] - 8 bytes
  }.as[StateHeader]

  // Register entry codec (8 bytes, little-endian)
  val stateRegisterCodec: Codec[StateRegister] = {
    ("addr" | uint32L) ::
      ("value" | uint32L)
  }.as[StateRegister]
}

/** Parser for Voodoo state dump files */
class StateParser(stateFile: File) {
  import StateCodecs._

  private val raf = new RandomAccessFile(stateFile, "r")

  // Decode header from first 64 bytes
  val header: StateHeader = {
    val headerSize = 64
    val headerBytes = new Array[Byte](headerSize)
    raf.seek(0)
    raf.readFully(headerBytes)

    stateHeaderCodec.decode(BitVector(headerBytes)) match {
      case Attempt.Successful(DecodeResult(h, _)) => h
      case Attempt.Failure(err)                   =>
        throw new RuntimeException(s"Failed to parse state header: $err")
    }
  }

  /** Read all registers from the state file */
  def readRegisters(): Array[StateRegister] = {
    val headerSize = 64
    val regSize = 8
    val numRegs = header.regCount.toInt

    val registers = new Array[StateRegister](numRegs)
    val regBytes = new Array[Byte](regSize)

    for (i <- 0 until numRegs) {
      val offset = headerSize + i * regSize
      raf.seek(offset)
      raf.readFully(regBytes)

      registers(i) = stateRegisterCodec.decode(BitVector(regBytes)) match {
        case Attempt.Successful(DecodeResult(reg, _)) => reg
        case Attempt.Failure(err)                     =>
          throw new RuntimeException(s"Failed to parse register $i: $err")
      }
    }

    registers
  }

  /** Read framebuffer data from the state file */
  def readFramebuffer(): Array[Byte] = {
    val headerSize = 64
    val regSize = 8
    val numRegs = header.regCount.toInt
    val fbOffset = headerSize + numRegs * regSize
    val fbSize = header.fbSize.toInt

    val fbData = new Array[Byte](fbSize)
    raf.seek(fbOffset)
    raf.readFully(fbData)

    fbData
  }

  /** Read texture memory for TMU 0 */
  def readTexture0(): Array[Byte] = {
    val headerSize = 64
    val regSize = 8
    val numRegs = header.regCount.toInt
    val fbSize = header.fbSize.toInt
    val texSize = header.texSize.toInt

    val texOffset = headerSize + numRegs * regSize + fbSize
    val texData = new Array[Byte](texSize)
    raf.seek(texOffset)
    raf.readFully(texData)

    texData
  }

  /** Read texture memory for TMU 1 (if present) */
  def readTexture1(): Option[Array[Byte]] = {
    if (header.numTmus < 2) return None

    val headerSize = 64
    val regSize = 8
    val numRegs = header.regCount.toInt
    val fbSize = header.fbSize.toInt
    val texSize = header.texSize.toInt

    val texOffset = headerSize + numRegs * regSize + fbSize + texSize
    val texData = new Array[Byte](texSize)
    raf.seek(texOffset)
    raf.readFully(texData)

    Some(texData)
  }

  /** Load complete state from file */
  def loadState(): VoodooState = {
    VoodooState(
      header = header,
      registers = readRegisters(),
      framebuffer = readFramebuffer(),
      texture0 = readTexture0(),
      texture1 = readTexture1()
    )
  }

  def close(): Unit = {
    raf.close()
  }
}

object StateParser {
  val STATE_MAGIC = 0x41545356L // "VSTA"

  /** Find state file for a given frame in the trace directory */
  def findStateFile(traceDir: File, frameNum: Int): Option[File] = {
    val stateFile = new File(traceDir, f"state_$frameNum%04d.bin")
    if (stateFile.exists()) Some(stateFile) else None
  }

  /** Get the trace directory from a trace file path */
  def getTraceDir(traceFile: File): File = {
    // The trace directory is the parent of the trace.bin file
    traceFile.getParentFile
  }
}
