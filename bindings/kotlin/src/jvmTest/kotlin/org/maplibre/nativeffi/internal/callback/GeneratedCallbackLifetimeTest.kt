package org.maplibre.nativeffi.internal.callback

import java.lang.ref.WeakReference
import kotlin.test.Test
import kotlin.test.assertNull
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.ResourceTransform
import org.maplibre.nativeffi.runtime.RuntimeHandle

class GeneratedCallbackLifetimeTest {
  @Test
  fun callbackCapturingItsRuntimeDoesNotPreventCollection() {
    val reference = createUnreachableRuntime()
    repeat(100) {
      if (reference.get() == null) return
      System.gc()
      Thread.sleep(20)
    }
    assertNull(reference.get(), "native callback token retained a cycle through its runtime")
  }

  private fun createUnreachableRuntime(): WeakReference<RuntimeHandle> = runBlocking {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    runtime.setResourceTransform(ResourceTransform { _, _, _ -> runtime.isClosed }).await()
    WeakReference(runtime)
  }
}
