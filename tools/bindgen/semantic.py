"""Resolve C declarations into shared, statically compiled binding plans.

Plans contain semantic decisions, never target-language source. Backends render
these plans and keep allocation, callback roots, and scheduling in their runtime.
"""

from __future__ import annotations

import functools
import os
from collections.abc import Iterable
from dataclasses import dataclass, field, replace
from typing import TypedDict

from . import docs
from .model import Api, CType, Function, ModelError
from .names import type_name
from .protocol import BUFFER_VIEW, STATUS, is_completion, is_status
from .schema import validate


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
    # The name that the handle's operations begin with: its `prefix=` metadata,
    # or the handle's own name.
    prefix: str = ""
    # Whether the handle is a root whose release is a lifecycle operation that
    # reports retirement through its completion, so a host can wait for it.
    observable_root_release: bool = False

    @property
    def stem(self) -> str:
        """The handle's public name: its operation prefix without `mln_`."""
        return public_stem(self.prefix or self.native)


@dataclass(frozen=True)
class OwnedOutputPlan:
    parameter: str
    handle: HandlePlan
    parent_parameter: str | None


@dataclass(frozen=True)
class CompletionPlan:
    parameter: str
    immediate_owners: tuple[OwnedOutputPlan, ...]
    result_owned: bool
    result_owner: OwnedOutputPlan | None = None


@dataclass(frozen=True)
class DecisionPlan:
    parameter: str
    handle: HandlePlan
    accept: str
    pass_through: str
    complete: str
    cancelled: str
    cancel_registration: str
    wait_retired: str


@dataclass(frozen=True)
class MaskFlag:
    mask: str
    name: str
    value: int
    # The flag's public member name: the constant without its enum's prefix.
    member: str = ""


@dataclass(frozen=True)
class RegistrationDescriptorPlan:
    callbacks: tuple[str, ...]
    user_data: str
    release: str


@dataclass(frozen=True)
class CallbackResponsePlan:
    native: str
    context: str
    callbacks: tuple[str, ...]
    methods: tuple[str, ...]
    lifetime: str = "callback"


@dataclass(frozen=True)
class ItemBufferPlan:
    field: str
    data: str
    size: str
    offset: str
    length: str
    encoding: str


@dataclass(frozen=True)
class ValuePlan:
    kind: str
    native: str
    ctype: CType
    ownership: str = "value"
    encoding: str | None = None
    lifetime: str = "call"
    nullable: bool = False
    optional: str | None = None
    length: str | None = None
    stride: str | None = None
    item_buffer: ItemBufferPlan | None = None
    fields: tuple[FieldPlan, ...] = ()
    element: ValuePlan | None = None
    handle: HandlePlan | None = None
    enum_kind: str | None = None
    enum_underlying: CType | None = None
    scalar_carrier: str | None = None
    enum_values: tuple[tuple[str, int], ...] = ()
    default: str | None = None
    tag: str | None = None
    empty_variant: tuple[str, int] | None = None
    mask_flags: tuple[MaskFlag, ...] = ()
    registration: RegistrationDescriptorPlan | None = None
    response: CallbackResponsePlan | None = None
    # For kind="buffer": "view" for the mln_buffer_view struct, "pointer" for a
    # character or byte pointer with a separate length or NUL terminator.
    buffer_form: str | None = None
    # For a record annotated `fields=ordered`: its field order is part of its
    # meaning, as with coordinates, so a binding may construct it positionally.
    ordered: bool = False


# Field roles that hold control state: a struct's size, reserved space, an
# array's count or stride, an item arena, a presence mask, a union tag, a
# callback context, and a registration's release callback.
CONTROL_ROLES = frozenset(
    {
        "size",
        "reserved",
        "count",
        "stride",
        "arena",
        "presence_mask",
        "tag",
        "context",
        "release",
    }
)


