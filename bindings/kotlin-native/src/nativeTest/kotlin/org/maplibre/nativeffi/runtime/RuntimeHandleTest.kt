package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException

class RuntimeHandleTest {
  @Test
  fun closeReleasesRuntimeOnceAndInvalidatesWrapper() {
    val runtime = RuntimeHandle.create()

    assertFalse(runtime.isClosed())
    runtime.runOnce()
    runtime.pollEvent()
    runtime.close()

    assertTrue(runtime.isClosed())
    runtime.close()
    assertFailsWith<InvalidStateException> { runtime.runOnce() }
  }
}
