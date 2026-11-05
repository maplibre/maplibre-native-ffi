package org.maplibre.nativeffi.internal.call

// The calling thread may wait for the thread start.
internal actual object ThreadStart {
  actual fun run(call: () -> Int, done: (Int) -> Unit) {
    done(call())
  }
}
