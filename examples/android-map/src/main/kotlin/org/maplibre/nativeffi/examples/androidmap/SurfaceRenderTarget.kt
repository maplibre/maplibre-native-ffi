package org.maplibre.nativeffi.examples.androidmap

import android.util.Log
import java.util.concurrent.Semaphore
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.generated.FrameDemand
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.OpenglSurfaceDescriptor
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.generated.Wake

/**
 * A native-surface render session. A Vulkan session renders and presents on its core worker. An EGL
 * window surface requires the caller driver, so the session's driver-work wake posts its work to
 * the UI thread, which services it. Either way the frame wake posts the results to the UI thread.
 */
internal class SurfaceRenderTarget
private constructor(
  private val session: RenderSessionHandle,
  val driver: RenderDriverKind,
  /** Permits from the driver-work wake, which only a caller driver has. */
  private val driverWork: Semaphore?,
) : AutoCloseable {
  private var nextToken = 0uL

  /** Set once the attachment completes; a session accepts frame demand only after that. */
  @Volatile
  var attached = false
    private set

  /** The viewport the session last took, through attachment or [follow]. */
  var viewport: Viewport? = null
    private set

  private val callerDriver: Boolean
    get() = driver == RenderDriverKind.CALLER_GRAPHICS_THREAD

  val driverLabel: String
    get() = if (callerDriver) "caller-graphics-thread" else "core-worker"

  /** Demands a frame. A forced frame renders and presents even when the map has no newer update. */
  fun requestFrame(force: Boolean) {
    val flags =
      if (force) FrameDemandFlag.PRESENT else FrameDemandFlag.IF_NEEDED or FrameDemandFlag.PRESENT
    session.requestFrame(FrameDemand(flags = flags, token = ++nextToken))
  }

  /** Runs every queued item of a caller driver after its wake. */
  fun serviceDriverWork() {
    if (callerDriver) session.serviceDriverWork(0uL)
  }

  /**
   * What one frame-result drain saw. A target that was not ready does not cause a map-update event,
   * so the view retries on its next frame.
   */
  data class Drained(val rendered: Boolean, val needsRepaint: Boolean, val targetNotReady: Boolean)

  /** Drains every frame result. */
  fun drainFrameResults(): Drained {
    val batch = session.drainFrameResults() ?: return Drained(false, false, false)
    return batch.use { results ->
      var rendered = false
      var needsRepaint = false
      var targetNotReady = false
      for (index in 0uL until results.count()) {
        val result = results.get(index)
        when (result.disposition) {
          RenderResult.RENDERED -> rendered = true
          RenderResult.TARGET_NOT_READY -> targetNotReady = true
          else -> continue
        }
        needsRepaint = needsRepaint || result.needsRepaint
      }
      Drained(rendered, needsRepaint, targetNotReady)
    }
  }

  /**
   * Points the session at the surface the graphics context presents through now, and at the
   * viewport. A session resize carries the map's extent itself. An EGL surface replacement changes
   * only the graphics resource, so that path submits the map resize alongside it. A caller whose
   * outgoing surface is about to go passes the returned completion to [await], and a failure
   * otherwise reaches [onFailure].
   */
  fun follow(
    map: MapHandle,
    graphics: GraphicsContext,
    viewport: Viewport,
    onFailure: (Throwable) -> Unit,
  ): Deferred<*> {
    this.viewport = viewport
    val completion =
      when (graphics) {
        is EglGraphicsContext -> {
          val replacement =
            session.setOpenglSurfaceTarget(
              OpenglSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
            )
          map.resize(
            LogicalExtent(
              viewport.logicalWidth.toUInt(),
              viewport.logicalHeight.toUInt(),
              viewport.scaleFactor,
            )
          )
          replacement
        }
        is VulkanGraphicsContext -> session.resize(viewport.extent)
        else -> error("Unsupported graphics context: ${graphics::class.java.name}")
      }
    completion.invokeOnCompletion { error -> if (error != null) onFailure(error) }
    return completion
  }

  /**
   * Waits until every frame demanded before it has a result, so that no frame renders after the app
   * leaves the foreground.
   */
  fun barrier() {
    if (attached) await(session.barrier())
  }

  /**
   * Releases the session. A failed detach, such as from a platform callback that arrives after the
   * surface is gone, abandons it instead.
   */
  override fun close() {
    try {
      await(session.detach())
    } catch (error: RuntimeException) {
      Log.w(TAG, "detaching the render session failed; abandoning it instead", error)
      runCatching { session.abandon() }
        .onSuccess { result ->
          if (result.quarantinedResourceCount > 0u) {
            Log.w(TAG, "render session quarantined ${result.quarantinedResourceCount} resources")
          }
        }
    }
    session.close()
  }

  /**
   * Waits for [completion] on the UI thread. A caller driver's completion progresses only through
   * driver service, so the thread services the session between waits for the driver-work wake or
   * for the completion. Only teardown, the background barrier, and a surface loss wait like this;
   * everything else follows the wakes.
   */
  fun await(completion: Deferred<*>) {
    if (driverWork != null) {
      // Permits from wakes the UI thread already serviced would only spin the loop.
      driverWork.drainPermits()
      completion.invokeOnCompletion { driverWork.release() }
      while (true) {
        session.serviceDriverWork(0uL)
        if (completion.isCompleted) break
        driverWork.acquire()
      }
    }
    runBlocking { completion.await() }
  }

  companion object {
    private const val TAG = "MapLibreAndroidMap"

    /**
     * Starts attaching a session. The UI thread services a caller driver and closes the session.
     * The session raises [frameWake] with frame results and, for a caller driver, [onDriverWork]
     * with driver work. [onAttached] runs on a native thread with the attachment's failure, or
     * null.
     */
    fun attach(
      map: MapHandle,
      graphics: GraphicsContext,
      viewport: Viewport,
      frameWake: Wake,
      onDriverWork: () -> Unit,
      onAttached: (Throwable?) -> Unit,
    ): SurfaceRenderTarget {
      // A Vulkan surface accepts a core worker. An OpenGL surface on an EGL context requires the
      // caller driver.
      val driver =
        when (graphics) {
          is VulkanGraphicsContext -> RenderDriverKind.CORE_WORKER
          else -> RenderDriverKind.CALLER_GRAPHICS_THREAD
        }
      val driverWork = if (driver == RenderDriverKind.CALLER_GRAPHICS_THREAD) Semaphore(0) else null
      val options =
        RenderSessionAttachOptions(
          driver = driver,
          frameWake = frameWake,
          driverWorkWake =
            driverWork?.let { work ->
              Wake {
                work.release()
                onDriverWork()
              }
            } ?: Wake(),
        )
      val attachment =
        when (graphics) {
          is EglGraphicsContext ->
            map.attachOpenglSurface(
              OpenglSurfaceDescriptor(
                viewport.extent,
                graphics.descriptor,
                graphics.surfacePointer,
              ),
              options,
            )
          is VulkanGraphicsContext ->
            map.attachVulkanSurface(
              VulkanSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfaceHandle),
              options,
            )
          else -> error("Unsupported graphics context: ${graphics::class.java.name}")
        }
      val target = SurfaceRenderTarget(attachment.session, driver, driverWork)
      target.viewport = viewport
      attachment.ready.invokeOnCompletion { error ->
        if (error == null) target.attached = true
        onAttached(error)
      }
      return target
    }
  }
}
