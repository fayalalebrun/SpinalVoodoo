package voodoo.trace

import javafx.application.{Application, Platform}
import javafx.beans.property.{
  SimpleDoubleProperty,
  SimpleIntegerProperty,
  SimpleLongProperty,
  SimpleStringProperty
}
import javafx.collections.FXCollections
import javafx.geometry.{Insets, Orientation, Pos}
import javafx.scene.Scene
import javafx.scene.canvas.Canvas
import javafx.scene.control.{
  Button,
  CheckBox,
  Label,
  ListView,
  Separator,
  SplitPane,
  Tab,
  TabPane,
  TableColumn,
  TableView,
  cell
}
import javafx.scene.image.{PixelWriter, WritableImage}
import javafx.scene.layout.{BorderPane, HBox, Pane, Priority, VBox}
import javafx.scene.paint.Color
import javafx.stage.Stage
import java.util.concurrent.{BlockingQueue, LinkedBlockingQueue}
import scala.collection.mutable
import scala.collection.JavaConverters._

/** Pipeline stage identifiers */
object PipelineStage extends Enumeration {
  val Framebuffer, TMU0, TMU1, ColorCombine = Value
}

/** A stage-specific framebuffer that accumulates pixel data */
class StageFramebuffer(val name: String, val width: Int, val height: Int) {
  private val image = new WritableImage(width, height)
  val pixelWriter: PixelWriter = image.getPixelWriter

  // Clear to black initially
  def clear(): Unit = {
    for (y <- 0 until height; x <- 0 until width) {
      pixelWriter.setColor(x, y, Color.BLACK)
    }
  }

  def getImage: WritableImage = image

  /** Write a pixel at the given coordinates (y=0 at bottom, OpenGL style) */
  def writePixel(x: Int, y: Int, r: Int, g: Int, b: Int): Unit = {
    if (x >= 0 && x < width && y >= 0 && y < height) {
      val flippedY = height - 1 - y
      pixelWriter.setColor(x, flippedY, Color.rgb(r & 0xff, g & 0xff, b & 0xff))
    }
  }

  clear()
}

/** Data for a single triangle's rasterization */
case class TriangleOverlayData(
    id: Long,
    edges: Array[(Double, Double, Double)],
    boundingBox: (Int, Int, Int, Int),
    vertices: Array[(Double, Double)] = Array.empty,
    pixelHits: mutable.Set[(Int, Int)] = mutable.Set.empty
) {
  def pixelCount: Int = pixelHits.size
}

/** Rasterizer overlay for visualizing edge equations and pixel hits */
class RasterizerOverlay(width: Int, height: Int) {
  // Store data for all triangles
  private val triangles = mutable.ArrayBuffer[TriangleOverlayData]()
  private var currentTriangle: Option[TriangleOverlayData] = None

  // Currently selected triangle for visualization
  private var selectedTriangleId: Long = -1

  /** Start a new triangle with edge equations, bounding box, and vertices */
  def startTriangle(
      triangleId: Long,
      coeffs: Array[(Double, Double, Double)],
      bbox: (Int, Int, Int, Int),
      verts: Array[(Double, Double)] = Array.empty
  ): Unit = {
    val tri = TriangleOverlayData(triangleId, coeffs, bbox, verts)
    triangles += tri
    currentTriangle = Some(tri)
    // Auto-select the latest triangle
    selectedTriangleId = triangleId
  }

  /** Record a pixel hit for the current triangle */
  def addPixelHit(x: Int, y: Int): Unit = {
    if (x >= 0 && x < width && y >= 0 && y < height) {
      currentTriangle.foreach(_.pixelHits += ((x, y)))
    }
  }

  /** Get all triangles */
  def getAllTriangles: Seq[TriangleOverlayData] = triangles.toSeq

  /** Get selected triangle */
  def getSelectedTriangle: Option[TriangleOverlayData] =
    triangles.find(_.id == selectedTriangleId)

  /** Select a triangle by ID */
  def selectTriangle(id: Long): Unit = {
    selectedTriangleId = id
  }

  /** Get selected triangle ID */
  def getSelectedId: Long = selectedTriangleId

  /** Clear all triangle data */
  def clearAll(): Unit = {
    triangles.clear()
    currentTriangle = None
    selectedTriangleId = -1
  }

  /** Keep only the last N triangles */
  def trimToLast(n: Int): Unit = {
    if (triangles.size > n) {
      val toRemove = triangles.size - n
      triangles.remove(0, toRemove)
    }
  }

  /** Get edge equations for selected triangle */
  def getEdges: Array[(Double, Double, Double)] = {
    getSelectedTriangle.map(_.edges).getOrElse(Array.empty)
  }

  /** Get bounding box for selected triangle */
  def getBoundingBox: (Int, Int, Int, Int) = {
    getSelectedTriangle.map(_.boundingBox).getOrElse((0, 0, 0, 0))
  }

  /** Check if a pixel was hit by the selected triangle */
  def getHit(x: Int, y: Int): Boolean = {
    getSelectedTriangle.exists(_.pixelHits.contains((x, y)))
  }

  /** Alias for setEdges with triangle ID (for compatibility) */
  def setEdges(
      triangleId: Long,
      coeffs: Array[(Double, Double, Double)],
      bbox: (Int, Int, Int, Int)
  ): Unit = {
    startTriangle(triangleId, coeffs, bbox)
  }

