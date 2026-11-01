// Snippets for the docs site's Events and logs page. They compile with the
// tests, which never run them.

import MaplibreNativeFFI

private func onStyleLoaded() {}

private func onLoadingFailed(_: String) {}

private func writeLog(_: LogSeverity, _: String) {}

@MainActor private var map: MapHandle?

// #region handler

@MainActor private func handleEvents(_ events: [RuntimeEvent]) {
  for event in events where event.source == map?.id {
    switch event.type {
    case .mapStyleLoaded:
      onStyleLoaded()
    case .mapLoadingFailed:
      onLoadingFailed(event.message)
    default:
      break
    }
  }
}

// #endregion handler

@MainActor private func eventsAndLogs(runtime: RuntimeHandle) async throws {
  // #region handler
  // #region queue
  // The handler runs on a library thread. It hands each batch to the main
  // actor.
  try runtime.discarding.setEventHandler(RuntimeEventHandler { events in
    Task { @MainActor in handleEvents(events) }
  })
  // #endregion queue
  // #endregion handler

  // #region mask
  let created = try runtime.createMap(options: MapOptions(
    initialExtent: LogicalExtent(width: 512, height: 512, scaleFactor: 1),
    eventMask: [.mapStyleLoaded, .mapLoadingFailed]
  ))
  // #endregion mask
  map = created.map
  try await created.ready.value

  // #region logs
  // Runs on a library thread, and can't call the library.
  try setLogCallback(handler: LogHandler { severity, _, _, message in
    writeLog(severity, message)
    // Consume the record. Return false to let the platform logger print it too.
    return true
  })
  // #endregion logs
}
