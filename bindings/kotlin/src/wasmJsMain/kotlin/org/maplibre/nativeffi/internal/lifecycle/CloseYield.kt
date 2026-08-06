package org.maplibre.nativeffi.internal.lifecycle

// Kotlin/Wasm has one thread and no scheduler call that gives it up, so the spin is the yield. The
// count a close waits on is held by a native call on another thread, exactly as it is elsewhere.
internal actual fun yieldWhileClosing() {}
