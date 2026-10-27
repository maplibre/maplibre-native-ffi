"""Generate Rust operations over the completion and handle runtimes."""

from __future__ import annotations

from collections import defaultdict

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api, served
from tools.bindgen.protocol import COMPLETION_RESULT, STATUS
from tools.bindgen.semantic import (
    BoundApi,
    OperationPlan,
)

from ..model import Api, CType, Lowered, Unsupported

DIRECTORY = "bindings/rust"

SCALARS = {
    "int8_t": "i8",
    "int16_t": "i16",
    "uint16_t": "u16",
    "intptr_t": "isize",
    "ptrdiff_t": "isize",
    "uintptr_t": "usize",
    "bool": "bool",
    "_Bool": "bool",
    "double": "f64",
    "float": "f32",
    "uint32_t": "u32",
    "int32_t": "i32",
    "uint64_t": "u64",
    "int64_t": "i64",
    "size_t": "usize",
    "uint8_t": "u8",
}


# The cfg predicate that selects each platform. OpenHarmony targets are Linux
# targets with their own environment.
PLATFORM_CFG = {
    "android": 'target_os = "android"',
    "ios": 'target_os = "ios"',
    "linux": 'all(target_os = "linux", not(target_env = "ohos"))',
    "macos": 'target_os = "macos"',
    "ohos": 'target_env = "ohos"',
    "tvos": 'target_os = "tvos"',
    "visionos": 'target_os = "visionos"',
    "web": 'target_os = "emscripten"',
    "windows": 'target_os = "windows"',
}


def gated(plan: OperationPlan, code: str) -> str:
    """An operation's code, limited by cfg to the platforms that it serves."""
    platforms = served(plan.function, frozenset(PLATFORM_CFG))
    if platforms is None:
        return code
    predicates = [PLATFORM_CFG[name] for name in sorted(platforms)]
    if {"linux", "ohos"} <= platforms:
        predicates.remove(PLATFORM_CFG["ohos"])
        predicates[predicates.index(PLATFORM_CFG["linux"])] = 'target_os = "linux"'
    predicate = (
        predicates[0] if len(predicates) == 1 else f"any({', '.join(predicates)})"
    )
    lines = code.splitlines(keepends=True)
    item = next(
        i for i, line in enumerate(lines) if not line.lstrip().startswith("///")
    )
    indent = lines[item][: len(lines[item]) - len(lines[item].lstrip())]
    lines.insert(item, f"{indent}#[cfg({predicate})]\n")
    return "".join(lines)


def doc(bound: BoundApi, native: str, indent: str = "") -> str:
    """The rustdoc comment of a declaration, or empty when it has none."""
    return docs.line_comment(bound.doc(native), symbols(bound), indent)


def receiver_native(plan: OperationPlan) -> str | None:
    """The C type of the handle or response that an operation is a method of."""
    receiver = plan.receiver or plan.scoped_receiver
    value = next((p.value for p in plan.inputs if p.name == receiver), None)
    if value is None:
        return None
    return value.unwrapped.native


def method_name(plan: OperationPlan) -> str:
    return identifier(plan.member)


def symbols(bound: BoundApi) -> docs.Symbol:
    """How Rust spells the C declarations that its summaries name."""
    return bound.symbols("rust", _symbols)


def _symbols(bound: BoundApi) -> docs.Symbol:
    from .rust_owners import owned_handles, owner_name
    from .rust_values import Values, enum_member

    values = Values(bound)
    owners = owned_handles(bound)
    # A record that holds only a union presents the union under its own name.
    holders = {
        member.value.native: value
        for value in bound.values.values()
        if (member := value.union_member)
    }

    def type_(native: str) -> str | None:
        if native in owners:
            return owner_name(native)
        value = holders.get(native) or bound.values.get(native)
        # The completion runtime presents a command's result as
        # `CommandCompletion`.
        if (
            value is None
            or value.native == COMPLETION_RESULT
            or value.kind not in {"record", "union", "enum"}
        ):
            return None
        return values.name(value)

    def operation(plan: OperationPlan) -> str | None:
        owner = receiver_native(plan) or plan.factory
        if owner is None:
            return method_name(plan)
        typ = type_(owner)
        return f"{typ}::{method_name(plan)}" if typ else None

    def constant(value, member: str) -> str | None:
        name = enum_member(value, member)
        return f"{values.name(value)}::{name}" if name else None

    def field(value, member) -> str | None:
        typ = type_(value.native)
        if typ is None or value.union_member:
            return None
        # A union's members are its enum's variants.
        name = pascal(member.name) if value.kind == "union" else identifier(member.name)
        return f"{typ}::{name}"

    def flag(value, mask_flag) -> str | None:
        return f"{values.name(value)}::{identifier(mask_flag.member)}"

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=constant,
        field=field,
        flag=flag,
        extra={
            f"{COMPLETION_RESULT}.{name}": f"CommandCompletion::{name}"
            for name in ("generation", "render_update_generation")
        },
    )


