"""Executable contracts shared by every binding emitter.

Metadata describes C semantics. Language backends choose syntax and report
unsupported shapes; they never invent ownership or erased completion types.
"""

from __future__ import annotations

from dataclasses import replace

from .model import Api, CType, Function, ModelError
from .protocol import (
    BUFFER_VIEW,
    COMPLETION,
    COMPLETION_RESULT,
    COMPLETION_VALUE,
    COMPLETION_VALUE_COUNT,
    COMPLETION_VALUE_SIZE,
    DELIVERY_PAYLOAD,
    DELIVERY_QUEUE,
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
    }
)
COMPLETION_EXECUTIONS = frozenset({"command", "query", "operation", "lifecycle"})
ENCODINGS = frozenset({"utf8", "json", "bytes"})
# The values that a header annotation may write for each key. Conventions fill
# in most of the rest of a key's vocabulary, such as direction=in, shape=none,
# ownership=value, encoding=utf8, so a header never writes
# them; an absent nullable means false. The semantic layer supplies
# lifetime=completion for an array completion result and discardable
# completions for commands.
ANNOTATION_VALUES = {
    "execution": EXECUTIONS - {"immediate"},
    "shape": frozenset({"array"}),
    "ownership": frozenset({"borrowed", "owned"}),
    "encoding": frozenset({"json", "bytes"}),
    "direction": frozenset({"out"}),
    "nullable": frozenset({"true"}),
    "optional": frozenset({"empty"}),
    "lifetime": frozenset({"call", "owner", "process"}),
    "synchronous": frozenset({"true"}),
    "capture": frozenset({"manual"}),
    "blocks": frozenset({"thread_start"}),
}

# The same semantic vocabulary applies to all target languages. A field's
# optional=empty means that its empty representation denotes absence.
COMMON_KEYS = frozenset({"ownership", "encoding", "nullable", "optional", "lifetime"})
FUNCTION_KEYS = COMMON_KEYS | frozenset(
    {
        "execution",
        "result",
        "shape",
        "kind",
        "length",
        "accepted_unless",
        "absent_on",
        "view_owner",
        "enum",
        "blocks",
    }
)
PARAMETER_KEYS = COMMON_KEYS | frozenset({"direction", "length", "enum", "kind"})
FIELD_KEYS = COMMON_KEYS | frozenset(
    {
        "length",
        "mask",
        "bit",
        "tag",
        "enum",
        "direction",
        "variant",
        "empty_variant",
        "kind",
        "stride",
    }
)
TYPEDEF_KEYS = COMMON_KEYS | frozenset(
    {
        "capture",
        "kind",
        "release",
        "parent",
        "dispose",
        "view_begin",
        "view_end",
        "default_function",
        "user_data",
        "failure",
        "enum",
        "borrowed_owner",
        "abandon",
        "decision_handle",
        "decision_accept",
        "delivery",
        "queue_result",
        "synchronous",
    }
)
ENUM_KEYS = frozenset({"kind"})
VALUE_KEYS = COMMON_KEYS | frozenset({"kind", "length", "enum"})

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
            "delivery_queue",
        }
    ),
    "typedef": frozenset({"handle", "callback_registration", "callback_response"}),
    "enum": frozenset({"bitmask"}),
}


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


def has_completion(function: Function) -> bool:
    return any(is_completion(parameter.type) for parameter in function.parameters)


# Conventions: the metadata that a declaration's C shape implies. A header
# annotates only what differs from these defaults, and the frontend fills them
# in, so every later stage reads a complete contract.

