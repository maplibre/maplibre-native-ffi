package org.maplibre.nativeffi.runtime

/**
 * Mutable descriptor used when creating a [RuntimeHandle].
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class RuntimeOptions {
  public var assetPath: String? = null

  public var cachePath: String? = null

  /**
   * Runtime-originated event types this runtime queues, the native library's default until a host
   * narrows it. That default selects every runtime-originated type the library reports, including a
   * type this version does not name, whose events reach a host as unknown event and payload
   * domains.
   *
   * [RuntimeHandle.create] fails with [org.maplibre.nativeffi.error.InvalidArgumentException] on a
   * bit the native library does not define.
   */
  public var eventMask: RuntimeEventMask = defaultRuntimeEventMask()

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: RuntimeOptions.() -> Unit = {}): RuntimeOptions =
    RuntimeOptions()
      .also {
        it.assetPath = assetPath
        it.cachePath = cachePath
        it.eventMask = eventMask
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(assetPath, cachePath, eventMask)

  override fun equals(other: Any?): Boolean = other is RuntimeOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
