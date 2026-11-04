// Snippets for the docs site's Render to a window page. They compile
// with the tests, which never
// run them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.FrameDemand
import org.maplibre.nativeffi.LogicalSize
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderResult
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.VulkanContextDescriptor
import org.maplibre.nativeffi.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.VulkanSurfaceReplacement
import org.maplibre.nativeffi.Wake
import org.maplibre.nativeffi.attachVulkanSurface
import org.maplibre.nativeffi.setVulkanSurfaceTarget

// Stand-ins for the host's event loop, display timer, and graphics
// thread.
private fun runOnEventLoop(task: () -> Unit) {}

private fun runAtNextRefresh(task: () -> Unit) {}

private fun runOnGraphicsThread(task: () -> Unit) {}

private fun replaceSession() {}

private fun destroySurface() {}

private fun useOptions(options: RenderSessionAttachOptions) {}

// #region attach
private lateinit var session: RenderSessionHandle

// #endregion attach

// #region frame-loop
// Draws the map if it changed since the last frame.
private fun demandFrame() {
  session.demandFrame(FrameDemand())
}

private fun drainFrameResults() {
  val results = session.drainFrameResults()
  if (results.isEmpty()) return

  if (results.any { it.disposition == RenderResult.FAILED }) {
    replaceSession() // Release the session and attach a new one.
  }
}

// #endregion frame-loop

// #region frame-wake
// Both run on a library thread.
private fun onRedrawWake() {
  runAtNextRefresh(::demandFrame)
}

private fun onFrameWake() {
  runOnEventLoop(::drainFrameResults)
}

// #endregion frame-wake

// #region resize
private fun onWindowResized(width: UInt, height: UInt) {
  // The window's new size in logical pixels.
  session.discarding.resize(
    LogicalSize(width = width, height = height),
  )
}

// #endregion resize

// #region service
// Your graphics context is current on this thread.
private fun serviceSession() {
  session.serviceDriverWork()
}

// Runs on a library thread.
private fun onDriverWorkWake() {
  runOnGraphicsThread(::serviceSession)
}

// #endregion service

private suspend fun surfaceLost() {
  // #region lost-surface
  session.releaseTarget().await()
  // #endregion lost-surface
}

private suspend fun surfaceRestored(newSurface: ULong) {
  // #region lost-surface
  session
    .setVulkanSurfaceTarget(
      VulkanSurfaceReplacement(
        logicalSize = LogicalSize(width = 800u, height = 600u),
        surface = newSurface,
      ),
    )
    .await()
  // #endregion lost-surface
}

private suspend fun renderToWindow(
  map: MapHandle,
  vulkan: VulkanContextDescriptor,
  windowSurface: ULong,
) {
  // #region attach
  session =
    map.attachVulkanSurface(
      VulkanSurfaceDescriptor(
        // The window's size in logical pixels. The map's scale factor
        // turns it into pixels.
        logicalSize = LogicalSize(width = 800u, height = 600u),
        // Your VkInstance, VkPhysicalDevice,
        // VkDevice, and graphics VkQueue.
        context = vulkan,
        // Your VkSurfaceKHR.
        surface = windowSurface,
      ),
      RenderSessionAttachOptions(
        driver = RenderDriverKind.CORE_WORKER,
        redrawWake = Wake(::onRedrawWake),
        frameWake = Wake(::onFrameWake),
      ),
    )
  session.ready.await()
  // #endregion attach

  onWindowResized(1024u, 768u)

  // #region service
  val graphicsThreadOptions =
    RenderSessionAttachOptions(
      driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
      redrawWake = Wake(::onRedrawWake),
      frameWake = Wake(::onFrameWake),
      driverWorkWake = Wake(::onDriverWorkWake),
    )
  // #endregion service
  useOptions(graphicsThreadOptions)

  session.release().await()
  destroySurface() // For example, with vkDestroySurfaceKHR.
}
