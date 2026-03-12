package math

import spinal.core._
import spinal.lib._
import spinal.lib.misc.pipeline._

class Fpxx2AFix(
    intNrBits: BitCount,
    fracNrBits: BitCount,
    c: FpxxConfig,
    pipeStages: Int = 1,
    generateFlags: Boolean = false
) extends Component {

  assert(c.ieee_like, "Can only handle IEEE compliant floats")
  assert(intNrBits.value + fracNrBits.value + 2 > c.mant_size, "Not enough bits for SInt size")

  val io = new Bundle {
    val op = slave Stream (Fpxx(c))
    val result = master Stream (new Bundle {
      val number = AFix.SQ(intNrBits, fracNrBits)
      val flags = generateFlags generate Fpxx.Flags()
      val overflow = Bool()
    })
  }

  val n0 = new Node {
    arbitrateFrom(io.op)

    val op = insert(io.op.payload)
    val sign = insert(op.sign)
    val flags = generateFlags generate insert(op.flags)
    val ge0 = insert(op.exp >= (c.bias - fracNrBits.value).max(0) && !op.is_zero())

    val _shift = SInt(c.exp_size + 2 bits)
    _shift := (intNrBits.value - 1 + c.bias) - (U"00" @@ op.exp).asSInt
    val shift = insert(_shift)
    val mant_full = insert((U"01" @@ op.mant).asSInt)
    val mant_2c = insert(sign ? -mant_full | mant_full)
  }

  val n1 = new Node {
    val shift_clipped = UInt(log2Up(intNrBits.value + fracNrBits.value) bits)
    shift_clipped := n0.shift.asUInt.resized

    val int = SInt(intNrBits + fracNrBits + (1 bit))

    val ovfl = Bool()

    when(n0.shift >= (intNrBits + fracNrBits).value || !n0.ge0) {
      int.clearAll()
      ovfl := False
    }.elsewhen(n0.shift < 0) {
      int := S(int.getWidth bits, (int.getWidth - 1) -> n0.mant_2c.msb, default -> !n0.mant_2c.msb)
      ovfl := True
    }.otherwise {
      int := (n0.mant_2c @@ S(
        0,
        (1 bit) + intNrBits + fracNrBits - (n0.mant_2c.getWidth bits)
      )) |>> shift_clipped
      ovfl := False
    }

    io.result.number.assignFromBits(int.asBits)
    io.result.overflow := ovfl
    if (generateFlags) io.result.payload.flags := n0.flags

    arbitrateTo(io.result)
  }

  val c01 = if (pipeStages >= 1) StageLink(n0, n1) else DirectLink(n0, n1)
  Builder(c01)
}
