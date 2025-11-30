package voodoo.trace

import java.util.concurrent.{ConcurrentHashMap, ConcurrentLinkedDeque, ConcurrentLinkedQueue}
import java.util.concurrent.atomic.AtomicLong
import scala.collection.mutable
import scala.collection.JavaConverters._

/** Gradient values for a triangle (start values and derivatives) */
case class TriangleGradients(
    startR: Double,
    startG: Double,
    startB: Double,
    startZ: Double,
    startA: Double,
    startW: Double,
    dRdX: Double,
    dGdX: Double,
    dBdX: Double,
    dZdX: Double,
    dAdX: Double,
    dWdX: Double,
    dRdY: Double,
    dGdY: Double,
    dBdY: Double,
    dZdY: Double,
    dAdY: Double,
    dWdY: Double
)

/** Tracked triangle with all geometry and gradient data */
case class TrackedTriangle(
    id: Long,
    var vertices: Array[(Double, Double)], // 3 vertex coords
    var boundingBox: (Int, Int, Int, Int), // xmin, ymin, xmax, ymax
    var coefficients: Array[(Double, Double, Double)], // 3 edges: (a, b, c)
    signBit: Boolean,
    gradients: TriangleGradients,
    timestamp: Long
)

/** Tracked pixel with triangle association */
case class TrackedPixel(
    triangleId: Long,
    x: Int,
    y: Int,
    r: Double,
    g: Double,
    b: Double,
    depth: Double,
    alpha: Double,
    timestamp: Long
)

/** Central tracking manager for triangles and pixels
  *
  * Thread-safe for concurrent access from simulation and UI threads. Maintains rolling windows for
  * both triangles and pixels to bound memory usage.
  *
  * @param maxTriangles
  *   Maximum triangles to keep in rolling window
  * @param maxPixels
  *   Maximum pixels to keep in rolling window
  * @param stride
  *   Framebuffer stride for spatial indexing
  */
