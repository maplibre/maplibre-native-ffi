import MaplibreNativeFFI

func installCApiLogging() {
  do {
    try Maplibre.setLogCallback(handler: LogHandler {
      severity, event, code, message in
      print(
        "[MapLibre] severity=\(severity) event=\(event) code=\(code): \(message)"
      )
      return true
    })
  } catch {
    print("log callback install failed: \(error)")
  }
}

func clearCApiLogging() {
  do {
    try Maplibre.clearLogCallback()
  } catch {
    print("log callback clear failed: \(error)")
  }
}

func logControls() {
  print(
    """
    Controls:
      left drag: pan
      right drag or Ctrl+left drag: rotate with X, pitch with Y
      scroll: zoom at cursor
      arrows or WASD: pan
      + / -: zoom at center
      Q / E: rotate
      ] / [: pitch
      0: reset pitch and bearing

    """
  )
}

func logStartupStatus(mode: RenderTargetMode, driver: RenderDriverKind) {
  print("render target: \(mode.rawValue)")
  print("render target status: \(mode.statusLine)")
  print(
    "render driver: \(driver == .coreWorker ? "core-worker" : "caller-graphics-thread")"
  )
}
