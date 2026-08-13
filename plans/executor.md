# Execution model

## Decision

The C API owns runtime and map execution. A binding adapts completion and
notification into its host runtime. It does not create a native thread, run a
MapLibre work queue, pump MapLibre, or preserve native thread identity.

Render execution follows the render target. A worker-capable target uses a
core-owned render worker. A thread-current target uses the graphics thread that
already owns that target. The second form calls a narrow render-driver API; it
does not run the MapLibre runtime or implement a general executor.

This gives every host one runtime model:

- A runtime owns one native thread and one continuously running MapLibre run
  loop.
- Maps and projections submit work to their runtime.
- Commands return after the C API has copied and accepted their input.
- Published snapshots return copied state without a runtime-thread round trip.
- Queries and lifecycle transitions complete through operation handles.
- Hosts drain owned event batches after a notification.
- A render target determines where graphics calls execute.

Runtime progress is independent of host calls. Runtime, map, and projection
operations have the same thread-safe contract from every host thread.

The runtime execution thread is an implementation detail. A public executor
handle would expose placement and lifetime policy that ordinary integrations do
not need. If a concrete integration needs to reuse a native thread across
runtime lifetimes or control thread priority, the C API may add a core-created
execution-group handle. Supplying one would change placement only; every runtime
would keep the same command, operation, snapshot, event, and lifecycle
semantics.

## Why the runtime owns its thread

A stable host thread answers which calls are legal. It does not bound the cost
of those calls. MapLibre runtime work can include a style parse, scheduler
drain, timer callback, or I/O completion. Running that work on a UI thread makes
input latency depend on unrelated native work even when the host can preserve
thread identity perfectly.

The C API is also below platform UI frameworks. A platform SDK can integrate
MapLibre directly into a main looper or run loop because it owns both sides of
that integration. A pure-FFI binding cannot provide an arbitrary native closure
runner, and every binding that builds its own dedicated thread duplicates the
same queue, wake, shutdown, and diagnostic machinery.

Core ownership gives calls a stable cost model. UI threads copy commands and
read snapshots. Ordered work executes continuously on the runtime thread.
Bindings translate operations and notification into their ordinary async
mechanisms.

This decision applies to runtime and map work. Graphics APIs add a separate
constraint: a context, surface, or compositor may already belong to a specific
thread. The render-driver contract preserves that constraint without moving
runtime ownership back into the host.

## Execution contract

### Runtime scheduling

Runtime creation starts a native thread. That thread creates the
`mbgl::util::RunLoop`, constructs the runtime state, and calls `RunLoop::run()`.
The run loop remains active until runtime shutdown completes.

Submissions use the run loop's invocation mechanism. The implementation MUST use
`RunLoop::invoke()` or the scheduler primitive that replaces it. It MUST NOT
maintain a condition-variable loop that alternates a separate FIFO queue with
`RunLoop::runOnce()`.

Submitting a command or operation through that mechanism wakes an idle run loop.
Runtime work never waits for a host poll, render request, or display callback.

`runOnce()` processes work that is ready at that instant. A condition variable
that knows only about C API submissions and wake flags cannot derive the next
libuv timer deadline. Such an executor could sleep while the run loop has a
future timer to service. The native run loop already owns queueing, timers, I/O,
and wakeup behavior, so it remains the only scheduler for runtime work.

One runtime may own multiple maps. Map operations share the runtime's ordered
submission stream. One runtime uses one native scheduler thread, which places no
restriction on a host thread.

### Render drivers

A render session remains bound to one map, render target, and driver for its
lifetime:

| Driver                 | Target requirement                                                             | Execution                                                                  |
| ---------------------- | ------------------------------------------------------------------------------ | -------------------------------------------------------------------------- |
| Core worker            | The target and graphics state can be created or transferred to a native worker | The C API owns a serial render worker and accepts nonblocking frame demand |
| Caller graphics thread | The target or context is current only on an existing host graphics thread      | The host calls the narrow render-driver functions from that thread         |

The target descriptor determines the supported driver. Selecting a driver is not
a choice between two runtime models.

The core-worker driver executes attach, target initialization, frame production,
readback, detach, and close on its worker. The caller-graphics-thread driver
executes target initialization, rendering, presentation, and target detach on
the thread where the graphics context is current. Its remaining control calls,
including frame-demand state, snapshots, and operation inspection, are safe from
any thread.

Accepting eligible frame demand or renderer-affine work wakes an idle core
render worker. The worker does not wait for a host poll or display callback.

Rendered-feature queries, source-feature queries, readback, and other
renderer-affine work retain one operation API across both drivers. A core-worker
session executes them on its worker. A caller-graphics-thread session stores
them in a C-owned typed driver mailbox. Its graphics receiver is notified that
driver work is ready, and a driver-service call executes that work on the
graphics thread even when no frame is presented.

The typed mailbox accepts only the render-session operations that the C API
defines. It cannot carry arbitrary native or host closures. An operation has one
notification source for completion, while the session's driver mailbox may use a
different source for its graphics receiver.

A caller-graphics-thread attach is a driver call on that thread. Target detach
releases every thread-current graphics resource on the same thread. The session
then closes through an any-thread operation. Session close returns invalid state
while a caller-thread target is still attached, so native code never needs to
schedule work onto a host graphics thread.

Normal lifecycle performs target detach before the graphics thread exits. When
that thread or realm is irrecoverably lost, an any-thread abandon-target call
provides the forced path. Abandonment is irreversible and performs no graphics
API calls. It marks the target lost, rejects new driver work, completes pending
driver operations with a target-lost status, and invalidates outstanding frame
accessors.

Every driver call holds a session lease. Abandonment atomically closes the typed
mailbox and succeeds only when no driver call is in flight; it returns busy
without changing state otherwise. A host that forcibly terminates a thread in
the middle of native code cannot establish that precondition, so the C API
cannot safely recover that session. Surface or realm loss between driver calls
is recoverable; asynchronous thread cancellation during a call is outside the
contract and may leak the session.

Resources that can only be destroyed through the lost graphics context enter a
quarantine that lasts until the host or operating system destroys that context.
The abandonment result reports the quarantine. The C API never reclaims such
resources while the GPU may still reach them. Caller-owned resources remain the
caller's responsibility. After abandonment, the session can release its CPU
state and close without the graphics thread.

