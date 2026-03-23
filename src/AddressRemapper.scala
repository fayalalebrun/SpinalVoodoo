package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._

/** Address Remapper for Voodoo register access
  *
  * When fbiInit3.remap is enabled, addresses with bit 21 set use a different register layout
  * (RGBZASTW order) for float triangle registers. This component translates those addresses to the
  * standard layout before they reach RegisterBank.
  *
  * For addresses with bit 21 set:
  *   - If in remapped float range (0x088-0x0fc base): translate RGBZASTW → standard layout
  *   - Otherwise: clear bit 21 to access regular registers
  */
case class AddressRemapper(inputParams: BmbParameter, outputParams: BmbParameter)
    extends Component {
  val io = new Bundle {
    val input = slave(Bmb(inputParams))
    val output = master(Bmb(outputParams))
  }

  // Address mapping from RGBZASTW layout to standard layout
  // Key: remapped address (lower 12 bits), Value: standard address
  val remapTable = Map(
    // Vertices (same in both layouts)
    0x088 -> 0x088, // fvertexAx
    0x08c -> 0x08c, // fvertexAy
    0x090 -> 0x090, // fvertexBx
    0x094 -> 0x094, // fvertexBy
    0x098 -> 0x098, // fvertexCx
    0x09c -> 0x09c, // fvertexCy
    // Red (start, dX, dY)
    0x0a0 -> 0x0a0, // fstartR
    0x0a4 -> 0x0c0, // fdRdX
    0x0a8 -> 0x0e0, // fdRdY
    // Green
    0x0ac -> 0x0a4, // fstartG
    0x0b0 -> 0x0c4, // fdGdX
    0x0b4 -> 0x0e4, // fdGdY
    // Blue
    0x0b8 -> 0x0a8, // fstartB
    0x0bc -> 0x0c8, // fdBdX
    0x0c0 -> 0x0e8, // fdBdY
    // Z
    0x0c4 -> 0x0ac, // fstartZ
    0x0c8 -> 0x0cc, // fdZdX
    0x0cc -> 0x0ec, // fdZdY
    // Alpha
    0x0d0 -> 0x0b0, // fstartA
    0x0d4 -> 0x0d0, // fdAdX
    0x0d8 -> 0x0f0, // fdAdY
    // S texture coord
    0x0dc -> 0x0b4, // fstartS
    0x0e0 -> 0x0d4, // fdSdX
    0x0e4 -> 0x0f4, // fdSdY
    // T texture coord
    0x0e8 -> 0x0b8, // fstartT
    0x0ec -> 0x0d8, // fdTdX
    0x0f0 -> 0x0f8, // fdTdY
    // W
    0x0f4 -> 0x0bc, // fstartW
    0x0f8 -> 0x0dc, // fdWdX
    0x0fc -> 0x0fc // fdWdY
  )

  // Extract address components
  val inputAddr = io.input.cmd.address
  val baseAddr = inputAddr(11 downto 0)
  val hasBit21 = inputAddr(21)

  // Check if address is in remapped float range (0x088-0x0fc)
  val inRemapRange = baseAddr >= 0x088 && baseAddr <= 0x0fc

  // Build translated address using switch/case for the remap table
  val outputAddrWidth = outputParams.access.addressWidth
  val translatedAddr = UInt(outputAddrWidth bits)

  when(hasBit21) {
    when(inRemapRange) {
      // Translate RGBZASTW layout to standard layout using switch
      translatedAddr := baseAddr.resized // default
      switch(baseAddr) {
        for ((from, to) <- remapTable) {
          is(U(from, 12 bits)) {
            translatedAddr := U(to, outputAddrWidth bits)
          }
        }
      }
    } otherwise {
      // Clear bit 21 to access regular registers (just use lower bits)
      translatedAddr := (inputAddr & ~(U(1, inputAddr.getWidth bits) |<< 21)).resized
    }
  } otherwise {
    // No translation needed
    translatedAddr := inputAddr.resized
  }

  // Connect command stream with translated address
  io.output.cmd.arbitrationFrom(io.input.cmd)
  io.output.cmd.last := io.input.cmd.last
  io.output.cmd.opcode := io.input.cmd.opcode
  io.output.cmd.address := translatedAddr
  io.output.cmd.length := io.input.cmd.length
  io.output.cmd.data := io.input.cmd.data
  io.output.cmd.mask := io.input.cmd.mask
  io.output.cmd.context := io.input.cmd.context
  io.output.cmd.source := io.input.cmd.source

  // Pass response back unchanged
  io.input.rsp << io.output.rsp
}
