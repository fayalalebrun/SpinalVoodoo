package voodoo.de10

object De10AddressMap {
  // Voodoo-style BAR layout exposed by Core.cpuBus decode.
  val regBase = 0x000000
  val regSpan = 0x400000

  val lfbBase = 0x400000
  val lfbSpan = 0x400000

  val texBase = 0x800000
  val texSpan = 0x800000

  // Planned external memory footprint.
  val fbMemBytes = 4 * 1024 * 1024
  val texMemBytes = 8 * 1024 * 1024

  // Reserve the top 16 MiB of HPS DDR for FPGA-owned framebuffer/texture
  // traffic. Linux must keep this range out of normal use via boot DT
  // reserved-memory/no-map, with a matching mem= limit still used during
  // bring-up as an extra guardrail. The Core-side memory buses use local
  // offsets; the DE10 memory backend expands them to 32-bit physical
  // addresses in this carveout.
  val hpsDdrBytes = 0x40000000
  // The current board software still boots Linux with `mem=1008M`, leaving the
  // top 16 MiB out of general RAM. Keep the active FPGA window to the minimum
  // footprint the design needs (4 MiB framebuffer + 8 MiB texture) so we can
  // test whether the remaining resets are sensitive to the exact DDR region.
  val ddrCarveoutBytes = 12 * 1024 * 1024
  val ddrBase = hpsDdrBytes - ddrCarveoutBytes
  val fbMemBase = ddrBase
  val texMemBase = fbMemBase + fbMemBytes
  val ddrCarveoutEnd = ddrBase + ddrCarveoutBytes

  val fbMemMask = fbMemBytes - 1
  val texMemMask = texMemBytes - 1
}
