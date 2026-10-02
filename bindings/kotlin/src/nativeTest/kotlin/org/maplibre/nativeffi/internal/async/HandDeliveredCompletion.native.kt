@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.internal.async

import kotlinx.cinterop.CFunction
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.invoke
import kotlinx.cinterop.toCPointer

internal actual fun callCompletion(callback: Long, userData: Long, result: Long) {
  val function: CPointer<CFunction<(COpaquePointer?, COpaquePointer?) -> Unit>> =
    requireNotNull(callback.toCPointer())
  function(userData.toCPointer(), result.toCPointer())
}

internal actual fun callCompletionRelease(release: Long, userData: Long) {
  val function: CPointer<CFunction<(COpaquePointer?) -> Unit>> =
    requireNotNull(release.toCPointer())
  function(userData.toCPointer())
}
