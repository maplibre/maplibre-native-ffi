// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlin.jvm.JvmName
import org.maplibre.nativeffi.internal.JvmSynthetic
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A rendered frame that the caller holds to read or show.
 *
 * See `mln_acquired_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class AcquiredFrameHandle
private constructor(
  handle: Long,
  @Suppress("unused") private val parent: RenderSessionHandle,
) {
  @get:JvmSynthetic
  internal val binding =
    HandleState("AcquiredFrameHandle", handle, GeneratedOwnerDisposal::acquiredFrame)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after release. */
  @get:JvmName("getId")
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /**
   * Gives up an acquired frame without saying when your GPU stops reading it.
   *
   * Because your GPU may still read the frame's texture, the session never renders into that
   * texture again, even when `RenderSessionHandle.setTarget()` gives it the texture again, until
   * `RenderSessionHandle.releaseTarget()` releases the ring. A view that is open on the frame keeps
   * the frame's sync objects in place until it ends.
   *
   * See `mln_acquired_frame_discard` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun discard(): Unit =
    nativeClose(this, binding) { check(C.mln_acquired_frame_discard(handle, diagnostic)) }

  /**
   * Gets the OpenGL texture of an acquired frame.
   *
   * It gets a borrowed ring's texture, or an exported ring's on an OpenGL device: the texture of
   * the dmabuf that `AcquiredFrameHandle.withDmaBuf()` gets, and the frame's depth texture with
   * `RenderSessionAttachOptions.keepDepth`, in the device's share group. An exported ring's
   * textures hold the frame upside down from OpenGL's rows, top row first, as a dmabuf's readers
   * take it, so negate clip y to draw into them. Wait for `AcquiredFrameHandle.withProducerSync()`
   * before you use them.
   *
   * See `mln_acquired_frame_get_open_gl_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
   */
  public fun <T> withOpenGlTexture(block: (OpenGlTextureFrame) -> T): T =
    nativeCall(this, binding) {
      borrowView(
        { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(w(48, 48, 40), w(8, 8, 4))
        check(C.mln_acquired_frame_get_open_gl_texture(handle, out, diagnostic))
        block(readOpenGlTextureFrame(out, scope))
      }
    }

  /**
   * Reads what to wait for before you read an acquired frame's texture.
   *
   * See `mln_acquired_frame_get_producer_sync` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
   */
  public fun <T> withProducerSync(block: (GpuSync) -> T): T =
    nativeCall(this, binding) {
      borrowView(
        { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(24, w(8, 8, 4))
        check(C.mln_acquired_frame_get_producer_sync(handle, out, diagnostic))
        block(readGpuSync(out, scope))
      }
    }

  /**
   * Reads the frame result of an acquired frame.
   *
   * It succeeds until you release or discard the frame, whatever the session's state.
   *
   * See `mln_acquired_frame_get_result` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun getResult(): RenderFrameResult =
    nativeCall(this, binding) {
      val out = sized(w(432, 432, 404), w(8, 8, 4))
      check(C.mln_acquired_frame_get_result(handle, out, diagnostic))
      readRenderFrameResult(out)
    }

  /**
   * Gives an acquired frame back to the session once you're done reading it.
   *
   * `consumerCompletion` says when your GPU work on the texture, and on its depth texture,
   * finishes. Pass `GpuSyncKind.CPU_COMPLETE` when that work has already finished. The session
   * waits for that point before it renders into the textures again. If that wait fails, such as for
   * an OpenGL fence outside the session's share group, the session fails.
   *
   * See `mln_acquired_frame_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.UNSUPPORTED` when `RenderSessionCapabilities.consumerSyncKinds` lacks the kind of
   *   `consumerCompletion`.
   */
  public fun release(consumerCompletion: GpuSync): Unit =
    nativeClose(this, binding) {
      check(C.mln_acquired_frame_release(handle, writeGpuSync(consumerCompletion), diagnostic))
    }

  override fun toString(): String =
    "AcquiredFrameHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  internal companion object {
    /** Creates the owner of `handle`. */
    @JvmSynthetic
    internal fun wrap(handle: Long, parent: RenderSessionHandle): AcquiredFrameHandle =
      AcquiredFrameHandle(handle, parent)
  }
}
