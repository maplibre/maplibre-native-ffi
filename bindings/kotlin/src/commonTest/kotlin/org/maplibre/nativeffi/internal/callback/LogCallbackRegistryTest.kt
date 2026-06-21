package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus

@OptIn(ExperimentalAtomicApi::class)
class LogCallbackRegistryTest {
  @Test
  fun firstSetInstallsNativeCallbackAndReplacementClosesPreviousState() {
    val registry = LogCallbackRegistry<TestCallbackState>()
    val first = TestCallbackState()
    val second = TestCallbackState()
    var installs = 0

    registry.set(first) {
      installs++
      MaplibreStatus.OK.nativeCode
    }
    registry.set(second) {
      installs++
      MaplibreStatus.OK.nativeCode
    }

    assertEquals(1, installs)
    assertTrue(first.isClosed())
    assertSame(second, registry.current())
  }

  @Test
  fun failedInitialInstallClosesReplacementAndLeavesCurrentEmpty() {
    val registry = LogCallbackRegistry<TestCallbackState>()
    val replacement = TestCallbackState()

    val error =
      assertFailsWith<MaplibreException> {
        registry.set(replacement) { MaplibreStatus.NATIVE_ERROR.nativeCode }
      }

    assertEquals(MaplibreStatus.NATIVE_ERROR, error.status)
    assertTrue(replacement.isClosed())
    assertNull(registry.current())
  }

  @Test
  fun clearOnlyCallsNativeClearWhenInstalledAndClosesCurrentState() {
    val registry = LogCallbackRegistry<TestCallbackState>()
    val state = TestCallbackState()
    var clears = 0

    registry.clear {
      clears++
      MaplibreStatus.OK.nativeCode
    }
    registry.set(state) { MaplibreStatus.OK.nativeCode }
    registry.clear {
      clears++
      MaplibreStatus.OK.nativeCode
    }
    registry.clear {
      clears++
      MaplibreStatus.OK.nativeCode
    }

    assertEquals(1, clears)
    assertTrue(state.isClosed())
    assertNull(registry.current())
  }

  private class TestCallbackState : AutoCloseable {
    private val closed = AtomicInt(0)

    override fun close() {
      closed.store(1)
    }

    fun isClosed(): Boolean = closed.load() != 0
  }
}
