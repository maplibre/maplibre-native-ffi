package org.maplibre.nativeffi.examples.androidmap

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.Choreographer
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.Wake

/**
 * The map view. The UI thread owns the surface, touch input, viewport, graphics context, and render
 * session.
 *
 * Touch input becomes map commands. A map update schedules one frame demand on the next
 * Choreographer frame. Native wakes post the work they announce to the UI thread: runtime events to
 * drain, frame results to drain, and, for a caller driver, driver work to service.
 */
internal class AndroidMapView(
  context: Context,
  private val styleJson: String? = null,
  private val onRendered: () -> Unit = {},
) : SurfaceView(context), SurfaceHolder.Callback2, Choreographer.FrameCallback, AutoCloseable {
  private val handler = Handler(Looper.getMainLooper())
  private val input = InputController(context) { mapState }
  private var graphics: GraphicsContext? = null
  private var renderTarget: SurfaceRenderTarget? = null
  private var mapState: MapState? = null
  private var viewport: Viewport? = null
  private var viewVisible = false
  private var appForeground = false
  private var frameCallbackPosted = false

  /** Set when the platform asked for a redraw, which renders even with no newer map update. */
  private var redrawRequested = false

  /** Set when a frame threw, so the view stops scheduling against a broken target. */
  private var frameFailed = false

  /** Set when a failed frame spent its one rebuild, until a rendered frame earns another. */
  private var contextRebuildSpent = false
  private var closed = false
  private val pendingDrawingFinished = ArrayDeque<Runnable>()

  private val eventWake = UiWake { if (mapState?.drainRenderUpdates() == true) scheduleFrame() }
  private val frameWake = UiWake(::drainFrameResults)
  private val driverWake = UiWake { renderTarget?.serviceDriverWork() }

  init {
    holder.addCallback(this)
    isFocusable = true
    isFocusableInTouchMode = true
  }

  override fun onAttachedToWindow() {
    super.onAttachedToWindow()
    viewVisible = true
    scheduleFrame()
  }

  override fun onDetachedFromWindow() {
    viewVisible = false
    stopFrames()
    detachSurface()
    super.onDetachedFromWindow()
  }

  fun enterForeground() {
    appForeground = true
    scheduleFrame()
  }

  /** Pauses demand, and waits until no frame renders after the app leaves the foreground. */
  fun enterBackground() {
    appForeground = false
    stopFrames()
    try {
      renderTarget?.barrier()
    } catch (error: RuntimeException) {
      Log.w(TAG, "the background render barrier failed", error)
    }
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
    requestRedraw()
  }

  override fun surfaceRedrawNeededAsync(holder: SurfaceHolder, drawingFinished: Runnable) {
    pendingDrawingFinished += drawingFinished
    requestRedraw()
    if (!canRenderFrame()) {
      finishPendingDrawing()
    }
  }

  override fun onTouchEvent(event: MotionEvent): Boolean = input.onTouchEvent(event)

  override fun doFrame(frameTimeNanos: Long) {
    frameCallbackPosted = false
    if (!canRenderFrame()) return
    val force = redrawRequested
    redrawRequested = false
    guarded { renderTarget?.requestFrame(force) }
  }

  override fun close() {
    if (closed) return
    closed = true
    stopFrames()
    // Close the render session before closing the map and runtime.
    detachSurface()
    mapState?.close()
    mapState = null
  }

  private fun drainFrameResults() {
    val drained = renderTarget?.drainFrameResults() ?: return
    if (drained.rendered) {
      contextRebuildSpent = false
      finishPendingDrawing()
      onRendered()
    }
    // A target that was not ready consumed the map update, so the next Choreographer frame forces
    // a retry. The result otherwise carries the map's own follow-up demand, so an ongoing
    // transition needs no runtime event round trip.
    if (drained.targetNotReady) requestRedraw() else if (drained.needsRepaint) scheduleFrame()
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
      // the session and attaches a cold one.
      detachSurface()
      val nextGraphics = GraphicsContext.create(holder.surface)
      graphics = nextGraphics
      Log.i(TAG, "graphics context: ${nextGraphics.backendName}")
    }
    val state = mapState
    if (state == null) {
      mapState = MapState(nextViewport, eventWake.wake, styleJson)
    } else if (renderTarget == null) {
      // With no session attached the map is the only extent authority; a live session carries the
      // extent through followSurface below.
      state.resize(nextViewport)
    }
    if (renderTarget == null) attachSurface() else followSurface("surface available")
    requestRedraw()
  }

  /**
   * Parks a live session off the outgoing surface before this callback returns, since the surface
   * is gone after that. A session still attaching against it closes instead.
   */
  private fun surfaceLost() {
    val target = renderTarget
    val parked =
      target?.attached != false &&
        try {
          graphics?.releaseSurface {
            // The context outlived the surface, so the session parks on it until a surface returns.
            followSurface("surface released")?.let { target?.await(it) }
          } == true
        } catch (error: RuntimeException) {
          Log.w(TAG, "parking the render session failed; closing it", error)
          false
        }
    if (!parked) detachSurface()
    finishPendingDrawing()
  }

  /** Starts attaching a session once a map, a context, and a non-empty viewport all exist. */
  private fun attachSurface() {
    val state = mapState ?: return
    val currentGraphics = graphics ?: return
    val currentViewport = viewport?.takeUnless { it.isEmpty } ?: return
    var target: SurfaceRenderTarget? = null
    target =
      SurfaceRenderTarget.attach(
        state.map,
        currentGraphics,
        currentViewport,
        frameWake.wake,
        driverWake::post,
      ) { error ->
        handler.post { onAttached(target, error) }
      }
    renderTarget = target
  }

  private fun onAttached(target: SurfaceRenderTarget?, error: Throwable?) {
    if (target == null || target !== renderTarget) return
    if (error != null) {
      Log.e(TAG, "attaching the render session failed", error)
      detachSurface()
      return
    }
    Log.i(TAG, "render target: native-surface")
    Log.i(TAG, "render target status: renders directly to the host window surface")
    Log.i(TAG, "render driver: ${target.driverLabel}")
    // The viewport may have changed while the session attached.
    if (target.viewport != viewport) followSurface("attached")
    scheduleFrame()
  }

  /**
   * Points the live session at the surface its graphics context presents through now, and at the
   * current viewport, and returns the handover. A session still attaching takes both once it
   * attaches.
   */
  private fun followSurface(change: String): Deferred<*>? {
    val currentGraphics = graphics ?: return null
    val currentViewport = viewport?.takeUnless { it.isEmpty } ?: return null
    val target = renderTarget?.takeIf { it.attached } ?: return null
    val state = mapState ?: return null
    val handover =
      target.follow(state.map, currentGraphics, currentViewport) { error ->
        // A failed handover may leave the session naming a destroyed surface, so close it; the next
        // surface attaches a new one.
        handler.post {
          Log.w(TAG, "$change: handing the surface over failed; the session is closed", error)
          if (renderTarget === target) detachSurface()
        }
      }
    Log.i(TAG, "$change: the live session follows it and keeps its renderer")
    return handover
  }

  /** Runs one piece of UI-thread render work, rebuilding the context once if it throws. */
  private inline fun guarded(action: () -> Unit) {
    try {
      action()
    } catch (error: RuntimeException) {
      Log.e(TAG, "frame failed", error)
      rebuildAfterFrameFailure()
    }
  }

  /**
   * Builds the graphics context again after a failed frame, once. A second failure with no good
   * frame in between stops scheduling rather than demanding frames from a broken target.
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
    attachSurface()
  }

  private fun detachSurface() {
    // Reached from surfaceDestroyed and onDetachedFromWindow, where a throw would escape into the
    // platform and leave the graphics context leaked.
    try {
      renderTarget?.close()
    } catch (error: RuntimeException) {
      Log.w(TAG, "closing the render session failed", error)
    }
    renderTarget = null
    graphics?.close()
    graphics = null
    finishPendingDrawing()
  }

  /** Schedules a redraw that renders even when the map has nothing newer. */
  private fun requestRedraw() {
    redrawRequested = true
    scheduleFrame()
  }

  /** Demands one frame on the next Choreographer frame, while the view can show it. */
  private fun scheduleFrame() {
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
      renderTarget?.attached == true

  private fun finishPendingDrawing() {
    while (pendingDrawingFinished.isNotEmpty()) {
      pendingDrawingFinished.removeFirst().run()
    }
  }

  private fun stopFrames() {
    if (frameCallbackPosted) {
      Choreographer.getInstance().removeFrameCallback(this)
      frameCallbackPosted = false
    }
  }

  /**
   * A native wake that runs [action] on the UI thread. It posts once however often the wake fires
   * before the action runs, and the action drains or services everything that is ready.
   */
  private inner class UiWake(private val action: () -> Unit) {
    private val posted = AtomicBoolean(false)

    fun post() {
      if (posted.compareAndSet(false, true)) {
        handler.post {
          posted.set(false)
          if (!closed) guarded(action)
        }
      }
    }

    val wake = Wake { post() }
  }

  private companion object {
    private const val TAG = "MapLibreAndroidMap"
  }
}