The caller-graphics-thread surface is deliberately narrow. It initializes and
detaches the target, renders, presents, and services typed driver work. It does
not accept arbitrary closures, advance the runtime, drain runtime events, or
execute map commands. An integration uses the display or compositor thread that
it already has, such as a Skiko redraw thread, a GLFW event thread, or a browser
WebGL thread.

Runtime and render execution communicate through immutable render updates,
commands, generations, and GPU synchronization. A runtime worker MUST NOT wait
synchronously for a render driver. A core render worker MUST NOT wait
synchronously for arbitrary runtime work. A caller-graphics-thread driver call
MUST NOT wait synchronously for runtime work.

### Public handles

Public handles remain opaque integer handles. Their registry entries hold the
control state that makes calls safe while execution and destruction race.

Runtime, map, projection, operation, notification source, event batch, and
acquired frame handles are callable from any native thread. Render-session
control calls are also any-thread. A caller-graphics-thread target documents the
small set of driver calls that require its graphics thread.

A binding may confine a wrapper when that is useful for host-language ownership.
Native runtime-thread affinity is not part of the C contract.

Registry lookup and submission form one atomic lifetime operation:

1. Resolve the handle under its registry lock.
2. Acquire a lease on its control state.
3. Reject a handle that is closing or closed.
4. Copy the command input or create the operation state.
5. Commit the submission before releasing the lease.

A committed submission reaches a terminal disposition. Destruction cannot
abandon it or access freed state.

## Public operation model

Each public function belongs to one category. The C API chooses the category; a
host never chooses between synchronous and asynchronous forms of the same
operation.

| Category           | Return boundary                                         | Result                                                                    |
| ------------------ | ------------------------------------------------------- | ------------------------------------------------------------------------- |
| Immediate          | The C call                                              | Input validation, handle lookup, capabilities, and copied registry state  |
| Command            | Input has been copied and accepted                      | Later state and events, correlated by command ID when failure is possible |
| Published snapshot | The C call                                              | A copied immutable view of the latest committed state                     |
| Operation          | Work completes on a core worker or typed driver service | A typed result, status, and copied diagnostic                             |
| Event batch        | A host drains an owned batch                            | Observations, command dispositions, repaint demand, and lifecycle changes |
| Render-driver call | The graphics-thread call                                | Rendering, typed work service, or target lifecycle for a caller target    |

Bindings MUST NOT invent a scheduler or a second operation boundary. They MUST
map C operation handles to the target language's ordinary suspension, promise,
future, task, or explicit blocking-wait idiom.

### Commands

Mutations are commands unless the caller needs a result that can only be
computed after execution. A command validates binding-owned and C-owned input
shape synchronously, deep-copies every value that outlives the call, assigns a
monotonic command ID, and enters the runtime's ordered stream.

The return status reports acceptance. A failure while applying an accepted
command enters the event stream with all of the following data:

- the command ID;
- the source handle and source kind;
- the terminal disposition and final status;
- a copied diagnostic;
- the committed generation, when the command changed state before failing.

Commands preserve submission order for one runtime. A query or barrier that is
submitted after a command observes that command's terminal disposition.
Submissions from concurrent host threads receive an order when the C API commits
them. The API promises that order rather than an order inferred from wall-clock
call start times.

Coalescing is an operation property. Absolute replaceable state, such as a
pending extent, may coalesce to its newest value. A coalesced command reaches
the terminal `superseded` disposition. It has no committed generation, and
barriers treat it as complete. Relative camera deltas, gesture boundaries,
lifecycle commands, and commands separated by a query or barrier retain every
submission.

Camera input uses one atomic command shape. A camera update can contain pan,
scale, bearing, pitch, anchor, padding, and gesture phase in one submission. The
atomic command preserves the gesture transaction that produced the update. A
gesture or animation ID lets events correlate will-change, changing, and
did-change observations.

Style mutations also use commands. State-dependent failures such as a duplicate
layer ID are asynchronous command failures. A separate conditional operation is
appropriate only when a caller needs a success result before continuing.

### Published snapshots

Published snapshots serve synchronous reads that must remain cheap on UI and
display threads. The initial snapshot set includes:

- camera state;
- map extent and scale factor;
- projection inputs required by UI layout;
- loading and fully-rendered state;
- latest render-update generation;
- repaint demand;
- render-session extent and latest completed frame generation.

Each snapshot contains its generation and is copied into caller-owned storage.
Snapshot publication uses an immutable object or equivalent synchronization; the
host never reads mutable MapLibre state directly.

A camera snapshot answers which camera belongs to the latest published render
state. An ordered camera query answers how MapLibre applied every preceding
command, including bounds and clamping. The names and documentation keep those
meanings distinct.

### Operations

Queries, resource creation that can fail after submission, readback, and
lifecycle transitions return an operation handle. Public ownership and internal
work ownership are separate:

```text
pending -> completed -> result taken -> released
   |           \------> result discarded -> released
   |\-> cancellation requested -> completed(CANCELLED)
   \-> observer released -> internal work continues -> result discarded
```

Completion stores the final status, a copied diagnostic, and an optional typed
result. Completion is permanent and safe to inspect from any native thread.
Taking a result transfers its owned native result exactly once. A failed take
that does not transfer ownership leaves the result available for retry.
`CANCELLED` is the generated operation disposition used by the manifest,
headers, events, and bindings; cancellation is a completion disposition rather
than a separate lifetime state.

Releasing an operation is always permitted. Releasing a completed operation
destroys an untaken result. Releasing a pending operation detaches the public
observer; the internal operation self-retains until work finishes and then
discards its result. Release requests cancellation first when cancellation is
supported, but release never depends on cancellation succeeding.

The common operation surface provides:

- a nonblocking completion query;
- an optional blocking wait for native applications;
- cancellation where the underlying work supports it;
- final status and copied diagnostic access;
- typed result take;
- result discard and release;
- association with a notification source.

Blocking wait is a convenience over the same operation. A binding that runs on a
cooperative executor, including Swift tasks, exposes suspension and MUST NOT
expose the blocking wait as its normal safe API. Browser and UI bindings do not
block their UI thread. Python releases the GIL around an explicit blocking wait.

