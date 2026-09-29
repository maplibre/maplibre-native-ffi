package org.maplibre.nativeffi

import kotlin.test.AfterTest
import kotlin.test.BeforeTest
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.internal.c.mln_log_clear_callback
import org.maplibre.nativeffi.internal.c.mln_log_set_callback
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

@OptIn(ExperimentalForeignApi::class)
open class NativeTestBase {
  @BeforeTest
  fun installNativeTestLogCallback() {
    // Native logs share the process output stream with Kotlin/Native test events, so
    // consume MapLibre records to keep them out of Gradle's test report.
    NativeDiagnostics.check { diagnostic ->
      mln_log_set_callback(staticCFunction(::consumeNativeTestLog), null, null, diagnostic)
    }
  }

  @AfterTest
  fun clearNativeTestLogCallback() {
    try {
      org.maplibre.nativeffi.generated.GeneratedApi.logClearCallback()
    } finally {
      NativeDiagnostics.check { diagnostic -> mln_log_clear_callback(diagnostic) }
    }
  }
}

@Suppress("UNUSED_PARAMETER")
@OptIn(ExperimentalForeignApi::class)
private fun consumeNativeTestLog(
  userData: COpaquePointer?,
  severity: UInt,
  event: UInt,
  code: Long,
  message: CPointer<ByteVar>?,
): UInt = 1U
