// Snippets for the docs site's Render to a window page. They compile with the tests, which never
// run them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.FrameDemand
import org.maplibre.nativeffi.FrameDemandFlag
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderResult
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.VulkanContextDescriptor
import org.maplibre.nativeffi.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.Wake
import org.maplibre.nativeffi.attachVulkanSurface
import org.maplibre.nativeffi.setVulkanSurfaceTarget

// Stand-ins for the host's event loop, display timer, and graphics thread.
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
// Set while the window has no surface, and when it closes.
private var paused = false

private fun requestFrame() {
  session.requestFrame(
    FrameDemand(flags = FrameDemandFlag.PRESENT or FrameDemandFlag.WAIT_FOR_UPDATE),
  )
}

private fun drainFrameResults() {
  val results = session.drainFrameResults()
  if (results.isEmpty()) return

  if (results.any { it.disposition == RenderResult.FAILED }) {
    replaceSession() // Close the session and attach a new one.
  } else if (!paused) {
    runAtNextRefresh(::requestFrame)
  }
}

// #endregion frame-loop

// #region frame-wake
// Runs on a library thread.
private fun onFrameWake() {
  runOnEventLoop(::drainFrameResults)
}

// #endregion frame-wake

// #region resize
private fun onWindowResized(width: UInt, height: UInt) {
  // The scale factor the session has now.
  session.discarding.resize(LogicalExtent(width = width, height = height, scaleFactor = 2.0))
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
  paused = true
  session.releaseTarget().await()
  // #endregion lost-surface
}

private suspend fun surfaceRestored(vulkan: VulkanContextDescriptor, newSurface: ULong) {
  // #region lost-surface
  session
    .setVulkanSurfaceTarget(
      VulkanSurfaceDescriptor(
        extent = LogicalExtent(width = 800u, height = 600u, scaleFactor = 2.0),
        context = vulkan,
        surface = newSurface,
      ),
    )
    .await()
  paused = false
  requestFrame()
  // #endregion lost-surface
}

private suspend fun renderToWindow(
  map: MapHandle,
  vulkan: VulkanContextDescriptor,
  windowSurface: ULong,
) {
  // #region attach
  val attachment =
    map.attachVulkanSurface(
      VulkanSurfaceDescriptor(
        // The window's size in logical pixels, and its scale factor.
        extent = LogicalExtent(width = 800u, height = 600u, scaleFactor = 2.0),
        // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
        context = vulkan,
        // Your VkSurfaceKHR.
        surface = windowSurface,
      ),
      RenderSessionAttachOptions(
        driver = RenderDriverKind.CORE_WORKER,
        frameWake = Wake(::onFrameWake),
      ),
    )
  session = attachment.session
  attachment.ready.await()
  // #endregion attach

  requestFrame()
  onWindowResized(1024u, 768u)

  // #region service
  val graphicsThreadOptions =
    RenderSessionAttachOptions(
      driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
      frameWake = Wake(::onFrameWake),
      driverWorkWake = Wake(::onDriverWorkWake),
    )
  // #endregion service
  useOptions(graphicsThreadOptions)

  // #region close
  paused = true
  session.release().await()
  destroySurface() // For example, with vkDestroySurfaceKHR.
  // #endregion close
}
