#ifndef MLN_BROWSER_DISPATCHER_H
#define MLN_BROWSER_DISPATCHER_H

#include <stdbool.h>
#include <stdint.h>

#include "maplibre_native_c/base.h"

// The dispatcher's entry points are reached from JavaScript, which resolves a
// name against the module rather than against a declaration, so most of them
// need no header at all. The one below is different: it is how another
// translation unit in this module places its own work on the owner thread, and
// a C caller needs a prototype the compiler can check the call against.

// A dispatcher and the thread it owns. Opaque here; see dispatcher.c.
typedef struct mln_browser_dispatcher mln_browser_dispatcher;

// One unit of module-local work, performed on the dispatcher's thread.
typedef void (*mln_browser_dispatcher_task)(void* argument);

MLN_API bool mln_browser_dispatcher_submit_task(
  mln_browser_dispatcher* dispatcher, mln_browser_dispatcher_task task,
  void* argument, uint32_t token
) MLN_NOEXCEPT;

#endif  // MLN_BROWSER_DISPATCHER_H
