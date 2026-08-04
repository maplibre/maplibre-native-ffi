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
allocation, the mutex, or the thread itself cannot be created.

`mln_browser_dispatcher_create_with_canvases` starts it owning page canvases the
host names, as a comma-separated list of canvas element ids. Emscripten moves an
`OffscreenCanvas` between agents only at `pthread_create`, so **this is the only
moment a canvas can reach the thread that renders**. A host that will ever
attach a surface session names its canvases here, before its first call, and
calls this from the page: only the agent holding a canvas can give it away. The
canvases stay with the thread for its life, so a session can be closed and
another attached to the same canvas. It returns null for the same reasons
`mln_browser_dispatcher_create` does, and additionally when a named canvas is
not in the document or control of it has already been transferred.

A canvas given away this way is no longer drawable from the page. The `<canvas>`
element becomes a placeholder that displays what the owner thread renders, with
no copy.

The thread runs one browser task per batch of calls rather than one task for its
whole life, and that is load-bearing rather than incidental. A browser
composites a canvas when the task that drew into it ends, so a thread that
rendered and then parked inside the same task would draw frames nothing ever
displays. Everything a submitted call reaches has to tolerate returning to an
event loop between calls; blocking _within_ a call is still legal, and is what
the thread exists for.

`mln_browser_dispatcher_submit` places one call on that thread, carrying the
same index and slots that a direct call takes, plus a token the host chooses.
Submission can be refused: it reports false for a null dispatcher or result
slot, for a dispatcher that is stopping, when 256 calls are already outstanding,
and when the call cannot be allocated. A host that parks on a token it did not
check waits forever, because no completion follows a refusal.

`mln_browser_dispatcher_take_completion` reports whether a completion was taken.
It is false for a null dispatcher, for a null token or invoked output, and for
an empty ring, and it leaves its outputs unwritten. On true it writes the token
and a separate value saying whether that call's entry point was invoked — the
same distinction a direct call draws, and false there for an unknown index, a
short slot count, or null slots an entry reads.

It also writes the diagnostic that the call left behind, into a buffer that the
host supplies with the buffer's capacity in bytes. The message arrives as
null-terminated UTF-8, empty for a call that did not fail, and truncated on a
UTF-8 boundary when it is longer than the buffer. A host that has no use for the
message passes a null buffer and a capacity of zero.

**The completion is the only place a dispatched call's message can be read.**
The C API's diagnostic is thread-local, so a failure on the dispatcher's thread
writes that thread's slot, and the next call placed there replaces it. A host
that reads `mln_thread_last_error_message` after its own caller resumes reads
its own thread's slot, which the dispatched call never wrote. The owner thread
therefore copies the message where it was produced, at the moment the call
finishes, and the completion carries it back. The copy is at most 512 bytes
including the terminator, so a longer message reaches the host cut short.

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

`mln_browser_dispatcher_submit_task` places work the module itself owns on that
thread. It takes no index and no slots, because its caller is another
translation unit in this module rather than a host packing a buffer; everything
else — the capacity bound, the token rules, the completion — is what
`mln_browser_dispatcher_submit` does. Its completion always reports that the
call was invoked, since there is no index or slot count that could be rejected.
The WebGL entry points below are the only caller.

## Creating a WebGL context

A browser host has no way to make a context a render target can use. The handle
in `mln_webgl_context_descriptor` is an index into this module's own context
table, so a context the page created with `canvas.getContext("webgl2")` is not
one native can look up, and a WebGL context belongs to the thread that created
it — which for a page host is the dispatcher's thread. The module therefore
creates contexts, on the thread that renders through them.

`mln_browser_webgl_context_create` places that work on a dispatcher's thread and
reports the handle through a host pointer it writes before the completion for
the token arrives. That pointer follows the same rule the argument slots do: it
stays the host's, and untouched, until then. The handle is zero when creation
failed, which is also the value the C API refuses in a descriptor.
`mln_browser_webgl_context_destroy` releases one the same way.

