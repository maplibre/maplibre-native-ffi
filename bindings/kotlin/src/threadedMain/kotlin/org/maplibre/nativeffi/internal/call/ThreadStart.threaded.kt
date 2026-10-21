package org.maplibre.nativeffi.internal.call

// The calling thread may wait for the thread start.
internal actual object ThreadStart {
  actual suspend fun run(call: () -> Int): Int = call()
}
