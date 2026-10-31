---
title: Guidelines
description: Rules to follow when you change the public API or documentation.
sidebar:
  order: 2
---

<!--
Scope: rules for changing the public API and documentation. Each section
introduces its concept, shows an example, then states rules for how things
should be, not how they are. Delete a rule once a tool enforces it.
-->

Follow these guidelines when you change the public API or documentation.

If a rule doesn't fit a case, change the rule in the same PR instead of making
an exception. When a tool enforces a rule, delete it here.

## Versioning

The native library and every binding share one version number,
`EPOCH.YYYYMM.REVISION`. `YYYYMM` is the UTC year and month of the release.
`REVISION` numbers the releases in that month, starting at 0.

The epoch says what stays compatible. In epoch 0, nothing does: make breaking
changes when they improve the API, and don't add compatibility shims. In epoch 1
and later, a release doesn't break the API, ABI, or documented behavior of
earlier releases with the same epoch, in the library or in any binding. A
breaking change increments the epoch. `MLN_C_VERSION` and `mln_c_version()`
equal the epoch.

## API

The public API is the C headers in `include/`. A generator in `tools/bindgen`
reads them and writes each language binding's declarations.

Besides the C types, the generator reads `MLN_BINDING` annotations. They say who
owns each value, how long it lives, whether it can be null, and how each
function runs. The generator uses them to write safe wrappers in each binding.

### Naming

- Name a function `mln_<owner>_<verb>[_<noun>]`, where the owner is the type of
  its first parameter: `mln_map_get_style_layer` takes an `mln_map`. A function
  without a handle parameter starts with its subsystem instead:
  `mln_log_set_callback`.
- Use `get_` for a function that reads one value, `list_` for one that reads all
  of them, and `is_` for a predicate.
- Name a bitmask enum `mln_<x>_flag`, and the type that holds a set of its bits
  `mln_<x>_flags`.
- Prefer the MapLibre style spec's terms over engine terms.
- When a plain number's type doesn't show its unit, end its name with the unit:
  `duration_ms`, `expires_unix_ms`.

### General API design

- A function that returns an error status must not change any state.
- An asynchronous function takes an `mln_completion`, which receives its result.
- Annotate each function with how it runs, as in
  `MLN_BINDING("execution=command")`:

  | Execution     | Function                                                  |
  | ------------- | --------------------------------------------------------- |
  | `command`     | Changes state asynchronously                              |
  | `query`       | Reads state asynchronously                                |
  | `operation`   | Does other asynchronous work                              |
  | `lifecycle`   | Creates, attaches, or retires a handle asynchronously     |
  | `immediate`   | Returns its result synchronously; the default             |
  | `snapshot`    | Copies the current state behind a handle synchronously    |
  | `event_batch` | Takes the queued frame results, or reports there are none |

- Fix surprising behavior instead of documenting it. When a contract is hard to
  state, change the API instead of writing more prose.

### Handles

A handle is an opaque 64-bit ID for state that native owns and keeps between
calls, such as a runtime, a map, a parsed style expression, or a delivery queue.
Callers can copy and compare handles and pass them between threads, but can't
see the state behind a handle, and must not create or decode one.
`MLN_HANDLE_NULL` is the null handle. Native never reuses a released handle.

```c
typedef uint64_t mln_style_expression MLN_BINDING(
  "kind=handle;release=mln_style_expression_release"
);
typedef uint64_t mln_map MLN_BINDING(
  "kind=handle;release=mln_map_release;dispose=mln_map_dispose"
);

// A create function writes the handle to an output that must hold the null
// handle on entry.
mln_style_expression expression = MLN_HANDLE_NULL;
mln_style_expression_create(json, NULL, &expression, NULL);
mln_style_expression_release(expression);
```

Most handles are freed right away, so they have one teardown function,
`void _release(handle)`. Some handles can't be freed until other work finishes,
such as a runtime that has to wait for its maps to close. These have two:

- `_release(handle, completion)` is for an orderly shutdown. It rejects a handle
  that isn't ready, such as a runtime that still has maps, and the completion
  runs once teardown finishes.
- `_dispose(handle)` is for destructors and finalizers, which can't wait or
  handle errors. It accepts the handle in any state and finishes teardown in the
  background.

### Types that may grow

In epoch 1 and later, a release can add to the API but can't break existing
declarations. Code built against older headers must keep working with a newer
library, so each type is declared in a way that lets later releases extend it.

```c
typedef enum mln_status : int32_t {
  MLN_STATUS_OK = 0,
  MLN_STATUS_INVALID_ARGUMENT = -1,
  MLN_STATUS_INVALID_STATE = -2,
  // A later release adds -3, -4, and so on.
} mln_status;

typedef struct mln_diagnostic {
  uint32_t size; // sizeof(mln_diagnostic) in the caller's headers
  char message[MLN_DIAGNOSTIC_MESSAGE_CAPACITY];
  // A later release adds members here.
} mln_diagnostic;

typedef struct mln_lat_lng { // No size: this type never changes.
  double latitude;
  double longitude;
} mln_lat_lng;
```

- Use fixed-width integers, `size_t`, `float`, `double`, and `bool`.
- An enum declares a fixed underlying type, usually `uint32_t`. A later release
  can add values, so code that reads an enum from native must accept values its
  headers don't name.
- A struct passed by pointer begins with `uint32_t size`, and a later release
  adds members only at its end. The side that allocates the struct sets `size`
  to its `sizeof`, and the other side reads or writes only the members within
  that size.
- A small value type that is not intended to grow, such as a coordinate or an
  ID, does not need `size`.

### Option bags

