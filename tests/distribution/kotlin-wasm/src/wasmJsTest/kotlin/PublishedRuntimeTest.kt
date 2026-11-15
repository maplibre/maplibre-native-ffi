@file:OptIn(ExperimentalWasmJsInterop::class)

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.time.Duration.Companion.seconds
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.EXPECTED_C_ABI_VERSION
import org.maplibre.nativeffi.RenderBackendFlag
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.RuntimeOptions
import org.maplibre.nativeffi.cVersion
import org.maplibre.nativeffi.loadWasmModule
import org.maplibre.nativeffi.supportedRenderBackendMask

class PublishedRuntimeTest {
  @Test
  fun loadsThePublishedBrowserRuntime() =
    runTest(timeout = 60.seconds) {
      // Native threads complete on the page's event loop, outside the virtual test scheduler.
      withContext(Dispatchers.Default) {
        loadWasmModule()
        assertEquals(EXPECTED_C_ABI_VERSION.toUInt(), cVersion())
        val expected =
          if (backend() == "webgl") RenderBackendFlag.OPEN_GL else RenderBackendFlag.WEB_GPU
        assertEquals(expected, supportedRenderBackendMask())
        RuntimeHandle.create(RuntimeOptions()).release().await()
      }
    }
}

private fun backend(): String = js("window.__karma__.config.maplibreBackend")
