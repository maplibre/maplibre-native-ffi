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

function demandFrame(): void {
  session.demandFrame({
    flags: FrameDemandFlag.waitForUpdate,
  });
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
    replaceSession(); // Close the session and attach a new one.
  } else if (!paused) {
    atNextRefresh(demandFrame);
  }
}
// #endregion frame-loop

// #region frame-wake
// The binding calls the wake on the JS
// thread, so it can drain right away.
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
  paused = true;
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
  paused = false;
  demandFrame();
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
      frameWake: { callback: onFrameWake },
    },
  );
  session = attachment.session;
  await attachment.ready;
  // #endregion attach

  demandFrame();
  onWindowResized(1024, 768);

  // #region service
  const graphicsThreadOptions: RenderSessionAttachOptions = {
    driver: RenderDriverKind.callerGraphicsThread,
    frameWake: { callback: onFrameWake },
    driverWorkWake: { callback: onDriverWorkWake },
  };
  // #endregion service
  void graphicsThreadOptions;

  paused = true;
  await session.close();
  destroySurface();
}
