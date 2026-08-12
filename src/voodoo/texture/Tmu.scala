package voodoo.texture

import voodoo._
import voodoo.xls._
import spinal.core._
import spinal.core.sim._
import spinal.lib._
import spinal.lib.bus.bmb._

/** Texture Mapping Unit (TMU)
  *
  * Performs texture sampling for a single texture unit. Two TMUs are instantiated and chained
  * sequentially (TMU0 → TMU1) to support multitexturing.
  *
  * Pipeline stages (fully pipelined using Stream fork/join):
  *   1. Perspective correction + coordinate generation (X.4 format with LOD scaling)
  *   2. Expander: point sampling emits 1 texel request, bilinear emits 4
  *   3. Fork: send address to memory, queue passthrough data
  *   4. Join: combine memory response with queued data
  *   5. Format decode: decode texel data to RGBA8888
  *   6. Collector: point passes through, bilinear accumulates 4 texels and blends
  */
case class Tmu(c: voodoo.Config) extends Component {
  val io = new Bundle {
    // Input stream (from Rasterizer for TMU0, from TMU0 for TMU1)
    val input = slave Stream (Tmu.Input(c))

    // Output stream (to TMU1 for TMU0, to ColorCombine for TMU1)
    val output = master Stream (Tmu.Output(c))

    // Texture memory read bus
    val texRead = master(Bmb(Tmu.bmbParams(c)))

    // Palette write port (from Core, driven by NCC table 0 I/Q register writes with bit 31 set)
    val paletteWrite = slave Flow (Tmu.PaletteWrite())
    val sendConfig = in Bool ()

    // Live NCC tables. Sync-backed table writes only happen after the render pipeline drains.
    val nccTables = in(Tmu.NccTables())

    // Invalidate texture-side fast caches after texture memory writes
    val invalidate = in Bool ()

    // Pipeline busy: pixels in flight inside fork-queue-join and collector
    val busy = out Bool ()
  }

  val requestIdWidth = Tmu.requestIdWidth
  val maxOutstanding = 7
  val inFlightCount = Reg(UInt(log2Up(maxOutstanding + 1) bits)) init 0

  import Tmu._

  val lookupTables = new Area {
    val recipTableValues = Array.tabulate(512) { i =>
      val clamped = if (i <= 256) i else 256
      U(scala.math.round((1 << 24).toDouble / (256.0 + clamped)).toLong, 17 bits)
    }
    val recipTable = Mem(UInt(17 bits), recipTableValues)

    val logTableValues: Array[Int] = Array(
      0x00, 0x01, 0x02, 0x04, 0x05, 0x07, 0x08, 0x09, 0x0b, 0x0c, 0x0e, 0x0f, 0x10, 0x12, 0x13,
      0x15, 0x16, 0x17, 0x19, 0x1a, 0x1b, 0x1d, 0x1e, 0x1f, 0x21, 0x22, 0x23, 0x25, 0x26, 0x27,
      0x28, 0x2a, 0x2b, 0x2c, 0x2e, 0x2f, 0x30, 0x31, 0x33, 0x34, 0x35, 0x36, 0x38, 0x39, 0x3a,
      0x3b, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x43, 0x44, 0x45, 0x46, 0x47, 0x49, 0x4a, 0x4b, 0x4c,
      0x4d, 0x4e, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d,
      0x5e, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6c, 0x6d, 0x6e,
      0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d,
      0x7e, 0x7f, 0x80, 0x81, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8c,
      0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
      0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
      0xaa, 0xab, 0xac, 0xad, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xcd, 0xce, 0xcf, 0xd0,
      0xd1, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xda, 0xdb, 0xdc,
      0xdd, 0xde, 0xde, 0xdf, 0xe0, 0xe1, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe5, 0xe6, 0xe7, 0xe8,
      0xe8, 0xe9, 0xea, 0xeb, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xef, 0xf0, 0xf1, 0xf2, 0xf2, 0xf3,
      0xf4, 0xf5, 0xf5, 0xf6, 0xf7, 0xf7, 0xf8, 0xf9, 0xfa, 0xfa, 0xfb, 0xfc, 0xfd, 0xfd, 0xfe, 0xff
    )
    val logTable = Mem(UInt(8 bits), logTableValues.map(v => U(v, 8 bits)))
  }
  val recipTable = lookupTables.recipTable
  val logTable = lookupTables.logTable

  val inputForStage = io.input

  val texMode = Tmu.TextureMode.decode(inputForStage.payload.config.textureMode)
  val oow = inputForStage.payload.w // 1/W in 2.30 format (wider internal accumulator)
  val sow = inputForStage.payload.s // S/W in 14.18 format (wider internal accumulator)
  val tow = inputForStage.payload.t // T/W in 14.18 format (wider internal accumulator)

  val frontEnd = new Area {
    val stageRecipPrep = inputForStage
      .translateWith {
        val recipMeta = new XlsTmuRecipMeta
        recipMeta.io.w := oow.raw.asSInt
        recipMeta.io.clamp_w := texMode.clampW
        val index = recipMeta.io.out(24 downto 17).asUInt
        val frac = recipMeta.io.out(16 downto 9).asUInt
        val msbPos = recipMeta.io.out(8 downto 2).asUInt
        val base = recipTable.readAsync(index.resize(9 bits))
        val next = recipTable.readAsync((index +^ U(1)).resize(9 bits))
        val out = Tmu.FrontRecipPrep(c)
        out.s := sow
        out.t := tow
        out.tLOD := inputForStage.payload.config.tLOD
        out.base := base
        out.diff := base - next
        out.frac := frac
        out.logFrac := logTable.readAsync(index)
        out.msbPos := msbPos
        out.validOow := recipMeta.io.out(1)
        out.perspectiveEnable := texMode.perspectiveEnable
        out.clampToZero := recipMeta.io.out(0)
        out.dSdX := inputForStage.payload.dSdX
        out.dTdX := inputForStage.payload.dTdX
        out.dSdY := inputForStage.payload.dSdY
        out.dTdY := inputForStage.payload.dTdY
        out.meta.config := inputForStage.payload.config
        if (c.trace.enabled) out.meta.trace := inputForStage.payload.trace
        out
      }
      .stage()

    val stageRecip = stageRecipPrep
      .translateWith {
        val recip = new XlsTmuRecip
        recip.io.base := stageRecipPrep.payload.base
        recip.io.difference := stageRecipPrep.payload.diff
        recip.io.fraction := stageRecipPrep.payload.frac
        recip.io.log_fraction := stageRecipPrep.payload.logFrac
        recip.io.msb := stageRecipPrep.payload.msbPos
        recip.io.valid := stageRecipPrep.payload.validOow
        val out = Tmu.FrontRecip(c)
        out.s := stageRecipPrep.payload.s
        out.t := stageRecipPrep.payload.t
        out.tLOD := stageRecipPrep.payload.tLOD
        out.interp := recip.io.out(32 downto 16).asUInt
        out.msbPos := stageRecipPrep.payload.msbPos
        out.perspectiveEnable := stageRecipPrep.payload.perspectiveEnable
        out.clampToZero := stageRecipPrep.payload.clampToZero
        out.perspLodAdjust := recip.io.out(15 downto 0).asSInt
        out.dSdX := stageRecipPrep.payload.dSdX
        out.dTdX := stageRecipPrep.payload.dTdX
        out.dSdY := stageRecipPrep.payload.dSdY
        out.dTdY := stageRecipPrep.payload.dTdY
        out.meta := stageRecipPrep.payload.meta
        out
      }
      .stage()

    val stageA = stageRecip
      .translateWith {
        val coords = new XlsTmuPerspectiveCoords
        coords.io.s := stageRecip.payload.s.raw.asSInt
        coords.io.t := stageRecip.payload.t.raw.asSInt
        coords.io.interpolated := stageRecip.payload.interp
        coords.io.msb := stageRecip.payload.msbPos
        coords.io.perspective := stageRecip.payload.perspectiveEnable
        val out = Tmu.FrontA(c)
        out.texS := coords.io.out(35 downto 18).asSInt
        out.texT := coords.io.out(17 downto 0).asSInt
        out.tLOD := stageRecip.payload.tLOD
        out.perspLodAdjust := stageRecip.payload.perspLodAdjust
        out.clampToZero := stageRecip.payload.clampToZero
        out.dSdX := stageRecip.payload.dSdX
        out.dTdX := stageRecip.payload.dTdX
        out.dSdY := stageRecip.payload.dSdY
        out.dTdY := stageRecip.payload.dTdY
        out.meta := stageRecip.payload.meta
        out
      }
      .m2sPipe()
  }
  val stageRecipPrep = frontEnd.stageRecipPrep
  val stageRecip = frontEnd.stageRecip
  val stageA = frontEnd.stageA

  val texS = stageA.payload.texS.setName("texS")
  val texT = stageA.payload.texT.setName("texT")
  texS.simPublic()
  texT.simPublic()

  val lodPipeline = new Area {
    val gradient = new XlsTmuGradient
    gradient.io.ds_dx := stageA.payload.dSdX.raw.asSInt
    gradient.io.dt_dx := stageA.payload.dTdX.raw.asSInt
    gradient.io.ds_dy := stageA.payload.dSdY.raw.asSInt
    gradient.io.dt_dy := stageA.payload.dTdY.raw.asSInt

    val stageGrad = stageA
      .translateWith {
        val out = Tmu.FrontGrad(c)
        out.texS := stageA.payload.texS
        out.texT := stageA.payload.texT
        out.tLOD := stageA.payload.tLOD
        out.clampToZero := stageA.payload.clampToZero
        out.tempLOD := gradient.io.out
        out.perspLodAdjust := stageA.payload.perspLodAdjust
        out.meta := stageA.payload.meta
        out
      }
      .stage()

    val lodIndex = new XlsTmuLodIndex
    lodIndex.io.temp_lod := stageGrad.payload.tempLOD
    val baseLod = new XlsTmuBaseLod
    baseLod.io.temp_lod := stageGrad.payload.tempLOD
    baseLod.io.msb := lodIndex.io.out(14 downto 8).asUInt
    baseLod.io.log_fraction := logTable.readAsync(lodIndex.io.out(7 downto 0).asUInt)

    val stageLodPrep = stageGrad
      .translateWith {
        val out = Tmu.FrontLodPrep(c)
        out.texS := stageGrad.payload.texS
        out.texT := stageGrad.payload.texT
        out.tLOD := stageGrad.payload.tLOD
        out.clampToZero := stageGrad.payload.clampToZero
        out.baseLod_8_8 := baseLod.io.out
        out.perspLodAdjust := stageGrad.payload.perspLodAdjust
        out.meta := stageGrad.payload.meta
        out
      }
      .stage()

    val tLOD = Tmu.TLOD.decode(stageLodPrep.payload.tLOD)
    val lodSelect = new XlsTmuLodSelect
    lodSelect.io.base_lod := stageLodPrep.payload.baseLod_8_8
    lodSelect.io.perspective_adjust := stageLodPrep.payload.perspLodAdjust
    lodSelect.io.t_lod := stageLodPrep.payload.tLOD
    val lodLevel = lodSelect.io.out(11 downto 8).asUInt
    val texWidthBits = lodSelect.io.out(7 downto 4).asUInt
    val texHeightBits = lodSelect.io.out(3 downto 0).asUInt

    val stageLod = stageLodPrep
      .translateWith {
        val out = Tmu.FrontLod(c)
        out.texS := stageLodPrep.payload.texS
        out.texT := stageLodPrep.payload.texT
        out.clampToZero := stageLodPrep.payload.clampToZero
        out.lodLevel := lodLevel
        out.texWidthBits := texWidthBits
        out.texHeightBits := texHeightBits
        out.meta := stageLodPrep.payload.meta
        out
      }
      .stage()
  }
  val stageGrad = lodPipeline.stageGrad
  val stageLodPrep = lodPipeline.stageLodPrep
  val tLOD = lodPipeline.tLOD
  val lodLevel = lodPipeline.lodLevel
  val texWidthBits = lodPipeline.texWidthBits
  val texHeightBits = lodPipeline.texHeightBits
  val stageLod = lodPipeline.stageLod

  val coordGen = new Area {
    val stageLodTexMode = Tmu.TextureMode.decode(stageLod.payload.meta.config.textureMode)
    val bilinearEnable = stageLodTexMode.minFilter || stageLodTexMode.magFilter

    val coordinates = new XlsTmuCoordinates
    coordinates.io.tex_s := stageLod.payload.texS
    coordinates.io.tex_t := stageLod.payload.texT
    coordinates.io.level := stageLod.payload.lodLevel
    coordinates.io.clamp_zero := stageLod.payload.clampToZero

    val stageB = stageLod
      .translateWith {
        val out = Tmu.FrontB(c)
        out.finalSPoint := coordinates.io.out(63 downto 50).asSInt
        out.finalTPoint := coordinates.io.out(49 downto 36).asSInt
        out.finalSi := coordinates.io.out(35 downto 22).asSInt
        out.finalTi := coordinates.io.out(21 downto 8).asSInt
        out.finalDs := coordinates.io.out(7 downto 4).asUInt
        out.finalDt := coordinates.io.out(3 downto 0).asUInt
        out.lodLevel := stageLod.payload.lodLevel
        out.texWidthBits := stageLod.payload.texWidthBits
        out.texHeightBits := stageLod.payload.texHeightBits
        out.meta := stageLod.payload.meta
        out
      }
      .stage()
  }
  val stageLodTexMode = coordGen.stageLodTexMode
  val bilinearEnable = coordGen.bilinearEnable
  val stageB = coordGen.stageB

  val stageBTexMode = Tmu.TextureMode.decode(stageB.payload.meta.config.textureMode)

  val is16BitFormat = stageBTexMode.format >= Tmu.TextureFormat.ARGB8332
  val addressGen = new Area {
    val packedTables = if (c.packedTexLayout) stageB.payload.meta.config.texTables else null
    val packedLodBase =
      if (c.packedTexLayout)
        packedTables.texBase(stageB.payload.lodLevel).resize(c.addressWidth.value bits)
      else U(0, c.addressWidth.value bits)
    val packedLodShift =
      if (c.packedTexLayout) packedTables.texShift(stageB.payload.lodLevel) else U(0, 4 bits)

    val addresses = new XlsTmuAddresses
    addresses.io.point_s := stageB.payload.finalSPoint
    addresses.io.point_t := stageB.payload.finalTPoint
    addresses.io.integer_s := stageB.payload.finalSi
    addresses.io.integer_t := stageB.payload.finalTi
    addresses.io.width_bits := stageB.payload.texWidthBits
    addresses.io.height_bits := stageB.payload.texHeightBits
    addresses.io.clamp_s := stageBTexMode.clampS
    addresses.io.clamp_t := stageBTexMode.clampT
    addresses.io.is_16_bit := is16BitFormat
    addresses.io.packed_layout := (if (c.packedTexLayout) True else False)
    addresses.io.lod_base := packedLodBase.resize(22 bits)
    addresses.io.lod_shift := packedLodShift
    addresses.io.base0 := stageB.payload.meta.config.texBaseAddr
    addresses.io.base1 := stageB.payload.meta.config.texBaseAddr1
    addresses.io.base2 := stageB.payload.meta.config.texBaseAddr2
    addresses.io.base38 := stageB.payload.meta.config.texBaseAddr38
    addresses.io.multibase := stageB.payload.meta.config.tLOD(24)
    addresses.io.level := stageB.payload.lodLevel

    val pointAddr = addresses.io.out(129 downto 104).asUInt.resize(c.addressWidth.value bits)
    val biAddr0 = addresses.io.out(103 downto 78).asUInt.resize(c.addressWidth.value bits)
    val biAddr1 = addresses.io.out(77 downto 52).asUInt.resize(c.addressWidth.value bits)
    val biAddr2 = addresses.io.out(51 downto 26).asUInt.resize(c.addressWidth.value bits)
    val biAddr3 = addresses.io.out(25 downto 0).asUInt.resize(c.addressWidth.value bits)
  }
  val packedLodBase = addressGen.packedLodBase
  val packedLodShift = addressGen.packedLodShift
  val pointAddr = addressGen.pointAddr
  val biAddr0 = addressGen.biAddr0
  val biAddr1 = addressGen.biAddr1
  val biAddr2 = addressGen.biAddr2
  val biAddr3 = addressGen.biAddr3

  val inputPassthrough = Tmu.TmuPassthrough(c)
  assignPassthrough(
    inputPassthrough,
    stageBTexMode.format,
    bilinearEnable,
    stageB.payload.meta.config.textureMode(5),
    stageB.payload.finalDs,
    stageB.payload.finalDt,
    if (c.trace.enabled) stageB.payload.meta.trace else null
  )

  val sampleRequestPrep = stageB
    .translateWith {
      val req = Tmu.SampleRequestPrep(c)
      val tapLineBase = Vec(UInt(22 bits), 4)
      tapLineBase(0) := bilinearEnable ? Tmu.lineBase22Of(
        biAddr0,
        log2Up(c.texFillLineWords * 4)
      ) | Tmu.lineBase22Of(pointAddr, log2Up(c.texFillLineWords * 4))
      tapLineBase(1) := Tmu.lineBase22Of(biAddr1, log2Up(c.texFillLineWords * 4))
      tapLineBase(2) := Tmu.lineBase22Of(biAddr2, log2Up(c.texFillLineWords * 4))
      tapLineBase(3) := Tmu.lineBase22Of(biAddr3, log2Up(c.texFillLineWords * 4))
      req.pointAddr := pointAddr
      req.biAddr0 := biAddr0
      req.biAddr1 := biAddr1
      req.biAddr2 := biAddr2
      req.biAddr3 := biAddr3
      req.lodBase := packedLodBase
      req.lodShift := packedLodShift
      req.is16Bit := is16BitFormat
      if (c.packedTexLayout) {
        req.texTables := stageB.payload.meta.config.texTables
      }
      for (i <- 0 until 4) {
        req.tapLineBase(i) := tapLineBase(i)
      }
      req.bilinear := bilinearEnable
      req.passthrough := inputPassthrough
      req
    }
    .stage()
  val sampleRequestBase = sampleRequestPrep
    .translateWith {
      val req = Tmu.SampleRequest(c)
      val packedDecode = new XlsTmuPackedDecode
      val tapAddr = Vec(UInt(c.addressWidth.value bits), 4)
      tapAddr(
        0
      ) := sampleRequestPrep.payload.bilinear ? sampleRequestPrep.payload.biAddr0 | sampleRequestPrep.payload.pointAddr
      tapAddr(1) := sampleRequestPrep.payload.biAddr1
      tapAddr(2) := sampleRequestPrep.payload.biAddr2
      tapAddr(3) := sampleRequestPrep.payload.biAddr3
      packedDecode.io.line0 := sampleRequestPrep.payload.tapLineBase(0)
      packedDecode.io.line1 := sampleRequestPrep.payload.tapLineBase(1)
      packedDecode.io.line2 := sampleRequestPrep.payload.tapLineBase(2)
      packedDecode.io.line3 := sampleRequestPrep.payload.tapLineBase(3)
      packedDecode.io.addr0 := tapAddr(0).resize(22 bits)
      packedDecode.io.addr1 := tapAddr(1).resize(22 bits)
      packedDecode.io.addr2 := tapAddr(2).resize(22 bits)
      packedDecode.io.addr3 := tapAddr(3).resize(22 bits)
      packedDecode.io.is_16_bit := sampleRequestPrep.payload.is16Bit
      packedDecode.io.lod_base := sampleRequestPrep.payload.lodBase.resize(22 bits)
      packedDecode.io.lod_shift := sampleRequestPrep.payload.lodShift
      packedDecode.io.bases := sampleRequestPrep.payload.texTables.texBase
        .map(_.asBits)
        .reduce(_ ## _)
      packedDecode.io.ends := sampleRequestPrep.payload.texTables.texEnd
        .map(_.asBits)
        .reduce(_ ## _)
      packedDecode.io.shifts := sampleRequestPrep.payload.texTables.texShift
        .map(_.asBits)
        .reduce(_ ## _)
      req.pointAddr := sampleRequestPrep.payload.pointAddr
      req.biAddr0 := sampleRequestPrep.payload.biAddr0
      req.biAddr1 := sampleRequestPrep.payload.biAddr1
      req.biAddr2 := sampleRequestPrep.payload.biAddr2
      req.biAddr3 := sampleRequestPrep.payload.biAddr3
      req.lodBase := sampleRequestPrep.payload.lodBase
      req.lodShift := sampleRequestPrep.payload.lodShift
      req.is16Bit := sampleRequestPrep.payload.is16Bit
      if (c.packedTexLayout) {
        req.texTables := sampleRequestPrep.payload.texTables
      }
      for (i <- 0 until 4) {
        val startup = packedDecode.io.out(79 - i * 16 downto 64 - i * 16)
        for (byte <- 0 until 4) {
          val low = (3 - byte) * 4
          req.tapStartupDecode(i).valid(byte) := startup(low + 3)
          req.tapStartupDecode(i).bank(byte) := startup(low + 2 downto low + 1).asUInt
          req.tapStartupDecode(i).pair(byte) := startup(low)
        }
        val location = packedDecode.io.out(15 - i * 4 downto 12 - i * 4)
        req.tapPackedBank(i) := location(2 downto 1).asUInt
        req.tapPackedPair(i) := location(0)
      }
      req.bilinear := sampleRequestPrep.payload.bilinear
      req.passthrough := sampleRequestPrep.payload.passthrough
      req
    }
    .m2sPipe()
    .queue(16)

  val bypassOutput = Stream(Tmu.Output(c))
  val bypassTexture = !stageB.payload.meta.config.textureEnable
  val bypassSafe = bypassTexture && (inFlightCount === 0)

  bypassOutput.valid := sampleRequestBase.valid && bypassSafe
  bypassOutput.payload.texture.r := 0
  bypassOutput.payload.texture.g := 0
  bypassOutput.payload.texture.b := 0
  bypassOutput.payload.textureAlpha := U(255, 8 bits)
  if (c.trace.enabled) {
    bypassOutput.payload.trace := sampleRequestBase.payload.passthrough.trace
  }

  val canAllocate = inFlightCount =/= maxOutstanding
  val sampleRequest = Stream(Tmu.SampleRequest(c))
  sampleRequest.valid := sampleRequestBase.valid && !bypassTexture && canAllocate
  copySampleRequest(sampleRequest.payload, sampleRequestBase.payload, c)
  sampleRequestBase.ready := Mux(
    bypassTexture,
    bypassOutput.ready && bypassSafe,
    sampleRequest.ready && canAllocate
  )

  val texturePath: TmuTexturePathBase =
    if (c.useTexFillCache) TmuTextureCache(c) else TmuNoTextureCache(c)
  sampleRequest >/-> texturePath.io.sampleRequest
  texturePath.io.invalidate := io.invalidate
  io.texRead <> texturePath.io.texRead

  val texelDecoder = TmuTexelDecoder(c)
  texturePath.io.sampleFetch >/-> texelDecoder.io.sampleFetch
  texelDecoder.io.paletteWrite <> io.paletteWrite
  texelDecoder.io.sendConfig := io.sendConfig
  texelDecoder.io.nccTables := io.nccTables

  texelDecoder.io.output.ready := io.output.ready && !bypassOutput.valid
  bypassOutput.ready := io.output.ready

  assert(!(bypassOutput.valid && texelDecoder.io.output.valid))

  io.output.valid := bypassOutput.valid || texelDecoder.io.output.valid
  io.output.payload := texelDecoder.io.output.payload
  when(bypassOutput.valid) {
    io.output.payload := bypassOutput.payload
  }

  when((sampleRequest.fire || bypassOutput.fire) && !io.output.fire) {
    inFlightCount := inFlightCount + 1
  }.elsewhen(!(sampleRequest.fire || bypassOutput.fire) && io.output.fire) {
    inFlightCount := inFlightCount - 1
  }
  io.busy := inFlightCount =/= 0 || stageRecipPrep.valid || stageRecip.valid || stageA.valid || stageGrad.valid || stageLodPrep.valid || stageLod.valid || stageB.valid || sampleRequestBase.valid

  /** BMB parameters for texture memory access */
  def bmbParams(c: voodoo.Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = false,
    alignment = BmbParameter.BurstAlignement.BYTE
  )
}

object Tmu {

  case class PackedStartupDecode() extends Bundle {
    val valid = Bits(4 bits)
    val bank = Vec(UInt(2 bits), 4)
    val pair = Bits(4 bits)
  }
  val requestIdWidth = 3

  def lineBase22Of(addr: UInt, lineByteShift: Int): UInt =
    ((addr.resize(22 bits) >> lineByteShift) << lineByteShift).resize(22 bits)

  def assignPassthrough(
      dst: TmuPassthrough,
      format: UInt,
      bilinear: Bool,
      nccTableSelect: Bool,
      ds: UInt,
      dt: UInt,
      trace: Trace.PixelKey = null
  ): Unit = {
    dst.format := format
    dst.bilinear := bilinear
    dst.nccTableSelect := nccTableSelect
    dst.ds := ds
    dst.dt := dt
    if (trace != null) dst.trace := trace
  }

  def copySampleRequest(dst: SampleRequest, src: SampleRequest, c: voodoo.Config): Unit = {
    dst.pointAddr := src.pointAddr
    dst.biAddr0 := src.biAddr0
    dst.biAddr1 := src.biAddr1
    dst.biAddr2 := src.biAddr2
    dst.biAddr3 := src.biAddr3
    dst.lodBase := src.lodBase
    dst.lodShift := src.lodShift
    dst.is16Bit := src.is16Bit
    if (c.packedTexLayout) dst.texTables := src.texTables
    for (i <- 0 until 4) {
      dst.tapStartupDecode(i).valid := src.tapStartupDecode(i).valid
      dst.tapStartupDecode(i).pair := src.tapStartupDecode(i).pair
      for (byteIdx <- 0 until 4) {
        dst.tapStartupDecode(i).bank(byteIdx) := src.tapStartupDecode(i).bank(byteIdx)
      }
      dst.tapPackedBank(i) := src.tapPackedBank(i)
      dst.tapPackedPair(i) := src.tapPackedPair(i)
    }
    dst.bilinear := src.bilinear
    dst.passthrough.format := src.passthrough.format
    dst.passthrough.bilinear := src.passthrough.bilinear
    dst.passthrough.nccTableSelect := src.passthrough.nccTableSelect
    dst.passthrough.ds := src.passthrough.ds
    dst.passthrough.dt := src.passthrough.dt
    if (c.trace.enabled) dst.passthrough.trace := src.passthrough.trace
  }

  /** Palette write command */
  case class PaletteWrite() extends Bundle {
    val address = UInt(8 bits)
    val data = Bits(24 bits)
  }

  /** Per-request metadata carried from address generation through fetch/decode */
  case class TmuPassthrough(c: voodoo.Config) extends Bundle {
    val format = UInt(4 bits)
    val bilinear = Bool()
    val nccTableSelect = Bool()
    val ds = UInt(4 bits)
    val dt = UInt(4 bits)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  case class FrontRecipPrep(c: voodoo.Config) extends Bundle {
    val s = AFix(c.texCoordsAccumFormat)
    val t = AFix(c.texCoordsAccumFormat)
    val tLOD = Bits(27 bits)
    val base = UInt(17 bits)
    val diff = UInt(17 bits)
    val frac = UInt(8 bits)
    val logFrac = UInt(8 bits)
    val msbPos = UInt(7 bits)
    val validOow = Bool()
    val perspectiveEnable = Bool()
    val clampToZero = Bool()
    val dSdX = AFix(c.texCoordsFormat)
    val dTdX = AFix(c.texCoordsFormat)
    val dSdY = AFix(c.texCoordsFormat)
    val dTdY = AFix(c.texCoordsFormat)
    val meta = FrontMeta(c)
  }

  case class FrontA(c: voodoo.Config) extends Bundle {
    val texS = SInt(18 bits)
    val texT = SInt(18 bits)
    val tLOD = Bits(27 bits)
    val perspLodAdjust = SInt(16 bits)
    val clampToZero = Bool()
    val dSdX = AFix(c.texCoordsFormat)
    val dTdX = AFix(c.texCoordsFormat)
    val dSdY = AFix(c.texCoordsFormat)
    val dTdY = AFix(c.texCoordsFormat)
    val meta = FrontMeta(c)
  }

  case class FrontRecip(c: voodoo.Config) extends Bundle {
    val s = AFix(c.texCoordsAccumFormat)
    val t = AFix(c.texCoordsAccumFormat)
    val tLOD = Bits(27 bits)
    val interp = UInt(17 bits)
    val msbPos = UInt(7 bits)
    val perspectiveEnable = Bool()
    val clampToZero = Bool()
    val perspLodAdjust = SInt(16 bits)
    val dSdX = AFix(c.texCoordsFormat)
    val dTdX = AFix(c.texCoordsFormat)
    val dSdY = AFix(c.texCoordsFormat)
    val dTdY = AFix(c.texCoordsFormat)
    val meta = FrontMeta(c)
  }

  case class FrontB(c: voodoo.Config) extends Bundle {
    val finalSPoint = SInt(14 bits)
    val finalTPoint = SInt(14 bits)
    val finalSi = SInt(14 bits)
    val finalTi = SInt(14 bits)
    val finalDs = UInt(4 bits)
    val finalDt = UInt(4 bits)
    val lodLevel = UInt(4 bits)
    val texWidthBits = UInt(4 bits)
    val texHeightBits = UInt(4 bits)
    val meta = FrontMeta(c)
  }

  case class FrontLodPrep(c: voodoo.Config) extends Bundle {
    val texS = SInt(18 bits)
    val texT = SInt(18 bits)
    val tLOD = Bits(27 bits)
    val clampToZero = Bool()
    val baseLod_8_8 = SInt(16 bits)
    val perspLodAdjust = SInt(16 bits)
    val meta = FrontMeta(c)
  }

  case class FrontLod(c: voodoo.Config) extends Bundle {
    val texS = SInt(18 bits)
    val texT = SInt(18 bits)
    val clampToZero = Bool()
    val lodLevel = UInt(4 bits)
    val texWidthBits = UInt(4 bits)
    val texHeightBits = UInt(4 bits)
    val meta = FrontMeta(c)
  }

  case class FrontGrad(c: voodoo.Config) extends Bundle {
    val texS = SInt(18 bits)
    val texT = SInt(18 bits)
    val tLOD = Bits(27 bits)
    val clampToZero = Bool()
    val tempLOD = UInt(64 bits)
    val perspLodAdjust = SInt(16 bits)
    val meta = FrontMeta(c)
  }

  case class FrontMeta(c: voodoo.Config) extends Bundle {
    val config = TmuConfig(c)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  case class SampleRequest(c: voodoo.Config) extends Bundle {
    val pointAddr = UInt(c.addressWidth.value bits)
    val biAddr0 = UInt(c.addressWidth.value bits)
    val biAddr1 = UInt(c.addressWidth.value bits)
    val biAddr2 = UInt(c.addressWidth.value bits)
    val biAddr3 = UInt(c.addressWidth.value bits)
    val lodBase = UInt(c.addressWidth.value bits)
    val lodShift = UInt(4 bits)
    val is16Bit = Bool()
    val texTables = if (c.packedTexLayout) TexLayoutTables.Tables() else null
    val tapStartupDecode = Vec(PackedStartupDecode(), 4)
    val tapPackedBank = Vec(UInt(2 bits), 4)
    val tapPackedPair = Vec(Bool(), 4)
    val bilinear = Bool()
    val passthrough = TmuPassthrough(c)
  }

  case class SampleRequestPrep(c: voodoo.Config) extends Bundle {
    val pointAddr = UInt(c.addressWidth.value bits)
    val biAddr0 = UInt(c.addressWidth.value bits)
    val biAddr1 = UInt(c.addressWidth.value bits)
    val biAddr2 = UInt(c.addressWidth.value bits)
    val biAddr3 = UInt(c.addressWidth.value bits)
    val lodBase = UInt(c.addressWidth.value bits)
    val lodShift = UInt(4 bits)
    val is16Bit = Bool()
    val texTables = if (c.packedTexLayout) TexLayoutTables.Tables() else null
    val tapLineBase = Vec(UInt(22 bits), 4)
    val bilinear = Bool()
    val passthrough = TmuPassthrough(c)
  }

  case class CachedReq(c: voodoo.Config, bankEntryWidth: Int) extends Bundle {
    val lineBase = UInt(c.addressWidth.value bits)
    val bankSel = UInt(2 bits)
    val bankEntry = UInt(bankEntryWidth bits)
    val lodBase = UInt(c.addressWidth.value bits)
    val lodShift = UInt(4 bits)
    val is16Bit = Bool()
    val texTables = if (c.packedTexLayout) TexLayoutTables.Tables() else null
    val startupDecode = PackedStartupDecode()
  }

  case class SampleFetch(c: voodoo.Config) extends Bundle {
    val bilinear = Bool()
    val texels = Vec.fill(4)(Bits(16 bits))
    val passthrough = TmuPassthrough(c)
  }

  /** Decoded textureMode register fields */
  case class TextureMode() extends Bundle {
    val perspectiveEnable = Bool() // Bit 0: tpersp_st - perspective correction
    val minFilter = Bool() // Bit 1: tminfilter - 0=point, 1=bilinear
    val magFilter = Bool() // Bit 2: tmagfilter - 0=point, 1=bilinear
    val clampW = Bool() // Bit 3: tclampw - clamp when W negative
    val lodDither = Bool() // Bit 4: tloddither (TODO)
    val nccSelect = Bool() // Bit 5: tnccselect - NCC table 0 or 1 (TODO)
    val clampS = Bool() // Bit 6: tclamps - 0=wrap, 1=clamp
    val clampT = Bool() // Bit 7: tclampt - 0=wrap, 1=clamp
    val format = UInt(4 bits) // Bits 11:8: tformat - texture format
  }

  object TextureMode {
    def decode(bits: Bits): TextureMode = {
      val mode = TextureMode()
      mode.perspectiveEnable := bits(0)
      mode.minFilter := bits(1)
      mode.magFilter := bits(2)
      mode.clampW := bits(3)
      mode.lodDither := bits(4)
      mode.nccSelect := bits(5)
      mode.clampS := bits(6)
      mode.clampT := bits(7)
      mode.format := bits(11 downto 8).asUInt
      mode
    }
  }

  /** Texture format enumeration (tformat field, bits 11:8) */
  object TextureFormat {
    val RGB332 = 0
    val YIQ422 = 1
    val A8 = 2
    val I8 = 3
    val AI44 = 4
    val P8 = 5
    val ARGB8332 = 8
    val AYIQ8422 = 9
    val RGB565 = 10
    val ARGB1555 = 11
    val ARGB4444 = 12
    val AI88 = 13
    val AP88 = 14
  }

  /** Decoded tLOD register fields */
  case class TLOD() extends Bundle {
    val lodmin = UInt(6 bits)
    val lodmax = UInt(6 bits)
    val lodbias = Bits(6 bits)
    val lodOdd = Bool()
    val lodSplit = Bool()
    val lodSIsWider = Bool()
    val lodAspect = UInt(2 bits)
    val lodZeroFrac = Bool()
    val tmultibaseaddr = Bool()
    val tdataSwizzle = Bool()
    val tdataSwap = Bool()
  }

  object TLOD {
    def decode(bits: Bits): TLOD = {
      val lod = TLOD()
      lod.lodmin := bits(5 downto 0).asUInt
      lod.lodmax := bits(11 downto 6).asUInt
      lod.lodbias := bits(17 downto 12)
      lod.lodOdd := bits(18)
      lod.lodSplit := bits(19)
      lod.lodSIsWider := bits(20)
      lod.lodAspect := bits(22 downto 21).asUInt
      lod.lodZeroFrac := bits(23)
      lod.tmultibaseaddr := bits(24)
      lod.tdataSwizzle := bits(25)
      lod.tdataSwap := bits(26)
      lod
    }
  }

  /** NCC table data, pre-extracted at triangle capture time. Y: 16 luminance values (8-bit
    * unsigned), pre-extracted from 4 packed 32-bit registers. I/Q: 4 chrominance entries each,
    * packed as [26:18]=R, [17:9]=G, [8:0]=B (9-bit signed).
    */
  case class NccTableData() extends Bundle {
    val y = Vec(UInt(8 bits), 16)
    val i = Vec(Bits(27 bits), 4)
    val q = Vec(Bits(27 bits), 4)
  }

  case class NccTables() extends Bundle {
    val table0 = NccTableData()
    val table1 = NccTableData()
  }

  /** Per-TMU configuration (captured per-triangle) */
  case class TmuConfig(c: voodoo.Config = null) extends Bundle {
    val textureMode = Bits(12 bits)
    val texBaseAddr = UInt(24 bits)
    val texBaseAddr1 = UInt(24 bits)
    val texBaseAddr2 = UInt(24 bits)
    val texBaseAddr38 = UInt(24 bits)
    val tLOD = Bits(27 bits)
    val textureEnable = Bool()
    val texTables = if (c != null && c.packedTexLayout) TexLayoutTables.Tables() else null
  }

  /** TMU input bundle */
  case class Input(c: voodoo.Config) extends Bundle {
    val s = AFix(c.texCoordsAccumFormat)
    val t = AFix(c.texCoordsAccumFormat)
    val w = AFix(c.wAccumFormat)
    val cOther = Color.u8()
    val aOther = UInt(8 bits)
    val config = TmuConfig(c)
    val dSdX = AFix(c.texCoordsFormat)
    val dTdX = AFix(c.texCoordsFormat)
    val dSdY = AFix(c.texCoordsFormat)
    val dTdY = AFix(c.texCoordsFormat)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  object Input {
    def fromRasterizer(c: voodoo.Config, in: Rasterizer.Output): Input = {
      val out = Input(c)
      out.s := in.grads.sGrad
      out.t := in.grads.tGrad
      out.w := in.grads.wGrad
      out.cOther.r := 0
      out.cOther.g := 0
      out.cOther.b := 0
      out.aOther := 0
      out.config.textureMode := in.tmuConfig.textureMode
      out.config.texBaseAddr := in.tmuConfig.texBaseAddr
      out.config.texBaseAddr1 := in.tmuConfig.texBaseAddr1
      out.config.texBaseAddr2 := in.tmuConfig.texBaseAddr2
      out.config.texBaseAddr38 := in.tmuConfig.texBaseAddr38
      out.config.tLOD := in.tmuConfig.tLOD
      out.config.textureEnable := in.tmuConfig.textureEnable
      if (c.packedTexLayout) out.config.texTables := in.tmuConfig.texTables
      out.dSdX := in.tmuConfig.dSdX
      out.dTdX := in.tmuConfig.dTdX
      out.dSdY := in.tmuConfig.dSdY
      out.dTdY := in.tmuConfig.dTdY
      if (c.trace.enabled) out.trace := in.trace
      out
    }
  }

  /** TMU output bundle */
  case class Output(c: voodoo.Config) extends Bundle {
    val texture = Color.u8()
    val textureAlpha = UInt(8 bits)
    val trace = if (c.trace.enabled) Trace.PixelKey() else null
  }

  def bmbParams(c: voodoo.Config) = BmbParameter(
    addressWidth = c.addressWidth.value,
    dataWidth = 32,
    sourceWidth = 1,
    contextWidth = 0,
    lengthWidth = c.memBurstLengthWidth,
    canRead = true,
    canWrite = false,
    alignment = BmbParameter.BurstAlignement.BYTE
  )

}