`mln_browser_webgl_context_create_here` and
`mln_browser_webgl_context_destroy_here` are the same work on the calling
thread, for a host that already owns the thread it renders on. The affinity rule
is the whole contract: create the context on the thread that will own the render
session, and destroy it there, after every target that borrowed the handle is
detached or destroyed.

Both take a canvas element id, and which kind of canvas it names decides what a
session can do with the context.

- A page canvas the dispatcher was created with. What a surface session renders
  into that context's default framebuffer is composited onto the page's canvas
  with no readback and no copy, which is the only zero-copy presentation a
  browser has.
- A private `OffscreenCanvas` this module constructs, when the id is null or
  empty. Nothing displays it. A texture session draws into a framebuffer of its
  own and never touches it, so it exists only because a WebGL context cannot be
  created without a canvas; the frame leaves through
  `mln_texture_read_premultiplied_rgba8`, which has no owner thread of its own.

Width and height size the drawing buffer in device pixels either way. For a
texture session they bound nothing the map renders and only have to be positive;
for a surface session they are the session's physical extent.

`mln_browser_webgl_canvas_resize` and `mln_browser_webgl_canvas_resize_here`
size a canvas's drawing buffer afterwards. A page owns its canvas's layout size
and the owner thread owns its drawing buffer, because a canvas given to another
thread can only be sized there, so a host resizes in two steps: this, and then
`mln_render_session_resize` or `mln_opengl_surface_set_target` with the matching
extent.

An OpenGL surface descriptor's `surface` field must be null for WebGL. Every
other provider names a drawable beside the context — an HDC, an EGLSurface — and
a browser has none: the context is bound to the canvas it was created on, and
that canvas's default framebuffer is what the session presents to. There is no
swap; the browser composites.

## Doing the host's own GL work

Every other platform expects a host to issue graphics calls of its own on the
thread it renders on: making the texture a caller-owned target draws into,
putting a rendered frame where the user can see it, reading one back. A browser
host owns no such thread, and cannot reach the one that renders — a WebGL
context belongs to the agent that created it, and WebGL shares no objects
between contexts, so a texture the page made through
`canvas.getContext("webgl2")` names nothing a session could attach. The module
therefore carries that work, placed where it has to run. Each entry point has a
`_here` form for a host that already owns the render thread, and a dispatched
form that answers through the completion ring.

`mln_browser_webgl_texture_create` makes an RGBA8 texture in a context, and its
name is what `mln_opengl_borrowed_texture_descriptor.texture` carries. The
texture is the host's: nothing tracks it, a render target only borrows it, and
`mln_browser_webgl_texture_destroy` is what releases it — before the context it
was made in is destroyed, or with that context, which releases everything made
in it.

`mln_browser_webgl_present_texture` blits a texture onto the default framebuffer
of the context that owns it. This is how a texture target's frame reaches the
page, and it is zero-copy in the sense that matters in a browser: the pixels
stay in GPU memory, never enter the module's heap, and never cross an agent
boundary. A surface target needs nothing, because it already renders into that
framebuffer. Either way the browser composites the canvas when the task that
drew into it ends, so the frame appears a page turn later rather than as the
call returns.

`mln_browser_webgl_read_pixels` reads a frame back instead, from a texture or
from the default framebuffer, as RGBA8 with row zero at the bottom. The C API's
own `mln_texture_read_premultiplied_rgba8` covers session-owned texture targets
and refuses the other two families, which everywhere else is not a gap because
the host reads its own texture with its own graphics API; here it would be one.
This is the expensive way to use a frame — it stalls the owner thread until the
GPU is done — and a host that only wants to show one presents it instead.

Every one of these restores the GL state it changed, including the scissor
enable that a blit is clipped by. MapLibre's GL backend remembers what it last
set and skips a redundant call, so state left changed behind its back is state
the next frame renders against without knowing.

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
version, and that version covers the slot layout, the argument order, the
struct-return convention, and what a completion reports. Both change
independently of each other.

The ABI manifest records the same digest and protocol. A host checks the
manifest before instantiating and the module's own values afterwards. The first
check refuses a mismatched module before its worker pool starts. The manifest
describes whatever file sits beside the module; the module describes itself.
