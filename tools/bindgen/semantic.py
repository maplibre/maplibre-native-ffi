"""Resolve C declarations into shared, statically compiled binding plans.

Plans contain semantic decisions, never target-language source. Backends render
these plans and keep allocation, callback roots, and scheduling in their runtime.
A plan names each public member once, and an emitter only converts its case and
escapes keywords. Resolve a new relationship here, once, so that every emitter
consumes it; emitters report the shapes that they cannot lower and never invent
ownership.
"""

from __future__ import annotations

import functools
from dataclasses import dataclass, field, replace
from typing import TypedDict

from . import docs
from .model import Api, CType, Function, ModelError
from .names import type_name
from .protocol import (
    ABI_VERSION,
    BUFFER_VIEW,
    COMPLETION_RESULT,
    COMPLETION_VALUE_COUNT,
    COMPLETION_VALUE_SIZE,
    DELIVERY_BATCH,
    DELIVERY_PAYLOAD,
    DELIVERY_QUEUE,
    RUNTIME_EVENT_SOURCE,
    RUNTIME_EVENT_SOURCE_PREFIX,
    STATUS,
    is_completion,
    is_status,
)
from .schema import record_reaches, validate


@dataclass(frozen=True)
class Presence:
    # A field behind a presence mask names the mask and its bit, which the
    # schema requires together; a union member names its tag and variant.
    mask: str | None = None
    bit: str | None = None
    tag: str | None = None
    variant: str | None = None


@dataclass(frozen=True)
class HandlePlan:
    native: str
    release: str
    parent: str | None
    dispose: str | None = None
    release_consumes: str = "success"
    abandon: str | None = None
    release_inputs: tuple[str, ...] = ()
    finalize: tuple[str, ...] = ()
    view_begin: str | None = None
    view_end: str | None = None
    # The cleanup of a handle without a dispose that is safe only for an owner
    # its host dropped, such as a frame whose consumer sync is unknown. A
    # finalizer or destructor runs it, but no scoped cleanup does.
    discard: str | None = None
    # Whether the handle is a root whose release is a lifecycle operation that
    # reports retirement through its completion, so a host can wait for it.
    observable_root_release: bool = False
    # The completion parameter of a dispose that is also the handle's release.
    # The release passes a completion that reports retirement, and the dispose,
    # like every disposer, passes none.
    dispose_completion: str | None = None

    @property
    def stem(self) -> str:
        """The handle's public name: its operation prefix without `mln_`."""
        return public_stem(self.native)

    @property
    def cleanup(self) -> str | None:
        """The operation that cleans up an owner in any state: its dispose,
        or its discard."""
        return self.dispose or self.discard

    @property
    def scoped_release(self) -> str | None:
        """The operation that scoped cleanup, such as `use` or `with`, calls.

        A handle with an abandon operation releases only after detach or
        abandon, so its scope disposes it instead, which accepts any state. A
        handle with a discard has no scoped cleanup: its release needs what
        only the host knows, and its discard is safe only for a dropped owner.
        """
        if self.discard:
            return None
        return self.dispose if self.abandon and self.dispose else self.release


@dataclass(frozen=True)
class OwnedOutputPlan:
    parameter: str
    handle: HandlePlan
    parent_parameter: str | None


@dataclass(frozen=True)
class CompletionPlan:
    parameter: str
    immediate_owners: tuple[OwnedOutputPlan, ...]
    discardable: bool = False
    # Whether the completion may be null without being a command's discard: the
    # function is also its handle's dispose, which passes none.
    optional: bool = False


@dataclass(frozen=True)
class MaskFlag:
    mask: str
    name: str
    value: int
    # The flag constant without its enum's shared prefix.
    member: str = ""


@dataclass(frozen=True)
class RegistrationDescriptorPlan:
    callbacks: tuple[str, ...]
    user_data: str
    release: str
    # The member that names a delivery queue in place of the callback, or None.
    queue: str | None = None


@dataclass(frozen=True)
class CallbackResponsePlan:
    native: str
    methods: tuple[str, ...]


@dataclass(frozen=True)
class ValuePlan:
    kind: str
    native: str
    ctype: CType
    ownership: str = "value"
    encoding: str | None = None
    lifetime: str = "call"
    nullable: bool = False
    length: str | None = None
    stride: str | None = None
    fields: tuple[FieldPlan, ...] = ()
    element: ValuePlan | None = None
    handle: HandlePlan | None = None
    enum_kind: str | None = None
    enum_underlying: CType | None = None
    scalar_carrier: str | None = None
    enum_values: tuple[tuple[str, int], ...] = ()
    tag: str | None = None
    empty_variant: tuple[str, int] | None = None
    mask_flags: tuple[MaskFlag, ...] = ()
    registration: RegistrationDescriptorPlan | None = None
    response: CallbackResponsePlan | None = None
    # For kind="buffer": "view" for the mln_buffer_view struct, "pointer" for a
    # character or byte pointer with a separate length or NUL terminator.
    buffer_form: str | None = None

    @property
    def unwrapped(self) -> ValuePlan:
        return self.element if self.kind == "reference" else self

    @property
    def union_member(self) -> FieldPlan | None:
        """The union of a record that holds only that union and its tag.

        A binding presents such a record as the union itself, under the record's
        name, and encodes the tag and the size from the selected variant.
        """
        if self.kind != "record" or any(
            field.role not in {"value", "tag", "size", "reserved"}
            for field in self.fields
        ):
            return None
        values = [field for field in self.fields if field.role == "value"]
        if len(values) == 1 and values[0].value.kind == "union":
            return values[0]
        return None


def plain_value(value: ValuePlan) -> bool:
    """Whether a value is fixed-size data that zeroed storage represents."""
    if value.kind in {"scalar", "enum"}:
        return True
    return value.kind == "record" and all(
        plain_value(field.value) for field in value.fields
    )


# Field roles that hold control state: a struct's size, reserved space, an
# array's count or stride, a presence mask, a union tag, a
# callback context, a registration's release callback, and a registration's
# delivery queue, which a binding runtime sets in place of the callback.
CONTROL_ROLES = frozenset(
    {
        "size",
        "reserved",
        "count",
        "stride",
        "presence_mask",
        "tag",
        "context",
        "release",
        "delivery_queue",
    }
)


