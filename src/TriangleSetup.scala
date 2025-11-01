package voodoo

import spinal.core._
import spinal.lib._

case class TriangleSetup(c: Config) extends Component {
  val i = slave(Stream(triangle(c.vertexFormat)))
  val o = master(Stream(TriangleSetup.Output(c.vertexFormat)))

  o << i.map { tri =>
    val out = cloneOf(o.payload)

    out.xrange(0) := tri.map(_(0)).reduceBalancedTree((a, b) => a.min(b))
    out.xrange(1) := tri.map(_(0)).reduceBalancedTree((a, b) => a.max(b))
    out.yrange(0) := tri.map(_(1)).reduceBalancedTree((a, b) => a.min(b))
    out.yrange(1) := tri.map(_(1)).reduceBalancedTree((a, b) => a.max(b))

    out.coeffs := Vec(Seq((0, 1), (1, 2), (2, 0)).map { case (i0, i1) =>
      val v0 = tri(i0)
      val v1 = tri(i1)

      val a = v0(1) - v1(1)
      val b = v1(0) - v0(0)

      val isTopEdge = a === AFix(0) && (b < AFix(0))
      val isLeftEdge = (a > AFix(0))

      val c = v0(0) * b - v0(1) * a - AFix(!isTopEdge && !isLeftEdge)

      val coeff = cloneOf(out.coeffs.dataType)
      coeff.a := a.truncated
      coeff.b := b.truncated
      coeff.c := c.truncated
      coeff
    })

    out.edgeStart := Vec(out.coeffs.zipWithIndex.map { case (coeff, i) =>
      (coeff.a * out.xrange(0) + coeff.b * out.yrange(0) + coeff.c).truncated
    })

    out
  }
}

object TriangleSetup {
  case class Output(vertexFmt: QFormat) extends Bundle {
    val coeffs = Vec.fill(3)(Coefficients(vertexFmt))
    val xrange = vertex2d(vertexFmt)
    val yrange = vertex2d(vertexFmt)
    val edgeStart = Vec.fill(3)(AFix(vertexFmt))
  }
}
