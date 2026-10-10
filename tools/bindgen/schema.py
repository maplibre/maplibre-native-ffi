"""Executable contracts shared by every binding emitter.

Metadata describes C semantics. Language backends choose syntax and report
unsupported shapes; they never invent ownership or erased completion types.
"""

from __future__ import annotations

import re
from dataclasses import replace

from .model import Api, CType, Function, ModelError
from .protocol import (
    BUFFER_VIEW,
    COMPLETION_RESULT,
    COMPLETION_VALUE_COUNT,
    COMPLETION_VALUE_SIZE,
    NOT_READY,
    STATUS,
    is_buffer_view,
    is_completion,
    is_status,
)

EXECUTIONS = frozenset(
    {
        "command",
        "query",
        "operation",
        "lifecycle",
        "immediate",
        "snapshot",
        "event_batch",
        "render_driver",
    }
)
COMPLETION_EXECUTIONS = frozenset({"command", "query", "operation", "lifecycle"})
ENCODINGS = frozenset({"utf8", "json", "bytes"})
# The values that a header annotation may write for each key. Conventions fill
# in most of the rest of a key's vocabulary, such as direction=in, shape=none,
# ownership=value, encoding=utf8, and reentry=allow, so a header never writes
# them; an absent nullable or handle_access means false or live. The semantic
# layer in semantic.py supplies the remaining values: lifetime=completion for an
# array completion result, and consumes=always for a release that returns void.
ANNOTATION_VALUES = {
    "execution": EXECUTIONS - {"immediate"},
    "shape": frozenset({"array"}),
    "ownership": frozenset({"borrowed", "owned"}),
    "encoding": frozenset({"json", "bytes"}),
    "direction": frozenset({"out", "inout"}),
    "nullable": frozenset({"true"}),
    "optional": frozenset({"empty"}),
    "lifetime": frozenset({"call", "owner", "process"}),
    "consumes": frozenset({"success"}),
    "reentry": frozenset({"forbid", "protocol"}),
    "release_reentry": frozenset({"forbid"}),
    "handle_access": frozenset({"issued"}),
    "synchronous": frozenset({"true"}),
}

# The same semantic vocabulary applies to all target languages. A field's
# optional=empty means that its empty representation denotes absence.
COMMON_KEYS = frozenset({"ownership", "encoding", "nullable", "optional", "lifetime"})
FUNCTION_KEYS = COMMON_KEYS | frozenset(
    {
        "execution",
        "result",
        "shape",
        "consumes",
        "kind",
        "length",
        "accepted_unless",
        "absent_on",
        "view_owner",
        "callback_adapter",
        "context_type",
        "enum",
    }
)
PARAMETER_KEYS = COMMON_KEYS | frozenset(
    {"direction", "length", "consumes", "enum", "kind", "handle_access"}
)
FIELD_KEYS = COMMON_KEYS | frozenset(
    {
        "length",
        "mask",
        "bit",
        "default",
        "tag",
        "enum",
        "direction",
        "variant",
        "empty_variant",
        "kind",
        "stride",
        "item_name",
        "item_buffer",
        "item_buffer_size",
        "item_offset",
        "item_length",
        "item_encoding",
    }
)
RECORD_KEYS = COMMON_KEYS | frozenset(
    {
        "kind",
        "default",
        "mask",
        "tag",
        "release",
        "release_reentry",
        "user_data",
        "fields",
    }
)
TYPEDEF_KEYS = COMMON_KEYS | frozenset(
    {
        "kind",
        "release",
        "release_reentry",
        "parent",
        "dispose",
        "view_begin",
        "view_end",
        "default",
        "user_data",
        "failure",
        "enum",
        "reentry",
        "reentry_calls",
        "reentry_owner",
        "abandon",
        "decision_handle",
        "decision_accept",
        "decision_pass",
        "complete",
        "cancelled",
        "cancel_registration",
        "wait_retired",
        "deferred",
        "synchronous",
        "prefix",
        "fields",
    }
)
ENUM_KEYS = frozenset({"kind"})
# Function keys that describe a returned value rather than the call.
VALUE_KEYS = COMMON_KEYS | frozenset({"kind", "length", "enum"})

# The roles that `kind` names at each declaration site.
KINDS = {
    "function": frozenset({"native_pointer"}),
    "parameter": frozenset({"context", "native_pointer"}),
    "field": frozenset(
        {
            "size",
            "reserved",
            "count",
            "presence_mask",
            "tag",
            "context",
            "native_pointer",
            "erased",
        }
    ),
    "record": frozenset({"callback_registration", "callback_response"}),
    "typedef": frozenset({"handle", "callback_registration", "callback_response"}),
    "enum": frozenset({"open", "bitmask"}),
}


# Integer ranges of the scalar results a deferred callback can answer with.
INTEGER_RANGES = {
    "_Bool": (0, 1),
    "bool": (0, 1),
    "signed char": (-(2**7), 2**7 - 1),
    "unsigned char": (0, 2**8 - 1),
    "short": (-(2**15), 2**15 - 1),
    "unsigned short": (0, 2**16 - 1),
    "int": (-(2**31), 2**31 - 1),
    "unsigned int": (0, 2**32 - 1),
    "long long": (-(2**63), 2**63 - 1),
    "unsigned long long": (0, 2**64 - 1),
}


# The integer ranges a field's `default=` may name. A `long` takes the narrower
# range of the targets where it is 32 bits.
DEFAULT_RANGES = {
    **INTEGER_RANGES,
    "long": INTEGER_RANGES["int"],
    "unsigned long": INTEGER_RANGES["unsigned int"],
}
INTEGER_LITERAL = re.compile(r"-?(0|[1-9][0-9]*)")
DECIMAL_LITERAL = re.compile(r"-?(0|[1-9][0-9]*)\.[0-9]+")
# The conventional `default` of each control role that has one.
CONTROL_DEFAULTS = {"size": "sizeof", "reserved": "0"}


def has_completion(function: Function) -> bool:
    return any(is_completion(parameter.type) for parameter in function.parameters)


def has_diagnostic(function: Function) -> bool:
    return function.diagnostic


# Conventions: the metadata that a declaration's C shape implies. A header
# annotates only what differs from these defaults, and the frontend fills them
# in, so every later stage reads a complete contract.

# A struct whose first member is `uint32_t size` is versioned: that member holds
# the struct's byte size, which a binding sets to sizeof the struct it writes.
# The name is the convention's only signal, since other structs begin with an
# unrelated uint32_t member.
STRUCT_SIZE_FIELD = "size"
# Pointers whose kind already says that the generator never reads through them.
OPAQUE_POINTER_KINDS = frozenset({"context", "native_pointer", "erased"})
TEXT_POINTEE_KINDS = frozenset({"char_s", "char_u"})