@dataclass(frozen=True)
class FieldPlan:
    name: str
    value: ValuePlan
    presence: Presence | None = None
    # The field's `kind`, or "value". A role in CONTROL_ROLES marks state that
    # a binding writes or derives rather than a member a host sets or reads.
    role: str = "value"
    # Native rejects this member at zero, because no default suits every host.
    required: bool = False

    @property
    def public(self) -> bool:
        return self.role not in CONTROL_ROLES

    @property
    def scoped(self) -> bool:
        """The field names a native object that its record's owner lends.

        In a borrowed view's output, it stays valid only while the view's scope
        is active, so a binding guards it, and no other field, with that scope.
        """
        return self.value.lifetime == "owner"


def required_member(field: FieldPlan) -> bool:
    """Whether a public member has no zero value that a binding may default it
    to, so that a host must always supply it.

    That is a member C marks required, a callback C does not mark nullable, a
    union with no empty variant, and a record or registration that holds such a
    member. A masked member is absent at zero, so it is never required.
    """
    if field.required:
        return True
    if field.presence and field.presence.mask:
        return False
    value = field.value
    if value.kind == "callback":
        return not value.nullable
    if value.kind == "union":
        return value.empty_variant is None
    if value.kind == "record":
        return any(required_member(f) for f in value.fields if f.public)
    return False


def required_callbacks(value: ValuePlan) -> tuple[FieldPlan, ...]:
    """The callbacks that a registration requires of its host.

    A callback that C does not mark nullable is required whenever the host
    leaves the delivery queue unset, which every binding's public shape does.
    """
    if not value.registration:
        return ()
    return tuple(
        f
        for f in value.fields
        if f.name in value.registration.callbacks and not f.value.nullable
    )


@dataclass(frozen=True)
class ParameterPlan:
    name: str
    value: ValuePlan
    direction: str
    # The input array whose length sizes this caller-allocated output array.
    sized_by: str | None = None
    # For caller-sized text, the output that reports the text's length; its
    # capacity input is the value's length. A binding allocates the storage,
    # passes its capacity, and calls again with enough when the reported
    # length does not fit. Neither control parameter reaches host code.
    result_length: str | None = None


@dataclass(frozen=True)
class DeliveryPlan:
    """How a queue carries a copy of one callback's arguments.

    `constant` is the mln_delivery_type value, `payload_member` the
    mln_delivery_payload member that holds the copy, and `record` the record
    whose fields mirror the callback's arguments other than its context.
    """

    constant: str
    payload_member: str
    record: str


@dataclass(frozen=True)
class CallbackPlan:
    native: str
    parameters: tuple[ParameterPlan, ...]
    result: ValuePlan
    failure: str | None = None
    context: str | None = None
    borrowed_owner: str | None = None
    status: bool = False
    # How a delivery queue carries the call, for a registration in queue mode.
    delivery: DeliveryPlan | None = None
    # The result that native code answers in queue mode, from the
    # `queue_result=` of a callback that returns a value.
    queue_result: str | None = None
    # Whether native code relies on the callback's work being done when it
    # returns, so a binding must run it on the calling thread rather than
    # deliver it later, even when it returns nothing.
    synchronous: bool = False

    @property
    def arguments(self) -> tuple[ParameterPlan, ...]:
        """The parameters that host code receives: all but the context and a
        borrowed owner."""
        return tuple(
            p
            for p in self.parameters
            if p.name not in {self.context, self.borrowed_owner}
        )


@dataclass(frozen=True)
class RegistrationPlan:
    parameter: str
    descriptor: str
    callbacks: tuple[str, ...]
    user_data: str
    release: str
    path: tuple[str, ...] = ()
    # The boolean output that, set on success, reports that native kept
    # nothing, so the caller still owns the registration; or None when
    # success always transfers it.
    accepted_unless: str | None = None
    # The descriptor member that names a delivery queue, or None.
    queue: str | None = None


@dataclass(frozen=True)
class BorrowedViewPlan:
    owner_parameter: str
    owner: HandlePlan
    # The one output that the view borrows, and the owner's scope operations.
    output: ParameterPlan
    begin: str
    end: str
    # The view operation's member without a leading `get_`, read as
    # `with_<stem>`.
    stem: str


@dataclass(frozen=True)
class DisposeSupport:
    """The operation disposes this handle when its owner is abandoned."""

    handle: HandlePlan


@dataclass(frozen=True)
class ViewSupport:
    """The operation begins or ends a borrowed view scope of this handle.

    Bindings call it only from the borrowed views that the handle owns, so it
    never becomes a public member.
    """

    handle: HandlePlan


@dataclass(frozen=True)
class DeliverySupport:
    """The operation belongs to the queue delivery protocol.

    A binding runtime creates, drains, and releases delivery queues itself, so
    the operation is a runtime export with no public member.
    """

    handle: str


@dataclass(frozen=True)
class DrainPlan:
    """A drain that publishes an owned batch and a borrowed slice view of it.

    A binding copies the slice, releases the batch, and returns the slice, which
    is empty when the drain reports nothing queued.
    """

    batch: ParameterPlan
    view: ParameterPlan
    slice: FieldPlan


@dataclass(frozen=True)
class AbsencePlan:
    """A failure status that reports the operation's one output as absent.

    A binding returns its language's empty form for this status instead of an
    error, and adopts or copies the output only on success.
    """

    status: str
    value: int
    output: ParameterPlan