def ctype(value: CType) -> str:
    return value.declaration or value.spelling.removeprefix("const ")


def pascal(value: str) -> str:
    return "".join(word.title() for word in value.removeprefix("mln_").split("_"))


RUST_KEYWORDS = {
    "abstract",
    "as",
    "async",
    "await",
    "become",
    "box",
    "break",
    "const",
    "continue",
    "crate",
    "do",
    "dyn",
    "else",
    "enum",
    "extern",
    "false",
    "final",
    "fn",
    "for",
    "gen",
    "if",
    "impl",
    "in",
    "let",
    "loop",
    "macro",
    "match",
    "mod",
    "move",
    "mut",
    "override",
    "priv",
    "pub",
    "ref",
    "return",
    "Self",
    "self",
    "static",
    "struct",
    "super",
    "trait",
    "true",
    "try",
    "type",
    "typeof",
    "unsafe",
    "unsized",
    "use",
    "virtual",
    "where",
    "while",
    "yield",
}
BINDGEN_RESERVED = RUST_KEYWORDS | {
    "alignof",
    "offsetof",
    "proc",
    "pure",
    "sizeof",
    "str",
    "bool",
    "f32",
    "f64",
    "usize",
    "isize",
    "u128",
    "i128",
    "u64",
    "i64",
    "u32",
    "i32",
    "u16",
    "i16",
    "u8",
    "i8",
    "_",
}


def is_text(value) -> bool:
    """Whether a buffer is text in Rust: UTF-8 text and JSON, which Rust
    presents as its string type."""
    return value.encoding in {"utf8", "json"}


def identifier(value: str) -> str:
    if value in {"self", "Self", "super", "crate", "_"}:
        return value + "_"
    return "r#" + value if value in RUST_KEYWORDS else value


def native_identifier(value: str) -> str:
    return value + "_" if value in BINDGEN_RESERVED else value


def native_call(
    function, arguments, prefix: str = "sys::", diagnostic: str = "diagnostic"
) -> str:
    """Calls a C function, passing `diagnostic` for its diagnostic parameter."""
    if function.diagnostic:
        arguments = [*arguments, diagnostic]
    return f"{prefix}{function.name}({', '.join(arguments)})"


def checked_call(
    function, arguments, prefix: str = "sys::", runtime: str = "crate::error"
) -> str:
    """Checks a status-returning C call against the diagnostic the runtime lends it."""
    if not function.diagnostic:
        raise Unsupported(f"{function.name}: status return has no diagnostic")
    call = native_call(function, arguments, prefix)
    return f"{runtime}::check(|diagnostic| unsafe {{ {call} }})"


def reads_receiver(plan: OperationPlan) -> bool:
    """An immediate receiver call copies borrowed output without a native view
    scope, which the Python binding guards."""
    from .rust_dynamic_values import dynamic

    def borrowed(value):
        target = value.unwrapped
        return "owner" in {value.lifetime, target.lifetime} or dynamic(target)

    return bool(
        plan.receiver
        and not plan.consumes
        and not plan.view
        and plan.execution == "immediate"
        and any(
            borrowed(output.value)
            for output in plan.outputs
            if not output.sized_by and not output.result_length
        )
    )


def counted_owner(bound: BoundApi, native: str) -> bool:
    """A release must not overlap a copy from this owner's borrowed output."""
    return any(
        reads_receiver(plan)
        and any(
            p.name == plan.receiver and p.value.native == native for p in plan.inputs
        )
        for plan in bound.operations
    )


