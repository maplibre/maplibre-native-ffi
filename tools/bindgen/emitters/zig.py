"""Emit the Zig public API from resolved values, owners, and callback protocols."""

from __future__ import annotations

import re

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api, served
from tools.bindgen.model import Api, CType, Function, ModelError, Record
from tools.bindgen.names import pascal
from tools.bindgen.protocol import COMPLETION_RESULT
from tools.bindgen.semantic import BoundApi, OperationPlan

SCALARS = {
    "int8_t": "i8",
    "int16_t": "i16",
    "uint16_t": "u16",
    "intptr_t": "isize",
    "ptrdiff_t": "isize",
    "uintptr_t": "usize",
    "double": "f64",
    "float": "f32",
    "bool": "bool",
    "_Bool": "bool",
    "uint32_t": "u32",
    "uint8_t": "u8",
    "uint64_t": "u64",
    "int32_t": "i32",
    "int64_t": "i64",
    "size_t": "usize",
}


# The comptime condition that selects each platform the package builds for.
# Android and OpenHarmony are Linux ABIs.
PLATFORMS = {
    "android": "builtin.abi.isAndroid()",
    "linux": "(builtin.os.tag == .linux and !builtin.abi.isAndroid() and !builtin.abi.isOpenHarmony())",
    "macos": "builtin.os.tag == .macos",
    "ohos": "builtin.abi.isOpenHarmony()",
    "windows": "builtin.os.tag == .windows",
}


def gated(plan, code: str, name: str) -> str:
    """A function that is void on the platforms it does not serve, as std marks
    a declaration that a target lacks."""
    platforms = served(plan.function, frozenset(PLATFORMS))
    if platforms is None:
        return code
    condition = " or ".join(PLATFORMS[platform] for platform in sorted(platforms))
    start = code.index(f"pub fn {name}(")
    function = code[start:].replace(f"pub fn {name}(", "fn function(", 1)
    return (
        code[:start]
        + f"pub const {name} = if ({condition}) struct {{\n"
        + function.rstrip("\n")
        + "\n}.function else {};\n"
    )


def identifier(value: str) -> str:
    return '@"' + value + '"'


def native(type_: CType) -> str:
    return type_.declaration or type_.spelling.removeprefix("const ")


def failure(decl: Function | Record, reason: str) -> ModelError:
    return ModelError([f"{decl.location}: Zig: {decl.name}: {reason}"])


DIAGNOSTIC_PARAMETER = "diagnostic: ?*diagnostics.Diagnostic"


def status_call(
    function: Function, arguments: list[str], diagnostic: str = "diagnostic"
) -> str:
    """Calls a status-returning C function through the diagnostic runtime."""
    if not function.diagnostic:
        raise failure(function, "status result requires a diagnostic parameter")
    tuple_ = ".{ " + ", ".join(arguments) + " }" if arguments else ".{}"
    return f"try status.call(c.{function.name}, {tuple_}, {diagnostic});"


def doc(bound: BoundApi, native: str, indent: str = "") -> str:
    """The Zig doc comment of a declaration, or empty when it has none."""
    return docs.line_comment(bound.doc(native), symbols(bound), indent)


def type_name(native: str) -> str:
    """The public type of a handle, record, enum, or named union."""
    return pascal(native.removeprefix("mln_"))


def status_error(constant: str) -> str:
    """The error that a failing `mln_status` constant names."""
    return pascal(constant.removeprefix("MLN_STATUS_").lower())


def owned_handles(bound: BoundApi):
    """The handles that have an owner type: those whose every finalizer the
    binding generates."""
    return [
        handle
        for handle in bound.handles.values()
        if all(name in bound.operations_by_name for name in handle.finalize)
    ]


def symbols(bound: BoundApi) -> docs.Symbol:
    """How Zig spells the C declarations that its summaries name."""
    return bound.symbols("zig", _symbols)