Typed functions take typed results. A generic untagged result union would make
binding generation and ownership harder to audit. JSON and GeoJSON keep their
existing buffer transit. The typed take function and its documented JSON schema
identify the result, while `mln_buffer` carries the owned bytes. The operation
owns that buffer until a successful take transfers it to the host.

Illustrative shapes are:

```c
mln_status mln_render_session_query_rendered_features_start(
    mln_render_session session,
    const mln_rendered_query_geometry* geometry,
    const mln_rendered_feature_query_options* options,
    mln_operation* out_operation);

mln_status mln_render_session_query_rendered_features_take(
    mln_operation operation,
    mln_buffer* out_result);

mln_status mln_operation_poll(
    mln_operation operation,
    bool* out_completed);

mln_status mln_operation_wait(
    mln_operation operation,
    int64_t timeout_ms,
    bool* out_completed);
```

The exact names are settled during implementation. The operation and ownership
boundaries are fixed by this plan.

### Lifecycle

Creation and destruction use operations whenever work must run on a worker or
graphics driver. This includes runtime initialization, map creation, core-worker
render-session attach, and close. Bindings present those operations through
their normal construction and cleanup idioms.

Close has a synchronous preflight and an asynchronous commit:

1. Resolve the handle and acquire its control-state lease.
2. Check every recoverable precondition before changing state.
3. Return invalid state while a live child or acquired frame prevents close.
4. Leave the handle open when preflight fails.
5. Commit closing, reject new submissions, and create the close operation.
6. Complete accepted work, quiesce callbacks, release native resources, and
   retire the registry entry.

Preflight and the transition to closing share one linearization point. Child
creation reserves its parent dependency when creation is accepted. A parent
therefore checks both live children and pending child-creation reservations
before closing. A child creation racing parent close either reserves first and
causes preflight to fail, or observes closing and is rejected.

A runtime with a live map stays open, so the map can still submit and complete
its own close. A caller awaits child close before retrying parent close.

A caller-graphics-thread session with an attached target stays open until the
graphics thread detaches that target or the host irreversibly abandons it. A
render session with a live acquired frame also stays open during normal
lifecycle. The host releases the frame with consumer-completion synchronization
and retries close. Once every frame handle is released, an accepted close may
remain pending while GPU consumer work retires. The close operation owns that
wait; a host thread and a runtime worker do not block on it.

Abandonment is the target-loss exception. It invalidates outstanding frame
accessors and quarantines resources whose safe destruction cannot be proven, so
those frame handles no longer prevent CPU-side session close.

After close commits, it is irreversible. Every condition that the caller can
repair has already passed preflight. An abandoned close operation detaches its
observer while internal teardown continues. A native teardown error becomes the
operation's terminal diagnostic, but it does not reopen the handle; control
state remains internally retained until resources are safe to retire. Completion
makes every public alias observe closed state, and a later binding close is a
no-op.

Non-deterministic host cleanup may begin asynchronous close only when the
binding can retain every callback root and native dependency until completion.
Otherwise it reports a leak and preserves the state that native code can still
reach.

### Barriers and deterministic work

A runtime barrier is an operation submitted through the same ordered stream as
map commands and queries. It completes after every preceding submission has a
terminal disposition, including `superseded` commands.

A render barrier registers a dependency on a requested map and frame generation.
The render driver signals producer completion, and the barrier operation then
completes. No worker waits synchronously for the other worker or driver.

Tests, command-line applications, and batch rendering use barriers and operation
waits. They use the same continuously running runtime as interactive hosts.

Time-dependent determinism requires control of both MapLibre's clock and
run-loop timer readiness. The new ABI must prove that a manual clock governs
MapLibre animation time and the libuv timer deadlines that release work. If the
current abstractions cannot provide that control, implementation adds a timer
abstraction or the required upstream seam. The pull request does not merge until
this gate passes.

Production frame requests carry explicit presentation timestamps. Still-image
rendering is a cancellable operation with an explicit frame time, output target,
and deadline. The operation owns all intermediate state, so observer release or
cancellation cannot leave a map stuck in still-image mode.

Each accepted still-image operation produces one result, fails, or is cancelled.
Still-image operations do not coalesce. A host submits them at its own cadence
and may submit the next operation as soon as its sequencing and backpressure
policy permits.

## Events and host notification

### Owned batches and drain ownership

The current C API drains fixed-stride event records and their message arena into
runtime-owned storage. The next drain invalidates that borrowed batch. Runtime
and map subscription masks suppress unselected events before their payloads,
messages, and wake signals are produced. Bindings already step by the reported
event stride, preserve unknown event data, and copy values that outlive a drain.

This plan retains that event representation and filtering policy. A host drains
one or more records into an owned event-batch handle. A later drain cannot
invalidate a batch that the host is still copying.

The C API MUST enforce one active drain lease per queue. A second concurrent
drain returns invalid state. Bindings expose one event stream and fan out copied
values above that point when their host API needs several observers.

Owned batch draining replaces the borrowed `mln_runtime_event_batch` result.
`mln_runtime_poll_event` is already retired. The borrowed batch contract is
retired when the owned handle is added, so two drain contracts never remain in
the supported API.

### Receiver-scoped notification sources

A notification source belongs to one host receiver: one event-loop realm,
isolate, coroutine dispatcher, graphics thread, or native waiter. The host
creates the source before the queues that report to it.

Each drainable queue or service endpoint associates with exactly one
notification source when it is created and keeps that association for its
lifetime. One source may aggregate any number of endpoints that the same
receiver services. A source MUST NOT aggregate endpoints owned by different host
receivers.

An associated endpoint retains the notification source's native state. Source
close returns invalid state while an associated endpoint is live and leaves the
source open. Clearing the host callback is independent of source close, so a
receiver can stop callback delivery before it releases the objects that retain
the source.

The runtime event queue selects its source during runtime creation. A render
session independently selects a frame-availability source and, for a
caller-graphics-thread driver, a driver-work source. Those two session roles may
share one graphics-receiver source. Adapter queues select a source when the
adapter is created. An operation explicitly selects a completion source when it
starts or inherits the default source of the owner that created it.

This lets a Flutter UI isolate drain runtime events while a native Choreographer
receiver services driver work. Compose can drain runtime events in a coroutine
while Skiko's graphics receiver observes frame and driver-work readiness. A
TypeScript process can create one source per realm.

