package org.maplibre.nativeffi.internal.async

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.mln_buffer_view
import org.maplibre.nativeffi.internal.c.mln_completion
import org.maplibre.nativeffi.internal.c.mln_completion_callback
import org.maplibre.nativeffi.internal.c.mln_completion_release
import org.maplibre.nativeffi.internal.c.mln_completion_result
import org.maplibre.nativeffi.internal.loader.CompletionBridge
import org.maplibre.nativeffi.runtime.CommandCompletion

internal actual class HandDeliveredCompletion<T>(
  actual val deferred: Deferred<T>,
  private val callback: MemorySegment,
  private val userData: MemorySegment,
  private val releaseUserData: MemorySegment,
) {
  actual fun deliver(status: Int, generation: ULong, diagnostic: String, disposition: UInt) {
    Arena.ofConfined().use { arena ->
      val result = mln_completion_result.allocate(arena)
      mln_completion_result.size(result, mln_completion_result.sizeof().toInt())
      mln_completion_result.status(result, status)
      mln_completion_result.disposition(result, disposition.toInt())
      mln_completion_result.generation(result, generation.toLong())
      val bytes = diagnostic.encodeToByteArray()
      if (bytes.isNotEmpty()) {
        val view = mln_completion_result.diagnostic(result)
        mln_buffer_view.data(
          view,
          arena.allocate(bytes.size.toLong()).copyFrom(MemorySegment.ofArray(bytes)),
        )
        mln_buffer_view.size(view, bytes.size.toLong())
      }
      mln_completion_callback.invoke(callback, userData, result)
    }
  }

  actual fun release() {
    mln_completion_release.invoke(releaseUserData, userData)
  }
}

/** Submits through [submit] and keeps the descriptor's fields for hand delivery. */
private fun <T> capture(
  submit: ((MemorySegment) -> Unit) -> Deferred<T>
): HandDeliveredCompletion<T> {
  var fields: Triple<MemorySegment, MemorySegment, MemorySegment>? = null
  val deferred = submit { descriptor ->
    fields =
      Triple(
        mln_completion.callback(descriptor),
        mln_completion.user_data(descriptor),
        mln_completion.release_user_data(descriptor),
      )
  }
  val (callback, userData, release) = requireNotNull(fields)
  return HandDeliveredCompletion(deferred, callback, userData, release)
}

private fun generation(result: MemorySegment): ULong =
  mln_completion_result.generation(result).toULong()

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
