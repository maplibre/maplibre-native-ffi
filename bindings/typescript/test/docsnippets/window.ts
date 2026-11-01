// Snippets for the docs site's Render to a window page. They are type-checked
// with the tests, which never run them.

import {
  FrameDemandFlag,
  type MapHandle,
  RenderDriverKind,
  RenderResult,
  type RenderSessionAttachOptions,
  type RenderSessionHandle,
  type VulkanContextDescriptor,
} from "../../src/index.ts";

// Stand-ins for the host's display timer and session replacement.
function atNextRefresh(_task: () => void): void {}

function replaceSession(): void {}

function destroySurface(): void {}

// #region attach
let session: RenderSessionHandle;
// #endregion attach

// #region frame-loop
// Set while the window has no surface, and when it closes.
let paused = false;

function requestFrame(): void {
  session.requestFrame({
    flags: FrameDemandFlag.present | FrameDemandFlag.waitForUpdate,
  });
}

function drainFrameResults(): void {
  const results = session.drainFrameResults();
  if (results.length === 0) {
    return;
  }
  if (results.some(({ disposition }) => disposition === RenderResult.failed)) {
    replaceSession(); // Close the session and attach a new one.
  } else if (!paused) {
    atNextRefresh(requestFrame);
  }
}
// #endregion frame-loop

// #region frame-wake
// The binding calls the wake on the JS thread, so it can drain right away.
function onFrameWake(): void {
  drainFrameResults();
}
// #endregion frame-wake

// #region resize
function onWindowResized(width: number, height: number): void {
  // The scale factor the session has now.
  session.discarding.resize({ width, height, scaleFactor: 2 });
}
// #endregion resize

// #region service
// The binding calls the wake on the JS thread. Your graphics context is current
// there.
function onDriverWorkWake(): void {
  session.serviceDriverWork();
}
// #endregion service

export async function surfaceLost(): Promise<void> {
  // #region lost-surface
  paused = true;
  await session.releaseTarget();
  // #endregion lost-surface
}

export async function surfaceRestored(
  vulkan: VulkanContextDescriptor,
  newSurface: bigint,
): Promise<void> {
  // #region lost-surface
  await session.setVulkanSurfaceTarget({
    extent: { width: 800, height: 600, scaleFactor: 2 },
    context: vulkan,
    surface: newSurface,
  });
  paused = false;
  requestFrame();
  // #endregion lost-surface
}

export async function renderToWindow(
  map: MapHandle,
  vulkan: VulkanContextDescriptor,
  windowSurface: bigint,
): Promise<void> {
  // #region attach
  const attachment = map.attachVulkanSurface(
    {
      // The window's size in logical pixels, and its scale factor.
      extent: { width: 800, height: 600, scaleFactor: 2 },
      // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
      context: vulkan,
      // Your VkSurfaceKHR.
      surface: windowSurface,
    },
    { driver: RenderDriverKind.coreWorker, frameWake: onFrameWake },
  );
  session = attachment.session;
  await attachment.ready;
  // #endregion attach

  requestFrame();
  onWindowResized(1024, 768);

  // #region service
  const graphicsThreadOptions: RenderSessionAttachOptions = {
    driver: RenderDriverKind.callerGraphicsThread,
    frameWake: onFrameWake,
    driverWorkWake: onDriverWorkWake,
  };
  // #endregion service
  void graphicsThreadOptions;

  // #region close
  paused = true;
  await session.close();
  destroySurface();
  // #endregion close
}
