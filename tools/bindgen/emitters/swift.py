"""Emit Swift operations over the native completion runtime."""

from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api, served
from tools.bindgen.model import Api, Function, Lowered, Unsupported
from tools.bindgen.names import camel
from tools.bindgen.protocol import COMPLETION_RESULT
from tools.bindgen.semantic import BoundApi, OperationPlan

# The condition that selects each platform the package builds for.
PLATFORMS = {
    "android": "os(Android)",
    "ios": "os(iOS)",
    "linux": "os(Linux)",
    "macos": "os(macOS)",
    "tvos": "os(tvOS)",
    "visionos": "os(visionOS)",
    "windows": "os(Windows)",
}
DIRECTORY = "bindings/swift/Sources/MaplibreNativeFFI"

# JSON is text, so it takes and returns a String as UTF-8 text does.
TEXT_ENCODINGS = frozenset({"utf8", "json"})

SCALARS = {
    "double": "Double",
    "float": "Float",
    "bool": "Bool",
    "_Bool": "Bool",
    "uint32_t": "UInt32",
    "uint64_t": "UInt64",
    "int32_t": "Int32",
    "int64_t": "Int64",
    "uint8_t": "UInt8",
    "size_t": "Int",
    "ptrdiff_t": "Int",
    "intptr_t": "Int",
    "uintptr_t": "UInt",
    "uint16_t": "UInt16",
    "int16_t": "Int16",
    "int8_t": "Int8",
    "long": "CLong",
    "unsigned long": "CUnsignedLong",
}


def identifier(value: str) -> str:
    return "`" + value + "`"


def name(value: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in value.split("_"))


def label_words(text: str) -> list[str]:
    """The lowercase words of a camel-case Swift name."""
    return [word.lower() for word in re.findall(r"[A-Z]?[a-z]+|[A-Z]+|\d+", text)]


def repeats(method: str, label: str) -> bool:
    """Whether a first argument label only repeats words of its method's name,
    as `setStyleUrl(url:)` and `deleteOfflineRegion(regionId:)` do. A trailing
    `Id` or `Name` names the key of what the method already names."""
    named = label_words(label)
    while len(named) > 1 and named[-1] in {"id", "name"}:
        named.pop()
    return set(named) <= set(label_words(method))


def native_call(function: Function, *arguments: str) -> str:
    """Calls a C function, passing the enclosing `diagnostic` when it takes one."""
    if function.diagnostic:
        arguments = (*arguments, "diagnostic")
    return f"{function.name}({', '.join(arguments)})"


def checked(statement: str) -> str:
    """Runs a status-returning statement under a fresh per-call diagnostic."""
    return f"try checkStatus {{ diagnostic in {statement} }}"


def doc(bound: BoundApi, native: str, indent: str = "") -> str:
    """The DocC comment of a declaration, or empty when it has none."""
    return docs.line_comment(bound.doc(native), symbols(bound), indent)


def receiver_native(plan: OperationPlan) -> str | None:
    """The C type of the handle or response that an operation is a method of."""
    receiver = plan.receiver or plan.scoped_receiver
    value = next((p.value for p in plan.inputs if p.name == receiver), None)
    if value is None:
        return None
    return (
        value.element.native
        if value.kind == "reference" and value.element
        else value.native
    )


def owner_type(plan: OperationPlan, bound: BoundApi) -> str | None:
    """The type that declares an operation: its receiver's or its factory's. A
    process global is a function of the module, which no type declares."""
    from .swift_ownership import owner_name

    receiver = receiver_native(plan)
    if plan.scoped_receiver and receiver:
        return name(receiver.removeprefix("mln_"))
    if receiver:
        return owner_name(bound.handles[receiver])
    if plan.factory:
        return owner_name(bound.handles[plan.factory])
    return None


def method_name(plan: OperationPlan) -> str:
    """The name of an operation's method, without the backticks that declare it."""
    if plan.view:
        from .swift_views import method_name as view_method

        return view_method(plan)
    if plan.consumes:
        from .swift_ownership import method_name as consuming_method

        return consuming_method(plan)
    return camel(plan.member)


def symbols(bound: BoundApi) -> docs.Symbol:
    """How Swift spells the C declarations that its summaries name."""
    return bound.symbols("swift", _symbols)


def _symbols(bound: BoundApi) -> docs.Symbol:
    from .swift_callbacks import alias
    from .swift_ownership import owner_name, public_handles
    from .swift_values import Values, enum_member

    values = Values(bound)
    owners = {handle.native: owner_name(handle) for handle in public_handles(bound)}
    # A record that holds only a union presents the union under its own name.
    holders = {
        member.value.native: value
        for value in bound.values.values()
        if (member := value.union_member)
    }

    def type_(native: str) -> str | None:
        if native in owners:
            return owners[native]
        if native in bound.callbacks:
            return alias(bound.callbacks[native])
        value = holders.get(native) or bound.values.get(native)
        # The completion runtime presents a command's result as
        # `CommandCompletion`, and a slice record as its array alone.
        if (
            value is None
            or value.native == COMPLETION_RESULT
            or value.native in bound.slices
            or value.kind not in {"record", "union", "enum"}
        ):
            return None
        return values.public(value)

    def operation(plan: OperationPlan) -> str | None:
        owner = receiver_native(plan) or plan.factory
        if owner is not None and type_(owner) is None:
            return None
        declared = owner_type(plan, bound)
        return f"{declared}.{method_name(plan)}" if declared else method_name(plan)

    def constant(value, member: str) -> str | None:
        return f"{values.public(value)}.{enum_member(value, member)}"

    def field(value, member) -> str | None:
        typ = type_(value.native)
        # A response presents no members, and a record that holds only a union
        # presents the union's variants instead.
        if typ is None or value.response or value.union_member:
            return None
        return f"{typ}.{camel(member.name)}"

    def flag(value, mask_flag) -> str | None:
        return f"{values.public(value)}.{camel(mask_flag.member)}"

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=constant,
        field=field,
        flag=flag,
        extra={
            f"{COMPLETION_RESULT}.{member}": f"CommandCompletion.{camel(member)}"
            for member in ("generation", "render_update_generation")
        },
    )


