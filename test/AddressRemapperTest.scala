package voodoo

import spinal.core._
import spinal.core.sim._
import spinal.lib.bus.bmb._
import org.scalatest.funsuite.AnyFunSuite

class AddressRemapperTest extends AnyFunSuite {
  val cfg = Config.voodoo1()
  val inputParams = RegisterBank.externalBmbParams(cfg)
  val outputParams = RegisterBank.bmbParams(cfg)

  def testRemap(inputAddr: Long, expectedOutput: Long): Unit = {
    SimConfig.withIVerilog.compile(AddressRemapper(inputParams, outputParams)).doSim { dut =>
      dut.clockDomain.forkStimulus(period = 10)

      // Initialize
      dut.io.input.cmd.valid #= false
      dut.io.output.cmd.ready #= true
      dut.clockDomain.waitSampling()

      // Send command with test address
      dut.io.input.cmd.valid #= true
      dut.io.input.cmd.address #= inputAddr
      dut.io.input.cmd.opcode #= 1 // Write
      dut.io.input.cmd.length #= 3
      dut.io.input.cmd.data #= 0x12345678L
      dut.io.input.cmd.mask #= 0xf
      dut.io.input.cmd.last #= true
      sleep(1)

      // Check output address
      val outputAddr = dut.io.output.cmd.address.toLong
      assert(
        outputAddr == expectedOutput,
        f"Address 0x$inputAddr%06x should map to 0x$expectedOutput%03x, got 0x$outputAddr%06x"
      )

      dut.io.input.cmd.valid #= false
    }
  }

  test("Standard addresses pass through unchanged") {
    // Standard register address (no bit 21)
    testRemap(0x000088, 0x000088) // fvertexAx
    testRemap(0x0000a0, 0x0000a0) // fstartR
    testRemap(0x0000c0, 0x0000c0) // fdRdX
    testRemap(0x000100, 0x000100) // Other standard register
  }

  test("Remapped vertices stay the same") {
    // Vertices are at same addresses in both layouts
    testRemap(0x200088, 0x000088) // fvertexAx
    testRemap(0x20008c, 0x00008c) // fvertexAy
    testRemap(0x200090, 0x000090) // fvertexBx
    testRemap(0x200094, 0x000094) // fvertexBy
    testRemap(0x200098, 0x000098) // fvertexCx
    testRemap(0x20009c, 0x00009c) // fvertexCy
  }

  test("Remapped Red values translate correctly") {
    // Red: start stays, but gradients move
    testRemap(0x2000a0, 0x0000a0) // fstartR -> fstartR (same)
    testRemap(0x2000a4, 0x0000c0) // fdRdX (remapped) -> fdRdX (standard)
    testRemap(0x2000a8, 0x0000e0) // fdRdY (remapped) -> fdRdY (standard)
  }

  test("Remapped Green values translate correctly") {
    testRemap(0x2000ac, 0x0000a4) // fstartG (remapped) -> fstartG (standard)
    testRemap(0x2000b0, 0x0000c4) // fdGdX (remapped) -> fdGdX (standard)
    testRemap(0x2000b4, 0x0000e4) // fdGdY (remapped) -> fdGdY (standard)
  }

  test("Remapped Blue values translate correctly") {
    testRemap(0x2000b8, 0x0000a8) // fstartB (remapped) -> fstartB (standard)
    testRemap(0x2000bc, 0x0000c8) // fdBdX (remapped) -> fdBdX (standard)
    testRemap(0x2000c0, 0x0000e8) // fdBdY (remapped) -> fdBdY (standard)
  }

  test("Remapped Z values translate correctly") {
    testRemap(0x2000c4, 0x0000ac) // fstartZ (remapped) -> fstartZ (standard)
    testRemap(0x2000c8, 0x0000cc) // fdZdX (remapped) -> fdZdX (standard)
    testRemap(0x2000cc, 0x0000ec) // fdZdY (remapped) -> fdZdY (standard)
  }

  test("Remapped Alpha values translate correctly") {
    testRemap(0x2000d0, 0x0000b0) // fstartA (remapped) -> fstartA (standard)
    testRemap(0x2000d4, 0x0000d0) // fdAdX (remapped) -> fdAdX (standard)
    testRemap(0x2000d8, 0x0000f0) // fdAdY (remapped) -> fdAdY (standard)
  }

  test("Remapped S texture coord values translate correctly") {
    testRemap(0x2000dc, 0x0000b4) // fstartS (remapped) -> fstartS (standard)
    testRemap(0x2000e0, 0x0000d4) // fdSdX (remapped) -> fdSdX (standard)
    testRemap(0x2000e4, 0x0000f4) // fdSdY (remapped) -> fdSdY (standard)
  }

  test("Remapped T texture coord values translate correctly") {
    testRemap(0x2000e8, 0x0000b8) // fstartT (remapped) -> fstartT (standard)
    testRemap(0x2000ec, 0x0000d8) // fdTdX (remapped) -> fdTdX (standard)
    testRemap(0x2000f0, 0x0000f8) // fdTdY (remapped) -> fdTdY (standard)
  }

  test("Remapped W values translate correctly") {
    testRemap(0x2000f4, 0x0000bc) // fstartW (remapped) -> fstartW (standard)
    testRemap(0x2000f8, 0x0000dc) // fdWdX (remapped) -> fdWdX (standard)
    testRemap(0x2000fc, 0x0000fc) // fdWdY (remapped) -> fdWdY (standard)
  }

  test("Bit 21 set but outside remap range clears bit 21") {
    // Addresses with bit 21 set but not in float triangle range
    // should just clear bit 21 to access standard registers
    testRemap(0x200000, 0x000000) // status register
    testRemap(0x200004, 0x000004) // intrCtrl
    testRemap(0x200100, 0x000100) // fbiInit0
    testRemap(0x200200, 0x000200) // some other register
  }
}