Drain ownership belongs to a queue, not to its notification source. Aggregating
several queues does not combine their records or their drain leases.

Association assigns each drainable queue or service endpoint an immutable ID and
kind within the source. After wakeup, the receiver drains an owned ready batch
containing those IDs, then invokes the corresponding event drain, completion
drain, adapter drain, frame acquisition, or driver-service call. Operation
completion records identify the completed operation; a binding does not poll
every outstanding operation to discover which one woke the source. The ready
batch carries identifiers, not borrowed event payloads.

A notification source is level-triggered. It becomes signaled when any
associated endpoint is ready and remains signaled until the receiver has
serviced every ready endpoint or confirmed that none is ready.

Clearing the signaled state and checking all associated endpoints is one atomic
operation. Data published during that operation leaves the source signaled and
causes another notification if the host has already consumed the preceding one.

Notification sources can cover:

- runtime event batches;
- operation completion;
- resource-request records;
- custom-geometry and other callback-adapter records;
- copied log records when logging uses the adapter;
- render-frame availability;
- typed driver-work readiness.

Adapter record queues keep their typed acquire, response, and release contracts.
They share the notification source rather than maintaining binding-specific wake
callbacks. Retiring pump wake sources therefore does not remove the trigger for
adapter-record drains.

The native notification callback has a deliberately narrow contract:

- it returns void;
- it carries no borrowed payload;
- native code may call it from an arbitrary thread;
- calls may coalesce;
- it only tells the binding to schedule a drain;
- it does not permit re-entry into the C API.

Replacing, clearing, or releasing a notification registration prevents new
callback entries and waits for in-flight entries before releasing callback
state. The source retains its native queue state independently of a host
registration, so an interval without a registered callback cannot lose drainable
data.

This shape works through pure FFI. A binding can connect it to a Dart native
port, an Emscripten worker message, a Kotlin coroutine dispatcher, a Node
thread-safe notification, or an operating-system event source. Those adapters
move a notification into the host event loop. They do not execute MapLibre work.

Immediate C failures continue to use thread-local diagnostics because the caller
reads them before another C call. Operation completion and command failure carry
owned diagnostics because they can finish on a different thread.

### Callbacks and requests

The runtime thread never invokes an arbitrary asynchronously delivered host
closure as part of an ordered command. A host callback may need its event loop
or may attempt a call whose completion depends on the worker invoking it.

Native-capable hosts may retain synchronous C callbacks, including resource
transform callbacks, when their runtime supports an upcall on the runtime
thread. Such a callback has a bounded borrow window, returns promptly, and MUST
NOT call runtime, map, query, lifecycle, or blocking operation APIs. This keeps
the capability for native, JVM FFM, Go, and Python integrations that can satisfy
the contract.

Pure-FFI hosts that cannot run a synchronous upcall use native data and
native-owned adapters:

- resource transforms use a native rule table;
- resource providers return a request handle and complete it asynchronously;
- custom geometry and similar extension points copy requests into owned records
  and accept later responses where MapLibre permits that model;
- logging and observation copy records before notifying the host.

`callback_adapter.h` provides those records and rule tables. Bindings do not
marshal a synchronous host closure onto the runtime worker.

Callbacks that MapLibre invokes from a worker MUST NOT wait for an operation
whose completion depends on that callback. Callback quiescence during close uses
explicit in-flight accounting and never depends on a host event loop running
synchronously inside native teardown.

An adapter request handle belongs to the receiver that drains its record. That
receiver completes or releases the request. A host may send copied request data
to another execution context and return copied response data to the receiver.
Bindings MUST NOT make the request handle transferable.

## Rendering and presentation

The C API assigns no interactive or headless mode to a render session. A render
target and driver define graphics ownership, not pacing. The host chooses when
to submit frame demand or a still-image operation. This choice is independent of
whether the render target is a native surface, an owned texture, or a borrowed
texture.

### Frame cadence and demand

A core-worker render session accepts a nonblocking frame request containing a
presentation timestamp, optional deadline, and optional frame token. The request
captures the session's current extent generation. Repeated requests may coalesce
to the newest demand before rendering begins. A request that has begun retains
its generation. A request becomes eligible as soon as its ordered map and render
dependencies are ready. Its presentation timestamp selects render state, and its
deadline reports urgency; neither supplies a cadence or delays eligible work.

A caller-graphics-thread session receives the same timestamp and token through a
render-driver call from the host's pacing callback or work loop. That call
consumes the latest immutable map update and submits or presents graphics work.
It may block for backend submission or presentation on the graphics thread; the
UI and runtime threads do not wait for it.

The render drivers preserve the current render-result distinctions. A caller
driver reports rendered, no update, size pending, or target not ready directly.
No update and size pending wait for a newer map update. Target not ready waits
for host target readiness or a paced retry; it never enters a busy loop waiting
for a map update.

A core-worker request remains pending while its map update or extent generation
is not ready. A target-not-ready result completes that request without a frame
and reports that disposition to the host, which may submit new demand after the
target becomes ready. A rendered result produces the frame-available event. The
new API may rename these outcomes, but it MUST preserve their distinct wake and
retry conditions.

The host may pace rendering from a display source, submit the next request after
an earlier completion, or use another application scheduler. Cadence does not
need to originate in the binding language. A native platform adapter may connect
Android Choreographer, a display link, or another native frame source to frame
demand. Flutter keeps its measured native Choreographer path. Compose uses the
Skiko redraw callback that already owns its graphics context. Browser targets
use their page or worker animation source.

Driver-work notification is independent of frame cadence. While a caller-thread
target is attached, its owner MUST schedule the driver-service call when typed
work is ready, even when presentation callbacks are paused because a window is
hidden or backgrounded. If that receiver can no longer run, the host abandons
the target rather than leaving renderer-affine operations pending indefinitely.

A frame-available event identifies the request, map generation, extent
generation, frame generation, and whether the rendered state still has pending
changes. A compositor acquisition returns NotReady rather than waiting and may
keep presenting the previous completed frame when the next frame misses its
deadline.

### Texture frame ring

Session-owned texture targets use a ring. The attach options request a depth,
and capability negotiation reports the granted depth. Interactive compositor
integrations normally need two or three slots; a depth of one remains useful for
readback and batch work.

Acquisition returns the newest completed frame that the host has not acquired.
The frame contains copied metadata, a stable backend handle for its slot, its
generation, and producer-completion synchronization.