@dataclass(frozen=True)
class FieldInitial:
    """The nonzero value that a field holds in its record's native default.

    `literal` is the annotation as written; `value` is the number it denotes.
    For an enum field, `enumerator` names the constant and `member` is that
    constant without its enum's shared prefix, in lower case.
    """

    literal: str
    value: int | float | bool
    enumerator: str | None = None
    member: str | None = None


@dataclass(frozen=True)
class FieldPlan:
    name: str
    value: ValuePlan
    presence: Presence | None = None
    # A control role's conventional value: `sizeof` for a size, `0` for
    # reserved space.
    default: str | None = None
    # The field's `kind`, or "value". A role in CONTROL_ROLES marks state that
    # a binding writes or derives rather than a member a host sets or reads.
    role: str = "value"
    # The field's annotated `default=`: its value in the record's native
    # default, which a binding that builds the record from language defaults
    # uses in place of zero.
    initial: FieldInitial | None = None

    @property
    def public(self) -> bool:
        return self.role not in CONTROL_ROLES


@dataclass(frozen=True)
class ParameterPlan:
    name: str
    value: ValuePlan
    direction: str
    consumes: str | None = None


@dataclass(frozen=True)
class CallbackReentryPlan:
    owner_parameter: str | None
    owner_type: str
    operations: tuple[str, ...]
    registration_owner: bool = False


@dataclass(frozen=True)
class CallbackPlan:
    native: str
    parameters: tuple[ParameterPlan, ...]
    result: ValuePlan
    failure: str | None = None
    decision: DecisionPlan | None = None
    context: str | None = None
    reentry: str = "allow"
    reentry_policy: CallbackReentryPlan | None = None
    # The C result an adapter may return at once for a host that receives a
    # copy of the call later.
    deferred: str | None = None
    # Whether the callback returns the status enum.
    status: bool = False
    # Whether native code relies on the callback's work being done when it
    # returns, so a binding must run it on the calling thread rather than
    # deliver it later, even when it returns nothing.
    synchronous: bool = False


@dataclass(frozen=True)
class RegistrationPlan:
    parameter: str
    descriptor: str
    callbacks: tuple[str, ...]
    user_data: str
    release: str
    path: tuple[str, ...] = ()


@dataclass(frozen=True)
class DirectRegistrationPlan:
    callback: str
    user_data: str
    release_callback: str | None
    accepted_unless: str | None


@dataclass(frozen=True)
class BorrowedViewPlan:
    owner_parameter: str
    owner: HandlePlan
    invalidated_by: tuple[str, ...]
    # The one output that the view borrows, and the owner's scope operations.
    output: ParameterPlan
    begin: str
    end: str
    # The accessor's name without its verb: the operation member without a
    # leading `get_`, so a scoped accessor reads as `with_<stem>`.
    stem: str


@dataclass(frozen=True)
class DefaultSupport:
    """The operation returns the default value of this record."""

    value: str


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
class AbsencePlan:
    """A failure status that reports the operation's one output as absent.

    A binding returns its language's empty form for this status instead of an
    error, and adopts or copies the output only on success.
    """

    # The status enumerator, and its value.
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
    support: DefaultSupport | DisposeSupport | ViewSupport | None = None
    completion: CompletionPlan | None = None
    owned_outputs: tuple[OwnedOutputPlan, ...] = ()
    direct_registrations: tuple[DirectRegistrationPlan, ...] = ()
    view: BorrowedViewPlan | None = None
    scoped_receiver: str | None = None
    receiver_access: str = "live"
    # Whether the function returns the status enum, which a binding checks
    # rather than returns.
    status: bool = False
    # The operation's language-neutral member name, which every binding only
    # case-converts and escapes; see `Binder.member`.
    member: str = ""
    absence: AbsencePlan | None = None

    @property
    def name(self) -> str:
        return self.function.name

    @property
    def role(self) -> str:
        return "support" if self.support else "public"


