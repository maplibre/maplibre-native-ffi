package org.maplibre.nativeffi.examples.androidmap

import android.content.Context
import android.util.Log
import android.view.Choreographer
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView

/**
 * The render loop.
 *
 * The UI thread owns the surface, touch input, the viewport, the graphics context, and the render
 * session it attaches. It touches the map only to attach, which native serves from any thread;
 * every other map call belongs to [MapRuntimeLoop].
 *
 * The platform destroys and recreates this view's surface across a rotation and a trip to the
 * background. A session survives that only while its graphics context does, so a context that goes
 * with its surface leaves the next frame attaching a cold session.
 */
internal class AndroidMapView(context: Context) :
  SurfaceView(context), SurfaceHolder.Callback2, Choreographer.FrameCallback, AutoCloseable {
  private val input = InputController(context, ::enqueueCameraCommand)
  private var graphics: GraphicsContext? = null
  private var renderTarget: SurfaceRenderTarget? = null
  private var runtimeLoop: MapRuntimeLoop? = null
  private var viewport: Viewport? = null
  private var viewVisible = false
  private var appForeground = false
  private var frameCallbackPosted = false

  /** Set when a frame threw, so the view stops scheduling against a broken target. */
  private var frameFailed = false

  /** Set when a failed frame spent its one rebuild, until a rendered frame earns another. */
  private var contextRebuildSpent = false
  private var closed = false
  private val pendingDrawingFinished = ArrayDeque<Runnable>()

  init {
    holder.addCallback(this)
    isFocusable = true
    isFocusableInTouchMode = true
  }

  override fun onAttachedToWindow() {
    super.onAttachedToWindow()
    viewVisible = true
    startLoopIfReady()
  }

  override fun onDetachedFromWindow() {
    viewVisible = false
    stopLoop()
    detachSurface()
    super.onDetachedFromWindow()
  }

  fun enterForeground() {
    appForeground = true
    startLoopIfReady()
  }

  fun enterBackground() {
    appForeground = false
    stopLoop()
    finishPendingDrawing()
  }

  override fun surfaceCreated(holder: SurfaceHolder) {
    surfaceAvailable(holder)
  }

  override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
    surfaceAvailable(holder)
  }

  override fun surfaceDestroyed(holder: SurfaceHolder) {
    surfaceLost()
  }

  override fun surfaceRedrawNeeded(holder: SurfaceHolder) {
    requestRender()
  }

  override fun surfaceRedrawNeededAsync(holder: SurfaceHolder, drawingFinished: Runnable) {
    pendingDrawingFinished += drawingFinished
    requestRender()
    if (!canRenderFrame()) {
      finishPendingDrawing()
    }
  }

  override fun onTouchEvent(event: MotionEvent): Boolean = input.onTouchEvent(event)

  override fun doFrame(frameTimeNanos: Long) {
    frameCallbackPosted = false
    val loop = runtimeLoop
    if (loop != null) {
      try {
        val target = ensureRenderTarget(loop)
        if (target != null && loop.renderRequest.consume()) {
          if (target.renderUpdate()) {
            contextRebuildSpent = false
            finishPendingDrawing()
          } else {
            // The map applies its logical size on the runtime loop's next pump, so an attach is
            // followed by frames with nothing to render.
            loop.renderRequest.set()
          }
        }
      } catch (error: RuntimeException) {
        Log.e(TAG, "frame failed", error)
        rebuildAfterFrameFailure()
      }
    }
    startLoopIfReady()
  }

  fun requestRender() {
    runtimeLoop?.renderRequest?.set()
    startLoopIfReady()
  }

  override fun close() {
    if (closed) return
    closed = true
    stopLoop()
    // Close the session before the runtime loop closes the map: a map with an attached session
    // cannot be destroyed.
    detachSurface()
    runtimeLoop?.close()
    runtimeLoop = null
  }

  private fun enqueueCameraCommand(command: CameraCommand) {
    runtimeLoop?.enqueue(command)
    requestRender()
  }

  private fun surfaceAvailable(holder: SurfaceHolder) {
    if (closed) return
    val nextViewport =
      Viewport.fromView(width, height, resources.displayMetrics.density).also { it.log("surface") }
    viewport = nextViewport
    if (nextViewport.isEmpty) {
      finishPendingDrawing()
      return
    }
    if (graphics?.setSurface(holder.surface) != true) {
      // A session outlives only the context it attached against, so replacing the context closes
      // the session and the next frame attaches a cold one.
      detachSurface()
      val nextGraphics = GraphicsContext.create(holder.surface)
      graphics = nextGraphics
      Log.i(TAG, "render-target=native-surface status=${nextGraphics.backendName}")
    }
    // The runtime loop outlives surface changes, so loading continues while there is nothing to
    // present to.
    if (runtimeLoop == null) {
      runtimeLoop = MapRuntimeLoop(nextViewport)
    }
    followSurface("surface available")
    requestRender()
  }

  private fun surfaceLost() {
    if (graphics?.releaseSurface() == true) {
      // The context outlived the surface, so the session parks on it until a surface returns.
      followSurface("surface released")
    } else {
      detachSurface()
    }
    finishPendingDrawing()
  }

  /**
   * Points the live session at the surface its graphics context presents through now, and at the
   * current viewport. A session yet to be attached takes both from [SurfaceRenderTarget.attach].
   */
  private fun followSurface(change: String) {
    val currentGraphics = graphics ?: return
    val currentViewport = viewport?.takeUnless { it.isEmpty } ?: return
    val target = renderTarget ?: return
    try {
      target.resize(currentGraphics, currentViewport)
    } catch (error: RuntimeException) {
      // A failed handover may leave the session naming a destroyed surface, so close it here; the
      // next surface attaches a new one.
      Log.w(TAG, "$change: handing the surface over failed; the session is closed", error)
      detachSurface()
      return
    }
    Log.i(TAG, "$change: the live session followed it and kept its renderer")
  }

  /**
   * Builds the graphics context again after a failed frame, once. A second failure with no good
   * frame in between stops scheduling rather than reposting every vsync against a broken target.
   */
  private fun rebuildAfterFrameFailure() {
    detachSurface()
    val surface = holder.surface
    if (contextRebuildSpent || !surface.isValid) {
      frameFailed = true
      return
    }
    contextRebuildSpent = true
    graphics =
      try {
        GraphicsContext.create(surface)
      } catch (error: RuntimeException) {
        Log.e(TAG, "rebuilding the graphics context failed", error)
        frameFailed = true
        null
      }
  }

  private fun detachSurface() {
    renderTarget?.close()
    renderTarget = null
    graphics?.close()
    graphics = null
    finishPendingDrawing()
  }

  /** Attaches a session against the published map, on this thread, which then owns it. */
  private fun ensureRenderTarget(loop: MapRuntimeLoop): SurfaceRenderTarget? {
    renderTarget?.let {
      return it
    }
    val currentGraphics = graphics ?: return null
    val currentViewport = viewport?.takeUnless { it.isEmpty } ?: return null
    val map = loop.map ?: return null
    val attached = SurfaceRenderTarget.attach(map, currentGraphics, currentViewport)
    renderTarget = attached
    loop.requestRepaint()
    loop.renderRequest.set()
    return attached
  }

  private fun startLoopIfReady() {
    if (frameCallbackPosted || !canRenderFrame()) {
      return
    }
    frameCallbackPosted = true
    Choreographer.getInstance().postFrameCallback(this)
  }

  private fun canRenderFrame(): Boolean =
    !closed &&
      viewVisible &&
      appForeground &&
      graphics?.hasSurface == true &&
      !frameFailed &&
      runtimeLoop != null &&
      // A loop whose setup failed never publishes a map.
      runtimeLoop?.setupFailure == null

  private fun finishPendingDrawing() {
    while (pendingDrawingFinished.isNotEmpty()) {
      pendingDrawingFinished.removeFirst().run()
    }
  }

  private fun stopLoop() {
    if (frameCallbackPosted) {
      Choreographer.getInstance().removeFrameCallback(this)
      frameCallbackPosted = false
    }
  }

  private companion object {
    private const val TAG = "MapLibreAndroidMap"
  }
}
