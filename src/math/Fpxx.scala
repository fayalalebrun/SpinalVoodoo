package math

import java.nio.ByteBuffer
import spinal.core._
import spinal.core.sim._

sealed trait InfinityEncoding
case class NoInfinity(replaceWith: BigInt) extends InfinityEncoding
case class IEEEInfinity() extends InfinityEncoding

sealed trait NanEncoding
case class IEEENan() extends NanEncoding
case class SpecialNan(encoding: BigInt) extends NanEncoding

sealed trait Bias
case class IEEEBias() extends Bias
case class CustomBias(bias: Int) extends Bias

object FpxxConfig {
  def float64() = FpxxConfig(11, 52)
  def float32() = FpxxConfig(8, 23)
  def float16() = FpxxConfig(5, 10)
  def bfloat16() = FpxxConfig(8, 7)
  def float8_e5m2fnuz() = FpxxConfig(
    5,
    2,
    SpecialNan(BigInt("10000000", 2)),
    inf_encoding = NoInfinity(BigInt("10000000", 2)),
    exp_bias = CustomBias(16),
    signed_zero = false
  )
  def float8_e4m3fnuz = FpxxConfig(
    4,
    3,
    SpecialNan(BigInt("10000000", 2)),
    inf_encoding = NoInfinity(BigInt("10000000", 2)),
    exp_bias = CustomBias(8),
    signed_zero = false
  )
}

case class FpxxConfig(
    exp_size: Int,
    mant_size: Int,
    nan_encoding: NanEncoding = IEEENan(),
    inf_encoding: InfinityEncoding = IEEEInfinity(),
    exp_bias: Bias = IEEEBias(),
    signed_zero: Boolean = true
) {

  def full_size = 1 + exp_size + mant_size

  def bias = exp_bias match {
    case CustomBias(bias) => bias
    case IEEEBias()       => (1 << (exp_size - 1)) - 1
  }

  def ieee_like =
    nan_encoding.isInstanceOf[IEEENan] && inf_encoding.isInstanceOf[IEEEInfinity] && exp_bias
      .isInstanceOf[IEEEBias] && signed_zero
}

object Fpxx {
  def apply(exp_size: Int, mant_size: Int): Fpxx = Fpxx(FpxxConfig(exp_size, mant_size))
  def apply(host: FpxxHost): Fpxx = {
    val fpxx = Fpxx(host.c)
    fpxx.sign.assignBigInt(host.sign)
    fpxx.exp.assignBigInt(host.exp)
    fpxx.mant.assignBigInt(host.mant)
    fpxx
  }

  case class Flags() extends Bundle {
    val nan = Bool()
    val inf = Bool()
    val sign = Bool()
  }
}

case class Fpxx(c: FpxxConfig) extends Bundle {
  val mant = UInt(c.mant_size bits)
  val exp = UInt(c.exp_size bits)
  val sign = Bool()

  def is_zero(): Bool = {
    exp === 0 && mant === 0
  }

  def is_nan(): Bool = {
    c.nan_encoding match {
      case IEEENan()            => this.exp.andR && this.mant.orR
      case SpecialNan(encoding) => this.asBits.asUInt === U(encoding)
    }
  }

  def is_infinite(): Bool = {
    c.inf_encoding match {
      case IEEEInfinity() => this.exp.andR && !this.mant.orR
      case NoInfinity(_)  => False
    }
  }

  def is_subnormal(): Bool = {
    exp === 0
  }

  def set_nan() = {
    c.nan_encoding match {
      case IEEENan() => {
        this.exp.setAll()
        this.mant.setAll()
      }
      case SpecialNan(encoding) =>
        this.assignFromBits(B(encoding))
    }
  }

  def set_inf() = {
    c.inf_encoding match {
      case IEEEInfinity() => {
        this.exp.setAll()
        this.mant := U(0)
      }
      case NoInfinity(replaceWith) => {
        this.assignFromBits(replaceWith)
      }
    }
  }

  def set_zero() = {
    if (!c.signed_zero) sign := False
    exp := 0
    mant := 0
  }

  def abs(): Fpxx = {
    val abs = Fpxx(c)

    abs.sign := False
    abs.exp := exp
    abs.mant := mant

    abs
  }

  def full_mant(): UInt = {
    mant.resize(c.mant_size + 1) | (1 << c.mant_size)
  }

  def init(): Fpxx = {
    sign init (False)
    exp init (0)
    mant init (0)
    this
  }

  def toHost(): FpxxHost = {
    val value = (this.sign.toBigInt << c.exp_size + c.mant_size) |
      (this.exp.toBigInt << c.mant_size) | this.mant.toBigInt
    FpxxHost(value, c)
  }

  def #=(value: FpxxHost) = {
    assert(value.c == c, "Cannot assign unless configuration is equal")
    this.sign.assignBigInt(value.sign)
    this.exp.assignBigInt(value.exp)
    this.mant.assignBigInt(value.mant)
  }

  def unary_~() = {
    val neg = cloneOf(this)
    neg.exp := exp
    neg.mant := mant
    neg.sign := !sign
    neg
  }

  def flags(): Fpxx.Flags = new Composite(this) {
    val flags = Fpxx.Flags()
    flags.sign := sign
    flags.inf := is_infinite()
    flags.nan := is_nan()
  }.flags
}

