"""Emit .NET public operations from the common header model.

The handwritten runtime owns tasks, callback roots, status handling, and handle
state. This module owns the owner classes and the files that hold each method,
and dotnet_operations owns each method's signature, native call, and borrowed
result copies. Unsupported declarations carry an explicit diagnostic.
"""

from __future__ import annotations

from collections import defaultdict
from dataclasses import replace

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api, served
from tools.bindgen.model import Api, Function, Lowered
from tools.bindgen.names import pascal, type_name
from tools.bindgen.protocol import COMPLETION_RESULT
from tools.bindgen.semantic import (
    BoundApi,
    HandlePlan,
    OperationPlan,
    public_stem,
)

from . import dotnet_native
from .dotnet_values import (
    Unsupported,
    Values,
    enum_member,
    raw_handle,
    typed_mask,
)

# The platform analyzer's name for each platform that the package serves.
PLATFORMS = {
    "android": ("android",),
    "linux": ("linux",),
    "macos": ("macos",),
    "ios": ("ios", "maccatalyst"),
    "tvos": ("tvos",),
    "windows": ("windows",),
}
TARGETS = frozenset(PLATFORMS)
DIRECTORY = "bindings/dotnet/src/Maplibre.NativeFfi"


def supported_platforms(plan: OperationPlan, indent: str = "    ") -> str:
    """The attributes that tell the platform analyzer where a call can work."""
    platforms = served(plan.function, TARGETS)
    if platforms is None:
        return ""
    return "".join(
        f'{indent}[global::System.Runtime.Versioning.SupportedOSPlatform("{platform}")]\n'
        for name in sorted(platforms)
        for platform in PLATFORMS[name]
    )


PRIMITIVES = {
    "double": "double",
    "float": "float",
    "bool": "bool",
    "_Bool": "bool",
    "int32_t": "int",
    "uint32_t": "uint",
    "int64_t": "long",
    "uint64_t": "ulong",
    "int16_t": "short",
    "uint16_t": "ushort",
    "size_t": "nuint",
    "int": "int",
    "unsigned int": "uint",
    "uint8_t": "byte",
    "int8_t": "sbyte",
}


CALLER_DRIVER_NOTE = (
    "With a caller-graphics-thread driver, `Ready`, `DetachAsync`, "
    "`BarrierAsync`, and `ResizeAsync` progress only through "
    "`ServiceDriverWork`, so awaiting one on the graphics thread without "
    "servicing driver work never completes."
)

# Paragraphs about what .NET adds to a declaration's C contract.
NOTES = {
    "mln_render_session": (CALLER_DRIVER_NOTE,),
    "mln_render_session_service_driver_work": (CALLER_DRIVER_NOTE,),
}


def doc(bound: BoundApi, native: str, indent: str = "") -> str:
    """The XML doc comment of a declaration, or empty when it has none."""
    found = bound.doc(native)
    if found is not None and native in NOTES:
        found = replace(found, notes=(*found.notes, *NOTES[native]))
    return docs.xml_comment(found, symbols(bound), indent)


def symbols(bound: BoundApi) -> docs.Symbol:
    """How .NET spells the C declarations that its summaries name."""
    return bound.symbols("dotnet", _symbols)