class Conventions:
    """Derive each declaration's default metadata from its C shape.

    Each rule reads the explicit metadata of the declaration it completes and
    never another declaration's name, so a renamed declaration keeps its
    contract.
    """

    def __init__(self, api: Api):
        self.api = api
        self.typedefs = api.typedefs_by_name
        self.records = api.records_by_name

    def resolve(self, type_: CType) -> CType:
        seen = set()
        while (
            type_.kind == "typedef"
            and type_.declaration in self.typedefs
            and type_.declaration not in seen
        ):
            seen.add(type_.declaration)
            type_ = self.typedefs[type_.declaration].type
        return type_

    def is_record(self, type_: CType) -> bool:
        record = self.records.get(self.resolve(type_).declaration or "")
        return record is not None and record.complete

    def value(
        self, type_: CType, explicit: dict[str, str], *, callback: bool = False
    ) -> dict[str, str]:
        """Defaults for a parameter, field, or return value of this C type.

        An untyped pointer is the context of the callback whose signature
        declares it, and elsewhere a native pointer that bindings pass through.
        A context outlives the call that passes it; every other value lasts for
        the call. A buffer view or character pointer holds UTF-8 text. A pointer
        borrows its target. A character pointer is NUL-terminated, and a pointer
        to a record addresses one record.
        """
        defaults = {}
        resolved = self.resolve(type_)
        pointee = (
            self.resolve(resolved.pointee)
            if resolved.kind == "pointer" and resolved.pointee
            else None
        )
        if pointee is not None and pointee.kind == "void":
            defaults["kind"] = "context" if callback else "native_pointer"
        kind = explicit.get("kind", defaults.get("kind"))
        defaults["lifetime"] = "owner" if kind == "context" else "call"
        if is_buffer_view(type_) or is_buffer_view(resolved):
            defaults["encoding"] = "utf8"
        if pointee is None:
            return defaults
        if pointee.kind == "function" and kind != "native_pointer":
            return defaults
        defaults["ownership"] = "borrowed"
        if kind in OPAQUE_POINTER_KINDS:
            return defaults
        if pointee.kind in TEXT_POINTEE_KINDS:
            defaults.update(length="nul", encoding="utf8")
        elif self.is_record(pointee):
            defaults["length"] = "1"
            if is_buffer_view(resolved.pointee) or is_buffer_view(pointee):
                defaults["encoding"] = "utf8"
        return defaults

    def parameter(
        self, type_: CType, explicit: dict[str, str], *, callback: bool = False
    ) -> dict[str, str]:
        """A parameter is an input; a handle that it outputs is the caller's."""
        defaults = {"direction": "in", **self.value(type_, explicit, callback=callback)}
        resolved = self.resolve(type_)
        handle = (
            self.typedefs.get(resolved.pointee.declaration or "")
            if resolved.kind == "pointer" and resolved.pointee
            else None
        )
        if (
            explicit.get("direction") == "out"
            and handle is not None
            and handle.metadata.get("kind") == "handle"
        ):
            defaults["ownership"] = "owned"
        return defaults

    def field(self, record, index: int, explicit: dict[str, str]) -> dict[str, str]:
        """Field roles follow from the struct layout and sibling references.

        A versioned struct's leading size member holds its byte size, a reserved
        member holds zero, and a member that a sibling's length, mask, or tag
        names is that sibling's count, presence mask, or union tag.
        """
        field = record.fields[index]
        defaults = {}
        if (
            index == 0
            and record.kind == "struct"
            and field.name == STRUCT_SIZE_FIELD
            and field.type.canonical == "unsigned int"
        ):
            defaults["kind"] = "size"
        for key, role in (
            ("length", "count"),
            ("mask", "presence_mask"),
            ("tag", "tag"),
        ):
            if any(item.metadata.get(key) == field.name for item in record.fields):
                defaults["kind"] = role
        kind = explicit.get("kind", defaults.get("kind"))
        if kind == "size":
            defaults["default"] = "sizeof"
        elif kind == "reserved":
            defaults["default"] = "0"
        return {**self.value(field.type, {**defaults, **explicit}), **defaults}

    def function(self, function: Function) -> dict[str, str]:
        """A function without a completion runs immediately, and a drain
        reports nothing queued as an absent output; a completion without a
        result delivers none, and a result is one borrowed value unless it is a
        handle, which the completion transfers."""
        explicit = function.metadata
        defaults = {}
        if not has_completion(function):
            defaults["execution"] = "immediate"
            if explicit.get("execution") == "event_batch":
                defaults["absent_on"] = NOT_READY
            if not is_status(function.return_type) and (
                function.return_type.kind != "void"
            ):
                defaults.update(self.value(function.return_type, explicit))
            return defaults
        result = explicit.get("result", "void")
        handle = self.typedefs.get(result)
        defaults.update(
            result="void",
            shape="none" if result == "void" else "value",
            ownership="value"
            if result == "void"
            else "owned"
            if handle and handle.metadata.get("kind") == "handle"
            else "borrowed",
        )
        if result == BUFFER_VIEW:
            defaults["encoding"] = "utf8"
        return defaults

    def typedef(self, typedef) -> dict[str, str]:
        """A callback allows reentry and contains a failure it cannot report; a
        handle has no parent, and its operations begin with its own name; a
        registration's user data is its context field; and a record's default
        value comes from the one function that takes nothing and returns it."""
        explicit = typedef.metadata
        defaults = {}
        signature = typedef.type.pointee if typedef.type.kind == "pointer" else None
        if signature is not None and signature.kind == "function":
            defaults["reentry"] = "allow"
            if signature.result is not None and signature.result.kind == "void":
                defaults["failure"] = "contain"
        if explicit.get("kind") == "handle":
            defaults.update(parent="none", prefix=typedef.name)
        record = self.records.get(typedef.name)
        if explicit.get("kind") == "callback_registration" and record:
            contexts = [
                field.name
                for field in record.fields
                if field.metadata.get("kind") == "context"
            ]
            if len(contexts) == 1:
                defaults["user_data"] = contexts[0]
        if self.is_record(typedef.type):
            constructors = [
                function.name
                for function in self.api.functions
                if not function.parameters
                and function.return_type.declaration == typedef.name
            ]
            if len(constructors) == 1:
                defaults["default"] = constructors[0]
        return defaults


def apply_defaults(api: Api) -> Api:
    """Complete every declaration's metadata with its conventional defaults.

    An explicit annotation that restates a default is an error, so the headers
    stay minimal and each annotation marks a real departure from convention. An
    annotation also writes only a value in ANNOTATION_VALUES. This step fills in
    a key's conventional values, and the semantic layer supplies the rest:
    lifetime=completion for an array completion result, and consumes=always for
    a release that returns void.
    """
    conventions = Conventions(api)
    errors: list[str] = []

    def complete(
        explicit: dict[str, str], defaults: dict[str, str], context: str
    ) -> dict[str, str]:
        for key, value in explicit.items():
            if defaults.get(key) == value:
                errors.append(f"{context}: {key}={value} restates the default")
            elif value not in ANNOTATION_VALUES.get(key, (value,)):
                errors.append(f"{context}: unsupported {key}={value!r}")
        return dict(sorted({**defaults, **explicit}.items()))

    functions = []
    for function in api.functions:
        context = f"{function.location}: {function.name}"
        functions.append(
            replace(
                function,
                metadata=complete(
                    function.metadata, conventions.function(function), context
                ),
                parameters=tuple(
                    replace(
                        parameter,
                        metadata=complete(
                            parameter.metadata,
                            conventions.parameter(parameter.type, parameter.metadata),
                            f"{context} parameter {parameter.name}",
                        ),
                    )
                    for parameter in function.parameters
                ),
            )
        )
    records = tuple(
        replace(
            record,
            fields=tuple(
                replace(
                    field,
                    metadata=complete(
                        field.metadata,
                        conventions.field(record, index, field.metadata),
                        f"{field.location}: {record.name}.{field.name}",
                    ),
                )
                for index, field in enumerate(record.fields)
            ),
        )
        for record in api.records
    )
    typedefs = tuple(
        replace(
            typedef,
            metadata=complete(
                typedef.metadata,
                conventions.typedef(typedef),
                f"{typedef.location}: {typedef.name}",
            ),
            parameters=tuple(
                replace(
                    parameter,
                    metadata=complete(
                        parameter.metadata,
                        conventions.parameter(
                            parameter.type, parameter.metadata, callback=True
                        ),
                        f"{typedef.location}: {typedef.name} callback parameter "
                        f"{parameter.name}",
                    ),
                )
                for parameter in typedef.parameters
            ),
        )
        for typedef in api.typedefs
    )
    for enum in api.enums:
        if enum.metadata.get("kind") == "open":
            errors.append(
                f"{enum.location}: {enum.name}: kind=open restates the default"
            )
    if errors:
        raise ModelError(errors)
    return replace(api, functions=tuple(functions), records=records, typedefs=typedefs)


