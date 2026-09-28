"""Executable contracts shared by every binding emitter.

Metadata describes C semantics. Language backends choose syntax and report
unsupported shapes; they never invent ownership or erased completion types.
"""

from __future__ import annotations

from .model import Api, CType, Function, ModelError

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
SHAPES = frozenset({"none", "value", "array", "bytes"})
OWNERSHIPS = frozenset({"value", "borrowed", "owned"})
ENCODINGS = frozenset({"utf8", "json", "bytes"})

# The same semantic vocabulary applies to all target languages. A field's
# optional=empty means that its empty representation denotes absence.
COMMON_KEYS = frozenset({"ownership", "encoding", "nullable", "optional", "lifetime"})
FUNCTION_KEYS = COMMON_KEYS | frozenset(
    {
        "execution",
        "result",
        "shape",
        "receiver",
        "consumes",
        "name",
        "support",
        "kind",
        "length",
        "registration",
        "user_data",
        "release_callback",
        "owner_release",
        "accepted_unless",
        "view_owner",
        "callback_adapter",
        "context_type",
        "invokes",
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
        "group_type",
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
    {"kind", "default", "mask", "tag", "release", "user_data", "projection"}
)
TYPEDEF_KEYS = COMMON_KEYS | frozenset(
    {
        "kind",
        "release",
        "parent",
        "dispose",
        "dispose_invalidates",
        "view_begin",
        "view_end",
        "release_consumes",
        "default",
        "user_data",
        "thread",
        "failure",
        "enum",
        "reentry",
        "reentry_calls",
        "reentry_owner",
        "abandon",
        "projection",
        "decision_handle",
        "decision_accept",
        "decision_pass",
        "complete",
        "cancelled",
        "cancel_registration",
        "wait_retired",
    }
)
ENUM_KEYS = frozenset({"kind"})


def is_completion(type_: CType) -> bool:
    return bool(
        type_.kind == "pointer"
        and type_.pointee
        and type_.pointee.const
        and type_.pointee.declaration == "mln_completion"
    )


def has_completion(function: Function) -> bool:
    return any(is_completion(parameter.type) for parameter in function.parameters)


def metadata_errors(
    metadata: dict[str, str], allowed: frozenset[str], context: str
) -> list[str]:
    errors = [
        f"{context}: unknown metadata key {key!r}" for key in metadata.keys() - allowed
    ]
    values = {
        "execution": EXECUTIONS,
        "shape": SHAPES,
        "ownership": OWNERSHIPS,
        "encoding": ENCODINGS,
        "direction": frozenset({"in", "out", "inout"}),
        "nullable": frozenset({"true", "false"}),
        "optional": frozenset({"empty", "null"}),
        "lifetime": frozenset({"call", "completion", "owner", "process"}),
        "release_consumes": frozenset({"success", "always"}),
        "consumes": frozenset({"success", "always"}),
        "thread": frozenset({"native", "host"}),
        "reentry": frozenset({"allow", "forbid", "protocol"}),
        "handle_access": frozenset({"live", "issued"}),
        "dispose_invalidates": frozenset({"self", "parent"}),
    }
    for key, accepted in values.items():
        if key in metadata and metadata[key] not in accepted:
            errors.append(f"{context}: unsupported {key}={metadata[key]!r}")
    if metadata.get("nullable") == "true" and "optional" in metadata:
        errors.append(
            f"{context}: nullable and optional specify different absence representations"
        )
    return errors


def value_metadata_errors(
    type_: CType, metadata: dict[str, str], context: str
) -> list[str]:
    errors = []
    is_buffer = (
        type_.declaration == "mln_buffer_view"
        or type_.canonical == "struct mln_buffer_view"
    )
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
        errors.extend(metadata_errors(metadata, FUNCTION_KEYS, context))
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
            if owner and owner.metadata.get("direction", "in") != "in":
                errors.append(f"{context}: view_owner must remain owned by the caller")
            if not any(
                parameter.metadata.get("direction") == "out"
                for parameter in function.parameters
            ):
                errors.append(f"{context}: view_owner requires a borrowed output view")
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
            scoped_records = (
                {
                    p.type.pointee.declaration
                    for p in callback.parameters
                    if p.type.pointee
                    and p.metadata.get("direction") in {"out", "inout"}
                }
                if callback
                else set()
            )
            allowed = {
                f.name
                for f in api.functions
                if f.parameters
                and f.parameters[0].type.pointee
                and f.parameters[0].type.pointee.declaration in scoped_records
            }
            if callback:
                allowed.update(
                    callback.metadata.get(key)
                    for key in ("complete", "cancelled", "cancel_registration")
                )
                decision_parameter = next(
                    (
                        p
                        for p in callback.parameters
                        if p.name == callback.metadata.get("decision_handle")
                    ),
                    None,
                )
                decision_type = (
                    api.typedefs_by_name.get(decision_parameter.type.declaration or "")
                    if decision_parameter
                    else None
                )
                if decision_type:
                    allowed.add(decision_type.metadata.get("release"))
            for invoked in metadata.get("invokes", "").split(","):
                if invoked and invoked not in allowed:
                    errors.append(
                        f"{context}: callback adapter invokes a function outside its callback protocol"
                    )
        elif "context_type" in metadata or "invokes" in metadata:
            errors.append(f"{context}: adapter relationships require callback_adapter")
        execution = metadata.get("execution")
        if execution is None:
            errors.append(f"{context}: missing execution metadata")
        completion = has_completion(function)
        if completion:
            if function.return_type.declaration != "mln_status":
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
            for key in ("result", "shape", "ownership"):
                if key not in metadata:
                    errors.append(
                        f"{context}: erased completion payload requires {key} metadata"
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
            elif result and result != "void" and shape == "none":
                errors.append(f"{context}: non-void completion requires a value shape")
            if execution == "command" and result != "void":
                errors.append(
                    f"{context}: commands use disposition and generation, not a value payload"
                )
            if execution == "query" and result == "void":
                errors.append(f"{context}: query requires a value payload")
            ownership = metadata.get("ownership")
            if result == "void" and ownership != "value":
                errors.append(f"{context}: void completion requires ownership=value")
            elif result != "void" and ownership == "value":
                errors.append(
                    f"{context}: completion storage requires borrowed or owned ownership"
                )
            if ownership == "owned":
                handle = api.typedefs_by_name.get(result or "")
                if handle is None or handle.metadata.get("kind") != "handle":
                    errors.append(
                        f"{context}: owned completion requires a declared handle type"
                    )
            if (
                "encoding" in metadata
                and result != "mln_buffer_view"
                and shape != "bytes"
            ):
                errors.append(
                    f"{context}: encoded completion requires a buffer view or byte shape"
                )
            if metadata.get("optional") == "empty" and result != "mln_buffer_view":
                errors.append(
                    f"{context}: optional=empty requires a buffer view result"
                )
        elif execution in COMPLETION_EXECUTIONS:
            errors.append(
                f"{context}: {execution} execution requires a completion parameter"
            )
        if function.variadic:
            errors.append(
                f"{context}: variadic public functions cannot be generated safely"
            )
        parameters = {parameter.name: parameter for parameter in function.parameters}
        if "support" in metadata:
            role, separator, owner = metadata["support"].partition(":")
            typedef = api.typedefs_by_name.get(owner)
            if (
                not separator
                or role != "default"
                or typedef is None
                or typedef.metadata.get("default") != function.name
            ):
                errors.append(
                    f"{context}: support requires a checked default-constructor consumer"
                )
        if (
            metadata.keys()
            & {"user_data", "release_callback", "owner_release", "accepted_unless"}
            and "registration" not in metadata
        ):
            errors.append(
                f"{context}: registration lifetime metadata requires a callback registration"
            )
        if "registration" in metadata:
            callback = parameters.get(metadata["registration"])
            callback_type = (
                api.typedefs_by_name.get(callback.type.declaration or "")
                if callback
                else None
            )
            signature = callback_type.type.pointee if callback_type else None
            if signature is None or signature.kind != "function":
                errors.append(f"{context}: registration requires a callback parameter")
            user_data = parameters.get(metadata.get("user_data", ""))
            if (
                user_data is None
                or user_data.type.spelling != "void *"
                or user_data.metadata.get("kind") != "context"
            ):
                errors.append(
                    f"{context}: registration user_data requires a void context parameter"
                )
            if ("release_callback" in metadata) == ("owner_release" in metadata):
                errors.append(
                    f"{context}: registration requires exactly one callback or owner release"
                )
            if "release_callback" in metadata:
                release = parameters.get(metadata["release_callback"])
                release_type = (
                    api.typedefs_by_name.get(release.type.declaration or "")
                    if release
                    else None
                )
                signature = release_type.type.pointee if release_type else None
                if (
                    signature is None
                    or signature.result is None
                    or signature.result.kind != "void"
                    or len(signature.parameters) != 1
                    or signature.parameters[0].spelling != "void *"
                ):
                    errors.append(
                        f"{context}: release_callback requires a void callback taking its context"
                    )
            if "owner_release" in metadata:
                owner = (
                    api.typedefs_by_name.get(
                        function.parameters[0].type.declaration or ""
                    )
                    if function.parameters
                    else None
                )
                if (
                    owner is None
                    or owner.metadata.get("release") != metadata["owner_release"]
                ):
                    errors.append(
                        f"{context}: owner_release must match the receiver handle release"
                    )
            if "accepted_unless" in metadata:
                condition = parameters.get(metadata["accepted_unless"])
                if (
                    condition is None
                    or condition.metadata.get("direction") != "out"
                    or condition.type.pointee is None
                    or condition.type.pointee.kind != "bool"
                ):
                    errors.append(
                        f"{context}: accepted_unless requires a boolean output"
                    )
        if "receiver" in metadata and metadata["receiver"] not in parameters:
            errors.append(f"{context}: receiver names an absent parameter")
        for parameter in function.parameters:
            pcontext = f"{context} parameter {parameter.name}"
            errors.extend(metadata_errors(parameter.metadata, PARAMETER_KEYS, pcontext))
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
        errors.extend(metadata_errors(record.metadata, RECORD_KEYS, context))
        projection = record.metadata.get("projection") or (
            api.typedefs_by_name[record.name].metadata.get("projection")
            if record.name in api.typedefs_by_name
            else None
        )
        if projection:
            source = api.records_by_name.get(projection)
            if (
                source is None
                or not source.complete
                or source.kind != "struct"
                or source.metadata.get("projection")
            ):
                errors.append(
                    f"{context}: projection requires a complete source record without another projection"
                )
            elif record.kind != "struct":
                errors.append(f"{context}: projection requires a struct")
            else:
                projected = {field.name: field for field in record.fields}
                for source_field in source.fields:
                    if source_field.metadata.get("kind") in {"size", "reserved"}:
                        continue
                    target = projected.get(source_field.name)
                    if (
                        target is None
                        or target.type.canonical != source_field.type.canonical
                    ):
                        errors.append(
                            f"{context}: projection field {source_field.name} must preserve its source C type"
                        )
        fields = {field.name for field in record.fields}
        for field in record.fields:
            fcontext = f"{field.location}: {record.name}.{field.name}"
            errors.extend(metadata_errors(field.metadata, FIELD_KEYS, fcontext))
            errors.extend(value_metadata_errors(field.type, field.metadata, fcontext))
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
            if ("mask" in field.metadata) != ("bit" in field.metadata):
                presence = field_path(record.name, field.metadata.get("mask", ""))
                if presence is None or presence.type.kind != "bool":
                    errors.append(
                        f"{fcontext}: presence requires both mask and bit or a boolean mask"
                    )
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
            if "group_type" in field.metadata:
                group_type = api.records_by_name.get(field.metadata["group_type"])
                if group_type is None or "mask" not in field.metadata:
                    errors.append(
                        f"{fcontext}: group_type requires a record and masked presence"
                    )
                else:
                    group_fields = [
                        member
                        for member in record.fields
                        if member.metadata.get("mask") == field.metadata.get("mask")
                        and member.metadata.get("bit") == field.metadata.get("bit")
                    ]
                    if any(
                        member.metadata.get("group_type") != group_type.name
                        for member in group_fields
                    ):
                        errors.append(
                            f"{fcontext}: presence group members must agree on group_type"
                        )
                    if [
                        (member.name, member.type.canonical) for member in group_fields
                    ] != [
                        (member.name, member.type.canonical)
                        for member in group_type.fields
                    ]:
                        errors.append(
                            f"{fcontext}: group_type fields must match presence group names and types"
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
        errors.extend(metadata_errors(typedef.metadata, TYPEDEF_KEYS, context))
        if "projection" in typedef.metadata and typedef.name not in api.records_by_name:
            errors.append(f"{context}: projection requires a record typedef")
        for parameter in typedef.parameters:
            pcontext = f"{context} callback parameter {parameter.name}"
            errors.extend(metadata_errors(parameter.metadata, PARAMETER_KEYS, pcontext))
            errors.extend(
                value_metadata_errors(parameter.type, parameter.metadata, pcontext)
            )
        release = typedef.metadata.get("release")
        if (
            release
            and typedef.metadata.get("kind") != "callback_registration"
            and release not in api.functions_by_name
        ):
            errors.append(f"{context}: release names an absent function")
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
        dispose = typedef.metadata.get("dispose")
        if dispose and dispose not in api.functions_by_name:
            errors.append(f"{context}: dispose names an absent function")
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
                    or begin.return_type.declaration != "mln_status"
                    or end.parameters[0].type.canonical != "void *"
                    or end.return_type.kind != "void"
                ):
                    errors.append(
                        f"{context}: view scope requires handle/token begin and token end operations"
                    )
            if typedef.metadata.get("dispose_invalidates") == "parent":
                parent_type = api.typedefs_by_name.get(parent)
                if (
                    not dispose
                    or parent_type is None
                    or not parent_type.metadata.get("abandon")
                ):
                    errors.append(
                        f"{context}: parent disposal invalidation requires a disposer and abandonable parent"
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
                    operation.return_type.declaration != "mln_status"
                    and operation.return_type.kind != "void"
                ):
                    errors.append(f"{context}: {key} must return status or void")
                if key in {"dispose", "abandon"} and has_completion(operation):
                    errors.append(f"{context}: {key} must have synchronous acceptance")
        if typedef.metadata.get("reentry") == "protocol":
            owner_name = typedef.metadata.get("reentry_owner")
            owner = next(
                (
                    p.type.pointee or p.type
                    for p in typedef.parameters
                    if p.name == owner_name
                ),
                None,
            )
            if owner_name == "registration":
                owners = [
                    f.parameters[0].type
                    for f in api.functions
                    if f.metadata.get("registration")
                    and any(
                        p.name == f.metadata["registration"]
                        and p.type.declaration == typedef.name
                        for p in f.parameters
                    )
                ]
                owner = (
                    owners[0]
                    if owners and len({o.declaration for o in owners}) == 1
                    else None
                )
            names = tuple(
                filter(None, typedef.metadata.get("reentry_calls", "").split(","))
            )
            if owner is None or not names:
                errors.append(
                    f"{context}: protocol reentry requires a callback or registration owner and operations"
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
                    or operation.return_type.declaration != "mln_status"
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
                or wait.return_type.declaration != "mln_status"
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
            metadata_errors(enum.metadata, ENUM_KEYS, f"{enum.location}: {enum.name}")
        )
        if enum.metadata.get("kind") not in {None, "open", "bitmask"}:
            errors.append(
                f"{enum.location}: {enum.name}: enum kind must be open or bitmask"
            )
    if errors:
        raise ModelError(errors)
