import kotlin.system.exitProcess
import org.maplibre.nativeffi.RenderBackendFlag
import org.maplibre.nativeffi.cVersion
import org.maplibre.nativeffi.loadNativeLibrary
import org.maplibre.nativeffi.supportedRenderBackendMask

fun main() {
  loadNativeLibrary()
  println("C API version: ${cVersion()}")
  if (supportedRenderBackendMask() == RenderBackendFlag.NONE) {
    exitProcess(1)
  }
}
