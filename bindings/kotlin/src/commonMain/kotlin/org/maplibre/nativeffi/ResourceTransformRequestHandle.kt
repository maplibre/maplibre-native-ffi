// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A URL rewrite that a resource transform answers with the URL to load.
 *
 * See `mln_resource_transform_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class ResourceTransformRequestHandle
internal constructor(
  handle: Long,
) : AutoCloseable {
  internal val binding =
    HandleState(
      "ResourceTransformRequestHandle",
      handle,
      GeneratedOwnerDisposal::resourceTransformRequest,
    )
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after release. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /** Calls [release], for the standard library's `use`. */
  public override fun close() {
    release()
  }

  /**
   * Answers a resource transform request with the URL to load instead.
   *
   * `url` must not be empty. A failed answer fails the request with `ResourceErrorReason.OTHER`, as
   * `ResourceTransformRequestHandle.release()` does.
   *
   * See `mln_resource_transform_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun complete(url: String): Unit =
    nativeAnswer(this, binding) {
      check(C.mln_resource_transform_request_complete(handle, view(url), diagnostic))
    }

  /**
   * Fails a resource transform request without sending it.
   *
   * The library retries a request that failed for a reason it retries, such as
   * `ResourceErrorReason.CONNECTION`, and the retry calls the transform again. When this call
   * fails, the request fails with `ResourceErrorReason.OTHER`, as
   * `ResourceTransformRequestHandle.release()` does.
   *
   * See `mln_resource_transform_request_fail` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun fail(error: ResourceTransformError): Unit =
    nativeAnswer(this, binding) {
      check(
        C.mln_resource_transform_request_fail(
          handle,
          writeResourceTransformError(error),
          diagnostic,
        ),
      )
    }

  /**
   * Releases a resource transform request.
   *
   * Releasing a request without an answer fails it with `ResourceErrorReason.OTHER`.
   *
   * See `mln_resource_transform_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun release(): Unit =
    nativeClose(this, binding) { C.mln_resource_transform_request_release(handle) }

  override fun toString(): String =
    "ResourceTransformRequestHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
