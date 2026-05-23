package org.maplibre.nativeffi.internal.struct

import cnames.structs.mln_style_id_list
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.c.mln_string_view
import org.maplibre.nativeffi.internal.c.mln_style_id_list_count
import org.maplibre.nativeffi.internal.c.mln_style_id_list_destroy
import org.maplibre.nativeffi.internal.c.mln_style_id_list_get
import org.maplibre.nativeffi.internal.c.mln_style_source_info
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType

/** Copies style-owned list and metadata handles into Kotlin values. */
@OptIn(ExperimentalForeignApi::class)
internal object StyleStructs {
  fun styleIdList(list: CPointer<mln_style_id_list>): List<String> =
    try {
      memScoped {
        val outCount = alloc<ULongVar>()
        Status.check(mln_style_id_list_count(list, outCount.ptr))
        List(outCount.value.toInt()) { index ->
          val outId = alloc<mln_string_view>()
          Status.check(mln_style_id_list_get(list, index.toULong(), outId.ptr))
          CoreStructs.stringView(outId)
        }
      }
    } finally {
      mln_style_id_list_destroy(list)
    }

  fun sourceInfo(value: mln_style_source_info, attribution: String?): SourceInfo =
    SourceInfo(SourceType.fromNative(value.type), value.id_size, value.is_volatile, attribution)
}