An options struct holds a function's settings. Each optional setting has a bit
in the struct's `fields` member. Native reads a setting only when its bit is
set, and uses the setting's default otherwise, so a struct with only `size` set
selects every default.

```c
typedef enum mln_style_expression_option_field : uint32_t {
  MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE = 1U << 0U,
} mln_style_expression_option_field;

typedef struct mln_style_expression_options {
  uint32_t size;
  mln_style_expression_option_fields fields;
  mln_style_expression_type expected_type
    MLN_BINDING("mask=fields;bit=MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE");
} mln_style_expression_options;

mln_style_expression_options options = {.size = sizeof(options)};
options.fields = MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE;
options.expected_type.kind = MLN_STYLE_EXPRESSION_TYPE_NUMBER;
```

- The bits are a bitmask enum named `mln_<x>_field`, and `fields` has the type
  `mln_<x>_fields`.
- A member with no sensible default, such as a width or a Vulkan device, is
  required instead. It has no bit and carries `MLN_BINDING("required=true")`.

### Data in and out

Neither side frees memory that the other allocated. Data passed into a function
is borrowed for the call. Data that native returns is either written into
storage that the caller provides, or lent by native until a stated point, such
as when a callback returns.

```c
// Borrowed for the call: the caller can free json once it returns.
mln_style_expression_create(json, NULL, &expression, NULL);

// Written into the caller's buffer. The first call reports the length.
size_t length = 0;
mln_style_expression_evaluate(expression, NULL, NULL, 0, &length, NULL);
char* result = malloc(length + 1);
mln_style_expression_evaluate(
  expression, NULL, result, length + 1, &length, NULL
);

// Lent until the callback returns, so the caller copies what it keeps.
void on_complete(void* user_data, const mln_completion_result* result) {
  keep_copy(result->diagnostic);
}
```

- Pass text as UTF-8 in an `mln_buffer_view`, not as a NUL-terminated string.
- A function that keeps an input copies it before it returns.
- A synchronous function that returns data of a length the caller can't know
  writes it into a buffer the caller passes, and always reports the full length,
  so the caller can call again with enough room.
- Lent data stays valid until a point that the declaration states, such as when
  a callback returns or a handle is released.

### Callbacks

Native calls caller code through callbacks, to deliver a result, report an
event, or ask the caller for data. A callback comes in a registration struct,
which pairs it with `user_data`, the caller's context pointer, and with a
function that frees that context. Native runs the callback on a thread that it
picks, unless the registration names an `mln_delivery_queue`. Then native queues
each call, and the callback runs when the caller dispatches the queue, on a
thread the caller chooses.

```c
typedef struct mln_log_handler {
  uint32_t size;
  mln_log_callback callback;
  void* user_data;
  mln_user_data_release release_user_data;
  mln_delivery_queue queue;
} mln_log_handler;

mln_status mln_log_set_callback(
  const mln_log_handler* handler, mln_diagnostic* out_diagnostic
);
```

- Native calls `release_user_data` once, after the last call to the callback.
- Callbacks should accept a delivery queue unless native needs the callback's
  result before it can continue.
- Some language bindings may not expose callbacks without a delivery queue, or
  may expose that functionality with an alternative declarative api.

### Platform-specific APIs

A function that only some platforms can use lists those platforms in its
annotation.

```c
MLN_BINDING("execution=lifecycle;platforms=ios,macos,tvos,visionos")
MLN_API mln_status mln_map_attach_metal_surface(/* ... */);
```

## Documentation

Use plain English, without idioms, Latin abbreviations, or jargon. Assume the
reader is lightly familiar with MapLibre, but not with its internals.

Docs site pages introduce concepts and show common tasks, not every feature or
detailed contracts. Avoid bloating these; not every feature needs a section and
example on the docs site. Avoid narrating implementation details.

An API reference exists for each language binding; the C reference is the
source, and the rest are generated from there. The API reference describes
observable functionality and contracts; it must not narrate implementation.

Inline comments in the code explain constraints that the code can't show, such
as from a dependency, platform, vendor, or protocol that we can't change. If the
surprising behavior is rooted in our own code's design, rethink the code
instead.

### API reference comments

A comment has a one-sentence summary, then paragraphs about behavior, then these
sections in order. Leave out a section that would be empty:

- `Threads:` lists the threads a callback runs on.
- `Returns:` lists every status the function returns, each with a condition the
  caller can check or act on.
- `Completes with:` does the same for the completion.
- `Lifetimes:` says how long each pointer stays valid and who ends it. Bindings
  manage lifetimes themselves, so only C callers need it.

The generator copies every part except `Lifetimes:` into each binding, and
replaces each C name with that binding's symbol and each status with its error.
The summary says what the function does for the caller, using names that every
binding has. The execution annotation already says that a function is
asynchronous, so a summary doesn't start with "Submits a command".

```c
/**
 * Removes a source from the map's style.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map isn't a valid handle, source_id is
 *   empty, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 *
 * Completes with:
 * - MLN_STATUS_OK when the source is removed.
 * - MLN_STATUS_NOT_FOUND when the style has no source with that ID.
 * - MLN_STATUS_INVALID_STATE when a layer still uses the source.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_remove_style_source(
  mln_map map, mln_buffer_view source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Receives the result of an asynchronous function.
 *
 * Threads:
 * Runs on a thread that native picks, which can be the calling thread. When the
 * completion names a queue, runs in mln_delivery_queue_dispatch() instead.
 *
 * Lifetimes:
 * result, and everything it points to, stay valid until the callback returns.
 */
typedef void (*mln_completion_callback)(
  void* user_data, const mln_completion_result* result
);
```
