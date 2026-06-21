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
import org.jetbrains.skia.Image
import org.jetbrains.skia.Rect
import org.jetbrains.skia.SamplingMode
import org.jetbrains.skia.Surface
import org.jetbrains.skia.SurfaceColorFormat
import org.jetbrains.skia.SurfaceOrigin

internal object SkikoHost {
  private const val SKIA_LAYER_CLASS = "org.jetbrains.skiko.SkiaLayer"
  private const val COMPOSE_WINDOW_CLASS = "androidx.compose.ui.awt.ComposeWindow"
  private const val METAL_REDRAWER_CLASS = "org.jetbrains.skiko.redrawer.MetalRedrawer"
  private const val RETAINED_IMAGE_COUNT = 8

  private val metalPresenters = mutableMapOf<Long, MetalTexturePresenter>()

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

  fun forgetMetalTexture(texture: NativeHandle) {
    metalPresenters.remove(texture.address)?.close()
  }

  fun close() {
    val presenters = metalPresenters.values.toList()
    metalPresenters.clear()
    presenters.forEach { it.close() }
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
    if (value.javaClass.name != expected) {
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

  private fun Class<*>.findMethod(name: String): java.lang.reflect.Method {
    var current: Class<*>? = this
    while (current != null) {
      try {
        return current.getDeclaredMethod(name)
      } catch (_: NoSuchMethodException) {
        current = current.superclass
      }
    }
    throw NoSuchMethodException("${this.name}.$name()")
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
          extent == target.extent
      ) {
        return
      }

      closeGpuResources()
      contextIdentity = nextContextIdentity
      extent = target.extent
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
          origin = SurfaceOrigin.TOP_LEFT,
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
}

internal data class SkikoMetalDevice(val ptr: Long)

internal class NativeSurfaceBridgeException(message: String, cause: Throwable? = null) :
  RuntimeException(message, cause)
