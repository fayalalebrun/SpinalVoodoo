import spinal.core._

package voodoo {
  case class PixelCoords(config: Config) extends Bundle {
    val x = SInt(config.vertexFormat.nonFraction bits)
    val y = SInt(config.vertexFormat.nonFraction bits)

    def apply(idx: Int): SInt = idx match {
      case 0 => x
      case 1 => y
    }

    def assignFromVec(src: Vec[SInt]): Unit = {
      x := src(0)
      y := src(1)
    }
  }

  case class FbRouting(config: Config) extends Bundle {
    val colorBaseAddr = UInt(config.addressWidth.value bits)
    val auxBaseAddr = UInt(config.addressWidth.value bits)
    val pixelStride = UInt(11 bits)
  }

  case class Coefficients(config: Config) extends Bundle {
    // Edge coefficients need much wider range than vertices
    // Width is calculated from vertex format to handle cross products
    private val fmt = config.coefficientFormat
    val a = AFix(fmt)
    val b = AFix(fmt)
    val c = AFix(fmt)
  }

  case class Color[T <: Data](rt: HardType[T], gt: HardType[T], bt: HardType[T]) extends Bundle {
    // Fields are packed in declaration order with later fields at higher bits
    // So declare in reverse order: b, g, r to get standard RGB565 bit layout [R:5][G:6][B:5]
    val b = bt()
    val g = gt()
    val r = rt()

    /** Access channels as a sequence for iteration (r, g, b order) */
    def channels: Seq[T] = Seq(r, g, b)

    /** Apply a function to each channel, returning results as a sequence */
    def map[U](f: T => U): Seq[U] = channels.map(f)

    /** Apply a function to each channel for side effects */
    def foreach(f: T => Unit): Unit = channels.foreach(f)

    /** Zip with another Color and apply a function to corresponding channels */
    def zipWith[U <: Data, V](other: Color[U])(f: (T, U) => V): Seq[V] =
      (channels, other.channels).zipped.map(f)

    /** Assign from a sequence of values (must have exactly 3 elements in r, g, b order) */
    def assignFromSeq(values: Seq[T]): Unit = {
      r := values(0)
      g := values(1)
      b := values(2)
    }
  }

  object Color {

    /** 8-bit unsigned RGB (0-255 per channel) */
    def u8() = Color(UInt(8 bits), UInt(8 bits), UInt(8 bits))

    /** 9-bit signed RGB for intermediate math (-256 to 255) */
    def s9() = Color(SInt(9 bits), SInt(9 bits), SInt(9 bits))

    /** 10-bit signed RGB after subtraction */
    def s10() = Color(SInt(10 bits), SInt(10 bits), SInt(10 bits))

    /** 18-bit signed RGB after multiplication */
    def s18() = Color(SInt(18 bits), SInt(18 bits), SInt(18 bits))

    /** Unsigned 0.8 fixed-point RGB for blend factors (0.0 to ~1.0) */
    def ufactor() = Color(AFix.UQ(0 bits, 8 bits), AFix.UQ(0 bits, 8 bits), AFix.UQ(0 bits, 8 bits))
  }
}

