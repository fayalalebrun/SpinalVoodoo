package voodoo

import spinal.core._

object FramebufferAddressMath {
  private def mulBySmallUnsigned(value: UInt, coeff: UInt): UInt = {
    val width = value.getWidth + coeff.getWidth
    val zero = U(0, width bits)
    val terms = (0 until coeff.getWidth).map { bit =>
      val shifted = (value.resize(width bits) |<< bit).resize(width bits)
      Mux(coeff(bit), shifted, zero)
    }
    terms.reduce(_ +^ _).resize(width bits)
  }

  def pixelByteOffset(x: UInt, y: UInt, fbPixelStride: UInt, addressWidth: Int): UInt = {
    val tilesX = (fbPixelStride >> 6).resize(4 bits)
    val rowTiles = mulBySmallUnsigned(y, tilesX)
    val rowBytes = (rowTiles.resize(addressWidth bits) |<< 7).resize(addressWidth bits)
    val xBytes = (x.resize(addressWidth bits) |<< 1).resize(addressWidth bits)
    (rowBytes +^ xBytes).resize(addressWidth bits)
  }

  def planeAddress(base: UInt, x: UInt, y: UInt, fbPixelStride: UInt): UInt = {
    val offset = pixelByteOffset(x, y, fbPixelStride, base.getWidth)
    (base +^ offset).resize(base.getWidth bits)
  }
}
