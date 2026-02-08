package voodoo

import spinal.core._
import spinal.lib._
import spinal.lib.misc.pipeline._

/** Color Combine Unit enums and types */
object ColorCombine {
  // RGB source selection (fbzColorPath bits 1:0)
  object RgbSel extends SpinalEnum {
    val ITERATED, TEXTURE, COLOR1, LFB = newElement()
  }

  // Alpha source selection (fbzColorPath bits 3:2)
  object AlphaSel extends SpinalEnum {
    val ITERATED, TEXTURE, COLOR1, LFB = newElement()
  }

  // Local color selection for CCU (fbzColorPath bit 4)
  object LocalSel extends SpinalEnum {
    val ITERATED, COLOR0 = newElement()
  }

  // Local alpha selection for ACU (fbzColorPath bits 6:5)
  object AlphaLocalSel extends SpinalEnum {
    val ITERATED, COLOR0, ITERATED_Z = newElement()
  }

  // Multiply factor selection (fbzColorPath bits 12:10 for CCU, bits 21:19 for ACU)
  object MSelect extends SpinalEnum {
    val ZERO, CLOCAL, AOTHER, ALOCAL, TEXTURE_ALPHA, TEXTURE_RGB = newElement()
  }

  // Add mode (fbzColorPath bits 15:14 for CCU, bits 24:23 for ACU)
  object AddMode extends SpinalEnum {
    val NONE, CLOCAL, ALOCAL = newElement()
  }

  /** Configuration bundle for Color Combine Unit - decoded from fbzColorPath */
  case class Config() extends Bundle {
    // RGB channel controls
    val rgbSel = RgbSel()
    val localSelect = LocalSel()
    val localSelectOverride = Bool()
    val zeroOther = Bool()
    val subClocal = Bool()
    val mselect = MSelect()
    val reverseBlend = Bool()
    val add = AddMode()
    val invertOutput = Bool()

    // Alpha channel controls
    val alphaSel = AlphaSel()
    val alphaLocalSelect = AlphaLocalSel()
    val alphaZeroOther = Bool()
    val alphaSubClocal = Bool()
    val alphaMselect = MSelect()
    val alphaReverseBlend = Bool()
    val alphaAdd = AddMode()
    val alphaInvertOutput = Bool()

    // Texture enable
    val textureEnable = Bool()
  }

  /** Input to Color Combine Unit */
  case class Input(c: voodoo.Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val iterated = Color.u8() // Iterated (Gouraud) colors, 0-255
    val iteratedAlpha = UInt(8 bits) // Iterated alpha, 0-255
    val iteratedZ = UInt(8 bits) // Upper bits of Z for alpha local select
    val depth = AFix(c.vDepthFormat)

    // Texture colors from TMU chain
    val texture = Color.u8()
    val textureAlpha = UInt(8 bits)

    // Constant colors from registers
    val color0 = Color.u8()
    val color1 = Color.u8()

    // Configuration (decoded enums)
    val config = ColorCombine.Config()
  }

  /** Output from Color Combine Unit */
  case class Output(c: voodoo.Config) extends Bundle {
    val coords = Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits))
    val color = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))
    val alpha = UInt(8 bits)
    val depth = AFix(c.vDepthFormat)
  }
}