package object voodoo {
  import spinal.core._

  def vertex2d(fmt: QFormat) = Vec.fill(2)(AFix(fmt))
  def triangle(fmt: QFormat) = Vec.fill(3)(vertex2d(fmt))

  case class TriangleWithSign(fmt: QFormat) extends Bundle {
    val tri = triangle(fmt)
    val signBit = Bool() // Bit 31 from triangleCMD: 0=CCW (positive area), 1=CW (negative area)
  }

  def rgb565() = Color(UInt(5 bits), UInt(6 bits), UInt(5 bits))

  /** Expand N-bit unsigned value to 8 bits by replicating MSBs into padding. Special case: width=1
    * returns 0x00 or 0xFF.
    */
  def expandTo8(v: UInt, srcWidth: Int): UInt = {
    require(srcWidth >= 1 && srcWidth <= 8)
    if (srcWidth >= 8) v.resize(8 bits)
    else if (srcWidth == 1) Mux(v(0), U(255, 8 bits), U(0, 8 bits))
    else {
      val reps = (8 + srcWidth - 1) / srcWidth
      val wide = Seq.fill(reps)(v).reduce(_ @@ _)
      wide(wide.getWidth - 1 downto wide.getWidth - 8)
    }
  }

  /** Expand RGB565 packed value to 8-bit Color */
  def expandRgb565(v: UInt): Color[UInt] = {
    val c = Color.u8()
    c.r := expandTo8(v(15 downto 11), 5)
    c.g := expandTo8(v(10 downto 5), 6)
    c.b := expandTo8(v(4 downto 0), 5)
    c
  }

  def assignRgb888(dst: Color[UInt], bits: Bits): Unit = {
    dst.r := bits(23 downto 16).asUInt
    dst.g := bits(15 downto 8).asUInt
    dst.b := bits(7 downto 0).asUInt
  }

  def rgb888Color(bits: Bits): Color[UInt] = {
    val c = Color.u8()
    assignRgb888(c, bits)
    c
  }

  def alignedWordAddress(address: UInt): UInt =
    (address(address.getWidth - 1 downto 2) ## U"2'b00").asUInt

  type AddressRemapper = voodoo.frontend.AddressRemapper
  val AddressRemapper = voodoo.frontend.AddressRemapper
  type RegisterBank = voodoo.frontend.RegisterBank
  val RegisterBank = voodoo.frontend.RegisterBank
  type SwapBuffer = voodoo.frontend.SwapBuffer
  val SwapBuffer = voodoo.frontend.SwapBuffer
  type Lfb = voodoo.frontend.Lfb
  val Lfb = voodoo.frontend.Lfb

  type TriangleSetup = voodoo.raster.TriangleSetup
  val TriangleSetup = voodoo.raster.TriangleSetup
  type Rasterizer = voodoo.raster.Rasterizer
  val Rasterizer = voodoo.raster.Rasterizer
  type Fastfill = voodoo.raster.Fastfill
  val Fastfill = voodoo.raster.Fastfill
  type FastfillWrite = voodoo.raster.FastfillWrite
  val FastfillWrite = voodoo.raster.FastfillWrite

  type Tmu = voodoo.texture.Tmu
  val Tmu = voodoo.texture.Tmu
  type TmuTexelDecoder = voodoo.texture.TmuTexelDecoder
  val TmuTexelDecoder = voodoo.texture.TmuTexelDecoder
  type TmuTextureCache = voodoo.texture.TmuTextureCache
  val TmuTextureCache = voodoo.texture.TmuTextureCache
  val TexLayoutTables = voodoo.texture.TexLayoutTables

  type ColorCombine = voodoo.pixel.ColorCombine
  val ColorCombine = voodoo.pixel.ColorCombine
  type Fog = voodoo.pixel.Fog
  val Fog = voodoo.pixel.Fog
  type FramebufferAccess = voodoo.pixel.FramebufferAccess
  val FramebufferAccess = voodoo.pixel.FramebufferAccess
  type Dither = voodoo.pixel.Dither
  val Dither = voodoo.pixel.Dither
  val DitherTables = voodoo.pixel.DitherTables
  type Write = voodoo.pixel.Write
  val Write = voodoo.pixel.Write

  type FramebufferAddressMath = voodoo.framebuffer.FramebufferAddressMath.type
  val FramebufferAddressMath = voodoo.framebuffer.FramebufferAddressMath
  type FramebufferPlaneBuffer = voodoo.framebuffer.FramebufferPlaneBuffer
  val FramebufferPlaneBuffer = voodoo.framebuffer.FramebufferPlaneBuffer
  type FramebufferPlaneReader = voodoo.framebuffer.FramebufferPlaneReader
  val FramebufferPlaneReader = voodoo.framebuffer.FramebufferPlaneReader

  type BmbRouting = voodoo.bus.BmbRouting.type
  val BmbRouting = voodoo.bus.BmbRouting
  type BmbBusInterface = voodoo.bus.BmbBusInterface
  val BmbBusInterface = voodoo.bus.BmbBusInterface
  type PciFifo = voodoo.bus.PciFifo
  val PciFifo = voodoo.bus.PciFifo
  type RegisterCategory = voodoo.bus.RegisterCategory.type
  val RegisterCategory = voodoo.bus.RegisterCategory
  val StreamWhile = voodoo.bus.StreamWhile

  type Fpxx = voodoo.math.Fpxx
  val Fpxx = voodoo.math.Fpxx
  type Fpxx2AFix = voodoo.math.Fpxx2AFix
  val Fpxx2AFix = voodoo.math.Fpxx2AFix
  type FpxxConfig = voodoo.math.FpxxConfig
  val FpxxConfig = voodoo.math.FpxxConfig
}
