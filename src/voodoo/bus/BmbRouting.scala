package voodoo.bus

import voodoo._
import spinal.core._
import spinal.lib.bus.bmb._

object BmbRouting {
  private def driveCmdFromInternal(
      dst: Bmb,
      src: Bmb,
      valid: Bool,
      address: UInt,
      source: UInt,
      length: UInt
  ): Unit = {
    dst.cmd.valid := valid
    dst.cmd.opcode := src.cmd.opcode
    dst.cmd.address := address.resize(dst.p.access.addressWidth bits)
    dst.cmd.data := src.cmd.data
    dst.cmd.mask := src.cmd.mask
    dst.cmd.length := length.resize(dst.p.access.lengthWidth bits)
    dst.cmd.last := src.cmd.last
    dst.cmd.source := source.resize(dst.p.access.sourceWidth bits)
    if (dst.p.access.contextWidth > 0) {
      dst.cmd.context := src.cmd.context.resize(dst.p.access.contextWidth bits)
    }
  }

  def driveCmdFrom(
      dst: Bmb,
      src: Bmb,
      valid: Bool,
      address: UInt
  ): Unit = {
    driveCmdFromInternal(dst, src, valid, address, src.cmd.source, src.cmd.length)
  }

  def driveCmdFrom(
      dst: Bmb,
      src: Bmb,
      valid: Bool,
      address: UInt,
      source: UInt,
      length: UInt
  ): Unit = {
    driveCmdFromInternal(dst, src, valid, address, source, length)
  }

  def routeRspTo(selected: Bool, src: Bmb, dst: Bmb): Unit = {
    src.rsp.ready := selected && dst.rsp.ready
    when(selected) {
      dst.rsp.valid := src.rsp.valid
      dst.rsp.data := src.rsp.data
      dst.rsp.opcode := src.rsp.opcode
      dst.rsp.last := src.rsp.last
      dst.rsp.source := src.rsp.source.resize(dst.p.access.sourceWidth bits)
      if (dst.p.access.contextWidth > 0) {
        if (src.p.access.contextWidth > 0)
          dst.rsp.context := src.rsp.context.resize(dst.p.access.contextWidth bits)
        else dst.rsp.context := 0
      }
    }
  }
}
