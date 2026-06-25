package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.skiaCanvas
import java.awt.Component
import java.awt.Container
import java.awt.Window
import javax.swing.SwingUtilities
import org.jetbrains.skia.BackendRenderTarget
import org.jetbrains.skia.ContentChangeMode
import org.jetbrains.skia.DirectContext
import org.jetbrains.skia.FramebufferFormat
import org.jetbrains.skia.Image
import org.jetbrains.skia.Rect
import org.jetbrains.skia.SamplingMode
import org.jetbrains.skia.Surface
import org.jetbrains.skia.SurfaceColorFormat
import org.jetbrains.skia.SurfaceOrigin
import org.lwjgl.opengl.GL
import org.lwjgl.opengl.GL11.GL_NO_ERROR
import org.lwjgl.opengl.GL11.glGetError
import org.lwjgl.opengl.GL30.GL_COLOR_ATTACHMENT0
import org.lwjgl.opengl.GL30.GL_FRAMEBUFFER
import org.lwjgl.opengl.GL30.GL_FRAMEBUFFER_BINDING
import org.lwjgl.opengl.GL30.GL_FRAMEBUFFER_COMPLETE
import org.lwjgl.opengl.GL30.glBindFramebuffer
import org.lwjgl.opengl.GL30.glCheckFramebufferStatus
import org.lwjgl.opengl.GL30.glDeleteFramebuffers
import org.lwjgl.opengl.GL30.glFramebufferTexture2D
import org.lwjgl.opengl.GL30.glGenFramebuffers
import org.lwjgl.opengl.GL30.glGetInteger

internal object SkikoHost {
  private const val SKIA_LAYER_CLASS = "org.jetbrains.skiko.SkiaLayer"
  private const val COMPOSE_WINDOW_CLASS = "androidx.compose.ui.awt.ComposeWindow"
  private const val METAL_REDRAWER_CLASS = "org.jetbrains.skiko.redrawer.MetalRedrawer"
  private const val DIRECT3D_REDRAWER_CLASS = "org.jetbrains.skiko.redrawer.Direct3DRedrawer"
  private const val LINUX_OPENGL_REDRAWER_CLASS = "org.jetbrains.skiko.redrawer.LinuxOpenGLRedrawer"
  private const val LINUX_OPENGL_REDRAWER_HELPERS_CLASS =
    "org.jetbrains.skiko.redrawer.LinuxOpenGLRedrawerKt"
  private const val AWT_LINUX_DRAWING_SURFACE_HELPERS_CLASS =
    "org.jetbrains.skiko.AWTLinuxDrawingSurfaceKt"
  private const val RETAINED_IMAGE_COUNT = 8

  private val metalPresenters = mutableMapOf<Long, MetalTexturePresenter>()
  private val direct3DPresenters = mutableMapOf<Long, Direct3DTexturePresenter>()
  private val openGlPresenters = mutableMapOf<Int, OpenGlTexturePresenter>()

  fun requireMetalDevice(): SkikoMetalDevice = onEdt {
    val layer =
      findSkiaLayer()
        ?: throw NativeSurfaceBridgeException(
          "SkikoHost could not find a live $SKIA_LAYER_CLASS. ${describeWindows()}"
        )
    val contextHandler = requireMetalContextHandler(layer)
    val device =
      contextHandler.getField("device")
        ?: throw NativeSurfaceBridgeException(
          "${contextHandler.javaClass.name}.device was null; Skiko has not created the Metal device yet"
        )
    val ptr =
      when (device) {
        is Long -> device
        else ->
          device.getField("ptr") as? Long
            ?: device.invokeNoArg("getPtr") as? Long
            ?: throw NativeSurfaceBridgeException(
              "${device.javaClass.name} did not expose the Skiko MetalDevice pointer"
            )
      }
    if (ptr == 0L) {
      throw NativeSurfaceBridgeException("${contextHandler.javaClass.name}.device.ptr was zero")
    }
    SkikoMetalDevice(ptr)
  }

