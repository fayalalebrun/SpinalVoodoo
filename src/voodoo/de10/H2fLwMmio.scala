package voodoo.de10

import spinal.core._
import spinal.lib._

case class H2fLwMmio(addressWidth: Int) extends Bundle with IMasterSlave {
  val address = UInt(addressWidth bits)
  val read = Bool()
  val write = Bool()
  val byteenable = Bits(4 bits)
  val writedata = Bits(32 bits)
  val waitrequest = Bool()
  val readdata = Bits(32 bits)
  val readdatavalid = Bool()

  override def asMaster(): Unit = {
    out(address, read, write, byteenable, writedata)
    in(waitrequest, readdata, readdatavalid)
  }
}
