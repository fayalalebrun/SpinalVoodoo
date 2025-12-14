package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._

/** Texture Mapping Unit (TMU)
  *
  * Performs texture sampling for a single texture unit. Two TMUs are instantiated and chained
  * sequentially (TMU0 → TMU1) to support multitexturing.
  *
  * Pipeline stages:
  *   1. Perspective correction: S' = S/W, T' = T/W (skip if W ≈ 1)
  *   2. Address generation: compute texel address from S', T'
  *   3. Memory fetch: read RGB565 texel via BMB bus
  *   4. Format conversion: RGB565 → RGB888
  *   5. Output: pass texture color downstream
  *
  * Initial implementation supports:
  *   - Point sampling only (nearest texel)
  *   - RGB565 texture format only
  *   - BMB bus interface for texture memory
  */
case class Tmu(c: voodoo.Config) extends Component {
  val io = new Bundle {
    // Input stream (from Rasterizer for TMU0, from TMU0 for TMU1)
    // Config is now part of the input stream (captured per-triangle)
    val input = slave Stream (Tmu.Input(c))

    // Output stream (to TMU1 for TMU0, to ColorCombine for TMU1)
    val output = master Stream (Tmu.Output(c))

    // Texture memory read bus
    val texRead = master(Bmb(Tmu.bmbParams(c)))
  }

  // State machine for texture fetch
  object TmuState extends SpinalEnum {
    val IDLE, FETCHING, DONE = newElement()
  }
  val state = RegInit(TmuState.IDLE)

  // Registers to hold input data during fetch
  val inputReg = Reg(Tmu.Input(c))
  val texelData = Reg(Bits(16 bits))

  // Texture coordinate calculations (simplified: assume W = 1.0)
  // Extract integer texel coordinates from S and T
  // S and T are in 14.18 format, we take the integer part
  val sInt = io.input.payload.s.floor(0).asSInt
  val tInt = io.input.payload.t.floor(0).asSInt

  // For now, assume 256x256 texture and just take lower 8 bits
  val texWidthBits = 8 // 256 texels
  val texHeightBits = 8 // 256 texels
  val texelX = sInt(texWidthBits - 1 downto 0).asUInt.resize(10 bits)
  val texelY = tInt(texHeightBits - 1 downto 0).asUInt.resize(10 bits)

  // Compute texture memory address: base + (y * stride + x) * 2 (RGB565 = 2 bytes)
  // Use config from the input stream (captured per-triangle)
  val texStride = U(256, 10 bits)
  val texelOffset = (texelY.resize(20 bits) * texStride.resize(20 bits) + texelX.resize(20 bits))
  val texAddr = (io.input.payload.config.texBaseAddr.resize(c.addressWidth.value bits) +
    (texelOffset << 1).resize(c.addressWidth.value bits))

  // BMB command interface
  io.texRead.cmd.valid := (state === TmuState.FETCHING)
  io.texRead.cmd.fragment.address := RegNextWhen(texAddr, io.input.fire)
  io.texRead.cmd.fragment.opcode := Bmb.Cmd.Opcode.READ
  io.texRead.cmd.fragment.length := 1 // 2 bytes - 1
  io.texRead.cmd.fragment.source := 0
  io.texRead.cmd.last := True

  // BMB response interface
  io.texRead.rsp.ready := (state === TmuState.FETCHING)

  // State machine logic
  io.input.ready := (state === TmuState.IDLE)

  switch(state) {
    is(TmuState.IDLE) {
      when(io.input.valid) {
        inputReg := io.input.payload
        state := TmuState.FETCHING
      }
    }
    is(TmuState.FETCHING) {
      when(io.texRead.cmd.fire) {
        // Command sent, wait for response
      }
      when(io.texRead.rsp.fire) {
        texelData := io.texRead.rsp.fragment.data(15 downto 0)
        state := TmuState.DONE
      }
    }
    is(TmuState.DONE) {
      when(io.output.fire) {
        state := TmuState.IDLE
      }
    }
  }

  // RGB565 to RGB888 conversion
  // RGB565: RRRRRGGG_GGGBBBBB (bits 15-11: R, bits 10-5: G, bits 4-0: B)
  val r5 = texelData(15 downto 11).asUInt
  val g6 = texelData(10 downto 5).asUInt
  val b5 = texelData(4 downto 0).asUInt

  // Expand to 8 bits by replicating MSBs to fill LSBs
  val r8 = r5 @@ r5(4 downto 2)
  val g8 = g6 @@ g6(5 downto 4)
  val b8 = b5 @@ b5(4 downto 2)

  // Output stream
  io.output.valid := (state === TmuState.DONE)
  io.output.payload.coords := inputReg.coords
  io.output.payload.texture.r := r8
  io.output.payload.texture.g := g8
  io.output.payload.texture.b := b8
  io.output.payload.textureAlpha := U(255, 8 bits) // RGB565 has no alpha
  io.output.payload.grads := inputReg.grads
}

object Tmu {

  /** Per-TMU configuration (captured per-triangle) */
  case class TmuConfig() extends Bundle {
    val textureMode = Bits(32 bits) // Texture mode register
    val texBaseAddr = UInt(24 bits) // Texture base address
  }

  /** TMU input bundle */
  case class Input(c: voodoo.Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val s = AFix(c.texCoordsFormat) // This TMU's S coordinate (14.18)
    val t = AFix(c.texCoordsFormat) // This TMU's T coordinate (14.18)
    val w = AFix(c.wFormat) // Shared W coordinate (2.30)
    val cOther = Color.u8() // RGB from upstream (zeros for TMU0)
    val aOther = UInt(8 bits) // Alpha from upstream (zero for TMU0)
    val grads = Rasterizer.GradientBundle(AFix(_), c)
    val config = TmuConfig() // Per-triangle TMU configuration
  }

  /** TMU output bundle */
  case class Output(c: voodoo.Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val texture = Color.u8() // Texture RGB
    val textureAlpha = UInt(8 bits) // Texture alpha
    val grads = Rasterizer.GradientBundle(AFix(_), c)
  }

  /** BMB parameters for texture memory access */
  def bmbParams(c: voodoo.Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = 2,
    canRead = true,
    canWrite = false,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}
