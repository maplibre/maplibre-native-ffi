@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.internal.async

import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.invoke
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.set
import kotlinx.cinterop.sizeOf
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.mln_completion
import org.maplibre.nativeffi.internal.c.mln_completion_callback
import org.maplibre.nativeffi.internal.c.mln_completion_release
import org.maplibre.nativeffi.internal.c.mln_completion_result
import org.maplibre.nativeffi.runtime.CommandCompletion

internal actual class HandDeliveredCompletion<T>(
  actual val deferred: Deferred<T>,
  private val callback: mln_completion_callback,
  private val userData: COpaquePointer?,
  private val releaseUserData: mln_completion_release,
) {
  actual fun deliver(status: Int, generation: ULong, diagnostic: String, disposition: UInt) {
    memScoped {
      val result = alloc<mln_completion_result>()
      result.size = sizeOf<mln_completion_result>().toUInt()
      result.status = status
      result.disposition = disposition
      result.generation = generation
      val bytes = diagnostic.encodeToByteArray()
      if (bytes.isNotEmpty()) {
        val data = allocArray<ByteVar>(bytes.size)
        bytes.forEachIndexed { index, byte -> data[index] = byte }
        result.diagnostic.data = data
        result.diagnostic.size = bytes.size.toULong()
      }
      callback.invoke(userData, result.ptr)
    }
  }

  actual fun release() {
    releaseUserData.invoke(userData)
  }
}

/** Submits through [submit] and keeps the descriptor's fields for hand delivery. */
private fun <T> capture(
  submit: ((CPointer<mln_completion>) -> Unit) -> Deferred<T>
): HandDeliveredCompletion<T> {
  var fields: Triple<mln_completion_callback, COpaquePointer?, mln_completion_release>? = null
  val deferred = submit { descriptor ->
    val completion = descriptor.pointed
    fields =
      Triple(
        requireNotNull(completion.callback),
        completion.user_data,
        requireNotNull(completion.release_user_data),
      )
  }
  val (callback, userData, release) = requireNotNull(fields)
  return HandDeliveredCompletion(deferred, callback, userData, release)
}

private fun generation(result: CPointer<mln_completion_result>): ULong = result.pointed.generation

internal actual fun handDeliveredGeneration(): HandDeliveredCompletion<ULong> = capture { call ->
  CompletionBridge.submit(::generation, call)
}

internal actual fun handDeliveredOwned(
  closeDropped: (ULong) -> Unit
): HandDeliveredCompletion<ULong> = capture { call ->
  CompletionBridge.submitOwned(::generation, closeDropped, disposeUnadopted = {}, call = call)
}

internal actual fun handDeliveredCommand(): HandDeliveredCompletion<CommandCompletion> =
  capture { call ->
    CompletionBridge.command(call)
  }
