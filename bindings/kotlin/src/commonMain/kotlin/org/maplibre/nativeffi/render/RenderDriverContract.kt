package org.maplibre.nativeffi.render

import kotlin.jvm.JvmInline
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.status.Status

/** Execution placement for a render session. Unknown native values remain representable. */
@JvmInline
public value class RenderDriver public constructor(public val nativeValue: Int) {
  public companion object {
    public val CORE_WORKER: RenderDriver = RenderDriver(1)
    public val CALLER_GRAPHICS_THREAD: RenderDriver = RenderDriver(2)

    internal fun fromNative(value: Int): RenderDriver = RenderDriver(value)
  }
}

/**
 * Attachment policy for a render session.
 *
 * [requestedTextureRingDepth] is a hint. Zero asks for the target's default depth, and native
 * clamps a larger request to the depth the target supports.
 */
public data class RenderSessionAttachOptions(
  public val driver: RenderDriver = RenderDriver.CORE_WORKER,
  public val requestedTextureRingDepth: Int = 0,
) {
  init {
    Status.requireArgument(requestedTextureRingDepth >= 0) {
      "requestedTextureRingDepth must not be negative"
    }
  }
}

/** A session and the deferred result of its asynchronous attachment. */
public data class RenderSessionAttachment(
  public val session: RenderSessionHandle,
  public val completed: Deferred<Unit>,
)

public data class RenderSessionCapabilities(
  public val driver: RenderDriver,
  public val textureRingDepth: Int,
  public val frameAcquisition: Boolean,
  public val readback: Boolean,
  public val consumerSync: Boolean,
  public val presentation: Boolean,
)

@JvmInline
public value class RenderSessionState public constructor(public val nativeValue: Int) {
  public companion object {
    public val ATTACHING: RenderSessionState = RenderSessionState(1)
    public val ATTACHED: RenderSessionState = RenderSessionState(2)
    public val DETACHING: RenderSessionState = RenderSessionState(3)
    public val DETACHED: RenderSessionState = RenderSessionState(4)
    public val TARGET_LOST: RenderSessionState = RenderSessionState(5)
    public val ABANDONED: RenderSessionState = RenderSessionState(6)

    internal fun fromNative(value: Int): RenderSessionState = RenderSessionState(value)
  }
}

/**
 * One frame demand. Every `uint64_t` field is preserved as a [Long] bit pattern; format through
 * `toULong()`.
 */
public data class FrameDemand(
  public val token: Long = 0L,
  public val coalescingBoundary: Long = 0L,
  public val timeoutNanoseconds: Long = 0L,
  /** Skips rendering when the map published nothing new, reporting `NO_UPDATE` instead. */
  public val ifNeeded: Boolean = true,
  /**
   * Presents the rendered frame on a presenting target. A demand that clears it still renders, and
   * the target keeps whatever it presented last.
   */
  public val present: Boolean = false,
)

/**
 * One frame outcome. Every `uint64_t` field is preserved as a [Long] bit pattern; format through
 * `toULong()`.
 */
public data class RenderFrameResult(
  public val disposition: RenderResult,
  public val token: Long,
  public val mapUpdateGeneration: Long,
  public val extentGeneration: Long,
  public val frameGeneration: Long,
  /**
   * Whether the map asked for another frame while it rendered this one, as during an ongoing camera
   * transition. Set only when [disposition] is [RenderResult.RENDERED], and false for every other
   * outcome, so a host can re-arm its frame loop without the runtime event round trip.
   */
  public val needsRepaint: Boolean,
)

/**
 * One published render-session generation. Every `uint64_t` field is preserved as a [Long] bit
 * pattern; format through `toULong()`.
 */
public data class RenderSessionSnapshot(
  public val state: RenderSessionState,
  public val driver: RenderDriver,
  public val latestResult: RenderResult,
  public val extent: RenderTargetExtent,
  public val generation: Long,
  public val mapUpdateGeneration: Long,
  public val renderedUpdateGeneration: Long,
  public val extentGeneration: Long,
  public val frameGeneration: Long,
  public val latestDemandToken: Long,
  public val pendingDemandCount: Int,
  public val acquiredFrameCount: Int,
  public val targetReady: Boolean,
  public val pendingChanges: Boolean,
)

@JvmInline
public value class GpuSyncKind public constructor(public val nativeValue: Int) {
  public companion object {
    public val CPU_COMPLETE: GpuSyncKind = GpuSyncKind(0)
    public val METAL_SHARED_EVENT: GpuSyncKind = GpuSyncKind(1)
    public val VULKAN_TIMELINE_SEMAPHORE: GpuSyncKind = GpuSyncKind(2)
    public val OPENGL_FENCE: GpuSyncKind = GpuSyncKind(3)
    public val WEBGPU_TOKEN: GpuSyncKind = GpuSyncKind(4)

    internal fun fromNative(value: Int): GpuSyncKind = GpuSyncKind(value)
  }
}

/**
 * Native synchronization payload. The object value follows the backend's C ownership rules.
 *
 * @property objectHandle Bit pattern of the backend object that [kind] names: the
 *   `id<MTLSharedEvent>` pointer, the `VkSemaphore` handle, the `GLsync` pointer, or the WebGPU
 *   token. Zero when [kind] is [GpuSyncKind.CPU_COMPLETE].
 * @property value Native `uint64_t` timeline value preserved as a [Long] bit pattern.
 */
public data class GpuSync(
  public val kind: GpuSyncKind = GpuSyncKind.CPU_COMPLETE,
  public val objectHandle: Long = 0L,
  public val value: Long = 0L,
)

@JvmInline
public value class RenderAbandonDisposition public constructor(public val nativeValue: Int) {
  public companion object {
    public val CLEAN: RenderAbandonDisposition = RenderAbandonDisposition(0)
    public val QUARANTINED: RenderAbandonDisposition = RenderAbandonDisposition(1)

    internal fun fromNative(value: Int): RenderAbandonDisposition = RenderAbandonDisposition(value)
  }
}

public data class RenderAbandonResult(
  public val disposition: RenderAbandonDisposition,
  public val quarantinedResourceCount: Int,
)

/** Copied readback pixels and the image metadata that describes them. */
public class TextureReadback(public val bytes: ByteArray, public val info: TextureImageInfo) {
  override fun equals(other: Any?): Boolean =
    other is TextureReadback && bytes.contentEquals(other.bytes) && info == other.info

  override fun hashCode(): Int = 31 * bytes.contentHashCode() + info.hashCode()

  override fun toString(): String = "TextureReadback(bytes=${bytes.size} bytes, info=$info)"
}