def metadata_errors(
    metadata: dict[str, str],
    allowed: frozenset[str],
    context: str,
    kinds: frozenset[str] = frozenset(),
) -> list[str]:
    errors = [
        f"{context}: unknown metadata key {key!r}" for key in metadata.keys() - allowed
    ]
    if "kind" in metadata and metadata["kind"] not in kinds:
        errors.append(f"{context}: unsupported kind={metadata['kind']!r}")
    if metadata.get("nullable") == "true" and "optional" in metadata:
        errors.append(
            f"{context}: nullable and optional specify different absence representations"
        )
    return errors


def value_metadata_errors(
    type_: CType, metadata: dict[str, str], context: str
) -> list[str]:
    errors = []
    is_buffer = is_buffer_view(type_)
    is_text_pointer = bool(type_.pointee and type_.pointee.kind in {"char_s", "char_u"})
    is_pointer = type_.kind == "pointer"
    if "encoding" in metadata and not (is_buffer or is_text_pointer or is_pointer):
        errors.append(
            f"{context}: encoding requires a buffer view or character pointer"
        )
    if metadata.get("optional") == "empty" and not is_buffer:
        errors.append(f"{context}: optional=empty requires a buffer view")
    if "length" in metadata and type_.kind not in {"pointer", "array"}:
        errors.append(f"{context}: length requires a pointer or array")
    return errors


