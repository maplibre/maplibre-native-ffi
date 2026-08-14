package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.internal.loader.NativeAccess

/** JVM bridge for the shared notification receiver. */
internal class NotificationDispatcher(val source: Long) : AutoCloseable {
  private val core =
    NotificationDispatcherCore(
      drainNative = { NativeAccess.drainReady(source) },
      operationCompleted = NativeAccess::pollOperation,
      checkOperation = NativeAccess::checkOperationStatus,
    )
  private val nativeCallback = NativeAccess.NotificationCallback(core::schedule)

  init {
    NativeAccess.setNotificationCallback(source, nativeCallback)
  }

  fun setCallback(value: () -> Unit) = core.setCallback(value)

  fun clearCallback() = core.clearCallback()

  fun drainReady(): List<ReadyEndpoint> = core.drainReady()

  suspend fun await(operation: Long) = core.await(operation)

  fun forget(operation: Long) = core.forget(operation)

  override fun close() {
    NativeAccess.clearNotificationCallback(source)
    core.close()
    nativeCallback.close()
  }
}
