package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope
import java.util.concurrent.ExecutionException
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicReference

internal interface NativeSurfaceBridge : AutoCloseable {
  val backend: ProducerBackend

  val consumerBackend: ConsumerBackend

  val capabilities: NativeSurfaceCapabilities

  fun resize(extent: SurfaceExtent) {}

  fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame

  fun completeProducerAccess(frame: NativeSurfaceFrame) {}

  fun releaseFrame(frame: NativeSurfaceFrame) {}

  fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T = action()

  fun <T> withRendererAccess(action: () -> T): T = action()

  fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean = false

  override fun close() {}

  companion object {
    val host: NativeSurfaceHost = detectHost()

    fun select(backend: ProducerBackend): NativeSurfaceBridgeSelection {
      val create = bridgeFactory(host, backend) ?: return NativeSurfaceBridgeSelection.Unsupported
      return try {
        NativeSurfaceBridgeSelection.Selected(create())
      } catch (error: Throwable) {
        if (error is VirtualMachineError || error is ThreadDeath) {
          throw error
        }
        NativeSurfaceBridgeSelection.Failed(backend, error)
      }
    }

    private fun bridgeFactory(
      host: NativeSurfaceHost,
      backend: ProducerBackend,
    ): (() -> NativeSurfaceBridge)? =
      when (host.operatingSystem to backend) {
        NativeSurfaceOperatingSystem.MACOS to ProducerBackend.METAL -> ::MacMetalBridge
        NativeSurfaceOperatingSystem.MACOS to ProducerBackend.VULKAN -> ::MacVulkanMetalBridge
        NativeSurfaceOperatingSystem.MACOS to ProducerBackend.OPENGL -> ::MacOpenGlMetalBridge
        NativeSurfaceOperatingSystem.LINUX to ProducerBackend.VULKAN -> ::LinuxVulkanOpenGlBridge
        NativeSurfaceOperatingSystem.LINUX to ProducerBackend.OPENGL -> ::LinuxOpenGlBridge
        NativeSurfaceOperatingSystem.WINDOWS to ProducerBackend.VULKAN -> ::WindowsVulkanD3d12Bridge
        NativeSurfaceOperatingSystem.WINDOWS to ProducerBackend.OPENGL -> ::WindowsOpenGlD3d12Bridge
        else -> null
      }
  }
}

internal class NativeSurfaceRendererDispatcher(threadName: String) : AutoCloseable {
  private val threadRef = AtomicReference<Thread?>()
  private val executor = Executors.newSingleThreadExecutor { task ->
    Thread(task, threadName).also {
      it.isDaemon = true
      threadRef.set(it)
    }
  }

  fun <T> run(action: () -> T): T {
    if (Thread.currentThread() == threadRef.get()) {
      return action()
    }
    return try {
      executor.submit<T> { action() }.get()
    } catch (error: ExecutionException) {
      throw error.cause ?: error
    }
  }

  override fun close() {
    executor.shutdown()
  }
}

internal sealed interface NativeSurfaceBridgeSelection {
  data class Selected(val bridge: NativeSurfaceBridge) : NativeSurfaceBridgeSelection

  data class Failed(val backend: ProducerBackend, val error: Throwable) :
    NativeSurfaceBridgeSelection {
    val message: String
      get() = "$backend bridge failed: ${error.message ?: error.javaClass.name}"
  }

  data object Unsupported : NativeSurfaceBridgeSelection
}

private fun detectHost(): NativeSurfaceHost {
  val os = System.getProperty("os.name").lowercase()
  return when {
    os.contains("mac") ->
      NativeSurfaceHost(NativeSurfaceOperatingSystem.MACOS, ConsumerBackend.METAL)
    os.contains("linux") ->
      NativeSurfaceHost(NativeSurfaceOperatingSystem.LINUX, ConsumerBackend.OPENGL)
    os.contains("windows") ->
      NativeSurfaceHost(NativeSurfaceOperatingSystem.WINDOWS, ConsumerBackend.DIRECT3D12)
    else -> NativeSurfaceHost(NativeSurfaceOperatingSystem.UNSUPPORTED, ConsumerBackend.OPENGL)
  }
}
