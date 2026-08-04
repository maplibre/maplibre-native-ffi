---
title: Browser module contract
description: The prelinked WebAssembly module a browser host calls, and the entry points it adds.
---

Every other platform ships a library that a host loads and calls through its own
foreign-function interface. A browser host has no link step, and an Emscripten
archive is only linkable by the emsdk version that produced it. So the browser
target's distributable artifact is the linked module itself, and the entry
points it carries beyond the C API are part of what it distributes.

The `emscripten-wasm32-webgl` build produces four files. Three sit under
`browser/` in the build tree and the export list sits at its root; installing
puts all four under `lib/browser`:

| File                         | Contents                                           |
| ---------------------------- | -------------------------------------------------- |
| `maplibre_native_c.mjs`      | The ES module factory                              |
| `maplibre_native_c.wasm`     | The module                                         |
| `maplibre_native_c-abi.json` | Lowered signatures, struct layouts, headers digest |
| `*-exports.txt`              | The exported symbol list the link was given        |

The factory is the module's default export. It returns a promise, because the
pthread pool spawns before it resolves. A page embedding this build is
cross-origin isolated, because pthreads need `SharedArrayBuffer`.

## Calling the C API

A host resolves an entry point by name once and then calls everything through
one function, rather than declaring 278 imports and keeping them in step with
the headers.

`mln_browser_entry_index` maps a C name to an index, or reports -1 for a name
this module does not carry. `mln_browser_entry_slots` reports how many argument
slots that entry reads, and `mln_browser_entry_total` reports how many entries
this module carries. `mln_browser_invoke_here` performs the call on the calling
thread, taking the index, a packed argument buffer, the count of slots the host
supplied, and a result slot.

Arguments travel as eight-byte slots whatever their declared width, which lets
one buffer layout serve every entry point. Width alone does not specify the
encoding: an integer, an enum, and a pointer occupy the slot as an integer, a
`double` occupies it as its own IEEE-754 representation, and a `float` occupies
the low four bytes as its own. A scalar result uses the matching representation.
Converting a `double` to an integer before writing it hands native garbage. An
entry point that returns a struct by value takes the destination as its first
slot, which is what its lowered signature does anyway. A struct passed by value
travels as the address of a copy, because the target passes those indirectly.

The boolean reports whether the entry point was invoked. It is false for an
index this module does not carry, for a slot count below what the entry reads,
for a null result slot, and for null slots when the entry reads any. The result
slot is unwritten in each case. Read it only after a true. Whatever status the C
API itself returned arrives in the result slot.

## Running on an owner thread

MapLibre blocks: it drains queues, makes synchronous cross-thread calls, and
joins during teardown. A runtime created on the page deadlocks the first time
any of those happens, because a page may not block. A dispatcher owns a thread
that may block instead.

`mln_browser_dispatcher_create` starts that thread. It returns null when the
allocation, the mutex, the condition variable, or the thread itself cannot be
created.

`mln_browser_dispatcher_submit` places one call on that thread, carrying the
same index and slots that a direct call takes, plus a token the host chooses.
Submission can be refused: it reports false for a null dispatcher or result
slot, for a dispatcher that is stopping, when 256 calls are already outstanding,
and when the call cannot be allocated. A host that parks on a token it did not
check waits forever, because no completion follows a refusal.

`mln_browser_dispatcher_take_completion` reports whether a completion was taken.
It is false for a null dispatcher, for either output being null, and for an
empty ring, and it leaves both outputs unwritten. On true it writes the token
and a separate value saying whether that call's entry point was invoked — the
same distinction a direct call draws, and false there for an unknown index, a
short slot count, or null slots an entry reads.

A host polls the ring on whatever cadence it already runs. There is no readiness
signal and no callback.

Four preconditions bind the host:

- One thread owns a dispatcher's lifetime. A stop that races a submit frees
  memory the submit is still reaching for, because the dispatcher's lock
  protects its fields rather than the object. A browser host is a single
  JavaScript agent, which is what makes this safe.
- A token is unique among outstanding calls. Two calls sharing one resolve
  whichever the host mapped last.
- Argument slots, the result slot, and everything a slot points at stay
  untouched until the completion arrives. Treating that storage as input-only
  races the thread's own writes: it writes results and output parameters as well
  as reading arguments.
- A callback reachable from a submitted call is callable from the dispatcher's
  thread or a MapLibre worker. A resource provider, transform, or custom
  geometry callback installed on the page as an `addFunction` trampoline reaches
  nothing from there, for the same reason log records are queued rather than
  delivered.

`mln_browser_dispatcher_stop` stops the thread without waiting, and
`mln_browser_dispatcher_destroy` waits for it. A page host uses the first,
because Emscripten implements a join by spinning. Both require that the host has
already destroyed its owner-affine handles and drained its outstanding calls: a
runtime, map, or render session can only be destroyed on the thread that owns
it.

## Receiving log records

MapLibre dispatches a log record from whichever thread produced it. A call from
a MapLibre worker reaches nothing when the callback is a JavaScript function
installed with Emscripten's `addFunction` on the page, because that function is
bound to the agent that installed it. The module therefore queues records and
the host drains them.

`mln_browser_log_install` registers the queue and reports the registration
status. Install it from one thread: a second caller arriving while the first is
still registering is told the callback is installed before it is. It also fixes
the value reported to MapLibre for every dispatched record, where zero leaves
the record to MapLibre's platform logger and a non-zero value consumes it. The
first successful installation fixes that value, and later ones report success
without changing it, because queued host code cannot answer per record.

`mln_browser_log_mark` reports how many records have been enqueued so far.
`mln_browser_log_take_since` returns the oldest record enqueued at or after a
mark, releasing anything older, and returns null once no qualifying record
remains. Null is what ends a drain. The host owns each record it receives and
releases it with `mln_adapter_log_record_destroy`.

`mln_browser_log_take_dropped` reports records the bounded ring dropped and
resets the count. Successive reads are therefore deltas rather than running
totals. A drop means the ring was full when a record arrived, which happens when
the host stops draining and equally when producers outrun a drain that is
running.

A host takes a mark when it starts listening and passes that mark back on every
take. That excludes every record enqueued before the mark. A record produced
just before the mark but enqueued just after it remains eligible, because the
listener stamps a record when it queues it rather than when MapLibre produced
it. Eligibility is not delivery: the bounded ring can still evict it before the
host drains.

The registration lasts the module's lifetime. A module that unregistered could
not tell which registration a record came from: the adapter's listener takes no
user data, while the adapter treats a registration's address as its identity. A
host stops receiving records by dropping its own callback. This module owns the
process-global log callback: anything registering through `mln_log_set_callback`
or `mln_adapter_log_set_callback` afterwards retires it permanently.

## Checking compatibility

The documented ABI version cannot tell a host that its generated offsets no
longer match the module it loaded, because that version stays 0 for the whole
prerelease.

`mln_browser_headers_digest` reports the digest of the public headers this
module was built from. `mln_browser_dispatch_protocol` reports the call protocol
version, and that version covers the slot layout, the argument order, and the
struct-return convention. Both change independently of each other.

The ABI manifest records the same digest and protocol. A host checks the
manifest before instantiating and the module's own values afterwards. The first
check refuses a mismatched module before its worker pool starts. The manifest
describes whatever file sits beside the module; the module describes itself.