  fun requireDirect3DDevice(): SkikoDirect3DDevice = onEdt {
    val layer =
      findSkiaLayer()
        ?: throw NativeSurfaceBridgeException(
          "SkikoHost could not find a live $SKIA_LAYER_CLASS. ${describeWindows()}"
        )
    val redrawer = requireDirect3DRedrawer(layer)
    val ptr =
      redrawer.getField("device") as? Long
        ?: throw NativeSurfaceBridgeException("$DIRECT3D_REDRAWER_CLASS.device was null")
    if (ptr == 0L) {
      throw NativeSurfaceBridgeException("$DIRECT3D_REDRAWER_CLASS.device was zero")
    }
    SkikoDirect3DDevice(ptr)
  }

  fun drawMetalTexture(scope: DrawScope, target: MetalTextureTarget): Boolean {
    var drew = false
    scope.drawIntoCanvas { composeCanvas ->
      val context = findMetalContext() ?: return@drawIntoCanvas
      val presenter =
        metalPresenters.getOrPut(target.texture.address) { MetalTexturePresenter(target.texture) }
      presenter.draw(composeCanvas.skiaCanvas, context, target, scope.size.width, scope.size.height)
      drew = true
    }
    return drew
  }

  fun drawDirect3DTexture(scope: DrawScope, target: Direct3DTextureTarget): Boolean {
    var drew = false
    scope.drawIntoCanvas { composeCanvas ->
      val context = findDirect3DContext() ?: return@drawIntoCanvas
      val presenter =
        direct3DPresenters.getOrPut(target.texture.address) {
          Direct3DTexturePresenter(target.texture)
        }
      presenter.draw(composeCanvas.skiaCanvas, context, target, scope.size.width, scope.size.height)
      drew = true
    }
    return drew
  }

  fun forgetMetalTexture(texture: NativeHandle) {
    metalPresenters.remove(texture.address)?.close()
  }

  fun forgetDirect3DTexture(texture: NativeHandle) {
    direct3DPresenters.remove(texture.address)?.close()
  }

  fun drawOpenGlTexture(scope: DrawScope, target: OpenGlTextureTarget): Boolean {
    var drew = false
    scope.drawIntoCanvas { composeCanvas ->
      val context = findLinuxOpenGlContext() ?: return@drawIntoCanvas
      ensureOpenGlCapabilities()
      val presenter =
        openGlPresenters.getOrPut(target.textureName) { OpenGlTexturePresenter(target.textureName) }
      presenter.draw(composeCanvas.skiaCanvas, context, target, scope.size.width, scope.size.height)
      drew = true
    }
    return drew
  }

  fun forgetOpenGlTexture(textureName: Int) {
    openGlPresenters.remove(textureName)?.close()
  }

  fun <T> withLinuxOpenGlContext(action: () -> T): T = onEdt {
    val layer =
      findSkiaLayer()
        ?: throw NativeSurfaceBridgeException("SkikoHost could not find a live $SKIA_LAYER_CLASS")
    val redrawer = requireLinuxOpenGlRedrawer(layer)
    val backedLayer =
      layer.getField("backedLayer")
        ?: throw NativeSurfaceBridgeException("$SKIA_LAYER_CLASS.backedLayer was null")
    val context =
      redrawer.getField("context") as? Long
        ?: throw NativeSurfaceBridgeException("$LINUX_OPENGL_REDRAWER_CLASS.context was null")
    check(context != 0L) { "$LINUX_OPENGL_REDRAWER_CLASS.context was zero" }
    val surfaceHelpers = Class.forName(AWT_LINUX_DRAWING_SURFACE_HELPERS_CLASS)
    val drawingSurface = surfaceHelpers.staticInvoke("lockLinuxDrawingSurface", backedLayer)
    try {
      Class.forName(LINUX_OPENGL_REDRAWER_HELPERS_CLASS)
        .staticInvoke("access\$makeCurrent", drawingSurface, context)
      ensureOpenGlCapabilities()
      action()
    } finally {
      surfaceHelpers.staticInvoke("unlockLinuxDrawingSurface", drawingSurface)
    }
  }

