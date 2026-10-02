package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import org.lwjgl.glfw.GLFW.glfwPostEmptyEvent
import org.lwjgl.glfw.GLFW.glfwWaitEvents
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.FrameDemand
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionAttachment
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.generated.Wake

/**
 * Where a session's graphics work runs. A core worker runs it on its own thread. A caller driver
 * queues it for the GLFW thread, which services it after the driver-work wake.
 */
internal class SessionDriver(val kind: RenderDriverKind) {
  private val work = GlfwWake()

  private val caller: Boolean
    get() = kind == RenderDriverKind.CALLER_GRAPHICS_THREAD

  val label: String
    get() = if (caller) "caller-graphics-thread" else "core-worker"

  fun attachOptions(frames: GlfwWake, ringDepth: UInt): RenderSessionAttachOptions =
    RenderSessionAttachOptions(
      driver = kind,
      requestedTextureRingDepth = ringDepth,
      frameWake = frames.wake,
      driverWorkWake = if (caller) work.wake else Wake(),
    )

  /** Runs every queued item after the driver-work wake. */
  fun service(session: RenderSessionHandle) {
    if (caller && work.consume()) session.serviceDriverWork(0uL)
  }

  /**
   * Waits for [completion]. A caller driver's completion progresses only through driver service, so
   * the GLFW thread services the session between waits for the driver-work wake or for the
   * completion. This must not run inside a GLFW callback.
   */
  fun <T> await(session: RenderSessionHandle, completion: Deferred<T>): T {
    if (caller) {
      completion.invokeOnCompletion { glfwPostEmptyEvent() }
      while (true) {
        session.serviceDriverWork(0uL)
        if (completion.isCompleted) break
        glfwWaitEvents()
      }
    }
    return runBlocking { completion.await() }
  }
}

/**
 * A render session and the frames it shows. The shell demands a frame for each map update, and
 * after the frame wake [handleWakes] drains the results and shows each rendered frame. A native
 * surface presents in the session; the texture modes compose the frame into the window in
 * [present].
 */
