package org.maplibre.nativeffi.examples.androidmap

import android.content.Context
import android.util.Log
import android.view.Choreographer
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView

internal class AndroidMapView(context: Context) :
  SurfaceView(context), SurfaceHolder.Callback2, Choreographer.FrameCallback, AutoCloseable {
  private val input = InputController(context, { mapState?.map }, ::requestRender)
  private var graphics: GraphicsContext? = null
  private var mapState: MapState? = null
  private var viewport: Viewport? = null
  private var viewVisible = false
  private var appForeground = false
  private var renderPending = true
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
    val state = mapState ?: return
    try {
      state.runOnce()
      renderPending = state.drainEvents() || renderPending
      if (renderPending) {
        state.renderUpdate()
        renderPending = false
        finishPendingDrawing()
      }
    } catch (error: RuntimeException) {
      Log.e(TAG, "frame failed", error)
      renderPending = true
    }
    startLoopIfReady()
  }

  fun requestRender() {
    renderPending = true
    startLoopIfReady()
  }

  override fun close() {
    if (closed) return
    closed = true
    stopLoop()
    detachSurface()
    mapState?.close()
    mapState = null
  }

  private fun recreateSurface(holder: SurfaceHolder) {
    if (closed) return
    detachSurface()
    val nextViewport =
      Viewport.fromView(width, height, resources.displayMetrics.density).also { it.log("surface") }
    if (nextViewport.isEmpty) {
      viewport = nextViewport
      finishPendingDrawing()
      return
    }
    val nextGraphics = GraphicsContext.create(holder.surface)
    graphics = nextGraphics
    viewport = nextViewport
    val state = mapState
    if (state == null) {
      mapState = MapState.create(nextGraphics, nextViewport)
    } else {
      state.attachOrResize(nextGraphics, nextViewport)
    }
    Log.i(TAG, "render-target=native-surface status=${nextGraphics.backendName}")
    requestRender()
  }

  private fun detachSurface() {
    mapState?.detachRenderTarget()
    graphics?.close()
    graphics = null
    finishPendingDrawing()
  }

  private fun startLoopIfReady() {
    if (frameCallbackPosted || !canRenderFrame()) {
      return
    }
    frameCallbackPosted = true
    Choreographer.getInstance().postFrameCallback(this)
  }

  private fun canRenderFrame(): Boolean =
    !closed && viewVisible && appForeground && graphics != null && mapState != null

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
