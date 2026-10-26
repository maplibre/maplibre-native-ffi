package org.maplibre.nativeffi.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.MapFixture
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.NativePointer
import org.maplibre.nativeffi.QueueLock
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.Wake
import org.maplibre.nativeffi.WakeCallback
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.callback.CallbackRoot
import org.maplibre.nativeffi.internal.callback.CallbackRoots
import org.maplibre.nativeffi.native
import org.maplibre.nativeffi.render.RED_BACKGROUND_STYLE
import org.maplibre.nativeffi.render.TestBackend
import org.maplibre.nativeffi.render.TestGraphics
import org.maplibre.nativeffi.render.attachDeviceTexture
import org.maplibre.nativeffi.render.withRenderSession
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.withMap

private const val WIDTH = 32
private const val HEIGHT = 16

/**
 * Native registrations, whose function pointers, context, and release the host's native code
 * supplies. Only the threaded targets declare them.
 */
@OptIn(ExperimentalAtomicApi::class)
class NativeRegistrationTest {
  @Test
  fun aNativeWakeReachesNativeWhichReleasesItOnce() = runSuspendTest {
    var fixtureWake = WakeCallback {}
    HostFunctions(onCall = { fixtureWake() }).use { host ->
      withSmallMap {
        withRenderSession(
          RenderDriverKind.CORE_WORKER,
          attach = { options ->
            // The fixture renders when its frame wake runs, which now only the native wake does.
            fixtureWake = options.frameWake.callback!!
            val frameWake = Wake.native(host.call, host.userData, host.release)
            attachDeviceTexture(map, WIDTH, HEIGHT, options.copy(frameWake = frameWake))
          },
        ) {
          setStyle(RED_BACKGROUND_STYLE)
          renderFrame()
        }
      }
      host.released.awaitWithin("native to release the wake")
      assertTrue(host.calls.load() > 0)
      assertEquals(1, host.releases.load())
    }
  }

  @Test
  fun theBindingReleasesANativeWakeThatNativeRejectsAndRefusesToReuseIt() = runSuspendTest {
    HostFunctions().use { host ->
      withSmallMap {
        val options =
          RenderSessionAttachOptions(
            driver = RenderDriverKind.CORE_WORKER,
            frameWake = Wake.native(host.call, host.userData, host.release),
          )
        // Native rejects a zero width.
        assertFailsWithStatus(Status.INVALID_ARGUMENT) {
          attachDeviceTexture(map, 0, HEIGHT, options)
        }
        assertEquals(1, host.releases.load())
        // Native would accept this attach, so only the binding refuses it.
        assertFailsWith<IllegalStateException> {
          attachDeviceTexture(map, WIDTH, HEIGHT, options)
        }
      }
      assertEquals(0, host.calls.load())
      assertEquals(1, host.releases.load())
    }
  }

  @Test
  fun aNativeQueueLockReachesNative() = runSuspendTest {
    HostFunctions().use { host ->
      withSmallMap {
        val lock = QueueLock.native(host.call, host.call, host.userData, host.release)
        if (TestGraphics.backend == TestBackend.VULKAN) {
          withRenderSession(
            RenderDriverKind.CALLER_GRAPHICS_THREAD,
            attach = { options ->
              TestGraphics.attachOwnedTexture(map, WIDTH, HEIGHT, options.copy(queueLock = lock))
            },
          ) {
            setStyle(RED_BACKGROUND_STYLE)
            renderFrame()
          }
          host.released.awaitWithin("native to release the queue lock")
          assertTrue(host.calls.load() > 0)
        } else {
          // Only a Vulkan target takes a queue lock, so native rejects one that reaches it.
          withContext(TestGraphics.thread) {
            assertFailsWithStatus(Status.UNSUPPORTED) {
              TestGraphics.attachOwnedTexture(
                map,
                WIDTH,
                HEIGHT,
                RenderSessionAttachOptions(
                  driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
                  queueLock = lock,
                ),
              )
            }
          }
        }
        assertEquals(1, host.releases.load())
        withContext(TestGraphics.thread) {
          assertFailsWith<IllegalStateException> {
            TestGraphics.attachOwnedTexture(
              map,
              WIDTH,
              HEIGHT,
              RenderSessionAttachOptions(
                driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
                queueLock = lock,
              ),
            )
          }
        }
      }
      assertEquals(1, host.releases.load())
    }
  }

  private suspend fun withSmallMap(block: suspend MapFixture.() -> Unit) =
    withMap(MapMode.STATIC, options = smallMapOptions(MapMode.STATIC, WIDTH, HEIGHT), block = block)
}

/**
 * Real C functions of type `void(void*)` in place of a host's: the binding's own queue-lock upcall
 * stubs. Called with [userData], [call] runs [onCall], and [release] counts a release and completes
 * [released]. [close] drops the root that [userData] names, once native code is done with it.
 */
@OptIn(ExperimentalAtomicApi::class)
private class HostFunctions(onCall: () -> Unit = {}) : AutoCloseable {
  val calls = AtomicInt(0)
  val releases = AtomicInt(0)
  val released = CompletableDeferred<Unit>()

  // CallbackRoots holds a root weakly, so this owner keeps it reachable.
  private val owner = CallbackOwner()
  private val token =
    CallbackRoot(
        QueueLock(
          lock = {
            calls.addAndFetch(1)
            onCall()
          },
          unlock = {
            releases.addAndFetch(1)
            released.complete(Unit)
          },
        ),
      )
      .let { root -> CallbackRoots.retain(root).also { owner.retain(root) } }

  val call: NativePointer = NativePointer.ofAddress(UpcallStubs.queueLockLock)
  val release: NativePointer = NativePointer.ofAddress(UpcallStubs.queueLockUnlock)
  val userData: NativePointer = NativePointer.ofAddress(token)

  override fun close() {
    CallbackRoots.release(token)
  }
}