  fun close() {
    val presenters = metalPresenters.values.toList()
    metalPresenters.clear()
    presenters.forEach { it.close() }
    val d3dPresenters = direct3DPresenters.values.toList()
    direct3DPresenters.clear()
    d3dPresenters.forEach { it.close() }
    val glPresenters = openGlPresenters.values.toList()
    openGlPresenters.clear()
    glPresenters.forEach { it.close() }
  }

  private fun findMetalContext(): DirectContext? = onEdt {
    val layer =
      findSkiaLayer()
        ?: throw NativeSurfaceBridgeException("SkikoHost could not find a live $SKIA_LAYER_CLASS")
    val contextHandler = requireMetalContextHandler(layer)
    (contextHandler.getField("context") as? DirectContext)
      ?: run {
        contextHandler.invokeDeclaredNoArg("initContext")
        (contextHandler.getField("context") as? DirectContext)
          ?: contextHandler.invokeDeclaredNoArg("getContext") as? DirectContext
      }
  }

  private fun requireMetalContextHandler(layer: Any): Any {
    val redrawer = requireMetalRedrawer(layer)
    return redrawer.getField("contextHandler")
      ?: throw NativeSurfaceBridgeException("$METAL_REDRAWER_CLASS.contextHandler was null")
  }

  private fun requireMetalRedrawer(layer: Any): Any {
    val redrawer =
      layer.invokeNoArg("getRedrawer\$skiko")
        ?: throw NativeSurfaceBridgeException("SkikoLayer.getRedrawer\$skiko returned null")
    requireClass(redrawer, METAL_REDRAWER_CLASS, "Skiko redrawer")
    return redrawer
  }

  private fun findDirect3DContext(): DirectContext? = onEdt {
    val layer =
      findSkiaLayer()
        ?: throw NativeSurfaceBridgeException("SkikoHost could not find a live $SKIA_LAYER_CLASS")
    val contextHandler = requireDirect3DContextHandler(layer)
    (contextHandler.getField("context") as? DirectContext)
      ?: run {
        contextHandler.invokeDeclaredNoArg("initContext")
        (contextHandler.getField("context") as? DirectContext)
          ?: contextHandler.invokeDeclaredNoArg("makeContext") as? DirectContext
      }
  }

  private fun requireDirect3DContextHandler(layer: Any): Any {
    val redrawer = requireDirect3DRedrawer(layer)
    return redrawer.getField("contextHandler")
      ?: throw NativeSurfaceBridgeException("$DIRECT3D_REDRAWER_CLASS.contextHandler was null")
  }

  private fun requireDirect3DRedrawer(layer: Any): Any {
    val redrawer =
      layer.invokeNoArg("getRedrawer\$skiko")
        ?: throw NativeSurfaceBridgeException("SkikoLayer.getRedrawer\$skiko returned null")
    requireClass(redrawer, DIRECT3D_REDRAWER_CLASS, "Skiko redrawer")
    return redrawer
  }

  private fun findLinuxOpenGlContext(): DirectContext? = onEdt {
    val layer =
      findSkiaLayer()
        ?: throw NativeSurfaceBridgeException("SkikoHost could not find a live $SKIA_LAYER_CLASS")
    val contextHandler = requireLinuxOpenGlContextHandler(layer)
    (contextHandler.getField("context") as? DirectContext)
      ?: run {
        contextHandler.invokeDeclaredNoArg("initContext")
        (contextHandler.getField("context") as? DirectContext)
          ?: contextHandler.invokeDeclaredNoArg("getContext") as? DirectContext
      }
  }

  private fun requireLinuxOpenGlContextHandler(layer: Any): Any {
    val redrawer = requireLinuxOpenGlRedrawer(layer)
    return redrawer.getField("contextHandler")
      ?: throw NativeSurfaceBridgeException("$LINUX_OPENGL_REDRAWER_CLASS.contextHandler was null")
  }