@dataclass(frozen=True)
class OperationPlan:
    function: Function
    execution: str
    receiver: str | None
    inputs: tuple[ParameterPlan, ...]
    outputs: tuple[ParameterPlan, ...]
    result: ValuePlan | None
    registrations: tuple[RegistrationPlan, ...] = ()
    consumes: str | None = None
    # Whether the call is its receiver's release or cleanup. Another call that
    # consumes its receiver, such as an answer, keeps its own name, and a
    # second call on the consumed receiver fails as on any released handle.
    release: bool = False
    # The handle disposal, borrowed view scope, or delivery protocol that the
    # operation backs.
    support: DisposeSupport | ViewSupport | DeliverySupport | None = None
    completion: CompletionPlan | None = None
    owned_outputs: tuple[OwnedOutputPlan, ...] = ()
    view: BorrowedViewPlan | None = None
    scoped_receiver: str | None = None
    # Whether the function returns the status enum.
    status: bool = False
    # The public name; see Binder.member.
    member: str = ""
    # A drain's batch and slice view, which bindings present as the slice.
    drain: DrainPlan | None = None
    # The `absent_on=` status, its value, and the output that it reports absent.
    absence: AbsencePlan | None = None
    # What an immediate call waits for before it returns, such as a native
    # thread start, which some host threads cannot wait for.
    blocks: str | None = None
    # The root handle that this receiverless call creates, which bindings
    # present as a factory on that handle's type, or None.
    factory: str | None = None
    # Whether the call skips the C ABI version check: the version query that
    # the check itself calls.
    unchecked: bool = False

    @property
    def name(self) -> str:
        return self.function.name

    @property
    def role(self) -> str:
        return "support" if self.support else "public"

    @property
    def caller_sized_text(self) -> ParameterPlan | None:
        """The JSON text output that a binding sizes and calls again to fit.

        It is the operation's only output besides its reported length.
        """
        return next((p for p in self.outputs if p.result_length), None)


@dataclass(frozen=True)
class BoundApi:
    source: Api
    operations: tuple[OperationPlan, ...]
    values: dict[str, ValuePlan]
    callbacks: dict[str, CallbackPlan]
    handles: dict[str, HandlePlan]
    diagnostics: tuple[str, ...] = ()
    unsupported: dict[str, tuple[str, ...]] = field(default_factory=dict)
    runtime_operations: tuple[OperationPlan, ...] = ()
    # The records and unions that a binding copies from native: those that an
    # operation's output or result or a callback's argument reaches.
    returned: frozenset[str] = frozenset()
    # The returned records and unions that only a callback's arguments reach,
    # which native lends for the callback's duration.
    lent: frozenset[str] = frozenset()

    @functools.cached_property
    def inputs(self) -> set[str]:
        return record_reaches(self.source)[1]

    @functools.cached_property
    def slices(self) -> dict[str, FieldPlan]:
        """The array field of each `kind=slice` record. A binding presents the
        array in place of the record, as a drain's result or a callback's
        argument."""
        return {
            record.name: next(
                field
                for field in self.values[record.name].fields
                if field.value.kind == "array"
            )
            for typedef in self.source.typedefs
            if typedef.metadata.get("kind") == "slice"
            for record in (self.source.records_by_name.get(typedef.name),)
            if record is not None and record.name in self.values
        }

    @functools.cached_property
    def event_sources(self) -> frozenset[str]:
        """The handles that runtime events name as their source."""
        enum = self.source.enums_by_name.get(RUNTIME_EVENT_SOURCE)
        return frozenset(
            "mln_" + member.name.removeprefix(RUNTIME_EVENT_SOURCE_PREFIX).lower()
            for member in (enum.values if enum else ())
        )

    @functools.cached_property
    def enum_numbers(self) -> dict[str, int]:
        return {
            member.name: member.value
            for enum in self.source.enums
            for member in enum.values
        }

    def enum_number(self, symbol: str) -> int:
        return self.enum_numbers[symbol]

    @functools.cached_property
    def docs(self) -> dict[str, docs.Doc]:
        """The documentation of each documented public declaration, by C name.

        A registration record, and an operation that takes one directly, end
        with the threads of the callbacks that it registers. A registration
        nested in another record, such as a camera command's end handler,
        carries them in its own record's documentation.
        """
        index = docs.index(self.source)

        def threads(descriptor: str, callbacks) -> tuple[str, ...]:
            record = self.values[descriptor]
            return tuple(
                thread
                for member in record.fields
                if member.name in callbacks
                for thread in getattr(index.get(member.value.native), "threads", ())
            )

        for value in self.values.values():
            if value.registration and value.native in index:
                index[value.native] = docs.with_threads(
                    index[value.native],
                    threads(value.native, value.registration.callbacks),
                )
        for operation in self.operations:
            if operation.name in index:
                index[operation.name] = docs.with_threads(
                    index[operation.name],
                    (
                        thread
                        for registration in operation.registrations
                        if not registration.path
                        for thread in threads(
                            registration.descriptor, registration.callbacks
                        )
                    ),
                )
        return index

    def doc(self, native: str) -> docs.Doc | None:
        """The documentation of a declaration, or `record.field` member."""
        return self.docs.get(native)

    @functools.cached_property
    def _symbols(self) -> dict[str, docs.Symbol]:
        return {}

    def symbols(self, binding: str, build) -> docs.Symbol:
        """The symbol resolver of `binding`, which `build(self)` makes once."""
        if binding not in self._symbols:
            self._symbols[binding] = build(self)
        return self._symbols[binding]

    @functools.cached_property
    def control_enums(self) -> frozenset[str]:
        """Enums that only presence masks, union tags, and completion results
        use, other than the status that every runtime names.

        A binding writes and reads those fields itself, so no host code names
        the enum, and a binding keeps its values private.
        """
        control, used = set(), set()

        def visit(value: ValuePlan | None, uses: set[str]) -> None:
            if value is None:
                return
            if value.kind == "enum":
                uses.add(value.native)
            visit(value.element, uses)

        for value in self.values.values():
            for member in value.fields:
                visit(
                    member.value,
                    control
                    if member.role in {"presence_mask", "tag"}
                    or (
                        value.native == COMPLETION_RESULT
                        and member.value.native != STATUS
                    )
                    else used,
                )
        for operation in (*self.operations, *self.runtime_operations):
            for parameter in (*operation.inputs, *operation.outputs):
                visit(parameter.value, used)
            visit(operation.result, used)
        for callback in self.callbacks.values():
            for parameter in callback.parameters:
                visit(parameter.value, used)
            visit(callback.result, used)
        return frozenset(control - used)

    @property
    def public_values(self) -> dict[str, ValuePlan]:
        # A slice record reaches host code as its slice alone.
        return {
            name: value
            for name, value in self.values.items()
            if name not in self.source.runtime_types
            and name not in self.control_enums
            and name not in self.slices
        }

    @property
    def public_handles(self) -> dict[str, HandlePlan]:
        """Handles a host can own; handles of runtime operations stay internal."""
        runtime = {operation.name for operation in self.runtime_operations}
        return {
            name: handle
            for name, handle in self.handles.items()
            if handle.release not in runtime
        }

    @property
    def operations_by_name(self) -> dict[str, OperationPlan]:
        return {operation.name: operation for operation in self.operations}