@dataclass(frozen=True)
class CallbackAdapterPlan:
    function: str
    callback: str
    context: str


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
    callback_adapters: tuple[CallbackAdapterPlan, ...] = ()
    # The scope operations of borrowed views, which bindings call from the
    # views they generate instead of exposing.
    view_scopes: tuple[OperationPlan, ...] = ()
    # The records and unions that a binding copies from native: those that an
    # operation's output or result, a callback's argument, or a record default
    # reaches. A copy leaves a callback registration field unset.
    returned: frozenset[str] = frozenset()

    @functools.cached_property
    def docs(self) -> dict[str, docs.Doc]:
        """The documentation of each documented public declaration, by C name."""
        return docs.index(self.source)

    def doc(self, native: str) -> docs.Doc | None:
        """The documentation of a declaration, or `record.field` member."""
        return self.docs.get(native)

    @property
    def public_values(self) -> dict[str, ValuePlan]:
        return {
            name: value
            for name, value in self.values.items()
            if name not in self.source.runtime_types
        }

    @property
    def public_handles(self) -> dict[str, HandlePlan]:
        """Handles a host can own; runtime-adapter handles stay internal."""
        runtime = {operation.name for operation in self.runtime_operations}
        return {
            name: handle
            for name, handle in self.handles.items()
            if handle.release not in runtime
        }

    @property
    def operations_by_name(self) -> dict[str, OperationPlan]:
        return {operation.name: operation for operation in self.operations}

    @property
    def decisions(self) -> dict[str, DecisionPlan]:
        """The decision protocol of each decision handle, by handle name."""
        return {
            callback.decision.handle.native: callback.decision
            for callback in self.callbacks.values()
            if callback.decision
        }

    def adapter_operations(
        self, adapter: CallbackAdapterPlan
    ) -> tuple[OperationPlan, ...]:
        """The operations that a callback adapter can call for its host.

        A callback answers through the response records it writes, so its
        adapter calls the operations scoped to those records.
        """
        callback = self.callbacks[adapter.callback]
        responses = {
            parameter.value.element.native
            for parameter in callback.parameters
            if parameter.direction in {"out", "inout"} and parameter.value.element
        }
        return tuple(
            plan
            for plan in self.operations
            if plan.scoped_receiver
            and next(
                p.value.element.native
                for p in (*plan.inputs, *plan.outputs)
                if p.name == plan.scoped_receiver
            )
            in responses
        )

    @property
    def defaults(self) -> dict[str, OperationPlan]:
        """The operation that returns each record's default value."""
        return {
            plan.support.value: plan
            for plan in (*self.operations, *self.runtime_operations)
            if isinstance(plan.support, DefaultSupport)
        }


def support_relation(plan: OperationPlan) -> dict[str, str] | None:
    """The support relation of an operation, as coverage reports record it."""
    if isinstance(plan.support, DefaultSupport):
        return {"default": plan.support.value}
    if isinstance(plan.support, DisposeSupport):
        return {"dispose": plan.support.handle.native}
    if isinstance(plan.support, ViewSupport):
        return {"view": plan.support.handle.native}
    return None


def view_support(bound: BoundApi, generated: Iterable[str]) -> dict[str, dict]:
    """The support relations of the view scopes that generated views call.

    A binding reports these with its coverage, since it calls each scope only
    from the borrowed views that it generates.
    """
    generated = set(generated)
    used = {
        name
        for plan in bound.operations
        if plan.view and plan.name in generated
        for name in (plan.view.begin, plan.view.end)
    }
    return {
        plan.name: support_relation(plan)
        for plan in bound.view_scopes
        if plan.name in used
    }


def output_member(name: str) -> str:
    """An output parameter's public member name: its name without `out_`."""
    return name.removeprefix("out_")


def public_stem(native: str) -> str:
    """A declaration's name without the C API's `mln_` namespace."""
    return native.removeprefix("mln_")