/** Color Combine Unit - combines iterated colors with texture and constants */
case class ColorCombine(c: voodoo.Config) extends Component {
  val io = new Bundle {
    val input = slave Stream (ColorCombine.Input(c))
    val output = master Stream (ColorCombine.Output(c))
  }

  import ColorCombine._

  // Define pipeline payloads (Stageables)
  val COORDS = Payload(Vec.fill(2)(SInt(c.vertexFormat.nonFraction bits)))
  val DEPTH = Payload(AFix(c.vDepthFormat))
  val CONFIG = Payload(ColorCombine.Config())

  // Color payloads at various precisions
  val C_OTHER = Payload(Color.s9())
  val C_LOCAL = Payload(Color.s9())
  val A_OTHER = Payload(SInt(9 bits))
  val A_LOCAL = Payload(SInt(9 bits))

  val TEXTURE_ALPHA = Payload(SInt(9 bits))
  val TEXTURE_RGB = Payload(Color.s9())

  // After zero/sub operations (wider for headroom)
  val C_AFTER_ZERO = Payload(Color.s9())
  val A_AFTER_ZERO = Payload(SInt(9 bits))
  // Use AFix for subtraction results to enable proper fixed-point multiplication
  // AFix.SQ(intBits, fracBits) - 10 integer bits, 0 fractional bits
  val C_AFTER_SUB = Payload(
    Color(AFix.SQ(10 bits, 0 bits), AFix.SQ(10 bits, 0 bits), AFix.SQ(10 bits, 0 bits))
  )
  val A_AFTER_SUB = Payload(AFix.SQ(10 bits, 0 bits))

  // Multiply factors (before and after reverse blend) - UQ(0,8) represents 0.0 to ~1.0
  val factorFormat = AFix.UQ(0 bits, 8 bits)
  val C_FACTOR_PRE = Payload(Color.ufactor())
  val A_FACTOR_PRE = Payload(AFix.UQ(0 bits, 8 bits))
  val C_FACTOR = Payload(Color(AFix.UQ(1 bits, 8 bits), AFix.UQ(1 bits, 8 bits), AFix.UQ(1 bits, 8 bits)))
  val A_FACTOR = Payload(AFix.UQ(1 bits, 8 bits))

  // After multiply/add (wide for intermediate results)
  // SQ(10, 0) * UQ(0, 8) = SQ(10, 8) - 10 integer bits + 8 fractional bits
  // QFormat(width, fraction, signed) - total width = 10 + 8 + 1 (sign) = 19
  val colorMulFormat = QFormat(19, 8, true)
  val C_AFTER_MUL = Payload(
    Color(AFix.SQ(10 bits, 8 bits), AFix.SQ(10 bits, 8 bits), AFix.SQ(10 bits, 8 bits))
  )
  val A_AFTER_MUL = Payload(AFix.SQ(10 bits, 8 bits))
  val C_AFTER_ADD = Payload(
    Color(AFix.SQ(10 bits, 8 bits), AFix.SQ(10 bits, 8 bits), AFix.SQ(10 bits, 8 bits))
  )
  val A_AFTER_ADD = Payload(AFix.SQ(10 bits, 8 bits))

  // Final clamped values
  val C_CLAMPED = Payload(Color.u8())
  val A_CLAMPED = Payload(UInt(8 bits))

  // Helper to clamp signed integer value to unsigned 0-255, returned as UQ(0,8) factor (0.0 to ~1.0)
  def clampSIntToFactor(v: SInt): AFix = {
    val clamped = v.max(S(0, v.getWidth bits)).min(255).asUInt.resize(8 bits)
    val result = AFix.UQ(0 bits, 8 bits)
    result := clamped
    result
  }

  // Helper to clamp AFix (integer format) to UQ(0,8) factor
  def clampAFixToFactor(v: AFix): AFix = {
    // Saturate to 0-255 range, then convert to UQ(0,8) format
    v.sat(satMax = 255, satMin = 0).fixTo(UQ(0, 8))
  }

  // Helper to clamp signed value to unsigned 0-255 (for final output)
  def clampToU8(v: SInt): UInt = v.max(S(0, v.getWidth bits)).min(255).asUInt.resize(8 bits)

  // Helper to clamp AFix to unsigned 0-255 (for final output)
  def clampAFixToU8(v: AFix): UInt = {
    // Saturate to 0-255 at integer resolution, then truncate fractional bits
    val saturated = v.sat(satMax = 255, satMin = 0, exp = 0 exp)
    // fixTo UQ(8,0) to get an 8-bit unsigned integer
    saturated.fixTo(UQ(8, 0)).asUInt
  }

  // Create pipeline nodes
  val n0, n1, n2, n3, n4, n5, n6, n7, n8 = Node()

  // Link nodes with stage registers
  val links = List(
    StageLink(n0, n1),
    StageLink(n1, n2),
    StageLink(n2, n3),
    StageLink(n3, n4),
    StageLink(n4, n5),
    StageLink(n5, n6),
    StageLink(n6, n7),
    StageLink(n7, n8)
  )

  // Connect input stream to first node
  n0.driveFrom(io.input) { (self, payload) =>
    self(COORDS) := payload.coords
    self(DEPTH) := payload.depth
    self(CONFIG) := payload.config

    // Helper to convert UInt(8) to SInt(9) by zero-extending
    def u8ToS9(v: UInt): SInt = (False ## v).asSInt
    def u8ColorToS9(dst: Color[SInt], src: Color[UInt]): Unit = {
      (dst.channels, src.channels).zipped.foreach { (d, s) =>
        d := u8ToS9(s)
      }
    }

    // Convert texture inputs to internal 9-bit signed format
    self(TEXTURE_ALPHA) := u8ToS9(payload.textureAlpha)
    u8ColorToS9(self(TEXTURE_RGB), payload.texture)

    // Select c_other based on rgb_sel
    switch(payload.config.rgbSel) {
      is(RgbSel.ITERATED) {
        u8ColorToS9(self(C_OTHER), payload.iterated)
      }
      is(RgbSel.TEXTURE) {
        u8ColorToS9(self(C_OTHER), payload.texture)
      }
      is(RgbSel.COLOR1) {
        u8ColorToS9(self(C_OTHER), payload.color1)
      }
      is(RgbSel.LFB) {
        self(C_OTHER).foreach(_ := 0)
      }
    }

    // Select c_local based on localselect (with override from texture alpha)
    when(payload.config.localSelectOverride && payload.textureAlpha(7)) {
      u8ColorToS9(self(C_LOCAL), payload.color0)
    }.otherwise {
      switch(payload.config.localSelect) {
        is(LocalSel.ITERATED) {
          u8ColorToS9(self(C_LOCAL), payload.iterated)
        }
        is(LocalSel.COLOR0) {
          u8ColorToS9(self(C_LOCAL), payload.color0)
        }
      }
    }

    // Select a_other based on alpha_sel
    switch(payload.config.alphaSel) {
      is(AlphaSel.ITERATED) {
        self(A_OTHER) := u8ToS9(payload.iteratedAlpha)
      }
      is(AlphaSel.TEXTURE) {
        self(A_OTHER) := u8ToS9(payload.textureAlpha)
      }
      is(AlphaSel.COLOR1) {
        self(A_OTHER) := u8ToS9(payload.color1.r)
      }
      is(AlphaSel.LFB) {
        self(A_OTHER) := 0
      }
    }

    // Select a_local based on alphaLocalSelect
    switch(payload.config.alphaLocalSelect) {
      is(AlphaLocalSel.ITERATED) {
        self(A_LOCAL) := u8ToS9(payload.iteratedAlpha)
      }
      is(AlphaLocalSel.COLOR0) {
        self(A_LOCAL) := u8ToS9(payload.color0.r)
      }
      is(AlphaLocalSel.ITERATED_Z) {
        self(A_LOCAL) := u8ToS9(payload.iteratedZ)
      }
    }
  }

  // Stage 1: Zero Other
  (n1(C_AFTER_ZERO).channels, n1(C_OTHER).channels).zipped.foreach { (dst, src) =>
    dst := n1(CONFIG).zeroOther ? S(0, 9 bits) | src
  }
  n1(A_AFTER_ZERO) := n1(CONFIG).alphaZeroOther ? S(0, 9 bits) | n1(A_OTHER)

  // Stage 2: Subtract Local
  // Convert SInt to AFix(SQ(10,0)) for the subtraction results
  (n2(C_AFTER_SUB).channels, n2(C_AFTER_ZERO).channels, n2(C_LOCAL).channels).zipped.foreach {
    (dst, zero, local) =>
      when(n2(CONFIG).subClocal) {
        dst := AFix(zero - local)
      }.otherwise {
        dst := AFix(zero)
      }
  }
  when(n2(CONFIG).alphaSubClocal) {
    n2(A_AFTER_SUB) := AFix(n2(A_AFTER_ZERO) - n2(A_LOCAL))
  }.otherwise {
    n2(A_AFTER_SUB) := AFix(n2(A_AFTER_ZERO))
  }

  // Stage 3: Select Multiply Factor (RGB)
  val zeroFactor = AFix.UQ(0 bits, 8 bits)
  zeroFactor := 0.0

  switch(n3(CONFIG).mselect) {
    is(MSelect.ZERO) {
      n3(C_FACTOR_PRE).foreach(_ := zeroFactor)
    }
    is(MSelect.CLOCAL) {
      (n3(C_FACTOR_PRE).channels, n3(C_LOCAL).channels).zipped.foreach { (dst, src) =>
        dst := clampSIntToFactor(src)
      }
    }
    is(MSelect.AOTHER) {
      val a = clampAFixToFactor(n3(A_AFTER_SUB))
      n3(C_FACTOR_PRE).foreach(_ := a)
    }
    is(MSelect.ALOCAL) {
      val a = clampSIntToFactor(n3(A_LOCAL))
      n3(C_FACTOR_PRE).foreach(_ := a)
    }
    is(MSelect.TEXTURE_ALPHA) {
      val a = clampSIntToFactor(n3(TEXTURE_ALPHA))
      n3(C_FACTOR_PRE).foreach(_ := a)
    }
    is(MSelect.TEXTURE_RGB) {
      (n3(C_FACTOR_PRE).channels, n3(TEXTURE_RGB).channels).zipped.foreach { (dst, src) =>
        dst := clampSIntToFactor(src)
      }
    }
  }

  // Stage 3: Select Multiply Factor (Alpha)
  switch(n3(CONFIG).alphaMselect) {
    is(MSelect.ZERO) {
      n3(A_FACTOR_PRE) := zeroFactor
    }
    is(MSelect.CLOCAL) {
      n3(A_FACTOR_PRE) := clampSIntToFactor(n3(A_LOCAL))
    }
    is(MSelect.AOTHER) {
      n3(A_FACTOR_PRE) := clampAFixToFactor(n3(A_AFTER_SUB))
    }
    is(MSelect.ALOCAL) {
      n3(A_FACTOR_PRE) := clampSIntToFactor(n3(A_LOCAL))
    }
    is(MSelect.TEXTURE_ALPHA) {
      n3(A_FACTOR_PRE) := clampSIntToFactor(n3(TEXTURE_ALPHA))
    }
    is(MSelect.TEXTURE_RGB) {
      n3(A_FACTOR_PRE) := clampSIntToFactor(n3(TEXTURE_ALPHA))
    }
  }

  // Stage 4: Reverse Blend
  // Voodoo1 convention: when cc_reverse_blend=0, factor IS inverted.
  // When cc_reverse_blend=1, factor passes through unchanged.
  // Inversion follows 86Box: factor ^= 0xFF; factor += 1
  // This maps 0 → 256, 255 → 1 (9-bit result)
  // Factor is UQ(0,8), representing 0..255/256.
  // After inversion, result needs 9 bits to hold 256 (=1.0).
  // We widen to UQ(1,8) for the inverted result.
  def invertFactor(f: AFix, reverseBlend: Bool): AFix = {
    val fRaw = f.raw.asUInt.resize(8 bits)
    val invRaw = (~fRaw).resize(9 bits) + 1  // XOR 0xFF + 1, result is 9 bits
    val passRaw = fRaw.resize(9 bits)
    val result = AFix.UQ(1 bits, 8 bits)  // wider: can hold 0..256
    when(reverseBlend) {
      result.raw := passRaw.asBits  // reverse_blend=1: pass through
    }.otherwise {
      result.raw := invRaw.asBits   // reverse_blend=0: invert
    }
    result
  }
  (n4(C_FACTOR).channels, n4(C_FACTOR_PRE).channels).zipped.foreach { (dst, src) =>
    dst := invertFactor(src, n4(CONFIG).reverseBlend)
  }
  n4(A_FACTOR) := invertFactor(n4(A_FACTOR_PRE), n4(CONFIG).alphaReverseBlend)

  // Stage 5: Multiply - AFix(SQ(10,0)) * AFix(UQ(0,8)) -> AFix(SQ(10,8))
  // AFix handles the fixed-point math automatically
  (n5(C_AFTER_MUL).channels, n5(C_AFTER_SUB).channels, n5(C_FACTOR).channels).zipped.foreach {
    (dst, sub, factor) =>
      dst := (sub * factor).fixTo(colorMulFormat)
  }
  n5(A_AFTER_MUL) := (n5(A_AFTER_SUB) * n5(A_FACTOR)).fixTo(colorMulFormat)

  // Stage 6: Add
  // Convert SInt to AFix for addition (C_LOCAL and A_LOCAL are SInt(9 bits))
  switch(n6(CONFIG).add) {
    is(AddMode.NONE) {
      n6(C_AFTER_ADD) := n6(C_AFTER_MUL)
    }
    is(AddMode.CLOCAL) {
      // Convert integer c_local to the same format as c_after_mul (SQ(10,8))
      // This means shifting the integer value left by 8 bits (multiply by 256)
      (n6(C_AFTER_ADD).channels, n6(C_AFTER_MUL).channels, n6(C_LOCAL).channels).zipped.foreach {
        (dst, mul, local) =>
          val localAFix = AFix(local).fixTo(colorMulFormat)
          dst := (mul + localAFix).fixTo(colorMulFormat)
      }
    }
    is(AddMode.ALOCAL) {
      val aLocalAFix = AFix(n6(A_LOCAL)).fixTo(colorMulFormat)
      (n6(C_AFTER_ADD).channels, n6(C_AFTER_MUL).channels).zipped.foreach { (dst, mul) =>
        dst := (mul + aLocalAFix).fixTo(colorMulFormat)
      }
    }
  }

  switch(n6(CONFIG).alphaAdd) {
    is(AddMode.NONE) {
      n6(A_AFTER_ADD) := n6(A_AFTER_MUL)
    }
    is(AddMode.CLOCAL) {
      val aLocalAFix = AFix(n6(A_LOCAL)).fixTo(colorMulFormat)
      n6(A_AFTER_ADD) := (n6(A_AFTER_MUL) + aLocalAFix).fixTo(colorMulFormat)
    }
    is(AddMode.ALOCAL) {
      val aLocalAFix = AFix(n6(A_LOCAL)).fixTo(colorMulFormat)
      n6(A_AFTER_ADD) := (n6(A_AFTER_MUL) + aLocalAFix).fixTo(colorMulFormat)
    }
  }

  // Stage 7: Clamp - AFix to unsigned 8-bit
  (n7(C_CLAMPED).channels, n7(C_AFTER_ADD).channels).zipped.foreach { (dst, src) =>
    dst := clampAFixToU8(src)
  }
  n7(A_CLAMPED) := clampAFixToU8(n7(A_AFTER_ADD))

  // Connect output from last node
  n8.driveTo(io.output) { (payload, self) =>
    payload.coords := self(COORDS)
    payload.depth := self(DEPTH)

    // Stage 8: Invert Output
    (payload.color.channels, self(C_CLAMPED).channels).zipped.foreach { (dst, src) =>
      dst := self(CONFIG).invertOutput ? ~src | src
    }
    payload.alpha := self(CONFIG).alphaInvertOutput ? ~self(A_CLAMPED) | self(A_CLAMPED)
  }

  // Build the pipeline
  Builder(links)
}
