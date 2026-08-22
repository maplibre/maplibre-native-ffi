package org.maplibre.nativeffi

/** Runs [block] on a second native thread and waits for it to finish. */
internal expect fun runOnBackgroundThread(block: () -> Unit)

/** Parks the current thread for [millis] milliseconds. */
internal expect fun sleepMillis(millis: Int)
