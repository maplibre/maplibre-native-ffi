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
 * Process-global log callback or queue state. This direct form computes native's answer on the
 * MapLibre thread that calls it, which only a threaded target can run Kotlin on.
 *
 * See `mln_log_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public data class DirectLogHandler(
  /** Receives each record. */
  public val callback: DirectLogCallback,
)

/**
 * Receives a MapLibre Native log record. This direct form computes native's answer on the MapLibre
 * thread that calls it, which only a threaded target can run Kotlin on.
 *
 * See `mln_log_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public typealias DirectLogCallback =
  (severity: LogSeverity, event: LogEvent, code: Long, message: String) -> UInt

/**
 * HTTP header transform state: a callback, or native header rules. This direct form computes
 * native's answer on the MapLibre thread that calls it, which only a threaded target can run Kotlin
 * on.
 *
 * See `mln_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public data class DirectHttpHeaderTransform(
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
 * Resource provider state. This direct form computes native's answer on the MapLibre thread that
 * calls it, which only a threaded target can run Kotlin on.
 *
 * See `mln_resource_provider` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public data class DirectResourceProvider(
  /** Decides each request that the routes leave to the provider. */
  public val callback: DirectResourceProviderCallback,
  /** The requests that reach the provider; empty to claim every request. */
  public val routes: List<ResourceRoute> = emptyList(),
)

/**
 * Intercepts a network resource request. This direct form computes native's answer on the MapLibre
 * thread that calls it, which only a threaded target can run Kotlin on.
 *
 * See `mln_resource_provider_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public typealias DirectResourceProviderCallback =
  (request: ResourceRequestInfo, handle: ResourceRequestHandle) -> ResourceProviderDecision

internal fun NativeCall.putDirectLogHandler(target: Long, value: DirectLogHandler) {
  writeU32(target, w(24, 40).toUInt())
  writeAddress(target + w(8, 16), registrations.register(value))
  writeAddress(target + w(4, 8), UpcallStubs.directLogHandlerCallback)
  writeAddress(target + w(12, 24), UpcallStubs.releaseRoot)
}

internal fun NativeCall.writeDirectLogHandler(value: DirectLogHandler): Long =
  allocate(w(24, 40), 8).also { putDirectLogHandler(it, value) }

internal fun NativeCall.putDirectHttpHeaderTransform(
  target: Long,
  value: DirectHttpHeaderTransform,
) {
  writeU32(target, w(24, 48).toUInt())
  writeAddress(
    target + w(16, 32),
    array(value.rules, w(24, 40), w(4, 8)) { at, item -> putHttpHeaderRule(at, item) },
  )
  writeSize(target + w(20, 40), value.rules.size.toULong())
  if (value.callback == null) return
  writeAddress(target + w(8, 16), registrations.register(value))
  writeAddress(target + w(4, 8), UpcallStubs.directHttpHeaderTransformCallback)
  writeAddress(target + w(12, 24), UpcallStubs.releaseRoot)
}

internal fun NativeCall.writeDirectHttpHeaderTransform(value: DirectHttpHeaderTransform): Long =
  allocate(w(24, 48), w(4, 8)).also { putDirectHttpHeaderTransform(it, value) }

internal fun NativeCall.putDirectResourceProvider(target: Long, value: DirectResourceProvider) {
  writeU32(target, w(32, 56).toUInt())
  writeAddress(
    target + w(24, 40),
    array(value.routes, w(16, 24), w(4, 8)) { at, item -> putResourceRoute(at, item) },
  )
  writeSize(target + w(28, 48), value.routes.size.toULong())
  writeAddress(target + w(8, 16), registrations.register(value))
  writeAddress(target + w(4, 8), UpcallStubs.directResourceProviderCallback)
  writeAddress(target + w(12, 24), UpcallStubs.releaseRoot)
}

internal fun NativeCall.writeDirectResourceProvider(value: DirectResourceProvider): Long =
  allocate(w(32, 56), 8).also { putDirectResourceProvider(it, value) }

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
public fun GeneratedApi.logSetCallback(handler: DirectLogHandler): Unit =
  nativeCall(null, null) {
    check(C.mln_log_set_callback(writeDirectLogHandler(handler), diagnostic))
    accept(CallbackOwner.global)
  }

/**
 * Registers or replaces the runtime-scoped outgoing HTTP header transform.
 *
 * See `mln_runtime_set_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun GeneratedRuntimeOperations.setHttpHeaderTransform(
  transform: DirectHttpHeaderTransform,
): Deferred<Unit> =
  nativeUnit(this, binding, bindingCallbacks) {
    check(
      C.mln_runtime_set_http_header_transform(
        handle,
        writeDirectHttpHeaderTransform(transform),
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
  provider: DirectResourceProvider,
): Deferred<Unit> =
  nativeUnit(this, binding, bindingCallbacks) {
    check(
      C.mln_runtime_set_resource_provider(
        handle,
        writeDirectResourceProvider(provider),
        completion,
        diagnostic,
      ),
    )
  }