  private fun requireLinuxOpenGlRedrawer(layer: Any): Any {
    val redrawer =
      layer.invokeNoArg("getRedrawer\$skiko")
        ?: throw NativeSurfaceBridgeException("SkikoLayer.getRedrawer\$skiko returned null")
    requireClass(redrawer, LINUX_OPENGL_REDRAWER_CLASS, "Skiko redrawer")
    return redrawer
  }

  private fun findSkiaLayer(): Any? = findSkiaLayerComponent() ?: findComposeWindowSkiaLayer()

  private fun findSkiaLayerComponent(): Any? =
    Window.getWindows()
      .asSequence()
      .filter { it.isDisplayable }
      .flatMap { it.walkComponents() }
      .firstOrNull { isSkiaLayer(it) }

  private fun findComposeWindowSkiaLayer(): Any? =
    Window.getWindows()
      .asSequence()
      .filter { it.isDisplayable && it.javaClass.name == COMPOSE_WINDOW_CLASS }
      .mapNotNull { window ->
        runCatching {
            val composePanel = window.getField("composePanel") ?: return@mapNotNull null
            val contentComponent =
              composePanel.invokeDeclaredNoArg("getContentComponent") ?: return@mapNotNull null
            if (isSkiaLayer(contentComponent)) {
              contentComponent
            } else {
              (contentComponent as? Component)?.walkComponents()?.firstOrNull { isSkiaLayer(it) }
            }
          }
          .getOrNull()
      }
      .firstOrNull()

  private fun isSkiaLayer(value: Any): Boolean = Class.forName(SKIA_LAYER_CLASS).isInstance(value)

  private fun describeWindows(): String =
    Window.getWindows().joinToString(prefix = "Windows: ", separator = " | ") { window ->
      buildString {
        append(window.javaClass.name)
        append("(displayable=")
        append(window.isDisplayable)
        append(", showing=")
        append(window.isShowing)
        append(")")
        append(" children=[")
        append(
          window.walkComponents().drop(1).take(12).joinToString { component ->
            component.javaClass.name
          }
        )
        append("]")
      }
    }

  private fun Component.walkComponents(): Sequence<Component> = sequence {
    yield(this@walkComponents)
    if (this@walkComponents is Container) {
      for (child in components) {
        yieldAll(child.walkComponents())
      }
    }
  }

  private fun requireClass(value: Any, expected: String, label: String) {
    if (!Class.forName(expected).isAssignableFrom(value.javaClass)) {
      throw NativeSurfaceBridgeException("$label was ${value.javaClass.name}, expected $expected")
    }
  }

  private fun Any.invokeNoArg(name: String): Any? = javaClass.getMethod(name).invoke(this)

  private fun Any.invokeDeclaredNoArg(name: String): Any? =
    javaClass.findMethod(name).let {
      it.isAccessible = true
      it.invoke(this)
    }

  private fun Any.getField(name: String): Any? =
    javaClass.findField(name).let {
      it.isAccessible = true
      it.get(this)
    }

  private fun Class<*>.staticInvoke(name: String, vararg args: Any?): Any? =
    methods
      .firstOrNull { method -> method.name == name && method.parameterCount == args.size }
      ?.invoke(null, *args) ?: throw NoSuchMethodException("${this.name}.$name/${args.size}")

  private fun Class<*>.findField(name: String): java.lang.reflect.Field {
    var current: Class<*>? = this
    while (current != null) {
      try {
        return current.getDeclaredField(name)
      } catch (_: NoSuchFieldException) {
        current = current.superclass
      }
    }
    throw NoSuchFieldException("${this.name}.$name")
  }

  private fun Class<*>.findMethod(
    name: String,
    vararg parameterTypes: Class<*>,
  ): java.lang.reflect.Method {
    var current: Class<*>? = this
    while (current != null) {
      try {
        return current.getDeclaredMethod(name, *parameterTypes)
      } catch (_: NoSuchMethodException) {
        current = current.superclass
      }
    }
    throw NoSuchMethodException("${this.name}.$name(${parameterTypes.joinToString { it.name }})")
  }