def validate(api: Api) -> None:
    """Reject incomplete or contradictory execution and result contracts.

    Emitters separately account for every function that their language supports.
    A valid common contract is necessary but does not imply that an emitter can
    safely translate a pointer, callback, tagged union, or foreign GPU object.
    """
    errors: list[str] = []
    enum_constants = {
        value.name: (enum.name, value.value)
        for enum in api.enums
        for value in enum.values
    }

    def field_path(record_name: str, path: str):
        record = api.records_by_name.get(record_name)
        found = None
        for component in path.split("."):
            if record is None:
                return None
            found = next(
                (item for item in record.fields if item.name == component), None
            )
            if found is None:
                return None
            record = api.records_by_name.get(found.type.declaration or "")
        return found

    known_types = {item.name for item in (*api.records, *api.enums, *api.typedefs)} | {
        "void",
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
        "size_t",
        "int8_t",
        "uint8_t",
        "int16_t",
        "uint16_t",
        "int32_t",
        "uint32_t",
        "int64_t",
        "uint64_t",
        "intptr_t",
        "uintptr_t",
    }
    for function in api.functions:
        context = f"{function.location}: {function.name}"
        metadata = function.metadata
        errors.extend(
            metadata_errors(metadata, FUNCTION_KEYS, context, KINDS["function"])
        )
        if "view_owner" in metadata:
            owner = next(
                (
                    parameter
                    for parameter in function.parameters
                    if parameter.name == metadata["view_owner"]
                ),
                None,
            )
            handle = (
                api.typedefs_by_name.get(owner.type.declaration or "")
                if owner
                else None
            )
            if handle is None or handle.metadata.get("kind") != "handle":
                errors.append(
                    f"{context}: view_owner requires an input handle parameter"
                )
            elif not {"view_begin", "view_end"} <= handle.metadata.keys():
                errors.append(
                    f"{context}: view_owner requires a handle that declares view_begin and view_end"
                )
            if owner and owner.metadata.get("direction", "in") != "in":
                errors.append(f"{context}: view_owner must remain owned by the caller")
            views = [
                parameter
                for parameter in function.parameters
                if parameter.metadata.get("direction") == "out"
            ]
            if len(views) != 1 or not (
                views[0].type.pointee
                and api.records_by_name.get(views[0].type.pointee.declaration or "")
            ):
                errors.append(
                    f"{context}: view_owner requires one borrowed output record"
                )
        if "enum" in metadata and metadata["enum"] not in {
            enum.name for enum in api.enums
        }:
            errors.append(f"{context}: return enum names an absent enum")
        if "callback_adapter" in metadata:
            callback = api.typedefs_by_name.get(metadata["callback_adapter"])
            signature = callback.type.pointee if callback else None
            if (
                signature is None
                or signature.kind != "function"
                or signature.result is None
                or signature.result.canonical != function.return_type.canonical
                or tuple(p.canonical for p in signature.parameters)
                != tuple(p.type.canonical for p in function.parameters)
            ):
                errors.append(
                    f"{context}: callback adapter signature differs from its callback"
                )
            if metadata.get("context_type") not in known_types:
                errors.append(
                    f"{context}: callback adapter requires a known context type"
                )
        elif "context_type" in metadata:
            errors.append(f"{context}: adapter relationships require callback_adapter")
        execution = metadata.get("execution")
        if execution is None:
            errors.append(f"{context}: missing execution metadata")
        returns_status = is_status(function.return_type)
        if function.diagnostic != (
            returns_status and "callback_adapter" not in metadata
        ):
            errors.append(
                f"{context}: a status-returning function other than a callback "
                "implementation takes a final mln_diagnostic* parameter, and no other "
                "function does"
            )
        completion = has_completion(function)
        if completion:
            if not is_status(function.return_type):
                errors.append(
                    f"{context}: completion submission requires mln_status acceptance gating"
                )
            completion_parameters = [
                parameter
                for parameter in function.parameters
                if is_completion(parameter.type)
            ]
            if len(completion_parameters) != 1 or not is_completion(
                function.parameters[-1].type
            ):
                errors.append(
                    f"{context}: submission requires exactly one final completion parameter"
                )
            if execution not in COMPLETION_EXECUTIONS:
                errors.append(
                    f"{context}: completion requires command, query, operation, or lifecycle execution"
                )
            result = metadata.get("result")
            shape = metadata.get("shape")
            if result and result not in known_types:
                errors.append(f"{context}: unknown result type {result!r}")
            record = api.records_by_name.get(result or "")
            if record is not None and not record.complete:
                errors.append(
                    f"{context}: completion value requires a complete record definition"
                )
            if result == "void" and shape != "none":
                errors.append(f"{context}: void completion requires shape=none")
            if execution == "command" and result != "void":
                errors.append(
                    f"{context}: commands use disposition and generation, not a value payload"
                )
            if execution == "query" and result == "void":
                errors.append(f"{context}: query requires a value payload")
            ownership = metadata.get("ownership")
            if result == "void" and ownership != "value":
                errors.append(f"{context}: void completion requires ownership=value")
            if ownership == "owned":
                handle = api.typedefs_by_name.get(result or "")
                if handle is None or handle.metadata.get("kind") != "handle":
                    errors.append(
                        f"{context}: owned completion requires a declared handle type"
                    )
            if "encoding" in metadata and result != BUFFER_VIEW:
                errors.append(f"{context}: encoded completion requires a buffer view")
            if metadata.get("optional") == "empty" and result != BUFFER_VIEW:
                errors.append(
                    f"{context}: optional=empty requires a buffer view result"
                )
        else:
            if execution in COMPLETION_EXECUTIONS:
                errors.append(
                    f"{context}: {execution} execution requires a completion parameter"
                )
            if "result" in metadata or "shape" in metadata:
                errors.append(f"{context}: result and shape describe a completion")
            if is_status(function.return_type) or (function.return_type.kind == "void"):
                extra = sorted(metadata.keys() & VALUE_KEYS)
                if extra:
                    errors.append(
                        f"{context}: a function without a return value takes no "
                        f"value metadata ({', '.join(extra)})"
                    )
        if "absent_on" in metadata:
            errors.extend(absence_errors(api, function, context))
        if function.variadic:
            errors.append(
                f"{context}: variadic public functions cannot be generated safely"
            )
        parameters = {parameter.name: parameter for parameter in function.parameters}
        if "accepted_unless" in metadata:
            errors.extend(acceptance_errors(api, function, context))
        for parameter in function.parameters:
            pcontext = f"{context} parameter {parameter.name}"
            errors.extend(
                metadata_errors(
                    parameter.metadata, PARAMETER_KEYS, pcontext, KINDS["parameter"]
                )
            )
            errors.extend(
                value_metadata_errors(parameter.type, parameter.metadata, pcontext)
            )
            if "handle_access" in parameter.metadata:
                owner = api.typedefs_by_name.get(parameter.type.declaration or "")
                if (
                    owner is None
                    or owner.metadata.get("kind") != "handle"
                    or parameter.metadata.get("direction", "in") != "in"
                    or parameter.metadata.get("consumes")
                    or metadata.get("consumes")
                ):
                    errors.append(
                        f"{pcontext}: handle_access requires a nonconsuming input handle"
                    )
            if "enum" in parameter.metadata and parameter.metadata["enum"] not in {
                enum.name for enum in api.enums
            }:
                errors.append(f"{pcontext}: enum names an absent enum")
            if parameter.metadata.get("direction") in ("out", "inout"):
                if parameter.type.kind != "pointer":
                    errors.append(f"{pcontext}: output direction requires a pointer")
                elif parameter.type.pointee and parameter.type.pointee.const:
                    errors.append(
                        f"{pcontext}: output direction contradicts const pointee"
                    )
            if (
                "length" in parameter.metadata
                and parameter.metadata["length"] not in parameters
                and parameter.metadata["length"] != "nul"
                and parameter.metadata["length"] != "1"
            ):
                errors.append(f"{pcontext}: length names an absent parameter")
    for record in api.records:
        context = f"{record.location}: {record.name}"
        errors.extend(
            metadata_errors(record.metadata, RECORD_KEYS, context, KINDS["record"])
        )
        if "fields" in record.metadata:
            errors.extend(ordered_errors(record, record.metadata["fields"], context))
        # A presence bit guards one member, so a value that several fields
        # describe together is a record embedded under that bit. An array's
        # count takes the array's presence and names no bit of its own.
        counts = {
            field.metadata["length"]
            for field in record.fields
            if "length" in field.metadata
        }
        guarded = set()
        for field in record.fields:
            if field.name in counts and (
                "mask" in field.metadata or "bit" in field.metadata
            ):
                errors.append(
                    f"{field.location}: {record.name}.{field.name}: a count takes "
                    "its array's presence; drop mask and bit"
                )
                continue
            if "mask" not in field.metadata or "bit" not in field.metadata:
                continue
            presence = (field.metadata["mask"], field.metadata["bit"])
            if presence in guarded:
                errors.append(
                    f"{field.location}: {record.name}.{field.name}: presence bit "
                    f"{presence[1]} guards more than one member; embed a record"
                )
            guarded.add(presence)
        for field in record.fields:
            fcontext = f"{field.location}: {record.name}.{field.name}"
            errors.extend(
                metadata_errors(field.metadata, FIELD_KEYS, fcontext, KINDS["field"])
            )
            errors.extend(value_metadata_errors(field.type, field.metadata, fcontext))
            # Native can write a value that an older binding's enum lacks.
            enum_name = field.type.canonical.removeprefix("enum ")
            if enum_name != field.type.canonical and enum_name.isidentifier():
                errors.append(
                    f"{fcontext}: an enum member requires an integer type and enum=<enum>"
                )
            if "stride" in field.metadata:
                stride = field_path(record.name, field.metadata["stride"])
                if (
                    field.type.kind != "pointer"
                    or "length" not in field.metadata
                    or stride is None
                    or stride.type.canonical
                    not in {"unsigned int", "unsigned long", "unsigned long long"}
                ):
                    errors.append(
                        f"{fcontext}: stride requires an array pointer and unsigned byte count"
                    )
            item_keys = {
                "item_name",
                "item_buffer",
                "item_buffer_size",
                "item_offset",
                "item_length",
                "item_encoding",
            }
            if item_keys & field.metadata.keys():
                if not item_keys <= field.metadata.keys():
                    errors.append(
                        f"{fcontext}: item buffer slice requires all relationships"
                    )
                else:
                    child = field.type.pointee
                    data = field_path(record.name, field.metadata["item_buffer"])
                    size = field_path(record.name, field.metadata["item_buffer_size"])
                    offset = (
                        field_path(
                            child.declaration or "", field.metadata["item_offset"]
                        )
                        if child
                        else None
                    )
                    length = (
                        field_path(
                            child.declaration or "", field.metadata["item_length"]
                        )
                        if child
                        else None
                    )
                    if (
                        field.type.kind != "pointer"
                        or "length" not in field.metadata
                        or not all((data, size, offset, length))
                    ):
                        errors.append(
                            f"{fcontext}: item buffer slice names absent storage or item fields"
                        )
                    elif (
                        data.type.kind != "pointer"
                        or data.metadata.get("length")
                        != field.metadata["item_buffer_size"]
                        or field.metadata["item_encoding"] not in ENCODINGS
                    ):
                        errors.append(
                            f"{fcontext}: item buffer slice requires a counted byte arena and encoding"
                        )
            if "enum" in field.metadata and field.metadata["enum"] not in {
                enum.name for enum in api.enums
            }:
                errors.append(f"{fcontext}: enum names an absent enum")
            if "." in field.metadata.get("mask", ""):
                errors.append(f"{fcontext}: mask must name a sibling field")
            if ("mask" in field.metadata) != ("bit" in field.metadata):
                errors.append(f"{fcontext}: presence requires both mask and bit")
            if "bit" in field.metadata:
                constant = enum_constants.get(field.metadata["bit"])
                if (
                    constant is None
                    or constant[1] <= 0
                    or constant[1] & (constant[1] - 1)
                ):
                    errors.append(
                        f"{fcontext}: bit requires one declared positive flag"
                    )
                mask = field_path(record.name, field.metadata.get("mask", ""))
                if (
                    mask
                    and constant
                    and mask.metadata.get("enum") not in {None, constant[0]}
                ):
                    errors.append(f"{fcontext}: bit belongs to a different mask enum")
            if (
                "variant" in field.metadata
                and field.metadata["variant"] not in enum_constants
            ):
                errors.append(f"{fcontext}: variant requires a declared enum value")
            if "variant" in field.metadata and record.kind != "union":
                errors.append(f"{fcontext}: variant requires a union member")
            if "empty_variant" in field.metadata and "tag" not in field.metadata:
                errors.append(f"{fcontext}: empty_variant requires a tagged union")
            if "tag" in field.metadata:
                tag = field_path(record.name, field.metadata["tag"])
                union = api.records_by_name.get(field.type.declaration or "")
                domain = tag.metadata.get("enum", tag.type.declaration) if tag else None
                variants = (
                    [
                        enum_constants.get(member.metadata.get("variant", ""))
                        for member in union.fields
                    ]
                    if union and union.kind == "union"
                    else []
                )
                if "empty_variant" in field.metadata:
                    empty = enum_constants.get(field.metadata["empty_variant"])
                    if empty is None or empty[0] != domain or empty in variants:
                        errors.append(
                            f"{fcontext}: empty_variant requires an unused value from the tag enum"
                        )
                if not variants or any(
                    variant is None or variant[0] != domain for variant in variants
                ):
                    errors.append(
                        f"{fcontext}: tagged union variants must belong to the tag enum"
                    )
                elif len({variant[1] for variant in variants if variant}) != len(
                    variants
                ):
                    errors.append(
                        f"{fcontext}: tagged union variants must have distinct values"
                    )
            if (
                field.metadata.get("kind") == "size"
                and field.type.canonical != "unsigned int"
            ):
                errors.append(f"{fcontext}: struct size requires a uint32_t field")
            if field.metadata.get("ownership") == "owned":
                handle = api.typedefs_by_name.get(field.type.declaration or "")
                if handle is None or handle.metadata.get("kind") != "handle":
                    errors.append(
                        f"{fcontext}: owned field requires a declared handle release contract"
                    )
            for key in ("length", "mask", "tag"):
                if (
                    key in field.metadata
                    and field_path(record.name, field.metadata[key]) is None
                    and not (key == "length" and field.metadata[key] in {"nul", "1"})
                ):
                    errors.append(f"{fcontext}: {key} names an absent field")
    for typedef in api.typedefs:
        context = f"{typedef.location}: {typedef.name}"
        errors.extend(
            metadata_errors(typedef.metadata, TYPEDEF_KEYS, context, KINDS["typedef"])
        )
        if "fields" in typedef.metadata:
            errors.extend(
                ordered_errors(
                    api.records_by_name.get(typedef.name),
                    typedef.metadata["fields"],
                    context,
                )
            )
        if "prefix" in typedef.metadata and typedef.metadata.get("kind") != "handle":
            errors.append(f"{context}: prefix names a handle's operations")
        for parameter in typedef.parameters:
            pcontext = f"{context} callback parameter {parameter.name}"
            errors.extend(
                metadata_errors(
                    parameter.metadata, PARAMETER_KEYS, pcontext, KINDS["parameter"]
                )
            )
            errors.extend(
                value_metadata_errors(parameter.type, parameter.metadata, pcontext)
            )
        if typedef.metadata.get("kind") == "callback_registration":
            record = api.records_by_name.get(typedef.name)
            fields = {field.name: field for field in record.fields} if record else {}
            for key in ("release", "user_data"):
                if typedef.metadata.get(key) not in fields:
                    errors.append(
                        f"{context}: registration {key} requires a descriptor field"
                    )
            release_field = fields.get(typedef.metadata.get("release", ""))
            release_type = (
                api.typedefs_by_name.get(release_field.type.declaration or "")
                if release_field
                else None
            )
            if release_type is None:
                errors.append(
                    f"{context}: registration release requires a callback typedef"
                )
            else:
                callback = release_type.type.pointee
                if (
                    callback is None
                    or callback.result is None
                    or callback.result.kind != "void"
                    or len(callback.parameters) != 1
                    or callback.parameters[0].spelling != "void *"
                ):
                    errors.append(
                        f"{context}: registration release must take one void pointer and return void"
                    )
            user_data = fields.get(typedef.metadata.get("user_data", ""))
            if user_data is not None and (
                user_data.type.spelling != "void *"
                or user_data.metadata.get("kind") != "context"
            ):
                errors.append(
                    f"{context}: registration user_data requires a void context pointer"
                )
        elif "release_reentry" in typedef.metadata:
            errors.append(
                f"{context}: release_reentry requires a callback registration"
            )
        default = typedef.metadata.get("default")
        if default:
            constructor = api.functions_by_name.get(default)
            if (
                constructor is None
                or constructor.parameters
                or constructor.return_type.declaration != typedef.name
            ):
                errors.append(
                    f"{context}: default requires a no-argument constructor for this type"
                )
        if typedef.metadata.get("kind") == "handle":
            for key in ("release", "parent"):
                if key not in typedef.metadata:
                    errors.append(f"{context}: handle requires {key} metadata")
            parent = typedef.metadata.get("parent", "none")
            if parent != "none" and (
                parent not in api.typedefs_by_name
                or api.typedefs_by_name[parent].metadata.get("kind") != "handle"
            ):
                errors.append(f"{context}: parent requires a declared handle")
            if "view_begin" in typedef.metadata or "view_end" in typedef.metadata:
                begin = api.functions_by_name.get(
                    typedef.metadata.get("view_begin", "")
                )
                end = api.functions_by_name.get(typedef.metadata.get("view_end", ""))
                if (
                    begin is None
                    or end is None
                    or len(begin.parameters) != 2
                    or len(end.parameters) != 1
                    or begin.parameters[0].type.declaration != typedef.name
                    or begin.parameters[1].type.canonical != "void **"
                    or begin.parameters[1].metadata.get("direction") != "out"
                    or not is_status(begin.return_type)
                    or end.parameters[0].type.canonical != "void *"
                    or end.return_type.kind != "void"
                ):
                    errors.append(
                        f"{context}: view scope requires handle/token begin and token end operations"
                    )
            for key in ("release", "dispose", "abandon"):
                operation = api.functions_by_name.get(typedef.metadata.get(key, ""))
                if key not in typedef.metadata:
                    continue
                if operation is None or not operation.parameters:
                    errors.append(f"{context}: {key} requires a handle operation")
                    continue
                receiver = operation.parameters[0].type
                if receiver.kind == "pointer" and receiver.pointee:
                    receiver = receiver.pointee
                if receiver.declaration != typedef.name:
                    errors.append(f"{context}: {key} receiver must be this handle type")
                if (
                    not is_status(operation.return_type)
                    and operation.return_type.kind != "void"
                ):
                    errors.append(f"{context}: {key} must return status or void")
                if key in {"dispose", "abandon"} and has_completion(operation):
                    errors.append(f"{context}: {key} must have synchronous acceptance")
        if typedef.metadata.get("reentry") == "protocol":
            owner = reentry_owner(api, typedef)
            names = tuple(
                filter(None, typedef.metadata.get("reentry_calls", "").split(","))
            )
            if owner is None or not names:
                errors.append(
                    f"{context}: protocol reentry requires an owner parameter or handle and operations"
                )
            for name in names:
                operation = api.functions_by_name.get(name)
                receiver = (
                    operation.parameters[0].type
                    if operation and operation.parameters
                    else None
                )
                receiver = receiver.pointee or receiver if receiver else None
                if (
                    receiver is None
                    or owner is None
                    or receiver.declaration != owner.declaration
                    or operation.metadata.get("execution") != "immediate"
                ):
                    errors.append(
                        f"{context}: reentry operation must be an immediate operation on the declared owner"
                    )
        elif "reentry_owner" in typedef.metadata or "reentry_calls" in typedef.metadata:
            errors.append(
                f"{context}: reentry owner and operations require protocol mode"
            )
        signature = typedef.type.pointee
        if signature is not None and signature.kind == "function":
            failure = typedef.metadata.get("failure")
            if signature.result is not None and signature.result.kind == "void":
                if failure != "contain":
                    errors.append(f"{context}: a void callback contains its failure")
            elif failure is None or failure == "contain":
                errors.append(
                    f"{context}: a callback with a result declares the failure it returns"
                )
            else:
                errors.extend(
                    callback_result_errors(
                        typedef, failure, "failure", enum_constants, context
                    )
                )
        elif "failure" in typedef.metadata:
            errors.append(f"{context}: failure requires a callback")
        if "deferred" in typedef.metadata:
            errors.extend(deferred_errors(typedef, enum_constants, context))
        if "synchronous" in typedef.metadata and (
            signature is None or signature.kind != "function"
        ):
            errors.append(f"{context}: synchronous requires a callback")
        if (
            typedef.metadata.get("synchronous") == "true"
            and "deferred" in typedef.metadata
        ):
            errors.append(f"{context}: a synchronous callback cannot be deferred")
        decision_keys = {
            "decision_handle",
            "decision_accept",
            "decision_pass",
            "complete",
            "cancelled",
            "cancel_registration",
            "wait_retired",
        }
        if decision_keys & typedef.metadata.keys():
            if not decision_keys <= typedef.metadata.keys():
                errors.append(
                    f"{context}: decision requires handle, both outcomes, completion, cancellation, registration, and retirement wait"
                )
                continue
            parameter = next(
                (
                    p
                    for p in typedef.parameters
                    if p.name == typedef.metadata["decision_handle"]
                ),
                None,
            )
            handle = (
                api.typedefs_by_name.get(parameter.type.declaration or "")
                if parameter
                else None
            )
            if handle is None or handle.metadata.get("kind") != "handle":
                errors.append(
                    f"{context}: decision_handle requires a callback handle parameter"
                )
                continue
            outcomes = [
                enum_constants.get(typedef.metadata[key])
                for key in ("decision_accept", "decision_pass")
            ]
            if (
                any(value is None for value in outcomes)
                or outcomes[0][0] != outcomes[1][0]
                or outcomes[0][1] == outcomes[1][1]
            ):
                errors.append(
                    f"{context}: decision outcomes require distinct values of one enum"
                )
            for key in ("complete", "cancelled", "cancel_registration"):
                operation = api.functions_by_name.get(typedef.metadata[key])
                if (
                    operation is None
                    or not operation.parameters
                    or operation.parameters[0].type.declaration != handle.name
                    or not is_status(operation.return_type)
                    or has_completion(operation)
                ):
                    errors.append(
                        f"{context}: {key} requires an immediate status operation on the decision handle"
                    )
            wait = api.functions_by_name.get(typedef.metadata["wait_retired"])
            if (
                wait is None
                or len(wait.parameters) != 1
                or wait.parameters[0].type.declaration != handle.name
                or wait.parameters[0].metadata.get("handle_access") != "issued"
                or wait.parameters[0].metadata.get("direction", "in") != "in"
                or wait.parameters[0].metadata.get("consumes") is not None
                or wait.metadata.get("consumes") is not None
                or wait.metadata.get("execution") != "immediate"
                or not is_status(wait.return_type)
                or has_completion(wait)
            ):
                errors.append(
                    f"{context}: wait_retired requires a nonconsuming immediate status operation on one issued decision handle"
                )
            if typedef.metadata["wait_retired"] in typedef.metadata.get(
                "reentry_calls", ""
            ).split(","):
                errors.append(
                    f"{context}: retirement waits cannot be callback reentry operations"
                )
    for typedef in api.typedefs:
        if typedef.metadata.get("kind") != "handle":
            continue
        seen = {typedef.name}
        parent = typedef.metadata.get("parent", "none")
        while parent in api.typedefs_by_name:
            if parent in seen:
                errors.append(
                    f"{typedef.location}: {typedef.name}: handle parent graph contains a cycle"
                )
                break
            seen.add(parent)
            parent = api.typedefs_by_name[parent].metadata.get("parent", "none")
    for enum in api.enums:
        errors.extend(
            metadata_errors(
                enum.metadata, ENUM_KEYS, f"{enum.location}: {enum.name}", KINDS["enum"]
            )
        )
    errors.extend(reference_errors(api))
    errors.extend(field_default_errors(api))
    errors.extend(versioning_errors(api))
    if errors:
        raise ModelError(errors)


