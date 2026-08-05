package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import org.maplibre.nativeffi.internal.status.Status

/**
 * The one way this binding reaches the C API.
 *
 * A browser host has no link step, so it cannot declare 278 imports and keep them in step with the
 * headers. The module instead carries a table of its own entry points, each called by name inside
 * the module so the C compiler checks every argument, and exposes one function that performs an
 * indexed call from a packed argument buffer. This is the Kotlin end of that.
 *
 * Every argument is one eight-byte slot whatever its declared width, which is what lets one buffer
 * layout serve every entry point. An entry point that returns a struct by value takes its
 * destination as the first slot, matching the hidden out-pointer its lowered signature already has.
 */
@JsFun("(address) => globalThis.__maplibreNativeC._mln_browser_entry_index(address)")
private external fun entryIndex(address: Int): Int

@JsFun(
  "(index, slots, count, result) => " +
    "globalThis.__maplibreNativeC._mln_browser_invoke_here(index, slots, count, result)"
)
private external fun invokeHere(index: Int, slots: Int, count: Int, result: Int): Boolean

@JsFun("() => globalThis.__maplibreNativeC._mln_browser_dispatch_protocol()")
private external fun dispatchProtocol(): Int

@JsFun("(index) => globalThis.__maplibreNativeC._mln_browser_entry_slots(index)")
private external fun entrySlots(index: Int): Int

internal object NativeCall {
  /**
   * The call protocol this binding packs for.
   *
   * Kept beside the packing code rather than generated, because it describes what the code below
   * does: change how a slot is written, how a struct return is addressed, or what a dispatched
   * call's completion reports, and this and the module's own constant both move. Mirrors
   * `MLN_BROWSER_DISPATCH_PROTOCOL` in `src/browser/dispatch_table.h`.
   */
  const val EXPECTED_PROTOCOL: Int = 2

  private val indices = mutableMapOf<String, Int>()

  /**
   * Checks that the loaded module packs calls the way this binding does.
   *
   * The headers' digest cannot cover this. Two modules built from identical headers can still
   * disagree on the slot layout or the struct-return convention, and a host that packed for one and
   * called the other would mispack memory rather than fail.
   */
  fun verifyProtocol() {
    val protocol = dispatchProtocol()
    if (protocol != EXPECTED_PROTOCOL) {
      throw Status.invalidState(
        "The MapLibre Native browser module uses call protocol $protocol, but this binding " +
          "packs for $EXPECTED_PROTOCOL."
      )
    }
  }

  /**
   * Resolves an entry point once and remembers it.
   *
   * A name the module does not carry means the binding and the module disagree about the API, which
   * is worth reporting where it is noticed rather than as a call that quietly does nothing.
   *
   * The name crosses through the same scratch every other argument does, rather than through an
   * allocation of its own inside the snippet. That is not tidiness: the allocation the snippet made
   * was unchecked, and one the module refuses hands back the null pointer -- so the name went to
   * address zero, over the bottom of linear memory, and the lookup that followed reported an entry
   * point the module does not carry. A binding that disagrees with its module is what a host would
   * then have been told to go and look for, for a heap that had simply filled up.
   */
  fun index(name: String): Int =
    indices.getOrPut(name) {
      // Asked before the name is measured, because measuring reaches the module too and a module
      // that has been released would report a JavaScript type error naming its own helper rather
      // than the binding failure that says the host shut it down.
      BrowserModule.require()
      val resolved =
        Heap.withScratch(Heap.utf8Size(name)) { block ->
          Heap.storeUtf8(block, name)
          entryIndex(block.address)
        }
      if (resolved < 0) {
        throw Status.invalidState(
          "The MapLibre Native browser module carries no entry point named $name."
        )
      }
      resolved
    }

  /** Reports how many slots [index] reads, including a struct return's destination. */
  fun slotCount(index: Int): Int = entrySlots(index)

  /**
   * Performs the call named [name] with [slotCount] arguments.
   *
   * [fill] writes the arguments and [read] takes the result, both against scratch this owns, so an
   * argument buffer never outlives the call it was packed for.
   */
  fun <T> call(name: String, slotCount: Int, fill: (Slots) -> Unit, read: (HeapPointer) -> T): T {
    val entry = index(name)
    // Slots and the result share one allocation, so a call costs one acquisition
    // rather than two.
    val bytes = (slotCount + 1) * SLOT_BYTES
    return Heap.withScratch(bytes) { scratch ->
      val result = scratch + slotCount * SLOT_BYTES
      fill(Slots(scratch))
      // This one runs on the page, so the page's own diagnostic slot is the authority on what it
      // says, and it is written by the invocation below. Saying so matters only where this call is
      // nested inside a dispatched call's result read -- a read that copies a native list or
      // snapshot makes these page-thread calls -- because that read has an owner-thread message
      // standing, and it is not this failure's.
      NativeDiagnostics.forPageCall {
        if (!invokeHere(entry, scratch.address, slotCount, result.address)) {
          // The module rejects a call it cannot place: an index it does not carry,
          // or a buffer shorter than the entry reads. Both mean this binding and
          // the module disagree, so neither is worth retrying.
          throw Status.invalidState(
            "The MapLibre Native browser module rejected a call to $name with $slotCount " +
              "slots; it reads ${slotCount(entry)}."
          )
        }
        read(result)
      }
    }
  }

  /** Writes a call's arguments, one eight-byte slot each. */
  internal class Slots(private val base: HeapPointer) {
    fun setInt(index: Int, value: Int) {
      // Widened rather than written as four bytes: a slot is read back as a
      // 64-bit value whatever the parameter's declared width.
      Heap.storeLong(base + index * SLOT_BYTES, value.toLong() and 0xFFFFFFFFL)
    }

    fun setLong(index: Int, value: Long) {
      Heap.storeLong(base + index * SLOT_BYTES, value)
    }

    fun setDouble(index: Int, value: Double) {
      Heap.storeDouble(base + index * SLOT_BYTES, value)
    }

    /** Writes a pointer, which is 32 bits on this target and zero-extended. */
    fun setPointer(index: Int, pointer: HeapPointer) {
      Heap.storeLong(base + index * SLOT_BYTES, pointer.address.toLong() and 0xFFFFFFFFL)
    }
  }

  private const val SLOT_BYTES = 8
}
