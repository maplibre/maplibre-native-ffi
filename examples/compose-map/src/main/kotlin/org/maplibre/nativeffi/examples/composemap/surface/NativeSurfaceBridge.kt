package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

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

    fun select(supportedBackends: Set<ProducerBackend>): NativeSurfaceBridgeSelection {
      val candidate =
        bridgeCandidates(host).firstOrNull { it.backend in supportedBackends }
          ?: return NativeSurfaceBridgeSelection.Unsupported
      return try {
        NativeSurfaceBridgeSelection.Selected(candidate.create())
      } catch (error: Throwable) {
        if (error is VirtualMachineError || error is ThreadDeath) {
          throw error
        }
        NativeSurfaceBridgeSelection.Failed(candidate.backend, error)
      }
    }

    private fun bridgeCandidates(host: NativeSurfaceHost): List<NativeSurfaceBridgeCandidate> =
      when (host.operatingSystem) {
        NativeSurfaceOperatingSystem.MACOS ->
          listOf(
            NativeSurfaceBridgeCandidate(ProducerBackend.METAL, ::MacMetalBridge),
            NativeSurfaceBridgeCandidate(ProducerBackend.VULKAN, ::MacVulkanMetalBridge),
            NativeSurfaceBridgeCandidate(ProducerBackend.OPENGL, ::MacOpenGlMetalBridge),
          )
        NativeSurfaceOperatingSystem.LINUX ->
          listOf(
            NativeSurfaceBridgeCandidate(ProducerBackend.VULKAN, ::LinuxVulkanOpenGlBridge),
            NativeSurfaceBridgeCandidate(ProducerBackend.OPENGL, ::LinuxOpenGlBridge),
          )
        NativeSurfaceOperatingSystem.WINDOWS ->
          listOf(
            NativeSurfaceBridgeCandidate(ProducerBackend.VULKAN, ::WindowsVulkanD3d12Bridge),
            NativeSurfaceBridgeCandidate(ProducerBackend.OPENGL, ::WindowsOpenGlD3d12Bridge),
          )
        NativeSurfaceOperatingSystem.UNSUPPORTED -> emptyList()
      }
  }
}

private data class NativeSurfaceBridgeCandidate(
  val backend: ProducerBackend,
  val create: () -> NativeSurfaceBridge,
)

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
