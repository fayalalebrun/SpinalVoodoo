package voodoo

import org.scalatest.funsuite.AnyFunSuite
import spinal.core.sim._

class FramebufferAccessTest extends AnyFunSuite {
  private val config = Config.voodoo1()

  private def init(dut: FramebufferAccess): Unit = {
    dut.clockDomain.forkStimulus(10)
    dut.io.input.valid #= false
    dut.io.output.ready #= true
    dut.io.fbReadColorReq.ready #= true
    dut.io.fbReadAuxReq.ready #= true
    dut.io.fbReadColorRsp.valid #= false
    dut.io.fbReadAuxRsp.valid #= false
    dut.clockDomain.waitSampling()
  }

  private def payload(
      dut: FramebufferAccess,
      depth: Int,
      depthFunction: Int,
      depthEnable: Boolean = true,
      wDepth: Int = 0,
      wSelect: Boolean = false,
      sourceZa: Boolean = false,
      bias: Int = 0,
      blend: Boolean = false,
      srcFactor: Int = 0,
      dstFactor: Int = 0
  ): Unit = {
    dut.io.input.payload.coords(0) #= 3
    dut.io.input.payload.coords(1) #= 4
    dut.io.input.payload.color.r #= 10
    dut.io.input.payload.color.g #= 20
    dut.io.input.payload.color.b #= 30
    dut.io.input.payload.alpha #= 255
    dut.io.input.payload.depth.raw #= (BigInt(depth) << 12)
    dut.io.input.payload.wDepth #= wDepth
    dut.io.input.payload.colorBeforeFog.r #= 10
    dut.io.input.payload.colorBeforeFog.g #= 20
    dut.io.input.payload.colorBeforeFog.b #= 30
    dut.io.input.payload.chromaKey #= 0
    dut.io.input.payload.zaColor #= (bias & 0xffff)

    val alpha = dut.io.input.payload.alphaMode
    alpha.alphaTestEnable #= false
    alpha.alphaFunc #= 7
    alpha.alphaBlendEnable #= blend
    alpha.enableAntiAlias #= false
    alpha.reserved1 #= 0
    alpha.rgbSrcFact #= srcFactor
    alpha.rgbDstFact #= dstFactor
    alpha.aSrcFact #= 0
    alpha.aDstFact #= 0
    alpha.alphaRef #= 0

    val fbz = dut.io.input.payload.fbzMode
    fbz.enableClipping #= false
    fbz.enableChromaKey #= false
    fbz.enableStipple #= false
    fbz.wBufferSelect #= wSelect
    fbz.enableDepthBuffer #= depthEnable
    fbz.depthFunction #= depthFunction
    fbz.enableDithering #= false
    fbz.rgbBufferMask #= true
    fbz.auxBufferMask #= true
    fbz.ditherAlgorithm #= false
    fbz.enableStipplePattern #= false
    fbz.enableAlphaMask #= false
    fbz.drawBuffer #= 0
    fbz.enableDepthBias #= bias != 0 && !sourceZa
    fbz.yOrigin #= false
    fbz.enableAlphaPlanes #= false
    fbz.enableDitherSubtract #= false
    fbz.depthSourceSelect #= sourceZa

    dut.io.input.payload.routing.colorBaseAddr #= 0x1000
    dut.io.input.payload.routing.auxBaseAddr #= 0x2000
    dut.io.input.payload.routing.pixelStride #= 640
  }

  private case class Result(output: Boolean, zFail: Boolean, color: (Int, Int, Int), depth: Int)

