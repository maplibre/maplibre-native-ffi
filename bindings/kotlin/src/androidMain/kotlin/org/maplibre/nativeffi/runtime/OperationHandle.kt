package org.maplibre.nativeffi.runtime

import java.nio.charset.StandardCharsets
import org.bytedeco.javacpp.BoolPointer
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.IntPointer
import org.bytedeco.javacpp.SizeTPointer
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status

/** Common operation observer backed by the Android JNI bridge. */
public actual class OperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  id: Long,
  kind: OperationKind,
  resultKind: OperationResultKind,
  ownerRetention: HandleStateCore.ChildRetention? = null,
) : AutoCloseable {
  private val runtimeRetention = runtime.retainChild("OperationHandle")
  private val core =
    OperationHandleCore(runtime, id, kind, resultKind) {
      ownerRetention?.close()
      runtimeRetention.close()
    }

  init {
    HandleLeakCleaner.registerOperation(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isClosed

  internal fun <R> withUse(block: (Long) -> R): R =
    try {
      core.withUse(runtime, block)
    } finally {
      retireConsumed()
    }

  internal fun <R> withResultUse(
    expectedKind: OperationKind,
    expectedResultKind: OperationResultKind,
    block: (Long) -> R,
  ): R =
    try {
      core.withUse(runtime, expectedKind, expectedResultKind, block)
    } finally {
      retireConsumed()
    }

  internal fun markResultConsumed() {
    core.markResultConsumed()
  }

  public actual fun poll(): Boolean = withUse { operation ->
    BoolPointer(1).use { completed ->
      Status.check(MaplibreNativeC.mln_operation_poll(operation, completed))
      completed.get()
    }
  }

  public actual fun waitForCompletion(timeoutMillis: Long): Boolean = withUse { operation ->
    BoolPointer(1).use { completed ->
      Status.check(MaplibreNativeC.mln_operation_wait(operation, timeoutMillis, completed))
      completed.get()
    }
  }

  public actual fun cancel() {
    withUse { Status.check(MaplibreNativeC.mln_operation_cancel(it)) }
  }

  public actual fun terminalStatus(): MaplibreStatus = withUse { operation ->
    IntPointer(1).use { outStatus ->
      Status.check(MaplibreNativeC.mln_operation_get_status(operation, outStatus))
      MaplibreStatus.fromNative(outStatus.get())
    }
  }

  public actual fun diagnostic(): String = withUse { operation ->
    SizeTPointer(1).use { outSize ->
      Status.check(
        MaplibreNativeC.mln_operation_copy_diagnostic(operation, null as BytePointer?, 0L, outSize)
      )
      val size = outSize.get()
      if (size == 0L) {
        ""
      } else {
        BytePointer(size).use { bytes ->
          Status.check(
            MaplibreNativeC.mln_operation_copy_diagnostic(operation, bytes, size, outSize)
          )
          val copied = ByteArray(Math.toIntExact(outSize.get()))
          bytes.get(copied)
          String(copied, StandardCharsets.UTF_8)
        }
      }
    }
  }

  public actual fun finish() {
    withUse {
      Status.check(MaplibreNativeC.mln_operation_finish(it))
      core.markResultConsumed()
    }
  }

  public actual override fun close() {
    if (!core.beginClose()) return
    runtime.forgetOperation(core.id)
    MaplibreNativeC.mln_operation_release(core.id)
    core.finishClose()
  }

  private fun retireConsumed() {
    if (!core.hasConsumedResult() || !core.beginClose()) return
    runtime.forgetOperation(core.id)
    core.finishClose()
  }
}
