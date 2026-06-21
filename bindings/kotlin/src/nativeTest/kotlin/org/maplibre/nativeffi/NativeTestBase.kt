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
import org.maplibre.nativeffi.internal.status.Status

@OptIn(ExperimentalForeignApi::class)
open class NativeTestBase {
  @BeforeTest
  fun installNativeTestLogCallback() {
    // Kotlin/Native test events share the process output stream with native logs.
    // Consume MapLibre records so async native logging cannot corrupt Gradle's test report.
    Status.check(mln_log_set_callback(staticCFunction(::consumeNativeTestLog), null))
  }

  @AfterTest
  fun clearNativeTestLogCallback() {
    try {
      Maplibre.clearLogCallback()
    } finally {
      Status.check(mln_log_clear_callback())
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
