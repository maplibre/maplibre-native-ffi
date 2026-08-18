package org.maplibre.nativeffi

internal actual fun runOnBackgroundThread(block: () -> Unit) {
  val thread = Thread(block, "maplibre-test-background")
  thread.start()
  thread.join()
}
