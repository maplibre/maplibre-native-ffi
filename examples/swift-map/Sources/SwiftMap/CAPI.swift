import MaplibreNative

func installCAPILogging() {
  do {
    try Maplibre.setLogCallback { record in
      print("[MapLibre] severity=\(record.severity) event=\(record.event) code=\(record.code): \(record.message)")
      return true
    }
  } catch {
    print("log callback install failed: \(error)")
  }
}

func clearCAPILogging() {
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
      PageUp / PageDown or [ / ]: pitch
      0: reset pitch and bearing

    """
  )
}
