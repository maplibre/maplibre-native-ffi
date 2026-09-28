// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*

public expect abstract class GeneratedRenderFrameBatchOperations internal constructor() {
  public fun count(): ULong

  public fun get(indexValue: ULong): RenderFrameResult

  public fun release(): Unit
}