Releasing a frame supplies consumer-completion synchronization when the host
submitted GPU work that reads the texture. A slot returns to the render driver
only after both of these conditions hold:

1. The host has released the acquired frame.
2. The consumer-completion signal proves that GPU reads have finished.

CPU release alone is insufficient because a compositor can retain the texture in
queued GPU work. The backend contracts map this rule to timeline semaphores or
fences for Vulkan, shared events or command-buffer completion for Metal, and
fence synchronization or documented submission ordering for OpenGL.

Session close rejects a live acquired-frame handle during synchronous preflight.
After every frame handle is released, close may wait asynchronously for the
consumer signals that make its slots safe to destroy. A timeout on a host's wait
detaches that waiter; it does not reclaim a texture that the GPU may still use.
Device loss follows the backend's device-loss destruction contract.

### Extent and scale factor

The render session owns one desired render-target extent containing width,
height, and scale factor. Resize is the only operation that changes it. Resize
is an absolute coalescing command and assigns a new extent generation.

The render session publishes the newest extent immediately and sends the
corresponding logical extent command to the runtime. This makes display-density
changes ordinary resize work rather than requiring a new map. Map scale factor
is no longer fixed for the map's lifetime.

The creation ABI replaces the standalone `mln_map_options.scale_factor` field
with an initial logical extent containing width, height, and scale factor. That
value initializes map state before a render session exists. After creation,
resize is the only command that changes extent or scale factor.

A frame request captures the current extent generation; it does not carry a
second extent value. The render driver records the extent generation used for
each frame.

Texture integrations may scale or crop their last completed frame while the map
publishes an update for the new extent. A caller-driven surface session renders
a compatible stale update at the current session extent when it can do so
without mixing projection and viewport dimensions. It otherwise keeps the last
presented frame until a matching generation arrives.

Continuous resize must not block the platform resize loop and must not suppress
every frame for the duration of the gesture.

### Backend ownership

Execution placement and graphics context use are independent. Execution
placement determines who schedules a render call and which thread executes it:

| Target constraint                           | Driver                 | Examples                                                                                             |
| ------------------------------------------- | ---------------------- | ---------------------------------------------------------------------------------------------------- |
| Worker-owned or transferable device/context | Core worker            | Vulkan device and queues, Metal device, transferred `OffscreenCanvas`, worker-created OpenGL context |
| Thread-current host context or surface      | Caller graphics thread | GLFW OpenGL, Skiko `LinuxOpenGLRedrawer`, thread-affine WebGL2 context                               |

Core-worker descriptors supply everything needed to initialize graphics state on
the worker. OpenGL uses a provider that creates or makes current a worker-owned
context, including any required share group. A target that cannot transfer or
share its graphics state uses the caller-graphics-thread driver; it does not
require a cross-context bridge merely to satisfy the execution model.

The existing OpenGL context-ownership enum controls how a session uses the
thread that attached it. For WGL and EGL, shared ownership makes a session
context current for a call, restores the host context afterward, and joins the
host share group. WebGL uses the host context directly and supports shared
ownership only. Dedicated ownership keeps a session-created context current on a
host thread that the host reserves for that session. Both modes currently use
the caller graphics thread; dedicated ownership does not create a C-owned
worker.

A future core worker may keep its context current exclusively or join a host
share group when the backend permits cross-thread sharing. Those properties do
not make the worker a caller-thread driver. Driver capability therefore remains
separate from the existing OpenGL ownership value. A target descriptor records
both the available execution placement and any context relationship that the
selected placement requires.

This target split is the largest backend integration constraint in the plan.
Every backend must document who owns context creation, which driver calls are
thread-current, how frame cadence enters, and how producer and consumer GPU
synchronization cross the boundary.

A platform-native render-target or cadence adapter may translate lifecycle and
graphics handles that raw FFI cannot express. It remains narrow: it supplies a
target or calls the render driver, and it never owns runtime scheduling.

### Backend context coverage

Core-owned execution does not itself create a Vulkan device, Metal device, or
OpenGL share group. Pure-FFI binding tests still need native support that
creates real backend resources.

The render-driver phase adds a small native test-support shared library that
creates one supported headless context and exports only the resources that the
public Dart binding accepts. The test then attaches through the public binding,
renders, acquires or reads back a frame, and queries rendered features.
Production integrations continue to obtain graphics resources from their host or
a separately reviewed platform support library.

## Binding and integration outcomes

Bindings share the C categories and use the host's normal result forms:

| Host                   | Commands          | Queries and lifecycle                   | Events and completion            | Render driver                                                      |
| ---------------------- | ----------------- | --------------------------------------- | -------------------------------- | ------------------------------------------------------------------ |
| Kotlin/JVM and Compose | Immediate enqueue | Suspending functions                    | Coroutine flow or channel        | Skiko graphics thread or core worker by target                     |
| Kotlin/Wasm            | Immediate enqueue | Suspending functions                    | Coroutine or browser-task wakeup | Transferred canvas worker or thread-affine browser graphics thread |
| TypeScript             | Immediate enqueue | `Promise`                               | Browser task or worker message   | Transferred canvas worker or page graphics thread                  |
| Dart and Flutter       | Immediate enqueue | `Future`                                | Native port                      | Native Choreographer or core worker by target                      |
| Swift                  | Immediate enqueue | `async throws`                          | Async sequence                   | Display-link graphics thread or core worker by target              |
| .NET                   | Immediate enqueue | `Task` or explicit blocking convenience | Async stream or event            | Platform graphics thread or core worker by target                  |
| Rust, Go, Zig, C       | Immediate enqueue | Future or explicit wait                 | Drain after notification or wait | Target-specific caller thread or core worker                       |
| Raster tile server     | Immediate enqueue | Future or explicit wait                 | Native waiter                    | Bounded core-worker sessions or server graphics thread             |

Compose keeps application-level coroutine scopes and state holders. Those
objects collect events and update UI state; they do not own MapLibre runtime
execution. Synchronous UI reads use published snapshots. Rendered-feature
queries and other ordered reads suspend. Compose Desktop may keep rendering on
Skiko's thread-current graphics callback without building a runtime executor.

