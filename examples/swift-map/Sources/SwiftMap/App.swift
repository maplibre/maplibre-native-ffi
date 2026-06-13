import AppKit
import SwiftUI

@main
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
  static let willTerminateMapViews = Notification.Name("SwiftMapWillTerminateMapViews")
  private var window: NSWindow?

  func applicationDidFinishLaunching(_ notification: Notification) {
    NSApp.setActivationPolicy(.regular)
    installCAPILogging()
    createWindow()
    NSApp.activate(ignoringOtherApps: true)
  }

  func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
    true
  }

  func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
    NotificationCenter.default.post(name: Self.willTerminateMapViews, object: nil)
    clearCAPILogging()
    return .terminateNow
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
    window.contentView = MetalMapView(mode: swiftMapConfiguration.mode)
    window.center()
    window.makeKeyAndOrderFront(nil)
    self.window = window
  }
}
