// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.delivery.HostDelivery
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Process-global log callback or queue state. This is the Kotlin/Wasm queue form: the page receives
 * each call after native has answered it. The JVM, Android, and Kotlin/Native take `LogHandler`,
 * whose callback computes the answer.
 *
 * See `mln_log_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public data class QueuedLogHandler(
  /** Receives each record. */
  public val callback: QueuedLogCallback,
)

/**
 * Receives a MapLibre Native log record. The page receives the call after native has answered it,
 * so the callback returns nothing, and native answers `1` as it does for a queue.
 *
 * The page calls it when it drains the binding's delivery queue.
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
 * HTTP header transform state: a callback, or native header rules. This is the Kotlin/Wasm form,
 * which holds only native header rules, because the page cannot run Kotlin while native waits for
 * an answer. The JVM, Android, and Kotlin/Native take `HttpHeaderTransform`.
 *
 * See `mln_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public data class HttpHeaderTransformRules(
  /** Header rules for a transform without a callback; empty with one. */
  public val rules: List<HttpHeaderRule> = emptyList(),
)

internal fun NativeCall.putQueuedLogHandler(target: Long, value: QueuedLogHandler) {
  writeU32(target, w(24, 40).toUInt())
  writeAddress(target + w(8, 16), registrations.register(value))
  writeI64(target + w(16, 32), HostDelivery.queue)
}

internal fun NativeCall.writeQueuedLogHandler(value: QueuedLogHandler): Long =
  allocate(w(24, 40), 8).also { putQueuedLogHandler(it, value) }

internal fun NativeCall.putHttpHeaderTransformRules(target: Long, value: HttpHeaderTransformRules) {
  writeU32(target, w(24, 48).toUInt())
  writeAddress(
    target + w(16, 32),
    array(value.rules, w(24, 40), w(4, 8)) { at, item -> putHttpHeaderRule(at, item) },
  )
  writeSize(target + w(20, 40), value.rules.size.toULong())
}

internal fun NativeCall.writeHttpHeaderTransformRules(value: HttpHeaderTransformRules): Long =
  allocate(w(24, 48), w(4, 8)).also { putHttpHeaderTransformRules(it, value) }

/**
 * Installs a process-global MapLibre Native log callback.
 *
 * See `mln_log_set_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun GeneratedApi.logSetCallback(handler: QueuedLogHandler): Unit =
  nativeCall(null, null) {
    check(C.mln_log_set_callback(writeQueuedLogHandler(handler), diagnostic))
    accept(CallbackOwner.global)
  }

/**
 * Registers or replaces the runtime-scoped outgoing HTTP header transform.
 *
 * See `mln_runtime_set_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun GeneratedRuntimeOperations.setHttpHeaderTransform(
  transform: HttpHeaderTransformRules,
): Deferred<Unit> =
  nativeUnit(this, binding, bindingCallbacks) {
    check(
      C.mln_runtime_set_http_header_transform(
        handle,
        writeHttpHeaderTransformRules(transform),
        completion,
        diagnostic,
      ),
    )
  }
