package voodoo.trace

import javafx.application.{Application, Platform}
import javafx.scene.Scene
import javafx.scene.image.{ImageView, PixelWriter, WritableImage}
import javafx.scene.layout.StackPane
import javafx.scene.paint.Color
import javafx.stage.Stage
import java.util.concurrent.{BlockingQueue, LinkedBlockingQueue}
import scala.collection.mutable
import scala.collection.JavaConverters._

/** JavaFX display window for real-time framebuffer rendering
  *
  * Displays RGB565 framebuffer data with pixel-by-pixel updates. Framebuffer format: 32-bit words
  * (RGB565 color + 16-bit depth/alpha)
  */
class DisplayWindow(width: Int, height: Int, stride: Int) {
  private var image: WritableImage = _
  private var pixelWriter: PixelWriter = _
  private var stage: Stage = _

  // Queue for pixel updates from simulation thread
  private val updateQueue: BlockingQueue[PixelUpdate] = new LinkedBlockingQueue()

  // Framebuffer backing store (32-bit format: RGB565 + depth/alpha, 4 bytes per pixel)
  private val framebuffer = new Array[Int](stride * height)

  case class PixelUpdate(addr: Long, value: Byte)

  /** Initialize JavaFX window (called from JavaFX application thread) */
  def initWindow(primaryStage: Stage): Unit = {
    stage = primaryStage
    image = new WritableImage(width, height)
    pixelWriter = image.getPixelWriter

    // Clear framebuffer to black
    for (y <- 0 until height; x <- 0 until width) {
      pixelWriter.setColor(x, y, Color.BLACK)
    }

    val imageView = new ImageView(image)
    val root = new StackPane()
    root.getChildren.add(imageView)

    val scene = new Scene(root, width, height)
    stage.setTitle(s"Voodoo Trace Player - ${width}x${height}")
    stage.setScene(scene)
    stage.setOnCloseRequest(_ => {
      Platform.exit()
      System.exit(0)
    })
    stage.show()

    // Start update loop
    startUpdateLoop()
  }

  /** Start the pixel update loop (runs on JavaFX thread) */
  private def startUpdateLoop(): Unit = {
    val timer = new javafx.animation.AnimationTimer() {
      override def handle(now: Long): Unit = {
        // Process all pending updates
        val updates = mutable.ArrayBuffer[PixelUpdate]()
        updateQueue.drainTo(updates.asJava)

        for (update <- updates) {
          applyPixelUpdate(update.addr, update.value)
        }
      }
    }
    timer.start()
  }

  /** Apply a pixel update to the framebuffer and display
    *
    * @param addr
    *   Byte address in framebuffer (4 bytes per pixel: RGB565 + depth/alpha)
    * @param value
    *   Byte value
    */
  private def applyPixelUpdate(addr: Long, value: Byte): Unit = {
    val pixelIndex = (addr / 4).toInt
    val byteOffset = (addr % 4).toInt

    if (pixelIndex >= 0 && pixelIndex < framebuffer.length) {
      // Update framebuffer (32-bit word: byte0, byte1, byte2, byte3)
      val mask = 0xff << (byteOffset * 8)
      val clearMask = ~mask
      val valueBits = (value & 0xff) << (byteOffset * 8)
      framebuffer(pixelIndex) = (framebuffer(pixelIndex) & clearMask) | valueBits

      // Convert pixel index to x,y coordinates
      val x = pixelIndex % stride
      val y = pixelIndex / stride

      // Only update display if within visible area
      if (x < width && y < height) {
        // Extract RGB565 from lower 16 bits (bytes 0 and 1)
        val rgb565 = framebuffer(pixelIndex) & 0xffff
        val r = ((rgb565 >> 11) & 0x1f) * 255 / 31
        val g = ((rgb565 >> 5) & 0x3f) * 255 / 63
        val b = (rgb565 & 0x1f) * 255 / 31

        val color = Color.rgb(r, g, b)
        pixelWriter.setColor(x, y, color)
      }
    }
  }

  /** Queue a pixel update (called from simulation thread)
    *
    * @param addr
    *   Byte address in framebuffer
    * @param value
    *   Byte value
    */
  def updatePixel(addr: Long, value: Byte): Unit = {
    updateQueue.offer(PixelUpdate(addr, value))
  }

  /** Launch JavaFX window (call from main thread) */
  def launch(): Unit = {
    // JavaFX needs to be launched from a separate thread
    val thread = new Thread(() => {
      Application.launch(classOf[DisplayWindowApp])
    })
    thread.setDaemon(true)
    thread.start()

    // Wait for window to initialize
    Thread.sleep(1000)
  }

  /** Clear the framebuffer */
  def clear(): Unit = {
    Platform.runLater(() => {
      for (y <- 0 until height; x <- 0 until width) {
        pixelWriter.setColor(x, y, Color.BLACK)
      }
    })
    framebuffer.indices.foreach(i => framebuffer(i) = 0)
  }
}

/** JavaFX Application class (required by JavaFX) */
class DisplayWindowApp extends Application {
  override def start(primaryStage: Stage): Unit = {
    DisplayWindowApp.window.initWindow(primaryStage)
  }
}

object DisplayWindowApp {
  var window: DisplayWindow = _
}

object DisplayWindow {
  def apply(width: Int, height: Int, stride: Int): DisplayWindow = {
    val window = new DisplayWindow(width, height, stride)
    DisplayWindowApp.window = window
    window.launch()
    window
  }
}
