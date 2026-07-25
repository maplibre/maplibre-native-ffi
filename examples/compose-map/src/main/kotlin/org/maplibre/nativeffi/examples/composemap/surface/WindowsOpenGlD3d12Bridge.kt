package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope
import java.util.concurrent.ExecutionException
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicReference
import org.jetbrains.skia.SurfaceColorFormat
import org.lwjgl.glfw.GLFW.GLFW_CLIENT_API
import org.lwjgl.glfw.GLFW.GLFW_CONTEXT_VERSION_MAJOR
import org.lwjgl.glfw.GLFW.GLFW_CONTEXT_VERSION_MINOR
import org.lwjgl.glfw.GLFW.GLFW_FALSE
import org.lwjgl.glfw.GLFW.GLFW_OPENGL_API
import org.lwjgl.glfw.GLFW.GLFW_RESIZABLE
import org.lwjgl.glfw.GLFW.GLFW_VISIBLE
import org.lwjgl.glfw.GLFW.glfwCreateWindow
import org.lwjgl.glfw.GLFW.glfwDefaultWindowHints
import org.lwjgl.glfw.GLFW.glfwDestroyWindow
import org.lwjgl.glfw.GLFW.glfwInit
import org.lwjgl.glfw.GLFW.glfwMakeContextCurrent
import org.lwjgl.glfw.GLFW.glfwTerminate
import org.lwjgl.glfw.GLFW.glfwWindowHint
import org.lwjgl.glfw.GLFWNativeWGL.glfwGetWGLContext
import org.lwjgl.glfw.GLFWNativeWin32.glfwGetWin32Window
import org.lwjgl.opengl.EXTMemoryObject.GL_DEDICATED_MEMORY_OBJECT_EXT
import org.lwjgl.opengl.EXTMemoryObject.GL_OPTIMAL_TILING_EXT
import org.lwjgl.opengl.EXTMemoryObject.GL_TEXTURE_TILING_EXT
import org.lwjgl.opengl.EXTMemoryObject.glCreateMemoryObjectsEXT
import org.lwjgl.opengl.EXTMemoryObject.glDeleteMemoryObjectsEXT
import org.lwjgl.opengl.EXTMemoryObject.glMemoryObjectParameteriEXT
import org.lwjgl.opengl.EXTMemoryObject.glTexStorageMem2DEXT
import org.lwjgl.opengl.EXTMemoryObjectWin32.GL_HANDLE_TYPE_D3D12_RESOURCE_EXT
import org.lwjgl.opengl.EXTMemoryObjectWin32.glImportMemoryWin32HandleEXT
import org.lwjgl.opengl.GL
import org.lwjgl.opengl.GL11.GL_LINEAR
import org.lwjgl.opengl.GL11.GL_NO_ERROR
import org.lwjgl.opengl.GL11.GL_RGBA8
import org.lwjgl.opengl.GL11.GL_TEXTURE_2D
import org.lwjgl.opengl.GL11.GL_TEXTURE_MAG_FILTER
import org.lwjgl.opengl.GL11.GL_TEXTURE_MIN_FILTER
import org.lwjgl.opengl.GL11.GL_TEXTURE_WRAP_S
import org.lwjgl.opengl.GL11.GL_TEXTURE_WRAP_T
import org.lwjgl.opengl.GL11.GL_TRUE
import org.lwjgl.opengl.GL11.glBindTexture
import org.lwjgl.opengl.GL11.glDeleteTextures
import org.lwjgl.opengl.GL11.glFinish
import org.lwjgl.opengl.GL11.glGenTextures
import org.lwjgl.opengl.GL11.glGetError
import org.lwjgl.opengl.GL11.glTexParameteri
import org.lwjgl.opengl.GL12.GL_CLAMP_TO_EDGE
import org.lwjgl.opengl.WGL
import org.lwjgl.opengl.WGLNVGPUAffinity
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.system.windows.User32

