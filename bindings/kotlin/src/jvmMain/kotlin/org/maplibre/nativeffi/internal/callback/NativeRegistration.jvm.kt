package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout.JAVA_LONG
import org.maplibre.nativeffi.internal.c.Ffm

// An `mln_user_data_release` takes its context as a pointer, which crosses as JAVA_LONG.
private val userDataRelease = Ffm.downcall(null, JAVA_LONG)

internal actual fun callUserDataRelease(release: Long, userData: Long) {
  userDataRelease.invoke(MemorySegment.ofAddress(release), userData)
}
