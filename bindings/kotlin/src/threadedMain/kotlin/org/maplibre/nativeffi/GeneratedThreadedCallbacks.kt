// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A log callback, for `setLogCallback()`. Kotlin/Wasm takes `QueuedLogHandler` in its place.
 *
 * Runs on the thread that logged the message, or on a background thread for the severities that
 * `setLogAsyncSeverityMask()` selects. The callback can't call the library.
 *
 * See `mln_log_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public data class LogHandler(
  public val callback: LogCallback,
)

/**
 * Receives a log message. Kotlin/Wasm takes `QueuedLogCallback` in its place.
 *
 * Native code calls it on one of its own threads and waits for it to return.
 *
 * If it throws, native receives `false`, and the binding reports it as a `CallbackException`.
 *
 * See `mln_log_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun interface LogCallback {
  public operator fun invoke(
    severity: LogSeverity,
    event: LogEvent,
    code: Long,
    message: String,
  ): Boolean
}

/**
 * Headers to add to HTTP requests, for `RuntimeHandle.setHttpHeaderTransform()`. Kotlin/Wasm takes
 * `HttpHeaderTransformRules` in its place.
 *
 * Set either `callback` or `rules`, not both.
 *
 * Runs on a library thread.
 *
 * See `mln_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public data class HttpHeaderTransform(
  /** The first rule that matches a request adds its headers, and no other. */
  public val rules: List<HttpHeaderRule> = emptyList(),
  public val callback: HttpHeaderTransformCallback? = null,
)

/**
 * Sets the headers of an HTTP request before the library sends it.
 *
 * `url` is the request's URL after any resource transform. The request waits for the callback, so
 * return quickly.
 *
 * Native code calls it on one of its own threads and waits for it to return.
 *
 * It returns normally to succeed and throws a `MaplibreException` that carries a failure status to
 * fail, and native receives that status. If it throws anything else, native receives
 * `Status.NATIVE_ERROR`, and the binding reports it as a `CallbackException`.
 *
 * See `mln_http_header_transform_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun interface HttpHeaderTransformCallback {
  public operator fun invoke(
    kind: ResourceKind,
    url: String,
    outResponse: HttpHeaderTransformResponse,
  )
}

/**
 * The headers that an HTTP header transform callback sets.
 *
 * See `mln_http_header_transform_response` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public class HttpHeaderTransformResponse
internal constructor(
  internal val bindingAddress: Long,
  internal val bindingScope: org.maplibre.nativeffi.internal.callback.CallbackScope,
)

internal fun NativeCall.putLogHandler(target: Long, value: LogHandler) {
  writeU32(target, w(24, 40).toUInt())
  writeAddress(target + w(8, 16), registrations.register(value))
  writeAddress(target + w(4, 8), UpcallStubs.logHandlerCallback)
  writeAddress(target + w(12, 24), UpcallStubs.releaseRoot)
}

internal fun NativeCall.writeLogHandler(value: LogHandler): Long =
  allocate(w(24, 40), w(8, 8, 4)).also { putLogHandler(it, value) }

internal fun NativeCall.putHttpHeaderTransform(target: Long, value: HttpHeaderTransform) {
  writeU32(target, w(24, 48).toUInt())
  writeAddress(
    target + w(16, 32),
    array(value.rules, w(28, 48), w(4, 8)) { at, item -> putHttpHeaderRule(at, item) },
  )
  writeSize(target + w(20, 40), value.rules.size.toULong())
  if (value.callback == null) return
  writeAddress(target + w(8, 16), registrations.register(value))
  writeAddress(target + w(4, 8), UpcallStubs.httpHeaderTransformCallback)
  writeAddress(target + w(12, 24), UpcallStubs.releaseRoot)
}

internal fun NativeCall.writeHttpHeaderTransform(value: HttpHeaderTransform): Long =
  allocate(w(24, 48), w(4, 8)).also { putHttpHeaderTransform(it, value) }

/**
 * Sets a header on the request that an HTTP header transform callback handles.
 *
 * Setting a header again, in any letter case, replaces its value. `name` can't be a header that the
 * library or the platform manages, such as `Host`, `Range`, `Accept-Encoding`, or `User-Agent`.
 *
 * See `mln_http_header_transform_response_set_header` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun HttpHeaderTransformResponse.setHeader(name: String, value: String): Unit =
  nativeRespond(bindingScope, bindingAddress) {
    check(
      C.mln_http_header_transform_response_set_header(handle, view(name), view(value), diagnostic),
    )
  }

/**
 * Sends the log messages of every runtime in the process to a callback.
 *
 * The callback replaces the one set before.
 *
 * Runs on the thread that logged the message, or on a background thread for the severities that
 * `setLogAsyncSeverityMask()` selects. The callback can't call the library.
 *
 * See `mln_log_set_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun setLogCallback(handler: LogHandler): Unit =
  nativeCall(null, null) {
    check(C.mln_log_set_callback(writeLogHandler(handler), diagnostic))
    accept(CallbackOwner.global)
  }

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
  transform: HttpHeaderTransform,
): Deferred<CommandCompletion> =
  nativeCommand(this, binding, bindingCallbacks) {
    check(
      C.mln_runtime_set_http_header_transform(
        handle,
        writeHttpHeaderTransform(transform),
        completion,
        diagnostic,
      ),
    )
  }

/** [RuntimeHandle.setHttpHeaderTransform] without its completion. */
public fun RuntimeDiscarding.setHttpHeaderTransform(transform: HttpHeaderTransform): Unit =
  nativeDiscard(owner, binding, bindingCallbacks) {
    check(
      C.mln_runtime_set_http_header_transform(
        handle,
        writeHttpHeaderTransform(transform),
        completion,
        diagnostic,
      ),
    )
  }