internal class WindowsOpenGlD3d12Bridge : NativeSurfaceBridge {
  private val producerThreadRef = AtomicReference<Thread?>()
  private val producerExecutor = Executors.newSingleThreadExecutor { task ->
    Thread(task, "compose-map-windows-wgl-producer").also {
      it.isDaemon = true
      producerThreadRef.set(it)
    }
  }
  private var wgl: WindowsWglContext? = null
  private var direct3DTexture = NativeHandle(0)
  private var producerTexture: WindowsWglImportedD3D12Texture? = null
  private var generation = 0L
  private var currentExtent = SurfaceExtent.Empty

  override val backend: ProducerBackend = ProducerBackend.OPENGL

  override val consumerBackend: ConsumerBackend = ConsumerBackend.DIRECT3D12

  override val capabilities: NativeSurfaceCapabilities =
    NativeSurfaceCapabilities(
      producerBackend = backend,
      consumerBackend = consumerBackend,
      supportsExplicitSynchronization = false,
      supportsResizeWithoutRecreate = false,
    )

  override fun resize(extent: SurfaceExtent) {
    val device = if (extent.isEmpty) null else SkikoHost.requireDirect3DDevice()
    runOnProducerThread { resizeOnProducerThread(extent, device) }
  }

  private fun resizeOnProducerThread(extent: SurfaceExtent, device: SkikoDirect3DDevice? = null) {
    if (extent == currentExtent && producerTexture != null) {
      return
    }
    recreateTexture(extent, device)
    currentExtent = extent
    generation += 1
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame {
    if (producerTexture == null || extent != currentExtent) {
      resize(extent)
    }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    runOnProducerThread { wgl?.waitIdle() }
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    runOnProducerThread(action)

  override fun <T> withRendererAccess(action: () -> T): T = runOnProducerThread(action)

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is OpenGlTextureTarget || direct3DTexture.address == 0L) {
      return false
    }
    return SkikoHost.drawDirect3DTexture(
      scope,
      Direct3DTextureTarget(
        texture = direct3DTexture,
        format = WindowsD3D12Interop.DXGI_FORMAT_R8G8B8A8_UNORM,
        colorFormat = SurfaceColorFormat.RGBA_8888,
        origin = TextureOrigin.BOTTOM_LEFT,
        extent = target.extent,
        generation = target.generation,
      ),
    )
  }

  override fun close() {
    try {
      disposeTexture()
    } finally {
      try {
        runOnProducerThread {
          wgl?.close()
          wgl = null
        }
      } finally {
        producerExecutor.shutdown()
      }
    }
  }

  private fun target(generation: Long): NativeSurfaceTarget =
    checkNotNull(producerTexture) { "Windows WGL texture is not initialized" }.target(generation)

  private fun recreateTexture(extent: SurfaceExtent, device: SkikoDirect3DDevice? = null) {
    if (extent.isEmpty) {
      disposeTexture()
      return
    }

    val direct3DDevice = device ?: SkikoHost.requireDirect3DDevice()
    disposeTexture()
    direct3DTexture =
      WindowsD3D12Interop.createSharedTexture(
        direct3DDevice,
        extent,
        dxgiFormat = WindowsD3D12Interop.DXGI_FORMAT_R8G8B8A8_UNORM,
      )
    val memorySize =
      WindowsD3D12Interop.textureMemorySize(
        direct3DTexture,
        extent,
        dxgiFormat = WindowsD3D12Interop.DXGI_FORMAT_R8G8B8A8_UNORM,
      )
    var sharedHandle = NULL
    try {
      sharedHandle = WindowsD3D12Interop.createSharedHandle(direct3DTexture)
      producerTexture = importTexture(sharedHandle, memorySize, extent)
    } catch (error: RuntimeException) {
      disposeTexture()
      throw error
    } finally {
      WindowsD3D12Interop.closeSharedHandle(sharedHandle)
    }
  }

