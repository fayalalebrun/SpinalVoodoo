package voodoo.trace

import org.scalatest.funsuite.AnyFunSuite
import java.io.{File, FileOutputStream, DataOutputStream}
import java.nio.{ByteBuffer, ByteOrder}

class StateParserTest extends AnyFunSuite {

  /** Create a test state file with known data */
  def createTestStateFile(
      frameNum: Int = 42,
      numRegs: Int = 3,
      fbSize: Int = 64, // Small for testing
      texSize: Int = 32, // Small for testing
      numTmus: Int = 2,
      rowWidth: Int = 2048,
      drawOffset: Int = 0,
      auxOffset: Int = 4096,
      hDisp: Int = 640,
      vDisp: Int = 480
  ): File = {
    val file = File.createTempFile("state_test_", ".bin")
    file.deleteOnExit()

    val fos = new FileOutputStream(file)
    val buffer = ByteBuffer.allocate(64 + numRegs * 8 + fbSize + texSize * numTmus)
    buffer.order(ByteOrder.LITTLE_ENDIAN)

    // Write header (64 bytes)
    buffer.putInt(0x41545356) // magic "VSTA"
    buffer.putInt(1) // version
    buffer.putInt(frameNum) // frame_num
    buffer.putInt(0) // voodoo_type (VOODOO_1)
    buffer.putInt(fbSize) // fb_size
    buffer.putInt(texSize) // tex_size
    buffer.putInt(numTmus) // num_tmus
    buffer.putInt(numRegs) // reg_count
    buffer.putInt(0) // flags
    // Layout info
    buffer.putInt(rowWidth) // row_width
    buffer.putInt(drawOffset) // draw_offset
    buffer.putInt(auxOffset) // aux_offset
    buffer.putInt(hDisp) // h_disp
    buffer.putInt(vDisp) // v_disp
    // reserved[2] - 8 bytes
    for (_ <- 0 until 2) buffer.putInt(0)

    // Write registers (8 bytes each: addr, value)
    buffer.putInt(0x210) // reg 0 addr: fbiInit0
    buffer.putInt(0xdeadbeef) // reg 0 value
    buffer.putInt(0x214) // reg 1 addr: fbiInit1
    buffer.putInt(0xcafebabe) // reg 1 value
    buffer.putInt(0x110) // reg 2 addr: fbzMode
    buffer.putInt(0x12345678) // reg 2 value

    // Write framebuffer data (pattern: index mod 256)
    for (i <- 0 until fbSize) {
      buffer.put((i % 256).toByte)
    }

    // Write texture 0 data (pattern: 0xA0 + index mod 32)
    for (i <- 0 until texSize) {
      buffer.put((0xa0 + (i % 32)).toByte)
    }

    // Write texture 1 data if numTmus > 1 (pattern: 0xB0 + index mod 32)
    if (numTmus > 1) {
      for (i <- 0 until texSize) {
        buffer.put((0xb0 + (i % 32)).toByte)
      }
    }

    fos.write(buffer.array())
    fos.close()
    file
  }

  test("StateParser should parse header correctly") {
    val file = createTestStateFile(frameNum = 42)
    val parser = new StateParser(file)

    assert(parser.header.magic == 0x41545356L, "Magic should be VSTA")
    assert(parser.header.version == 1L, "Version should be 1")
    assert(parser.header.frameNum == 42L, "Frame number should be 42")
    assert(parser.header.voodooType == 0L, "Voodoo type should be 0 (VOODOO_1)")
    assert(parser.header.fbSize == 64L, "FB size should be 64")
    assert(parser.header.texSize == 32L, "Tex size should be 32")
    assert(parser.header.numTmus == 2L, "Num TMUs should be 2")
    assert(parser.header.regCount == 3L, "Reg count should be 3")
    // Layout info
    assert(parser.header.rowWidth == 2048L, "Row width should be 2048")
    assert(parser.header.drawOffset == 0L, "Draw offset should be 0")
    assert(parser.header.auxOffset == 4096L, "Aux offset should be 4096")
    assert(parser.header.hDisp == 640L, "H disp should be 640")
    assert(parser.header.vDisp == 480L, "V disp should be 480")

    parser.close()
    file.delete()
  }

