package voodoo

import spinal.core._

case class Config(
    maxFbDims: (Int, Int) = (800, 600),
    addressWidth: BitCount = 26 bits,
    memBurstLengthWidth: Int = 11
)
