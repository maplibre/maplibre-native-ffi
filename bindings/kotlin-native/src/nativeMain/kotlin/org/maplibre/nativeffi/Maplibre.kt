package org.maplibre.nativeffi

import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.internal.c.MLN_LOG_SEVERITY_MASK_DEFAULT
import org.maplibre.nativeffi.internal.c.mln_c_version
import org.maplibre.nativeffi.internal.c.mln_log_set_async_severity_mask
import org.maplibre.nativeffi.internal.callback.LogCallbackState
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogSeverity

/** Process-global entry points for the Kotlin/Native binding. */
@OptIn(ExperimentalForeignApi::class)
public object Maplibre {
  /** Returns the native C ABI contract version. */
  public fun cVersion(): UInt = mln_c_version()

  /** Installs or replaces the process-global native log callback. */
  public fun setLogCallback(callback: LogCallback) {
    LogCallbackState.set(callback)
  }

  /** Clears the process-global native log callback. */
  public fun clearLogCallback() {
    LogCallbackState.clear()
  }

  /** Configures severities that native logging may dispatch asynchronously. */
  public fun setAsyncLogSeverities(severities: Set<LogSeverity>) {
    val mask = severities.fold(0U) { acc, severity -> acc or severity.nativeMask() }
    Status.check(mln_log_set_async_severity_mask(mask))
  }

  /** Restores the native default async log severity mask. */
  public fun restoreDefaultAsyncLogSeverities() {
    Status.check(mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_MASK_DEFAULT))
  }
}