def field_initial(value: ValuePlan, literal: str) -> FieldInitial:
    """Resolve a field's `default=` annotation, which the schema checked."""
    if value.kind == "enum":
        constants = dict(value.enum_values)
        return FieldInitial(
            literal,
            constants[literal],
            literal,
            literal.removeprefix(enum_member_prefix(constants)).lower(),
        )
    if literal == "true":
        return FieldInitial(literal, True)
    if "." in literal:
        return FieldInitial(literal, float(literal))
    return FieldInitial(literal, int(literal))


def enum_member_prefix(constants) -> str:
    """The prefix that an enum's constants share, cut back to a whole word."""
    names = list(constants)
    return os.path.commonprefix(names).rsplit("_", 1)[0] + "_" if names else ""


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
    optional: str | None
    length: str | None
    stride: str | None
    item_buffer: ItemBufferPlan | None


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
        self.handles = {
            name: HandlePlan(
                name,
                typedef.metadata["release"],
                None
                if typedef.metadata.get("parent", "none") == "none"
                else typedef.metadata["parent"],
                typedef.metadata.get("dispose"),
                # A release that cannot fail consumes its handle every time.
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
                (typedef.metadata["dispose"],)
                if typedef.metadata.get("dispose")
                else tuple(
                    filter(
                        None,
                        (typedef.metadata.get("abandon"), typedef.metadata["release"]),
                    )
                ),
                view_begin=typedef.metadata.get("view_begin"),
                view_end=typedef.metadata.get("view_end"),
                prefix=typedef.metadata.get("prefix", name),
                observable_root_release=self.observable_root_release(typedef),
            )
            for name, typedef in self.typedefs.items()
            if typedef.metadata.get("kind") == "handle"
            and "release" in typedef.metadata
        }

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
            "optional": metadata.get("optional"),
            "length": metadata.get("length"),
            "stride": metadata.get("stride"),
            "item_buffer": ItemBufferPlan(
                metadata["item_name"],
                metadata["item_buffer"],
                metadata["item_buffer_size"],
                metadata["item_offset"],
                metadata["item_length"],
                metadata["item_encoding"],
            )
            if "item_buffer" in metadata
            else None,
        }
        if metadata.get("kind") == "native_pointer":
            return ValuePlan(kind="native_pointer", **common)
        if type_.volatile or type_.restrict:
            # These qualifiers remain in the CType for raw ABI emission.
            pass
        resolved = typedef.type if typedef else type_
        enum_name = metadata.get("enum", name)
        if enum_name in self.enums and resolved.kind != "pointer":
            enum = self.enums[enum_name]
            common["native"] = enum_name
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
                if key in {"encoding", "ownership", "lifetime", "enum", "tag"}
            }
            child_name = type_name(pointee)
            if metadata.get("direction") in {"out", "inout"}:
                if pointee.kind != "pointer" and metadata.get("length") not in {
                    None,
                    "1",
                }:
                    element = self.value(
                        pointee, child_metadata, context + " output element"
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
            record_metadata = {**record.metadata, **metadata}
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
                    if item.bit_width is not None:
                        raise ModelError(
                            [
                                f"{field_context}: bitfield has no portable ABI conversion"
                            ]
                        )
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
                    control_default = role in {"size", "reserved"}
                    fields.append(
                        FieldPlan(
                            item.name,
                            member_value,
                            presence,
                            item.metadata.get("default") if control_default else None,
                            role,
                            None
                            if control_default or "default" not in item.metadata
                            else field_initial(member_value, item.metadata["default"]),
                        )
                    )
                storage_roles = {}
                if record_metadata.get("kind") == "callback_registration":
                    storage_roles[record_metadata["release"]] = "release"
                for member in fields:
                    if member.value.stride:
                        storage_roles[member.value.stride] = "stride"
                    if member.value.item_buffer:
                        storage_roles[member.value.item_buffer.data] = "arena"
                        storage_roles[member.value.item_buffer.size] = "count"
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
                    default=record_metadata.get("default"),
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
                    ordered=record_metadata.get("fields") == "ordered",
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
                                    name for name, _ in control.value.enum_values
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
                parameter.name, value, direction, metadata.get("consumes")
            )

            def validate_input_lifetime(item, parameter_name=parameter.name):
                """An input that a binding marshals lasts for the call.

                A binding copies a buffer, array, or record into storage that
                it frees when the call returns, so no input value may claim a
                longer lifetime. Opaque pointers, handles, callbacks, and
                registrations carry their own retention contracts.
                """
                if item.kind in {"native_pointer", "handle", "callback"} or (
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
                if (
                    item.stride
                    or item.item_buffer
                    or any(member.role in {"stride", "arena"} for member in item.fields)
                ):
                    raise ModelError(
                        [
                            f"{context} parameter {parameter_name}: strided or arena-backed record requires an input reconstruction contract"
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
            if (
                result.ownership == "owned"
                and result.handle
                and not result.handle.dispose
            ):
                raise ModelError(
                    [
                        f"{context}: owned result requires a disposal contract for abandoned delivery"
                    ]
                )
            if metadata.get("shape") == "array":
                result = ValuePlan(
                    "array",
                    result.native,
                    result.ctype,
                    ownership=metadata.get("ownership", "borrowed"),
                    lifetime="completion",
                    nullable=result.nullable,
                    length="value_count",
                    element=replace(result, nullable=False),
                )
        elif not any(is_completion(p.type) for p in function.parameters):
            if function.return_type.kind != "void" and not is_status(
                function.return_type
            ):
                result = self.value(function.return_type, metadata, context + " return")
        support: DefaultSupport | DisposeSupport | ViewSupport | None = None
        for name, typedef in self.typedefs.items():
            if typedef.metadata.get("default") == function.name:
                support = DefaultSupport(name)
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
                owners.append(OwnedOutputPlan(output.name, value.handle, parent))
        completion = None
        if completion_parameter:
            result_value = (
                result.element if result and result.kind == "array" else result
            )
            result_owner = None
            if (
                result_value
                and result_value.handle
                and result_value.ownership == "owned"
            ):
                parent = next(
                    (
                        parameter.name
                        for parameter in inputs
                        if parameter.value.native == result_value.handle.parent
                    ),
                    None,
                )
                if result_value.handle.parent and parent is None:
                    raise ModelError(
                        [
                            f"{context}: owned completion requires its parent handle input"
                        ]
                    )
                result_owner = OwnedOutputPlan("result", result_value.handle, parent)
            completion = CompletionPlan(
                completion_parameter,
                tuple(owners),
                bool(result and result.ownership == "owned"),
                result_owner=result_owner,
            )
        consumes = metadata.get("consumes")
        for handle in self.handles.values():
            if function.name in {handle.release, handle.dispose}:
                consumes = handle.release_consumes
                if function.name == handle.dispose and function.name != handle.release:
                    support = DisposeSupport(handle)
                break
            if function.name in {handle.view_begin, handle.view_end}:
                support = ViewSupport(handle)
                break
        for parameter in inputs:
            if parameter.consumes:
                consumed = (
                    parameter.value.element
                    if parameter.value.kind == "reference"
                    else parameter.value
                )
                if consumed.kind != "handle" or parameter.name != receiver:
                    raise ModelError(
                        [
                            f"{context}: consumed input requires its managed handle receiver"
                        ]
                    )
                if consumes and consumes != parameter.consumes:
                    raise ModelError([f"{context}: conflicting consumption rules"])
                consumes = parameter.consumes
        if consumes and receiver is None:
            raise ModelError(
                [f"{context}: consumption requires a managed handle receiver"]
            )
        # A callback response's operations take the response record first.
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
        member = self.member(function, receiver or scoped_receiver)
        view = None
        if "view_owner" in metadata:
            parameter = next(
                p for p in function.parameters if p.name == metadata["view_owner"]
            )
            handle = self.handles[type_name(parameter.type)]
            invalidated_by = []
            current = handle
            while current:
                invalidated_by.extend(
                    operation
                    for operation in (current.release, current.dispose, current.abandon)
                    if operation
                )
                current = self.handles.get(current.parent) if current.parent else None
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
                tuple(dict.fromkeys(invalidated_by)),
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
            if registrations or "registration" in metadata:
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
        # A runtime export's caller reads its outputs itself, so only a
        # generated operation and a default copy what native writes.
        if function.name not in self.api.runtime_exports or isinstance(
            support, DefaultSupport
        ):
            self.returned |= self.copied(
                (
                    *outputs,
                    *(p for p in inputs if p.direction == "inout"),
                ),
                result,
                context,
                isinstance(support, DefaultSupport),
            )
        return OperationPlan(
            function,
            metadata["execution"],
            receiver,
            tuple(inputs),
            tuple(outputs),
            result,
            tuple(registrations),
            consumes,
            support,
            completion,
            tuple(owners),
            (
                DirectRegistrationPlan(
                    metadata["registration"],
                    metadata["user_data"],
                    metadata.get("release_callback"),
                    metadata.get("accepted_unless"),
                ),
            )
            if "registration" in metadata
            else (),
            view,
            scoped_receiver,
            next(
                (
                    parameter.metadata.get("handle_access", "live")
                    for parameter in function.parameters
                    if parameter.name == receiver
                ),
                "live",
            ),
            is_status(function.return_type),
            member,
            absence,
        )

    def copied(
        self,
        parameters,
        result: ValuePlan | None,
        context: str,
        default: bool = False,
    ) -> set[str]:
        """The records and unions that a binding copies from these values.

        A binding builds a callback registration from host callbacks and never
        adopts one from native. Only a record default holds one, with null
        callbacks, in a record field without presence. A copy of the default
        leaves a registration field unset, and the copy of a registration's own
        default copies only its other fields. A registration that is optional,
        referenced, or a union variant would need a copy of its presence, so
        the plan rejects one.
        """
        found: set[str] = set()

        def visit(
            value: ValuePlan, path: str, root: bool = False, field: bool = False
        ) -> None:
            if value.registration and not (default and root):
                if default and field:
                    return
                raise ModelError(
                    [
                        f"{path}: a record default holds a callback registration "
                        "only in a field without presence"
                        if default
                        else f"{path}: native cannot return a callback registration"
                    ]
                )
            if value.response:
                return
            if value.element:
                visit(value.element, path)
            if value.kind not in {"record", "union"} or value.native in found:
                return
            found.add(value.native)
            for member in value.fields:
                visit(
                    member.value,
                    f"{path}.{member.name}",
                    field=value.kind == "record" and member.presence is None,
                )

        for parameter in parameters:
            visit(parameter.value, f"{context} parameter {parameter.name}")
        if result is not None:
            visit(result, f"{context} result", root=True)
        return found

    def member(self, function: Function, receiver: str | None) -> str:
        """Name an operation once for every binding.

        An operation on a handle, or on a callback response, drops its
        receiver's prefix when its name starts with it; every other name drops
        `mln_`.
        """
        if receiver is not None:
            parameter = next(p for p in function.parameters if p.name == receiver)
            owner = type_name(parameter.type.pointee or parameter.type)
            prefix = self.handles[owner].prefix if owner in self.handles else owner
            if function.name.startswith(prefix + "_"):
                return function.name.removeprefix(prefix + "_")
        return public_stem(function.name)

    def response(self, name: str) -> CallbackResponsePlan:
        record = self.records[name]
        contexts = [
            f.name for f in record.fields if f.metadata.get("kind") == "context"
        ]
        if len(contexts) != 1:
            raise ModelError(
                [f"{record.location}: callback response requires one context field"]
            )
        callbacks = tuple(
            t.name
            for t in self.typedefs.values()
            if t.type.pointee
            and t.type.pointee.kind == "function"
            and any(
                p.type.pointee
                and type_name(p.type.pointee) == name
                and p.metadata.get("direction") in {"out", "inout"}
                for p in t.parameters
            )
        )
        methods = tuple(
            f.name
            for f in self.api.functions
            if f.parameters
            and f.parameters[0].type.pointee
            and type_name(f.parameters[0].type.pointee) == name
        )
        if not callbacks or not methods:
            raise ModelError(
                [f"{record.location}: callback response requires callbacks and methods"]
            )
        return CallbackResponsePlan(name, contexts[0], callbacks, methods)

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
                parameter.metadata.get("consumes"),
            )
            for parameter in typedef.parameters
        )
        # Native passes the arguments that a callback does not write.
        self.returned |= self.copied(
            tuple(p for p in parameters if p.direction != "out"), None, context
        )
        return CallbackPlan(
            name,
            parameters,
            self.value(
                function.result,
                {"enum": typedef.metadata["enum"]}
                if "enum" in typedef.metadata
                else {},
                context + " return",
            ),
            typedef.metadata["failure"],
            self.decision(typedef),
            contexts[0] if contexts else None,
            typedef.metadata.get("reentry", "allow"),
            self.callback_reentry(typedef),
            typedef.metadata.get("deferred"),
            is_status(function.result),
            typedef.metadata.get("synchronous") == "true",
        )

    def callback_reentry(self, typedef) -> CallbackReentryPlan | None:
        metadata = typedef.metadata
        if metadata.get("reentry") != "protocol":
            return None
        owner = metadata["reentry_owner"]
        if owner == "registration":
            function = next(
                f
                for f in self.api.functions
                if f.metadata.get("registration")
                and any(
                    p.name == f.metadata["registration"]
                    and p.type.declaration == typedef.name
                    for p in f.parameters
                )
            )
            type_ = function.parameters[0].type
        else:
            type_ = next(p.type for p in typedef.parameters if p.name == owner)
        type_ = type_.pointee or type_
        return CallbackReentryPlan(
            None if owner == "registration" else owner,
            type_name(type_),
            tuple(metadata["reentry_calls"].split(",")),
            owner == "registration",
        )

    def decision(self, typedef) -> DecisionPlan | None:
        metadata = typedef.metadata
        if "decision_handle" not in metadata:
            return None
        parameter = next(
            p for p in typedef.parameters if p.name == metadata["decision_handle"]
        )
        return DecisionPlan(
            parameter.name,
            self.handles[type_name(parameter.type)],
            metadata["decision_accept"],
            metadata["decision_pass"],
            metadata["complete"],
            metadata["cancelled"],
            metadata["cancel_registration"],
            metadata["wait_retired"],
        )

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
        for function in self.api.functions:
            if "context_type" in function.metadata:
                name = function.metadata["context_type"]
                type_ = (
                    self.typedefs[name].type
                    if name in self.typedefs
                    else CType("record", name, "struct " + name, name)
                )
                self.value(
                    CType("typedef", name, type_.canonical, name),
                    {},
                    f"{function.location}: adapter context",
                )
        if require_complete and diagnostics:
            raise ModelError(diagnostics)
        return BoundApi(
            self.api,
            tuple(
                operation
                for operation in operations
                if operation.name not in self.api.runtime_exports
                and not isinstance(operation.support, ViewSupport)
            ),
            self.values,
            self.callbacks,
            self.handles,
            tuple(sorted(set(diagnostics))),
            unsupported,
            tuple(
                operation
                for operation in operations
                if operation.name in self.api.runtime_exports
            ),
            tuple(
                CallbackAdapterPlan(
                    f.name,
                    f.metadata["callback_adapter"],
                    f.metadata["context_type"],
                )
                for f in self.api.functions
                if "callback_adapter" in f.metadata
            ),
            tuple(
                operation
                for operation in operations
                if operation.name not in self.api.runtime_exports
                and isinstance(operation.support, ViewSupport)
            ),
            frozenset(self.returned),
        )


def bind(api: Api, *, require_complete: bool = False) -> BoundApi:
    return Binder(api).bind(require_complete=require_complete)
