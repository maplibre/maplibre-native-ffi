package org.maplibre.nativeffi.map

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.runtime.RuntimeHandle
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
      }
    ) { native ->
      assertEquals(320U, native.width)
      assertEquals(240U, native.height)
      assertEquals(2.0, native.scale_factor)
      assertEquals(MapMode.STATIC.nativeValue.toUInt(), native.map_mode)
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
      runtime.runOnce()
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
      assertEquals("RuntimeHandle has 1 live child handle(s)", error.diagnostic)
      assertFalse(runtime.isClosed)

      runtime.runOnce()
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

      runtime.runOnce()

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
