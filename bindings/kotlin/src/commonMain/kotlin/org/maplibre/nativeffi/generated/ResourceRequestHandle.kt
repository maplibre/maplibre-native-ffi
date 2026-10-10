// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * A resource request that a resource provider handles.
 *
 * See `mln_resource_request_handle` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class ResourceRequestHandle
internal constructor(
  handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::resourceRequestHandle,
) : GeneratedResourceRequestHandleOperations(), AutoCloseable {
  internal override val binding =
    DecisionOwnerState(
      "ResourceRequestHandle",
      handle,
      accept = 1u,
      passThrough = 0u,
      dispose = dispose,
    )
  @Suppress("unused") private val cleanup = closeWhenUnreachable(this, binding.core)
  public val isClosed: Boolean
    get() = binding.isClosed

  public override fun close() {
    release()
  }
}
