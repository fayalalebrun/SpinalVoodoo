package voodoo.trace

import javafx.application.{Application, Platform}
import javafx.scene.Scene
import javafx.scene.canvas.Canvas
import javafx.scene.image.{PixelWriter, WritableImage}
import javafx.scene.layout.Pane
import javafx.scene.paint.Color
import javafx.stage.Stage
import java.util.concurrent.{BlockingQueue, LinkedBlockingQueue}
import scala.collection.mutable
import scala.collection.JavaConverters._

/** JavaFX display window for real-time framebuffer rendering
  *
  * Displays RGB565 framebuffer data with pixel-by-pixel updates. Framebuffer format: 32-bit words
  * (RGB565 color + 16-bit depth/alpha)
  *
  * Controls:
  *   - Mouse wheel: zoom in/out
  *   - Click and drag: pan
  *   - Double-click: reset zoom and pan
  */
class DisplayWindow(width: Int, height: Int, stride: Int) {
  private var image: WritableImage = _
  private var imagePixelWriter: PixelWriter = _
  private var stage: Stage = _
  private var canvas: Canvas = _
  private var root: Pane = _

  // Queue for pixel updates from simulation thread
  private val updateQueue: BlockingQueue[PixelUpdate] = new LinkedBlockingQueue()

  // Framebuffer backing store (32-bit format: RGB565 + depth/alpha, 4 bytes per pixel)
  private val framebuffer = new Array[Int](stride * height)

  // Zoom and pan state
  private var zoomLevel = 1.0
  private var panX = 0.0
  private var panY = 0.0

  // Drag state for panning
  private var dragStartX = 0.0
  private var dragStartY = 0.0
  private var dragStartPanX = 0.0
  private var dragStartPanY = 0.0

  // Flag to trigger redraw
  private var needsRedraw = true

  case class PixelUpdate(addr: Long, value: Byte)

  /** Initialize JavaFX window (called from JavaFX application thread) */
  def initWindow(primaryStage: Stage): Unit = {
    stage = primaryStage
    image = new WritableImage(width, height)
    imagePixelWriter = image.getPixelWriter

    // Clear framebuffer to black
    for (y <- 0 until height; x <- 0 until width) {
      imagePixelWriter.setColor(x, y, Color.BLACK)
    }

    // Use Canvas and draw at scaled size (not node scaling) for pixel-perfect rendering
    canvas = new Canvas(width, height)

    root = new Pane()
    root.getChildren.add(canvas)
    root.setStyle("-fx-background-color: #222222;") // Dark gray background

    val scene = new Scene(root, width, height)

    // Zoom with scroll wheel
    scene.setOnScroll(event => {
      val scrollDelta = event.getDeltaY
      if (scrollDelta == 0) return

      val oldZoom = zoomLevel
      val zoomFactor = Math.pow(1.01, scrollDelta)
      zoomLevel = Math.max(0.5, Math.min(20.0, zoomLevel * zoomFactor))

      if (zoomLevel != oldZoom) {
        // Zoom towards mouse position
        val mouseX = event.getSceneX
        val mouseY = event.getSceneY

        // Calculate image coordinate under mouse before zoom
        val imgX = (mouseX - panX) / oldZoom
        val imgY = (mouseY - panY) / oldZoom

        // Adjust pan so same image point stays under mouse after zoom
        panX = mouseX - imgX * zoomLevel
        panY = mouseY - imgY * zoomLevel

        needsRedraw = true
      }
      event.consume()
    })

    // Pan with mouse drag
    scene.setOnMousePressed(event => {
      dragStartX = event.getSceneX
      dragStartY = event.getSceneY
      dragStartPanX = panX
      dragStartPanY = panY
    })

    scene.setOnMouseDragged(event => {
      panX = dragStartPanX + (event.getSceneX - dragStartX)
      panY = dragStartPanY + (event.getSceneY - dragStartY)
      needsRedraw = true
    })

    // Reset on double-click
    scene.setOnMouseClicked(event => {
      if (event.getClickCount == 2) {
        zoomLevel = 1.0
        panX = 0.0
        panY = 0.0
        needsRedraw = true
      }
    })

    stage.setTitle(s"Voodoo Trace Player - ${width}x${height} (scroll to zoom, drag to pan)")
    stage.setScene(scene)
    stage.setOnCloseRequest(_ => {
      Platform.exit()
      System.exit(0)
    })
    stage.show()

    // Start update loop
    startUpdateLoop()
  }

  /** Redraw canvas with current zoom and pan */
  private def redrawCanvas(): Unit = {
    val gc = canvas.getGraphicsContext2D

    // Resize canvas to fit zoomed image
    val scaledWidth = width * zoomLevel
    val scaledHeight = height * zoomLevel
    canvas.setWidth(scaledWidth)
    canvas.setHeight(scaledHeight)

    // Position canvas for pan
    canvas.setLayoutX(panX)
    canvas.setLayoutY(panY)

    // Clear and draw image at scaled size with nearest-neighbor interpolation
    gc.setImageSmoothing(false) // Pixel-perfect, no anti-aliasing!
    gc.clearRect(0, 0, scaledWidth, scaledHeight)
    gc.drawImage(image, 0, 0, scaledWidth, scaledHeight)
  }

  /** Start the pixel update loop (runs on JavaFX thread) */
  private def startUpdateLoop(): Unit = {
    val timer = new javafx.animation.AnimationTimer() {
      override def handle(now: Long): Unit = {
        // Process all pending updates
        val updates = mutable.ArrayBuffer[PixelUpdate]()
        updateQueue.drainTo(updates.asJava)

        if (updates.nonEmpty) {
          for (update <- updates) {
            applyPixelUpdate(update.addr, update.value)
          }
          needsRedraw = true
        }

        if (needsRedraw) {
          redrawCanvas()
          needsRedraw = false
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
        // Flip y coordinate: framebuffer has y=0 at bottom (OpenGL style),
        // but JavaFX has y=0 at top
        imagePixelWriter.setColor(x, height - 1 - y, color)
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
        imagePixelWriter.setColor(x, y, Color.BLACK)
      }
      needsRedraw = true
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
