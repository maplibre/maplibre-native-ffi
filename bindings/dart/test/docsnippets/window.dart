// Snippets for the docs site's Render to a window page. They are analyzed with
// the tests, which never run them.

// ignore_for_file: unused_local_variable

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

// Stand-ins for the host's display timer and session replacement.
void atNextRefresh(void Function() task) {}

void replaceSession() {}

void destroySurface() {}

// #region attach
late RenderSessionHandle session;
// #endregion attach

// #region frame-loop
// Draws the map if it changed since the last frame.
void demandFrame() {
  session.demandFrame(const FrameDemand());
}

void drainFrameResults() {
  final results = session.drainFrameResults();
  if (results.isEmpty) {
    return;
  }
  if (results.any(
    (result) => result.disposition == RenderResult.failed,
  )) {
    replaceSession(); // Release the session and attach a new one.
  }
}
// #endregion frame-loop

// #region frame-wake
// The binding calls both wakes in this isolate,
// so the frame wake can drain right away.
void onRedrawWake() {
  atNextRefresh(demandFrame);
}

void onFrameWake() {
  drainFrameResults();
}
// #endregion frame-wake

// #region resize
void onWindowResized(int width, int height) {
  // The window's new size in logical pixels.
  session.discarding.resize(
    LogicalSize(width: width, height: height),
  );
}
// #endregion resize

Future<void> surfaceLost() async {
  // #region lost-surface
  await session.releaseTarget();
  // #endregion lost-surface
}

Future<void> surfaceRestored(int newSurface) async {
  // #region lost-surface
  await session.setVulkanSurfaceTarget(
    VulkanSurfaceReplacement(
      logicalSize: const LogicalSize(width: 800, height: 600),
      surface: newSurface,
    ),
  );
  // #endregion lost-surface
}

Future<void> renderToWindow(
  MapHandle map,
  VulkanContextDescriptor vulkan,
  int windowSurface,
  NativePointer onDriverWorkWake,
) async {
  // #region attach
  session = map.attachVulkanSurface(
    VulkanSurfaceDescriptor(
      // The window's size in logical pixels. The map's scale factor
      // turns it into pixels.
      logicalSize: const LogicalSize(width: 800, height: 600),
      // Your VkInstance, VkPhysicalDevice,
      // VkDevice, and graphics VkQueue.
      context: vulkan,
      // Your VkSurfaceKHR.
      surface: windowSurface,
    ),
    const RenderSessionAttachOptions(
      driver: RenderDriverKind.coreWorker,
      redrawWake: Wake(callback: onRedrawWake),
      frameWake: Wake(callback: onFrameWake),
    ),
  );
  await session.ready;
  // #endregion attach

  onWindowResized(1024, 768);

  // #region service
  // An isolate can't service the session, because it can resume on
  // another thread after any await. Your native wake schedules
  // mln_render_session_service_driver_work on your graphics thread,
  // with the session's id.
  final graphicsThreadOptions = RenderSessionAttachOptions(
    driver: RenderDriverKind.callerGraphicsThread,
    redrawWake: const Wake(callback: onRedrawWake),
    frameWake: const Wake(callback: onFrameWake),
    driverWorkWake: Wake.native(callback: onDriverWorkWake),
  );
  // #endregion service

  await session.release();
  destroySurface();
}
