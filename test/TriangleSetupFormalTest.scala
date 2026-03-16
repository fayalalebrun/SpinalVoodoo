//> using target.scope test

package voodoo

import spinal.core._
import spinal.core.formal._
import spinal.lib._
import spinal.lib.formal._

class TriangleSetupFormalDut(formalStrong: Boolean) extends Component {
  val c = Config
    .voodoo1(TraceConfig())
    .copy(
      vertexFormat = QFormat(5, 2, true),
      vColorFormat = QFormat(5, 2, true),
      vDepthFormat = QFormat(5, 2, true),
      wAccumFormat = QFormat(5, 2, true),
      coefficientFormat = QFormat(8, 2, true),
      texCoordsFormat = QFormat(5, 2, true),
      texCoordsAccumFormat = QFormat(8, 2, true),
      texCoordsHiFormat = QFormat(8, 2, true)
    )

  val dut = TriangleSetup(c, formalStrong = formalStrong)
  val reset = ClockDomain.current.isResetActive
  val pastValid = RegNext(True) init (False)

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

  // Pre-sort vertices y-coordinates to match rasterizer driver assumption (A=top, C=bottom)
  when(dut.i.valid) {
    val tri = dut.i.payload.triWithSign.tri
    assume(tri(0)(1) <= tri(1)(1))
    assume(tri(1)(1) <= tri(2)(1))

    // Constrain coordinates to prevent ceil() overflow in fixed point representation
    // vertexFormat has 12 integer bits, max value is 2047. ceil(2047.x) = 2048 (overflows to -2048)
    val maxInt = (1 << (c.vertexFormat.nonFraction - 1)) - 1
    for (v <- tri) {
      assume(v(0).floor(0).asSInt < maxInt)
      assume(v(1).floor(0).asSInt < maxInt)
    }
  }

  dut.i.formalAssumesSlave()
  if (formalStrong) {
    dut.o.formalAssertsMaster()
  }
}

class TriangleSetupFormalBmcDut extends TriangleSetupFormalDut(formalStrong = true)

class TriangleSetupFormalProveDut extends TriangleSetupFormalDut(formalStrong = false)

class TriangleSetupFormalTest extends SpinalFormalFunSuite {
  test("TriangleSetup invariants bmc") {
    FormalConfig
      .withBMC(1)
      .withAsync
      .doVerify(new TriangleSetupFormalBmcDut)
  }

  // No prove test needed for purely combinational component
}
