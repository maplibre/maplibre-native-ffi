"""How a callback registration reaches a host that runs no code on native threads.

Dart isolates and Node-API realms run host code only on their own thread, after
native code returns. Such a host gives each registration one of five forms:

- `queue`: the registration names a delivery queue, and every callback has a
  queue delivery. The host sets its queue and a token in place of the
  callbacks; native code answers each call itself and queues a copy, which the
  host drains on its own thread and dispatches by the token.
- `wake`: the registration's one callback only asks to be called again, and the
  registration carries no queue. The host supplies a wake that posts to its
  thread.
- `rules`: every callback must answer on its native thread and has no queue
  delivery. The host leaves them unset, so the native rules that the
  registration carries answer each call.
- `native`: the callbacks are optional and synchronous, so native code relies
  on their work being done when they return. The host accepts only a native
  registration, whose function pointers and context native code supplies, and
  a record with a native default keeps such a field at its disabled default.

A wake also accepts a native registration, which skips the post to the host's
thread. Python, Kotlin, .NET, and Go accept one for the same registrations:
each of their callbacks enters the language runtime on the native thread, by
taking the interpreter lock or by a garbage-collector transition, which a
native function never needs.

Both the Dart and the TypeScript emitters choose a registration's form here.
"""

from __future__ import annotations

from collections.abc import Iterable

from .semantic import BoundApi, CallbackPlan, FieldPlan, OperationPlan, ValuePlan


def callback_fields(value: ValuePlan) -> list[FieldPlan]:
    """The callback fields of a registration descriptor."""
    if not value.registration:
        return []
    return [f for f in value.fields if f.name in value.registration.callbacks]


def wake_callback(callback: CallbackPlan) -> bool:
    """Whether a callback only asks to be called again: it takes nothing but
    its context, returns nothing, and native code neither waits for it nor
    delivers it through a queue."""
    return (
        bool(callback.context)
        and not callback.parameters[1:]
        and callback.result.native == "void"
        and not callback.synchronous
        and not callback.delivery
    )


def is_wake(bound: BoundApi, value: ValuePlan) -> bool:
    """Whether a registration is a wake: its one callback is a wake callback,
    and it carries no queue."""
    if not value.registration or value.registration.queue:
        return False
    callbacks = callback_fields(value)
    return len(callbacks) == 1 and wake_callback(
        bound.callbacks[callbacks[0].value.native]
    )


def queue_callbacks(bound: BoundApi, value: ValuePlan) -> list[FieldPlan]:
    """The callbacks of a registration that a delivery queue carries, or none.

    The host sets the registration's queue in place of every callback, so
    every callback must have a delivery.
    """
    if not value.registration or not value.registration.queue:
        return []
    fields = callback_fields(value)
    if not all(bound.callbacks[f.value.native].delivery for f in fields):
        return []
    return fields


def omitted(bound: BoundApi, field: FieldPlan) -> bool:
    """Whether the host leaves a callback field unset: an optional callback
    that must answer on its native thread and has no queue delivery, such as
    the HTTP header transform's, whose registration then applies native
    rules."""
    if field.value.kind != "callback" or not field.value.nullable:
        return False
    callback = bound.callbacks[field.value.native]
    return (
        callback.delivery is None
        and callback.result.native != "void"
        and not callback.synchronous
    )


def rules_only(bound: BoundApi, value: ValuePlan) -> bool:
    """Whether the host writes a registration without any callback, so native
    rules answer in place of the host."""
    fields = callback_fields(value)
    return bool(fields) and all(omitted(bound, f) for f in fields)


def native_only(bound: BoundApi, value: ValuePlan) -> bool:
    """Whether a registration accepts only a native registration.

    The host cannot run a synchronous callback that native code calls on its
    own thread, so only native code can supply one. A record that holds such a
    registration and has a native default keeps the field disabled until the
    host supplies one.
    """
    callbacks = [f.value for f in callback_fields(value)]
    return (
        bool(callbacks)
        and all(callback.nullable for callback in callbacks)
        and any(bound.callbacks[callback.native].synchronous for callback in callbacks)
    )


def form(bound: BoundApi, value: ValuePlan) -> str | None:
    """The form of a registration for an async-only host, or None when it has
    none."""
    if not value.registration:
        return None
    if is_wake(bound, value):
        return "wake"
    if queue_callbacks(bound, value):
        return "queue"
    if rules_only(bound, value):
        return "rules"
    if native_only(bound, value):
        return "native"
    return None


def accepts_native(bound: BoundApi, value: ValuePlan) -> bool:
    """Whether a registration accepts a native registration: a wake, or one
    that only native code can supply."""
    return bool(value.registration) and (
        is_wake(bound, value) or native_only(bound, value)
    )


def rule_answered_operations(
    bound: BoundApi, registrations: Iterable[ValuePlan]
) -> set[str]:
    """The operations that only an omitted callback calls, through the response
    records it writes. Native rules answer in its place, so they have no caller
    on an async-only host."""
    responses = set()
    for value in registrations:
        for field in value.fields:
            if not value.registration or not omitted(bound, field):
                continue
            callback = bound.callbacks[field.value.native]
            responses |= {
                parameter.value.element.native
                for parameter in callback.parameters
                if parameter.direction == "out" and parameter.value.element
            }
    return {plan.name for plan in bound.operations if scoped_to(plan) in responses}


def scoped_to(plan: OperationPlan) -> str | None:
    """The response record that a scoped operation answers through, or None."""
    if not plan.scoped_receiver:
        return None
    return next(
        (
            p.value.element.native
            for p in (*plan.inputs, *plan.outputs)
            if p.name == plan.scoped_receiver and p.value.element
        ),
        None,
    )
