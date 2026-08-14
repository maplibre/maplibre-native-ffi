package org.maplibre.nativeffi.render

import kotlin.jvm.JvmInline
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.runtime.OperationHandle

/** Execution placement for a render session. Unknown native values remain representable. */
@JvmInline
public value class RenderDriver public constructor(public val nativeValue: Int) {
  public companion object {
    public val CORE_WORKER: RenderDriver = RenderDriver(1)
    public val CALLER_GRAPHICS_THREAD: RenderDriver = RenderDriver(2)

    internal fun fromNative(value: Int): RenderDriver = RenderDriver(value)
  }
}

/** Attachment policy. The native notification endpoints inherit the map runtime's source. */
public data class RenderSessionAttachOptions(
  public val driver: RenderDriver = RenderDriver.CORE_WORKER,
  public val requestedTextureRingDepth: Int = 0,
) {
  init {
    Status.requireArgument(requestedTextureRingDepth in 0..3) {
      "requestedTextureRingDepth must be between zero and three"
    }
  }
}

/** A session and the operation that completes its asynchronous attachment. */
public data class RenderSessionAttachment(
  public val session: RenderSessionHandle,
  public val operation: OperationHandle<Unit>,
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

public data class FrameDemand(
  public val token: ULong = 0u,
  public val coalescingBoundary: ULong = 0u,
  public val presentationTimeNanoseconds: Long = 0,
  public val deadlineNanoseconds: Long = 0,
  public val ifNeeded: Boolean = true,
  public val present: Boolean = false,
) {
  init {
    Status.requireArgument(deadlineNanoseconds >= 0) { "deadlineNanoseconds must be non-negative" }
  }
}

public data class RenderFrameResult(
  public val disposition: RenderResult,
  public val token: ULong,
  public val mapUpdateGeneration: ULong,
  public val extentGeneration: ULong,
  public val frameGeneration: ULong,
  public val presentationTimeNanoseconds: Long,
)

public data class RenderSessionSnapshot(
  public val state: RenderSessionState,
  public val driver: RenderDriver,
  public val latestResult: RenderResult,
  public val extent: RenderTargetExtent,
  public val generation: ULong,
  public val mapUpdateGeneration: ULong,
  public val renderedUpdateGeneration: ULong,
  public val extentGeneration: ULong,
  public val frameGeneration: ULong,
  public val latestDemandToken: ULong,
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

/** Native synchronization payload. The object value follows the backend's C ownership rules. */
public data class GpuSync(
  public val kind: GpuSyncKind = GpuSyncKind.CPU_COMPLETE,
  public val objectHandle: ULong = 0u,
  public val value: ULong = 0u,
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

public data class TextureReadback(public val bytes: ByteArray, public val info: TextureImageInfo)