def _symbols(bound: BoundApi) -> docs.Symbol:
    from .dotnet_operations import emit_operation

    values = Values(bound)
    supported, used_records = set(), set()
    for plan in bound.operations:
        if plan.support:
            continue
        try:
            _, _, records = emit_operation(plan, bound)
        except Unsupported:
            continue
        supported.add(plan.name)
        used_records |= records
    enums = declared_enums(bound, values)
    delegates = delegate_types(values, used_records)
    # A union is a nested type of the record that holds it, or that record
    # itself when the record holds only the union.
    unions = {
        field.value.native: (holder, name, field)
        for holder in bound.values.values()
        if holder.native in used_records
        for name, field in values.members(holder)
        if field.value.kind == "union"
    }

    def type_(native: str) -> str | None:
        if handle := bound.handles.get(native):
            return owner_name(handle)
        if native in bound.callbacks:
            return public_type(native) if native in delegates else None
        value = bound.values.get(native)
        # The completion runtime presents a command's result as
        # `CommandCompletion`.
        if value is None or native == COMPLETION_RESULT:
            return None
        if value.kind == "union":
            if native not in unions:
                return None
            holder, name, field = unions[native]
            return values.member_type(holder, name, field)
        if value.kind == "enum":
            return public_type(native) if native in enums else None
        if value.kind == "record" and native not in bound.slices:
            return public_type(native) if native in used_records else None
        return None

    def operation(plan: OperationPlan) -> str | None:
        name = method_name(plan, bound)
        if plan.name not in supported or name is None:
            return None
        return f"{operation_owner(plan, bound)}.{name}"

    def constant(value, key: str) -> str | None:
        typ = type_(value.native)
        if typ is None:
            return None
        return (
            f"{typ}.{enum_member(value.native, (k for k, _ in value.enum_values), key)}"
        )

    def field(value, member) -> str | None:
        typ = type_(value.native)
        if typ is None or value.response or value.union_member:
            return None
        if value.kind == "union":
            # Each variant is a nested record of the union's type.
            return f"{typ}.{pascal(member.name)}"
        name = next(
            (n for n, f in values.members(value) if f.name == member.name), None
        )
        return f"{typ}.{name}" if name else None

    def flag(value, mask_flag) -> str | None:
        typ = type_(value.native)
        return f"{typ}.{values.flag_name(mask_flag)}" if typ else None

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=constant,
        field=field,
        flag=flag,
        extra={
            f"{COMPLETION_RESULT}.{name}": f"CommandCompletion.{pascal(name)}"
            for name in ("generation", "render_update_generation")
        },
    )


def declared_enums(bound: BoundApi, values: Values) -> set[str]:
    """The public enums that the binding declares."""
    names = set()
    for value in bound.public_values.values():
        if value.kind == "enum":
            try:
                values.supported(value)
            except Unsupported:
                continue
            names.add(value.native)
    return names


def delegate_types(values: Values, records) -> list[str]:
    """The callback types of the registrations among `records`, which the
    binding declares as delegates."""
    return sorted(
        {
            next(field for field in plan.fields if field.name == name).value.native
            for plan in map(values.record, records)
            if plan.registration
            for name in plan.registration.callbacks
        }
    )


def owner_name(handle: HandlePlan | str) -> str:
    """The public owner class of a handle plan, or of a receiver type that
    declares no handle contract."""
    stem = handle.stem if isinstance(handle, HandlePlan) else public_stem(handle)
    return pascal(stem) + "Handle"


NAMESPACE = "Maplibre.NativeFfi"


def directory_for(path: str) -> str:
    """The source directory of a header's declarations, named after the header."""
    return pascal(path.rsplit("/", 1)[-1].removesuffix(".h"))


def public_type(name: str) -> str:
    if name in PRIMITIVES:
        return PRIMITIVES[name]
    return pascal(name.removeprefix("mln_"))


# Generated sources take their namespace imports from GlobalUsings.g.cs. The
# compiler treats a .g.cs file as generated code, which disables nullable
# annotations unless the file enables them.
HEADER = (
    "// Generated from the C headers by tools/bindgen. Do not edit.\n#nullable enable\n"
)

OPERATION_HELPERS = (
    "using static Maplibre.NativeFfi.Internal.NativeCall;\n"
    "using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;\n\n"
)
VALUE_HELPERS = "using static Maplibre.NativeFfi.Internal.Struct.NativeValues;\n"


def native_call(function: Function, arguments: str, diagnostic: str) -> str:
    """Calls a C function, passing diagnostic when it takes one."""
    if function.diagnostic:
        arguments = f"{arguments}, {diagnostic}" if arguments else diagnostic
    return f"NativeMethods.{function.name}({arguments})"


def disposer_arguments(handle: HandlePlan, function: Function, live: str) -> str:
    """The arguments of a handle's disposer: the handle, and a null completion
    when the dispose is also the release."""
    arguments = [live]
    for parameter in function.parameters[1:]:
        if parameter.name != handle.dispose_completion:
            raise Unsupported(f"{function.name} disposes with more than its handle")
        arguments.append("null")
    return ", ".join(arguments)


def method(signature: str, body: list[str]) -> str:
    return (
        f"    {signature}\n    {{\n"
        + "".join(f"        {line}\n" for line in body)
        + "    }\n"
    )