  private fun importTexture(
    sharedHandle: Long,
    memorySize: Long,
    extent: SurfaceExtent,
  ): WindowsWglImportedD3D12Texture {
    val currentContext = wgl
    if (currentContext != null) {
      currentContext.tryImportD3D12(sharedHandle, memorySize, extent)?.let {
        return it
      }
      currentContext.close()
      wgl = null
    }

    val selectedImport = WindowsWglContext.createCompatibleImport(sharedHandle, memorySize, extent)
    wgl = selectedImport.context
    return selectedImport.texture
  }

  private fun disposeTexture() {
    val oldProducerTexture = producerTexture
    producerTexture = null
    if (oldProducerTexture != null) {
      runOnProducerThread { oldProducerTexture.close() }
    }
    if (direct3DTexture.address != 0L) {
      SkikoHost.forgetDirect3DTexture(direct3DTexture)
      WindowsD3D12Interop.release(direct3DTexture)
      direct3DTexture = NativeHandle(0)
    }
  }

  private fun <T> runOnProducerThread(action: () -> T): T {
    if (Thread.currentThread() == producerThreadRef.get()) {
      return action()
    }
    return try {
      producerExecutor.submit<T> { action() }.get()
    } catch (error: ExecutionException) {
      throw error.cause ?: error
    }
  }
}

