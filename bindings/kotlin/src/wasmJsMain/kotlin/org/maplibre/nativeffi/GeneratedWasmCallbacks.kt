// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.delivery.HostDelivery
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A log callback, for `setLogCallback()`. This is the Kotlin/Wasm queue form: the page receives
 * each call after native has answered it. The JVM, Android, and Kotlin/Native take `LogHandler`,
 * whose callback computes the answer.
 *
 * Runs on the thread that logged the message, or on a background thread for the severities that
 * `setLogAsyncSeverityMask()` selects. The callback can't call the library.
 *
 * See `mln_log_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public data class QueuedLogHandler(
  public val callback: QueuedLogCallback,
)

/**
 * Receives a log message. The page receives the call after native has answered it, so the callback
 * returns nothing, and native answers `true` as it does for a queue.
 *
 * Return true if you handled the message, or false to also send it to the platform's log.
 *
 * The page calls it when it dispatches the binding's delivery queue.
 *
 * If it throws, the binding reports it as a `CallbackException`.
 *
 * See `mln_log_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun interface QueuedLogCallback {
  public operator fun invoke(severity: LogSeverity, event: LogEvent, code: Long, message: String)
}

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

internal fun NativeCall.putQueuedLogHandler(target: Long, value: QueuedLogHandler) {
  writeU32(target, w(24, 40).toUInt())
  writeAddress(target + w(8, 16), registrations.register(value))
  writeAddress(target + w(4, 8), UpcallStubs.queuedLogHandlerCallback)
  writeAddress(target + w(12, 24), UpcallStubs.releaseRoot)
  writeI64(target + w(16, 32), HostDelivery.queue)
}

internal fun NativeCall.writeQueuedLogHandler(value: QueuedLogHandler): Long =
  allocate(w(24, 40), w(8, 8, 4)).also { putQueuedLogHandler(it, value) }

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
public fun setLogCallback(handler: QueuedLogHandler): Unit =
  nativeCall(null, null) {
    check(C.mln_log_set_callback(writeQueuedLogHandler(handler), diagnostic))
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
  transform: HttpHeaderTransformRules,
): Deferred<CommandCompletion> =
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