def receiver_native(plan: OperationPlan) -> str:
    """The C type of the handle or response that an operation is a method of,
    or empty for a static method."""
    parameter = next(
        (
            parameter
            for parameter in plan.function.parameters
            if parameter.name == (plan.receiver or plan.scoped_receiver)
        ),
        None,
    )
    if parameter is None:
        return ""
    return type_name(parameter.type.pointee or parameter.type)


def operation_owner(plan: OperationPlan, bound: BoundApi) -> str:
    """The class that declares an operation's method: the receiver's owner or
    response class, the factory's owner, or `MaplibreNative` for a process
    global."""
    receiver = receiver_native(plan)
    if plan.scoped_receiver:
        return public_type(receiver)
    if receiver:
        return owner_name(bound.handles.get(receiver) or receiver)
    if plan.factory:
        return owner_name(bound.handles[plan.factory])
    return "MaplibreNative"


def method_name(plan: OperationPlan, bound: BoundApi, *, discard=False) -> str | None:
    """The .NET name of an operation's method: `Dispose` for a release that
    the owner's dispose runs, or None for one that no method runs. A
    discarding command keeps the name without `Async`."""
    function = plan.function
    handle = bound.handles.get(receiver_native(plan))
    name = pascal(plan.member)
    if handle and function.name == handle.discard:
        return name
    if (
        plan.consumes
        and handle
        and function.name == handle.release
        and handle.release_inputs
    ) or (plan.answers and handle):
        return name if plan.answers else "Release"
    if (
        handle is not None
        and function.name == handle.release
        and plan.completion
        and not handle.release_inputs
    ):
        return "CloseAsync"
    if plan.view:
        return "With" + name.removeprefix("Get")
    # A drain, and a call that creates an owner, which awaits its readiness
    # through the owner, keep their names.
    if plan.drain or plan.owned_outputs:
        return name
    if not plan.completion:
        if handle and handle.release == function.name and len(function.parameters) == 1:
            # Dispose already runs the release when no separate dispose exists.
            if handle.dispose not in {None, handle.release}:
                return "Close"
            return "Dispose" if handle.scoped_release == function.name else None
        return name
    return name if discard else name + "Async"


