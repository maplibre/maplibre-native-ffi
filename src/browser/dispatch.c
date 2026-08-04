// The browser module's generic call entry points.
//
// A host without a C toolchain cannot declare 278 imports and keep them in step
// with the headers. It resolves each entry point by name once and then calls
// everything through one function, passing a packed argument buffer. The
// generated table is what turns that index back into a checked call.
//
// This half is deliberately thread-agnostic: it performs the call on whichever
// thread asks. Placing a call on the thread that owns a runtime is the
// dispatcher's job, and it is built on top of this rather than inside it, so a
// host that already owns the right thread pays nothing for the machinery a page
// host needs.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "browser/dispatch_table.h"
#include "maplibre_native_c/base.h"

/**
 * Resolves an entry point's index, or -1 when this module has no such entry.
 *
 * A host resolves once at startup. A name this module does not carry is a
 * mismatch between the host's generated bindings and the module it loaded,
 * which is worth reporting as a missing entry rather than as a call that does
 * nothing.
 */
MLN_API int32_t mln_browser_entry_index(const char* name) MLN_NOEXCEPT {
  if (name == NULL) {
    return -1;
  }
  for (uint32_t index = 0; index < mln_browser_entry_count; ++index) {
    if (strcmp(mln_browser_entries[index].name, name) == 0) {
      return (int32_t)index;
    }
  }
  return -1;
}

/** Reports how many entry points this module carries. */
MLN_API uint32_t mln_browser_entry_total(void) MLN_NOEXCEPT {
  return mln_browser_entry_count;
}

/**
 * Reports the generic call protocol this module was built for.
 *
 * A host checks this before its first call. The public headers' digest cannot
 * cover it: two modules built from identical headers can still disagree on the
 * slot layout or the struct-return convention, and a host that packed for one
 * and called the other would mispack memory rather than fail.
 */
MLN_API uint32_t mln_browser_dispatch_protocol(void) MLN_NOEXCEPT {
  return MLN_BROWSER_DISPATCH_PROTOCOL;
}

/** Reports how many slots an entry reads, or zero for an unknown index. */
MLN_API uint32_t mln_browser_entry_slots(uint32_t index) MLN_NOEXCEPT {
  return index < mln_browser_entry_count ? mln_browser_entries[index].slot_count
                                         : 0;
}

/**
 * Performs one call on the calling thread.
 *
 * `slots` holds the arguments in declaration order, `slot_count` says how many
 * the host supplied, and `result` receives the return value. All are ordinary
 * module memory the host allocated, so nothing here owns them.
 *
 * The boolean reports whether the call was placed, which is separate from
 * whatever status the entry point itself returned in `result`. A bad index or a
 * buffer too short for the entry reports false and calls nothing: reading past
 * a short buffer would hand native a pointer made of whatever followed it.
 */
MLN_API bool mln_browser_invoke_here(
  uint32_t index, const mln_browser_slot* slots, uint32_t slot_count,
  mln_browser_slot* result
) MLN_NOEXCEPT {
  if (index >= mln_browser_entry_count || result == NULL) {
    return false;
  }
  const mln_browser_entry* entry = &mln_browser_entries[index];
  if (slot_count < entry->slot_count) {
    return false;
  }
  if (entry->slot_count > 0 && slots == NULL) {
    return false;
  }
  entry->invoke(slots, result);
  return true;
}
