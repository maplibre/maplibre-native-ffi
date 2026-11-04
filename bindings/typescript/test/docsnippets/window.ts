// Snippets for the docs site's Render to a window page. They are type-checked
// with the tests, which never run them.

import {
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
// Draws the map if it changed since the last frame.
function demandFrame(): void {
  session.demandFrame({});
}

function drainFrameResults(): void {
  const results = session.drainFrameResults();
  if (results.length === 0) {
    return;
  }
  if (
    results.some(
      ({ disposition }) => disposition === RenderResult.failed,
    )
  ) {
    replaceSession(); // Release the session and attach a new one.
  }
}
// #endregion frame-loop

// #region frame-wake
// The binding calls both wakes on the JS thread,
// so the frame wake can drain right away.
function onRedrawWake(): void {
  atNextRefresh(demandFrame);
}

function onFrameWake(): void {
  drainFrameResults();
}
// #endregion frame-wake

// #region resize
function onWindowResized(width: number, height: number): void {
  // The window's new size in logical pixels.
  session.discarding.resize({ width, height });
}
// #endregion resize

// #region service
// The binding calls the wake on the JS thread. Your graphics context
// is current there.
function onDriverWorkWake(): void {
  session.serviceDriverWork();
}
// #endregion service

export async function surfaceLost(): Promise<void> {
  // #region lost-surface
  await session.releaseTarget();
  // #endregion lost-surface
}

export async function surfaceRestored(
  newSurface: bigint,
): Promise<void> {
  // #region lost-surface
  await session.setVulkanSurfaceTarget({
    logicalSize: { width: 800, height: 600 },
    surface: newSurface,
  });
  // #endregion lost-surface
}

export async function renderToWindow(
  map: MapHandle,
  vulkan: VulkanContextDescriptor,
  windowSurface: bigint,
): Promise<void> {
  // #region attach
  session = map.attachVulkanSurface(
    {
      // The window's size in logical pixels. The map's scale factor turns
      // it into pixels.
      logicalSize: { width: 800, height: 600 },
      // Your VkInstance, VkPhysicalDevice,
      // VkDevice, and graphics VkQueue.
      context: vulkan,
      // Your VkSurfaceKHR.
      surface: windowSurface,
    },
    {
      driver: RenderDriverKind.coreWorker,
      redrawWake: { callback: onRedrawWake },
      frameWake: { callback: onFrameWake },
    },
  );
  await session.ready;
  // #endregion attach

  onWindowResized(1024, 768);

  // #region service
  const graphicsThreadOptions: RenderSessionAttachOptions = {
    driver: RenderDriverKind.callerGraphicsThread,
    redrawWake: { callback: onRedrawWake },
    frameWake: { callback: onFrameWake },
    driverWorkWake: { callback: onDriverWorkWake },
  };
  // #endregion service
  void graphicsThreadOptions;

  await session.release();
  destroySurface();
}
