package voodoo.framebuffer

import voodoo._
import voodoo.xls.XlsFramebufferAddress
import spinal.core._

object FramebufferAddressMath {
  private def calculate(base: UInt, x: UInt, y: UInt, fbPixelStride: UInt): UInt = {
    val xls = new XlsFramebufferAddress
    xls.io.base := base.resize(26 bits)
    xls.io.x := x.resize(16 bits)
    xls.io.y := y.resize(16 bits)
    xls.io.stride := fbPixelStride.resize(11 bits)
    xls.io.out
  }

  def pixelByteOffset(x: UInt, y: UInt, fbPixelStride: UInt, addressWidth: Int): UInt =
    calculate(U(0, 26 bits), x, y, fbPixelStride).resize(addressWidth bits)

  def planeAddress(base: UInt, x: UInt, y: UInt, fbPixelStride: UInt): UInt =
    calculate(base, x, y, fbPixelStride).resize(base.getWidth bits)
}
