package org.maplibre.nativeffi.runtime

import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.IntVar
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.c.mln_notification_source_clear_callback
import org.maplibre.nativeffi.internal.c.mln_notification_source_set_callback
import org.maplibre.nativeffi.internal.c.mln_operation_get_status
import org.maplibre.nativeffi.internal.c.mln_operation_poll
import org.maplibre.nativeffi.internal.status.Status

/** Kotlin/Native bridge for the shared notification receiver. */
@OptIn(ExperimentalForeignApi::class)
internal class NotificationDispatcher(private val source: ULong) : AutoCloseable {
  private val core =
    NotificationDispatcherCore(
      drainNative = { drainNativeReady(source) },
      operationCompleted = { operation -> operationCompleted(operation.toULong()) },
      checkOperation = { operation -> checkOperation(operation.toULong()) },
    )
  private val self = StableRef.create(this)

  init {
    Status.check(
      mln_notification_source_set_callback(
        source,
        staticCFunction(::dispatchNotification),
        self.asCPointer(),
      )
    )
  }

  fun setCallback(value: () -> Unit) = core.setCallback(value)

  fun clearCallback() = core.clearCallback()

  fun drainReady(): List<ReadyEndpoint> = core.drainReady()

  suspend fun await(operation: ULong) = core.await(operation.toLong())

  fun forget(operation: ULong) = core.forget(operation.toLong())

  fun schedule() = core.schedule()

  override fun close() {
    Status.check(mln_notification_source_clear_callback(source))
    core.close()
    self.dispose()
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun operationCompleted(operation: ULong): Boolean = memScoped {
  val completed = alloc<BooleanVar>()
  Status.check(mln_operation_poll(operation, completed.ptr))
  completed.value
}

@OptIn(ExperimentalForeignApi::class)
private fun checkOperation(operation: ULong) = memScoped {
  val status = alloc<IntVar>()
  Status.check(mln_operation_get_status(operation, status.ptr))
  Status.check(status.value)
}

@OptIn(ExperimentalForeignApi::class)
private fun dispatchNotification(userData: COpaquePointer?) {
  userData?.asStableRef<NotificationDispatcher>()?.get()?.schedule()
}
