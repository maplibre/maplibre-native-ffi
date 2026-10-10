import MaplibreNativeFFI
import Metal
import os
import QuartzCore
import UIKit

@main
final class AppDelegate: UIResponder, UIApplicationDelegate {
  var window: UIWindow?
  private let log = Logger(
    subsystem: "org.maplibre.nativeffi.examples.swift-map-ios",
    category: "App"
  )

  func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions _: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    installCAPILogging()
    validateMetalBackend()

    let window = UIWindow(frame: windowFrame(application))
    window.rootViewController = MapViewController()
    window.makeKeyAndVisible()
    self.window = window
    application.isIdleTimerDisabled = true
    return true
  }

  func applicationWillTerminate(_: UIApplication) {
    NotificationCenter.default.post(
      name: MetalMapView.willTerminateMapViews,
      object: nil
    )
    clearCAPILogging()
  }

  private func validateMetalBackend() {
    let backends = (try! Maplibre.supportedRenderBackendMask())
    log.info("native render backends: \(renderBackendLabel(backends))")
    guard backends.contains(.metal) else {
      fatalError("the loaded MapLibre native library does not support Metal")
    }
  }
}

@MainActor
private func windowFrame(_ application: UIApplication) -> CGRect {
  application.connectedScenes
    .compactMap { ($0 as? UIWindowScene)?.screen.bounds }
    .first ?? UIScreen.main.bounds
}

@MainActor
final class MapViewController: UIViewController {
  override func loadView() {
    view = MetalMapView()
  }
}

private func renderBackendLabel(_ backends: RenderBackendFlag) -> String {
  var labels: [String] = []
  if backends.contains(.metal) {
    labels.append("metal")
  }
  if backends.contains(.opengl) {
    labels.append("opengl")
  }
  if backends.contains(.vulkan) {
    labels.append("vulkan")
  }
  return labels.isEmpty ? "none" : labels.joined(separator: ",")
}

private func installCAPILogging() {
  let log = Logger(
    subsystem: "org.maplibre.nativeffi.examples.swift-map-ios",
    category: "MapLibre"
  )
  do {
    try Maplibre.logSetCallback(handler: LogHandler {
      severity, event, code, message in
      log.info(
        "severity=\(String(describing: severity), privacy: .public) event=\(String(describing: event), privacy: .public) code=\(code): \(message, privacy: .public)"
      )
      return 1
    })
  } catch {
    log
      .error(
        "log callback install failed: \(String(describing: error), privacy: .public)"
      )
  }
}

private func clearCAPILogging() {
  do {
    try Maplibre.logClearCallback()
  } catch {
    let log = Logger(
      subsystem: "org.maplibre.nativeffi.examples.swift-map-ios",
      category: "MapLibre"
    )
    log
      .error(
        "log callback clear failed: \(String(describing: error), privacy: .public)"
      )
  }
}
