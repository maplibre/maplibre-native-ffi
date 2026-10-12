package org.maplibre.nativeffi.internal.async

import java.lang.foreign.FunctionDescriptor
import java.lang.foreign.Linker
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout.JAVA_LONG

private val linker = Linker.nativeLinker()

internal actual fun callCompletion(callback: Long, userData: Long, result: Long) {
  linker
    .downcallHandle(
      MemorySegment.ofAddress(callback),
      FunctionDescriptor.ofVoid(JAVA_LONG, JAVA_LONG),
    )
    .invoke(userData, result)
}

internal actual fun callCompletionRelease(release: Long, userData: Long) {
  linker
    .downcallHandle(MemorySegment.ofAddress(release), FunctionDescriptor.ofVoid(JAVA_LONG))
    .invoke(userData)
}
