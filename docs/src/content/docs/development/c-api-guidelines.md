---
title: C API guidelines
description: Rules for declaring the public C API that every binding wraps.
sidebar:
  order: 2
---

`tools/bindgen` reads the public C headers that
`include/binding-interfaces.toml` lists and generates every binding's
declarations from them. After you change a header, run
`mise run bindings:generate`.

## Design a declaration

Pick the execution category that describes the native operation:

| Execution     | Operation                                                        |
| ------------- | ---------------------------------------------------------------- |
| `command`     | Mutates state; completes with a disposition and a generation     |
| `query`       | Reads state; completes with a value                              |
| `operation`   | Runs other asynchronous native work; completes with its result   |
| `lifecycle`   | Creates, attaches, or retires a handle                           |
| `immediate`   | Returns its result synchronously; the default without completion |
| `snapshot`    | Copies current state synchronously from a live handle            |
| `event_batch` | Drains frame results into an owned batch, or reports none queued |

Name a function `mln_<owner>_<verb>[_<object>]`, where the owner is the handle
in its first parameter, as in `mln_map_get_style_layer`. A read uses `get_`, a
read of every object uses `list_`, and a predicate uses `is_`. A function
without a handle names its subsystem or record, as in `mln_log_set_callback`. A
handle's teardown is `_release`.

Follow these rules, which keep the ABI open to growth:

- Use fixed-width integers, `size_t`, `float`, `double`, and `bool`. An enum
  declares a fixed underlying type, `uint32_t` unless its values need another.
- A struct passed by pointer begins with `uint32_t size`, and new members go at
  its end. Small value types, such as coordinates and IDs, carry no size and
  never change; a change adds a new type.
- An input struct whose defaults are not all zero has `mln_<struct>_default()`.
- An optional member has its own bit in the struct's `fields` mask.
- Strings that the host passes in use `mln_buffer_view`, and strings that native
  returns use NUL-terminated `const char*`.
- A synchronous function that returns text of a length the caller cannot know
  writes it into a buffer the caller passes, reports its length whether or not
  it fit, and returns the same text for the same inputs, so a binding can call
  again with enough storage. Annotate it as `mln_style_expression_evaluate()`
  does.
- A handle output parameter must hold `MLN_HANDLE_NULL` on entry. Every other
  pointer is borrowed for the call, and a function that keeps an input copies
  it.

`MLN_BINDING` annotations state only what the C type cannot, such as which
parameter counts an array. Copy them from a similar declaration; the generator
rejects an annotation that restates a default.

## Document a declaration

The header comment is the caller's contract. List every status that a function
returns under `Returns:`, the outcomes of an asynchronous function under
`Completes with:`, and the threads that a callback runs on.

Every binding copies the comment's first paragraph, so write one that stands
alone as a summary, and keep the comment directly above its declaration.
