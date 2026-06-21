package org.maplibre.nativeffi.internal.status

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.toKString
import org.maplibre.nativeffi.internal.c.mln_thread_last_error_message

@OptIn(ExperimentalForeignApi::class)
internal actual object NativeDiagnostics {
  actual fun currentDiagnostic(): String = mln_thread_last_error_message()?.toKString().orEmpty()
}
