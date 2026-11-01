// Snippets for the docs site's Events and logs page. They compile with the tests, which never run
// them.
package org.maplibre.nativeffi.docsnippets

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch
import org.maplibre.nativeffi.LogHandler
import org.maplibre.nativeffi.LogSeverity
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.MapOptions
import org.maplibre.nativeffi.RuntimeEvent
import org.maplibre.nativeffi.RuntimeEventFlag
import org.maplibre.nativeffi.RuntimeEventHandler
import org.maplibre.nativeffi.RuntimeEventType
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.setLogCallback

private var map: MapHandle? = null

private fun onStyleLoaded() {}

private fun onLoadingFailed(message: String) {}

private fun writeLog(severity: LogSeverity, message: String) {}

// #region handler
private fun handleEvents(events: List<RuntimeEvent>) {
  for (event in events) {
    if (event.source != map?.id) continue

    when (event.type) {
      RuntimeEventType.MAP_STYLE_LOADED -> onStyleLoaded()
      RuntimeEventType.MAP_LOADING_FAILED -> onLoadingFailed(event.message)
    }
  }
}

// #endregion handler

// appScope runs coroutines on your application's thread, such as MainScope() on Android.
private suspend fun eventsAndLogs(runtime: RuntimeHandle, appScope: CoroutineScope) {
  // #region handler
  // Runs on a library thread.
  runtime.discarding.setEventHandler(RuntimeEventHandler(::handleEvents))
  // #endregion handler

  // #region queue
  // The binding has no delivery queue. Post each batch to your thread, and return.
  runtime.discarding.setEventHandler(
    RuntimeEventHandler { events -> appScope.launch { handleEvents(events) } },
  )
  // #endregion queue

  // #region mask
  val (created, ready) =
    runtime.createMap(
      MapOptions(
        initialExtent = LogicalExtent(width = 512u, height = 512u, scaleFactor = 1.0),
        eventMask = RuntimeEventFlag.MAP_STYLE_LOADED or RuntimeEventFlag.MAP_LOADING_FAILED,
      ),
    )
  // #endregion mask
  map = created
  ready.await()

  // #region logs
  // Runs on a library thread, and can't call the library.
  setLogCallback(
    LogHandler { severity, _, _, message ->
      writeLog(severity, message)
      // Consume the record. Return false to let the platform logger print it too.
      true
    },
  )
  // #endregion logs
}
