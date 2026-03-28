package voodoo

import spinal.core._

case class TraceConfig(enabled: Boolean = false)

object Trace {
  object Origin {
    val triangle = 0
    val fastfill = 1
    val lfb = 2
  }

  case class PrimitiveKey() extends Bundle {
    val valid = Bool()
    val origin = UInt(2 bits)
    val drawId = UInt(32 bits)
    val primitiveId = UInt(32 bits)
  }

  case class PixelKey() extends Bundle {
    val primitive = PrimitiveKey()
    val pixelSeq = UInt(24 bits)
  }

  def zeroPrimitiveKey(): PrimitiveKey = {
    val key = PrimitiveKey()
    key.valid := False
    key.origin := 0
    key.drawId := 0
    key.primitiveId := 0
    key
  }

  def zeroPixelKey(): PixelKey = {
    val key = PixelKey()
    key.primitive.valid := False
    key.primitive.origin := 0
    key.primitive.drawId := 0
    key.primitive.primitiveId := 0
    key.pixelSeq := 0
    key
  }

  def originPixelKey(origin: Int): PixelKey = {
    val key = PixelKey()
    key.primitive.valid := True
    key.primitive.origin := origin
    key.primitive.drawId := 0
    key.primitive.primitiveId := 0
    key.pixelSeq := 0
    key
  }
}