Kotlin/Wasm and TypeScript use nonblocking commands and asynchronous queries.
The page thread never performs an operation wait. The expected result of the
browser feasibility gate is a page-resident Kotlin/Wasm binding: the native run
loop already lives on a C-owned worker, so `PROXY_TO_PTHREAD` is not part of the
binding architecture. Work on the draft pivots now to the command, operation,
and notification surface, and ports its existing workflow and test coverage.
Only an independently demonstrated toolchain or graphics constraint may retain a
proxied application pthread; in that case it remains a deployment detail and
does not own or pump the runtime.

The TypeScript binding maps every operation to a promise and keeps commands as
synchronous enqueue calls. Its loader, notifier, and tests are revised together;
the synchronous generic call layer is not treated as an API constraint.

Flutter calls runtime and map commands directly from the UI isolate and maps
operations to futures. Its measured native Choreographer path remains the render
driver so that Dart scheduling cannot delay frame cadence. A helper isolate may
still perform expensive Dart-side decoding, but it has no runtime ownership
role.

A raster tile server shards maps across a bounded runtime pool rather than
creating one runtime per request. Maps in a shard share its scheduler thread,
and each active core-worker render session has its serial driver thread. The
server may pool long-lived map and render-session pairs by style, scale factor,
backend, or another workload key. Keeping a pair in the pool preserves its
session-owned caches between requests. MapLibre continues to build and
invalidate render pipelines based on style and scale factor; the execution API
does not collapse those keys. Mutable scale factor does not require a server to
combine scale-specific pools, and a pooled session is never rebound to another
map or render target. Host worker threads become clients of those shards. The
feasibility and latency gates record the thread, memory, and throughput cost so
server capacity planning is explicit.

## Correctness requirements

The implementation and public documentation satisfy these requirements:

- The C API MUST copy command inputs before returning acceptance.
- One runtime MUST establish a total order for its committed commands,
  operations, barriers, and close request.
- A query MUST observe every command ordered before it.
- A coalesced command MUST reach the terminal `superseded` disposition.
- A still-image operation MUST NOT coalesce.
- A close preflight failure MUST leave the handle open.
- Parent close preflight MUST include pending child-creation reservations.
- A handle MUST reject new work after its close request commits.
- Accepted work MUST reach a terminal disposition.
- Releasing an operation MUST NOT abandon internal work or leak an untaken
  result.
- Runtime teardown MUST stop the run loop and join its thread before releasing
  state that the thread can reach.
- Render-session close MUST reject an attached caller-thread target and live
  acquired frames during normal lifecycle. After target abandonment, it MUST
  invalidate frame accessors and quarantine resources whose safe destruction
  cannot be proven. It MUST preserve GPU resources until released consumer work
  retires or the target-loss contract transfers their destruction to the host or
  operating system.
- A runtime worker MUST NOT wait synchronously for a render driver.
- A core render worker MUST NOT wait synchronously for arbitrary runtime work.
- A caller-graphics-thread driver call MUST NOT wait synchronously for runtime
  work.
- The C API MUST wake an idle runtime or core render worker when eligible work
  arrives, without a host poll or display callback.
- The C API MUST NOT infer render pacing from the render-target kind or driver.
- Renderer-affine operations on a caller-thread session MUST enter only the
  typed driver mailbox and MUST complete with target lost if that target is
  abandoned.
- Event records and operation diagnostics MUST remain valid until their owned
  container is released.
- The C API MUST enforce one active drain lease per queue.
- Notification MUST be level-triggered or provide an equivalent no-lost-wakeup
  guarantee across every associated drain domain.
- Every drainable queue or service endpoint MUST retain exactly one
  receiver-scoped notification source for its lifetime.
- After map creation, resize MUST be the sole authority for extent and scale
  factor.
- Every language binding MUST expose one runtime execution model.
- Every interactive example MUST use the command, operation, snapshot, and
  notification model.

Registry locks protect lookup and state transitions only. They are released
before run-loop joins, callback quiescence, operation waits, GPU waits, or host
notification. Lock ordering is documented next to the implementation.

Every exported function is classified in a machine-readable manifest or
equivalent generated metadata. A mechanical check verifies all of the following:

- every exported function has exactly one execution category;
- every public comment documents that category;
- every implementation uses the matching boundary helper;
- every command input has a complete deep-copy implementation;
- every operation result has take, discard, and release coverage;
- every render-driver function documents its target thread requirement;
- every renderer-affine operation identifies either its core-worker execution or
  its typed caller-driver mailbox route;
- every drainable queue and service endpoint declares its receiver and
  notification-source association.

## Delivery phases and merge gate

All phases are required work within one pull request. They do not describe
stacked pull requests or a gradual supported migration. The branch may use
private scaffolding while the implementation is incomplete. The completed pull
request exposes one runtime execution model, carries no compatibility surface
for the prerelease ABI, and merges only after every phase and gate passes.

### Phase 1 — Operations, batches, and notification

Add control-state leases, the operation registry, copied diagnostics, owned
event batches, enforced drain leases, and the unified notification source.
Preserve the current fixed-stride event layout and subscription-mask behavior.

Offline operations already provide operation IDs, deferred completion events,
typed take functions, and explicit result discard. Convert them to the common
operation handle first. Their existing work validates the migration, then adds
cancellation, observer release, completion races, and copied operation
diagnostics.

Move callback-adapter record queues onto the notification source. Replace the
runtime-owned borrowed batch with an owned batch handle and update every
binding's event copy path in this phase. The current batch drain has already
replaced `mln_runtime_poll_event`.

Complete this ownership foundation before moving runtime and map execution. The
final pull request exposes the ownership and execution changes together.

Tests cover:

- completion before and after notification registration;
- poll, timeout, cancellation, observer release, and concurrent completion;
- result take and result discard;
- an uncancellable operation released while pending;
- event batches remaining stable across later drains;
- a second concurrent drain returning invalid state;
- subscription masks suppressing the same event construction and notification
  before and after the owned-batch cutover;
- unknown event and payload data surviving through the reported event stride;
- notification arriving between an empty drain and host-loop suspension;
- adapter records and runtime events sharing one level-triggered source;
- independent runtime and graphics receivers using separate sources without
  competing for one drain lease;
- immutable endpoint-to-source association and source close failing while an
  endpoint remains associated;
