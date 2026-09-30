package org.maplibre.nativeffi.internal.async

import kotlinx.coroutines.Deferred
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.runtime.CommandCompletion

internal actual class HandDeliveredCompletion<T>(
  actual val deferred: Deferred<T>,
  private val callback: MaplibreNativeC.mln_completion_callback,
  private val userData: Pointer,
  private val releaseUserData: MaplibreNativeC.mln_completion_release,
) {
  actual fun deliver(status: Int, generation: ULong, diagnostic: String, disposition: UInt) {
    MaplibreNativeC.mln_completion_result().use { result ->
      result
        .size(result.sizeof())
        .status(status)
        .disposition(disposition.toInt())
        .generation(generation.toLong())
      val bytes = diagnostic.encodeToByteArray()
      BytePointer(maxOf(bytes.size, 1).toLong()).use { data ->
        if (bytes.isNotEmpty()) {
          data.put(*bytes)
          result.diagnostic().data(data).size(bytes.size.toLong())
        }
        callback.call(userData, result)
      }
    }
  }

  actual fun release() {
    releaseUserData.call(userData)
  }
}

/** Submits through [submit] and keeps the descriptor's fields for hand delivery. */
private fun <T> capture(
  submit: ((MaplibreNativeC.mln_completion) -> Unit) -> Deferred<T>
): HandDeliveredCompletion<T> {
  var fields:
    Triple<
      MaplibreNativeC.mln_completion_callback,
      Pointer,
      MaplibreNativeC.mln_completion_release,
    >? =
    null
  val deferred = submit { descriptor ->
    fields =
      Triple(descriptor.callback(), Pointer(descriptor.user_data()), descriptor.release_user_data())
  }
  val (callback, userData, release) = requireNotNull(fields)
  return HandDeliveredCompletion(deferred, callback, userData, release)
}

private fun generation(result: MaplibreNativeC.mln_completion_result): ULong =
  result.generation().toULong()

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
