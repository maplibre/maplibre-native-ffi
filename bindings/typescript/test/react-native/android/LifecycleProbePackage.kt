package org.maplibre.lifecycle

import com.facebook.react.BaseReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.model.ReactModuleInfo
import com.facebook.react.module.model.ReactModuleInfoProvider

class LifecycleProbePackage : BaseReactPackage() {
  override fun getModule(name: String, context: ReactApplicationContext): NativeModule? =
    if (name == "LifecycleProbe") LifecycleProbeModule(context) else null

  override fun getReactModuleInfoProvider() = ReactModuleInfoProvider {
    mapOf(
      "LifecycleProbe" to
        ReactModuleInfo(
          "LifecycleProbe",
          "LifecycleProbe",
          false,
          false,
          false,
          true,
        ),
    )
  }
}