private class WindowsWglContext
private constructor(private val kind: Kind, private val label: String) : AutoCloseable {
  private var window = NULL
  private var hwnd = NULL
  private var deviceContext = NULL
  private var shareContext = NULL
  private var initialized = false
  private var capabilitiesInitialized = false
  private var glfwRuntimeAcquired = false

  val handles: WglContextHandles
    get() =
      WglContextHandles(
        deviceContext = NativeHandle(deviceContext),
        shareContext = NativeHandle(shareContext),
        // Null makes the native WGL descriptor resolve entry points from the current context.
        getProcAddress = NativeHandle(0),
      )

  fun makeCurrent() {
    check(deviceContext != NULL && shareContext != NULL) {
      "Windows WGL context is not initialized"
    }
    when (kind) {
      Kind.GLFW -> {
        check(window != NULL) { "Windows GLFW WGL context is missing its window" }
        glfwMakeContextCurrent(window)
      }
      Kind.NV_AFFINITY -> {
        check(WGL.nwglMakeCurrent(NULL, deviceContext, shareContext) != 0) {
          "wglMakeCurrent failed for $label"
        }
      }
    }
    if (!capabilitiesInitialized) {
      GL.setCapabilities(null)
      GL.createCapabilities()
      capabilitiesInitialized = true
    } else {
      ensureLwjglOpenGlCapabilities()
    }
  }

  fun waitIdle() {
    if (deviceContext != NULL && shareContext != NULL) {
      makeCurrent()
      glFinish()
    }
  }

  fun tryImportD3D12(
    sharedHandle: Long,
    memorySize: Long,
    extent: SurfaceExtent,
  ): WindowsWglImportedD3D12Texture? =
    try {
      WindowsWglImportedD3D12Texture.create(this, sharedHandle, memorySize, extent)
    } catch (_: RuntimeException) {
      null
    }

  override fun close() {
    if (initialized) {
      runCatching { waitIdle() }
      initialized = false
    }
    GL.setCapabilities(null)
    when (kind) {
      Kind.GLFW -> {
        if (window != NULL) {
          glfwMakeContextCurrent(NULL)
        }
        if (hwnd != NULL && deviceContext != NULL) {
          User32.ReleaseDC(hwnd, deviceContext)
        }
        if (window != NULL) {
          glfwDestroyWindow(window)
        }
        if (glfwRuntimeAcquired) {
          WindowsGlfwRuntime.release()
          glfwRuntimeAcquired = false
        }
      }
      Kind.NV_AFFINITY -> {
        if (shareContext != NULL) {
          WGL.nwglDeleteContext(NULL, shareContext)
        }
        if (deviceContext != NULL) {
          WGLNVGPUAffinity.wglDeleteDCNV(deviceContext)
        }
      }
    }
    window = NULL
    hwnd = NULL
    deviceContext = NULL
    shareContext = NULL
    capabilitiesInitialized = false
  }

  private fun createDefault() {
    WindowsGlfwRuntime.acquire()
    glfwRuntimeAcquired = true
    glfwDefaultWindowHints()
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0)
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE)
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE)
    window = glfwCreateWindow(8, 8, "MapLibre Compose WGL", NULL, NULL)
    check(window != NULL) { "GLFW WGL window creation failed" }
    glfwMakeContextCurrent(window)
    ensureLwjglOpenGlCapabilities()
    hwnd = glfwGetWin32Window(window)
    shareContext = glfwGetWGLContext(window)
    deviceContext = User32.GetDC(hwnd)
    check(hwnd != NULL && deviceContext != NULL && shareContext != NULL) {
      "GLFW did not expose complete WGL handles"
    }
    initialized = true
  }

  private fun createNvAffinity(gpu: Long) {
    MemoryStack.stackPush().use { stack ->
      val affinityMask = stack.mallocPointer(2)
      affinityMask.put(0, gpu)
      affinityMask.put(1, NULL)
      deviceContext = WGLNVGPUAffinity.wglCreateAffinityDCNV(affinityMask)
      check(deviceContext != NULL) { "wglCreateAffinityDCNV failed for $label" }
      shareContext = WGL.nwglCreateContext(NULL, deviceContext)
      check(shareContext != NULL) { "wglCreateContext failed for $label" }
      initialized = true
      makeCurrent()
    }
  }

  private fun createNvAffinityCandidate(
    gpuIndex: Int,
    gpu: Long,
    sharedHandle: Long,
    memorySize: Long,
    extent: SurfaceExtent,
  ): WindowsWglImport? {
    val candidate = WindowsWglContext(Kind.NV_AFFINITY, "NVIDIA affinity GPU $gpuIndex")
    var selected = false
    return try {
      candidate.createNvAffinity(gpu)
      val texture = candidate.tryImportD3D12(sharedHandle, memorySize, extent)
      if (texture != null) {
        selected = true
        WindowsWglImport(candidate, texture)
      } else {
        null
      }
    } catch (_: RuntimeException) {
      null
    } finally {
      if (!selected) {
        candidate.close()
      }
    }
  }

  companion object {
    fun createCompatibleImport(
      sharedHandle: Long,
      memorySize: Long,
      extent: SurfaceExtent,
    ): WindowsWglImport {
      val context = WindowsWglContext(Kind.GLFW, "default GLFW WGL context")
      try {
        context.createDefault()
      } catch (error: RuntimeException) {
        context.close()
        throw error
      }
      val texture = context.tryImportD3D12(sharedHandle, memorySize, extent)
      if (texture != null) {
        return WindowsWglImport(context, texture)
      }

      val affinityContext =
        try {
          context.findNvAffinityContext(sharedHandle, memorySize, extent)
        } finally {
          context.close()
        }
      return affinityContext
        ?: throw NativeSurfaceBridgeException(
          "No WGL context could import the Skiko D3D12 shared texture; " +
            "the OpenGL producer context must use the same graphics adapter as Skiko Direct3D"
        )
    }
  }

  private fun findNvAffinityContext(
    sharedHandle: Long,
    memorySize: Long,
    extent: SurfaceExtent,
  ): WindowsWglImport? {
    makeCurrent()
    val wglCapabilities =
      runCatching { GL.getCapabilitiesWGL() }.getOrElse { GL.createCapabilitiesWGL() }
    if (!wglCapabilities.WGL_NV_gpu_affinity) {
      return null
    }
    MemoryStack.stackPush().use { stack ->
      val gpuOut = stack.mallocPointer(1)
      var index = 0
      while (true) {
        makeCurrent()
        if (!WGLNVGPUAffinity.wglEnumGpusNV(index, gpuOut)) {
          return null
        }
        val gpu = gpuOut[0]
        createNvAffinityCandidate(index, gpu, sharedHandle, memorySize, extent)?.let {
          return it
        }
        index += 1
      }
    }
  }

  private enum class Kind {
    GLFW,
    NV_AFFINITY,
  }
}