def _symbols(bound: BoundApi) -> docs.Symbol:
    from .zig_callbacks import parameter_name
    from .zig_operations import function_name, method_name
    from .zig_values import Values, enum_member

    values = Values(bound)
    owners = {handle.native for handle in owned_handles(bound)}
    # A record that holds only a union presents the union under its own name.
    holders = {
        member.value.native: value
        for value in bound.values.values()
        if (member := value.union_member)
    }

    def type_(native: str) -> str | None:
        if native in owners:
            return type_name(native)
        value = holders.get(native) or bound.values.get(native)
        # The completion runtime presents a command's result as
        # `CommandCompletion`, and a slice record is presented as its slice.
        if (
            value is None
            or value.native == COMPLETION_RESULT
            or value.native in bound.slices
            or value.kind not in {"record", "union", "enum"}
        ):
            return None
        return values.public(value)

    def operation(plan: OperationPlan) -> str | None:
        native = receiver_native(plan)
        # A factory is a function of the type that it creates.
        container = native or plan.factory
        if container is None:
            return function_name(plan, values)
        typ = type_(container)
        if typ is None:
            return None
        name = method_name(plan) if native else function_name(plan, values)
        return f"{typ}.{name}"

    def constant(value, member: str) -> str | None:
        # A failing status is an error of every call that returns a status.
        if value.native == "mln_status" and dict(value.enum_values)[member] != 0:
            return f"error.{status_error(member)}"
        return f"{values.public(value)}.{parameter_name(enum_member(value, member))}"

    def field(value, member) -> str | None:
        typ = type_(value.native)
        # A response presents no members, a record that holds only a union
        # presents the union's variants instead, and a registration presents
        # its user data and release as `context` and `release_context`.
        if (
            typ is None
            or value.response
            or value.union_member
            or (
                value.registration
                and member.name
                in {value.registration.user_data, value.registration.release}
            )
        ):
            return None
        return f"{typ}.{parameter_name(member.name)}"

    def flag(value, mask_flag) -> str | None:
        return f"{values.public(value)}.{parameter_name(mask_flag.member)}"

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=constant,
        field=field,
        flag=flag,
        extra={
            f"{COMPLETION_RESULT}.{member}": f"CommandCompletion.{member}"
            for member in ("generation", "render_update_generation")
        },
    )


def event_sources(bound: BoundApi) -> frozenset[str]:
    """The handles that runtime events name as their source."""
    prefix = "MLN_RUNTIME_EVENT_SOURCE_"
    return frozenset(
        "mln_" + member.name.removeprefix(prefix).lower()
        for enum in bound.source.enums
        if enum.name == "mln_runtime_event_source_type"
        for member in enum.values
    )


def documented(bound: BoundApi, native: str, code: str, marker: str) -> str:
    """Code with a declaration's comment before the line that starts it."""
    start = code.index(marker)
    return code[:start] + doc(bound, native) + code[start:]


def expose_parameter_names(plan, code, name):
    """Keep generated temporaries private while naming parameters from the header."""
    marker = f"pub fn {name}("
    start = code.index(marker) + len(marker)
    end = code.index("\n", start)
    signature = code[start:end]
    body = code[end:]
    aliases = []
    used = {"allocator", "diagnostic"}
    for index, parameter in enumerate(plan.function.parameters):
        internal = f"binding_arg_{index}"
        if not re.search(rf"\b{internal}:", signature):
            continue
        public = parameter.name
        if public == internal:
            continue
        if public in used or re.search(rf"\b{re.escape(public)}\b", body):
            public += "_input"
        while public in used:
            public += "_input"
        used.add(public)
        signature = signature.replace(internal + ":", identifier(public) + ":", 1)
        aliases.append(f"    const {internal} = {identifier(public)};")
    return (
        code[:start] + signature + ("\n" + "\n".join(aliases) if aliases else "") + body
    )


# The declarations every handle type has besides its operations.
HANDLE_DECLARATIONS = {
    "native_name",
    "methods",
    "adopt",
    "id",
    "isClosed",
    "format",
    "deinit",
    "discarding",
}


def receiver_native(plan) -> str | None:
    """The native type whose generated type holds `plan` as a method."""
    receiver = plan.receiver or plan.scoped_receiver
    if receiver is None:
        return None
    value = next(p.value for p in plan.inputs if p.name == receiver)
    return value.unwrapped.native if value.kind == "reference" else value.native