# How the public interface reaches a record. A record that a caller passes by
# pointer, that native passes to a callback by pointer, or that a function takes
# or returns by value versions itself. An embedded record is versioned by its
# container, and a completion value or a strided element by its stride.
BY_POINTER = "pointer"
EMBEDDED = "embedded"
DELIVERED = "delivered"


def record_reaches(api: Api) -> tuple[dict[str, set[str]], set[str]]:
    """How the public interface reaches each record, and which it takes in.

    The first map gives each record the ways the public functions and callbacks
    reach it. A record default function is not a reach, since it only
    initializes a record that something else takes. The set holds the records
    that native reads from the host: an input parameter, an output that a
    callback fills for native, and every record that one of those holds.

    A borrowed array inside a record takes its reach from that record. Inside
    a record that versions itself or that native reads, the array's elements
    version themselves too. Inside a record that native only delivers or
    embeds, the elements are delivered when the array carries a stride, and
    are otherwise indexed by the binding's own size, which freezes them as an
    embedded record is frozen.
    """
    conventions = Conventions(api)
    reaches: dict[str, set[str]] = {}
    inputs: set[str] = set()
    internal = set(api.runtime_types)
    defaults = {
        typedef.metadata["default"]
        for typedef in api.typedefs
        if "default" in typedef.metadata
    }

    def record_of(type_: CType) -> str | None:
        name = conventions.resolve(type_).declaration
        return name if name in api.records_by_name else None

    def reach(
        type_: CType,
        how: str,
        metadata: dict[str, str],
        incoming: bool,
        unstrided: str = BY_POINTER,
    ):
        resolved = conventions.resolve(type_)
        if resolved.kind == "pointer" and resolved.pointee is not None:
            if metadata.get("kind") in OPAQUE_POINTER_KINDS:
                return
            how = DELIVERED if "stride" in metadata else unstrided
            resolved = conventions.resolve(resolved.pointee)
        name = record_of(resolved)
        if name is None:
            return
        reaches.setdefault(name, set()).add(how)
        if incoming:
            take(name)

    def take(name: str):
        if name in inputs:
            return
        inputs.add(name)
        for field in api.records_by_name[name].fields:
            reach(field.type, EMBEDDED, field.metadata, True)

    for function in api.public_functions:
        for parameter in function.parameters:
            if is_completion(parameter.type):
                continue
            reach(
                parameter.type,
                BY_POINTER,
                parameter.metadata,
                parameter.metadata.get("direction", "in") != "out",
            )
        if function.name not in defaults and function.return_type.kind != "void":
            reach(function.return_type, BY_POINTER, {}, False)
        result = function.metadata.get("result")
        if result and result != "void" and result in api.records_by_name:
            reaches.setdefault(result, set()).add(DELIVERED)
    for typedef in api.typedefs:
        if typedef.name in internal:
            continue
        for parameter in typedef.parameters:
            reach(
                parameter.type,
                BY_POINTER,
                parameter.metadata,
                parameter.metadata.get("direction", "in") == "out",
            )
    # A record's members reach further records only as the record itself is
    # reached, so this repeats until a pass adds nothing. Reaches only grow,
    # and a record that a later pass finds versioning itself also passes that
    # to its arrays.
    while True:
        before = sum(len(how) for how in reaches.values())
        for record in api.records:
            if record.name in internal or record.name not in reaches:
                continue
            versions_itself = (
                BY_POINTER in reaches[record.name] or record.name in inputs
            )
            for field in record.fields:
                reach(
                    field.type,
                    EMBEDDED,
                    field.metadata,
                    False,
                    BY_POINTER if versions_itself else EMBEDDED,
                )
        if sum(len(how) for how in reaches.values()) == before:
            return reaches, inputs


