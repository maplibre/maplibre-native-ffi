package org.maplibre.nativeffi.runtime

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.IntVar
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.value
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_operation_cancel
import org.maplibre.nativeffi.internal.c.mln_operation_copy_diagnostic
import org.maplibre.nativeffi.internal.c.mln_operation_discard_result
import org.maplibre.nativeffi.internal.c.mln_operation_get_status
import org.maplibre.nativeffi.internal.c.mln_operation_poll
import org.maplibre.nativeffi.internal.c.mln_operation_release
import org.maplibre.nativeffi.internal.c.mln_operation_wait
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status

@OptIn(ExperimentalNativeApi::class, ExperimentalForeignApi::class)
public actual class OperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  nativeId: ULong,
  kind: OperationKind,
  resultKind: OperationResultKind,
  ownerRetention: HandleStateCore.ChildRetention? = null,
) : AutoCloseable {
  private val runtimeRetention = runtime.retainChild("OperationHandle")
  private val core =
    OperationHandleCore(runtime, nativeId.toLong(), kind, resultKind) {
      ownerRetention?.close()
      runtimeRetention.close()
    }
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(core.leakReport) { it.report() }

  public actual val isClosed: Boolean
    get() = core.isClosed

  internal fun <R> withUse(block: (ULong) -> R): R = core.withUse(runtime) { block(it.toULong()) }

  internal fun <R> withResultUse(
    expectedKind: OperationKind,
    expectedResultKind: OperationResultKind,
    block: (ULong) -> R,
  ): R = core.withUse(runtime, expectedKind, expectedResultKind) { block(it.toULong()) }

  internal fun markResultConsumed() {
    core.markResultConsumed()
  }

  public actual fun poll(): Boolean = withUse { operation ->
    memScoped {
      val completed = alloc<BooleanVar>()
      Status.check(mln_operation_poll(operation, completed.ptr))
      completed.value
    }
  }

  public actual fun waitForCompletion(timeoutMillis: Long): Boolean = withUse { operation ->
    memScoped {
      val completed = alloc<BooleanVar>()
      Status.check(mln_operation_wait(operation, timeoutMillis, completed.ptr))
      completed.value
    }
  }

  public actual fun cancel() {
    withUse { Status.check(mln_operation_cancel(it)) }
  }

  public actual fun terminalStatus(): MaplibreStatus = withUse { operation ->
    memScoped {
      val outStatus = alloc<IntVar>()
      Status.check(mln_operation_get_status(operation, outStatus.ptr))
      MaplibreStatus.fromNative(outStatus.value)
    }
  }

  public actual fun diagnostic(): String = withUse { operation ->
    memScoped {
      val outSize = alloc<ULongVar>()
      Status.check(mln_operation_copy_diagnostic(operation, null, 0UL, outSize.ptr))
      if (outSize.value == 0UL) {
        ""
      } else {
        val bytes = allocArray<ByteVar>(outSize.value.toInt())
        Status.check(mln_operation_copy_diagnostic(operation, bytes, outSize.value, outSize.ptr))
        bytes.readBytes(outSize.value.toInt()).decodeToString()
      }
    }
  }

  public actual fun discard() {
    withUse {
      Status.check(mln_operation_discard_result(it))
      core.markResultConsumed()
    }
  }

  public actual override fun close() {
    if (!core.beginClose()) return
    runtime.forgetOperation(core.id.toULong())
    mln_operation_release(core.id.toULong())
    core.finishClose()
  }
}