internal open class RenderTarget(
  protected val session: RenderSessionHandle,
  protected val driver: SessionDriver,
  private val frames: GlfwWake,
) : AutoCloseable {
  private var nextToken = 0uL
  private var closed = false

  val driverLabel: String
    get() = driver.label

  /**
   * Set by a target whose texture the session and the compositor take turns on. Such a target keeps
   * at most one demand outstanding, and holds a wanted frame until the outstanding result arrives.
   */
  protected open val exclusiveTexture: Boolean = false

  /** Set by a native surface, the only target whose demands ask the session to present. */
  protected open val presents: Boolean = false
  private var demandOutstanding = false
  private var wanted: Boolean? = null

  /** When a paced retry is due, as a [System.nanoTime] value, or null with none pending. */
  var retryAtNanos: Long? = null
    private set

  /** Demands a frame. A forced frame renders even when the map has no newer update. */
  fun requestFrame(force: Boolean = false) {
    if (exclusiveTexture && demandOutstanding) {
      wanted = force || wanted == true
      return
    }
    demandOutstanding = true
    var flags = FrameDemandFlag(0u)
    if (!force) flags = flags or FrameDemandFlag.IF_NEEDED
    if (presents) flags = flags or FrameDemandFlag.PRESENT
    session.requestFrame(FrameDemand(flags = flags, token = ++nextToken))
  }

  /**
   * Runs the session's work after its wakes: queued driver work, a due retry, and the frame
   * results. Reports whether a frame reached the window.
   */
  fun handleWakes(): Boolean {
    driver.service(session)
    val due = retryAtNanos
    if (due != null && System.nanoTime() >= due) {
      retryAtNanos = null
      requestFrame(force = true)
    }
    return frames.consume() && drainFrameResults()
  }

  /**
   * Drains every frame result and shows the newest rendered frame, reporting whether it reached the
   * window. A rendered frame that asks for another, as during a paint transition, demands it. A
   * target that was not ready, or a frame that missed the window, consumed its map update, so a
   * forced retry follows after about one refresh.
   */
  protected fun drainFrameResults(): Boolean {
    val batch =
      try {
        session.drainFrameResults()
      } catch (error: MaplibreException) {
        if (error.status == MaplibreStatus.NOT_READY) return false
        throw error
      }
    var rendered = false
    var retry = false
    var repaint = false
    batch.use { results ->
      for (index in 0uL until results.count()) {
        val result = results.get(index)
        demandOutstanding = false
        when (result.disposition) {
          RenderResult.RENDERED -> {
            rendered = true
            repaint = repaint || result.needsRepaint
          }
          RenderResult.TARGET_NOT_READY -> retry = true
        }
      }
    }
    val shown = rendered && present()
    if (rendered && !shown) retry = true
    if (retry) {
      retryAtNanos = System.nanoTime() + RETRY_DELAY_NANOS
    } else if (repaint) {
      requestFrame()
    }
    wanted?.let { force ->
      if (!demandOutstanding) {
        wanted = null
        requestFrame(force)
      }
    }
    return shown
  }

  /** Shows the frame the session rendered, and reports whether it reached the window. */
  protected open fun present(): Boolean = true

  /** Follows a resized host. The session resize carries the new extent to the map. */
  open fun resize(viewport: Viewport) {
    session.resize(extent(viewport)).reportFailure("render session resize")
  }

  /** Releases the acquired frames that the target holds. */
  protected open fun releaseFrames() {}

  /** Releases the host resources of the target's mode, after the session detached. */
  protected open fun closeHost() {}

  /** Waits for a session operation as the driver requires. */
  protected fun <T> await(completion: Deferred<T>): T = driver.await(session, completion)

  /**
   * Releases held frames, detaches, and destroys the session, then releases the mode's host
   * resources. A failed detach abandons the session, which ends its graphics calls at once.
   */
  override fun close() {
    if (closed) return
    closed = true
    try {
      releaseFrames()
      await(session.detach())
    } catch (error: RuntimeException) {
      System.err.println("render session detach failed, abandoning: ${error.message}")
      val abandoned = session.abandon()
      if (abandoned.quarantinedResourceCount > 0u) {
        System.err.println(
          "render session quarantined ${abandoned.quarantinedResourceCount} resources"
        )
      }
    } finally {
      session.close()
      closeHost()
    }
  }

  companion object {
    /** A session-owned texture ring deep enough to keep compositing while the map renders. */
    const val OWNED_TEXTURE_RING_DEPTH = 2u

    /** How long a frame that did not reach the window waits to retry, about one refresh. */
    const val RETRY_DELAY_NANOS = 16_000_000L

    /**
     * Selects the driver from the graphics API and the mode: a core worker wherever the target
     * accepts one. OpenGL on a WGL or EGL context requires the caller driver. A Vulkan core worker
     * in a texture mode needs a queue of its own, because the compositor submits to the host's.
     */
    fun driverFor(graphics: GraphicsContext, mode: RenderTargetMode): RenderDriverKind =
      when (graphics) {
        is MetalContext -> RenderDriverKind.CORE_WORKER
        is VulkanContext ->
          if (mode == RenderTargetMode.NATIVE_SURFACE || graphics.hasSessionQueue()) {
            RenderDriverKind.CORE_WORKER
          } else {
            RenderDriverKind.CALLER_GRAPHICS_THREAD
          }
        else -> RenderDriverKind.CALLER_GRAPHICS_THREAD
      }

    /** Attaches a render session for the active graphics API and mode, and awaits it. */
    fun attach(
      graphics: GraphicsContext,
      map: MapHandle,
      viewport: Viewport,
      mode: RenderTargetMode,
    ): RenderTarget {
      val driver = SessionDriver(driverFor(graphics, mode))
      return when (graphics) {
        is MetalContext -> MetalRenderTarget.attach(graphics, map, viewport, mode, driver)
        is VulkanContext -> VulkanRenderTarget.attach(graphics, map, viewport, mode, driver)
        is OpenGLContext -> OpenGLRenderTarget.attach(graphics, map, viewport, mode, driver)
        else -> error("Unsupported graphics context: ${graphics.backend()}")
      }
    }

    fun extent(viewport: Viewport): RenderTargetExtent =
      RenderTargetExtent(
        viewport.width().toUInt(),
        viewport.height().toUInt(),
        viewport.scaleFactor(),
      )
  }
}

/** A started attachment, awaited, with the driver and frame wake that it attached with. */
internal class AttachedSession(
  val session: RenderSessionHandle,
  val driver: SessionDriver,
  val frames: GlfwWake,
) {
  companion object {
    /**
     * Starts an attachment with [start] and awaits it. A failed attachment abandons its session.
     */
    fun attach(
      driver: SessionDriver,
      ringDepth: UInt = 0u,
      start: (RenderSessionAttachOptions) -> RenderSessionAttachment,
    ): AttachedSession {
      val frames = GlfwWake()
      val attachment = start(driver.attachOptions(frames, ringDepth))
      try {
        driver.await(attachment.session, attachment.ready)
      } catch (error: Throwable) {
        runCatching { attachment.session.abandon() }.onFailure(error::addSuppressed)
        runCatching { attachment.session.close() }.onFailure(error::addSuppressed)
        throw error
      }
      return AttachedSession(attachment.session, driver, frames)
    }
  }
}

