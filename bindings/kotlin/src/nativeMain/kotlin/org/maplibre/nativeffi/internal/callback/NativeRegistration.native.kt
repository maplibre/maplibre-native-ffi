@file:OptIn(ExperimentalForeignApi::class)

package org.maplibre.nativeffi.internal.callback

import kotlinx.cinterop.CFunction
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.invoke
import kotlinx.cinterop.toCPointer

internal actual fun callUserDataRelease(release: Long, userData: Long) {
  release.toCPointer<CFunction<(COpaquePointer?) -> Unit>>()!!(userData.toCPointer())
}
