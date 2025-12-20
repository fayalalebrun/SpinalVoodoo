package voodoo.trace

import javafx.application.{Application, Platform}
import javafx.beans.property.{
  SimpleDoubleProperty,
  SimpleIntegerProperty,
  SimpleLongProperty,
  SimpleStringProperty
}
import javafx.collections.FXCollections
import javafx.geometry.{Insets, Orientation}
import javafx.scene.Scene
import javafx.scene.canvas.Canvas
import javafx.scene.control.{Label, SplitPane, TableColumn, TableView, cell}
import javafx.scene.image.{PixelWriter, WritableImage}
import javafx.scene.layout.{Pane, Priority, VBox}
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
  *   - Single-click: inspect triangles/pixels at position
  *   - Double-click: reset zoom and pan
  */
class DisplayWindow(
    width: Int,
    height: Int,
    stride: Int,
    debugTracker: Option[DebugTracker] = None
) {
  private var image: WritableImage = _
  private var imagePixelWriter: PixelWriter = _
  private var stage: Stage = _
  private var canvas: Canvas = _
  private var canvasPane: Pane = _

  // Debug panel components
  private var positionLabel: Label = _
  private var triangleTable: TableView[TriangleRow] = _
  private var pixelTable: TableView[PixelRow] = _

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
  private var wasDragged = false // Track if mouse was dragged (to distinguish from click)

  // Flag to trigger redraw (start false, first pixel update will trigger)
  private var needsRedraw = false

  // Flag to track if window has been closed
  @volatile private var windowClosed = false

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

    canvasPane = new Pane()
    canvasPane.getChildren.add(canvas)
    canvasPane.setStyle("-fx-background-color: #222222;") // Dark gray background

    // Create root layout - either SplitPane with debug panel or just canvas
    val root = debugTracker match {
      case Some(_) =>
        val splitPane = new SplitPane()
        splitPane.setOrientation(Orientation.HORIZONTAL)
        splitPane.getItems.addAll(canvasPane, createDebugPanel())
        splitPane.setDividerPositions(0.7)
        splitPane
      case None =>
        canvasPane
    }

    val sceneWidth = if (debugTracker.isDefined) width + 350 else width
    val scene = new Scene(root, sceneWidth, height)

    // Zoom with scroll wheel (on canvasPane for proper containment)
    canvasPane.setOnScroll(event => {
      val scrollDelta = event.getDeltaY
      if (scrollDelta != 0) {
        val oldZoom = zoomLevel
        val zoomFactor = Math.pow(1.01, scrollDelta)
        // Cap zoom to avoid exceeding texture limits (4096 / 640 ≈ 6.4)
        zoomLevel = Math.max(0.5, Math.min(6.0, zoomLevel * zoomFactor))

        if (zoomLevel != oldZoom) {
          // Zoom towards mouse position (use local coords relative to canvasPane)
          val mouseX = event.getX
          val mouseY = event.getY

          // Calculate image coordinate under mouse before zoom
          val imgX = (mouseX - panX) / oldZoom
          val imgY = (mouseY - panY) / oldZoom

          // Adjust pan so same image point stays under mouse after zoom
          panX = mouseX - imgX * zoomLevel
          panY = mouseY - imgY * zoomLevel

          needsRedraw = true
        }
        event.consume()
      }
    })

    // Pan with mouse drag (on canvasPane)
    canvasPane.setOnMousePressed(event => {
      dragStartX = event.getX
      dragStartY = event.getY
      dragStartPanX = panX
      dragStartPanY = panY
      wasDragged = false
    })

    canvasPane.setOnMouseDragged(event => {
      val dx = event.getX - dragStartX
      val dy = event.getY - dragStartY
      // Only start dragging if moved more than 3 pixels (to allow for small jitter on clicks)
      if (Math.abs(dx) > 3 || Math.abs(dy) > 3) {
        wasDragged = true
        panX = dragStartPanX + dx
        panY = dragStartPanY + dy
        needsRedraw = true
      }
    })

    // Handle clicks on canvasPane: single-click for inspection, double-click for reset
    canvasPane.setOnMouseClicked(event => {
      if (event.getClickCount == 2) {
        // Double-click: reset zoom and pan
        zoomLevel = 1.0
        panX = 0.0
        panY = 0.0
        needsRedraw = true
      } else if (event.getClickCount == 1 && !wasDragged && debugTracker.isDefined) {
        // Single-click: inspect position (only if we have debug tracker and wasn't dragging)
        val mouseX = event.getX
        val mouseY = event.getY

        // Convert screen coords to framebuffer coords
        val fbX = ((mouseX - panX) / zoomLevel).toInt
        // Flip Y coordinate (framebuffer has y=0 at bottom)
        val fbY = height - 1 - ((mouseY - panY) / zoomLevel).toInt

        if (fbX >= 0 && fbX < width && fbY >= 0 && fbY < height) {
          updateDebugPanel(fbX, fbY)
        }
      }
    })

    // Update position on mouse move (on canvasPane)
    canvasPane.setOnMouseMoved(event => {
      if (debugTracker.isDefined && positionLabel != null) {
        val mouseX = event.getX
        val mouseY = event.getY

        val fbX = ((mouseX - panX) / zoomLevel).toInt
        val fbY = height - 1 - ((mouseY - panY) / zoomLevel).toInt

        if (fbX >= 0 && fbX < width && fbY >= 0 && fbY < height) {
          positionLabel.setText(s"Position: ($fbX, $fbY)")
        } else {
          positionLabel.setText("Position: (outside)")
        }
      }
    })

    stage.setTitle(s"Voodoo Trace Player - ${width}x${height} (scroll to zoom, drag to pan)")
    stage.setScene(scene)
    stage.setOnCloseRequest(_ => {
      windowClosed = true
      Platform.exit()
    })
    stage.show()

    // Do initial draw after stage is shown
    redrawCanvas()

    // Start update loop
    startUpdateLoop()
  }

  /** Redraw canvas with current zoom and pan */
  private def redrawCanvas(): Unit = {
    // Guard against redraw before canvas is fully initialized
    if (canvas == null || image == null) return

    val gc = canvas.getGraphicsContext2D
    if (gc == null) return

    // Resize canvas to fit zoomed image, but cap to avoid exceeding texture limits
    val maxTextureSize = 4096.0
    val scaledWidth = Math.min(width * zoomLevel, maxTextureSize)
    val scaledHeight = Math.min(height * zoomLevel, maxTextureSize)
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

  /** Check if window has been closed */
  def isClosed: Boolean = windowClosed

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

  /** Create the debug panel with position label and tables */
  private def createDebugPanel(): VBox = {
    val panel = new VBox(10)
    panel.setPadding(new Insets(10))
    panel.setStyle("-fx-background-color: #333333;")
    panel.setMinWidth(300)

    // Position label
    positionLabel = new Label("Position: (click to inspect)")
    positionLabel.setStyle("-fx-text-fill: white; -fx-font-size: 14px; -fx-font-weight: bold;")

    // Triangle table
    val triangleLabel = new Label("Triangles at position:")
    triangleLabel.setStyle("-fx-text-fill: #aaaaaa; -fx-font-size: 12px;")

    triangleTable = new TableView[TriangleRow]()
    triangleTable.setStyle("-fx-background-color: #444444;")
    triangleTable.setColumnResizePolicy(TableView.CONSTRAINED_RESIZE_POLICY)

    val triIdCol = new TableColumn[TriangleRow, java.lang.Long]("ID")
    triIdCol.setCellValueFactory(data => new SimpleLongProperty(data.getValue.id).asObject())
    triIdCol.setPrefWidth(50)

    val triBboxCol = new TableColumn[TriangleRow, String]("BBox")
    triBboxCol.setCellValueFactory(data => new SimpleStringProperty(data.getValue.bbox))
    triBboxCol.setPrefWidth(120)

    val triVertCol = new TableColumn[TriangleRow, String]("Vertices")
    triVertCol.setCellValueFactory(data => new SimpleStringProperty(data.getValue.vertices))
    triVertCol.setPrefWidth(120)

    val triPixelsCol = new TableColumn[TriangleRow, java.lang.Integer]("Pixels")
    triPixelsCol.setCellValueFactory(data =>
      new SimpleIntegerProperty(data.getValue.pixels).asObject()
    )
    triPixelsCol.setPrefWidth(50)

    triangleTable.getColumns.addAll(triIdCol, triBboxCol, triVertCol, triPixelsCol)
    VBox.setVgrow(triangleTable, Priority.ALWAYS)

    // Pixel table
    val pixelLabel = new Label("Pixel history at position:")
    pixelLabel.setStyle("-fx-text-fill: #aaaaaa; -fx-font-size: 12px;")

    pixelTable = new TableView[PixelRow]()
    pixelTable.setStyle("-fx-background-color: #444444;")
    pixelTable.setColumnResizePolicy(TableView.CONSTRAINED_RESIZE_POLICY)

    val pxTriCol = new TableColumn[PixelRow, java.lang.Long]("Tri")
    pxTriCol.setCellValueFactory(data =>
      new SimpleLongProperty(data.getValue.triangleId).asObject()
    )
    pxTriCol.setPrefWidth(50)

    val pxColorCol = new TableColumn[PixelRow, String]("Color (RGB)")
    pxColorCol.setCellValueFactory(data => new SimpleStringProperty(data.getValue.color))
    pxColorCol.setPrefWidth(100)

    val pxDepthCol = new TableColumn[PixelRow, String]("Depth")
    pxDepthCol.setCellValueFactory(data => new SimpleStringProperty(data.getValue.depth))
    pxDepthCol.setPrefWidth(80)

    val pxAlphaCol = new TableColumn[PixelRow, String]("Alpha")
    pxAlphaCol.setCellValueFactory(data => new SimpleStringProperty(data.getValue.alpha))
    pxAlphaCol.setPrefWidth(60)

    pixelTable.getColumns.addAll(pxTriCol, pxColorCol, pxDepthCol, pxAlphaCol)
    VBox.setVgrow(pixelTable, Priority.ALWAYS)

    // Stats label
    val statsLabel = new Label("Stats: -")
    statsLabel.setId("statsLabel")
    statsLabel.setStyle("-fx-text-fill: #888888; -fx-font-size: 10px;")

    panel.getChildren.addAll(
      positionLabel,
      triangleLabel,
      triangleTable,
      pixelLabel,
      pixelTable,
      statsLabel
    )

    panel
  }

  /** Update the debug panel with data at the given position */
  private def updateDebugPanel(x: Int, y: Int): Unit = {
    debugTracker.foreach { tracker =>
      positionLabel.setText(s"Position: ($x, $y) - SELECTED")

      // Get pixels at this position
      val pixels = tracker.getPixelsAt(x, y)

      // Count pixels per triangle ID
      val pixelCountByTriangle = pixels.groupBy(_.triangleId).mapValues(_.size)

      // Get triangles that geometrically contain this point
      val containingTriangles = tracker.getTrianglesContaining(x, y)

      // Get triangles from pixel history (may not geometrically contain point anymore due to rounding)
      val pixelTriangleIds = pixels.map(_.triangleId).distinct.filter(_ >= 0)
      val pixelTriangles = pixelTriangleIds.flatMap(id => tracker.getTriangle(id))

      // Merge triangles (union by ID, containing triangles take precedence)
      val containingIds = containingTriangles.map(_.id).toSet
      val additionalFromPixels = pixelTriangles.filterNot(t => containingIds.contains(t.id))
      val allTriangles = containingTriangles ++ additionalFromPixels

      // Update triangle table with pixel counts
      val triRows = allTriangles.map { tri =>
        val bbox =
          s"(${tri.boundingBox._1},${tri.boundingBox._2})-(${tri.boundingBox._3},${tri.boundingBox._4})"
        val verts = tri.vertices.take(3).map { case (vx, vy) => f"($vx%.1f,$vy%.1f)" }.mkString(" ")
        val pxCount = pixelCountByTriangle.getOrElse(tri.id, 0)
        TriangleRow(tri.id, bbox, verts, pxCount)
      }
      triangleTable.setItems(FXCollections.observableArrayList(triRows.asJava))

      // Update pixel table (most recent first)
      val pxRows = pixels.reverse.map { px =>
        val color = f"${px.r}%.2f, ${px.g}%.2f, ${px.b}%.2f"
        PixelRow(px.triangleId, color, f"${px.depth}%.4f", f"${px.alpha}%.2f")
      }
      pixelTable.setItems(FXCollections.observableArrayList(pxRows.asJava))

      // Update stats
      val (triCount, pxCount) = tracker.getStats
      val statsLabel = positionLabel.getParent.getChildrenUnmodifiable.asScala
        .find(n => n.getId == "statsLabel")
        .map(_.asInstanceOf[Label])
      statsLabel.foreach(_.setText(s"Stats: $triCount triangles, $pxCount pixels tracked"))
    }
  }

  /** Row data for triangle table */
  case class TriangleRow(id: Long, bbox: String, vertices: String, pixels: Int)

  /** Row data for pixel table */
  case class PixelRow(triangleId: Long, color: String, depth: String, alpha: String)
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
  def apply(
      width: Int,
      height: Int,
      stride: Int,
      debugTracker: Option[DebugTracker] = None
  ): DisplayWindow = {
    val window = new DisplayWindow(width, height, stride, debugTracker)
    DisplayWindowApp.window = window
    window.launch()
    window
  }
}