def lower(api: Api | BoundApi):
    """The generated values and free functions, the methods of each receiver
    type by native name, the generated C names, and the unsupported ones."""
    chunks, generated, unsupported = [], [], {}
    methods: dict[str, list[str]] = {}
    bound = compile_api(api, frozenset(PLATFORMS))
    from .zig_operations import function_name, method_name
    from .zig_operations import operation as lower_operation
    from .zig_values import Values

    value_types = Values(bound)
    for value in bound.public_values.values():
        if value.kind == "enum":
            value_types.add(value)
    unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    taken: dict[str, set[str]] = {}
    for plan in bound.operations:
        if native := receiver_native(plan) or plan.factory:
            names = taken.setdefault(native, set(HANDLE_DECLARATIONS))
            names.add(method_name(plan))
            if plan.completion and plan.completion.optional:
                names.add(method_name(plan, discard=True))
    for plan in bound.operations:
        function = plan.function
        native = receiver_native(plan)
        # A factory is a function of the type that it creates.
        container = native or plan.factory
        previous = dict(value_types.used), dict(value_types.results)
        try:
            lowered = []
            # A dispose that takes an optional completion is also its handle's
            # release; its dispose passes none, as every disposer does.
            for discard in (
                [False, True]
                if plan.completion and plan.completion.optional
                else [False]
            ):
                code = lower_operation(
                    plan,
                    value_types,
                    discard=discard,
                    method=native is not None,
                    taken=frozenset(taken.get(container, ())),
                )
                name = (
                    method_name(plan, discard=discard)
                    if native
                    else function_name(plan, value_types, discard=discard)
                )
                lowered.append(
                    gated(
                        plan,
                        documented(
                            bound,
                            function.name,
                            expose_parameter_names(plan, code, name),
                            f"pub fn {name}(",
                        ),
                        name,
                    )
                )
            (methods.setdefault(container, []) if container else chunks).extend(lowered)
            generated.append(function.name)
        except ModelError as error:
            value_types.used, value_types.results = previous
            unsupported[function.name] = str(error)
    value_types.methods = methods
    return [value_types.render(), *chunks], methods, generated, unsupported


