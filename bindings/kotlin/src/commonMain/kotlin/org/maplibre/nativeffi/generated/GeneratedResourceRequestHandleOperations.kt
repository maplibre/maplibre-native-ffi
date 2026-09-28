// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*

public expect abstract class GeneratedResourceRequestHandleOperations internal constructor() {
  internal val bindingCallbacks: org.maplibre.nativeffi.internal.callback.CallbackOwner

  public fun resourceRequestCancelled(): Boolean

  public fun resourceRequestComplete(response: ResourceResponse): Unit

  public fun resourceRequestRelease(): Unit

  public fun resourceRequestSetCancelCallback(callback: ResourceRequestCancelCallback): Boolean

  public fun resourceRequestWaitUntilRetired(): Unit
}