private object WindowsGlfwRuntime {
  private var references = 0

  @Synchronized
  fun acquire() {
    if (references == 0) {
      check(glfwInit()) { "GLFW initialization failed for Windows WGL bridge" }
    }
    references += 1
  }

  @Synchronized
  fun release() {
    check(references > 0) { "Windows GLFW runtime was released without a matching acquire" }
    references -= 1
    if (references == 0) {
      glfwTerminate()
    }
  }
}

private data class WindowsWglImport(
  val context: WindowsWglContext,
  val texture: WindowsWglImportedD3D12Texture,
)

private class WindowsWglImportedD3D12Texture
private constructor(
  private val context: WindowsWglContext,
  private val sharedHandle: Long,
  private val memorySize: Long,
  private val extent: SurfaceExtent,
) : AutoCloseable {
  private var memoryObject = 0
  private var textureName = 0

  fun target(generation: Long): OpenGlTextureTarget =
    OpenGlTextureTarget(
      context = context.handles,
      textureName = textureName,
      textureTarget = GL_TEXTURE_2D,
      format = GL_RGBA8,
      origin = TextureOrigin.BOTTOM_LEFT,
      contextProvider = OpenGlContextProvider { context.makeCurrent() },
      extent = extent,
      generation = generation,
    )

  private fun create() {
    context.makeCurrent()
    val capabilities = ensureLwjglOpenGlCapabilities()
    check(capabilities.GL_EXT_memory_object) {
      "Windows WGL context does not expose GL_EXT_memory_object"
    }
    check(capabilities.GL_EXT_memory_object_win32) {
      "Windows WGL context does not expose GL_EXT_memory_object_win32"
    }

    memoryObject = glCreateMemoryObjectsEXT()
    glMemoryObjectParameteriEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT, GL_TRUE)
    glImportMemoryWin32HandleEXT(
      memoryObject,
      memorySize,
      GL_HANDLE_TYPE_D3D12_RESOURCE_EXT,
      sharedHandle,
    )
    checkGl("glImportMemoryWin32HandleEXT")

    textureName = glGenTextures()
    glBindTexture(GL_TEXTURE_2D, textureName)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_OPTIMAL_TILING_EXT)
    glTexStorageMem2DEXT(
      GL_TEXTURE_2D,
      1,
      GL_RGBA8,
      extent.physicalWidth,
      extent.physicalHeight,
      memoryObject,
      0,
    )
    glBindTexture(GL_TEXTURE_2D, 0)
    checkGl("glTexStorageMem2DEXT")
  }

  override fun close() {
    context.makeCurrent()
    glFinish()
    if (textureName != 0) {
      glDeleteTextures(textureName)
      textureName = 0
    }
    if (memoryObject != 0) {
      glDeleteMemoryObjectsEXT(memoryObject)
      memoryObject = 0
    }
  }

  companion object {
    fun create(
      context: WindowsWglContext,
      sharedHandle: Long,
      memorySize: Long,
      extent: SurfaceExtent,
    ): WindowsWglImportedD3D12Texture {
      val texture = WindowsWglImportedD3D12Texture(context, sharedHandle, memorySize, extent)
      try {
        texture.create()
        return texture
      } catch (error: RuntimeException) {
        texture.close()
        throw error
      }
    }
  }
}

private fun checkGl(operation: String) {
  val error = glGetError()
  check(error == GL_NO_ERROR) { "$operation failed with GL error 0x${error.toString(16)}" }
}
