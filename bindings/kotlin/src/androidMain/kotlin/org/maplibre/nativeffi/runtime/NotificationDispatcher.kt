package org.maplibre.nativeffi.runtime

import org.bytedeco.javacpp.BoolPointer
import org.bytedeco.javacpp.IntPointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status

/** Android bridge for the shared notification receiver. */
internal class NotificationDispatcher(private val source: Long) : AutoCloseable {
  private val core =
    NotificationDispatcherCore(
      drainNative = { drainNativeReady(source) },
      operationCompleted = ::operationCompleted,
      checkOperation = ::checkOperation,
    )
  private val nativeCallback =
    object : MaplibreNativeC.mln_notification_callback() {
      override fun call(userData: Pointer?) = core.schedule()
    }

  init {
    Status.check(MaplibreNativeC.mln_notification_source_set_callback(source, nativeCallback, null))
  }

  fun setCallback(value: () -> Unit) = core.setCallback(value)

  fun clearCallback() = core.clearCallback()

  fun drainReady(): List<ReadyEndpoint> = core.drainReady()

  suspend fun await(operation: Long) = core.await(operation)

  fun forget(operation: Long) = core.forget(operation)

  override fun close() {
    Status.check(MaplibreNativeC.mln_notification_source_clear_callback(source))
    core.close()
    nativeCallback.close()
  }
}

private fun operationCompleted(operation: Long): Boolean =
  BoolPointer(1).use { completed ->
    Status.check(MaplibreNativeC.mln_operation_poll(operation, completed))
    completed.get()
  }

private fun checkOperation(operation: Long) {
  IntPointer(1).use { status ->
    Status.check(MaplibreNativeC.mln_operation_get_status(operation, status))
    Status.check(status.get())
  }
}