def lower(api: Api | BoundApi) -> Lowered:
    from .dotnet_operations import emit_operation

    methods: dict[str, list[str]] = defaultdict(list)
    # Process globals go beside the other declarations of their header.
    globals_by_directory: dict[str, list[str]] = defaultdict(list)
    discards: dict[str, list[str]] = defaultdict(list)
    records: dict[str, set[str]] = defaultdict(set)
    supported = []
    bound = compile_api(api, TARGETS)
    api = bound.source
    owners = {name: owner_name(handle) for name, handle in bound.handles.items()}
    unsupported = {
        name: "\n".join(reasons) for name, reasons in bound.unsupported.items()
    }
    owner_headers: dict[str, str] = {}
    for plan in bound.operations:
        function = plan.function
        try:
            if plan.support:
                raise Unsupported("support operation requires its generated owner")
            owner, emitted, used_records = emit_operation(plan, bound)
        except Unsupported as error:
            unsupported[function.name] = f"{function.location}: {error}"
            continue
        if owner == "MaplibreNative":
            directory = directory_for(function.location.path)
            globals_by_directory[directory].append(
                doc(bound, function.name, "    ") + supported_platforms(plan) + emitted
            )
            records[owner].update(used_records)
            supported.append(function.name)
            continue
        if emitted:
            methods[owner].append(
                doc(bound, function.name, "    ") + supported_platforms(plan) + emitted
            )
        if plan.receiver and plan.completion and plan.completion.discardable:
            _, discarded, _ = emit_operation(plan, bound, discard=True)
            discards[owner].append(supported_platforms(plan) + discarded)
        records[owner].update(used_records)
        supported.append(function.name)
        owner_headers.setdefault(owner, function.location.path)
    files = {}
    files.update(dotnet_native.generate(bound))
    created_handles = {
        handle.native
        for plan in bound.operations
        if plan.function.name in supported
        for handle in (
            [
                owner.handle
                for owner in plan.owned_outputs
                # A drain releases its batch itself.
                if not (plan.drain and owner.parameter == plan.drain.batch.name)
            ]
            + (
                [plan.result.handle]
                if plan.result
                and plan.result.kind == "handle"
                and plan.result.ownership == "owned"
                and plan.result.handle
                else []
            )
        )
    }
    created_handles.update(native for native in owners if owners[native] in methods)
    async_disposable: set[str] = set()
    for native in sorted(created_handles):
        handle = bound.handles[native]
        owner = owners[native]
        native_type = raw_handle(handle)
        parent = f"{owners[handle.parent]} parent, " if handle.parent else ""
        argument = "parent, " if handle.parent else ""
        release = next(
            plan for plan in bound.operations if plan.function.name == handle.release
        )
        asynchronous_release = bool(release.completion)
        if (
            asynchronous_release
            and not handle.release_inputs
            and handle.release in supported
        ):
            async_disposable.add(owner)
        immediate_completion = any(
            plan.completion
            and any(output.handle.native == native for output in plan.owned_outputs)
            for plan in bound.operations
        )
        extra = ", Task completion" if immediate_completion else ""
        extra_argument = ", completion" if immediate_completion else ""
        adopt_extra = ", Task completion" if immediate_completion else ""
        cleanup = handle.cleanup or handle.release
        cleanup_function = api.functions_by_name[cleanup]
        # Abandon disposes a handle that no owner can close any more. A dispose
        # that is also the release passes no completion.
        cleanup_arguments = disposer_arguments(handle, cleanup_function, "live")
        abandon = (
            f"=> {native_call(cleanup_function, cleanup_arguments, 'diagnostic')};"
            if cleanup_function.diagnostic
            else f"{{ NativeMethods.{cleanup}({cleanup_arguments}); return mln_status.MLN_STATUS_OK; }}"
        )
        destroy = (
            "StartRelease"
            if asynchronous_release
            else f"static (live, diagnostic) => {native_call(release.function, 'live', 'diagnostic')}"
        )
        if handle.release_inputs or (
            not asynchronous_release
            and type_name(release.function.return_type) == "void"
        ):
            destroy = "Abandon"
        fields = [f"private readonly NativeHandleState<{native_type}> state;"]
        if asynchronous_release:
            fields.append("private volatile Task teardown = Task.CompletedTask;")
        if immediate_completion:
            fields.append(
                "/// <summary>\n"
                "    /// Completes once the handle is ready, and faults with a\n"
                '    /// <see cref="MaplibreException" /> when it fails to become ready.\n'
                "    /// </summary>\n"
                "    public Task Ready { get; }"
            )
        constructor = []
        if immediate_completion:
            constructor.append(
                "// Mono may retain a completed continuation on its worker stack.\n"
                "// Clear its holder so it no longer roots this handle.\n"
                "Ready = completion.ContinueWith(static (finished, retained) => { ((System.Runtime.CompilerServices.StrongBox<object?>)retained!).Value = null; return finished; }, new System.Runtime.CompilerServices.StrongBox<object?>(this), CancellationToken.None, TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default).Unwrap();"
            )
        constructor.append(
            f"state = new(handle, {destroy}, nameof({owner}), Abandon{', retainedParent: parent' if handle.parent else ''});"
        )
        declarations = (
            "".join(f"    {field}\n" for field in fields)
            + "\n"
            + method(
                f"internal {owner}({parent}{native_type} handle{extra})", constructor
            )
            + f"\n    internal static {owner} Adopt({parent}{native_type} handle{adopt_extra}) =>\n"
            f"        NativeHandleState<{native_type}>.Adopt(handle, () => new {owner}({argument}handle{extra_argument}), Abandon);\n"
            f"\n    private static mln_status Abandon({native_type} live, mln_diagnostic* diagnostic) {abandon}\n"
            f"\n    NativeHandleState<{native_type}> INativeOwner<{native_type}>.State => state;\n"
            f"    internal {native_type} Handle => state.Handle;\n"
            + (
                "    /// <summary>\n"
                "    /// The native handle value, which runtime events report as their\n"
                "    /// source. It stays readable after close, so events drained later\n"
                "    /// still match this handle.\n"
                "    /// </summary>\n"
                if native in bound.event_sources
                else "    /// <summary>\n"
                "    /// The native handle value. It stays readable after close.\n"
                "    /// </summary>\n"
            )
            + "    public ulong Id => state.Handle.Value;\n"
            "    /// <summary>\n"
            "    /// Whether a release, dispose, close, or discard has ended the handle.\n"
            "    /// </summary>\n"
            "    public bool IsClosed => state.IsClosed;\n"
            "    /// <summary>\n"
            '    /// Names the handle\'s type, its <see cref="Id" />, and whether it is closed.\n'
            "    /// </summary>\n"
            "    public override string ToString() => state.ToString();\n"
            + (
                ""
                if handle.scoped_release is None
                else "    public void Dispose() { state.Retire(); }\n"
            )
        )
        methods[owner].insert(0, declarations)
    used_records = set().union(*records.values()) if records else set()
    values = Values(bound)
    enum_names = declared_enums(bound, values)
    declared_owners = {
        owner_name(handle): api.functions_by_name[handle.release].location.path
        for handle in bound.handles.values()
    } | {
        public_type(value.native): api.records_by_name[value.native].location.path
        for value in bound.public_values.values()
        if value.response
    }
    owner_directories = {
        owner: directory_for(declared_owners.get(owner, owner_headers[owner]))
        for owner in methods
    }
    files["GlobalUsings.g.cs"] = (
        "// Generated from the C headers by tools/bindgen. Do not edit.\n"
        + "global using System.Collections.Immutable;\n"
        + "global using Maplibre.NativeFfi.Internal;\n"
        + "".join(
            f"global using Maplibre.NativeFfi.Internal.{namespace};\n"
            for namespace in ("C", "Callback", "Memory", "Pointer", "Struct")
        )
        + "".join(
            f"global using static Maplibre.NativeFfi.Internal.C.{enum};\n"
            for enum in sorted(
                filter(
                    None,
                    {
                        typed_mask(field)
                        for value in bound.values.values()
                        for field in value.fields
                    },
                )
            )
        )
    )
    values = Values(bound)
    for owner, body in sorted(methods.items()):
        if owner in discards:
            body.append(
                "    /// <summary>\n"
                "    /// Submits each command without a completion. Native logs a failure as\n"
                "    /// a warning, and the caller sees none.\n"
                "    /// </summary>\n"
                "    public DiscardingCommands Discarding => new(this);\n\n"
                "    /// <summary>\n"
                "    /// The commands of a handle, each submitted without a completion.\n"
                "    /// Native logs a failure as a warning, and the caller sees none.\n"
                "    /// </summary>\n"
                "    public readonly struct DiscardingCommands\n    {\n"
                f"        private readonly {owner} owner;\n"
                f"        internal DiscardingCommands({owner} owner) {{ this.owner = owner; }}\n"
                + "\n".join(discards[owner])
                + "    }\n"
            )
        directory = owner_directories[owner]
        handle_types = {
            owners[native]: raw_handle(bound.handles[native])
            for native in created_handles
        }
        scoped_owners = {
            owners[native]
            for native in created_handles
            if bound.handles[native].scoped_release is not None
        }
        interfaces = [
            *(["IDisposable"] if owner in scoped_owners else []),
            *(["IAsyncDisposable"] if owner in async_disposable else []),
            *(
                [f"INativeOwner<{handle_types[owner]}>"]
                if owner in handle_types
                else []
            ),
        ]
        bases = f" : {', '.join(interfaces)}" if interfaces else ""
        handle_doc = next(
            (
                doc(bound, native)
                for native in created_handles
                if owners[native] == owner
            ),
            "",
        )
        files[f"{directory}/{owner}.Operations.g.cs"] = (
            HEADER
            + OPERATION_HELPERS
            + f"namespace {NAMESPACE};\n\n"
            + handle_doc
            + f"public sealed unsafe partial class {owner}{bases}\n{{\n"
            + "\n".join(body)
            + "}\n"
        )
    for directory, body in sorted(globals_by_directory.items()):
        files[f"{directory}/MaplibreNative.Operations.g.cs"] = (
            HEADER
            + OPERATION_HELPERS
            + f"namespace {NAMESPACE};\n\n"
            + "public static unsafe partial class MaplibreNative\n{\n"
            + "\n".join(body)
            + "}\n"
        )
    used_records = set().union(*records.values()) if records else set()
    converters = []
    for record_name in sorted(used_records):
        plan = values.record(record_name)
        if record_name in bound.returned:
            converters.append(
                values.decoder(plan).replace("private static", "internal static")
            )
        converters.append(values.callback_methods(plan))
        if values.can_encode(plan):
            converters.append(
                values.encoder(plan).replace("private static", "internal static")
            )
    files["Internal/Struct/GeneratedValues.g.cs"] = (
        HEADER
        + VALUE_HELPERS
        + "using System.Runtime.CompilerServices;\nusing System.Runtime.InteropServices;\n\n"
        + "namespace Maplibre.NativeFfi.Internal.Struct;\n\ninternal static unsafe class GeneratedValues\n{\n"
        + "\n".join(converters)
        + "}\n"
    )
    for name in sorted(used_records):
        if name in bound.slices:
            continue
        record = api.records_by_name[name]
        directory = directory_for(record.location.path)
        plan = values.record(name)
        declaration = values.declaration(plan)
        files[f"{directory}/{public_type(name)}.g.cs"] = (
            HEADER + f"namespace {NAMESPACE};\n\n" + doc(bound, name) + declaration
        )
    delegates = delegate_types(values, used_records)
    for native in delegates:
        directory = directory_for(api.typedefs_by_name[native].location.path)
        found = bound.doc(native)
        note = values.delegate_note(native)
        if found is not None and note:
            found = replace(found, notes=(*found.notes, note))
        files[f"{directory}/{public_type(native)}.g.cs"] = (
            HEADER
            + f"namespace {NAMESPACE};\n\n"
            + docs.xml_comment(found, symbols(bound))
            + values.delegate(native)
        )
    for enum in api.enums:
        if enum.name not in enum_names:
            continue
        directory = directory_for(enum.location.path)
        constants = [value.name for value in enum.values]
        underlying = PRIMITIVES.get(enum.underlying_type.spelling)
        if underlying is None:
            underlying = {
                "unsigned long long": "ulong",
                "unsigned long": "ulong",
                "unsigned int": "uint",
                "int": "int",
            }[enum.underlying_type.canonical]
        flags = "[Flags]\n" if enum.metadata.get("kind") == "bitmask" else ""
        fields = [
            doc(bound, value.name, "    ")
            + f"    {enum_member(enum.name, constants, value.name)} = {value.value},"
            for value in enum.values
        ]
        files[f"{directory}/{public_type(enum.name)}.g.cs"] = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            f"namespace {NAMESPACE};\n\n{doc(bound, enum.name)}{flags}"
            f"public enum {public_type(enum.name)} : {underlying}\n{{\n"
            + "\n".join(fields)
            + "\n}\n"
        )
    for native in created_handles:
        dispose = bound.handles[native].cleanup
        if dispose and dispose not in supported:
            supported.append(dispose)
            unsupported.pop(dispose, None)
    views = {
        output.value.element.native
        for operation in bound.operations
        if operation.function.name in supported and operation.view
        for output in operation.outputs
        if output.value.element
    }
    for native in sorted(views):
        value = values.record(native)
        name = public_type(native)
        directory = directory_for(api.records_by_name[native].location.path)
        # The scope guards only what the view's owner lends; a copy of any
        # other member stays readable after the scope ends.
        properties = "".join(
            f"    public {values.property_type(value, member, field)} {member} => "
            + (f"scope.Active(value).{member}" if field.scoped else f"value.{member}")
            + ";\n"
            for member, field in values.members(value)
        )
        files[f"{directory}/{name}View.g.cs"] = (
            HEADER + f"namespace {NAMESPACE};\n\npublic sealed class {name}View\n{{\n"
            f"    private readonly {name} value;\n    private readonly NativeViewScope scope;\n"
            f"    internal {name}View({name} value, NativeViewScope scope) {{ this.value = value; this.scope = scope; }}\n"
            + properties
            + "}\n"
        )
    return Lowered(
        {f"{DIRECTORY}/{path}": source for path, source in files.items()},
        unsupported,
        sorted(supported),
    )