  private def send(
      dut: FramebufferAccess,
      oldDepth: Option[Int],
      oldColor: Option[Int]
  ): Result = {
    while (dut.io.busy.toBoolean) dut.clockDomain.waitSampling()
    dut.io.fbReadColorReq.ready #= oldColor.isEmpty
    dut.io.fbReadAuxReq.ready #= oldDepth.isEmpty
    dut.io.input.valid #= true
    while (oldColor.nonEmpty && !dut.io.fbReadColorReq.valid.toBoolean)
      dut.clockDomain.waitSampling()
    while (oldDepth.nonEmpty && !dut.io.fbReadAuxReq.valid.toBoolean)
      dut.clockDomain.waitSampling()
    dut.io.fbReadColorReq.ready #= true
    dut.io.fbReadAuxReq.ready #= true
    dut.clockDomain.waitSamplingWhere(dut.io.input.ready.toBoolean)
    dut.io.input.valid #= false

    oldColor.foreach { value =>
      dut.io.fbReadColorRsp.payload.data #= value
      dut.io.fbReadColorRsp.valid #= true
      dut.clockDomain.waitSamplingWhere(dut.io.fbReadColorRsp.ready.toBoolean)
      dut.io.fbReadColorRsp.valid #= false
    }
    oldDepth.foreach { value =>
      dut.io.fbReadAuxRsp.payload.data #= value
      dut.io.fbReadAuxRsp.valid #= true
      dut.clockDomain.waitSamplingWhere(dut.io.fbReadAuxRsp.ready.toBoolean)
      dut.io.fbReadAuxRsp.valid #= false
    }

    var output = false
    var zFail = false
    var color = (0, 0, 0)
    var depth = 0
    for (_ <- 0 until 40 if !output) {
      dut.clockDomain.waitSampling()
      zFail ||= dut.io.zFuncFail.toBoolean
      if (dut.io.output.valid.toBoolean) {
        output = true
        color = (
          dut.io.output.payload.color.r.toInt,
          dut.io.output.payload.color.g.toInt,
          dut.io.output.payload.color.b.toInt
        )
        depth = dut.io.output.payload.newDepth.toInt
      }
    }
    Result(output, zFail, color, depth)
  }

  test("all depth functions pass and reject correctly") {
    SimConfig.withIVerilog.compile(FramebufferAccess(config)).doSim { dut =>
      init(dut)
      val expected = Seq(false, true, false, true, false, true, false, true)
      for ((passes, function) <- expected.zipWithIndex) {
        payload(dut, depth = 10, depthFunction = function)
        val result = send(dut, oldDepth = Some(20), oldColor = None)
        assert(result.output == passes, s"depth function $function output mismatch")
        assert(result.zFail == !passes, s"depth function $function failure pulse mismatch")
        if (passes) assert(result.depth == 10)
      }
    }
  }

  test("W depth, zaColor source, and signed depth bias select the comparison depth") {
    SimConfig.withIVerilog.compile(FramebufferAccess(config)).doSim { dut =>
      init(dut)

      payload(dut, depth = 10, depthFunction = 2, wDepth = 0x1234, wSelect = true)
      assert(send(dut, Some(0x1234), None).output)

      payload(dut, depth = 10, depthFunction = 2, sourceZa = true, bias = 0x3456)
      assert(send(dut, Some(0x3456), None).output)

      payload(dut, depth = 10, depthFunction = 2, bias = 1)
      val biased = send(dut, Some(11), None)
      assert(biased.output && biased.depth == 11)

      payload(dut, depth = 10, depthFunction = 2, bias = 0xffff)
      val negativeBias = send(dut, Some(9), None)
      assert(negativeBias.output && negativeBias.depth == 9)
    }
  }

  test("read bypass preserves source color and alpha blend can select destination") {
    SimConfig.withIVerilog.compile(FramebufferAccess(config)).doSim { dut =>
      init(dut)

      payload(dut, depth = 10, depthFunction = 7, depthEnable = false)
      val bypass = send(dut, oldDepth = None, oldColor = None)
      assert(bypass.output && bypass.color == (10, 20, 30))

      payload(
        dut,
        depth = 10,
        depthFunction = 7,
        depthEnable = false,
        blend = true,
        srcFactor = 0,
        dstFactor = 4
      )
      val blended = send(dut, oldDepth = None, oldColor = Some(0xf800))
      assert(blended.output && blended.color == (255, 0, 0))
    }
  }
}
