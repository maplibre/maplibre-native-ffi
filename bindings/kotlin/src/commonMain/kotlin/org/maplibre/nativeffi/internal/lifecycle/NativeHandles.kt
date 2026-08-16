package org.maplibre.nativeffi.internal.lifecycle

import kotlin.jvm.JvmInline

/**
 * A handle the C API issued. The C API spells every handle as one integer type, so each kind gets
 * its own value class here to stay distinct at compile time. [raw] carries no ownership and is safe
 * to copy, compare, and hash. Zero is the null handle.
 */
internal sealed interface NativeHandle {
  val raw: Long

  val isNull: Boolean
    get() = raw == 0L
}

@JvmInline internal value class NativeRuntime(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeMap(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeMapProjection(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeRenderSession(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeOfflineRegionSnapshot(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeOfflineRegionList(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeOwnedBuffer(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeStyleIdList(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeStyleStringList(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeQueriedFeatureList(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeWakeSource(override val raw: Long) : NativeHandle

@JvmInline internal value class NativeResourceRequest(override val raw: Long) : NativeHandle
