package voodoo

import spinal.core._
import java.nio.file.{Files, Path, Paths, StandardCopyOption}

object GenSupport {
  def simVerilog(targetDirectory: String = "emu/sim/rtl"): SpinalConfig =
    SpinalConfig(mode = Verilog, targetDirectory = targetDirectory)

  def de10Verilog(targetDirectory: String = "hw/de10/rtl"): SpinalConfig =
    SpinalConfig(
      mode = Verilog,
      targetDirectory = targetDirectory,
      defaultConfigForClockDomains = ClockDomainConfig(
        resetKind = ASYNC,
        resetActiveLevel = HIGH
      )
    )

  def mirrorRomSidecars(targetDirectory: String, topName: String): Unit = {
    val dir = Paths.get(targetDirectory)
    if (!Files.isDirectory(dir)) return

    val entries = Files.list(dir)
    try {
      val paths = entries.toArray.map(_.asInstanceOf[Path])
      val targetNames =
        (paths.iterator
          .map(_.getFileName.toString)
          .filter(_.endsWith(".v"))
          .map(_.stripSuffix(".v")) ++ Iterator(topName)).toSet

      paths.foreach { path =>
        val fileName = path.getFileName.toString
        val split = fileName.indexOf(".v_")
        if (split > 0 && split + 3 < fileName.length && fileName.endsWith(".bin")) {
          val suffix = fileName.substring(split)
          targetNames.foreach { targetBase =>
            val targetName = targetBase + suffix
            if (targetName != fileName) {
              val targetPath = dir.resolve(targetName)
              if (!Files.exists(targetPath)) {
                Files.copy(path, targetPath, StandardCopyOption.REPLACE_EXISTING)
              }
            }
          }
        }
      }
    } finally {
      entries.close()
    }
  }
}