  /** Clear current triangle data (for single-triangle mode compatibility) */
  def clear(): Unit = {
    // In multi-triangle mode, we don't clear all - just prepare for next
    currentTriangle = None
  }
}

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

  // Rasterizer overlay panel components
  private var rasterizerTriangleList: ListView[String] = _
  private var showEdgesCheckbox: CheckBox = _
  private var overlayStatsLabel: Label = _

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

  // Rasterizer overlay for debug visualization
  val rasterizerOverlay = new RasterizerOverlay(width, height)

  // Pipeline stage framebuffers
  val rasterizerFramebuffer = new StageFramebuffer("Rasterizer", width, height)
  val tmu0Framebuffer = new StageFramebuffer("TMU0", width, height)
  val tmu1Framebuffer = new StageFramebuffer("TMU1", width, height)
  val colorCombineFramebuffer = new StageFramebuffer("ColorCombine", width, height)

  // Tab pane and per-tab canvases
  private var tabPane: TabPane = _
  private var fbTab: Tab = _
  private var rasterizerTab: Tab = _
  private var tmu0Tab: Tab = _
  private var tmu1Tab: Tab = _
  private var ccTab: Tab = _
  private var rasterizerCanvas: Canvas = _
  private var tmu0Canvas: Canvas = _
  private var tmu1Canvas: Canvas = _
  private var ccCanvas: Canvas = _
  private var rasterizerCanvasPane: Pane = _
  private var tmu0CanvasPane: Pane = _
  private var tmu1CanvasPane: Pane = _
  private var ccCanvasPane: Pane = _

  // Overlay visibility flags (controlled by ControlPanelWindow)
  @volatile var showEdgeLines = false

  // Flag to enable/disable overlay data collection (enabled by default)
  @volatile var overlayTrackingEnabled = true

  // Play/pause control
  @volatile var isPaused = true // Start paused by default
  @volatile var stepRequested = false // Single-step mode
  private var playPauseButton: Button = _
  private var stepButton: Button = _

  // Register state tracking
  private val registerState = mutable.Map[Long, Long]()
  private var registerTable: TableView[RegisterRow] = _
  private var registersTab: Tab = _

  // Trace viewer
  private var traceTab: Tab = _
  private var traceListView: ListView[String] = _

  // Windowed trace view - store raw data, format on demand
  case class TraceEntryData(index: Int, cmdType: Int, addr: Long, data: Long, count: Int)
  private val traceEntries = mutable.ArrayBuffer[TraceEntryData]()
  private val windowSize = 200 // Show 200 entries at a time
  private var windowStart = 0 // Current window start index in traceEntries

  @volatile private var currentTraceIndex = 0
  private var tracePositionLabel: Label = _

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

    // Create pipeline stage canvases
    rasterizerCanvas = new Canvas(width, height)
    rasterizerCanvasPane = new Pane()
    rasterizerCanvasPane.getChildren.add(rasterizerCanvas)
    rasterizerCanvasPane.setStyle("-fx-background-color: #222222;")

    tmu0Canvas = new Canvas(width, height)
    tmu0CanvasPane = new Pane()
    tmu0CanvasPane.getChildren.add(tmu0Canvas)
    tmu0CanvasPane.setStyle("-fx-background-color: #222222;")

    tmu1Canvas = new Canvas(width, height)
    tmu1CanvasPane = new Pane()
    tmu1CanvasPane.getChildren.add(tmu1Canvas)
    tmu1CanvasPane.setStyle("-fx-background-color: #222222;")

    ccCanvas = new Canvas(width, height)
    ccCanvasPane = new Pane()
    ccCanvasPane.getChildren.add(ccCanvas)
    ccCanvasPane.setStyle("-fx-background-color: #222222;")

    // Create tabbed interface for different views
    tabPane = new TabPane()
    tabPane.setTabClosingPolicy(TabPane.TabClosingPolicy.UNAVAILABLE)

    fbTab = new Tab("Framebuffer", canvasPane)
    rasterizerTab = new Tab("Rasterizer", rasterizerCanvasPane)
    tmu0Tab = new Tab("TMU0", tmu0CanvasPane)
    tmu1Tab = new Tab("TMU1", tmu1CanvasPane)
    ccTab = new Tab("ColorCombine", ccCanvasPane)

    // Create registers tab
    registersTab = new Tab("Registers", createRegistersPanel())

    // Create trace tab
    traceTab = new Tab("Trace", createTracePanel())

    tabPane.getTabs.addAll(fbTab, rasterizerTab, tmu0Tab, tmu1Tab, ccTab, registersTab, traceTab)

    // Setup mouse handlers for all canvas panes
    setupCanvasPaneHandlers(canvasPane)
    setupCanvasPaneHandlers(rasterizerCanvasPane)
    setupCanvasPaneHandlers(tmu0CanvasPane)
    setupCanvasPaneHandlers(tmu1CanvasPane)
    setupCanvasPaneHandlers(ccCanvasPane)

    // Create toolbar with play/pause button
    val toolbar = createToolbar()

    // Create root layout - BorderPane with toolbar at top
    val mainContent = debugTracker match {
      case Some(_) =>
        val splitPane = new SplitPane()
        splitPane.setOrientation(Orientation.HORIZONTAL)
        splitPane.getItems.addAll(tabPane, createDebugPanel())
        splitPane.setDividerPositions(0.7)
        splitPane
      case None =>
        tabPane
    }

    val root = new BorderPane()
    root.setTop(toolbar)
    root.setCenter(mainContent)

    val sceneWidth = if (debugTracker.isDefined) width + 350 else width
    val scene = new Scene(root, sceneWidth, height + 40)

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

  /** Setup mouse handlers for a canvas pane (zoom, pan, click) */
  private def setupCanvasPaneHandlers(pane: Pane): Unit = {
    // Zoom with scroll wheel
    pane.setOnScroll(event => {
      val scrollDelta = event.getDeltaY
      if (scrollDelta != 0) {
        val oldZoom = zoomLevel
        val zoomFactor = Math.pow(1.01, scrollDelta)
        zoomLevel = Math.max(0.5, Math.min(6.0, zoomLevel * zoomFactor))

        if (zoomLevel != oldZoom) {
          val mouseX = event.getX
          val mouseY = event.getY
          val imgX = (mouseX - panX) / oldZoom
          val imgY = (mouseY - panY) / oldZoom
          panX = mouseX - imgX * zoomLevel
          panY = mouseY - imgY * zoomLevel
          needsRedraw = true
        }
        event.consume()
      }
    })

    // Pan with mouse drag
    pane.setOnMousePressed(event => {
      dragStartX = event.getX
      dragStartY = event.getY
      dragStartPanX = panX
      dragStartPanY = panY
      wasDragged = false
    })

    pane.setOnMouseDragged(event => {
      val dx = event.getX - dragStartX
      val dy = event.getY - dragStartY
      if (Math.abs(dx) > 3 || Math.abs(dy) > 3) {
        wasDragged = true
        panX = dragStartPanX + dx
        panY = dragStartPanY + dy
        needsRedraw = true
      }
    })

    // Handle clicks: double-click reset, single-click inspect
    pane.setOnMouseClicked(event => {
      if (event.getClickCount == 2) {
        zoomLevel = 1.0
        panX = 0.0
        panY = 0.0
        needsRedraw = true
      } else if (event.getClickCount == 1 && !wasDragged && debugTracker.isDefined) {
        val mouseX = event.getX
        val mouseY = event.getY
        val fbX = ((mouseX - panX) / zoomLevel).toInt
        val fbY = height - 1 - ((mouseY - panY) / zoomLevel).toInt
        if (fbX >= 0 && fbX < width && fbY >= 0 && fbY < height) {
          updateDebugPanel(fbX, fbY)
        }
      }
    })

    // Update position on mouse move
    pane.setOnMouseMoved(event => {
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
  }

  /** Redraw a single canvas with an image */
  private def redrawSingleCanvas(
      targetCanvas: Canvas,
      targetImage: WritableImage,
      withOverlays: Boolean = false
  ): Unit = {
    if (targetCanvas == null || targetImage == null) return
    val gc = targetCanvas.getGraphicsContext2D
    if (gc == null) return

    val maxTextureSize = 4096.0
    val scaledWidth = Math.min(width * zoomLevel, maxTextureSize)
    val scaledHeight = Math.min(height * zoomLevel, maxTextureSize)
    targetCanvas.setWidth(scaledWidth)
    targetCanvas.setHeight(scaledHeight)
    targetCanvas.setLayoutX(panX)
    targetCanvas.setLayoutY(panY)

    gc.setImageSmoothing(false)
    gc.clearRect(0, 0, scaledWidth, scaledHeight)
    gc.drawImage(targetImage, 0, 0, scaledWidth, scaledHeight)

    if (withOverlays) {
      if (showEdgeLines) drawEdgeLines(gc)
    }
  }

  /** Redraw all canvases with current zoom and pan */
  private def redrawCanvas(): Unit = {
    // Redraw main framebuffer with overlays
    redrawSingleCanvas(canvas, image, withOverlays = true)

    // Redraw pipeline stage canvases
    redrawSingleCanvas(rasterizerCanvas, rasterizerFramebuffer.getImage)
    redrawSingleCanvas(tmu0Canvas, tmu0Framebuffer.getImage)
    redrawSingleCanvas(tmu1Canvas, tmu1Framebuffer.getImage)
    redrawSingleCanvas(ccCanvas, colorCombineFramebuffer.getImage)
  }

  /** Draw edge equation lines for the current triangle */
  private def drawEdgeLines(gc: javafx.scene.canvas.GraphicsContext): Unit = {
    rasterizerOverlay.getSelectedTriangle match {
      case None      => return
      case Some(tri) =>
        val edges = tri.edges
        if (edges.isEmpty) return

        val bbox = tri.boundingBox
        val vertices = tri.vertices
        val colors = Array(Color.RED, Color.LIME, Color.BLUE)

        // Use extended range for line drawing (50 pixel margin beyond bbox)
        val margin = 50.0
        val xMin = bbox._1.toDouble - margin
        val xMax = bbox._3.toDouble + margin
        val yMin = bbox._2.toDouble - margin
        val yMax = bbox._4.toDouble + margin

        gc.setLineWidth(2.0)

        for (i <- edges.indices) {
          val (a, b, c) = edges(i)
          gc.setStroke(colors(i % colors.length))

          // Draw line ax + by + c = 0
          // Compute line-bbox intersection using Liang-Barsky style clipping
          // Try both x-based and y-based parameterization, pick the one that works

          val points = scala.collection.mutable.ArrayBuffer[(Double, Double)]()

          // Intersection with x = xMin
          if (Math.abs(b) > 0.0001) {
            val y = -(a * xMin + c) / b
            if (y >= yMin && y <= yMax) points += ((xMin, y))
          }

          // Intersection with x = xMax
          if (Math.abs(b) > 0.0001) {
            val y = -(a * xMax + c) / b
            if (y >= yMin && y <= yMax) points += ((xMax, y))
          }

          // Intersection with y = yMin
          if (Math.abs(a) > 0.0001) {
            val x = -(b * yMin + c) / a
            if (x >= xMin && x <= xMax) points += ((x, yMin))
          }

          // Intersection with y = yMax
          if (Math.abs(a) > 0.0001) {
            val x = -(b * yMax + c) / a
            if (x >= xMin && x <= xMax) points += ((x, yMax))
          }

          // Draw line between the two intersection points (if we found 2)
          if (points.size >= 2) {
            val (x1, y1) = points(0)
            val (x2, y2) = points(1)
            val screenX1 = x1 * zoomLevel
            val screenY1 = (height - 1 - y1) * zoomLevel
            val screenX2 = x2 * zoomLevel
            val screenY2 = (height - 1 - y2) * zoomLevel
            gc.strokeLine(screenX1, screenY1, screenX2, screenY2)
          }
        }

        // Draw vertex markers (if we have vertices)
        if (vertices.length >= 3) {
          gc.setFill(Color.WHITE)
          gc.setStroke(Color.BLACK)
          gc.setLineWidth(1.0)
          for (i <- 0 until 3) {
            val (vx, vy) = vertices(i)
            val screenX = vx * zoomLevel
            val screenY = (height - 1 - vy) * zoomLevel
            val markerSize = 6.0
            gc.fillOval(screenX - markerSize / 2, screenY - markerSize / 2, markerSize, markerSize)
            gc.strokeOval(
              screenX - markerSize / 2,
              screenY - markerSize / 2,
              markerSize,
              markerSize
            )

            // Label the vertex
            gc.setFill(colors(i))
            gc.fillText(s"V$i", screenX + 5, screenY - 5)
          }
        }

        // Draw bounding box outline
        gc.setStroke(Color.YELLOW)
        gc.setLineWidth(1.0)
        gc.setLineDashes(4.0, 4.0)
        val bboxX = bbox._1 * zoomLevel
        val bboxY = (height - 1 - bbox._4) * zoomLevel
        val bboxW = (bbox._3 - bbox._1 + 1) * zoomLevel
        val bboxH = (bbox._4 - bbox._2 + 1) * zoomLevel
        gc.strokeRect(bboxX, bboxY, bboxW, bboxH)
        gc.setLineDashes() // Reset to solid
    }
  }

  /** Trigger a redraw (thread-safe) */
  def requestRedraw(): Unit = {
    needsRedraw = true
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

  /** Create the toolbar with play/pause button and status */
  private def createToolbar(): HBox = {
    val toolbar = new HBox(10)
    toolbar.setPadding(new Insets(5, 10, 5, 10))
    toolbar.setAlignment(Pos.CENTER_LEFT)
    toolbar.setStyle("-fx-background-color: #444444;")

    // Play/Pause button - starts showing Play since we start paused
    playPauseButton = new Button("▶ Play")
    playPauseButton.setStyle(
      "-fx-font-size: 14px; -fx-min-width: 100px; -fx-background-color: #4CAF50;"
    )
    playPauseButton.setOnAction(_ => {
      isPaused = !isPaused
      updatePlayPauseButton()
    })

    // Step button
    stepButton = new Button("⏭ Step")
    stepButton.setStyle("-fx-font-size: 14px; -fx-min-width: 80px;")
    stepButton.setOnAction(_ => {
      stepRequested = true
    })

    // Status label
    val statusLabel = new Label("Paused")
    statusLabel.setId("playbackStatusLabel")
    statusLabel.setStyle("-fx-text-fill: #ffaa00; -fx-font-size: 12px;")

    toolbar.getChildren.addAll(playPauseButton, stepButton, statusLabel)
    toolbar
  }

  /** Update play/pause button text based on current state */
  private def updatePlayPauseButton(): Unit = {
    if (playPauseButton != null) {
      Platform.runLater(() => {
        if (isPaused) {
          playPauseButton.setText("▶ Play")
          playPauseButton.setStyle(
            "-fx-font-size: 14px; -fx-min-width: 100px; -fx-background-color: #4CAF50;"
          )
        } else {
          playPauseButton.setText("⏸ Pause")
          playPauseButton.setStyle("-fx-font-size: 14px; -fx-min-width: 100px;")
        }
      })
    }
  }

  /** Create the registers panel with TableView */
  private def createRegistersPanel(): VBox = {
    val panel = new VBox(10)
    panel.setPadding(new Insets(10))
    panel.setStyle("-fx-background-color: #333333;")

    val headerLabel = new Label("Register State")
    headerLabel.setStyle("-fx-text-fill: white; -fx-font-size: 14px; -fx-font-weight: bold;")

    registerTable = new TableView[RegisterRow]()
    registerTable.setStyle("-fx-background-color: #444444;")
    registerTable.setColumnResizePolicy(TableView.CONSTRAINED_RESIZE_POLICY)

    val addrCol = new TableColumn[RegisterRow, String]("Address")
    addrCol.setCellValueFactory(data => new SimpleStringProperty(data.getValue.addrHex))
    addrCol.setPrefWidth(80)

    val nameCol = new TableColumn[RegisterRow, String]("Name")
    nameCol.setCellValueFactory(data => new SimpleStringProperty(data.getValue.name))
    nameCol.setPrefWidth(150)

    val valueCol = new TableColumn[RegisterRow, String]("Value")
    valueCol.setCellValueFactory(data => new SimpleStringProperty(data.getValue.valueHex))
    valueCol.setPrefWidth(120)

    val decodedCol = new TableColumn[RegisterRow, String]("Decoded")
    decodedCol.setCellValueFactory(data => new SimpleStringProperty(data.getValue.decoded))
    decodedCol.setPrefWidth(200)

    registerTable.getColumns.addAll(addrCol, nameCol, valueCol, decodedCol)
    VBox.setVgrow(registerTable, Priority.ALWAYS)

    panel.getChildren.addAll(headerLabel, registerTable)
    panel
  }

  /** Update a register value (called from simulation thread) */
  def updateRegister(addr: Long, value: Long): Unit = {
    registerState(addr) = value
    // Refresh the table periodically (handled in update loop)
  }

  /** Refresh the register table display (call from JavaFX thread) */
  def refreshRegisterTable(): Unit = {
    Platform.runLater(() => {
      if (registerTable != null) {
        val rows = registerState.toSeq.sortBy(_._1).map { case (addr, value) =>
          val name = RegisterNames.getName(addr)
          val decoded = RegisterNames.decode(addr, value)
          val addrStr = if (addr >= 0x200000) f"0x${addr}%06X" else f"0x${addr}%03X"
          RegisterRow(addrStr, name, f"0x${value}%08X", decoded)
        }
        registerTable.setItems(FXCollections.observableArrayList(rows.asJava))
      }
    })
  }

  /** Row data for register table */
  case class RegisterRow(addrHex: String, name: String, valueHex: String, decoded: String)

  /** Create the trace panel with ListView */
  private def createTracePanel(): VBox = {
    val panel = new VBox(10)
    panel.setPadding(new Insets(10))
    panel.setStyle("-fx-background-color: #333333;")

    val headerLabel = new Label("Trace Entries")
    headerLabel.setStyle("-fx-text-fill: white; -fx-font-size: 14px; -fx-font-weight: bold;")

    tracePositionLabel = new Label("Position: 0 / 0")
    tracePositionLabel.setStyle("-fx-text-fill: #aaaaaa; -fx-font-size: 12px;")

    traceListView = new ListView[String]()
    traceListView.setStyle(
      "-fx-background-color: #444444; -fx-control-inner-background: #222222; -fx-font-family: monospace;"
    )
    VBox.setVgrow(traceListView, Priority.ALWAYS)

    // Custom cell factory to highlight current entry
    // Note: getIndex returns position within the window, convert to absolute trace index
    traceListView.setCellFactory(_ =>
      new javafx.scene.control.ListCell[String]() {
        override def updateItem(item: String, empty: Boolean): Unit = {
          super.updateItem(item, empty)
          if (empty || item == null) {
            setText(null)
            setStyle("-fx-background-color: transparent;")
          } else {
            setText(item)
            // Convert window index to absolute trace index
            val actualIdx = windowStart + getIndex
            if (actualIdx == currentTraceIndex) {
              setStyle(
                "-fx-background-color: #4a6fa5; -fx-text-fill: white; -fx-font-family: monospace;"
              )
            } else if (actualIdx < currentTraceIndex) {
              setStyle(
                "-fx-background-color: #2a2a2a; -fx-text-fill: #888888; -fx-font-family: monospace;"
              )
            } else {
              setStyle(
                "-fx-background-color: transparent; -fx-text-fill: #cccccc; -fx-font-family: monospace;"
              )
            }
          }
        }
      }
    )

    panel.getChildren.addAll(headerLabel, tracePositionLabel, traceListView)
    panel
  }

  /** Format a TraceEntryData to a display string */
  private def formatEntry(entry: TraceEntryData): String = {
    val cmdName = entry.cmdType match {
      case 0  => "WR_REG_L"
      case 1  => "WR_REG_W"
      case 2  => "WR_FB_L"
      case 3  => "WR_FB_W"
      case 4  => "WR_TEX_L"
      case 5  => "WR_CMDFIFO"
      case 6  => "RD_REG_L"
      case 7  => "RD_REG_W"
      case 8  => "RD_FB_L"
      case 9  => "RD_FB_W"
      case 10 => "VSYNC"
      case 11 => "SWAP"
      case 12 => "CONFIG"
      case _  => f"CMD_${entry.cmdType}%02X"
    }

    val regName = if (entry.cmdType == 0 || entry.cmdType == 1) {
      val maskedAddr = entry.addr & 0xfff
      " " + RegisterNames.getName(maskedAddr)
    } else ""

    f"${entry.index}%6d: $cmdName%-12s 0x${entry.addr}%06X = 0x${entry.data}%08X${
        if (entry.count > 1) s" x${entry.count}" else ""
      }$regName"
  }

  /** Add a trace entry (called during trace loading) */
  def addTraceEntry(index: Int, cmdType: Int, addr: Long, data: Long, count: Int): Unit = {
    traceEntries += TraceEntryData(index, cmdType, addr, data, count)
  }

  /** Finalize trace loading (call after all entries added) */
  def finalizeTraceLoading(): Unit = {
    updateTraceWindow(force = true)
  }

  /** Update the windowed trace view - only shows ~200 entries centered on current position */
  private def updateTraceWindow(force: Boolean = false): Unit = {
    // Calculate window centered on current position
    val newStart = Math.max(0, currentTraceIndex - windowSize / 2)
    val newEnd = Math.min(traceEntries.size, newStart + windowSize)

    // Only update if window actually changed or forced
    if (force || newStart != windowStart) {
      windowStart = newStart
      val windowEntries = traceEntries.slice(newStart, newEnd).map(formatEntry)
      Platform.runLater(() => {
        if (traceListView != null) {
          traceListView.setItems(FXCollections.observableArrayList(windowEntries.asJava))
        }
        if (tracePositionLabel != null) {
          tracePositionLabel.setText(
            f"Position: $currentTraceIndex%,d / ${traceEntries.size}%,d [window: $newStart%,d-${newEnd - 1}%,d]"
          )
        }
      })
    }
  }

  /** Update the current trace position (called during replay) */
  def updateTracePosition(index: Int, forceUpdate: Boolean = false): Unit = {
    currentTraceIndex = index
    // Update UI periodically (every 50 entries to avoid overhead), or always if forced
    if (forceUpdate || index % 50 == 0) {
      updateTraceWindow()
      Platform.runLater(() => {
        if (traceListView != null) {
          // Scroll to keep current entry visible within window
          val relativeIndex = currentTraceIndex - windowStart
          if (relativeIndex >= 0 && relativeIndex < windowSize) {
            traceListView.scrollTo(Math.max(0, relativeIndex - 5))
          }
          traceListView.refresh()
        }
      })
    }
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

    // Rasterizer overlay section
    val overlaySection = createOverlaySection()

    panel.getChildren.addAll(
      positionLabel,
      triangleLabel,
      triangleTable,
      pixelLabel,
      pixelTable,
      statsLabel,
      overlaySection
    )

    panel
  }

  /** Create the rasterizer overlay section with toggles and triangle list */
  private def createOverlaySection(): VBox = {
    val section = new VBox(8)
    section.setPadding(new Insets(10, 0, 0, 0))

    // Section header with separator
    val separator = new Separator()
    separator.setStyle("-fx-background-color: #666666;")

    val headerLabel = new Label("Rasterizer Debug Overlay")
    headerLabel.setStyle("-fx-text-fill: white; -fx-font-size: 14px; -fx-font-weight: bold;")

    // Overlay toggles
    showEdgesCheckbox = new CheckBox("Show Edge Lines")
    showEdgesCheckbox.setStyle("-fx-text-fill: #cccccc;")
    showEdgesCheckbox.setSelected(showEdgeLines)
    showEdgesCheckbox
      .selectedProperty()
      .addListener((_, _, newValue) => {
        showEdgeLines = newValue
        needsRedraw = true
      })

    // Triangle list label
    val triListLabel = new Label("Triangles (select to view overlay):")
    triListLabel.setStyle("-fx-text-fill: #aaaaaa; -fx-font-size: 12px;")

    // Triangle list view
    rasterizerTriangleList = new ListView[String]()
    rasterizerTriangleList.setStyle(
      "-fx-background-color: #444444; -fx-control-inner-background: #444444;"
    )
    rasterizerTriangleList.setPrefHeight(120)

    // Selection listener
    rasterizerTriangleList.getSelectionModel
      .selectedItemProperty()
      .addListener((_, _, newValue) => {
        if (newValue != null) {
          // Extract triangle ID from the list item string (format: "Tri #123: 45 pixels")
          val idMatch = "Tri #(\\d+)".r.findFirstMatchIn(newValue)
          idMatch.foreach { m =>
            val id = m.group(1).toLong
            rasterizerOverlay.selectTriangle(id)
            needsRedraw = true
          }
        }
      })

    // Overlay stats
    overlayStatsLabel = new Label("Overlay: 0 triangles")
    overlayStatsLabel.setStyle("-fx-text-fill: #888888; -fx-font-size: 10px;")

    section.getChildren.addAll(
      separator,
      headerLabel,
      showEdgesCheckbox,
      triListLabel,
      rasterizerTriangleList,
      overlayStatsLabel
    )

    section
  }

  /** Update the rasterizer overlay triangle list (call from simulation thread via
    * Platform.runLater)
    */
  def updateOverlayTriangleList(): Unit = {
    Platform.runLater(() => {
      if (rasterizerTriangleList != null) {
        val triangles = rasterizerOverlay.getAllTriangles
        val items = triangles.map { tri =>
          val bbox = tri.boundingBox
          val vertsStr = if (tri.vertices.nonEmpty) {
            tri.vertices.map { case (x, y) => f"(${x}%.1f,${y}%.1f)" }.mkString(" ")
          } else {
            "(no verts)"
          }
          f"Tri #${tri.id}%d: ${tri.pixelCount}%d px | $vertsStr"
        }
        rasterizerTriangleList.setItems(FXCollections.observableArrayList(items.asJava))

        // Update stats
        if (overlayStatsLabel != null) {
          val totalPixels = triangles.map(_.pixelCount).sum
          overlayStatsLabel.setText(s"Overlay: ${triangles.size} triangles, $totalPixels pixels")
        }

        // Auto-scroll to bottom (most recent triangle)
        if (items.nonEmpty) {
          rasterizerTriangleList.scrollTo(items.size - 1)
          // Select the most recent if nothing is selected
          if (rasterizerTriangleList.getSelectionModel.getSelectedItem == null) {
            rasterizerTriangleList.getSelectionModel.selectLast()
          }
        }
      }
    })
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

/** Voodoo register names and decoding - loaded from RegIf interface */
object RegisterNames {
  import scala.collection.mutable

  private val registerMap = mutable.Map[Long, String]()

  /** Load register names from RegisterBank's RegIf interface */
  def loadFromRegBank(regBank: voodoo.RegisterBank): Unit = {
    registerMap.clear()
    regBank.busif.slices.foreach { slice =>
      // Use getDoc() which contains the register name passed to newRegAt
      // (the second parameter is doc/name, not the implicit SymbolName)
      registerMap(slice.getAddr().toLong) = slice.getDoc()
    }
    println(s"[RegisterNames] Loaded ${registerMap.size} registers from RegIf")
  }

  def getName(addr: Long): String = registerMap.getOrElse(
    addr,
    if (addr >= 0x200000) f"reg_0x${addr}%06X" else f"reg_0x${addr}%03X"
  )

  private val depthFuncNames = Seq("never", "<", "==", "<=", ">", "!=", ">=", "always")
  private val blendFuncNames = Seq(
    "zero",
    "srcA",
    "srcColor",
    "dstA",
    "one",
    "1-srcA",
    "1-srcColor",
    "1-dstA",
    "dstColor",
    "1-dstColor",
    "srcA_sat",
    "x",
    "x",
    "x",
    "x",
    "x"
  )

  def decode(addr: Long, value: Long): String = {
    addr match {
      case 0x000L => // status
        val pciFifoFree = value & 0x3f
        val vRetrace = (value >> 6) & 0x1
        val fbiBusy = (value >> 7) & 0x1
        val trexBusy = (value >> 8) & 0x1
        val sstBusy = (value >> 9) & 0x1
        val dispBuf = (value >> 10) & 0x3
        val memFifoFree = (value >> 12) & 0xffff
        val swapsPend = (value >> 28) & 0x7
        f"pciFifo:$pciFifoFree,vRet:$vRetrace,busy:fbi=$fbiBusy/trex=$trexBusy/sst=$sstBusy,buf:$dispBuf,swaps:$swapsPend"

      case 0x110L => // fbzMode (detailed field decode from RegisterBank)
        val enableClipping = (value >> 0) & 0x1
        val enableChromaKey = (value >> 1) & 0x1
        val enableStipple = (value >> 2) & 0x1
        val wBufferSelect = (value >> 3) & 0x1
        val enableDepthBuffer = (value >> 4) & 0x1
        val depthFunction = (value >> 5) & 0x7
        val enableDithering = (value >> 8) & 0x1
        val rgbBufferMask = (value >> 9) & 0x1
        val auxBufferMask = (value >> 10) & 0x1
        val ditherAlgo = (value >> 11) & 0x1
        val enableStipplePattern = (value >> 12) & 0x1
        val enableAlphaMask = (value >> 13) & 0x1
        val drawBuffer = (value >> 14) & 0x3
        val enableDepthBias = (value >> 16) & 0x1
        val yOrigin = (value >> 17) & 0x1
        val enableAlphaPlanes = (value >> 18) & 0x1
        val depthSource = (value >> 20) & 0x1
        f"clip:$enableClipping,chroma:$enableChromaKey,depth:$enableDepthBuffer(${depthFuncNames(depthFunction.toInt)}),dither:$enableDithering,rgbMask:$rgbBufferMask,auxMask:$auxBufferMask,drawBuf:$drawBuffer,yOrig:${
            if (yOrigin == 1) "bot" else "top"
          }"

      case 0x114L => // lfbMode
        val writeFormat = value & 0xf
        val writeBufSel = (value >> 4) & 0x3
        val readBufSel = (value >> 6) & 0x3
        val pixPipeEn = (value >> 8) & 0x1
        val rgbaLanes = (value >> 9) & 0x3
        val wordSwapW = (value >> 11) & 0x1
        val byteSwizzW = (value >> 12) & 0x1
        val yOrigin = (value >> 13) & 0x1
        val fmtNames = Seq(
          "565",
          "555",
          "1555",
          "x888",
          "888",
          "8888",
          "565_d",
          "555_d",
          "1555_d",
          "x888_d",
          "888_d",
          "8888_d",
          "Z",
          "x",
          "x",
          "x"
        )
        val bufNames = Seq("front", "back", "aux", "?")
        f"fmt:${fmtNames(writeFormat.toInt)},wBuf:${bufNames(writeBufSel.toInt)},rBuf:${bufNames(readBufSel.toInt)},pipe:$pixPipeEn,yOrig:${
            if (yOrigin == 1) "bot" else "top"
          }"

      case 0x118L => // clipLeftRight
        val clipLeft = value & 0x3ff
        val clipRight = (value >> 16) & 0x3ff
        f"left:$clipLeft,right:$clipRight"

      case 0x11cL => // clipLowYHighY
        val clipLow = value & 0x3ff
        val clipHigh = (value >> 16) & 0x3ff
        f"top:$clipLow,bottom:$clipHigh"

      case 0x104L => // fbzColorPath
        val rgbSelA = value & 0x3
        val aSelA = (value >> 2) & 0x3
        val ccLocalSel = (value >> 4) & 0x1
        val cczLocalSel = (value >> 5) & 0x3
        val ccASelMux = (value >> 7) & 0x7
        val ccARGBInvert = (value >> 10) & 0xf
        val ccAShift = (value >> 14) & 0x3
        val ccAInvert = (value >> 16) & 0x1
        val ccAClamp = (value >> 17) & 0x1
        val ccASubClocal = (value >> 18) & 0x1
        val ccRGBAdd = (value >> 19) & 0x3
        val ccAAdd = (value >> 21) & 0x3
        val textureEn = (value >> 27) & 0x1
        val rgbSelNames = Seq("iter", "tex", "color1", "lfb")
        f"rgbA:${rgbSelNames(rgbSelA.toInt)},aA:$aSelA,texEn:$textureEn,ccLocal:$ccLocalSel"

      case 0x108L => // fogMode
        val fogEnable = value & 0x1
        val fogAdd = (value >> 1) & 0x1
        val fogMult = (value >> 2) & 0x1
        val fogZa = (value >> 3) & 0x1
        val fogConst = (value >> 4) & 0x1
        val fogDither = (value >> 5) & 0x1
        val fogZones = (value >> 6) & 0x1
        f"en:$fogEnable,add:$fogAdd,mult:$fogMult,za:$fogZa,const:$fogConst,dither:$fogDither"

      case 0x10cL => // alphaMode
        val alphaFunc = value & 0x7
        val alphaRef = (value >> 8) & 0xff
        val srcRGBFactor = (value >> 8) & 0xf // overlaps with alphaRef for blend
        val dstRGBFactor = (value >> 12) & 0xf
        val srcAFactor = (value >> 16) & 0xf
        val dstAFactor = (value >> 20) & 0xf
        val alphaTest = (value >> 1) & 0x1
        val alphaBlend = (value >> 4) & 0x1
        f"test:${depthFuncNames(alphaFunc.toInt)},ref:$alphaRef,srcRGB:${blendFuncNames(srcRGBFactor.toInt)},dstRGB:${blendFuncNames(dstRGBFactor.toInt)}"

      case 0x20cL => // videoDimensions
        val hDisp = value & 0xfff
        val vDisp = (value >> 16) & 0xfff
        f"${hDisp + 1}x$vDisp"

      case 0x210L => // fbiInit0
        val vgaPassthrough = value & 0x1
        val graphicsReset = (value >> 1) & 0x1
        f"vgaPass:$vgaPassthrough,gfxReset:$graphicsReset"

      case 0x214L => // fbiInit1
        val pciWriteWait = (value >> 1) & 0x1
        val multiSst = (value >> 2) & 0x1
        val videoReset = (value >> 8) & 0x1
        val sliEnable = (value >> 23) & 0x1
        f"pciWait:$pciWriteWait,multiSST:$multiSst,vidReset:$videoReset,sli:$sliEnable"

      case 0x218L => // fbiInit2
        val swapAlgo = (value >> 9) & 0x3
        val bufOffset = (value >> 11) & 0x3ff
        val algoNames = Seq("vsync", "data", "stall", "sli")
        f"swap:${algoNames(swapAlgo.toInt)},bufOffset:${bufOffset * 4}KB"

      case 0x21cL => // fbiInit3
        val remapEn = value & 0x1
        f"remap:$remapEn"

      case 0x220L => // hSync
        val hSyncOn = value & 0xff
        val hSyncOff = (value >> 16) & 0x3ff
        f"on:$hSyncOn,off:$hSyncOff"

      case 0x224L => // vSync
        val vSyncOn = value & 0xffff
        val vSyncOff = (value >> 16) & 0xffff
        f"on:$vSyncOn,off:$vSyncOff"

      case 0x300L | 0x340L => // textureMode0/1
        val format = value & 0xf
        val reverseS = (value >> 4) & 0x1
        val reverseT = (value >> 5) & 0x1
        val clampS = (value >> 6) & 0x1
        val clampT = (value >> 7) & 0x1
        val filterMode = (value >> 8) & 0x3
        val tcGen = (value >> 12) & 0x1
        val perspCorr = (value >> 31) & 0x1
        val fmtNames = Seq(
          "8idx",
          "YIQ",
          "A8",
          "I8",
          "AI44",
          "P8",
          "ARGB8332",
          "A8I8",
          "ARGB4444",
          "ARGB1555",
          "ARGB565",
          "x",
          "x",
          "x",
          "x",
          "x"
        )
        val filtNames = Seq("point", "bilin", "x", "trilin")
        f"fmt:${fmtNames(format.toInt)},filt:${filtNames(filterMode.toInt)},clampS:$clampS,clampT:$clampT,persp:$perspCorr"

      case 0x304L | 0x344L => // tLOD0/1
        val lodMin = value & 0x3f
        val lodMax = (value >> 6) & 0x3f
        val lodBias = ((value >> 12) & 0x3f).toInt
        val biasSign = if (lodBias >= 32) lodBias - 64 else lodBias
        val split = (value >> 18) & 0x1
        val odd = (value >> 19) & 0x1
        val smode = (value >> 20) & 0x3
        f"min:$lodMin,max:$lodMax,bias:$biasSign,split:$split"

      case 0x30cL | 0x34cL => // texBaseAddr0/1
        val baseAddr = value & 0xffffff
        f"addr:0x${baseAddr}%06X (${baseAddr * 8} bytes)"

      case 0x12cL => // fogColor
        val b = value & 0xff
        val g = (value >> 8) & 0xff
        val r = (value >> 16) & 0xff
        f"RGB($r,$g,$b)"

      case 0x144L | 0x148L => // color0/1
        val b = value & 0xff
        val g = (value >> 8) & 0xff
        val r = (value >> 16) & 0xff
        val a = (value >> 24) & 0xff
        f"ARGB($a,$r,$g,$b)"

      case _ => ""
    }
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