def output_member(name: str) -> str:
    """An output parameter's public member name: its name without `out_`."""
    return name.removeprefix("out_")


def public_stem(native: str) -> str:
    """A declaration's name without the C API's `mln_` namespace."""
    return native.removeprefix("mln_")


# The verbs that a process global names after its subject in C, and first in
# every binding: mln_log_set_callback is set_log_callback.
GLOBAL_VERBS = frozenset({"clear", "get", "init", "set"})


def enum_member_prefix(constants, native: str | None = None) -> str:
    """The prefix that an enum's constants share, cut back to a whole word.

    When native names the enum's type, the prefix is also one the type name
    shares, so an enum with a single constant keeps that constant's last words.
    """
    names = list(constants)
    if not names:
        return ""
    if native is not None:
        names.append(native.upper() + "_")
    # The lexically first and last names differ wherever any two names do.
    first, last = min(names), max(names)
    shared = next(
        (first[:index] for index, (a, b) in enumerate(zip(first, last)) if a != b),
        first,
    )
    return shared.rsplit("_", 1)[0] + "_"


SCALAR_CANONICAL_TYPES = frozenset(
    {
        "void",
        "_Bool",
        "bool",
        "char",
        "signed char",
        "unsigned char",
        "short",
        "unsigned short",
        "int",
        "unsigned int",
        "long",
        "unsigned long",
        "long long",
        "unsigned long long",
        "float",
        "double",
        "long double",
        "__int128",
        "unsigned __int128",
    }
)


class ValueAttributes(TypedDict):
    native: str
    ctype: CType
    ownership: str
    encoding: str | None
    lifetime: str
    nullable: bool
    length: str | None
    stride: str | None


