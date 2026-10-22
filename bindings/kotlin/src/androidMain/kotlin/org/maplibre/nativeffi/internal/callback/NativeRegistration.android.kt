package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.internal.c.Jni

internal actual fun callUserDataRelease(release: Long, userData: Long) {
  Jni.callUserDataRelease(release, userData)
}