def lower(api: Api | BoundApi) -> Lowered:
    chunks, generated, unsupported = defaultdict(list), [], {}
    bound = compile_api(api)
    from . import rust_owners
    from .rust_operations import operation
    from .rust_values import Values

    value_types = Values(bound)
    for value in bound.public_values.values():
        if value.kind == "enum":
            value_types.add(value)
    owners = rust_owners.owned_handles(bound)
    value_types.owners = {native: rust_owners.module_name(native) for native in owners}
    value_types.handle_types = {
        native: rust_owners.owner_name(native) for native in owners
    }
    unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    for plan in bound.operations:
        function = plan.function
        previous_values = dict(value_types.used)
        previous_directions = {
            name: set(known) for name, known in value_types.directions.items()
        }
        try:
            module, code = operation(plan, value_types)
            if module == "view" and plan.view:
                module = f"view:{plan.view.owner.native}"
            if code:
                chunks[module].append(gated(plan, code))
            if plan.completion and plan.completion.optional:
                chunks[module].append(
                    gated(plan, operation(plan, value_types, discard=True)[1])
                )
            generated.append(function.name)
        except Unsupported as error:
            value_types.used = previous_values
            value_types.directions = previous_directions
            unsupported[function.name] = f"{function.location}: {error}"
    marker = "// Generated from C headers by tools/bindgen. Do not edit.\n"
    root = "crates/maplibre-native-ffi/src/generated"
    files = {}
    for native, handle in owners.items():
        module = rust_owners.module_name(native)
        body = chunks.pop(module, [])
        files[f"{root}/{module}.rs"] = (
            marker
            + "use super::*;\n\n"
            + rust_owners.declaration(bound, handle)
            + (
                f"\nimpl {rust_owners.owner_name(native)} {{\n"
                + "\n".join(body)
                + "}\n"
                if body
                else ""
            )
        )
        commands = [
            p
            for p in bound.operations
            if p.name in generated
            and p.completion
            and p.completion.discardable
            and any(i.name == p.receiver and i.value.native == native for i in p.inputs)
        ]
        if views := chunks.pop(f"view:{native}", []):
            files[f"{root}/{module}.rs"] += rust_owners.view_guard(bound, handle, views)
        if commands:
            owner = rust_owners.owner_name(native)
            view = f"{owner}Discarding"
            files[f"{root}/{module}.rs"] += (
                f"\n/// Submits [`{owner}`] commands without a completion. Each call returns\n"
                "/// the same acceptance status as its completing form, and native logs a\n"
                "/// command that later fails as a warning through the log handler.\n"
                f"pub struct {view}<'a>(&'a {owner});\n"
                f"impl {owner} {{ pub fn discarding(&self) -> {view}<'_> {{ {view}(self) }} }}\n"
                f"impl {view}<'_> {{\n"
                + "\n".join(
                    gated(p, operation(p, value_types, discard=True)[1])
                    for p in commands
                )
                + "}\n"
            )
    modules = [rust_owners.module_name(native) for native in owners]
    if global_body := chunks.pop("global", []):
        files[f"{root}/global.rs"] = (
            marker + "use super::*;\n\n" + "\n".join(global_body)
        )
        modules.append("global")
    assert not chunks, f"operations without an owner module: {sorted(chunks)}"
    declarations = [value_types.render()]
    files[f"{root}/values.rs"] = marker + "use super::*;\n\n" + "\n".join(declarations)
    files[f"{root}/mod.rs"] = marker + rust_owners.module_index(
        owners, ["values", *modules]
    )
    from . import rust_sys

    files["crates/maplibre-native-ffi/src/status.rs"] = status_source(bound)
    sys_declarations = rust_sys.declarations(bound)
    files[rust_sys.PATH] = sys_declarations.render()
    for name, reason in sys_declarations.unsupported.items():
        if name in generated:
            generated.remove(name)
        unsupported.setdefault(name, reason)
    return Lowered(
        {f"{DIRECTORY}/{path}": source for path, source in files.items()},
        unsupported,
        generated,
    )


def status_source(bound):
    values = bound.values[STATUS].enum_values
    variants = [
        (
            pascal(key.removeprefix("MLN_STATUS_").lower()),
            number,
            key.removeprefix("MLN_STATUS_"),
        )
        for key, number in values
    ]
    return (
        "// Generated from C headers by tools/bindgen. Do not edit.\n"
        + doc(bound, STATUS)
        + "#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]\npub enum Status {\n"
        + "".join(
            f"{doc(bound, 'MLN_STATUS_' + label, '    ')}    {name},\n"
            for name, _, label in variants
        )
        + "    Unknown(i32),\n}\nimpl Status {\n"
        + "    pub fn from_raw(raw: i32) -> Self { match raw {\n"
        + "".join(
            f"        {number} => Self::{name},\n" for name, number, _ in variants
        )
        + "        value => Self::Unknown(value),\n    } }\n"
        + "    pub fn as_raw(self) -> i32 { match self {\n"
        + "".join(
            f"        Self::{name} => {number},\n" for name, number, _ in variants
        )
        + "        Self::Unknown(value) => value,\n    } }\n}\n"
        + "impl Default for Status { fn default() -> Self { Self::from_raw(0) } }\n"
        + "impl std::fmt::Display for Status { fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result { match self {\n"
        + "".join(
            f'        Self::{name} => f.write_str("{label}"),\n'
            for name, _, label in variants
        )
        + '        Self::Unknown(value) => write!(f, "UNKNOWN({value})"),\n    } } }\n'
    )
