package org.maplibre.nativeffi.examples.androidmap

import android.util.Log
import java.util.concurrent.Semaphore
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.GeneratedApi
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
 * A caller-driver native surface that the UI thread drives. The session's wakes post its driver
 * work and frame results to the UI thread, where the view services and drains them.
 */
internal class SurfaceRenderTarget
private constructor(private val session: RenderSessionHandle, private val driverWork: Semaphore) :
  AutoCloseable {
  /** Set once the attachment completes; a session accepts frame demand only after that. */
  @Volatile
  var attached = false
    private set

  /** The viewport the session last took, through attachment or [follow]. */
  var viewport: Viewport? = null
    private set

  /**
   * Asks for a frame. A forced frame renders and presents even when the map has no newer update.
   */
  fun requestFrame(force: Boolean) {
    val flags =
      if (force) FrameDemandFlag.PRESENT else FrameDemandFlag.IF_NEEDED or FrameDemandFlag.PRESENT
    session.requestFrame(GeneratedApi.frameDemandDefault().copy(flags = flags))
  }

  fun serviceDriverWork() {
    session.serviceDriverWork(0uL)
  }

  /**
   * What one frame-result drain saw. A target that was not ready consumed its map update, so the
   * view retries with a forced frame.
   */
  data class Drained(val rendered: Boolean, val needsRepaint: Boolean, val targetNotReady: Boolean)

  /** Drains every frame result. */
  fun drainFrameResults(): Drained {
    val batch =
      try {
        session.drainFrameResults()
      } catch (error: MaplibreException) {
        if (error.status == MaplibreStatus.NOT_READY) return Drained(false, false, false)
        throw error
      }
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
   * viewport. A session resize carries the map's extent itself. An EGL surface handover replaces
   * only the graphics resource, so that path submits the map resize alongside it. The returned
   * handover or resize completes through driver work; a caller whose outgoing surface is about to
   * go passes it to [awaitDriverWork], and a failure otherwise reaches [onFailure].
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
          val handover =
            session.openglSurfaceSetTarget(
              OpenglSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
            )
          map.resize(
            LogicalExtent(
              viewport.logicalWidth.toUInt(),
              viewport.logicalHeight.toUInt(),
              viewport.scaleFactor,
            )
          )
          handover
        }
        is VulkanGraphicsContext -> session.resize(viewport.extent)
        else -> error("Unsupported graphics context: ${graphics::class.java.name}")
      }
    completion.invokeOnCompletion { error -> if (error != null) onFailure(error) }
    return completion
  }

  /**
   * Releases the session. Detach services driver work on the UI thread until it completes; a
   * platform callback that arrives after the surface is gone falls back to abandoning it.
   */
  override fun close() {
    try {
      awaitDriverWork(session.detach())
    } catch (error: RuntimeException) {
      Log.w(TAG, "detaching the render session failed; abandoning it instead", error)
      runCatching { session.abandon() }
    }
    session.close()
  }

  /**
   * Services driver work until [completion] finishes, sleeping until the session's driver-work wake
   * or the completion arrives. Only teardown and a surface loss wait like this; everything else
   * follows the wakes.
   */
  fun awaitDriverWork(completion: Deferred<*>) {
    // Permits from wakes the UI thread already serviced would only spin the loop.
    driverWork.drainPermits()
    completion.invokeOnCompletion { driverWork.release() }
    while (true) {
      session.serviceDriverWork(0uL)
      if (completion.isCompleted) break
      driverWork.acquire()
    }
    runBlocking { completion.await() }
  }

  companion object {
    private const val TAG = "MapLibreAndroidMap"

    /**
     * Starts attaching a session on the UI thread, which owns it until close. The session raises
     * [frameWake] with frame results and [onDriverWork] with driver work. The attachment completes
     * through that driver work, and then [onAttached] runs on a native thread with the attachment's
     * failure, or null.
     */
    fun attach(
      map: MapHandle,
      graphics: GraphicsContext,
      viewport: Viewport,
      frameWake: Wake,
      onDriverWork: () -> Unit,
      onAttached: (Throwable?) -> Unit,
    ): SurfaceRenderTarget {
      val driverWork = Semaphore(0)
      val options =
        RenderSessionAttachOptions(
          driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
          frameWake = frameWake,
          driverWorkWake =
            Wake {
              driverWork.release()
              onDriverWork()
            },
        )
      val attachment =
        when (graphics) {
          is EglGraphicsContext ->
            map.openglSurfaceAttach(
              OpenglSurfaceDescriptor(
                viewport.extent,
                graphics.descriptor,
                graphics.surfacePointer,
              ),
              options,
            )
          is VulkanGraphicsContext ->
            map.vulkanSurfaceAttach(
              VulkanSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfaceHandle),
              options,
            )
          else -> error("Unsupported graphics context: ${graphics::class.java.name}")
        }
      val target = SurfaceRenderTarget(attachment.session, driverWork)
      target.viewport = viewport
      attachment.ready.invokeOnCompletion { error ->
        if (error == null) target.attached = true
        onAttached(error)
      }
      return target
    }
  }
}
