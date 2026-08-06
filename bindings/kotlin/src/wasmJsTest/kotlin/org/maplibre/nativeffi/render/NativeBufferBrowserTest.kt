package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertContains
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.internal.wasm.Heap

/**
 * What a browser host is told when the module's heap cannot serve it.
 *
 * A fixed heap makes this the one native failure that is certain rather than unlikely, and for a
 * while it was the one the binding could not report at all. Emscripten aborts the module by default
 * when an allocation needs a byte past the initial memory, so every `if (address == 0)` in this
 * binding was unreachable and a host that asked for too much lost the whole module rather than
 * catching an error. The link turns that default off; these are what say so.
 */
class NativeBufferBrowserTest {
  @Test
  fun aBufferLargerThanTheWholeHeapIsRefusedAsAnArgument() {
    // Refused as an argument even though it reads like a shortage. The heap is fixed at link time,
    // so no state a host could reach makes this request succeed, and reporting it as invalid state
    // would send a caller looking for something to free.
    val error =
      assertFailsWith<InvalidArgumentException> { NativeBuffer.allocate(Heap.byteLength() + 1) }
    assertContains(error.diagnostic, "whole ${Heap.byteLength()}-byte heap")
  }

  @Test
  fun aBufferTheHeapCannotServeIsReportedAndLeavesTheModuleUsable() {
    // Exactly the heap's size: the largest request this binding accepts, and one no heap can serve,
    // because that same memory already holds the module's code, its threads' stacks, and everything
    // the suite has allocated. So the allocator is really asked and really refuses, without the
    // result depending on how much of the heap happens to be in use when this test runs. Nothing is
    // consumed by the refusal either, which is what makes it safe on a page the rest of the suite
    // shares.
    val heapBytes = Heap.byteLength()
    val error = assertFailsWith<InvalidStateException> { NativeBuffer.allocate(heapBytes) }
    assertContains(error.diagnostic, "could not allocate $heapBytes bytes")

    // The whole point of reporting rather than aborting. An abort takes the heap, the worker pool,
    // and every live handle with it, so a module that took the old path fails here by never
    // reaching this line at all.
    NativeBuffer.allocate(64).use { buffer ->
      assertEquals(64L, buffer.byteLength())
      assertEquals(64, buffer.toByteArray().size)
    }
  }
}
