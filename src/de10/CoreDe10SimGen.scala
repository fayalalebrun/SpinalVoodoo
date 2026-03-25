package voodoo.de10

import spinal.core._
import voodoo.Config

object CoreDe10SimGen extends App {
  val useFbWriteBuffer =
    !args.contains("--no-fb-write-buffer") && !args.contains("--no-fb-fill-cache")
  val useTexFillCache = !args.contains("--no-tex-fill-cache")

  SpinalConfig(
    mode = Verilog,
    targetDirectory = "emu/sim/rtl"
  ).generate(
    CoreDe10(
      Config
        .voodoo1()
        .copy(
          useFbWriteBuffer = useFbWriteBuffer,
          useTexFillCache = useTexFillCache
        )
    )
  )
}
