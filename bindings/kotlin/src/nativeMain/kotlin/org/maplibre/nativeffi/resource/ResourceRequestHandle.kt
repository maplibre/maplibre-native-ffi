package org.maplibre.nativeffi.resource

import cnames.structs.mln_resource_request_handle
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_resource_request_cancelled
import org.maplibre.nativeffi.internal.c.mln_resource_request_complete
import org.maplibre.nativeffi.internal.c.mln_resource_request_release
import org.maplibre.nativeffi.internal.c.mln_resource_response
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ResourceStructs

@OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)
public actual class ResourceRequestHandle
internal constructor(
  private val handle: CPointer<mln_resource_request_handle>,
  private val completer:
    (CPointer<mln_resource_request_handle>, CPointer<mln_resource_response>) -> Int =
    ::mln_resource_request_complete,
  private val cancellationChecker:
    (CPointer<mln_resource_request_handle>, CPointer<BooleanVar>) -> Int =
    { requestHandle, outCancelled ->
      mln_resource_request_cancelled(requestHandle, outCancelled)
    },
  private val releaser: (CPointer<mln_resource_request_handle>) -> Unit =
    ::mln_resource_request_release,
) : AutoCloseable {
  private val core = ResourceRequestHandleCore { releaser(handle) }
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(core) { it.releaseIfOwned() }

  public actual fun complete(response: ResourceResponse) {
    val operation = core.beginComplete()
    var reachedNative = false
    try {
      val nativeStatus = memScoped {
        val nativeResponse = ResourceStructs.resourceResponse(response, this)
        reachedNative = true
        completer(handle, nativeResponse)
      }
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
      operation.close()
    }
  }

  public actual fun isCancelled(): Boolean = core.withLiveHandle {
    memScoped {
      val outCancelled = alloc<BooleanVar>()
      outCancelled.value = false
      Status.check(cancellationChecker(handle, outCancelled.ptr))
      outCancelled.value
    }
  }

  public actual override fun close() {
    core.close()
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): UInt {
    return core.finishProviderDecision(decision).nativeValue.toUInt()
  }

  internal fun finishProviderException(): UInt {
    return core.finishProviderException()?.nativeValue?.toUInt() ?: UInt.MAX_VALUE
  }
}