  test("StateParser should read registers correctly") {
    val file = createTestStateFile()
    val parser = new StateParser(file)
    val regs = parser.readRegisters()

    assert(regs.length == 3, "Should have 3 registers")

    assert(regs(0).addr == 0x210L, "Reg 0 addr should be 0x210")
    assert(regs(0).value == 0xdeadbeefL, "Reg 0 value should be 0xDEADBEEF")

    assert(regs(1).addr == 0x214L, "Reg 1 addr should be 0x214")
    assert(regs(1).value == 0xcafebabeL, "Reg 1 value should be 0xCAFEBABE")

    assert(regs(2).addr == 0x110L, "Reg 2 addr should be 0x110")
    assert(regs(2).value == 0x12345678L, "Reg 2 value should be 0x12345678")

    parser.close()
    file.delete()
  }

  test("StateParser should read framebuffer correctly") {
    val file = createTestStateFile(fbSize = 64)
    val parser = new StateParser(file)
    val fb = parser.readFramebuffer()

    assert(fb.length == 64, "Framebuffer should be 64 bytes")

    // Verify pattern: index mod 256
    for (i <- fb.indices) {
      val expected = (i % 256).toByte
      assert(fb(i) == expected, s"FB byte $i should be $expected, got ${fb(i)}")
    }

    parser.close()
    file.delete()
  }

  test("StateParser should read texture 0 correctly") {
    val file = createTestStateFile(texSize = 32)
    val parser = new StateParser(file)
    val tex0 = parser.readTexture0()

    assert(tex0.length == 32, "Texture 0 should be 32 bytes")

    // Verify pattern: 0xA0 + index mod 32
    for (i <- tex0.indices) {
      val expected = (0xa0 + (i % 32)).toByte
      assert(tex0(i) == expected, s"Tex0 byte $i should be $expected, got ${tex0(i)}")
    }

    parser.close()
    file.delete()
  }

  test("StateParser should read texture 1 correctly when present") {
    val file = createTestStateFile(texSize = 32, numTmus = 2)
    val parser = new StateParser(file)
    val tex1Opt = parser.readTexture1()

    assert(tex1Opt.isDefined, "Texture 1 should be present for 2 TMUs")
    val tex1 = tex1Opt.get

    assert(tex1.length == 32, "Texture 1 should be 32 bytes")

    // Verify pattern: 0xB0 + index mod 32
    for (i <- tex1.indices) {
      val expected = (0xb0 + (i % 32)).toByte
      assert(tex1(i) == expected, s"Tex1 byte $i should be $expected, got ${tex1(i)}")
    }

    parser.close()
    file.delete()
  }

  test("StateParser should return None for texture 1 with single TMU") {
    val file = createTestStateFile(numTmus = 1)
    val parser = new StateParser(file)
    val tex1Opt = parser.readTexture1()

    assert(tex1Opt.isEmpty, "Texture 1 should not be present for 1 TMU")

    parser.close()
    file.delete()
  }

  test("StateParser loadState should return complete state") {
    val file =
      createTestStateFile(frameNum = 99, numRegs = 3, fbSize = 64, texSize = 32, numTmus = 2)
    val parser = new StateParser(file)
    val state = parser.loadState()

    // Verify header
    assert(state.header.frameNum == 99L)
    assert(state.header.regCount == 3L)

    // Verify registers
    assert(state.registers.length == 3)
    assert(state.registers(0).value == 0xdeadbeefL)

    // Verify framebuffer
    assert(state.framebuffer.length == 64)
    assert(state.framebuffer(0) == 0.toByte)
    assert(state.framebuffer(63) == 63.toByte)

    // Verify texture 0
    assert(state.texture0.length == 32)
    assert(state.texture0(0) == 0xa0.toByte)

    // Verify texture 1
    assert(state.texture1.isDefined)
    assert(state.texture1.get.length == 32)
    assert(state.texture1.get(0) == 0xb0.toByte)

    parser.close()
    file.delete()
  }

  test("StateParser.findStateFile should find existing state file") {
    val tempDir =
      new File(System.getProperty("java.io.tmpdir"), "state_test_" + System.currentTimeMillis())
    tempDir.mkdirs()
    tempDir.deleteOnExit()

    // Create state_0005.bin
    val stateFile = new File(tempDir, "state_0005.bin")
    val fos = new FileOutputStream(stateFile)
    fos.write(new Array[Byte](64)) // Dummy header
    fos.close()
    stateFile.deleteOnExit()

    val found = StateParser.findStateFile(tempDir, 5)
    assert(found.isDefined, "Should find state_0005.bin")
    assert(found.get.getName == "state_0005.bin")

    val notFound = StateParser.findStateFile(tempDir, 999)
    assert(notFound.isEmpty, "Should not find state_0999.bin")

    stateFile.delete()
    tempDir.delete()
  }

  test("StateParser magic constant should match") {
    assert(StateParser.STATE_MAGIC == 0x41545356L, "STATE_MAGIC should be VSTA")
  }
}
