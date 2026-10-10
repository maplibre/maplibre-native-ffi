package org.maplibre.nativeffi.internal.lifecycle

// A Kotlin/Native host ends its render sessions' graphics calls itself before exit.
internal actual fun endGraphicsAtExit(handle: Long, report: HandleStateCore.LeakReport) {}
