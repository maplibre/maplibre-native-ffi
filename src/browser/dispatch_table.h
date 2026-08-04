#ifndef MLN_BROWSER_DISPATCH_TABLE_H
#define MLN_BROWSER_DISPATCH_TABLE_H

#include <stdbool.h>
#include <stdint.h>

#include "maplibre_native_c/base.h"

// The shape of the generic call protocol: the slot width, the argument order,
// the struct-return convention, and the signatures below.
//
// The public headers' digest cannot stand in for this. Two modules built from
// identical headers can still disagree here, and a host that packed for one and
// called the other would mispack memory rather than fail. Bump this whenever
// any of those change.
#define MLN_BROWSER_DISPATCH_PROTOCOL 1

// One packed call argument, or a call's result.
//
// Every slot is eight bytes whatever the declared type is, so one buffer layout
// serves every entry point. A float occupies the slot as itself rather than as
// a converted integer: the union is what keeps the bits intact across the
// page's write and the owner thread's read.
typedef union mln_browser_slot {
  uint64_t u;
  double f64;
  float f32;
} mln_browser_slot;

// Performs one call. `slots` holds the arguments in declaration order, and
// `result` receives the return value, or zero when the entry point returns
// nothing. An entry point that returns a struct by value takes the destination
// as its first slot, which is what its lowered signature does anyway.
typedef void (*mln_browser_invoke)(
  const mln_browser_slot* slots, mln_browser_slot* result
);

typedef struct mln_browser_entry {
  // The C name, so a host resolves an entry by name once and then passes the
  // index. Names travel with the module rather than being a second artifact a
  // host has to keep in step with it.
  const char* name;
  // Slots this entry reads, including the destination slot a struct-returning
  // entry takes first. A wrapper reads exactly this many, so a host that
  // supplies fewer is rejected before the read rather than after it.
  uint32_t slot_count;
  mln_browser_invoke invoke;
} mln_browser_entry;

extern const mln_browser_entry mln_browser_entries[];
extern const uint32_t mln_browser_entry_count;

// Declared here rather than only in the translation unit that defines them,
// because the dispatcher performs its calls through the same entry point a
// direct caller uses.
MLN_API int32_t mln_browser_entry_index(const char* name) MLN_NOEXCEPT;
MLN_API uint32_t mln_browser_entry_total(void) MLN_NOEXCEPT;
MLN_API uint32_t mln_browser_dispatch_protocol(void) MLN_NOEXCEPT;
MLN_API uint32_t mln_browser_entry_slots(uint32_t index) MLN_NOEXCEPT;
MLN_API bool mln_browser_invoke_here(
  uint32_t index, const mln_browser_slot* slots, uint32_t slot_count,
  mln_browser_slot* result
) MLN_NOEXCEPT;

#endif  // MLN_BROWSER_DISPATCH_TABLE_H
