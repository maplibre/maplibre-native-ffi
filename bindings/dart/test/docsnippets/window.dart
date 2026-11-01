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
// Set while the window has no surface, and when it closes.
var paused = false;

void requestFrame() {
  session.requestFrame(
    FrameDemand(flags: FrameDemandFlag.present | FrameDemandFlag.waitForUpdate),
  );
}

void drainFrameResults() {
  final results = session.drainFrameResults();
  if (results.isEmpty) {
    return;
  }
  if (results.any((result) => result.disposition == RenderResult.failed)) {
    replaceSession(); // Close the session and attach a new one.
  } else if (!paused) {
    atNextRefresh(requestFrame);
  }
}
// #endregion frame-loop

// #region frame-wake
// The binding calls the wake in this isolate, so it can drain right away.
void onFrameWake() {
  drainFrameResults();
}
// #endregion frame-wake

// #region resize
void onWindowResized(int width, int height) {
  // The scale factor the session has now.
  session.discarding.resize(
    LogicalExtent(width: width, height: height, scaleFactor: 2),
  );
}
// #endregion resize

Future<void> surfaceLost() async {
  // #region lost-surface
  paused = true;
  await session.releaseTarget();
  // #endregion lost-surface
}

Future<void> surfaceRestored(
  VulkanContextDescriptor vulkan,
  int newSurface,
) async {
  // #region lost-surface
  await session.setVulkanSurfaceTarget(
    VulkanSurfaceDescriptor(
      extent: const LogicalExtent(width: 800, height: 600, scaleFactor: 2),
      context: vulkan,
      surface: newSurface,
    ),
  );
  paused = false;
  requestFrame();
  // #endregion lost-surface
}

Future<void> renderToWindow(
  MapHandle map,
  VulkanContextDescriptor vulkan,
  int windowSurface,
  NativePointer onDriverWorkWake,
) async {
  // #region attach
  final attachment = map.attachVulkanSurface(
    VulkanSurfaceDescriptor(
      // The window's size in logical pixels, and its scale factor.
      extent: const LogicalExtent(width: 800, height: 600, scaleFactor: 2),
      // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
      context: vulkan,
      // Your VkSurfaceKHR.
      surface: windowSurface,
    ),
    const RenderSessionAttachOptions(
      driver: RenderDriverKind.coreWorker,
      frameWake: Wake(callback: onFrameWake),
    ),
  );
  session = attachment.session;
  await attachment.ready;
  // #endregion attach

  requestFrame();
  onWindowResized(1024, 768);

  // #region service
  // An isolate can't service the session, because it can resume on another
  // thread after any await. Your native wake schedules
  // mln_render_session_service_driver_work on your graphics thread, with the
  // session's id.
  final graphicsThreadOptions = RenderSessionAttachOptions(
    driver: RenderDriverKind.callerGraphicsThread,
    frameWake: const Wake(callback: onFrameWake),
    driverWorkWake: Wake.native(callback: onDriverWorkWake),
  );
  // #endregion service

  // #region close
  paused = true;
  await session.close();
  destroySurface();
  // #endregion close
}
