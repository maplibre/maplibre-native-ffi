package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.internal.wasm.generated.MlnAdapterResourceRewriteRule
import org.maplibre.nativeffi.internal.wasm.generated.MlnAdapterResourceRewriteRules
import org.maplibre.nativeffi.internal.wasm.generated.MlnAdapterUrlMatchFlags
import org.maplibre.nativeffi.internal.wasm.generated.MlnResourceTransform
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_rewrite_transform_callback
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_clear_resource_transform
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_set_resource_transform
import org.maplibre.nativeffi.resource.ResourceUrlRewriteRule

/**
 * One runtime's native rule table, which answers this target's resource transforms.
 *
 * MapLibre needs a transformed URL on the thread that raised the request, and a rule table is the
 * answer the C API offers a binding that cannot run host code there. The table is read only while
 * the transform is registered, so the block holding it is released once the call that replaced or
 * cleared it returns.
 */
internal class ResourceRewriteRules {
  private var installed: HeapPointer? = null
    set(value) {
      if ((field == null) != (value == null)) liveRegistrations += if (value == null) -1 else 1
      field = value
    }

  /** Registers or replaces [runtime]'s rewrite rules. */
  fun set(runtime: Long, rules: List<ResourceUrlRewriteRule>) {
    val block = place(rules)
    try {
      InjectedFaults.beginCall(SET)
      Status.check(mln_runtime_set_resource_transform(runtime, block.descriptor.address))
    } catch (error: Throwable) {
      Heap.release(block.base)
      throw error
    }
    // The call above returned, so native reads the previous table no more.
    installed?.let { Heap.release(it) }
    installed = block.base
  }

  /** Clears [runtime]'s rewrite rules, which native accepts whether or not a table was set. */
  fun clear(runtime: Long) {
    InjectedFaults.beginCall(CLEAR)
    Status.check(mln_runtime_clear_resource_transform(runtime))
    installed?.let { Heap.release(it) }
    installed = null
  }

  /** Releases the installed table after native has dropped it with the runtime that held it. */
  fun release() {
    installed?.let { Heap.release(it) }
    installed = null
  }

  private class Placed(val base: HeapPointer, val descriptor: HeapPointer)

  /** Places the transform descriptor, the rule table, and every rule URL in one block. */
  private fun place(rules: List<ResourceUrlRewriteRule>): Placed {
    rules.forEach {
      Heap.requireCString(it.url, "rule url")
      it.replacementUrl?.let { replacement -> Heap.requireCString(replacement, "replacement url") }
    }
    var total = HeapArena.aligned(MlnResourceTransform.SIZEOF.toLong(), POINTER_ALIGN)
    total += HeapArena.aligned(MlnAdapterResourceRewriteRules.SIZEOF.toLong(), POINTER_ALIGN)
    total +=
      HeapArena.aligned(
        Heap.sizeOf(MlnAdapterResourceRewriteRule.SIZEOF, rules.size).toLong(),
        POINTER_ALIGN,
      )
    rules.forEach {
      total += Heap.utf8Size(it.url).toLong()
      it.replacementUrl?.let { replacement -> total += Heap.utf8Size(replacement).toLong() }
    }
    Status.requireArgument(total <= Int.MAX_VALUE) { "the rule table is too large to place" }

    val base = Heap.acquire(total.toInt())
    try {
      val arena = HeapArena(base, total.toInt())
      val descriptor = arena.allocate(MlnResourceTransform.SIZEOF, POINTER_ALIGN)
      val table = arena.allocate(MlnAdapterResourceRewriteRules.SIZEOF, POINTER_ALIGN)
      val entries =
        arena.allocate(Heap.sizeOf(MlnAdapterResourceRewriteRule.SIZEOF, rules.size), POINTER_ALIGN)
      rules.forEachIndexed { index, rule ->
        val entry = entries + index * MlnAdapterResourceRewriteRule.SIZEOF
        MlnAdapterResourceRewriteRule.setKind(entry, rule.kind?.nativeValue ?: RESOURCE_KIND_ANY)
        MlnAdapterResourceRewriteRule.setFlags(
          entry,
          if (rule.matchGlob) {
            MlnAdapterUrlMatchFlags.MLN_ADAPTER_URL_MATCH_GLOB
          } else {
            MlnAdapterUrlMatchFlags.MLN_ADAPTER_URL_MATCH_FLAGS_NONE
          },
        )
        MlnAdapterResourceRewriteRule.setUrl(entry, write(arena, rule.url))
        // A null replacement leaves the URL unchanged, so it stays the null pointer the zeroed
        // block already holds.
        rule.replacementUrl?.let {
          MlnAdapterResourceRewriteRule.setReplacementUrl(entry, write(arena, it))
        }
      }
      MlnAdapterResourceRewriteRules.setRules(table, entries)
      MlnAdapterResourceRewriteRules.setCount(table, rules.size)
      MlnResourceTransform.setSize(descriptor, MlnResourceTransform.SIZEOF)
      // The layout generator leaves a function-pointer field to its caller, so the table index the
      // shim reports is written at the offset the generator declares for it.
      Heap.storeInt(
        descriptor + MlnResourceTransform.OFFSET_CALLBACK,
        mln_kotlin_rewrite_transform_callback(),
      )
      MlnResourceTransform.setUserData(descriptor, table)
      return Placed(base, descriptor)
    } catch (error: Throwable) {
      Heap.release(base)
      throw error
    }
  }

  private fun write(arena: HeapArena, text: String): HeapPointer {
    val pointer = arena.allocate(Heap.utf8Size(text), BYTE_ALIGN)
    Heap.storeUtf8(pointer, text)
    return pointer
  }

  internal companion object {
    /**
     * The rule tables still holding a block of the module's heap, across every runtime.
     *
     * For the tests: a table native refused holds none, so this is what says a refusal left the
     * previous one standing and nothing else.
     */
    var liveRegistrations: Int = 0
      private set

    const val SET = "mln_runtime_set_resource_transform"
    const val CLEAR = "mln_runtime_clear_resource_transform"
    const val POINTER_ALIGN = 4
    const val BYTE_ALIGN = 1
  }
}