- operation completion using a different source from caller-driver work;
- owned ready batches identifying only ready endpoints without borrowed payloads
  or polling every outstanding operation;
- notification replacement and release racing an in-flight callback;
- copied diagnostics surviving calls on other threads.

### Phase 2 — Runtime and map cutover

Move runtime creation, the MapLibre run loop, maps, projections, commands,
queries, barriers, and lifecycle onto the core-owned runtime thread. Classify
every runtime, map, camera, projection, style, and query entry point. Add atomic
camera updates, command dispositions, published snapshots, and close preflight.

Land the cutover in this internal order:

1. Add the classification manifest, generated checks, and boundary helpers.
2. Add the private runtime control state and core-owned run-loop worker.
3. Convert runtime lifecycle, events, and notification association.
4. Convert map creation, map lifecycle, projection, and snapshots.
5. Convert camera commands and camera queries.
6. Convert style, resource, and callback-adapter domains.
7. Convert ordered queries, barriers, and diagnostics.
8. Regenerate the raw bindings and migrate the runtime and map surface in every
   binding.
9. Flip the exported runtime and map ABI and remove the host-pumped surface.

Retire the host-owned runtime surface in this phase:

- `mln_runtime_pump`;
- pump wake sources;
- `mln_thread_token`;
- runtime, map, and projection owner-thread fields and checks;
- wrong-thread status producers for those domains;
- binding owner-thread helpers and thread-pinning requirements.

Update every affected binding and example. Examples use operation waits or
suspension, notification drains, and barriers. The Dart suite gains the
isolate-migration regression and returns to CI.

### Phase 3 — Render drivers and backend contracts

Implement the core-worker and caller-graphics-thread drivers. Add frame demand,
native cadence adapters where justified, update and extent generations, texture
rings, consumer GPU synchronization, frame-close preflight, and asynchronous
render barriers. Add the caller driver's typed work mailbox, receiver
notification, driver-service call, normal detach, and irreversible target
abandonment.

Phase 3 keeps driver coordination in the C/C++ layer. The typed caller-driver
mailbox, frame-demand state, frame queues, driver-work association, completion
state, frame tokens and generations, acquired-frame leases, detach state,
abandonment state, and target-loss completion are native state. Core-worker
drivers are native-owned and native-scheduled. A caller driver is serviced
explicitly by its host on the target's graphics thread; a binding adapts the
typed service call without hiding it behind a binding-owned thread.

Bindings reuse their existing operation primitive and receiver-scoped
notification source. Driver and frame readiness add endpoint kinds to that
source. A binding MUST NOT add another operation registry, mailbox, frame queue,
runtime pump, per-session scheduler, driver thread, or notification mechanism.
It exposes an asynchronous language API only for a C operation, without adding a
paired blocking workflow unless the language's resource-management contract
requires one.

Implement and validate one core-worker backend contract and one caller-thread
backend contract before expanding across every backend and binding. Freeze the
driver structs, ownership rules, lifecycle transitions, and result dispositions
before regenerating bindings. Generate repetitive binding adapters from compact
descriptors where practical; keep language-owned safety and lifecycle policy
handwritten. Correctness and maintainability come first, without a source-line
cap. Contract remaining duplication after Phase 3 before starting Phase 4.

Keep driver selection separate from OpenGL context ownership. Preserve the
rendered, no-update, size-pending, and target-not-ready retry conditions across
caller-driver results and core-worker frame dispositions.

Verify Vulkan, Metal, OpenGL, and browser targets independently. Preserve
thread-current OpenGL, Skiko, and WebGL paths through the caller driver. Verify
a transferred `OffscreenCanvas` through the core-worker driver. Tests pause
presentation while servicing an ordered renderer query, destroy the graphics
receiver with work pending, abandon the target, observe target-lost completion,
and close CPU-side state without touching quarantined graphics resources.

Add the Dart native test-support context library, public binding render
coverage, readback, and rendered-feature query tests.

### Phase 4 — Integration convergence

Implement the Compose, Kotlin/Wasm, TypeScript, Flutter, and OpenHarmony
integrations with commands, operations, notification, and the appropriate render
driver. Port the draft binding test suites to the selected public surface.

Update the binding specification and examples specification. Bindings expose
native async idioms only where the C API exposes an operation. Host-language
concurrency serves notification, result adaptation, and application state rather
than runtime scheduling. The Python binding releases the GIL around explicit
operation waits.

### Phase 5 — Contract audit

Audit the public headers and mechanical classification metadata. Every function
identifies its input borrow window, execution category, ordering boundary,
result ownership, diagnostic channel, and close behavior.

Update `concepts.md`, `c-conventions.md`, the binding specification, and all API
reference comments to describe core-owned runtime execution and target-owned
render drivers. Tests cover every category and backend driver.

### Phase 6 — Feasibility and latency gates

Run every gate against the completed public ABI after the runtime,
render-driver, and binding cutovers. Build the host-pumped baseline from the
pull request's merge-base commit in a clean worktree. Run the baseline and new
builds on controlled hardware with the same backend, target, style, camera-input
trace, and presentation path. Commit the benchmark source, environment, raw
results, and baseline summary as merge evidence.

The runtime latency matrix covers 60 Hz and at least one 90 Hz or 120 Hz display
rate, with input delivered once and twice per display interval. It measures:

- command acceptance;
- command-to-published-state latency;
- command-to-render-update latency;
- command-to-presented-frame latency;
- query and barrier completion;
- behavior while a style parse is in progress.

Normalize results to display intervals and report at least median and 95th
percentile deltas against the host-pumped baseline. The pull request does not
merge if the median command-to-presented-frame regression exceeds half a display
interval or the 95th-percentile regression exceeds one display interval in any
interactive scenario.

If the threshold is crossed, redesign atomic camera commands, publication,
render-update delivery, or frame scheduling and rerun the gate. Measurements
tune the selected runtime model; they do not make UI threads responsible for
runtime execution.

The browser gate uses the public Kotlin/Wasm and TypeScript bindings. It proves
that a page-thread command returns without waiting, an operation completes
through the browser event loop, and notification cannot be lost. Kotlin/Wasm
runs without `PROXY_TO_PTHREAD`. A failure must identify an independent
toolchain or graphics constraint rather than runtime execution.

