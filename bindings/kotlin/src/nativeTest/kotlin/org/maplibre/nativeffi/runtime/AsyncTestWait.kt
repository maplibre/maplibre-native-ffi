@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.runtime

import platform.posix.usleep

internal actual fun waitForAsyncTestWork() {
  usleep(1_000u)
}
