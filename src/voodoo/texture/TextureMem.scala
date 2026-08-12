package voodoo.texture

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo._
import voodoo.bus.TexWritePayload
import voodoo.frontend.RegisterBank
import voodoo.xls.XlsTextureWriteAddress

object TextureMem {
  case class DownloadConfig(c: Config) extends Bundle {
    val textureMode = Bits(32 bits)
    val texBaseAddr = UInt(24 bits)
    val texBaseAddr1 = UInt(24 bits)
    val texBaseAddr2 = UInt(24 bits)
    val texBaseAddr38 = UInt(24 bits)
    val tLOD = Bits(32 bits)
  }

  object DownloadConfig {
    def fromRegisterBank(c: Config, regBank: RegisterBank): DownloadConfig = {
      val cfg = DownloadConfig(c)
      cfg.textureMode := regBank.tmuConfig.textureMode
      cfg.texBaseAddr := regBank.tmuConfig.texBaseAddr
      cfg.texBaseAddr1 := regBank.tmuConfig.texBaseAddr1
      cfg.texBaseAddr2 := regBank.tmuConfig.texBaseAddr2
      cfg.texBaseAddr38 := regBank.tmuConfig.texBaseAddr38
      cfg.tLOD := regBank.tmuConfig.tLOD
      cfg
    }
  }

  case class WriteCmd(c: Config) extends Bundle {
    val address = UInt(c.addressWidth.value bits)
    val data = Bits(32 bits)
    val mask = Bits(4 bits)
  }

  object WriteCmd {

    private case class LinearCpuWritePrep(c: Config) extends Bundle {
      val validWrite = Bool()
      val address = UInt(c.addressWidth.value bits)
      val data = Bits(32 bits)
      val mask = Bits(4 bits)
    }

    def toBmb(cmdIn: Stream[WriteCmd], bmbParams: BmbParameter, source: Int): Bmb = new Composite(
      cmdIn
    ) {
      val bmb = Bmb(bmbParams)
      bmb.cmd.valid := cmdIn.valid
      bmb.cmd.address := cmdIn.address.resize(bmb.p.access.addressWidth bits)
      bmb.cmd.opcode := Bmb.Cmd.Opcode.WRITE
      bmb.cmd.length := 3
      bmb.cmd.source := U(source, bmb.p.access.sourceWidth bits)
      bmb.cmd.data := cmdIn.data
      bmb.cmd.mask := cmdIn.mask
      bmb.cmd.last := True
      cmdIn.ready := bmb.cmd.ready
      bmb.rsp.ready := True
    }.bmb

    def fromPciDrain(
        c: Config,
        drain: Stream[TexWritePayload],
        cfg: DownloadConfig
    ): Stream[WriteCmd] = {
      val pciAddr = drain.pciAddr

      if (c.packedTexLayout) {
        val texCfg = TexLayoutTables.TexConfig()
        texCfg.texBaseAddr := cfg.texBaseAddr(18 downto 0)
        texCfg.texBaseAddr1 := cfg.texBaseAddr1(18 downto 0)
        texCfg.texBaseAddr2 := cfg.texBaseAddr2(18 downto 0)
        texCfg.texBaseAddr38 := cfg.texBaseAddr38(18 downto 0)
        texCfg.tformat := cfg.textureMode(11 downto 8).asUInt
        texCfg.tLOD_aspect := cfg.tLOD(22 downto 21).asUInt
        texCfg.tLOD_sIsWider := cfg.tLOD(20)
        texCfg.tLOD_multibase := cfg.tLOD(24)
        val writeTables = TexLayoutTables.compute(texCfg)

        val lod = pciAddr(20 downto 17)
        val address = new XlsTextureWriteAddress
        address.io.pci_address := pciAddr
        address.io.is_16_bit := texCfg.tformat >= Tmu.TextureFormat.ARGB8332
        address.io.sequential_8_bit := cfg.textureMode(31)
        address.io.packed_layout := True
        address.io.lod_base := writeTables.texBase(lod)
        address.io.lod_shift := writeTables.texShift(lod)
        address.io.base0 := cfg.texBaseAddr
        address.io.base1 := cfg.texBaseAddr1
        address.io.base2 := cfg.texBaseAddr2
        address.io.base38 := cfg.texBaseAddr38
        address.io.multibase := cfg.tLOD(24)

        val prepared = drain
          .translateWith {
            val prep = LinearCpuWritePrep(c)
            prep.validWrite := address.io.out(26)
            prep.address := address.io.out(25 downto 0).asUInt
            prep.data := drain.data
            prep.mask := drain.mask
            prep
          }
          .m2sPipe()

        prepared
          .throwWhen(!prepared.payload.validWrite)
          .translateWith {
            val prep = prepared.payload
            val cmd = WriteCmd(c)
            cmd.address := prep.address
            cmd.data := prep.data
            cmd.mask := prep.mask
            cmd
          }
          .m2sPipe()
      } else {
        val address = new XlsTextureWriteAddress
        address.io.pci_address := pciAddr
        address.io.is_16_bit := cfg.textureMode(11 downto 8).asUInt >= Tmu.TextureFormat.ARGB8332
        address.io.sequential_8_bit := cfg.textureMode(31)
        address.io.packed_layout := False
        address.io.lod_base := 0
        address.io.lod_shift := 0
        address.io.base0 := cfg.texBaseAddr
        address.io.base1 := cfg.texBaseAddr1
        address.io.base2 := cfg.texBaseAddr2
        address.io.base38 := cfg.texBaseAddr38
        address.io.multibase := cfg.tLOD(24)

        val prepared = drain
          .translateWith {
            val prep = LinearCpuWritePrep(c)
            prep.validWrite := address.io.out(26)
            prep.address := address.io.out(25 downto 0).asUInt
            prep.data := drain.data
            prep.mask := drain.mask
            prep
          }
          .m2sPipe()

        prepared
          .throwWhen(!prepared.payload.validWrite)
          .translateWith {
            val prep = prepared.payload
            val cmd = WriteCmd(c)
            cmd.address := prep.address
            cmd.data := prep.data
            cmd.mask := prep.mask
            cmd
          }
          .m2sPipe()
      }
    }
  }
}
