package org.maplibre.nativeffi.resource

import org.bytedeco.javacpp.BytePointer
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelBridge
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelRegistration
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelSetResult
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelState
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.UnreachableActions
import org.maplibre.nativeffi.internal.status.Status

/** Owned Android JNI handle for a resource provider request. */
public actual class ResourceRequestHandle
internal constructor(
  private val handleId: Long,
  private val completer: (Long, ResourceResponse) -> Int = { handle, response ->
    NativeResourceResponseScope(response).use {
      MaplibreNativeC.mln_resource_request_complete(handle, it.response)
    }
  },
  private val cancellationChecker: (Long) -> Boolean = { handle ->
    val outCancelled = booleanArrayOf(false)
    Status.check(MaplibreNativeC.mln_resource_request_cancelled(handle, outCancelled))
    outCancelled[0]
  },
  private val cancelCallbackSetter: (Long, Long) -> ResourceRequestCancelSetResult =
    { handle, token ->
      val outCancelled = booleanArrayOf(false)
      val status =
        MaplibreNativeC.mln_resource_request_set_cancel_callback(
          handle,
          ResourceRequestCancelBridge.stub,
          ResourceRequestCancelBridge.userData(token),
          outCancelled,
        )
      ResourceRequestCancelSetResult(status, outCancelled[0])
    },
  releaser: (Long) -> Unit = MaplibreNativeC::mln_resource_request_release,
) : AutoCloseable {
  private val cancelRegistration = ResourceRequestCancelRegistration()
  private val cancelState = ResourceRequestCancelState(cancelRegistration)
  private val core =
    ResourceRequestHandleCore(ReleaseNativeRequest(handleId, releaser, cancelRegistration))

  init {
    require(handleId != 0L) { "Resource request handle is null" }
    UnreachableActions.register(this, CloseWhenUnreachableAction(core))
  }

  public actual fun complete(response: ResourceResponse) {
    val operation = core.beginComplete()
    var reachedNative = false
    try {
      val nativeStatus = completer(handleId, response).also { reachedNative = true }
      val nativeFailure =
        if (nativeStatus == MaplibreStatus.OK.nativeCode) null else Status.exception(nativeStatus)
      operation.markCompleted()
      nativeFailure?.let { throw it }
    } catch (error: Throwable) {
      if (reachedNative) {
        operation.markCompleted()
      } else {
        operation.markNotReachedNative()
      }
      throw error
    } finally {
      if (reachedNative) cancelState.drop()
      operation.close()
    }
  }

  public actual fun isCancelled(): Boolean = core.withLiveHandle { cancellationChecker(handleId) }

  public actual fun setCancelCallback(callback: () -> Unit) {
    val alreadyCancelled = core.withLiveHandle {
      cancelState.register(callback) { token -> cancelCallbackSetter(handleId, token) }
    }
    // The borrow has ended, so the callback may close this handle and release it immediately.
    alreadyCancelled?.let(ResourceRequestCancelState::runContained)
  }

  public actual override fun close() {
    cancelState.drop()
    core.close()
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): Int =
    finishProvider(core.finishProviderDecision(decision))

  internal fun finishProviderException(): Int =
    core.finishProviderException()?.let(::finishProvider) ?: handedBackToNative(UNKNOWN_DECISION)

  private fun finishProvider(decision: ResourceProviderDecision): Int =
    if (decision == ResourceProviderDecision.PASS_THROUGH) {
      handedBackToNative(decision.nativeValue)
    } else {
      decision.nativeValue
    }

  /** MapLibre retires a request the provider did not handle, so the release path never runs. */
  private fun handedBackToNative(result: Int): Int {
    cancelState.drop()
    cancelRegistration.dispose()
    return result
  }

  internal class NativeResourceResponseScope(value: ResourceResponse) : AutoCloseable {
    val response: MaplibreNativeC.mln_resource_response = MaplibreNativeC.mln_resource_response()
    private val bytes: BytePointer?
    private val errorMessage: BytePointer?
    private val etag: BytePointer?

    init {
      if (!value.errorReason.isKnown) {
        throw Status.invalidArgument(
          "Unknown resource error reason cannot be used as input: ${value.errorReason.nativeValue}"
        )
      }
      response.size(response.sizeof())
      response.status(value.status.nativeValue)
      response.error_reason(value.errorReason.nativeValue)
      val responseBytes = value.bytes
      bytes =
        if (responseBytes.isNotEmpty()) {
          BytePointer(responseBytes.size.toLong()).also { storage ->
            storage.put(responseBytes, 0, responseBytes.size)
            response.bytes(storage)
            response.byte_count(responseBytes.size.toLong())
          }
        } else {
          null
        }
      errorMessage = optionalCString(value.errorMessage, "error message")
      response.error_message(errorMessage)
      response.must_revalidate(value.mustRevalidate)
      value.modifiedUnixMs?.let {
        response.has_modified(true)
        response.modified_unix_ms(it)
      }
      value.expiresUnixMs?.let {
        response.has_expires(true)
        response.expires_unix_ms(it)
      }
      etag = optionalCString(value.etag, "ETag")
      response.etag(etag)
      value.retryAfterUnixMs?.let {
        response.has_retry_after(true)
        response.retry_after_unix_ms(it)
      }
    }

    override fun close() {
      response.close()
      bytes?.close()
      errorMessage?.close()
      etag?.close()
    }

    private fun optionalCString(value: String?, description: String): BytePointer? {
      value ?: return null
      if ('\u0000' in value) {
        throw Status.invalidArgument("$description contains embedded NUL")
      }
      return JavaCppSupport.utf8(value)
    }
  }

  /**
   * Releases the native request once the wrapper becomes unreachable.
   *
   * Request handles carry no owner-thread affinity, so the cleanup thread may reclaim one. This
   * holds the ownership state alone; holding the wrapper would keep it reachable and suppress every
   * reclaim.
   */
  private class CloseWhenUnreachableAction(private val core: ResourceRequestHandleCore) : Runnable {
    override fun run() {
      core.close()
    }
  }

  /**
   * Releases the native request and then drops its registry token.
   *
   * Native release returns once a cancel callback running on another thread has returned, so no
   * native use of the token outlives this call. This holds the token alone; the callback state
   * would reach the host callback and whatever it captures.
   */
  private class ReleaseNativeRequest(
    private val handleId: Long,
    private val releaser: (Long) -> Unit,
    private val cancelRegistration: ResourceRequestCancelRegistration,
  ) : () -> Unit {
    override fun invoke() {
      try {
        releaser(handleId)
      } finally {
        cancelRegistration.dispose()
      }
    }
  }

  private companion object {
    private const val UNKNOWN_DECISION: Int = -1
  }
}

/** Direct test seam for the JavaCPP resource-response adapter. */
internal object JavaCppResourceStructs {
  fun resourceResponseSnapshot(value: ResourceResponse): ResourceResponseSnapshot =
    ResourceRequestHandle.NativeResourceResponseScope(value).use {
      ResourceResponseSnapshot(
        it.response.status(),
        JavaCppSupport.byteArray(it.response.bytes(), it.response.byte_count()),
        it.response.must_revalidate(),
        it.response.has_modified(),
        it.response.has_expires(),
        it.response.has_retry_after(),
      )
    }

  data class ResourceResponseSnapshot(
    val status: Int,
    val bytes: ByteArray,
    val mustRevalidate: Boolean,
    val hasModified: Boolean,
    val hasExpires: Boolean,
    val hasRetryAfter: Boolean,
  )
}
