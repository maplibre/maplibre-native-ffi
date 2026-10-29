package org.maplibre.lifecycle

import com.facebook.proguard.annotations.DoNotStrip
import com.facebook.react.ReactApplication
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.UiThreadUtil
import com.facebook.react.turbomodule.core.interfaces.BindingsInstallerHolder
import com.facebook.react.turbomodule.core.interfaces.TurboModuleWithJSIBindings
import java.util.concurrent.CountDownLatch

class LifecycleProbeModule(private val context: ReactApplicationContext) :
  NativeLifecycleProbeSpec(context), TurboModuleWithJSIBindings {
  private val host = checkNotNull((context.applicationContext as ReactApplication).reactHost)
  private var nativeProbe: Long = 0
  private val beforeDestroy: () -> Unit = {
    val completed = CountDownLatch(1)
    // invalidate() runs after ReactInstance has stopped this queue. The host's
    // earlier listener must wait for the JS-thread cleanup before it returns.
    check(
      context.runOnJSQueueThread {
        cleanup(nativeProbe)
        completed.countDown()
      },
    )
    completed.await()
  }

  init {
    host.addBeforeDestroyListener(beforeDestroy)
  }

  override fun getName() = "LifecycleProbe"

  override fun reload() {
    UiThreadUtil.runOnUiThread { host.reload("MapLibre lifecycle probe") }
  }

  override fun invalidate() {
    host.removeBeforeDestroyListener(beforeDestroy)
    destroy(nativeProbe)
    nativeProbe = 0
    super.invalidate()
  }

  @DoNotStrip external override fun getBindingsInstaller(): BindingsInstallerHolder

  private external fun cleanup(probe: Long)

  private external fun destroy(probe: Long)

  companion object {
    init {
      System.loadLibrary("mln_lifecycle_probe")
    }
  }
}
