//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.formal._

class RasterizerFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      addressWidth = 8 bits,
      maxFbDims = (8, 8) // Limit dimensions for formal tractability
    )

  val dut = Rasterizer(c, formalStrong = formalStrong)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

  // Tie off clip inputs
  dut.enableClipping := anyconst(Bool())
  dut.clipLeft := anyconst(UInt(10 bits))
  dut.clipRight := anyconst(UInt(10 bits))
  dut.clipLowY := anyconst(UInt(10 bits))
  dut.clipHighY := anyconst(UInt(10 bits))

  dut.i.valid := anyseq(Bool())
  dut.i.payload := anyseq(cloneOf(dut.i.payload))

  dut.o.ready := anyseq(Bool())

  assumeInitial(reset)
  when(reset) {
    assume(!dut.i.valid)
  }
  when(pastValid) {
    assume(!reset)
  }

  // Sanity check assumptions on input ranges
  when(dut.i.valid) {
    val zero = AFix.SQ(1 bits, 0 bits)
    zero := 0.0

    assume(dut.i.payload.xrange(0) >= zero)
    assume(dut.i.payload.xrange(1) >= dut.i.payload.xrange(0))
    assume(dut.i.payload.yrange(0) >= zero)
    assume(dut.i.payload.yrange(1) >= dut.i.payload.yrange(0))
    // Constrain iteration count to maxFbDims
    assume(
      (dut.i.payload.xrange(1) - dut.i.payload.xrange(0)).floor(0).asUInt *
        (dut.i.payload.yrange(1) - dut.i.payload.yrange(0)).floor(0).asUInt
        <= c.maxFbDims._1 * c.maxFbDims._2
    )
  }

  dut.i.formalAssumesSlave()
  if (formalStrong) {
    dut.o.formalAssertsMaster()
  }
}

class RasterizerFormalBmcDut extends RasterizerFormalDut(formalStrong = true)

class RasterizerFormalProveDut extends RasterizerFormalDut(formalStrong = false)

class RasterizerFormalTest extends SpinalFormalFunSuite {
  test("Rasterizer invariants bmc") {
    FormalConfig
      .withBMC(10)
      .withAsync
      .doVerify(new RasterizerFormalBmcDut)
  }

  test("Rasterizer invariants prove") {
    // Disabled for now as we don't expose state to assert bounds
    // FormalConfig
    //   .withProve(10)
    //   .withAsync
    //   .doVerify(new RasterizerFormalProveDut)
  }
}
