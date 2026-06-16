package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import org.maplibre.nativeffi.internal.c.mln_texture_image_info
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.TextureImageInfo
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor

@OptIn(ExperimentalForeignApi::class)
class RenderStructsTest {
  @Test
  fun renderBackendMasksRoundTrip() {
    assertEquals(
      setOf(RenderBackend.METAL, RenderBackend.VULKAN, RenderBackend.OPENGL),
      RenderBackend.fromMask(7U),
    )
    assertEquals(
      setOf(OpenGLContextProvider.WGL, OpenGLContextProvider.EGL),
      OpenGLContextProvider.fromMask(3U),
    )
    assertEquals(emptySet(), RenderBackend.fromMask(0U))
  }

  @Test
  fun textureImageInfoCopiesMetadataAndRejectsOversizedLengths() {
    memScoped {
      val info = alloc<mln_texture_image_info>()
      info.width = 2U
      info.height = 3U
      info.stride = 8U
      info.byte_length = 24UL

      assertEquals(TextureImageInfo(2, 3, 8, 24), RenderStructs.textureImageInfo(info))

      info.byte_length = Long.MAX_VALUE.toULong() + 1UL
      assertFailsWith<IllegalArgumentException> { RenderStructs.textureImageInfo(info) }
    }
  }

  @Test
  fun metalDescriptorsMaterializeOpaquePointersAndExtents() {
    memScoped {
      val extent = RenderTargetExtent(640, 480, 2.0)
      val owned =
        RenderStructs.metalOwnedTextureDescriptor(
            MetalOwnedTextureDescriptor(
              extent,
              MetalContextDescriptor(NativePointer.ofAddress(0x10L)),
            ),
            this,
          )
          .pointed
      assertEquals(640U, owned.extent.width)
      assertEquals(2.0, owned.extent.scale_factor)
      assertFalse(owned.context.device == null)

      val borrowed =
        RenderStructs.metalBorrowedTextureDescriptor(
            MetalBorrowedTextureDescriptor(extent, NativePointer.ofAddress(0x20L)),
            this,
          )
          .pointed
      assertFalse(borrowed.texture == null)

      val surface =
        RenderStructs.metalSurfaceDescriptor(
            MetalSurfaceDescriptor(
              extent,
              MetalContextDescriptor(NativePointer.NULL),
              NativePointer.ofAddress(0x30L),
            ),
            this,
          )
          .pointed
      assertFalse(surface.layer == null)
    }
  }

  @Test
  fun vulkanDescriptorsMaterializeContextPointersAndOptionalFinalLayout() {
    memScoped {
      val context =
        VulkanContextDescriptor(
          NativePointer.ofAddress(0x10L),
          NativePointer.ofAddress(0x20L),
          NativePointer.ofAddress(0x30L),
          NativePointer.ofAddress(0x40L),
          7,
          NativePointer.ofAddress(0x41L),
          NativePointer.ofAddress(0x42L),
        )
      val borrowed =
        RenderStructs.vulkanBorrowedTextureDescriptor(
            VulkanBorrowedTextureDescriptor(
                RenderTargetExtent(256, 256, 1.0),
                context,
                NativePointer.ofAddress(0x50L),
                NativePointer.ofAddress(0x60L),
                44,
                1,
              )
              .apply { finalLayout = 2 },
            this,
          )
          .pointed
      assertFalse(borrowed.context.instance == null)
      assertEquals(7U, borrowed.context.graphics_queue_family_index)
      assertFalse(borrowed.context.get_instance_proc_addr == null)
      assertFalse(borrowed.context.get_device_proc_addr == null)
      assertEquals(44U, borrowed.format)
      assertEquals(2U, borrowed.final_layout)

      val surface =
        RenderStructs.vulkanSurfaceDescriptor(
            VulkanSurfaceDescriptor(
              RenderTargetExtent(256, 256, 1.0),
              context,
              NativePointer.ofAddress(0x70L),
            ),
            this,
          )
          .pointed
      assertFalse(surface.surface == null)
    }
  }

  @Test
  fun openglDescriptorsMaterializeWglAndEglContexts() {
    memScoped {
      val extent = RenderTargetExtent(320, 240, 1.5)
      val wglContext =
        org.maplibre.nativeffi.render.WglContextDescriptor(
          NativePointer.ofAddress(0x10L),
          NativePointer.ofAddress(0x20L),
          NativePointer.ofAddress(0x30L),
        )
      val owned =
        RenderStructs.openglOwnedTextureDescriptor(
            OpenGLOwnedTextureDescriptor(extent, wglContext),
            this,
          )
          .pointed
      assertEquals(320U, owned.extent.width)
      assertFalse(owned.context.data.wgl.device_context == null)
      assertFalse(owned.context.data.wgl.share_context == null)
      assertFalse(owned.context.data.wgl.get_proc_address == null)

      val borrowed =
        RenderStructs.openglBorrowedTextureDescriptor(
            OpenGLBorrowedTextureDescriptor(extent, wglContext, texture = 17, target = 3553),
            this,
          )
          .pointed
      assertEquals(17U, borrowed.texture)
      assertEquals(3553U, borrowed.target)

      val eglContext =
        EglContextDescriptor(
          NativePointer.ofAddress(0x40L),
          NativePointer.ofAddress(0x50L),
          NativePointer.ofAddress(0x60L),
          NativePointer.ofAddress(0x70L),
        )
      val surface =
        RenderStructs.openglSurfaceDescriptor(
            OpenGLSurfaceDescriptor(extent, eglContext, NativePointer.ofAddress(0x80L)),
            this,
          )
          .pointed
      assertFalse(surface.context.data.egl.display == null)
      assertFalse(surface.context.data.egl.config == null)
      assertFalse(surface.context.data.egl.share_context == null)
      assertFalse(surface.context.data.egl.get_proc_address == null)
      assertFalse(surface.surface == null)
    }
  }
}
