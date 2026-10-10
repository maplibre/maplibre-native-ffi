package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.runBlocking
import org.lwjgl.glfw.GLFW.glfwPostEmptyEvent
import org.lwjgl.glfw.GLFW.glfwWaitEvents
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.generated.FrameDemand
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.QueueLock
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionAttachment
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.Wake
import org.maplibre.nativeffi.runtime.CommandCompletion

/**
 * Where a session's graphics work runs. A core worker runs it on its own thread. A caller driver
 * queues it for the GLFW thread, which services it after the driver-work wake. [queueLock] is the
 * host's lock on a queue that the session shares with it.
 */
internal class SessionDriver(
  val kind: RenderDriverKind,
  private val queueLock: QueueLock = QueueLock(),
) {
  private val caller = kind == RenderDriverKind.CALLER_GRAPHICS_THREAD

  /** The driver-work wake, which only a caller driver has. */
  private val work = if (caller) GlfwWake() else null

  val label: String
    get() = if (caller) "caller-graphics-thread" else "core-worker"

  fun attachOptions(frames: GlfwWake, ringDepth: UInt): RenderSessionAttachOptions =
    RenderSessionAttachOptions(
      driver = kind,
      requestedTextureRingDepth = ringDepth,
      frameWake = frames.wake,
      driverWorkWake = work?.wake ?: Wake(),
      queueLock = queueLock,
    )

  /** Runs every queued item after the driver-work wake. */
  fun service(session: RenderSessionHandle) {
    if (work?.consume() == true) session.serviceDriverWork(0uL)
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

  /** Set by a native surface, the only target whose demands ask the session to present. */
  protected open val presents: Boolean = false

  /** When a paced retry is due, as a [System.nanoTime] value, or null with none pending. */
  var retryAtNanos: Long? = null
    private set

  /** Demands a frame. A forced frame renders even when the map has no newer update. */
  fun requestFrame(force: Boolean = false) {
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
   * window. A rendered frame that asks for another, as during a paint transition, demands it.
   * Neither a target that was not ready nor a frame that missed the window causes a map-update
   * event, so a retry follows after about one refresh. The retry is forced, because a frame that
   * missed the window consumed its update.
   */
  protected fun drainFrameResults(): Boolean {
    val batch = session.drainFrameResults() ?: return false
    var rendered = false
    var retry = false
    var repaint = false
    batch.use { results ->
      for (result in results.get().results) {
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
    return shown
  }

  /** Shows the frame the session rendered, and reports whether it reached the window. */
  protected open fun present(): Boolean = true

  /** Follows a resized host. The session resize carries the new extent to the map. */
  open fun resize(viewport: Viewport) {
    submit("render session resize") { session.resize(extent(viewport)) }
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
      abandon(session)
    } finally {
      session.close()
      closeHost()
    }
  }

  companion object {
    /**
     * The depth of a texture ring, session-owned or borrowed: the target holds the newest frame
     * until a newer one arrives, and the session renders into the other slot meanwhile.
     */
    const val TEXTURE_RING_DEPTH = 2

    /** How long a frame that did not reach the window waits to retry, about one refresh. */
    const val RETRY_DELAY_NANOS = 16_000_000L

    /**
     * Whether an abandon kept graphics objects until the process exits. A kept Vulkan object is a
     * child of the host's device, and a kept swapchain of its surface, so a Vulkan host then keeps
     * those until the process exits too.
     */
    @Volatile
    var graphicsKept = false
      private set

    /** Ends the session's graphics work at once. */
    fun abandon(session: RenderSessionHandle) {
      val abandoned = session.abandon()
      if (abandoned.quarantinedResourceCount > 0u) {
        graphicsKept = true
        System.err.println(
          "render session abandon kept ${abandoned.quarantinedResourceCount} resource groups until exit"
        )
      }
    }

    /**
     * Selects the driver from the graphics API: a core worker wherever the target accepts one.
     * OpenGL on a WGL or EGL context requires the caller driver. A Vulkan core worker shares the
     * host's queue and takes the host's queue lock around each call on it.
     */
    fun driverFor(graphics: GraphicsContext): SessionDriver =
      when (graphics) {
        is MetalContext -> SessionDriver(RenderDriverKind.CORE_WORKER)
        is VulkanContext -> SessionDriver(RenderDriverKind.CORE_WORKER, graphics.queueLock())
        else -> SessionDriver(RenderDriverKind.CALLER_GRAPHICS_THREAD)
      }

    /** Attaches a render session for the active graphics API and mode, and awaits it. */
    fun attach(
      graphics: GraphicsContext,
      map: MapHandle,
      viewport: Viewport,
      mode: RenderTargetMode,
    ): RenderTarget {
      val driver = driverFor(graphics)
      return when (graphics) {
        is MetalContext -> MetalRenderTarget.attach(graphics, map, viewport, mode, driver)
        is VulkanContext -> VulkanRenderTarget.attach(graphics, map, viewport, mode, driver)
        is OpenGLContext -> OpenGLRenderTarget.attach(graphics, map, viewport, mode, driver)
        else -> error("Unsupported graphics context: ${graphics.backend()}")
      }
    }

    fun extent(viewport: Viewport): LogicalExtent =
      LogicalExtent(viewport.width().toUInt(), viewport.height().toUInt(), viewport.scaleFactor())
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
        runCatching { RenderTarget.abandon(attachment.session) }.onFailure(error::addSuppressed)
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
 * A texture ring whose frames the target acquires and composes into the window. After a rendered
 * result, the target acquires every ready frame, keeps the newest, and composes it. It holds that
 * frame until a newer one replaces it, so the session renders into the ring's other slot meanwhile.
 */
internal abstract class TextureTarget(attached: AttachedSession) :
  RenderTarget(attached.session, attached.driver, attached.frames) {
  private var held: AcquiredFrameHandle? = null

  /** Composes the frame into the window, and reports whether it reached the window. */
  protected abstract fun draw(frame: AcquiredFrameHandle): Boolean

  /** Resizes the swapchain and compositor resources. */
  protected abstract fun resizeHost(viewport: Viewport)

  override fun present(): Boolean {
    var newest: AcquiredFrameHandle? = null
    while (true) {
      val frame = session.acquireFrame() ?: break
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

  override fun releaseFrames() {
    held?.release()
    held = null
  }
}

/** A session-owned texture ring, which the session sizes and allocates. */
internal abstract class OwnedTextureTarget(attached: AttachedSession) : TextureTarget(attached) {
  /** A session resize needs every frame released, so the held frame goes first. */
  override fun resize(viewport: Viewport) {
    releaseFrames()
    resizeHost(viewport)
    super.resize(viewport)
  }
}

/**
 * A ring of caller-owned textures that the session renders into. The host allocates the ring and
 * sizes it, so a resize hands the session a new ring.
 */
internal abstract class BorrowedTextureTarget<T : AutoCloseable>(
  attached: AttachedSession,
  private val map: MapHandle,
  protected var ring: List<T>,
) : TextureTarget(attached) {
  /** Allocates a texture at the viewport's physical size. */
  protected abstract fun allocate(viewport: Viewport): T

  /** Starts handing the session [replacement]. */
  protected abstract fun setTarget(viewport: Viewport, replacement: List<T>): Deferred<Unit>

  /**
   * Replaces the ring, because its owner sets its size. A replacement is refused while the host
   * holds a frame, so the held one goes first, and the window keeps what it last presented. The
   * outgoing ring stays alive until the replacement completes. A replacement that fails once
   * started leaves it unknown which ring the session holds, so the session detaches before either
   * ring is released.
   */
  override fun resize(viewport: Viewport) {
    releaseFrames()
    resizeHost(viewport)
    val replacement = allocateRing(viewport, ::allocate)
    val handover =
      try {
        setTarget(viewport, replacement)
      } catch (error: RuntimeException) {
        replacement.forEach(AutoCloseable::close)
        throw error
      }
    // A target replacement leaves the map's extent unchanged.
    submit("map resize") { map.resize(extent(viewport)) }
    try {
      await(handover)
    } catch (error: RuntimeException) {
      runCatching { close() }.onFailure(error::addSuppressed)
      replacement.forEach(AutoCloseable::close)
      throw error
    }
    ring.forEach(AutoCloseable::close)
    ring = replacement
    // A replacement publishes no map update, and a frame rendered before it can no longer be
    // acquired, so the new ring needs a forced frame.
    requestFrame(force = true)
  }

  override fun closeHost() {
    ring.forEach(AutoCloseable::close)
  }

  companion object {
    /** Allocates a ring of [RenderTarget.TEXTURE_RING_DEPTH] textures with [allocate]. */
    fun <T : AutoCloseable> allocateRing(viewport: Viewport, allocate: (Viewport) -> T): List<T> {
      val ring = ArrayList<T>(TEXTURE_RING_DEPTH)
      try {
        repeat(TEXTURE_RING_DEPTH) { ring.add(allocate(viewport)) }
      } catch (error: RuntimeException) {
        ring.forEach(AutoCloseable::close)
        throw error
      }
      return ring
    }
  }
}

/**
 * Submits a command without waiting on it. A rejection or a terminal failure is printed, so that
 * one bad input or resize does not escape a GLFW callback or the event loop.
 */
internal inline fun submit(operation: String, command: () -> Deferred<CommandCompletion>) {
  try {
    command().reportFailure(operation)
  } catch (error: MaplibreException) {
    System.err.println("$operation rejected: $error")
  }
}

/**
 * Prints the command's failure once it completes: an error, or a FAILED terminal disposition. A
 * superseded or cancelled command ended without failing, so it prints nothing.
 */
@OptIn(ExperimentalCoroutinesApi::class)
internal fun Deferred<CommandCompletion>.reportFailure(operation: String) {
  invokeOnCompletion { error ->
    if (error != null) {
      System.err.println("$operation failed: $error")
      return@invokeOnCompletion
    }
    val completion = getCompleted()
    if (completion.disposition == CommandDisposition.FAILED) {
      System.err.println("$operation failed: ${completion.status}: ${completion.diagnostic}")
    }
  }
}
