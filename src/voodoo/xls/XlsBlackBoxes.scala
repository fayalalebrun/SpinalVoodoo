package voodoo.xls

import spinal.core._

private object XlsRtl {
  def path(module: String): String = s"xls/rtl/$module.v"
}

class XlsFramebufferAddress extends BlackBox {
  val io = new Bundle {
    val base = in UInt (26 bits)
    val x = in UInt (16 bits)
    val y = in UInt (16 bits)
    val stride = in UInt (11 bits)
    val out = spinal.core.out(UInt(26 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsFramebufferAddress")
  addRTLPath(XlsRtl.path("VoodooXlsFramebufferAddress"))
}

class XlsWritePack extends BlackBox {
  val io = new Bundle {
    val base = in UInt (26 bits)
    val x = in UInt (16 bits)
    val y = in UInt (16 bits)
    val stride = in UInt (11 bits)
    val data = in Bits (16 bits)
    val out = spinal.core.out(Bits(62 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsWritePack")
  addRTLPath(XlsRtl.path("VoodooXlsWritePack"))
}

class XlsDepthCompare extends BlackBox {
  val io = new Bundle {
    val comparison = in UInt (3 bits)
    val new_depth = in UInt (16 bits)
    val old_depth = in UInt (16 bits)
    val out = spinal.core.out(Bool())
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsDepthCompare")
  addRTLPath(XlsRtl.path("VoodooXlsDepthCompare"))
}

class XlsAlphaBlend extends BlackBox {
  val io = new Bundle {
    val src_rgb = in Bits (24 bits)
    val dest_rgb = in Bits (24 bits)
    val before_fog_rgb = in Bits (24 bits)
    val src_alpha = in UInt (8 bits)
    val src_selector = in UInt (4 bits)
    val dest_selector = in UInt (4 bits)
    val enable = in Bool ()
    val out = spinal.core.out(Bits(24 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsAlphaBlend")
  addRTLPath(XlsRtl.path("VoodooXlsAlphaBlend"))
}

class XlsFogWDepth extends BlackBox {
  val io = new Bundle {
    val raw_w = in SInt (32 bits)
    val out = spinal.core.out(UInt(16 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsFogWDepth")
  addRTLPath(XlsRtl.path("VoodooXlsFogWDepth"))
}

class XlsFogTableFactor extends BlackBox {
  val io = new Bundle {
    val w_depth = in UInt (16 bits)
    val table_entry = in Bits (16 bits)
    val out = spinal.core.out(UInt(8 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsFogTableFactor")
  addRTLPath(XlsRtl.path("VoodooXlsFogTableFactor"))
}

class XlsFogBlend extends BlackBox {
  val io = new Bundle {
    val src_rgb = in Bits (24 bits)
    val fog_rgb = in Bits (24 bits)
    val factor = in UInt (8 bits)
    val enable = in Bool ()
    val add = in Bool ()
    val mult = in Bool ()
    val constant = in Bool ()
    val out = spinal.core.out(Bits(24 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsFogBlend")
  addRTLPath(XlsRtl.path("VoodooXlsFogBlend"))
}

class XlsColorCombine extends BlackBox {
  val io = new Bundle {
    val iterated_rgb = in Bits (24 bits)
    val iterated_alpha = in UInt (8 bits)
    val iterated_z = in UInt (8 bits)
    val texture_rgb = in Bits (24 bits)
    val texture_alpha = in UInt (8 bits)
    val color0_rgb = in Bits (24 bits)
    val color0_alpha = in UInt (8 bits)
    val color1_rgb = in Bits (24 bits)
    val color1_alpha = in UInt (8 bits)
    val rgb_sel = in Bits (2 bits)
    val local_sel = in Bits (1 bits)
    val local_override = in Bool ()
    val zero_other = in Bool ()
    val sub_local = in Bool ()
    val factor_sel = in Bits (3 bits)
    val reverse_blend = in Bool ()
    val add_mode = in Bits (2 bits)
    val invert_rgb = in Bool ()
    val alpha_sel = in Bits (2 bits)
    val alpha_local_sel = in Bits (2 bits)
    val alpha_zero_other = in Bool ()
    val alpha_sub_local = in Bool ()
    val alpha_factor_sel = in Bits (3 bits)
    val alpha_reverse_blend = in Bool ()
    val alpha_add_mode = in Bits (2 bits)
    val invert_alpha = in Bool ()
    val out = spinal.core.out(Bits(32 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsColorCombine")
  addRTLPath(XlsRtl.path("VoodooXlsColorCombine"))
}

class XlsDataSwizzle extends BlackBox {
  val io = new Bundle {
    val data = in Bits (32 bits)
    val word_swap = in Bool ()
    val byte_swizzle = in Bool ()
    val out = spinal.core.out(Bits(32 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsDataSwizzle")
  addRTLPath(XlsRtl.path("VoodooXlsDataSwizzle"))
}

class XlsPixelTests extends BlackBox {
  val io = new Bundle {
    val color = in Bits (24 bits)
    val chroma = in Bits (24 bits)
    val alpha = in UInt (8 bits)
    val alpha_reference = in UInt (8 bits)
    val alpha_function = in UInt (3 bits)
    val out = spinal.core.out(Bits(2 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsPixelTests")
  addRTLPath(XlsRtl.path("VoodooXlsPixelTests"))
}

class XlsLfbDecode extends BlackBox {
  val io = new Bundle {
    val data32 = in Bits (32 bits)
    val data16 = in UInt (16 bits)
    val format = in UInt (4 bits)
    val lanes = in UInt (2 bits)
    val default_depth = in Bits (16 bits)
    val alpha_planes = in Bool ()
    val out = spinal.core.out(Bits(50 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsLfbDecode")
  addRTLPath(XlsRtl.path("VoodooXlsLfbDecode"))
}

class XlsTexelDecode extends BlackBox {
  val io = new Bundle {
    val texel = in Bits (16 bits)
    val format = in UInt (4 bits)
    val palette = in Bits (24 bits)
    val ncc_y = in UInt (8 bits)
    val ncc_i = in Bits (27 bits)
    val ncc_q = in Bits (27 bits)
    val out = spinal.core.out(Bits(32 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTexelDecode")
  addRTLPath(XlsRtl.path("VoodooXlsTexelDecode"))
}

class XlsTextureFilter extends BlackBox {
  val io = new Bundle {
    val t0 = in Bits (32 bits)
    val t1 = in Bits (32 bits)
    val t2 = in Bits (32 bits)
    val t3 = in Bits (32 bits)
    val ds = in UInt (4 bits)
    val dt = in UInt (4 bits)
    val bilinear = in Bool ()
    val out = spinal.core.out(Bits(32 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTextureFilter")
  addRTLPath(XlsRtl.path("VoodooXlsTextureFilter"))
}

class XlsTmuRecipMeta extends BlackBox {
  val io = new Bundle {
    val w = in SInt (48 bits)
    val clamp_w = in Bool ()
    val out = spinal.core.out(Bits(25 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuRecipMeta")
  addRTLPath(XlsRtl.path("VoodooXlsTmuRecipMeta"))
}

class XlsTmuRecip extends BlackBox {
  val io = new Bundle {
    val base = in UInt (17 bits)
    val difference = in UInt (17 bits)
    val fraction = in UInt (8 bits)
    val log_fraction = in UInt (8 bits)
    val msb = in UInt (7 bits)
    val valid = in Bool ()
    val out = spinal.core.out(Bits(33 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuRecip")
  addRTLPath(XlsRtl.path("VoodooXlsTmuRecip"))
}

class XlsTmuPerspectiveCoords extends BlackBox {
  val io = new Bundle {
    val s = in SInt (48 bits)
    val t = in SInt (48 bits)
    val interpolated = in UInt (17 bits)
    val msb = in UInt (7 bits)
    val perspective = in Bool ()
    val out = spinal.core.out(Bits(36 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuPerspectiveCoords")
  addRTLPath(XlsRtl.path("VoodooXlsTmuPerspectiveCoords"))
}

class XlsTmuGradient extends BlackBox {
  val io = new Bundle {
    val ds_dx = in SInt (32 bits)
    val dt_dx = in SInt (32 bits)
    val ds_dy = in SInt (32 bits)
    val dt_dy = in SInt (32 bits)
    val out = spinal.core.out(UInt(64 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuGradient")
  addRTLPath(XlsRtl.path("VoodooXlsTmuGradient"))
}

class XlsTmuLodIndex extends BlackBox {
  val io = new Bundle {
    val temp_lod = in UInt (64 bits)
    val out = spinal.core.out(Bits(15 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuLodIndex")
  addRTLPath(XlsRtl.path("VoodooXlsTmuLodIndex"))
}

class XlsTmuBaseLod extends BlackBox {
  val io = new Bundle {
    val temp_lod = in UInt (64 bits)
    val msb = in UInt (7 bits)
    val log_fraction = in UInt (8 bits)
    val out = spinal.core.out(SInt(16 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuBaseLod")
  addRTLPath(XlsRtl.path("VoodooXlsTmuBaseLod"))
}

class XlsTmuLodSelect extends BlackBox {
  val io = new Bundle {
    val base_lod = in SInt (16 bits)
    val perspective_adjust = in SInt (16 bits)
    val t_lod = in Bits (27 bits)
    val out = spinal.core.out(Bits(12 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuLodSelect")
  addRTLPath(XlsRtl.path("VoodooXlsTmuLodSelect"))
}

class XlsTmuCoordinates extends BlackBox {
  val io = new Bundle {
    val tex_s = in SInt (18 bits)
    val tex_t = in SInt (18 bits)
    val level = in UInt (4 bits)
    val clamp_zero = in Bool ()
    val out = spinal.core.out(Bits(64 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuCoordinates")
  addRTLPath(XlsRtl.path("VoodooXlsTmuCoordinates"))
}

class XlsTmuAddresses extends BlackBox {
  val io = new Bundle {
    val point_s = in SInt (14 bits)
    val point_t = in SInt (14 bits)
    val integer_s = in SInt (14 bits)
    val integer_t = in SInt (14 bits)
    val width_bits = in UInt (4 bits)
    val height_bits = in UInt (4 bits)
    val clamp_s = in Bool ()
    val clamp_t = in Bool ()
    val is_16_bit = in Bool ()
    val packed_layout = in Bool ()
    val lod_base = in UInt (22 bits)
    val lod_shift = in UInt (4 bits)
    val base0 = in UInt (24 bits)
    val base1 = in UInt (24 bits)
    val base2 = in UInt (24 bits)
    val base38 = in UInt (24 bits)
    val multibase = in Bool ()
    val level = in UInt (4 bits)
    val out = spinal.core.out(Bits(130 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuAddresses")
  addRTLPath(XlsRtl.path("VoodooXlsTmuAddresses"))
}

class XlsTmuPackedDecode extends BlackBox {
  val io = new Bundle {
    val line0 = in UInt (22 bits)
    val line1 = in UInt (22 bits)
    val line2 = in UInt (22 bits)
    val line3 = in UInt (22 bits)
    val addr0 = in UInt (22 bits)
    val addr1 = in UInt (22 bits)
    val addr2 = in UInt (22 bits)
    val addr3 = in UInt (22 bits)
    val is_16_bit = in Bool ()
    val lod_base = in UInt (22 bits)
    val lod_shift = in UInt (4 bits)
    val bases = in Bits (198 bits)
    val ends = in Bits (198 bits)
    val shifts = in Bits (36 bits)
    val out = spinal.core.out(Bits(80 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTmuPackedDecode")
  addRTLPath(XlsRtl.path("VoodooXlsTmuPackedDecode"))
}

class XlsTextureWriteAddress extends BlackBox {
  val io = new Bundle {
    val pci_address = in UInt (23 bits)
    val is_16_bit = in Bool ()
    val sequential_8_bit = in Bool ()
    val packed_layout = in Bool ()
    val lod_base = in UInt (22 bits)
    val lod_shift = in UInt (4 bits)
    val base0 = in UInt (24 bits)
    val base1 = in UInt (24 bits)
    val base2 = in UInt (24 bits)
    val base38 = in UInt (24 bits)
    val multibase = in Bool ()
    val out = spinal.core.out(Bits(27 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTextureWriteAddress")
  addRTLPath(XlsRtl.path("VoodooXlsTextureWriteAddress"))
}

class XlsTriangleIterLoad extends BlackBox {
  val io = new Bundle {
    val start = in SInt (48 bits)
    val dx = in SInt (48 bits)
    val dy = in SInt (48 bits)
    val dx_subpixel = in UInt (4 bits)
    val dy_subpixel = in UInt (4 bits)
    val adjust = in Bool ()
    val dx_negative = in Bool ()
    val dy_negative = in Bool ()
    val out = spinal.core.out(Bits(198 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTriangleIterLoad")
  addRTLPath(XlsRtl.path("VoodooXlsTriangleIterLoad"))
}

class XlsTriangleIterStep extends BlackBox {
  val io = new Bundle {
    val accumulator = in SInt (66 bits)
    val dx_term = in SInt (66 bits)
    val dy_term = in SInt (66 bits)
    val dx_chunk = in UInt (2 bits)
    val dy_chunk = in UInt (2 bits)
    val out = spinal.core.out(Bits(198 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTriangleIterStep")
  addRTLPath(XlsRtl.path("VoodooXlsTriangleIterStep"))
}

class XlsSignedMagnitude16 extends BlackBox {
  val io = new Bundle {
    val value = in SInt (16 bits)
    val out = spinal.core.out(UInt(16 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsSignedMagnitude16")
  addRTLPath(XlsRtl.path("VoodooXlsSignedMagnitude16"))
}

class XlsTriangleSetup(coordinateWidth: Int) extends BlackBox {
  require(coordinateWidth == 16 || coordinateWidth == 20)
  private val moduleName =
    if (coordinateWidth == 16) "VoodooXlsTriangleSetupS16" else "VoodooXlsTriangleSetup"
  val io = new Bundle {
    val ax = in SInt (coordinateWidth bits)
    val ay = in SInt (coordinateWidth bits)
    val bx = in SInt (coordinateWidth bits)
    val by = in SInt (coordinateWidth bits)
    val cx = in SInt (coordinateWidth bits)
    val cy = in SInt (coordinateWidth bits)
    val reverse = in Bool ()
    val out = spinal.core.out(Bits(560 bits))
  }
  noIoPrefix()
  setDefinitionName(moduleName)
  addRTLPath(XlsRtl.path(moduleName))
}

class XlsSpanStep extends BlackBox {
  val io = new Bundle {
    val current = in SInt (48 bits)
    val dx = in SInt (48 bits)
    val dy = in SInt (48 bits)
    val mode = in UInt (2 bits)
    val out = spinal.core.out(SInt(48 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsSpanStep")
  addRTLPath(XlsRtl.path("VoodooXlsSpanStep"))
}

class XlsTextureLayout extends BlackBox {
  val io = new Bundle {
    val tex_base = in UInt (19 bits)
    val tex_base1 = in UInt (19 bits)
    val tex_base2 = in UInt (19 bits)
    val tex_base38 = in UInt (19 bits)
    val format = in UInt (4 bits)
    val aspect = in UInt (2 bits)
    val s_wider = in Bool ()
    val multibase = in Bool ()
    val out = spinal.core.out(Bits(432 bits))
  }
  noIoPrefix()
  setDefinitionName("VoodooXlsTextureLayout")
  addRTLPath(XlsRtl.path("VoodooXlsTextureLayout"))
}
