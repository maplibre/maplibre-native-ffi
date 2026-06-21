package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import org.maplibre.nativeffi.internal.c.mln_string_view

@OptIn(ExperimentalForeignApi::class)
class CoreStructsTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-063: borrowed native string views are copied using explicit lengths.

  @Test
  fun stringViewsPreserveEmbeddedNulWithExplicitLength() {
    memScoped {
      val native = alloc<mln_string_view>()
      CoreStructs.stringView("a\u0000b", this).place(native.ptr)

      assertEquals(3UL, native.size)
      assertEquals("a\u0000b", CoreStructs.stringView(native))
    }
  }

  @Test
  fun setStringViewPreservesEmbeddedNulWithExplicitLength() {
    memScoped {
      val native = alloc<mln_string_view>()

      CoreStructs.setStringView(native, "a\u0000b", this)

      assertEquals(3UL, native.size)
      assertEquals("a\u0000b", CoreStructs.stringView(native))
    }
  }
}
