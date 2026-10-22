package org.maplibre.nativeffi.internal.callback

// Only the threaded targets declare a factory of native registrations, so no call reaches this.
internal actual fun callUserDataRelease(release: Long, userData: Long): Unit =
  error("Kotlin/Wasm has no native registrations")