def member_doc(bound: BoundApi, native: str) -> str:
    """The DocC comment of an operation, which `render` places in an extension
    and so wraps at the member indent."""
    return doc(bound, native, "  ")


def gated(plan: OperationPlan, code: str) -> str:
    """An operation's declaration, compiled only for the platforms it serves."""
    platforms = served(plan.function, frozenset(PLATFORMS))
    if platforms is None:
        return code
    condition = " || ".join(PLATFORMS[name] for name in sorted(platforms))
    return f"#if {condition}\n{code}#endif\n"


# Headers outside the umbrella header import as explicit submodules, as they do
# from MaplibreNativeC.framework.
SUBMODULES = {"plugin": "MaplibreNativeC.Plugin"}


def _lower(
    api: Api | BoundApi,
) -> tuple[dict[str, list[str]], dict[str, set[str]], list[str], dict[str, str]]:
    chunks, imports, generated, failures = defaultdict(list), defaultdict(set), [], {}
    bound = compile_api(api, frozenset(PLATFORMS))
    api = bound.source
    from .swift_operations import operation
    from .swift_values import Values

    value_types = Values(bound)
    for value in bound.public_values.values():
        if value.kind == "enum":
            value_types.add(value)
    failures.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    for plan in bound.operations:
        function = plan.function
        previous = dict(value_types.used)
        try:
            chunk, extended = operation(plan, value_types)
            chunk = gated(plan, chunk)
            if extended:
                chunk = (extended, chunk)
            receiver = next(
                (item.value for item in plan.inputs if item.name == plan.receiver), None
            )
            if receiver and receiver.kind == "reference":
                receiver = receiver.element
            owner = (
                name(receiver.native.removeprefix("mln_"))
                if receiver and receiver.kind == "handle"
                else name(plan.factory.removeprefix("mln_"))
                if plan.factory
                else "Globals"
            )
            stem = Path(function.location.path).stem
            path = f"Generated/Operations/Generated{name(stem)}{owner}Operations.swift"
            chunks[path].append(chunk)
            if plan.receiver and plan.completion and plan.completion.discardable:
                discarded, view = operation(plan, value_types, discard=True)
                chunks[path].append((view, gated(plan, discarded)))
            if stem in SUBMODULES:
                imports[path].add(SUBMODULES[stem])
            generated.append(function.name)
        except Unsupported as error:
            value_types.used = previous
            failures[function.name] = str(error)
    from .swift_ownership import owner_declarations

    chunks["Generated/GeneratedOwners.swift"].append(owner_declarations(bound))
    from .swift_ownership import owner_name

    command_owners = {
        owner_name(bound.handles[p.value.native])
        for plan in bound.operations
        if plan.name in generated and plan.completion and plan.completion.discardable
        for p in plan.inputs
        if p.name == plan.receiver
    }
    for owner in sorted(command_owners):
        chunks["Generated/GeneratedOwners.swift"].append(
            f"public struct {owner}Discarding {{ let owner: {owner} }}\n"
            f"public extension {owner} {{ var discarding: {owner}Discarding {{ {owner}Discarding(owner: self) }} }}\n"
        )
    declarations = {
        item.name: item for item in (*api.typedefs, *api.records, *api.enums)
    }
    for native, value in sorted(value_types.used.items()):
        stem = Path(declarations[native].location.path).stem
        path = f"Generated/Values/Generated{name(stem)}Values.swift"
        chunks[path].append(value_types.record(value))
        if stem in SUBMODULES:
            imports[path].add(SUBMODULES[stem])
    if value_types.attachments:
        chunks["Generated/GeneratedAttachments.swift"].extend(
            value_types.attachments.values()
        )
    if value_types.views:
        chunks["Generated/GeneratedViews.swift"].extend(value_types.views.values())
    return dict(chunks), dict(imports), generated, failures


def render(bodies: list) -> str:
    """Joins a file's declarations, gathering members by the type they extend."""
    members: dict[str, list[str]] = {}
    parts: list = []
    for body in bodies:
        if isinstance(body, str):
            parts.append(body)
            continue
        extended, member = body
        if extended not in members:
            members[extended] = []
            parts.append(extended)
        members[extended].append(member)
    return "\n".join(
        f"public extension {part} {{\n" + "\n".join(members[part]) + "}\n"
        if part in members
        else part
        for part in parts
    )


def lower(api: Api | BoundApi) -> Lowered:
    chunks, imports, generated, failures = _lower(api)
    files = {
        f"{DIRECTORY}/{path}": "// Code generated by tools/bindgen; DO NOT EDIT.\n\n"
        + "".join(
            f"internal import {module}\n"
            for module in ("MaplibreNativeC", *sorted(imports.get(path, ())))
        )
        + "import Foundation\n\n"
        + render(bodies)
        for path, bodies in sorted(chunks.items())
    }
    return Lowered(files, failures, generated)
