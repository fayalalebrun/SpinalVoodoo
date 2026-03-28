package voodoo.texture

import spinal.core._
import spinal.lib._
import spinal.lib.bus.bmb._
import voodoo._
import voodoo.bus.TexWritePayload
import voodoo.frontend.RegisterBank

object TextureMem {
  case class DownloadConfig(c: Config) extends Bundle {
    val textureMode = Bits(32 bits)
    val texBaseAddr = UInt(24 bits)
    val tLOD = Bits(32 bits)
  }

  object DownloadConfig {
    def fromRegisterBank(c: Config, regBank: RegisterBank): DownloadConfig = {
      val cfg = DownloadConfig(c)
      cfg.textureMode := regBank.tmuConfig.textureMode
      cfg.texBaseAddr := regBank.tmuConfig.texBaseAddr
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
    private case class PackedCpuWritePrep(c: Config) extends Bundle {
      val validWrite = Bool()
      val lodBase = UInt(22 bits)
      val lodShift = UInt(4 bits)
      val s = UInt(8 bits)
      val t = UInt(8 bits)
      val is16bit = Bool()
      val data = Bits(32 bits)
      val mask = Bits(4 bits)
    }

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
      val tmuValid = pciAddr(22 downto 21) === 0

      if (c.packedTexLayout) {
        val texCfg = TexLayoutTables.TexConfig()
        texCfg.texBaseAddr := cfg.texBaseAddr(18 downto 0)
        texCfg.tformat := cfg.textureMode(11 downto 8).asUInt
        texCfg.tLOD_aspect := cfg.tLOD(22 downto 21).asUInt
        texCfg.tLOD_sIsWider := cfg.tLOD(20)
        val writeTables = TexLayoutTables.compute(texCfg)

        val is16bit = texCfg.tformat >= Tmu.TextureFormat.ARGB8332
        val seq8 = cfg.textureMode(31)
        val lod = pciAddr(20 downto 17)
        val t = pciAddr(16 downto 9)
        val s = UInt(8 bits)

        when(is16bit) {
          s := ((pciAddr >> 1) & 0xfe).resize(8 bits)
        }.elsewhen(seq8) {
          s := (pciAddr & 0xfc).resize(8 bits)
        }.otherwise {
          s := ((pciAddr >> 1) & 0xfc).resize(8 bits)
        }

        val drainValid = lod <= 8 && tmuValid

        val prepared = drain
          .translateWith {
            val prep = PackedCpuWritePrep(c)
            prep.validWrite := drainValid
            prep.lodBase := writeTables.texBase(lod.resize(4 bits))
            prep.lodShift := writeTables.texShift(lod.resize(4 bits))
            prep.s := s
            prep.t := t
            prep.is16bit := is16bit
            prep.data := drain.data
            prep.mask := drain.mask
            prep
          }
          .m2sPipe()

        prepared
          .throwWhen(!prepared.payload.validWrite)
          .translateWith {
            val prep = prepared.payload
            val sramAddr = UInt(c.addressWidth.value bits)
            when(prep.is16bit) {
              sramAddr := (prep.lodBase + (prep.s << 1).resize(22 bits) +
                (prep.t.resize(22 bits) << (prep.lodShift +^ U(1)).resize(5 bits)).resize(22 bits))
                .resize(c.addressWidth.value bits)
            }.otherwise {
              sramAddr :=
                (prep.lodBase + prep.s.resize(22 bits) + (prep.t.resize(22 bits) << prep.lodShift)
                  .resize(22 bits)).resize(c.addressWidth.value bits)
            }

            val cmd = WriteCmd(c)
            cmd.address := sramAddr
            cmd.data := prep.data
            cmd.mask := prep.mask
            cmd
          }
          .m2sPipe()
      } else {
        val flatSramAddr =
          ((cfg.texBaseAddr(18 downto 0) << 3) +^ pciAddr).resize(c.addressWidth.value bits)

        val prepared = drain
          .translateWith {
            val prep = LinearCpuWritePrep(c)
            prep.validWrite := tmuValid
            prep.address := flatSramAddr
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
