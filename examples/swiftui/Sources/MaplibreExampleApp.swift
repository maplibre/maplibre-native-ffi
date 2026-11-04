import SwiftUI

@main
struct MaplibreExampleApp: App {
  var body: some Scene {
    WindowGroup {
      ContentView()
    }
    .defaultSize(width: 1000, height: 700)
  }
}

/// A map that fills the window, with a status line over it while the map
/// loads or after it fails.
struct ContentView: View {
  @State private var controller = MapController()
  @Environment(\.scenePhase) private var scenePhase

  var body: some View {
    MapSurface(controller: controller)
      .ignoresSafeArea()
      .overlay(alignment: .bottom) {
        if !controller.status.message.isEmpty {
          Text(controller.status.message)
            .font(.system(size: 15))
            .foregroundStyle(controller.status.error ? Color(
              red: 0.63,
              green: 0.15,
              blue: 0.11
            ) : Color(white: 0.13))
            .padding(.horizontal, 14)
            .padding(.vertical, 8)
            .background(
              controller.status.error ? Color(
                red: 0.99,
                green: 0.93,
                blue: 0.92
              ) : Color.white.opacity(0.92),
              in: .rect(cornerRadius: 8)
            )
            .padding(.horizontal, 20)
            .padding(.bottom, 48)
            .allowsHitTesting(false)
        }
      }
      .background(Color(red: 0.95, green: 0.94, blue: 0.91))
      .onAppear { controller.start() }
      .onDisappear { controller.dispose() }
      .onChange(of: scenePhase, initial: true) { _, phase in
        // The window can't show the map in the background, and iOS doesn't
        // let a background app render.
        controller.setVisible(phase != .background)
      }
  }
}
