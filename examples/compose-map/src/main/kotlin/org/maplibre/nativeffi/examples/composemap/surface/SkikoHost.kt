package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.skiaCanvas
import java.awt.Component
import java.awt.Container
import java.awt.Window
import javax.swing.SwingUtilities
import org.jetbrains.skia.BackendRenderTarget
import org.jetbrains.skia.ColorSpace
import org.jetbrains.skia.DirectContext
import org.jetbrains.skia.PixelGeometry
import org.jetbrains.skia.Surface
import org.jetbrains.skia.SurfaceColorFormat
import org.jetbrains.skia.SurfaceOrigin
import org.jetbrains.skia.SurfaceProps

internal object SkikoHost {
  private const val SKIA_LAYER_CLASS = "org.jetbrains.skiko.SkiaLayer"
  private const val COMPOSE_WINDOW_CLASS = "androidx.compose.ui.awt.ComposeWindow"
  private const val METAL_REDRAWER_CLASS = "org.jetbrains.skiko.redrawer.MetalRedrawer"

  fun requireMetalDevice(): SkikoMetalDevice = onEdt {
    val layer =
      findSkiaLayer()
        ?: throw NativeSurfaceBridgeException(
          "SkikoHost could not find a live $SKIA_LAYER_CLASS. ${describeWindows()}"
        )
    val redrawer = requireMetalRedrawer(layer)
    val device =
      redrawer.getField("_device")
        ?: throw NativeSurfaceBridgeException(
          "$METAL_REDRAWER_CLASS._device was null; Skiko has not created the Metal device yet"
        )
    val ptr =
      device.invokeNoArg("getPtr") as? Long
        ?: throw NativeSurfaceBridgeException(
          "${device.javaClass.name}.getPtr() did not return the Skiko MetalDevice pointer"
        )
    if (ptr == 0L) {
      throw NativeSurfaceBridgeException("$METAL_REDRAWER_CLASS._device.ptr was zero")
    }
    SkikoMetalDevice(ptr)
  }

  fun drawMetalTexture(scope: DrawScope, target: MetalTextureTarget) {
    scope.drawIntoCanvas { composeCanvas ->
      val nativeCanvas = composeCanvas.skiaCanvas
      val context = requireMetalContext()
      BackendRenderTarget.makeMetal(
          target.extent.width,
          target.extent.height,
          target.texture.address,
        )
        .use { renderTarget ->
          val surface =
            Surface.makeFromBackendRenderTarget(
              context,
              renderTarget,
              SurfaceOrigin.TOP_LEFT,
              SurfaceColorFormat.BGRA_8888,
              ColorSpace.sRGB,
              SurfaceProps(pixelGeometry = PixelGeometry.UNKNOWN),
            )
              ?: throw NativeSurfaceBridgeException(
                "Skia could not wrap Metal texture ${target.texture.address} as a render target"
              )
          surface.use {
            val saveCount = nativeCanvas.save()
            try {
              nativeCanvas.scale(
                scope.size.width / target.extent.width.toFloat(),
                scope.size.height / target.extent.height.toFloat(),
              )
              it.draw(nativeCanvas, 0, 0, null)
            } finally {
              nativeCanvas.restoreToCount(saveCount)
            }
          }
        }
    }
  }

  private fun requireMetalContext(): DirectContext = onEdt {
    val layer =
      findSkiaLayer()
        ?: throw NativeSurfaceBridgeException("SkikoHost could not find a live $SKIA_LAYER_CLASS")
    val redrawer = requireMetalRedrawer(layer)
    val contextHandler =
      redrawer.getField("contextHandler")
        ?: throw NativeSurfaceBridgeException("$METAL_REDRAWER_CLASS.contextHandler was null")
    contextHandler.invokeDeclaredNoArg("getContext") as? DirectContext
      ?: throw NativeSurfaceBridgeException(
        "${contextHandler.javaClass.name}.getContext() did not return a Skia DirectContext"
      )
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
}

internal data class SkikoMetalDevice(val ptr: Long)

internal class NativeSurfaceBridgeException(message: String, cause: Throwable? = null) :
  RuntimeException(message, cause)
