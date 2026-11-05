// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Headers to add to HTTP requests, for `RuntimeHandle.setHttpHeaderTransform()`. This is the
 * Kotlin/Wasm form, which holds only native header rules, because the page cannot run Kotlin while
 * native waits for an answer. The JVM, Android, and Kotlin/Native take `HttpHeaderTransform`.
 *
 * Set either `callback` or `rules`, not both.
 *
 * Runs on a library thread.
 *
 * See `mln_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public data class HttpHeaderTransformRules(
  /** The first rule that matches a request adds its headers, and no other. */
  public val rules: List<HttpHeaderRule> = emptyList(),
)

internal fun NativeCall.putHttpHeaderTransformRules(target: Long, value: HttpHeaderTransformRules) {
  writeU32(target, w(24, 48).toUInt())
  writeAddress(
    target + w(16, 32),
    array(value.rules, w(28, 48), w(4, 8)) { at, item -> putHttpHeaderRule(at, item) },
  )
  writeSize(target + w(20, 40), value.rules.size.toULong())
}

internal fun NativeCall.writeHttpHeaderTransformRules(value: HttpHeaderTransformRules): Long =
  allocate(w(24, 48), w(4, 8)).also { putHttpHeaderTransformRules(it, value) }

/**
 * Adds headers to the runtime's HTTP requests.
 *
 * The transform replaces the one set before. In a browser or on OpenHarmony, serve the requests
 * with `RuntimeHandle.setResourceProvider()` instead.
 *
 * Runs on a library thread.
 *
 * See `mln_runtime_set_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when the library runs in a browser or on OpenHarmony.
 */
public fun RuntimeHandle.setHttpHeaderTransform(
  transform: HttpHeaderTransformRules,
): Completion<CommandCompletion> =
  nativeCommand(this, binding, bindingCallbacks) {
    check(
      C.mln_runtime_set_http_header_transform(
        handle,
        writeHttpHeaderTransformRules(transform),
        completion,
        diagnostic,
      ),
    )
  }

/** [RuntimeHandle.setHttpHeaderTransform] without its completion. */
public fun RuntimeDiscarding.setHttpHeaderTransform(transform: HttpHeaderTransformRules): Unit =
  nativeDiscard(owner, binding, bindingCallbacks) {
    check(
      C.mln_runtime_set_http_header_transform(
        handle,
        writeHttpHeaderTransformRules(transform),
        completion,
        diagnostic,
      ),
    )
  }
