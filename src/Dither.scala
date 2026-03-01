package voodoo

import spinal.core._
import spinal.lib._

/** Synchronous ROM ordered-dither module with Stream interface.
  *
  * Converts 8-bit R/G/B to 5-6-5 bit using ordered dithering with either a 4x4 or 2x2 Bayer matrix,
  * matching the 86Box vid_voodoo_dither.h lookup tables exactly.
  *
  * Uses Mem-based synchronous ROMs (block RAM) with `streamReadSync` for the R channel (which
  * provides the Stream handshake and backpressure), and `readSync` for G/B channels piggy-backing
  * on the same fire signal. Each channel has one 5120-entry ROM packing both 4x4 (0-4095) and 2x2
  * (4096-5119) tables. The 4x4 table is exactly 2^12 entries so the 2x2 base is page-aligned (bit
  * 12 set, no adder needed).
  */
case class Dither() extends Component {
  val io = new Bundle {
    val input = slave Stream (Dither.Input())
    val output = master Stream (Dither.Output())
  }

  // Synchronous ROMs: 3 independent single-port Mems (R, G, B)
  // R and B are initialized with identical rb data but are separate Mems
  // Layout: [0-4095] = 4x4 table, [4096-5119] = 2x2 table
  val romR = Mem(UInt(5 bits), DitherTables.rbPacked.map(v => U(v, 5 bits)))
  val romG = Mem(UInt(6 bits), DitherTables.gPacked.map(v => U(v, 6 bits)))
  val romB = Mem(UInt(5 bits), DitherTables.rbPacked.map(v => U(v, 5 bits)))

  val in = io.input.payload

  // Address computation: pack 4x4 and 2x2 into one address space
  // 4x4: v(8) ## y(2) ## x(2) = 12 bits, range [0, 4095]
  // 2x2: 1 ## v(8) ## y(1) ## x(1) = bit12 set + 10 bits, range [4096, 5119]
  def addr4x4(v: UInt): UInt = (v ## in.y ## in.x).asUInt
  def addr2x2(v: UInt): UInt = (v ## in.y(0) ## in.x(0)).asUInt
  def packedAddr(v: UInt): UInt = {
    // 2x2: 1 ## addr2x2(10).resize(12) = 13 bits, bit 12 set, range [4096, 5119]
    // 4x4: addr4x4(12).resize(13) = 13 bits, range [0, 4095]
    in.use2x2 ? (True ## addr2x2(v).resize(12 bits)).asUInt | addr4x4(v).resize(13 bits)
  }

  // Bypass values to link through the ROM pipeline (for when dithering is disabled)
  val linked = Dither.Linked()
  linked.enable := in.enable
  linked.truncR := (in.r >> 3).resize(5 bits)
  linked.truncG := (in.g >> 2).resize(6 bits)
  linked.truncB := (in.b >> 3).resize(5 bits)

  // R channel: streamReadSync manages the Stream handshake + backpressure.
  // linkedData carries bypass values through the pipeline register.
  val addrStream = Stream(UInt(13 bits))
  addrStream.valid := io.input.valid
  addrStream.payload := packedAddr(in.r)
  io.input.ready := addrStream.ready

  val romRResult = romR.streamReadSync(addrStream, linked)

  // G and B channels: readSync piggy-backing on the same fire signal
  val ditGRom = romG.readSync(packedAddr(in.g), addrStream.fire)
  val ditBRom = romB.readSync(packedAddr(in.b), addrStream.fire)

  // Output: mux between ROM and truncated bypass based on linked enable
  val rLinked = romRResult.payload.linked
  io.output.valid := romRResult.valid
  io.output.payload.ditR := rLinked.enable ? romRResult.payload.value | rLinked.truncR
  io.output.payload.ditG := rLinked.enable ? ditGRom | rLinked.truncG
  io.output.payload.ditB := rLinked.enable ? ditBRom | rLinked.truncB
  romRResult.ready := io.output.ready
}

object Dither {
  case class Input() extends Bundle {
    val r = UInt(8 bits)
    val g = UInt(8 bits)
    val b = UInt(8 bits)
    val x = UInt(2 bits)
    val y = UInt(2 bits)
    val enable = Bool()
    val use2x2 = Bool()
  }

  case class Output() extends Bundle {
    val ditR = UInt(5 bits)
    val ditG = UInt(6 bits)
    val ditB = UInt(5 bits)
  }

  /** Internal linked data piped through streamReadSync alongside the ROM read. */
  case class Linked() extends Bundle {
    val enable = Bool()
    val truncR = UInt(5 bits)
    val truncG = UInt(6 bits)
    val truncB = UInt(5 bits)
  }
}
