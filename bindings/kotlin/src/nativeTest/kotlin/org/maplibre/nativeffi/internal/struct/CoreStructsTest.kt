package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import org.maplibre.nativeffi.internal.c.mln_buffer_view

@OptIn(ExperimentalForeignApi::class)
class CoreStructsTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-063.

  @Test
  fun stringViewsPreserveEmbeddedNulWithExplicitLength(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        val native = alloc<mln_buffer_view>()
        CoreStructs.stringView("a\u0000b", this).place(native.ptr)

        assertEquals(3UL, native.size.toULong())
        assertEquals("a\u0000b", CoreStructs.stringView(native))
      }
    }

  @Test
  fun setStringViewPreservesEmbeddedNulWithExplicitLength(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        val native = alloc<mln_buffer_view>()

        CoreStructs.setStringView(native, "a\u0000b", this)

        assertEquals(3UL, native.size.toULong())
        assertEquals("a\u0000b", CoreStructs.stringView(native))
      }
    }
}