object FpxxHost {
  implicit def apply(f: Float): FpxxHost = {
    FpxxHost(BigInt(1, ByteBuffer.allocate(4).putFloat(f).array()), FpxxConfig.float32())
  }

  implicit def apply(d: Double): FpxxHost = {
    FpxxHost(BigInt(1, ByteBuffer.allocate(8).putDouble(d).array()), FpxxConfig.float64())
  }
}

case class FpxxHost(
    value: BigInt,
    c: FpxxConfig,
    maxUlpDist: Int = 0,
    testZeroSign: Boolean = true
) {
  val exp_mask: BigInt = (BigInt(1) << c.exp_size) - 1
  val mant_mask: BigInt = (BigInt(1) << c.mant_size) - 1

  def sign = value >> (c.exp_size + c.mant_size)
  def exp = (value >> c.mant_size) & exp_mask
  def mant = value & mant_mask

  def isDenormal = exp == 0 && mant != 0

  def isZero = exp == 0 && mant == 0

  def isNan = c.nan_encoding match {
    case IEEENan()            => exp == exp_mask && mant != 0
    case SpecialNan(encoding) => value == encoding
  }

  def isInf = c.inf_encoding match {
    case IEEEInfinity() => exp == exp_mask && mant == 0
    case NoInfinity(_)  => false
  }

  def ulpDist(other: FpxxHost) = {
    require(c == other.c)
    (value - other.value).abs
  }

  def setMaxUlpDist(newMaxUlpDist: Int) = FpxxHost(value, c, newMaxUlpDist, testZeroSign)

  override def toString = {
    val mantissa_aligned = mant << (4 - (c.mant_size % 4))
    val mantissa_str = mantissa_aligned.toString(16)
    val mantissa_str_padded = mantissa_str.reverse.padTo((c.mant_size + 3) / 4, '0').reverse
    val leading = if (exp == 0) "0" else "1"
    val exponent_centered = if (exp == 0) 1 - c.bias else exp.toInt - c.bias
    val signStr = if (sign == 1) "-" else ""
    val str =
      if (isNan) "NaN"
      else if (isInf) f"${signStr}inf"
      else {
        f"$signStr$leading%s.${mantissa_str_padded}p$exponent_centered%d"
      }
    f"$str(0x${value.toString(16)})"
  }

  override def equals(other: Any): Boolean = {
    other match {
      case o @ FpxxHost(_, oc, _, _) => {
        val maxUlp = maxUlpDist.max(o.maxUlpDist)
        c == oc &&
        (ulpDist(o) <= maxUlp ||
          isNan && o.isNan ||
          isInf && o.isInf && sign == o.sign ||
          (!testZeroSign || !o.testZeroSign) && isZero && o.isZero)
      }
      case _ => false
    }
  }
}