  private fun <T> onEdt(block: () -> T): T {
    if (SwingUtilities.isEventDispatchThread()) {
      return block()
    }
    var result: Result<T>? = null
    SwingUtilities.invokeAndWait { result = runCatching(block) }
    return result!!.getOrThrow()
  }

  private class MetalTexturePresenter(private val texture: NativeHandle) : AutoCloseable {
    private var contextIdentity = 0
    private var extent = SurfaceExtent.Empty
    private var origin = TextureOrigin.TOP_LEFT
    private var renderTarget: BackendRenderTarget? = null
    private var surface: Surface? = null
    private val retainedImages = ArrayDeque<Image>()

    fun draw(
      canvas: org.jetbrains.skia.Canvas,
      context: DirectContext,
      target: MetalTextureTarget,
      destinationWidth: Float,
      destinationHeight: Float,
    ) {
      ensureSurface(context, target)
      val currentSurface =
        surface
          ?: throw NativeSurfaceBridgeException(
            "Skia could not wrap Metal texture ${target.texture.address}"
          )
      currentSurface.notifyContentWillChange(ContentChangeMode.DISCARD)
      val image = currentSurface.makeImageSnapshot()
      retainImageForRecordedFrame(image)
      canvas.drawImageRect(
        image = image,
        src = Rect.makeWH(image.width.toFloat(), image.height.toFloat()),
        dst = Rect.makeWH(destinationWidth, destinationHeight),
        samplingMode = SamplingMode.LINEAR,
        paint = null,
        strict = true,
      )
    }

    private fun ensureSurface(context: DirectContext, target: MetalTextureTarget) {
      val nextContextIdentity = System.identityHashCode(context)
      if (
        surface != null &&
          renderTarget != null &&
          contextIdentity == nextContextIdentity &&
          extent == target.extent &&
          origin == target.origin
      ) {
        return
      }

      closeGpuResources()
      contextIdentity = nextContextIdentity
      extent = target.extent
      origin = target.origin
      renderTarget =
        BackendRenderTarget.makeMetal(
          width = target.extent.physicalWidth,
          height = target.extent.physicalHeight,
          texturePtr = texture.address,
        )
      surface =
        Surface.makeFromBackendRenderTarget(
          context = context,
          rt = checkNotNull(renderTarget),
          origin = target.origin.toSkiaOrigin(),
          colorFormat = SurfaceColorFormat.BGRA_8888,
          colorSpace = null,
          surfaceProps = null,
        )
          ?: throw NativeSurfaceBridgeException(
            "Skia could not wrap Metal texture ${target.texture.address} as a render target"
          )
    }

    private fun retainImageForRecordedFrame(image: Image) {
      retainedImages.addLast(image)
      while (retainedImages.size > RETAINED_IMAGE_COUNT) {
        retainedImages.removeFirst().close()
      }
    }

    override fun close() {
      closeGpuResources()
      contextIdentity = 0
      extent = SurfaceExtent.Empty
      origin = TextureOrigin.TOP_LEFT
    }

    private fun closeGpuResources() {
      while (retainedImages.isNotEmpty()) {
        retainedImages.removeFirst().close()
      }
      surface?.close()
      surface = null
      renderTarget?.close()
      renderTarget = null
    }
  }

  private class Direct3DTexturePresenter(private val texture: NativeHandle) : AutoCloseable {
    private var contextIdentity = 0
    private var extent = SurfaceExtent.Empty
    private var colorFormat = SurfaceColorFormat.BGRA_8888
    private var origin = TextureOrigin.TOP_LEFT
    private var renderTarget: BackendRenderTarget? = null
    private var surface: Surface? = null
    private val retainedImages = ArrayDeque<Image>()