class Binder:
    def __init__(self, api: Api):
        validate(api)
        self.api = api
        self.records = api.records_by_name
        self.typedefs = api.typedefs_by_name
        self.enums = {enum.name: enum for enum in api.enums}
        self.values: dict[str, ValuePlan] = {}
        self.callbacks: dict[str, CallbackPlan] = {}
        self.resolving: set[str] = set()
        self.returned: set[str] = set()
        self.lent: set[str] = set()
        self.kept: set[str] = set()
        self.handles = {
            name: HandlePlan(
                name,
                typedef.metadata["release"],
                None
                if typedef.metadata.get("parent", "none") == "none"
                else typedef.metadata["parent"],
                typedef.metadata.get("dispose"),
                "always"
                if api.functions_by_name[typedef.metadata["release"]].return_type.kind
                == "void"
                else "success",
                typedef.metadata.get("abandon"),
                tuple(
                    parameter.name
                    for parameter in api.functions_by_name[
                        typedef.metadata["release"]
                    ].parameters[1:]
                    if not is_completion(parameter.type)
                    and parameter.metadata.get("direction", "in") != "out"
                ),
                (cleanup,)
                if (
                    cleanup := typedef.metadata.get(
                        "dispose", typedef.metadata.get("discard")
                    )
                )
                else tuple(
                    filter(
                        None,
                        (typedef.metadata.get("abandon"), typedef.metadata["release"]),
                    )
                ),
                view_begin=typedef.metadata.get("view_begin"),
                view_end=typedef.metadata.get("view_end"),
                discard=typedef.metadata.get("discard"),
                observable_root_release=self.observable_root_release(typedef),
                dispose_completion=self.dispose_completion(typedef),
            )
            for name, typedef in self.typedefs.items()
            if typedef.metadata.get("kind") == "handle"
            and "release" in typedef.metadata
        }

    def dispose_completion(self, typedef) -> str | None:
        if typedef.metadata.get("dispose") != typedef.metadata["release"]:
            return None
        release = self.api.functions_by_name[typedef.metadata["release"]]
        return next((p.name for p in release.parameters if is_completion(p.type)), None)

    def observable_root_release(self, typedef) -> bool:
        release = self.api.functions_by_name[typedef.metadata["release"]]
        return (
            typedef.metadata.get("parent", "none") == "none"
            and release.metadata.get("execution") == "lifecycle"
            and release.diagnostic
            and len(release.parameters) == 2
            and is_completion(release.parameters[1].type)
        )

    def scalar_carrier(self, type_: CType) -> str:
        """Preserve portable integer typedefs before Clang's host ABI expansion."""
        portable = {
            "bool",
            "_Bool",
            "float",
            "double",
            "void",
            "char",
            "size_t",
            "ptrdiff_t",
            "intptr_t",
            "uintptr_t",
            *(f"{sign}int{bits}_t" for sign in ("", "u") for bits in (8, 16, 32, 64)),
        }
        seen = set()
        while True:
            spelling = type_.spelling.removeprefix("const ")
            if spelling in portable:
                return spelling
            if spelling in seen or spelling not in self.typedefs:
                return type_.canonical.removeprefix("const ")
            seen.add(spelling)
            type_ = self.typedefs[spelling].type

    def value(self, type_: CType, metadata: dict[str, str], context: str) -> ValuePlan:
        name = type_name(type_)
        typedef = self.typedefs.get(name)
        combined = dict(typedef.metadata) if typedef else {}
        combined.update(metadata)
        metadata = combined
        common: ValueAttributes = {
            "native": name,
            "ctype": type_,
            "ownership": metadata.get("ownership", "value"),
            "encoding": metadata.get("encoding"),
            "lifetime": metadata.get("lifetime", "call"),
            "nullable": metadata.get("nullable") == "true",
            "length": metadata.get("length"),
            "stride": metadata.get("stride"),
        }
        if metadata.get("kind") == "native_pointer":
            return ValuePlan(kind="native_pointer", **common)
        resolved = typedef.type if typedef else type_
        enum = self.api.enum_for(name)
        if enum is not None and resolved.kind != "pointer":
            common["native"] = enum.name
            return ValuePlan(
                kind="enum",
                enum_kind=enum.metadata.get("kind", "open"),
                enum_underlying=enum.underlying_type,
                scalar_carrier=self.scalar_carrier(enum.underlying_type),
                enum_values=tuple((item.name, item.value) for item in enum.values),
                **common,
            )
        if name in self.handles:
            return ValuePlan(kind="handle", handle=self.handles[name], **common)
        if name == BUFFER_VIEW:
            record = self.records.get(name)
            if (
                record is None
                or tuple(f.name for f in record.fields)
                != (
                    "data",
                    "size",
                )
                or record.fields[0].type.kind != "pointer"
                or record.fields[0].type.pointee is None
                or record.fields[0].type.pointee.kind != "void"
                or record.fields[1].type.canonical
                not in {"unsigned long", "unsigned long long", "unsigned int"}
            ):
                raise ModelError([f"{context}: buffer view requires data and size"])
            common["encoding"] = metadata.get("encoding", "bytes")
            return ValuePlan(kind="buffer", buffer_form="view", **common)
        if resolved.kind == "pointer":
            pointee = resolved.pointee
            if pointee is None:
                raise ModelError([f"{context}: pointer has no pointee"])
            if pointee.kind == "function" and metadata.get("kind") != "native_pointer":
                if name not in self.typedefs:
                    raise ModelError(
                        [f"{context}: a callback requires a named function typedef"]
                    )
                if name not in self.callbacks:
                    self.callbacks[name] = self.callback(name)
                return ValuePlan(kind="callback", **common)
            if metadata.get("kind") in {"native_pointer", "context", "erased"}:
                return ValuePlan(kind="native_pointer", **common)
            child_metadata = {
                key: value
                for key, value in metadata.items()
                if key in {"encoding", "ownership", "lifetime", "tag"}
            }
            child_name = type_name(pointee)
            if metadata.get("direction") == "out":
                if "result_length" in metadata:
                    common["encoding"] = metadata["encoding"]
                    return ValuePlan(kind="buffer", buffer_form="pointer", **common)
                if pointee.kind != "pointer" and metadata.get("length") not in {
                    None,
                    "1",
                }:
                    element = self.value(
                        pointee, child_metadata, context + " output element"
                    )
                    # Each emitter decodes the elements after the call from
                    # zeroed storage, so an element must own nothing and borrow
                    # nothing.
                    if not plain_value(element):
                        raise ModelError(
                            [
                                (
                                    f"{context}: a caller-sized output element "
                                    "must be a scalar, an enum, or a record of "
                                    "them"
                                )
                            ]
                        )
                    return ValuePlan(kind="array", element=element, **common)
                if pointee.kind == "pointer" and "length" in metadata:
                    child_metadata["length"] = metadata["length"]
                element = self.value(pointee, child_metadata, context + " output")
                return ValuePlan(kind="reference", element=element, **common)
            if metadata.get("length") == "1":
                element = self.value(pointee, child_metadata, context + " pointee")
                return ValuePlan(kind="reference", element=element, **common)
            if metadata.get("length") == "nul":
                if pointee.kind not in {"char_s", "char_u", "schar", "uchar"}:
                    raise ModelError([f"{context}: nul length requires character data"])
                return ValuePlan(kind="buffer", buffer_form="pointer", **common)
            if "length" in metadata:
                if metadata.get("encoding") in {"utf8", "json", "bytes"} and (
                    pointee.kind in {"char_s", "char_u", "schar", "uchar"}
                    or pointee.canonical.removeprefix("const ")
                    in {"char", "signed char", "unsigned char"}
                ):
                    return ValuePlan(kind="buffer", buffer_form="pointer", **common)
                if child_name == "void":
                    if metadata.get("encoding") not in {"utf8", "json", "bytes"}:
                        raise ModelError([f"{context}: erased bytes require encoding"])
                    return ValuePlan(kind="buffer", buffer_form="pointer", **common)
                element = self.value(pointee, child_metadata, context + " element")
                return ValuePlan(kind="array", element=element, **common)
            raise ModelError([f"{context}: pointer requires length or native kind"])
        if resolved.kind == "array":
            if resolved.element is None or resolved.length is None:
                raise ModelError([f"{context}: array requires a fixed extent"])
            common["length"] = str(resolved.length)
            return ValuePlan(
                kind="array",
                element=self.value(resolved.element, {}, context + " element"),
                **common,
            )
        if name in self.records:
            record = self.records[name]
            if not record.complete:
                raise ModelError([f"{context}: incomplete record has no value layout"])
            if name in self.resolving:
                raise ModelError(
                    [f"{context}: recursive record requires a lifetime boundary"]
                )
            record_metadata = metadata
            if name in self.values:
                return replace(
                    self.values[name],
                    **common,
                    tag=record_metadata.get("tag", self.values[name].tag),
                    empty_variant=next(
                        (
                            (item.name, item.value)
                            for enum in self.api.enums
                            for item in enum.values
                            if item.name == record_metadata.get("empty_variant")
                        ),
                        self.values[name].empty_variant,
                    ),
                )
            self.resolving.add(name)
            try:
                fields = []
                for item in record.fields:
                    field_context = f"{item.location}: {name}.{item.name}"
                    presence = None
                    if any(key in item.metadata for key in ("mask", "tag", "variant")):
                        presence = Presence(
                            **{
                                key: item.metadata.get(key)
                                for key in ("mask", "bit", "tag", "variant")
                            }
                        )
                    role = item.metadata.get("kind", "value")
                    member_value = self.value(item.type, item.metadata, field_context)
                    fields.append(
                        FieldPlan(
                            item.name,
                            member_value,
                            presence,
                            role,
                            item.metadata.get("required") == "true",
                        )
                    )
                storage_roles = {}
                if record_metadata.get("kind") == "callback_registration":
                    storage_roles[record_metadata["release"]] = "release"
                for member in fields:
                    if member.value.stride:
                        storage_roles[member.value.stride] = "stride"

                fields = [
                    replace(member, role=storage_roles[member.name])
                    if member.name in storage_roles
                    else member
                    for member in fields
                ]
                if record.kind == "union" and any(
                    member.presence is None or member.presence.variant is None
                    for member in fields
                ):
                    raise ModelError(
                        [f"{context}: every union member requires a variant"]
                    )
                value = ValuePlan(
                    kind="union" if record.kind == "union" else "record",
                    fields=tuple(fields),
                    tag=record_metadata.get("tag"),
                    empty_variant=next(
                        (
                            (item.name, item.value)
                            for enum in self.api.enums
                            for item in enum.values
                            if item.name == record_metadata.get("empty_variant")
                        ),
                        None,
                    ),
                    response=self.response(record.name)
                    if record_metadata.get("kind") == "callback_response"
                    else None,
                    registration=RegistrationDescriptorPlan(
                        tuple(
                            member.name
                            for member in fields
                            if member.value.kind == "callback"
                            and member.name != record_metadata["release"]
                        ),
                        record_metadata["user_data"],
                        record_metadata["release"],
                        next(
                            (
                                member.name
                                for member in fields
                                if member.role == "delivery_queue"
                            ),
                            None,
                        ),
                    )
                    if record_metadata.get("kind") == "callback_registration"
                    else None,
                    mask_flags=tuple(
                        MaskFlag(
                            control.name,
                            flag,
                            flag_value,
                            flag.removeprefix(
                                enum_member_prefix(
                                    (name for name, _ in control.value.enum_values),
                                    control.value.native,
                                )
                            ).lower(),
                        )
                        for control in fields
                        if control.role == "presence_mask"
                        for flag, flag_value in control.value.enum_values
                        if flag_value > 0
                        and flag_value & (flag_value - 1) == 0
                        and not any(
                            member.presence
                            and member.presence.mask == control.name
                            and member.presence.bit == flag
                            for member in fields
                        )
                    ),
                    **common,
                )
                self.values[name] = value
                return value
            finally:
                self.resolving.remove(name)
        if resolved.kind == "function":
            return ValuePlan(kind="callback", **common)
        if resolved.canonical.removeprefix(
            "const "
        ) in SCALAR_CANONICAL_TYPES or resolved.kind in {
            "void",
            "bool",
            "char_s",
            "char_u",
            "schar",
            "uchar",
            "short",
            "ushort",
            "int",
            "uint",
            "long",
            "ulong",
            "longlong",
            "ulonglong",
            "float",
            "double",
            "longdouble",
            "int128",
            "uint128",
        }:
            return ValuePlan(
                kind="scalar", scalar_carrier=self.scalar_carrier(type_), **common
            )
        if typedef and resolved.declaration and resolved.declaration != name:
            return replace(self.value(resolved, metadata, context), **common)
        raise ModelError([f"{context}: unresolved value {name} ({resolved.kind})"])

    def operation(self, function: Function) -> OperationPlan:
        context = f"{function.location}: {function.name}"
        inputs, outputs, registrations = [], [], []
        receiver = None
        if function.parameters:
            first = function.parameters[0]
            first_type = (
                first.type.pointee if first.type.kind == "pointer" else first.type
            )
            if (
                first.metadata.get("direction", "in") != "out"
                and first_type
                and type_name(first_type) in self.handles
            ):
                receiver = first.name
        for parameter in function.parameters:
            if is_completion(parameter.type):
                continue
            metadata = parameter.metadata
            value = self.value(
                parameter.type, metadata, f"{context} parameter {parameter.name}"
            )
            direction = metadata.get("direction", "in")
            plan = ParameterPlan(
                parameter.name,
                value,
                direction,
                next(
                    (
                        input_.name
                        for input_ in function.parameters
                        if input_.metadata.get("direction", "in") == "in"
                        and input_.metadata.get("length") == value.length
                        and input_.type.kind == "pointer"
                    ),
                    None,
                )
                if direction == "out" and value.kind == "array"
                else None,
                metadata.get("result_length"),
            )

            def validate_input_lifetime(item, parameter_name=parameter.name):
                """An input that a binding marshals lasts for the call.

                A binding copies a buffer, array, or record into storage that
                it frees when the call returns, so no input value may claim a
                longer lifetime. Opaque pointers, handles, callbacks, and
                registrations carry their own retention contracts, and a scalar
                is copied by value, such as the bit pattern of a native object
                that a record lends when native writes it.
                """
                if item.kind in {"native_pointer", "handle", "callback", "scalar"} or (
                    item.registration
                ):
                    return
                if item.lifetime != "call":
                    raise ModelError(
                        [
                            f"{context} parameter {parameter_name}: retained input requires a lifetime contract"
                        ]
                    )
                if item.element:
                    validate_input_lifetime(item.element)
                for member in item.fields:
                    validate_input_lifetime(member.value)

            def validate_input_layout(item, parameter_name=parameter.name):
                if item.stride or any(
                    member.role in {"stride"} for member in item.fields
                ):
                    raise ModelError(
                        [
                            f"{context} parameter {parameter_name}: strided record requires an input reconstruction contract"
                        ]
                    )
                if item.element:
                    validate_input_layout(item.element)
                for member in item.fields:
                    validate_input_layout(member.value)

            if direction != "out":
                validate_input_layout(value)
                validate_input_lifetime(value)
            (outputs if direction == "out" else inputs).append(plan)

            def collect_registrations(
                descriptor, path=(), parameter_name=parameter.name
            ):
                if descriptor.kind == "reference" and descriptor.element:
                    collect_registrations(descriptor.element, path)
                elif descriptor.kind == "record":
                    registration = descriptor.registration
                    if registration:
                        registrations.append(
                            RegistrationPlan(
                                parameter_name,
                                descriptor.native,
                                registration.callbacks,
                                registration.user_data,
                                registration.release,
                                path,
                                function.metadata.get("accepted_unless"),
                                registration.queue,
                            )
                        )
                    else:
                        for member in descriptor.fields:
                            collect_registrations(member.value, (*path, member.name))

            collect_registrations(value)
        result = None
        metadata = function.metadata
        if "result" in metadata and metadata["result"] != "void":
            result_name = metadata["result"]
            typedef = self.typedefs.get(result_name)
            type_ = (
                CType("typedef", result_name, typedef.type.canonical, result_name)
                if typedef
                else CType(result_name, result_name, result_name)
            )
            result = self.value(type_, metadata, context + " completion")
            if metadata.get("shape") == "array":
                # An array result steps by the stride of the native build
                # rather than by the binding's own size of the element.
                result = ValuePlan(
                    "array",
                    result.native,
                    result.ctype,
                    ownership=metadata.get("ownership", "borrowed"),
                    lifetime="completion",
                    nullable=result.nullable,
                    length=COMPLETION_VALUE_COUNT,
                    stride=COMPLETION_VALUE_SIZE,
                    element=replace(result, nullable=False),
                )
        elif not any(is_completion(p.type) for p in function.parameters):
            if function.return_type.kind != "void" and not is_status(
                function.return_type
            ):
                result = self.value(function.return_type, metadata, context + " return")
        support: DisposeSupport | ViewSupport | DeliverySupport | None = None
        for parameter in function.parameters:
            delivery_handle = type_name(parameter.type.pointee or parameter.type)
            if delivery_handle in {DELIVERY_QUEUE, DELIVERY_BATCH}:
                support = DeliverySupport(delivery_handle)
                break
        completion_parameter = next(
            (
                parameter.name
                for parameter in function.parameters
                if is_completion(parameter.type)
            ),
            None,
        )
        owners = []
        for output in outputs:
            value = (
                output.value.element
                if output.value.kind == "reference"
                else output.value
            )
            if value and value.handle and value.ownership == "owned":
                parent = next(
                    (
                        parameter.name
                        for parameter in inputs
                        if parameter.value.native == value.handle.parent
                    ),
                    None,
                )
                if value.handle.parent and parent is None:
                    raise ModelError(
                        [
                            f"{context}: owned output {output.name} requires its parent handle input"
                        ]
                    )
                if parent is not None and parent != receiver:
                    raise ModelError(
                        [f"{context}: owned output requires its parent receiver"]
                    )
                owners.append(OwnedOutputPlan(output.name, value.handle, parent))
        completion = None
        if completion_parameter:
            completion = CompletionPlan(
                completion_parameter,
                tuple(owners),
                discardable=metadata["execution"] == "command",
                optional=any(
                    handle.dispose_completion == completion_parameter
                    and handle.dispose == function.name
                    for handle in self.handles.values()
                ),
            )
        # A release consumes its handle; another call that answers its handle
        # once declares consumes=, as a resource request's completion does.
        consumes = metadata.get("consumes")
        release = False
        for handle in self.handles.values():
            if function.name in {handle.release, handle.cleanup}:
                consumes = handle.release_consumes
                release = True
                if function.name == handle.dispose and function.name != handle.release:
                    support = DisposeSupport(handle)
                break
            if function.name in {handle.view_begin, handle.view_end}:
                support = ViewSupport(handle)
                break
        if consumes and receiver is None:
            raise ModelError(
                [f"{context}: consumption requires a managed handle receiver"]
            )
        scoped_receiver = next(
            (
                parameter.name
                for parameter in (*inputs, *outputs)
                if function.parameters
                and parameter.name == function.parameters[0].name
                and parameter.value.element
                and parameter.value.element.response
            ),
            None,
        )
        factory = self.factory(function, receiver or scoped_receiver, owners)
        member = self.member(function, receiver or scoped_receiver, factory)
        view = None
        if "view_owner" in metadata:
            parameter = next(
                p for p in function.parameters if p.name == metadata["view_owner"]
            )
            handle = self.handles[type_name(parameter.type)]
            if (
                handle.view_begin is None
                or handle.view_end is None
                or len(outputs) != 1
            ):
                raise ModelError(
                    [
                        f"{context}: a borrowed view requires its owner's view scope and one output"
                    ]
                )
            view = BorrowedViewPlan(
                parameter.name,
                handle,
                outputs[0],
                handle.view_begin,
                handle.view_end,
                member.removeprefix("get_"),
            )
        absence = None
        if "absent_on" in metadata:
            # An absent call publishes nothing, so it neither consumes its
            # receiver nor roots a registration.
            if view or consumes:
                raise ModelError(
                    [f"{context}: absent_on requires an output that the caller owns"]
                )
            if registrations:
                raise ModelError(
                    [f"{context}: absent_on requires a call without registrations"]
                )
            absence = AbsencePlan(
                metadata["absent_on"],
                next(
                    value.value
                    for enum in self.api.enums
                    if enum.name == STATUS
                    for value in enum.values
                    if value.name == metadata["absent_on"]
                ),
                outputs[0],
            )
        drain = None
        if absence and len(outputs) == 2:
            # A drain publishes its owned batch with a slice view of it.
            def slice_record(parameter):
                value = parameter.value
                value = value.element if value.kind == "reference" else value
                typedef = self.typedefs.get(value.native) if value else None
                is_slice = (
                    typedef is not None and typedef.metadata.get("kind") == "slice"
                )
                return value if is_slice else None

            (slice_view,) = [o for o in outputs if slice_record(o)]
            (batch,) = [o for o in outputs if not slice_record(o)]
            record = slice_record(slice_view)
            drain = DrainPlan(
                batch,
                slice_view,
                next(f for f in record.fields if f.value.kind == "array"),
            )
        # A runtime export's caller reads its outputs itself, so only a
        # generated operation copies what native writes.
        if function.name not in self.api.runtime_exports and not isinstance(
            support, DeliverySupport
        ):
            kept = self.copied(outputs, result, context)
            self.returned |= kept
            self.kept |= kept
        return OperationPlan(
            function,
            metadata["execution"],
            receiver,
            tuple(inputs),
            tuple(outputs),
            result,
            tuple(registrations),
            consumes,
            release,
            support,
            completion,
            tuple(owners),
            view,
            scoped_receiver,
            is_status(function.return_type),
            member,
            drain=drain,
            absence=absence,
            blocks=metadata.get("blocks"),
            factory=factory,
            unchecked=function.name == ABI_VERSION,
        )

    def copied(self, parameters, result: ValuePlan | None, context: str) -> set[str]:
        """The records and unions that a binding copies from these values.

        A binding builds a callback registration from host callbacks and never
        adopts one from native, so the plan rejects a copy that holds one.
        """
        found: set[str] = set()

        def visit(value: ValuePlan, path: str) -> None:
            if value.registration:
                raise ModelError(
                    [f"{path}: native cannot return a callback registration"]
                )
            if value.response:
                return
            if value.element:
                visit(value.element, path)
            if value.kind not in {"record", "union"} or value.native in found:
                return
            found.add(value.native)
            for member in value.fields:
                visit(member.value, f"{path}.{member.name}")

        for parameter in parameters:
            visit(parameter.value, f"{context} parameter {parameter.name}")
        if result is not None:
            visit(result, f"{context} result")
        return found

    def factory(self, function: Function, receiver: str | None, owners) -> str | None:
        """The root handle that a public receiverless call creates and is named
        for, such as mln_runtime_create for mln_runtime."""
        if receiver is not None or function.name in self.api.runtime_exports:
            return None
        created = [
            owned.handle.native
            for owned in owners
            if owned.handle.parent is None
            and function.name.startswith(owned.handle.native + "_")
        ]
        return created[0] if len(created) == 1 else None

    def member(
        self, function: Function, receiver: str | None, factory: str | None = None
    ) -> str:
        """Name an operation once for every binding.

        An operation on a handle, or on a callback response, drops its
        receiver's prefix when its name starts with it, and a factory drops the
        prefix of the handle it creates. A public process global names its verb
        first, as set_log_callback does for mln_log_set_callback; every other
        name drops `mln_`. A dispose that is also its handle's release is named
        for the release, which passes the completion, because the dispose
        passes none.
        """
        if factory is not None:
            return function.name.removeprefix(factory + "_")
        if any(
            handle.dispose_completion and handle.release == function.name
            for handle in self.handles.values()
        ):
            return "release"
        if receiver is not None:
            parameter = next(p for p in function.parameters if p.name == receiver)
            owner = type_name(parameter.type.pointee or parameter.type)
            if function.name.startswith(owner + "_"):
                return function.name.removeprefix(owner + "_")
        stem = public_stem(function.name)
        subject, *rest = stem.split("_")
        if (
            receiver is None
            and function.name not in self.api.runtime_exports
            and rest
            and rest[0] in GLOBAL_VERBS
        ):
            return "_".join((rest[0], subject, *rest[1:]))
        return stem

    def response(self, name: str) -> CallbackResponsePlan:
        methods = tuple(
            f.name
            for f in self.api.functions
            if f.parameters
            and f.parameters[0].type.pointee
            and type_name(f.parameters[0].type.pointee) == name
        )
        return CallbackResponsePlan(name, methods)

    def callback(self, name: str) -> CallbackPlan:
        typedef = self.typedefs[name]
        function = typedef.type.pointee
        assert function is not None and function.result is not None
        context = f"{typedef.location}: {name}"
        if "failure" not in typedef.metadata:
            raise ModelError([f"{context}: callback requires a failure contract"])
        contexts = [
            parameter.name
            for parameter in typedef.parameters
            if parameter.metadata.get("kind") == "context"
        ]
        if len(contexts) > 1:
            raise ModelError(
                [f"{context}: callback requires at most one context parameter"]
            )
        parameters = tuple(
            ParameterPlan(
                parameter.name,
                self.value(
                    parameter.type,
                    parameter.metadata,
                    context + " parameter " + parameter.name,
                ),
                parameter.metadata.get("direction", "in"),
            )
            for parameter in typedef.parameters
        )
        lent = self.copied(
            tuple(p for p in parameters if p.direction != "out"), None, context
        )
        self.returned |= lent
        self.lent |= lent
        return CallbackPlan(
            name,
            parameters,
            self.value(function.result, {}, context + " return"),
            typedef.metadata["failure"],
            contexts[0] if contexts else None,
            typedef.metadata.get("borrowed_owner"),
            is_status(function.result),
            synchronous=typedef.metadata.get("synchronous") == "true",
            delivery=self.delivery(typedef),
            queue_result=(
                typedef.metadata.get("queue_result")
                if "delivery" in typedef.metadata
                else None
            ),
        )

    def delivery(self, typedef) -> DeliveryPlan | None:
        """The queue delivery that a callback's `delivery=` names."""
        constant = typedef.metadata.get("delivery")
        if constant is None:
            return None
        payload = self.api.records_by_name[DELIVERY_PAYLOAD]
        member = next(
            field
            for field in payload.fields
            if field.metadata.get("variant") == constant
        )
        return DeliveryPlan(constant, member.name, type_name(member.type))

    def bind(self, *, require_complete: bool = False) -> BoundApi:
        operations, diagnostics, unsupported = [], [], {}
        for enum in self.api.enums:
            self.values[enum.name] = self.value(
                CType("enum", enum.name, "enum " + enum.name, enum.name),
                {},
                f"{enum.location}: enum {enum.name}",
            )
        for name, typedef in self.typedefs.items():
            if typedef.type.pointee and typedef.type.pointee.kind == "function":
                try:
                    self.callbacks[name] = self.callback(name)
                except ModelError as error:
                    unsupported[name] = tuple(error.diagnostics)
                    diagnostics.extend(error.diagnostics)
        for function in self.api.functions:
            try:
                operations.append(self.operation(function))
            except ModelError as error:
                unsupported[function.name] = tuple(error.diagnostics)
                diagnostics.extend(error.diagnostics)
        if require_complete and diagnostics:
            raise ModelError(diagnostics)
        # A drain releases the batch it publishes itself, so the batch handle
        # and its release stay internal.
        batches = {
            (value.element if value.kind == "reference" else value).native
            for operation in operations
            if operation.drain
            for value in (operation.drain.batch.value,)
        }
        drained = {
            self.handles[native].release for native in batches if native in self.handles
        }

        def internal(operation):
            return (
                operation.name in self.api.runtime_exports
                or operation.name in drained
                or isinstance(operation.support, DeliverySupport)
            )

        return BoundApi(
            self.api,
            tuple(
                operation
                for operation in operations
                if not internal(operation)
                and not isinstance(operation.support, ViewSupport)
            ),
            self.values,
            self.callbacks,
            self.handles,
            tuple(sorted(set(diagnostics))),
            unsupported,
            tuple(operation for operation in operations if internal(operation)),
            frozenset(self.returned),
            frozenset(self.lent - self.kept),
        )


def bind(api: Api, *, require_complete: bool = False) -> BoundApi:
    return Binder(api).bind(require_complete=require_complete)
