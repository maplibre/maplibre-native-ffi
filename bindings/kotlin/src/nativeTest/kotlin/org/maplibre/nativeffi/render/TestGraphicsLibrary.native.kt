@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.render

import cnames.structs.mln_test_graphics
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.toCPointer
import kotlinx.cinterop.toKString
import kotlinx.cinterop.toLong
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_context
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_create
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_get_context
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_last_error
import org.maplibre.nativeffi.internal.graphics.mln_test_graphics_make_current

/** tests/graphics over cinterop, linked into the test executable from graphics.c. */
internal actual object TestGraphicsLibrary {
  actual fun create(backend: UInt): Long {
    val graphics =
      checkNotNull(mln_test_graphics_create(backend)) {
        "tests/graphics has no context: ${lastError()}"
      }
    return graphics.rawValue.toLong()
  }

  actual fun context(graphics: Long): TestGraphicsContext = memScoped {
    val context = alloc<mln_test_graphics_context>()
    check(mln_test_graphics_get_context(handle(graphics), context.ptr)) {
      "tests/graphics context lookup failed: ${lastError()}"
    }
    TestGraphicsContext(
      metalDevice = context.metal_device.toLong(),
      vulkanInstance = context.vulkan_instance.toLong(),
      vulkanPhysicalDevice = context.vulkan_physical_device.toLong(),
      vulkanDevice = context.vulkan_device.toLong(),
      vulkanQueue = context.vulkan_queue.toLong(),
      vulkanQueueFamilyIndex = context.vulkan_queue_family_index,
      vulkanGetInstanceProcAddr = context.vulkan_get_instance_proc_addr.toLong(),
      vulkanGetDeviceProcAddr = context.vulkan_get_device_proc_addr.toLong(),
      eglDisplay = context.egl_display.toLong(),
      eglConfig = context.egl_config.toLong(),
      eglContext = context.egl_context.toLong(),
      wglDeviceContext = context.wgl_device_context.toLong(),
      wglContext = context.wgl_context.toLong(),
    )
  }

  actual fun makeCurrent(graphics: Long) {
    check(mln_test_graphics_make_current(handle(graphics))) {
      "tests/graphics could not make its context current: ${lastError()}"
    }
  }

  private fun handle(graphics: Long): CPointer<mln_test_graphics> =
    checkNotNull(graphics.toCPointer())

  private fun lastError(): String = mln_test_graphics_last_error()?.toKString().orEmpty()
}
