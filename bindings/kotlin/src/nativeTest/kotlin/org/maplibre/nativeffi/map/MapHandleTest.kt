package org.maplibre.nativeffi.map

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.DoubleVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.UIntVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.internal.c.mln_map_get_size
import org.maplibre.nativeffi.internal.c.mln_runtime_pump
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class MapHandleTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-042, BND-100, BND-190, BND-191: map creation, child lifetime, and owner-thread errors.

  @Test
  fun mapCreationOptionsMaterializeExtentScaleAndMode() {
    MapHandle.mapOptionsForTesting(
      MapOptions().apply {
        width = 320
        height = 240
        scaleFactor = 2.0
        mapMode = MapMode.STATIC
        fastPforEnabled = true
      }
    ) { native ->
      assertEquals(320U, native.width)
      assertEquals(240U, native.height)
      assertEquals(2.0, native.scale_factor)
      assertEquals(MapMode.STATIC.nativeValue.toUInt(), native.map_mode)
      assertTrue(native.fast_pfor_enabled)
    }
  }

  @Test
  fun mapCreationLeavesFastPforDecodingOffByDefault() {
    MapHandle.mapOptionsForTesting(MapOptions()) { native -> assertFalse(native.fast_pfor_enabled) }
  }

  @Test
  fun mapSizeReportsCreationExtentAndPixelRatio() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 512
            height = 256
            scaleFactor = 2.0
          },
        )
      try {
        val size = map.size
        assertEquals(512, size.width)
        assertEquals(256, size.height)
        assertEquals(2.0, size.scaleFactor)
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun closeReleasesMapOnceKeepsRuntimeLiveAndInvalidatesWrapper() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
            scaleFactor = 1.0
          },
        )

      assertFalse(map.isClosed)
      assertEquals(runtime, map.runtime())
      map.close()

      assertTrue(map.isClosed)
      map.close()
      runtime.pump(0)
      assertFailsWith<InvalidStateException> { map.setStyleJson("{}") }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun runtimeCloseFailsWhileMapChildIsLive() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          scaleFactor = 1.0
        },
      )
    try {
      val error = assertFailsWith<InvalidStateException> { runtime.close() }
      assertEquals(MaplibreStatus.INVALID_STATE, error.status)
      assertEquals("RuntimeHandle has 1 live child handle(s): MapHandle", error.diagnostic)
      assertFalse(runtime.isClosed)

      runtime.pump(0)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapOwnerThreadCallFromAnotherNativeThreadReportsCopiedDiagnostic() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
        },
      )
    val callError = AtomicReference<Throwable?>(null)
    try {
      spawnMapCallOnNativeThread(map, callError)
      val error = callError.load()
      if (error !is WrongThreadException)
        throw error ?: AssertionError("wrong-thread map call succeeded")
      val diagnostic = error.diagnostic
      assertEquals(MaplibreStatus.WRONG_THREAD, error.status)
      assertEquals(MaplibreStatus.WRONG_THREAD.nativeCode, error.nativeStatusCode)
      assertTrue(diagnostic.isNotBlank())

      runtime.pump(0)

      assertEquals(diagnostic, error.diagnostic)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapCloseFromAnotherNativeThreadReportsWrongThreadAndLeavesHandleLive() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
        },
      )
    val closeError = AtomicReference<Throwable?>(null)
    try {
      spawnMapCloseOnNativeThread(map, closeError)
      val error = closeError.load()
      if (error !is WrongThreadException)
        throw error ?: AssertionError("wrong-thread map close succeeded")
      assertEquals(MaplibreStatus.WRONG_THREAD, error.status)
      assertFalse(map.isClosed)

      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
    } finally {
      map.close()
      runtime.close()
    }
    assertTrue(map.isClosed)
  }

  private fun spawnMapCallOnNativeThread(map: MapHandle, callError: AtomicReference<Throwable?>) {
    memScoped {
      val call = BackgroundMapCall(map, callError)
      val selfRef = StableRef.create(call)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::runMapCallOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      pthread_join(thread.ptr[0], null)
    }
  }

  private fun spawnMapCloseOnNativeThread(map: MapHandle, closeError: AtomicReference<Throwable?>) {
    memScoped {
      val close = BackgroundMapClose(map, closeError)
      val selfRef = StableRef.create(close)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::closeMapOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      pthread_join(thread.ptr[0], null)
    }
  }

  @Test
  fun releasedMapIdReplayedAfterANewMapIsReportedStale() {
    // BND-045.
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val first =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
        },
      )
    val released = first.nativeHandle()
    first.close()

    // The released slot is the one the next map takes, so the replayed id
    // names a retired generation of a slot that is live again.
    val second =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
        },
      )
    try {
      val error = assertFailsWith<InvalidArgumentException> { mapSizeForTesting(released) }
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
      assertTrue(error.message!!.contains("stale"), error.message!!)

      // The live map is unaffected by the replay.
      mapSizeForTesting(second.nativeHandle())
    } finally {
      second.close()
      runtime.close()
    }
  }

  @Test
  fun mapIdPassedToARuntimeOperationIsRejectedOnItsKind() {
    // BND-047.
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
        },
      )
    try {
      // NativeMap and NativeRuntime are distinct value classes, so this call
      // has no expression in the safe API and needs the raw id.
      val error =
        assertFailsWith<InvalidArgumentException> {
          Status.check(mln_runtime_pump(map.nativeHandle().rawHandleValue, 0))
        }
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
      assertTrue(error.message!!.contains("map"), error.message!!)
      assertTrue(error.message!!.contains("runtime"), error.message!!)
    } finally {
      map.close()
      runtime.close()
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun mapSizeForTesting(map: NativeMap) = memScoped {
  val width = alloc<UIntVar>()
  val height = alloc<UIntVar>()
  val scaleFactor = alloc<DoubleVar>()
  Status.check(mln_map_get_size(map.rawHandleValue, width.ptr, height.ptr, scaleFactor.ptr))
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundMapCall(
  private val map: MapHandle,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun runMapCallOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundMapCall>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundMapClose(
  private val map: MapHandle,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      map.close()
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun closeMapOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundMapClose>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}