def versioning_errors(api: Api) -> list[str]:
    """Check that each struct is versioned by exactly one thing.

    A struct that a caller passes by pointer, or that native passes to a
    callback by pointer, carries its own size. A struct that the interface only
    embeds by value is versioned by its container, and a completion value or a
    strided element by the stride that native delivers with it, so neither
    carries a size. A record default initializes a record that native reads, so
    an output-only record has none. Every binding steps through an array result
    by the stride that the completion result reports beside its count.
    """
    reaches, inputs = record_reaches(api)
    errors = []
    completion = api.records_by_name.get(COMPLETION_RESULT)
    for field in completion.fields if completion else ():
        if field.metadata.get("kind") == "erased" and (
            field.metadata.get("length"),
            field.metadata.get("stride"),
        ) != (COMPLETION_VALUE_COUNT, COMPLETION_VALUE_SIZE):
            errors.append(
                f"{field.location}: {COMPLETION_RESULT}.{field.name}: requires "
                f"length={COMPLETION_VALUE_COUNT};stride={COMPLETION_VALUE_SIZE}"
            )
    for record in api.records:
        how = reaches.get(record.name)
        if (
            how
            and BY_POINTER not in how
            and any(field.metadata.get("kind") == "size" for field in record.fields)
        ):
            errors.append(
                f"{record.location}: {record.name}: struct versioned by its "
                "container or stride must not carry size"
            )
    for typedef in api.typedefs:
        if (
            "default" in typedef.metadata
            and typedef.name in api.records_by_name
            and typedef.name in reaches
            and typedef.name not in inputs
        ):
            errors.append(
                f"{typedef.location}: {typedef.name}: default requires a record "
                "that native reads"
            )
    return errors


