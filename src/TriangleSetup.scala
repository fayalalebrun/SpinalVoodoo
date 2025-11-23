package voodoo

import spinal.core._
import spinal.lib._

case class TriangleSetup(c: Config) extends Component {
  val i = slave(Stream(TriangleWithSign(c.vertexFormat)))
  val o = master(Stream(TriangleSetup.Output(c)))

  o << i.map { input =>
    val tri = input.tri
    val signBit = input.signBit
    val out = cloneOf(o.payload)

    out.xrange(0) := tri.map(_(0)).reduceBalancedTree((a, b) => a.min(b))
    out.xrange(1) := tri.map(_(0)).reduceBalancedTree((a, b) => a.max(b))
    out.yrange(0) := tri.map(_(1)).reduceBalancedTree((a, b) => a.min(b))
    out.yrange(1) := tri.map(_(1)).reduceBalancedTree((a, b) => a.max(b))

    // Compute edge coefficients for all 3 edges
    // Note: Our formula produces inverted signs. For CCW triangles (signBit=0),
    // we need to negate coefficients to get positive values inside the triangle.
    val coeffsVec = Vec(Coefficients(c), 3)
    Seq((0, 1), (1, 2), (2, 0)).zipWithIndex.foreach { case ((i0, i1), idx) =>
      val v0 = tri(i0)
      val v1 = tri(i1)

      val a_raw = v0(1) - v1(1)
      val b_raw = v1(0) - v0(0)

      // Top-left fill rule: computed using raw edge orientation (before sign flip)
      val isTopEdge = a_raw === AFix(0) && (b_raw < AFix(0))
      val isLeftEdge = (a_raw > AFix(0))
      val fillBias = AFix(!isTopEdge && !isLeftEdge)

      // Compute c using raw coefficients
      // Standard formula: c = x0*y1 - x1*y0
      val c_raw = v0(0) * v1(1) - v1(0) * v0(1) - fillBias

      // Flip signs for CW triangles (signBit=1) to match rasterizer expectation (edge >= 0 inside)
      // CCW (signBit=0): use raw coefficients (naturally positive inside)
      // CW (signBit=1): negate coefficients (to make positive inside)
      val a_neg = -a_raw
      val b_neg = -b_raw
      val c_neg = -c_raw

      val a = Mux(signBit, a_neg, a_raw)
      val b = Mux(signBit, b_neg, b_raw)
      val c = Mux(signBit, c_neg, c_raw)

      coeffsVec(idx).a := a.truncated
      coeffsVec(idx).b := b.truncated
      coeffsVec(idx).c := c.truncated
    }
    out.coeffs := coeffsVec

    // Compute starting edge values at the corner of the bounding box
    // Use coefficient format since edge values can be as large as coefficients
    val edgeStartVec = Vec(AFix(c.coefficientFormat), 3)
    out.coeffs.zipWithIndex.foreach { case (coeff, i) =>
      edgeStartVec(i) := (coeff.a * out.xrange(0) + coeff.b * out.yrange(0) + coeff.c).truncated
    }
    out.edgeStart := edgeStartVec

    out
  }
}

object TriangleSetup {
  case class Output(c: Config) extends Bundle {
    val coeffs = Vec.fill(3)(Coefficients(c))
    val xrange = vertex2d(c.vertexFormat)
    val yrange = vertex2d(c.vertexFormat)
    val edgeStart = Vec.fill(3)(AFix(c.coefficientFormat))
  }
}
