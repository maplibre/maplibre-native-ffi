package org.maplibre.nativeffi.browser

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.TEST_MODULE_URL
import org.maplibre.nativeffi.TEST_POOL_SIZE
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.internal.loader.ModuleLoader
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.workersAtLoad

/** The page loads the browser module asynchronously, before any call can reach it. */
class ModuleLoadTest {
  @Test
  fun aCallBeforeTheModuleLoadsFailsWithItsReason() = runSuspendTest {
    // The suite's page loads the module once, before the first test that needs it, so the test
    // removes it for the call.
    val failure = ModuleLoader.withoutModuleForTesting {
      assertFailsWith<IllegalStateException> { GeneratedApi.supportedRenderBackendMask() }
    }
    assertTrue("Maplibre.loadModule" in failure.message.orEmpty(), failure.message)
    GeneratedApi.supportedRenderBackendMask()
  }

  @Test
  fun theModuleLoadsFromItsUrlAndReportsTheExpectedAbi() = runSuspendTest {
    // A later load returns the module that the first one loaded.
    Maplibre.loadModule(TEST_MODULE_URL)
    Maplibre.loadNativeLibrary()
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION.toUInt(), GeneratedApi.cVersion())
    assertEquals(TEST_POOL_SIZE, workersAtLoad, "the module ignored the pool size it was given")
  }
}