def defaulted_records(api: Api) -> set[str]:
    """The structs whose fields the generated default cases check.

    These are the structs that a record default function returns, and the
    structs nested by value in one through a required member. The cases skip
    optional members, unions, buffer views, and arrays, so a struct reached
    only through one of those has no case to check its defaults.
    """
    conventions = Conventions(api)
    pending = [
        typedef.name
        for typedef in api.typedefs
        if typedef.metadata.get("default") and typedef.name in api.records_by_name
    ]
    found: set[str] = set()
    while pending:
        name = pending.pop()
        record = api.records_by_name.get(name)
        if name in found or record is None or record.kind != "struct":
            continue
        found.add(name)
        for field in record.fields:
            if "mask" in field.metadata or is_buffer_view(field.type):
                continue
            # An array, a union, or a union's integer tag resolves to no
            # struct, so the loop skips it.
            pending.append(conventions.resolve(field.type).declaration or "")
    return found


def field_default_errors(api: Api) -> list[str]:
    """Check each field's `default=`: its value in its record's native default.

    A control role keeps its conventional value. Any other field may state a
    nonzero literal of its own type, or an enumerator of its enum, when it is
    a plain value that a default function's result holds. A generated C test
    checks the value against the default function.
    """
    conventions = Conventions(api)
    defaulted = defaulted_records(api)
    enums = {enum.name: enum for enum in api.enums}
    errors = []
    for record in api.records:
        for field in record.fields:
            literal = field.metadata.get("default")
            if literal is None:
                continue
            context = f"{field.location}: {record.name}.{field.name}"
            kind = field.metadata.get("kind")
            if kind in CONTROL_DEFAULTS:
                if literal != CONTROL_DEFAULTS[kind]:
                    errors.append(f"{context}: a {kind} member's default is fixed")
                continue
            resolved = conventions.resolve(field.type)
            if (
                kind is not None
                or field.metadata.keys() & {"mask", "tag", "variant"}
                or resolved.kind in {"pointer", "array"}
                or conventions.is_record(resolved)
            ):
                errors.append(f"{context}: default requires a plain value member")
                continue
            if record.name not in defaulted:
                errors.append(
                    f"{context}: default requires a record that a default "
                    "function returns"
                )
                continue
            enum = enums.get(
                field.metadata.get("enum", field.type.declaration or "")
            ) or enums.get(resolved.declaration or "")
            canonical = resolved.canonical.removeprefix("const ")
            if enum is not None:
                values = {value.name: value.value for value in enum.values}
                if literal not in values:
                    errors.append(f"{context}: default names no {enum.name} value")
                    continue
                zero = values[literal] == 0
            elif canonical in {"bool", "_Bool"}:
                if literal not in {"true", "false"}:
                    errors.append(f"{context}: default requires true")
                    continue
                zero = literal == "false"
            elif canonical in {"float", "double"}:
                if not DECIMAL_LITERAL.fullmatch(literal):
                    errors.append(f"{context}: default requires a decimal with a point")
                    continue
                zero = float(literal) == 0
            elif canonical in DEFAULT_RANGES:
                low, high = DEFAULT_RANGES[canonical]
                if not INTEGER_LITERAL.fullmatch(literal) or not (
                    low <= int(literal) <= high
                ):
                    errors.append(f"{context}: default requires a decimal {canonical}")
                    continue
                zero = int(literal) == 0
            else:
                errors.append(f"{context}: default requires a scalar or enum")
                continue
            if zero:
                errors.append(f"{context}: default={literal} restates zero")
    return errors


# Keys whose values name other declarations, by the kind of declaration named.
# A list value separates its names with commas.
FUNCTION_REFERENCES = {
    "function": (),
    "typedef": (
        "release",
        "dispose",
        "abandon",
        "view_begin",
        "view_end",
        "default",
        "complete",
        "cancelled",
        "cancel_registration",
        "wait_retired",
        "reentry_calls",
    ),
}
PARAMETER_REFERENCES = {
    "function": ("view_owner", "accepted_unless"),
    "typedef": ("decision_handle", "reentry_owner"),
}


