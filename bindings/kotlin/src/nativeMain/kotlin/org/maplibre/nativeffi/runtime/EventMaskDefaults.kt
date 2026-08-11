package org.maplibre.nativeffi.runtime

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.useContents
import org.maplibre.nativeffi.internal.c.mln_map_options_default
import org.maplibre.nativeffi.internal.c.mln_runtime_options_default

@OptIn(ExperimentalForeignApi::class)
internal actual fun defaultRuntimeEventMask(): RuntimeEventMask =
  RuntimeEventMask(mln_runtime_options_default().useContents { event_mask.toLong() })

@OptIn(ExperimentalForeignApi::class)
internal actual fun defaultMapEventMask(): RuntimeEventMask =
  RuntimeEventMask(mln_map_options_default().useContents { event_mask.toLong() })