/** A window surface, where the session presents each frame that it renders. */
internal class NativeSurfaceTarget(attached: AttachedSession) :
  RenderTarget(attached.session, attached.driver, attached.frames) {
  override val presents: Boolean = true
}

/**
 * A session-owned texture ring. After a rendered result, the target acquires every ready frame,
 * keeps the newest, and composes it. It holds that frame until a newer one replaces it, so the
 * session renders into the ring's other slots meanwhile.
 */
internal abstract class OwnedTextureTarget(attached: AttachedSession) :
  RenderTarget(attached.session, attached.driver, attached.frames) {
  private var held: AcquiredFrameHandle? = null

  /** Composes the frame into the window, and reports whether it reached the window. */
  protected abstract fun draw(frame: AcquiredFrameHandle): Boolean

  /** Resizes the swapchain and compositor resources. */
  protected abstract fun resizeHost(viewport: Viewport)

  override fun present(): Boolean {
    var newest: AcquiredFrameHandle? = null
    while (true) {
      val frame =
        try {
          session.acquireFrame()
        } catch (error: MaplibreException) {
          if (error.status == MaplibreStatus.NOT_READY) break
          throw error
        }
      // Nothing read an older frame, so it releases CPU-complete.
      newest?.release()
      newest = frame
    }
    if (newest == null) return held != null
    // The compositors finish their reads before they return, so a frame releases CPU-complete.
    val shown = draw(newest)
    held?.release()
    held = newest
    return shown
  }

  /** A session resize needs every frame released, so the held frame goes first. */
  override fun resize(viewport: Viewport) {
    releaseFrames()
    resizeHost(viewport)
    super.resize(viewport)
  }

  override fun releaseFrames() {
    held?.release()
    held = null
  }
}

/**
 * A caller-owned texture that the session renders into and the compositor samples. The texture
 * belongs to the session from a demand until its result, and to the compositor from a rendered
 * result until the draw returns, so the target keeps one demand outstanding.
 */
internal abstract class BorrowedTextureTarget<T : AutoCloseable>(
  attached: AttachedSession,
  private val map: MapHandle,
  protected var texture: T,
) : RenderTarget(attached.session, attached.driver, attached.frames) {
  override val exclusiveTexture: Boolean = true

  /** Allocates a texture at the viewport's physical size. */
  protected abstract fun allocate(viewport: Viewport): T

  /** Starts handing the session [replacement]. */
  protected abstract fun setTarget(viewport: Viewport, replacement: T): Deferred<Unit>

  /** Composes [texture] into the window, and reports whether it reached the window. */
  protected abstract fun draw(texture: T): Boolean

  /** Resizes the swapchain and compositor resources. */
  protected abstract fun resizeHost(viewport: Viewport)

  override fun present(): Boolean = draw(texture)

  /**
   * Replaces the texture, because its owner sets its size. The outgoing texture stays current until
   * the replacement completes, and the frames rendered before it are drawn from it. A replacement
   * that fails once started leaves it unknown which texture the session holds, so the session
   * detaches before either texture is released.
   */
  override fun resize(viewport: Viewport) {
    resizeHost(viewport)
    val replacement = allocate(viewport)
    val handover =
      try {
        setTarget(viewport, replacement)
      } catch (error: RuntimeException) {
        replacement.close()
        throw error
      }
    // A target replacement leaves the map's extent unchanged.
    map
      .resize(
        LogicalExtent(viewport.width().toUInt(), viewport.height().toUInt(), viewport.scaleFactor())
      )
      .reportFailure("map resize")
    try {
      await(handover)
    } catch (error: RuntimeException) {
      runCatching { close() }.onFailure(error::addSuppressed)
      replacement.close()
      throw error
    }
    drainFrameResults()
    texture.close()
    texture = replacement
    requestFrame(force = true)
  }
}

internal fun Deferred<*>.reportFailure(operation: String) {
  invokeOnCompletion { error -> if (error != null) System.err.println("$operation failed: $error") }
}