The clock gate proves control of MapLibre time and libuv timer readiness through
the public ABI. The scale-factor gate proves that one live resize updates map
projection, renderer state, atlases and pixel-ratio-dependent resources, query
results, and published snapshots without reconstructing the map. A failure
requires the missing timer or live pixel-ratio seam before merge.

The backend gate validates both render drivers through the public ABI on one
core-worker target and one thread-current OpenGL or browser target. The
raster-server matrix records cold session creation, warm renders from pools
keyed by style and scale factor, the first render after an idle interval, and
steady throughput under concurrent requests. It measures
submission-to-worker-start and completion latency without installing a display
source, together with the shard's thread and memory cost.

Run the interaction scenarios on every interactive integration. Record
command-to-frame latency, notification delivery, missed frames, texture-ring
occupancy, and resize behavior. Compare each distribution with the committed
baseline and thresholds. This controlled regression run is required merge
evidence. It is not a per-commit timing assertion in routine CI.

## Issue disposition

| Issue | Disposition                                                                                                         |
| ----- | ------------------------------------------------------------------------------------------------------------------- |
| #403  | The runtime and map cutover provides the native runtime dispatcher through the core-owned run loop.                 |
| #412  | The runtime and map cutover removes host native-thread identity from Dart runtime and map validity.                 |
| #418  | The runtime and map cutover restores the Dart test suite to CI with an isolate-migration regression.                |
| #409  | The render-driver phase adds a real backend context through native test support and public Dart rendering coverage. |
| #410  | The render-driver phase exercises rendered-feature queries against that live Dart render session.                   |
| #411  | The runtime and map cutover retires transferable pump wake sources. Notification remains receiver-scoped.           |

## Verification

The C suite covers:

- concurrent command submission with a deterministic committed order;
- barriers and queries observing preceding commands;
- coalesced command disposition and command failure correlation;
- close preflight with pending child creation, live children, an attached
  caller-thread target, and acquired frames;
- observer release for cancellable and uncancellable operations;
- runtime timers and I/O progressing while no host calls the C API;
- runtime and core-render work submitted after an idle interval starting without
  a host poll or display callback;
- callback quiescence without registry-lock or cross-worker deadlock;
- stable event batches, enforced drain ownership, and no lost notification;
- adapter records sharing a receiver source while unrelated receiver queues use
  independent sources;
- caller-thread and core-worker render drivers;
- renderer-affine operations completing through the typed driver mailbox when
  frame callbacks are paused;
- target abandonment completing pending driver work as target lost, invalidating
  frame accessors, and allowing CPU-side close without unsafe GPU destruction;
- target abandonment returning busy without state change while a driver call is
  in flight;
- frame requests coalescing without losing gesture or barrier boundaries;
- texture slots remaining unavailable until consumer GPU completion;
- resize and scale-factor changes producing frames continuously;
- teardown after pending operations and released frames.

Each binding suite covers the same public workflow: create a runtime, create a
map, load a style, observe events, submit camera input, read a snapshot, await
an ordered query, attach a render session, render or acquire a frame, read it
back, and close every handle. The workflow crosses host async suspension points
and never depends on native runtime-thread identity.

Integration tests add platform evidence:

- Compose remains responsive while style work runs and contains no dedicated
  MapLibre runtime thread in Kotlin code.
- Kotlin/Wasm and TypeScript submit page-thread commands and complete operations
  without `Atomics.wait` or `PROXY_TO_PTHREAD` as a runtime requirement.
- Resource-provider tests send copied work through a helper isolate or realm and
  complete the request handle on its receiver.
- Flutter completes runtime work from one UI isolate while native Choreographer
  retains frame cadence.
- Thread-current OpenGL and WebGL targets render without a cross-context bridge.
- A headless program waits for barriers and renders deterministic output without
  pumping a runtime.
- Every interactive example builds without a pump loop.
- A raster-server workload meets the recorded thread, memory, latency, and
  throughput budget for cold, warm-cache, and post-idle requests. Its session
  pools retain their style- and scale-specific render pipelines without
  rebinding a session.

The standard repository verification includes:

```bash
mise run build
mise run test
mise run //bindings/rust:test
mise run //bindings/kotlin:test
mise run //bindings/dart:test
mise run //examples/zig-readback:run
mise run fix
```

## Risks and measurements

Moving camera commands off the UI thread adds a queue handoff. The latency gate
measures that cost on the new public ABI before merge. Atomic camera updates,
continuous runtime execution, and direct render-update publication are the
mechanisms that prevent a full added frame.

Render targets now expose their real execution constraint. The caller driver
avoids forcing GLFW, Skiko, and WebGL integrations through cross-context
sharing, but it keeps a small thread-current surface that every affected binding
must represent safely. This is the largest backend integration cost in the
model.

Receiver-scoped notification adds explicit wiring at queue creation. It avoids
cross-realm drain contention, but every binding must retain each source until
its associated queues close and route frame, driver-work, operation, and runtime
signals to the intended receiver. Phase 1 race tests and generated association
metadata enforce that ownership shape.

A lost caller graphics thread cannot run ordinary target teardown. Abandonment
keeps CPU-side lifecycle recoverable by rejecting further driver work and
quarantining graphics resources instead of issuing unsafe API calls. The host or
operating system remains responsible for destruction through context loss; the
reported quarantine makes that exceptional resource cost visible.

Core-worker rendering requires transferable graphics resources. Each backend
must prove that its descriptor can create or own the necessary context on the
worker. Native target and cadence adapters remain small and platform-specific.

Texture ring depth and synchronization affect memory and compositor latency.
Backend measurements determine defaults. The safety rule that prevents reuse
before consumer completion does not depend on those measurements.

Operation handles add allocations to queries and lifecycle work. Pooling and
small-object optimization are implementation choices after profiles show their
value. Commands and snapshot reads remain free of operation allocation.

The runtime thread consumes resources in headless programs. Multiple maps share
one runtime when a process needs to amortize that cost. A raster server also
pays one driver thread per active core-worker session, so the shard benchmark
establishes usable concurrency and memory limits before merge. The server
chooses its session-pool keys and render cadence; mutable scale factor and frame
demand do not impose either policy.

Browser binding migration is substantial. Kotlin/Wasm's pthread-proxied draft
and TypeScript's synchronous call layer provide test coverage and implementation
evidence, but the new page-thread command and operation façade must pass the
browser gate before merge.
