// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedResourceTransformRequestOperations internal constructor() {
  internal abstract val binding: HandleState

  /**
   * Answers a resource transform request with a replacement URL and consumes its handle.
   *
   * See `mln_resource_transform_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun complete(url: String): Unit =
    nativeAnswer(this, binding) {
      check(C.mln_resource_transform_request_complete(handle, view(url), diagnostic))
    }

  /**
   * Releases a resource transform request without answering it.
   *
   * See `mln_resource_transform_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun release(): Unit =
    nativeClose(this, binding) { C.mln_resource_transform_request_release(handle) }
}