def reference_errors(api: Api) -> list[str]:
    """Reject an annotation that names a function or parameter that is absent.

    Later checks give each relationship its meaning; this one makes a misspelled
    or renamed declaration fail with the name that no longer resolves.
    """
    errors = []
    functions = api.functions_by_name

    def check(site: str, declaration, context: str) -> None:
        metadata = declaration.metadata
        for key in FUNCTION_REFERENCES[site]:
            if key == "release" and metadata.get("kind") == "callback_registration":
                continue  # A registration's release names its descriptor field.
            for name in filter(None, metadata.get(key, "").split(",")):
                if name not in functions:
                    errors.append(f"{context}: {key} names an absent function {name!r}")
        parameters = {parameter.name for parameter in declaration.parameters}
        for key in PARAMETER_REFERENCES[site]:
            name = metadata.get(key)
            if key == "reentry_owner" and is_handle(api, name):
                continue  # A callback without an owner parameter names its handle.
            if name is not None and name not in parameters:
                errors.append(f"{context}: {key} names an absent parameter {name!r}")

    for function in api.functions:
        check("function", function, f"{function.location}: {function.name}")
    for typedef in api.typedefs:
        check("typedef", typedef, f"{typedef.location}: {typedef.name}")
    return errors


def is_handle(api: Api, name: str | None) -> bool:
    typedef = api.typedefs_by_name.get(name or "")
    return typedef is not None and typedef.metadata.get("kind") == "handle"


def reentry_owner(api: Api, typedef):
    """The type whose operations a protocol callback may call: the type of its
    `reentry_owner` parameter, or the handle that the key names when the
    callback has no parameter that identifies its owner."""
    name = typedef.metadata.get("reentry_owner")
    owner = next(
        (p.type.pointee or p.type for p in typedef.parameters if p.name == name),
        None,
    )
    if owner is None and is_handle(api, name):
        owner = api.typedefs_by_name[name].type
        owner = CType("typedef", name, owner.canonical, name)
    return owner


def registration_parameters(api: Api, function: Function) -> list:
    """The parameters that pass a callback registration record by pointer."""
    result = []
    for parameter in function.parameters:
        pointee = parameter.type.pointee
        typedef = api.typedefs_by_name.get(
            (pointee.declaration if pointee else None) or ""
        )
        if typedef and typedef.metadata.get("kind") == "callback_registration":
            result.append(parameter)
    return result


def acceptance_errors(api: Api, function: Function, context: str) -> list[str]:
    """Check `accepted_unless`: a boolean output that, when set on success,
    reports that native kept nothing from the call's one registration."""
    errors = []
    condition = next(
        (
            p
            for p in function.parameters
            if p.name == function.metadata["accepted_unless"]
        ),
        None,
    )
    if (
        condition is None
        or condition.metadata.get("direction") != "out"
        or condition.type.pointee is None
        or condition.type.pointee.kind != "bool"
    ):
        errors.append(f"{context}: accepted_unless requires a boolean output")
    if len(registration_parameters(api, function)) != 1:
        errors.append(f"{context}: accepted_unless requires one registration parameter")
    if has_completion(function) or not is_status(function.return_type):
        errors.append(
            f"{context}: accepted_unless requires a status return without a completion"
        )
    return errors


def absence_errors(api: Api, function: Function, context: str) -> list[str]:
    """Check `absent_on`: a failure status that reports the one output absent.

    A binding returns its language's empty form for that status instead of an
    error, so the function must report its outcome synchronously through a
    status and publish exactly one output.
    """
    errors = []
    if not is_status(function.return_type) or has_completion(function):
        errors.append(
            f"{context}: absent_on requires a status return without a completion"
        )
    directions = [
        parameter.metadata.get("direction", "in") for parameter in function.parameters
    ]
    if directions.count("out") != 1 or "inout" in directions:
        errors.append(f"{context}: absent_on requires exactly one output")
    statuses = {
        value.name: value.value
        for enum in api.enums
        if enum.name == STATUS
        for value in enum.values
    }
    if statuses.get(function.metadata["absent_on"], 0) == 0:
        errors.append(f"{context}: absent_on requires a failure enumerator of {STATUS}")
    return errors


def ordered_errors(record, value: str, context: str) -> list[str]:
    """Check a `fields=ordered` record: a struct of plain values only.

    Its fields are the whole value in declaration order, so none may be
    control state, a pointer, or an array.
    """
    if value != "ordered":
        return [f"{context}: unsupported fields={value!r}"]
    if (
        record is None
        or record.kind != "struct"
        or not record.complete
        or any(
            "kind" in field.metadata or field.type.kind in {"pointer", "array"}
            for field in record.fields
        )
    ):
        return [f"{context}: fields=ordered requires a struct of plain values"]
    return []


def callback_result_errors(
    typedef, value: str, key: str, enum_constants, context: str
) -> list[str]:
    """Check that a callback can return `value` as its declared result."""
    signature = typedef.type.pointee
    enum = typedef.metadata.get("enum")
    result = signature.result
    if enum is None and result is not None:
        named = result.declaration or ""
        enum = (
            named
            if any(owner == named for owner, _ in enum_constants.values())
            else None
        )
    if enum:
        if enum_constants.get(value, (None,))[0] != enum:
            return [f"{context}: {key} must name a value of {enum}"]
        return []
    bounds = (
        INTEGER_RANGES.get(result.canonical.removeprefix("const ")) if result else None
    )
    try:
        number = int(value, 0)
    except ValueError:
        number = None
    if bounds is None or number is None or not bounds[0] <= number <= bounds[1]:
        return [f"{context}: {key} must be an integer the callback result can hold"]
    return []


def deferred_errors(typedef, enum_constants, context: str) -> list[str]:
    """Check that an adapter can answer a callback early and copy its call.

    A deferred callback returns its declared value at once and delivers a copy
    of its arguments later, so the value must be a result the callback can
    return and every argument must outlive the call only as a copy.
    """
    value = typedef.metadata["deferred"]
    signature = typedef.type.pointee
    if (
        signature is None
        or signature.kind != "function"
        or signature.result is None
        or signature.result.kind == "void"
    ):
        return [f"{context}: deferred requires a callback with a result"]
    errors = callback_result_errors(typedef, value, "deferred", enum_constants, context)
    contexts = [p for p in typedef.parameters if p.metadata.get("kind") == "context"]
    if len(contexts) != 1:
        errors.append(f"{context}: deferred requires one context parameter")
    for parameter in typedef.parameters:
        if parameter in contexts:
            continue
        if (
            parameter.metadata.get("direction", "in") != "in"
            or "consumes" in parameter.metadata
        ):
            errors.append(
                f"{context}: deferred argument {parameter.name} must be a copyable input"
            )
        if parameter.type.kind == "pointer" and parameter.metadata.get(
            "lifetime"
        ) not in {None, "call"}:
            errors.append(
                f"{context}: deferred argument {parameter.name} must be borrowed for the call"
            )
    handle = typedef.metadata.get("decision_handle")
    if handle and typedef.metadata.get("decision_accept") != value:
        # A handle is only the adapter's to keep after an accepting answer.
        errors.append(
            f"{context}: deferred decision handle {handle} requires the accept value"
        )
    return errors