# A struct whose first member is `uint32_t size` is versioned: that member holds
# the struct's byte size, which a binding sets to sizeof the struct it writes.
# The name is the convention's only signal, since other structs begin with an
# unrelated uint32_t member.
STRUCT_SIZE_FIELD = "size"
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
        if record.name == COMPLETION_RESULT and field.name == COMPLETION_VALUE:
            defaults.update(
                kind="erased",
                length=COMPLETION_VALUE_COUNT,
                stride=COMPLETION_VALUE_SIZE,
            )
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
        """A callback contains a failure it cannot report; a
        handle has no parent, and its operations begin with its own name; a
        registration's user data is its context field; and a record's default
        value comes from the one function that takes nothing and returns it."""
        explicit = typedef.metadata
        defaults = {}
        signature = typedef.type.pointee if typedef.type.kind == "pointer" else None
        if (
            signature is not None
            and signature.kind == "function"
            and signature.result is not None
            and signature.result.kind == "void"
        ):
            defaults["failure"] = "contain"
        if explicit.get("kind") == "handle":
            defaults.update(parent="none")
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
                defaults["default_function"] = constructors[0]
        return defaults


def apply_defaults(api: Api) -> Api:
    """Complete every declaration's metadata with its conventional defaults.

    An explicit annotation that restates a default is an error, so the headers
    stay minimal and each annotation marks a real departure from convention. An
    annotation also writes only a value in ANNOTATION_VALUES. This step fills in
    a key's conventional values, and the semantic layer supplies the rest:
    lifetime=completion for an array completion result, and consumption for
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
        execution = metadata.get("execution")
        returns_status = is_status(function.return_type)
        if function.diagnostic != returns_status:
            errors.append(
                f"{context}: a status-returning function takes a final "
                "mln_diagnostic* parameter, and no other function does"
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
            for parameter in completion_parameters:
                if "nullable" in parameter.metadata:
                    errors.append(
                        f"{context}: completion nullability follows execution; "
                        "nullable is not allowed on a completion parameter"
                    )
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
            if result and result not in known_types:
                errors.append(f"{context}: unknown result type {result!r}")
            record = api.records_by_name.get(result or "")
            if record is not None and not record.complete:
                errors.append(
                    f"{context}: completion value requires a complete record definition"
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
            if is_status(function.return_type) or (function.return_type.kind == "void"):
                extra = sorted(metadata.keys() & VALUE_KEYS)
                if extra:
                    errors.append(
                        f"{context}: a function without a return value takes no "
                        f"value metadata ({', '.join(extra)})"
                    )
        if "blocks" in metadata and (
            execution not in {None, "immediate"} or not returns_status
        ):
            errors.append(
                f"{context}: blocks requires an immediate function that returns a status"
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
            if "enum" in parameter.metadata and parameter.metadata["enum"] not in {
                enum.name for enum in api.enums
            }:
                errors.append(f"{pcontext}: enum names an absent enum")
            if parameter.metadata.get("direction") == "out":
                if parameter.type.kind != "pointer":
                    errors.append(f"{pcontext}: output direction requires a pointer")
                elif (
                    parameter.type.pointee
                    and parameter.type.pointee.kind != "pointer"
                    and parameter.metadata.get("length") not in {None, "1"}
                ):
                    errors.append(
                        f"{pcontext}: output arrays require an owned pointer output"
                    )
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
            if field.metadata.get("kind") == "delivery_queue" and (
                api.typedefs_by_name.get(record.name) is None
                or api.typedefs_by_name[record.name].metadata.get("kind")
                != "callback_registration"
            ):
                errors.append(
                    f"{fcontext}: kind=delivery_queue requires a callback registration"
                )
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
            if "enum" in field.metadata and field.metadata["enum"] not in {
                enum.name for enum in api.enums
            }:
                errors.append(f"{fcontext}: enum names an absent enum")
            if "." in field.metadata.get("mask", ""):
                errors.append(f"{fcontext}: mask must name a sibling field")
            if ("mask" in field.metadata) != ("bit" in field.metadata):
                errors.append(f"{fcontext}: presence requires both mask and bit")
            if (
                "variant" in field.metadata
                and field.metadata["variant"] not in enum_constants
            ):
                errors.append(f"{fcontext}: variant requires a declared enum value")
            if "bit" in field.metadata and field.metadata["bit"] not in enum_constants:
                errors.append(f"{fcontext}: bit names an absent constant")
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
            errors.extend(delivery_queue_errors(api, typedef, record, context))
        if typedef.metadata.get("kind") == "handle":
            if "release" not in typedef.metadata:
                errors.append(f"{context}: handle requires release metadata")
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
            if "release" in typedef.metadata:
                operation = api.functions_by_name.get(typedef.metadata["release"])
                if operation is None or not operation.parameters:
                    errors.append(f"{context}: release requires a handle operation")
                else:
                    receiver = operation.parameters[0].type
                    if receiver.kind == "pointer" and receiver.pointee:
                        receiver = receiver.pointee
                    if receiver.declaration != typedef.name:
                        errors.append(
                            f"{context}: release receiver must be this handle type"
                        )
                    if (
                        not is_status(operation.return_type)
                        and operation.return_type.kind != "void"
                    ):
                        errors.append(f"{context}: release must return status or void")
        if "borrowed_owner" in typedef.metadata:
            parameter = next(
                (
                    p
                    for p in typedef.parameters
                    if p.name == typedef.metadata["borrowed_owner"]
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
                    f"{context}: borrowed_owner requires a callback handle parameter"
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
        if "delivery" in typedef.metadata:
            errors.extend(delivery_errors(api, typedef, enum_constants, context))
        errors.extend(queue_result_errors(typedef, enum_constants, context))
        if "synchronous" in typedef.metadata and (
            signature is None or signature.kind != "function"
        ):
            errors.append(f"{context}: synchronous requires a callback")
        if (
            typedef.metadata.get("synchronous") == "true"
            and "delivery" in typedef.metadata
        ):
            errors.append(
                f"{context}: a synchronous callback cannot be delivered through a queue"
            )
        decision_keys = {"decision_handle", "decision_accept"}
        if decision_keys & typedef.metadata.keys():
            if not decision_keys <= typedef.metadata.keys():
                errors.append(
                    f"{context}: decision requires a handle and an accepted outcome"
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
            if enum_constants.get(typedef.metadata["decision_accept"], (None,))[
                0
            ] != typedef.metadata.get("enum"):
                errors.append(
                    f"{context}: decision_accept must name a value of the callback enum"
                )
    for enum in api.enums:
        errors.extend(
            metadata_errors(
                enum.metadata, ENUM_KEYS, f"{enum.location}: {enum.name}", KINDS["enum"]
            )
        )
    errors.extend(reference_errors(api))
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
        typedef.metadata["default_function"]
        for typedef in api.typedefs
        if "default_function" in typedef.metadata
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
            "default_function" in typedef.metadata
            and typedef.name in api.records_by_name
            and typedef.name in reaches
            and typedef.name not in inputs
        ):
            errors.append(
                f"{typedef.location}: {typedef.name}: default requires a record "
                "that native reads"
            )
    return errors


FUNCTION_REFERENCES = {
    "function": (),
    "typedef": (
        "release",
        "dispose",
        "abandon",
        "view_begin",
        "view_end",
        "default_function",
    ),
}
PARAMETER_REFERENCES = {
    "function": ("view_owner", "accepted_unless"),
    "typedef": ("decision_handle", "borrowed_owner"),
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
            if name is not None and name not in parameters:
                errors.append(f"{context}: {key} names an absent parameter {name!r}")

    for function in api.functions:
        check("function", function, f"{function.location}: {function.name}")
    for typedef in api.typedefs:
        check("typedef", typedef, f"{typedef.location}: {typedef.name}")
    return errors


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
    if directions.count("out") != 1:
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
        if enum_constants.get(value, (None,))[0] == enum:
            return []
        try:
            int(value, 0)
        except ValueError:
            return [f"{context}: {key} must name a value of {enum} or an integer"]
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


def delivery_queue_errors(api: Api, typedef, record, context: str) -> list[str]:
    """Check a registration's queue field.

    A registration may name one delivery queue in place of its callback, and
    native code then queues what the callback would receive. Only the protocol
    completion and a callback that declares its delivery have such a copy.
    """
    if record is None:
        return []
    queues = [f for f in record.fields if f.metadata.get("kind") == "delivery_queue"]
    if not queues:
        return []
    errors = []
    if len(queues) > 1:
        errors.append(f"{context}: a registration takes at most one delivery queue")
    for field in queues:
        if field.type.declaration != DELIVERY_QUEUE:
            errors.append(
                f"{context}: {field.name}: kind=delivery_queue requires {DELIVERY_QUEUE}"
            )
    if record.name == COMPLETION:
        return errors
    release = typedef.metadata.get("release")
    for field in record.fields:
        callback = api.typedefs_by_name.get(field.type.declaration or "")
        signature = callback.type.pointee if callback else None
        if (
            field.name == release
            or signature is None
            or signature.kind != "function"
            or "delivery" in callback.metadata
        ):
            continue
        errors.append(
            f"{context}: a queue registration's callback {field.name} requires delivery="
        )
    return errors


def queue_result_errors(typedef, enum_constants, context: str) -> list[str]:
    """Check the result that native answers for a callback in queue mode.

    A queue-mode registration answers each call without its callback. A
    decision callback answers its accepted outcome; any other delivered
    callback with a result names the answer in `queue_result=`, which a host
    that serves the queue form through a callback returns.
    """
    metadata = typedef.metadata
    signature = typedef.type.pointee
    returns = (
        signature is not None
        and signature.kind == "function"
        and signature.result is not None
        and signature.result.kind != "void"
        and not is_status(signature.result)
    )
    needed = returns and "delivery" in metadata and "decision_accept" not in metadata
    if "queue_result" not in metadata:
        return (
            [f"{context}: a delivered callback with a result declares its queue_result"]
            if needed
            else []
        )
    if not needed:
        return [
            (
                f"{context}: queue_result requires a delivered callback with a "
                "result and no decision"
            )
        ]
    return callback_result_errors(
        typedef, metadata["queue_result"], "queue_result", enum_constants, context
    )


def delivery_errors(api: Api, typedef, enum_constants, context: str) -> list[str]:
    """Check that a queue can carry a copy of a callback's arguments.

    `delivery=` names the mln_delivery_type constant whose payload variant
    carries the copy. The variant's fields mirror the callback's arguments
    other than its context, in order, by name and C type: a pointer argument
    becomes a pointer the delivery owns, and a handle stays the same handle.
    """
    value = typedef.metadata["delivery"]
    signature = typedef.type.pointee
    if signature is None or signature.kind != "function":
        return [f"{context}: delivery requires a callback"]
    owner = enum_constants.get(value, (None,))[0]
    if owner is None:
        return [f"{context}: delivery names an unknown constant {value}"]
    payload = api.records_by_name.get(DELIVERY_PAYLOAD)
    variant = next(
        (
            field
            for field in (payload.fields if payload else ())
            if field.metadata.get("variant") == value
        ),
        None,
    )
    if variant is None:
        return [f"{context}: delivery {value} names no {DELIVERY_PAYLOAD} variant"]
    record = api.records_by_name.get(variant.type.declaration or "")
    if record is None:
        return [f"{context}: delivery {value} requires a record variant"]
    parameters = [p for p in typedef.parameters if p.metadata.get("kind") != "context"]
    mirrored = len(parameters) == len(record.fields) and all(
        parameter.name == field.name
        and parameter.type.canonical == field.type.canonical
        and (
            parameter.type.kind != "pointer"
            or field.metadata.get("lifetime") == "owner"
        )
        for parameter, field in zip(parameters, record.fields, strict=False)
    )
    if not mirrored:
        errors = [
            f"{context}: delivery record {record.name} must mirror the callback's arguments"
        ]
        return errors
    return []
