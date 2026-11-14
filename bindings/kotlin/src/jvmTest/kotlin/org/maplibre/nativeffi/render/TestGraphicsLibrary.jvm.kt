package org.maplibre.nativeffi.render

import java.lang.foreign.Arena
import java.lang.foreign.FunctionDescriptor
import java.lang.foreign.Linker
import java.lang.foreign.MemoryLayout
import java.lang.foreign.MemoryLayout.PathElement.groupElement
import java.lang.foreign.MemorySegment
import java.lang.foreign.StructLayout
import java.lang.foreign.SymbolLookup
import java.lang.foreign.ValueLayout.ADDRESS
import java.lang.foreign.ValueLayout.JAVA_BOOLEAN
import java.lang.foreign.ValueLayout.JAVA_INT
import java.lang.foreign.ValueLayout.JAVA_LONG
import java.lang.invoke.MethodHandle

/** tests/graphics over FFM, loaded from the path the build passes in a system property. */
internal actual object TestGraphicsLibrary {
  private val linker = Linker.nativeLinker()
  private val library =
    SymbolLookup.libraryLookup(
      checkNotNull(System.getProperty("org.maplibre.nativeffi.test.graphics.library")) {
        "org.maplibre.nativeffi.test.graphics.library names no tests/graphics library"
      },
      Arena.global(),
    )
  private val lastErrorFunction =
    function("mln_test_graphics_last_error", FunctionDescriptor.of(ADDRESS))
  private val create =
    function("mln_test_graphics_create", FunctionDescriptor.of(ADDRESS, JAVA_INT))
  private val getContext =
    function("mln_test_graphics_get_context", FunctionDescriptor.of(JAVA_BOOLEAN, ADDRESS, ADDRESS))
  private val makeCurrent =
    function("mln_test_graphics_make_current", FunctionDescriptor.of(JAVA_BOOLEAN, ADDRESS))
  private val textureCreate =
    function(
      "mln_test_graphics_texture_create",
      FunctionDescriptor.of(ADDRESS, ADDRESS, JAVA_INT, JAVA_INT),
    )
  private val textureGetInfo =
    function(
      "mln_test_graphics_texture_get_info",
      FunctionDescriptor.of(JAVA_BOOLEAN, ADDRESS, ADDRESS),
    )
  private val textureDestroy =
    function("mln_test_graphics_texture_destroy", FunctionDescriptor.ofVoid(ADDRESS))

  actual suspend fun create(backend: UInt): Long {
    val graphics = create.invokeWithArguments(backend.toInt()) as MemorySegment
    check(graphics.address() != 0L) { "tests/graphics has no context: ${lastError()}" }
    return graphics.address()
  }

  actual fun context(graphics: Long): TestGraphicsContext =
    Arena.ofConfined().use { arena ->
      val context = arena.allocate(contextLayout)
      check(getContext.invokeWithArguments(MemorySegment.ofAddress(graphics), context) as Boolean) {
        "tests/graphics context lookup failed: ${lastError()}"
      }
      fun pointer(field: String): Long =
        context.get(ADDRESS, contextLayout.byteOffset(groupElement(field))).address()
      TestGraphicsContext(
        metalDevice = pointer("metal_device"),
        vulkanInstance = pointer("vulkan_instance"),
        vulkanPhysicalDevice = pointer("vulkan_physical_device"),
        vulkanDevice = pointer("vulkan_device"),
        vulkanQueue = pointer("vulkan_queue"),
        vulkanQueueFamilyIndex =
          context
            .get(JAVA_INT, contextLayout.byteOffset(groupElement("vulkan_queue_family_index")))
            .toUInt(),
        vulkanGetInstanceProcAddr = pointer("vulkan_get_instance_proc_addr"),
        vulkanGetDeviceProcAddr = pointer("vulkan_get_device_proc_addr"),
        eglDisplay = pointer("egl_display"),
        eglConfig = pointer("egl_config"),
        eglContext = pointer("egl_context"),
        wglDeviceContext = pointer("wgl_device_context"),
        wglContext = pointer("wgl_context"),
      )
    }

  actual fun makeCurrent(graphics: Long) {
    check(makeCurrent.invokeWithArguments(MemorySegment.ofAddress(graphics)) as Boolean) {
      "tests/graphics could not make its context current: ${lastError()}"
    }
  }

  actual fun createTexture(graphics: Long, width: Int, height: Int): TestTexture {
    val texture =
      textureCreate.invokeWithArguments(MemorySegment.ofAddress(graphics), width, height)
        as MemorySegment
    check(texture.address() != 0L) { "tests/graphics could not create a texture: ${lastError()}" }
    val destroy = { textureDestroy.invokeWithArguments(texture) }
    return try {
      Arena.ofConfined().use { arena ->
        // mln_test_graphics_texture_info: two uint64 fields, a pointer, then six uint32 fields.
        val metalOffset = 16L
        val fieldsOffset = metalOffset + ADDRESS.byteSize()
        val info = arena.allocate(fieldsOffset + 6L * JAVA_INT.byteSize(), 8)
        check(textureGetInfo.invokeWithArguments(texture, info) as Boolean) {
          "tests/graphics could not describe a texture: ${lastError()}"
        }
        fun field(index: Int): UInt = info.get(JAVA_INT, fieldsOffset + index * 4L).toUInt()
        TestTexture(
          width = field(0),
          height = field(1),
          format = field(2),
          metalTexture = info.get(ADDRESS, metalOffset).address(),
          vulkanImage = info.get(JAVA_LONG, 0).toULong(),
          vulkanImageView = info.get(JAVA_LONG, 8).toULong(),
          vulkanFinalLayout = field(3),
          openGlTexture = field(4),
          openGlTarget = field(5),
        ) {
          destroy()
        }
      }
    } catch (failure: Throwable) {
      destroy()
      throw failure
    }
  }

  private fun lastError(): String =
    (lastErrorFunction.invokeWithArguments() as MemorySegment)
      .reinterpret(Long.MAX_VALUE)
      .getString(0)

  private fun function(name: String, descriptor: FunctionDescriptor): MethodHandle =
    linker.downcallHandle(library.find(name).orElseThrow(), descriptor)

  /** mln_test_graphics_context, field for field, as tests/graphics/include declares it. */
  private val contextLayout: StructLayout =
    MemoryLayout.structLayout(
      *listOfNotNull(
          JAVA_INT.withName("backend"),
          JAVA_INT.withName("vulkan_queue_family_index"),
          ADDRESS.withName("metal_device"),
          ADDRESS.withName("vulkan_instance"),
          ADDRESS.withName("vulkan_physical_device"),
          ADDRESS.withName("vulkan_device"),
          ADDRESS.withName("vulkan_queue"),
          ADDRESS.withName("vulkan_get_instance_proc_addr"),
          ADDRESS.withName("vulkan_get_device_proc_addr"),
          ADDRESS.withName("vulkan_device_extensions"),
          JAVA_INT.withName("vulkan_device_extension_count"),
          // The padding that aligns egl_display after the count.
          (ADDRESS.byteAlignment() - JAVA_INT.byteSize())
            .takeIf { it > 0 }
            ?.let(MemoryLayout::paddingLayout),
          ADDRESS.withName("egl_display"),
          ADDRESS.withName("egl_config"),
          ADDRESS.withName("egl_context"),
          ADDRESS.withName("wgl_device_context"),
          ADDRESS.withName("wgl_context"),
          ADDRESS.withName("get_proc_address"),
        )
        .toTypedArray(),
    )
}
