import Darwin
import Foundation
import MaplibreNative

let swiftMapConfiguration = AppConfiguration.loadOrExit()

struct AppConfiguration {
  let mode: RenderTargetMode

  static func loadOrExit(arguments: [String] = CommandLine
    .arguments) -> AppConfiguration
  {
    let args = Array(arguments.dropFirst())
    guard let mode = parseMode(args) else {
      exit(0)
    }

    let backends = Maplibre.supportedRenderBackends()
    print("native render backends: \(renderBackendLabel(backends))")
    guard backends.contains(.metal) else {
      printError("the loaded MapLibre native library does not support Metal")
      exit(1)
    }

    return AppConfiguration(mode: mode)
  }

  private static func parseMode(_ args: [String]) -> RenderTargetMode? {
    if args.count == 1, args[0] == "--help" {
      printUsage()
      return nil
    }
    guard args.count == 1,
          !args[0].hasPrefix("-"),
          let mode = RenderTargetMode(rawValue: args[0])
    else {
      printUsage()
      exit(1)
    }
    return mode
  }

  private static func printUsage() {
    printError(
      """
      Usage: swift-map <mode>

      Modes:
        owned-texture     session-owned texture render target
        borrowed-texture  caller-owned texture render target
        native-surface    native surface render target

      """
    )
  }

  private static func renderBackendLabel(_ backends: RenderBackend) -> String {
    var labels: [String] = []
    if backends.contains(.metal) {
      labels.append("metal")
    }
    if backends.contains(.openGL) {
      labels.append("opengl")
    }
    if backends.contains(.vulkan) {
      labels.append("vulkan")
    }
    return labels.isEmpty ? "none" : labels.joined(separator: ",")
  }

  private static func printError(_ message: String) {
    fputs(message, stderr)
    if !message.hasSuffix("\n") {
      fputs("\n", stderr)
    }
  }
}

enum RenderTargetMode: String, CaseIterable {
  case ownedTexture = "owned-texture"
  case borrowedTexture = "borrowed-texture"
  case nativeSurface = "native-surface"

  var statusLine: String {
    switch self {
    case .ownedTexture:
      "samples MapLibre-owned texture frames into the host swapchain"
    case .borrowedTexture:
      "renders into a host-owned texture, then samples it into the host swapchain"
    case .nativeSurface:
      "renders directly to the host window surface"
    }
  }
}
