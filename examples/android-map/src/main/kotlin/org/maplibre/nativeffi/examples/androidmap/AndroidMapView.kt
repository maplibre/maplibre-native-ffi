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
    recreateSurface(holder)
  }

  override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
    recreateSurface(holder)
  }

  override fun surfaceDestroyed(holder: SurfaceHolder) {
    detachSurface()
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
        // Consume before rendering, so a request the runtime loop publishes during the render call
        // is not discarded.
        if (target != null && loop.renderRequest.consume()) {
          if (target.renderUpdate()) {
            finishPendingDrawing()
          } else {
            // The map applies its logical size on the runtime loop's next runOnce, so an attach is
            // followed by frames with nothing to render. Keep pacing and retry.
            loop.renderRequest.set()
          }
        }
      } catch (error: RuntimeException) {
        Log.e(TAG, "frame failed", error)
        loop.renderRequest.set()
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

  private fun recreateSurface(holder: SurfaceHolder) {
    if (closed) return
    detachSurface()
    val nextViewport =
      Viewport.fromView(width, height, resources.displayMetrics.density).also { it.log("surface") }
    viewport = nextViewport
    if (nextViewport.isEmpty) {
      finishPendingDrawing()
      return
    }
    val nextGraphics = GraphicsContext.create(holder.surface)
    graphics = nextGraphics
    // The runtime loop outlives surface changes: it keeps the runtime and the map alive so loading
    // continues while there is nothing to present to.
    if (runtimeLoop == null) {
      runtimeLoop = MapRuntimeLoop(nextViewport)
    }
    Log.i(TAG, "render-target=native-surface status=${nextGraphics.backendName}")
    requestRender()
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
    !closed && viewVisible && appForeground && graphics != null && runtimeLoop != null

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