    fun draw(
      canvas: org.jetbrains.skia.Canvas,
      context: DirectContext,
      target: Direct3DTextureTarget,
      destinationWidth: Float,
      destinationHeight: Float,
    ) {
      ensureSurface(context, target)
      val currentSurface =
        surface
          ?: throw NativeSurfaceBridgeException(
            "Skia could not wrap Direct3D texture ${target.texture.address}"
          )
      currentSurface.notifyContentWillChange(ContentChangeMode.DISCARD)
      val image = currentSurface.makeImageSnapshot()
      retainImageForRecordedFrame(image)
      canvas.drawImageRect(
        image = image,
        src = Rect.makeWH(image.width.toFloat(), image.height.toFloat()),
        dst = Rect.makeWH(destinationWidth, destinationHeight),
        samplingMode = SamplingMode.LINEAR,
        paint = null,
        strict = true,
      )
    }

    private fun ensureSurface(context: DirectContext, target: Direct3DTextureTarget) {
      val nextContextIdentity = System.identityHashCode(context)
      if (
        surface != null &&
          renderTarget != null &&
          contextIdentity == nextContextIdentity &&
          extent == target.extent &&
          colorFormat == target.colorFormat &&
          origin == target.origin
      ) {
        return
      }

      closeGpuResources()
      contextIdentity = nextContextIdentity
      extent = target.extent
      colorFormat = target.colorFormat
      origin = target.origin
      renderTarget =
        BackendRenderTarget.makeDirect3D(
          width = target.extent.physicalWidth,
          height = target.extent.physicalHeight,
          texturePtr = texture.address,
          format = target.format,
          sampleCnt = 1,
          levelCnt = 0,
        )
      surface =
        Surface.makeFromBackendRenderTarget(
          context = context,
          rt = checkNotNull(renderTarget),
          origin = target.origin.toSkiaOrigin(),
          colorFormat = target.colorFormat,
          colorSpace = null,
          surfaceProps = null,
        )
          ?: throw NativeSurfaceBridgeException(
            "Skia could not wrap Direct3D texture ${target.texture.address} as a render target"
          )
    }

    override fun close() {
      closeGpuResources()
      contextIdentity = 0
      extent = SurfaceExtent.Empty
      colorFormat = SurfaceColorFormat.BGRA_8888
      origin = TextureOrigin.TOP_LEFT
    }

    private fun retainImageForRecordedFrame(image: Image) {
      retainedImages.addLast(image)
      while (retainedImages.size > RETAINED_IMAGE_COUNT) {
        retainedImages.removeFirst().close()
      }
    }

    private fun closeGpuResources() {
      while (retainedImages.isNotEmpty()) {
        retainedImages.removeFirst().close()
      }
      surface?.close()
      surface = null
      renderTarget?.close()
      renderTarget = null
    }
  }

  private class OpenGlTexturePresenter(private val textureName: Int) : AutoCloseable {
    private var contextIdentity = 0
    private var extent = SurfaceExtent.Empty
    private var origin = TextureOrigin.TOP_LEFT
    private var framebuffer = 0
    private var renderTarget: BackendRenderTarget? = null
    private var surface: Surface? = null
    private val retainedImages = ArrayDeque<Image>()

    fun draw(
      canvas: org.jetbrains.skia.Canvas,
      context: DirectContext,
      target: OpenGlTextureTarget,
      destinationWidth: Float,
      destinationHeight: Float,
    ) {
      ensureSurface(context, target)
      val currentSurface =
        surface
          ?: throw NativeSurfaceBridgeException(
            "Skia could not wrap OpenGL texture ${target.textureName}"
          )
      context.resetGLAll()
      currentSurface.notifyContentWillChange(ContentChangeMode.DISCARD)
      val image = currentSurface.makeImageSnapshot()
      retainImageForRecordedFrame(image)
      canvas.drawImageRect(
        image = image,
        src = Rect.makeWH(image.width.toFloat(), image.height.toFloat()),
        dst = Rect.makeWH(destinationWidth, destinationHeight),
        samplingMode = SamplingMode.LINEAR,
        paint = null,
        strict = true,
      )
    }

