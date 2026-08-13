package org.maplibre.nativeffi.runtime

/** One endpoint reported by a runtime's receiver-scoped notification source. */
public data class ReadyEndpoint(public val kind: Kind, public val id: Long) {
  public enum class Kind(internal val nativeValue: Int) {
    RUNTIME_EVENTS(1),
    OPERATION(2),
    ADAPTER_RESOURCE_REQUESTS(3),
    ADAPTER_LOG_RECORDS(4),
    RENDER_FRAMES(5),
    DRIVER_WORK(6),
    UNKNOWN(0);

    internal companion object {
      fun fromNative(value: Int): Kind = entries.firstOrNull { it.nativeValue == value } ?: UNKNOWN
    }
  }
}
