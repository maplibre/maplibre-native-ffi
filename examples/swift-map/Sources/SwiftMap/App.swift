import AppKit
import SwiftUI

/// Runs the app, or with `--smoke <mode>`, renders one frame headless in that
/// render-target mode and exits.
@main
enum SwiftMapMain {
  static func main() {
    let args = Array(CommandLine.arguments.dropFirst())
    guard args.first == "--smoke" else {
      SwiftMapApp.main()
      return
    }
    guard args.count == 2, let mode = RenderTargetMode(rawValue: args[1]) else {
      fputs("Usage: swift-map --smoke <mode>\n", stderr)
      exit(1)
    }
    Task { @MainActor in
      installCAPILogging()
      let status = await runSmoke(mode: mode)
      clearCAPILogging()
      exit(status)
    }
    // The run loop drains the main queue on the main thread, which the caller
    // driver keeps as its graphics thread. dispatchMain() would drain it on
    // pool threads instead.
    RunLoop.main.run()
  }
}

struct SwiftMapApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) private var appDelegate

  var body: some Scene {
    Settings {
      EmptyView()
    }
  }
}

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
  private var window: NSWindow?
  private var mapView: MetalMapView?

  func applicationDidFinishLaunching(_: Notification) {
    NSApp.setActivationPolicy(.regular)
    installCAPILogging()
    createWindow()
    NSApp.activate(ignoringOtherApps: true)
  }

  func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication)
    -> Bool
  {
    true
  }

  /// Native teardown is asynchronous, so the reply waits for it rather than
  /// letting the process exit while the runtime is still releasing.
  func applicationShouldTerminate(_: NSApplication) -> NSApplication
    .TerminateReply
  {
    guard let mapView else {
      clearCAPILogging()
      return .terminateNow
    }
    Task { @MainActor in
      await mapView.shutdown()
      clearCAPILogging()
      NSApp.reply(toApplicationShouldTerminate: true)
    }
    return .terminateLater
  }

  private func createWindow() {
    let contentRect = NSRect(x: 0, y: 0, width: 960, height: 640)
    let window = NSWindow(
      contentRect: contentRect,
      styleMask: [.titled, .closable, .miniaturizable, .resizable],
      backing: .buffered,
      defer: false
    )
    window.title = "MapLibre Swift Map"
    let mapView = MetalMapView(mode: swiftMapConfiguration.mode)
    self.mapView = mapView
    window.contentView = mapView
    window.center()
    window.makeKeyAndOrderFront(nil)
    self.window = window
  }
}