    private fun ensureSurface(context: DirectContext, target: OpenGlTextureTarget) {
      val nextContextIdentity = System.identityHashCode(context)
      if (
        surface != null &&
          renderTarget != null &&
          framebuffer != 0 &&
          contextIdentity == nextContextIdentity &&
          extent == target.extent &&
          origin == target.origin
      ) {
        return
      }

      closeGpuResources()
      contextIdentity = nextContextIdentity
      extent = target.extent
      origin = target.origin
      framebuffer = createFramebuffer(target)
      renderTarget =
        BackendRenderTarget.makeGL(
          width = target.extent.physicalWidth,
          height = target.extent.physicalHeight,
          sampleCnt = 0,
          stencilBits = 0,
          fbId = framebuffer,
          fbFormat = FramebufferFormat.GR_GL_RGBA8,
        )
      surface =
        Surface.makeFromBackendRenderTarget(
          context = context,
          rt = checkNotNull(renderTarget),
          origin = target.origin.toSkiaOrigin(),
          colorFormat = SurfaceColorFormat.RGBA_8888,
          colorSpace = null,
          surfaceProps = null,
        )
          ?: throw NativeSurfaceBridgeException(
            "Skia could not wrap OpenGL framebuffer $framebuffer for texture ${target.textureName}"
          )
    }

    override fun close() {
      closeGpuResources()
      contextIdentity = 0
      extent = SurfaceExtent.Empty
      origin = TextureOrigin.TOP_LEFT
    }

    private fun createFramebuffer(target: OpenGlTextureTarget): Int {
      ensureOpenGlCapabilities()
      val previous = glGetInteger(GL_FRAMEBUFFER_BINDING)
      val next = glGenFramebuffers()
      try {
        glBindFramebuffer(GL_FRAMEBUFFER, next)
        glFramebufferTexture2D(
          GL_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          target.textureTarget,
          target.textureName,
          0,
        )
        val status = glCheckFramebufferStatus(GL_FRAMEBUFFER)
        checkGl("glFramebufferTexture2D")
        check(status == GL_FRAMEBUFFER_COMPLETE) {
          "OpenGL framebuffer for texture ${target.textureName} is incomplete: 0x${status.toString(16)}"
        }
        return next
      } catch (error: RuntimeException) {
        runCatching { glDeleteFramebuffers(next) }
        throw error
      } finally {
        glBindFramebuffer(GL_FRAMEBUFFER, previous)
      }
    }

    private fun retainImageForRecordedFrame(image: Image) {
      retainedImages.addLast(image)
      while (retainedImages.size > RETAINED_IMAGE_COUNT) {
        retainedImages.removeFirst().close()
      }
    }

    private fun closeGpuResources() {
      while (retainedImages.isNotEmpty()) {
        retainedImages.removeFirst().close()
      }
      surface?.close()
      surface = null
      renderTarget?.close()
      renderTarget = null
      if (framebuffer != 0) {
        runCatching {
          ensureOpenGlCapabilities()
          glDeleteFramebuffers(framebuffer)
        }
        framebuffer = 0
      }
    }
  }
}

internal data class SkikoMetalDevice(val ptr: Long)

internal data class SkikoDirect3DDevice(val ptr: Long)

internal data class Direct3DTextureTarget(
  val texture: NativeHandle,
  val format: Int = 87,
  val colorFormat: SurfaceColorFormat = SurfaceColorFormat.BGRA_8888,
  val origin: TextureOrigin = TextureOrigin.TOP_LEFT,
  val extent: SurfaceExtent,
  val generation: Long,
)

internal class NativeSurfaceBridgeException(message: String, cause: Throwable? = null) :
  RuntimeException(message, cause)

private fun TextureOrigin.toSkiaOrigin(): SurfaceOrigin =
  when (this) {
    TextureOrigin.TOP_LEFT -> SurfaceOrigin.TOP_LEFT
    TextureOrigin.BOTTOM_LEFT -> SurfaceOrigin.BOTTOM_LEFT
  }

private fun ensureOpenGlCapabilities() {
  runCatching { GL.getCapabilities() }.getOrNull() ?: GL.createCapabilities()
}

private fun checkGl(operation: String) {
  val error = glGetError()
  check(error == GL_NO_ERROR) { "$operation failed with GL error 0x${error.toString(16)}" }
}