class DebugTracker(
    maxTriangles: Int = 10000,
    maxPixels: Int = 100000,
    stride: Int = 1024
) {

  // Triangle ID generator
  private val triangleIdCounter = new AtomicLong(0)

  // Rolling window of all triangles (for history)
  private val triangleBuffer = new ConcurrentLinkedDeque[TrackedTriangle]()

  // Triangle lookup by ID
  private val triangleById = new ConcurrentHashMap[Long, TrackedTriangle]()

  // Pipeline tracking: triangles being built (between setup.i and rasterizer.i)
  private val pendingTriangles = new ConcurrentLinkedQueue[TrackedTriangle]()

  // Pipeline tracking: triangles in rasterizer (FIFO, front = current)
  private val rasterizerFifo = new ConcurrentLinkedQueue[TrackedTriangle]()

  // Spatial index: position -> list of pixels
  private val pixelsByPosition = new ConcurrentHashMap[Long, ConcurrentLinkedDeque[TrackedPixel]]()

  // Rolling window of all pixels (for eviction tracking)
  private val pixelBuffer = new ConcurrentLinkedDeque[TrackedPixel]()

  // Pixel count for eviction
  private val pixelCount = new AtomicLong(0)

  /** Create a new triangle when it enters triangleSetup.i
    *
    * @return
    *   The new triangle ID
    */
  def createTriangle(
      vertices: Array[(Double, Double)],
      signBit: Boolean,
      gradients: TriangleGradients,
      timestamp: Long
  ): Long = {
    val id = triangleIdCounter.incrementAndGet()

    val triangle = TrackedTriangle(
      id = id,
      vertices = vertices,
      boundingBox = (0, 0, 0, 0), // Will be updated by triangleSetup.o
      coefficients = Array.empty, // Will be updated by triangleSetup.o
      signBit = signBit,
      gradients = gradients,
      timestamp = timestamp
    )

    // Add to pending queue (awaiting setup completion)
    pendingTriangles.offer(triangle)

    // Add to main buffer and lookup
    triangleBuffer.addLast(triangle)
    triangleById.put(id, triangle)

    // Evict old triangles if needed
    while (triangleBuffer.size() > maxTriangles) {
      val old = triangleBuffer.pollFirst()
      if (old != null) {
        triangleById.remove(old.id)
      }
    }

    id
  }

  /** Update the most recent triangle with setup output (coefficients, bbox)
    *
    * Called when triangleSetup.o fires
    */
  def updateTriangleSetup(
      coefficients: Array[(Double, Double, Double)],
      boundingBox: (Int, Int, Int, Int)
  ): Unit = {
    // Update the most recently created triangle (front of pending queue, but don't remove yet)
    val triangle = pendingTriangles.peek()
    if (triangle != null) {
      triangle.coefficients = coefficients
      triangle.boundingBox = boundingBox
    }
  }

  /** Move triangle from pending to rasterizer FIFO
    *
    * Called when rasterizer.i fires (triangle enters rasterizer)
    */
  def pushToRasterizer(): Unit = {
    val triangle = pendingTriangles.poll()
    if (triangle != null) {
      rasterizerFifo.offer(triangle)
    }
  }

  /** Add a pixel associated with the current triangle in rasterizer
    *
    * Called when rasterizer.o fires
    */
  def addPixel(
      x: Int,
      y: Int,
      r: Double,
      g: Double,
      b: Double,
      depth: Double,
      alpha: Double,
      timestamp: Long
  ): Unit = {
    // Get current triangle from rasterizer FIFO
    val currentTriangle = rasterizerFifo.peek()
    val triangleId = if (currentTriangle != null) currentTriangle.id else -1L

    val pixel = TrackedPixel(
      triangleId = triangleId,
      x = x,
      y = y,
      r = r,
      g = g,
      b = b,
      depth = depth,
      alpha = alpha,
      timestamp = timestamp
    )

    // Add to spatial index
    val posKey = y.toLong * stride + x
    val pixelList = pixelsByPosition.computeIfAbsent(posKey, _ => new ConcurrentLinkedDeque())
    pixelList.addLast(pixel)

    // Add to rolling buffer
    pixelBuffer.addLast(pixel)
    val count = pixelCount.incrementAndGet()

    // Evict old pixels if needed
    if (count > maxPixels) {
      evictOldPixels()
    }
  }

  /** Called when rasterizer finishes a triangle (running falls) */
  def rasterizerFinished(): Unit = {
    rasterizerFifo.poll() // Remove completed triangle
  }

  /** Get all pixels at a screen position */
  def getPixelsAt(x: Int, y: Int): Seq[TrackedPixel] = {
    val posKey = y.toLong * stride + x
    val pixelList = pixelsByPosition.get(posKey)
    if (pixelList != null) {
      pixelList.asScala.toSeq
    } else {
      Seq.empty
    }
  }

  /** Get triangle by ID */
  def getTriangle(id: Long): Option[TrackedTriangle] = {
    Option(triangleById.get(id))
  }

  /** Get all triangles (for debugging) */
  def getAllTriangles: Seq[TrackedTriangle] = {
    triangleBuffer.asScala.toSeq
  }

  /** Test if a point is inside a triangle using cross-product same-side test. Uses only vertex
    * coordinates (not edge coefficients).
    */
  private def pointInTriangle(px: Double, py: Double, tri: TrackedTriangle): Boolean = {
    if (tri.vertices.length < 3) return false

    val (ax, ay) = tri.vertices(0)
    val (bx, by) = tri.vertices(1)
    val (cx, cy) = tri.vertices(2)

    // Cross product sign for each edge
    def sign(x1: Double, y1: Double, x2: Double, y2: Double, x3: Double, y3: Double): Double =
      (x1 - x3) * (y2 - y3) - (x2 - x3) * (y1 - y3)

    val d1 = sign(px, py, ax, ay, bx, by)
    val d2 = sign(px, py, bx, by, cx, cy)
    val d3 = sign(px, py, cx, cy, ax, ay)

    val hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0)
    val hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0)

    !(hasNeg && hasPos) // Inside if all same sign (or zero)
  }

  /** Get all triangles that geometrically contain the given point. Uses bounding box for fast
    * rejection, then cross-product test.
    */
  def getTrianglesContaining(x: Int, y: Int): Seq[TrackedTriangle] = {
    triangleBuffer.asScala.filter { tri =>
      val (xmin, ymin, xmax, ymax) = tri.boundingBox
      // Fast bounding box rejection
      x >= xmin && x <= xmax && y >= ymin && y <= ymax &&
      // Precise point-in-triangle test
      pointInTriangle(x.toDouble, y.toDouble, tri)
    }.toSeq
  }

  /** Get statistics */
  def getStats: (Int, Long) = {
    (triangleBuffer.size(), pixelCount.get())
  }

  /** Evict old pixels to stay under limit */
  private def evictOldPixels(): Unit = {
    while (pixelCount.get() > maxPixels) {
      val old = pixelBuffer.pollFirst()
      if (old != null) {
        // Remove from spatial index
        val posKey = old.y.toLong * stride + old.x
        val pixelList = pixelsByPosition.get(posKey)
        if (pixelList != null) {
          pixelList.removeFirstOccurrence(old)
          if (pixelList.isEmpty) {
            pixelsByPosition.remove(posKey)
          }
        }
        pixelCount.decrementAndGet()
      } else {
        // Buffer empty, reset count
        pixelCount.set(0)
        return
      }
    }
  }

  /** Clear all tracked data */
  def clear(): Unit = {
    triangleBuffer.clear()
    triangleById.clear()
    pendingTriangles.clear()
    rasterizerFifo.clear()
    pixelsByPosition.clear()
    pixelBuffer.clear()
    pixelCount.set(0)
  }
}

object DebugTracker {
  def apply(stride: Int): DebugTracker = new DebugTracker(stride = stride)
}