def generate(api: Api | BoundApi) -> dict[str, str]:
    chunks, methods, generated, _ = lower(api)
    prefix = """// Code generated by tools/bindgen; DO NOT EDIT.
const std = @import("std");
const builtin = @import("builtin");
const c = @import("c.zig").raw;
const call = @import("call.zig");
const callbacks = @import("callback.zig");
const completion = @import("completion.zig");
const diagnostics = @import("diagnostics.zig");
const marshal = @import("marshal.zig");
const owner = @import("owner.zig");
const status = @import("status.zig");

pub const OwnedValue = marshal.OwnedValue;

"""
    bound = compile_api(api, frozenset(PLATFORMS))
    owners = []
    for handle in owned_handles(bound):
        calls = []
        for finalizer in handle.finalize:
            function = bound.operations_by_name[finalizer].function
            # A dispose that takes a completion gets a null one.
            arguments = [
                "raw",
                *(
                    ["null"]
                    if handle.dispose_completion and finalizer == handle.dispose
                    else []
                ),
            ]
            calls.append(
                f"c.{finalizer}({', '.join(arguments)});"
                if function.return_type.kind == "void"
                else status_call(function, arguments, "null")
            )
        public = type_name(handle.native)
        dispose = (
            "null"
            if handle.scoped_release is None
            else f"struct {{ fn dispose(raw: u64) status.Error!void {{ {' '.join(calls)} }} }}.dispose"
        )
        source = (
            "///\n/// Runtime events report its `id()` as their source.\n"
            if handle.native in event_sources(bound)
            else ""
        )
        commands = any(
            plan.receiver
            and plan.completion
            and plan.completion.discardable
            and plan.name in generated
            and receiver_native(plan) == handle.native
            for plan in bound.operations
        )
        members = [
            "raw: u64,",
            "closed: bool = false,",
            f'pub const native_name = "{handle.native}";',
            f'const methods = owner.Handle(@This(), "{public}", {dispose});',
            "pub const adopt = methods.adopt;",
            "pub const id = methods.id;",
            "pub const isClosed = methods.isClosed;",
            "pub const format = methods.format;",
            *(["pub const deinit = methods.deinit;"] if handle.scoped_release else []),
            *(
                [
                    "/// Submits this handle's commands without a future; native logs a failed one as a warning.",
                    f"pub fn discarding(self: {public}) {public}Discarding {{ return .{{ .receiver = self }}; }}",
                ]
                if commands
                else []
            ),
            *methods.get(handle.native, []),
        ]
        owners.append(
            doc(bound, handle.native)
            + source
            + f"pub const {public} = struct {{\n"
            + "\n".join(members)
            + "\n};\n"
        )
    from .zig_operations import method_name, operation
    from .zig_values import Values

    values = Values(bound)
    command_owners = {}
    generated = set(generated)
    for plan in bound.operations:
        if (
            plan.name in generated
            and plan.receiver
            and plan.completion
            and plan.completion.discardable
        ):
            native = next(
                p.value.native for p in plan.inputs if p.name == plan.receiver
            )
            command_owners.setdefault(native, []).append(
                gated(
                    plan,
                    operation(plan, values, discard=True),
                    method_name(plan, discard=True),
                )
            )
    variants = []
    for native, methods in command_owners.items():
        public = pascal(native.removeprefix("mln_"))
        variants.append(
            f"pub const {public}Discarding = struct {{ receiver: {public},\n"
            + "\n".join(methods)
            + "\n};\n"
        )
    docs = (
        "//! Low-level Zig binding for the MapLibre Native C API. Release owners\n"
        "//! explicitly or call deinit.\n"
        "//! A call takes an allocator only to convert its inputs or to own its result.\n"
        "//! OwnedValue snapshots and futures retain their storage until deinit.\n"
        "//! Each has unique ownership: move it rather than copy it, since deinit on two\n"
        "//! copies releases the native value twice.\n"
        "//! Futures and callback registrations keep their state in std.heap.smp_allocator.\n"
        "//! Callback contexts stay valid until release_context runs after native retirement.\n"
        "//! GPU descriptors are borrowed only inside the scoped getter callback.\n"
        "//! Callback errors are logged through std.log; native receives the failure value.\n"
        "//! A callback that returns a status answers native, unlogged, the status that\n"
        "//! an error such as error.InvalidArgument names, since it fails on purpose.\n"
        "//! Text that a callback receives is borrowed from native until the callback returns.\n"
        "//! A function that serves only some targets is void on the others.\n"
    )
    runtime = """
pub const Future = completion.Future;
pub const CommandCompletion = completion.CommandCompletion;
pub const Error = status.Error;
pub const BindingError = status.BindingError;
pub const Diagnostic = diagnostics.Diagnostic;

"""
    source = docs + prefix + runtime + "\n".join([*owners, *chunks, *variants])
    status_value = bound.public_values.get("mln_status")
    files = {"maplibre_native_ffi.zig": source}
    if status_value:
        entries = [
            (key, status_error(key))
            for key, number in status_value.enum_values
            if number != 0
        ]
        members = ", ".join(member for _, member in entries)
        from_raw = "\n".join(f"c.{key} => error.{member}," for key, member in entries)
        to_raw = "\n".join(f"error.{member} => c.{key}," for key, member in entries)
        files[
            "generated_status.zig"
        ] = f"""// Code generated by tools/bindgen; DO NOT EDIT.
const c = @import("c.zig").raw;
pub const NativeStatusError = error{{ {members}, UnknownStatus }};
pub fn fromRaw(raw: i32) NativeStatusError {{
    return switch (raw) {{ {from_raw} else => error.UnknownStatus, }};
}}
pub fn toRaw(err: anyerror) ?c.mln_status {{
    return switch (err) {{ {to_raw} else => null, }};
}}
"""
    return files


def coverage(api: Api | BoundApi) -> dict:
    bound = compile_api(api, frozenset(PLATFORMS))
    _, _, generated, unsupported = lower(bound)
    return {
        "generated": generated,
        "unsupported": unsupported,
    }
