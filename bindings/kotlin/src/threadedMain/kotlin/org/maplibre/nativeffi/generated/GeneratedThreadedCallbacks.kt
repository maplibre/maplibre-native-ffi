// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public class HttpHeaderTransformResponse
internal constructor(
  internal val bindingAddress: Long,
  internal val bindingScope: org.maplibre.nativeffi.internal.callback.CallbackScope,
)

/**
 * Process-global log callback or queue state. Kotlin/Wasm takes `QueuedLogHandler` in its place.
 *
 * See `mln_log_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public data class LogHandler(
  /** Receives each record. */
  public val callback: LogCallback,
)

/**
 * Receives a MapLibre Native log record. Kotlin/Wasm takes `QueuedLogCallback` in its place.
 *
 * See `mln_log_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public typealias LogCallback =
  (severity: LogSeverity, event: LogEvent, code: Long, message: String) -> UInt

/**
 * HTTP header transform state: a callback, or native header rules. Kotlin/Wasm takes
 * `HttpHeaderTransformRules` in its place.
 *
 * See `mln_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public data class HttpHeaderTransform(
  /** The callback, or null to apply rules. */
  public val callback: HttpHeaderTransformCallback? = null,
  /** Header rules for a transform without a callback; empty with one. */
  public val rules: List<HttpHeaderRule> = emptyList(),
)

/**
 * Adds end-to-end headers to one outgoing HTTP request attempt.
 *
 * See `mln_http_header_transform_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public typealias HttpHeaderTransformCallback =
  (kind: ResourceKind, url: String, outResponse: HttpHeaderTransformResponse) -> Unit

/**
 * Resource provider state. Kotlin/Wasm takes `QueuedResourceProvider` in its place.
 *
 * See `mln_resource_provider` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public data class ResourceProvider(
  /** Decides each request that the routes leave to the provider. */
  public val callback: ResourceProviderCallback,
  /** The requests that reach the provider; empty to claim every request. */
  public val routes: List<ResourceRoute> = emptyList(),
)

/**
 * Intercepts a network resource request. Kotlin/Wasm takes `QueuedResourceProviderCallback` in its
 * place.
 *
 * See `mln_resource_provider_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public typealias ResourceProviderCallback =
  (request: ResourceRequestInfo, handle: ResourceRequestHandle) -> ResourceProviderDecision

internal fun NativeCall.putLogHandler(target: Long, value: LogHandler) {
  writeU32(target, w(24, 40).toUInt())
  writeAddress(target + w(8, 16), registrations.register(value))
  writeAddress(target + w(4, 8), UpcallStubs.logHandlerCallback)
  writeAddress(target + w(12, 24), UpcallStubs.releaseRoot)
}

internal fun NativeCall.writeLogHandler(value: LogHandler): Long =
  allocate(w(24, 40), 8).also { putLogHandler(it, value) }

internal fun NativeCall.putHttpHeaderTransform(target: Long, value: HttpHeaderTransform) {
  writeU32(target, w(24, 48).toUInt())
  writeAddress(
    target + w(16, 32),
    array(value.rules, w(24, 40), w(4, 8)) { at, item -> putHttpHeaderRule(at, item) },
  )
  writeSize(target + w(20, 40), value.rules.size.toULong())
  if (value.callback == null) return
  writeAddress(target + w(8, 16), registrations.register(value))
  writeAddress(target + w(4, 8), UpcallStubs.httpHeaderTransformCallback)
  writeAddress(target + w(12, 24), UpcallStubs.releaseRoot)
}

internal fun NativeCall.writeHttpHeaderTransform(value: HttpHeaderTransform): Long =
  allocate(w(24, 48), w(4, 8)).also { putHttpHeaderTransform(it, value) }

internal fun NativeCall.putResourceProvider(target: Long, value: ResourceProvider) {
  writeU32(target, w(32, 56).toUInt())
  writeAddress(
    target + w(24, 40),
    array(value.routes, w(16, 24), w(4, 8)) { at, item -> putResourceRoute(at, item) },
  )
  writeSize(target + w(28, 48), value.routes.size.toULong())
  writeAddress(target + w(8, 16), registrations.register(value))
  writeAddress(target + w(4, 8), UpcallStubs.resourceProviderCallback)
  writeAddress(target + w(12, 24), UpcallStubs.releaseRoot)
}

internal fun NativeCall.writeResourceProvider(value: ResourceProvider): Long =
  allocate(w(32, 56), 8).also { putResourceProvider(it, value) }

/**
 * Sets one outgoing HTTP request header for the current transform invocation.
 *
 * See `mln_http_header_transform_response_set_header` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun GeneratedApi.httpHeaderTransformResponseSetHeader(
  response: HttpHeaderTransformResponse,
  name: String,
  value: String,
): Unit =
  nativeRespond(response.bindingScope, response.bindingAddress) {
    check(
      C.mln_http_header_transform_response_set_header(handle, view(name), view(value), diagnostic),
    )
  }

/**
 * Installs a process-global MapLibre Native log callback.
 *
 * See `mln_log_set_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun GeneratedApi.logSetCallback(handler: LogHandler): Unit =
  nativeCall(null, null) {
    check(C.mln_log_set_callback(writeLogHandler(handler), diagnostic))
    accept(CallbackOwner.global)
  }

/**
 * Registers or replaces the runtime-scoped outgoing HTTP header transform.
 *
 * See `mln_runtime_set_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun GeneratedRuntimeOperations.setHttpHeaderTransform(
  transform: HttpHeaderTransform,
): Deferred<Unit> =
  nativeUnit(this, binding, bindingCallbacks) {
    check(
      C.mln_runtime_set_http_header_transform(
        handle,
        writeHttpHeaderTransform(transform),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Registers or replaces a runtime-scoped network resource provider.
 *
 * See `mln_runtime_set_resource_provider` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun GeneratedRuntimeOperations.setResourceProvider(
  provider: ResourceProvider,
): Deferred<Unit> =
  nativeUnit(this, binding, bindingCallbacks) {
    check(
      C.mln_runtime_set_resource_provider(
        handle,
        writeResourceProvider(provider),
        completion,
        diagnostic,
      ),
    )
  }
