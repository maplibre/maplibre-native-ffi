"""Generate Dart operations and value conversion from the shared binding plans."""

from __future__ import annotations

import re
from collections import defaultdict
from dataclasses import replace

from tools.bindgen import async_hosts, docs
from tools.bindgen.compiler import compile_api
from tools.bindgen.managed_contracts import KEYWORDS
from tools.bindgen.model import Api, Lowered
from tools.bindgen.names import camel, pascal, type_name
from tools.bindgen.protocol import COMPLETION_RESULT
from tools.bindgen.semantic import (
    BoundApi,
    OperationPlan,
    output_member,
)

from .dart_values import (
    Unsupported,
    Values,
    doc,
    enum_member,
    identifier,
    owner_names,
    public_name,
    union_name,
    variant_name,
)

# Functions that only host platform code can call. `mln_android_init` takes a
# `JNIEnv*` and a `Context`, which an isolate has no safe way to obtain, so a
# Flutter app's Android host code calls it before the isolate starts.
HOST_ONLY = {"mln_android_init"}
# The platforms that the package builds for. Its library serves them all, so it
# leaves out only what none of them can use.
TARGETS = frozenset({"android", "ios", "linux", "macos", "windows"})
OPERATIONS = "bindings/dart/lib/src/runtime/generated_operations.dart"
VALUES = "bindings/dart/lib/src/generated_values.dart"


def native_call(function, arguments):
    """Call a C function, passing the isolate's diagnostic when it takes one."""
    if function.diagnostic:
        arguments = [*arguments, "nativeDiagnostic"]
    return f"raw.{function.name}({', '.join(arguments)})"


def receiver_handle(plan: OperationPlan, bound: BoundApi) -> tuple[str | None, bool]:
    """The public handle that an operation is a method of, or None for a
    global, and whether the call takes a pointer to it."""
    function = plan.function
    receiver = type_name(function.parameters[0].type) if plan.receiver else None
    reference = False
    if (
        receiver
        and function.parameters[0].type.pointee
        and type_name(function.parameters[0].type.pointee) in bound.public_handles
    ):
        receiver = type_name(function.parameters[0].type.pointee)
        reference = True
    if receiver not in bound.public_handles:
        return None, reference
    return receiver, reference


def method_name(plan: OperationPlan, bound: BoundApi, *, dispose=False) -> str:
    """The Dart name of an operation: a method, static factory, or top-level
    function. A receiver's release is its `close`, and a release that also
    disposes has a synchronous `dispose` besides."""
    name = camel(plan.member)
    receiver, _ = receiver_handle(plan, bound)
    handle = bound.handles.get(receiver)
    if (
        plan.consumes
        and handle
        and plan.name == handle.release
        and not handle.release_inputs
    ):
        name = "close"
    if dispose:
        name = "dispose"
    if name in KEYWORDS["dart"]:
        name += "Value"
    return name


def view_method(name: str) -> str:
    """The method that lends a borrowed view to a callback, for the
    operation named `name`."""
    return "with" + name.removeprefix("get")


def symbols(bound: BoundApi) -> docs.Symbol:
    """How Dart spells the C declarations that its summaries name."""
    return bound.symbols("dart", _symbols)


def _symbols(bound: BoundApi) -> docs.Symbol:
    values = Values(bound)
    # Each union, with the record and field that hold it.
    unions = {
        field.value.native: (value, field)
        for value in bound.public_values.values()
        for field in value.fields
        if field.value.kind == "union"
    }
    # The callback types that a registration takes from Dart.
    callbacks = {
        field.value.native: field.value
        for value in bound.public_values.values()
        if not (value.registration and values.native_only(value))
        for field in values.fields(value)
        if field.value.kind == "callback"
    }

    def type_(native: str) -> str | None:
        if native in bound.public_handles:
            return owner_names(bound.handles[native])[0]
        if native in unions:
            return union_name(*unions[native])
        value = callbacks.get(native) or bound.values.get(native)
        # The completion runtime presents a command's result as
        # `CommandCompletion`, and a slice is presented as its list alone.
        if (
            value is None
            or native == COMPLETION_RESULT
            or native in bound.slices
            or value.kind not in {"record", "enum", "callback"}
        ):
            return None
        # A lone callback that stands for its registration may be optional.
        try:
            return values.public(replace(value, nullable=False)).removesuffix("?")
        except Unsupported:
            return None

    def operation(plan: OperationPlan) -> str | None:
        if (
            plan.name in HOST_ONLY
            or plan.name in bound.unsupported
            or plan.scoped_receiver
        ):
            return None
        name = method_name(plan, bound)
        if plan.view:
            name = view_method(name)
        owner, _ = receiver_handle(plan, bound)
        owner = owner or plan.factory
        if owner is None:
            return name
        return f"{owner_names(bound.handles[owner])[0]}.{name}"

    def constant(value, member: str) -> str | None:
        return f"{public_name(value.native)}.{enum_member(value, member)}"

    def field(value, member) -> str | None:
        if value.kind == "union":
            record, union = unions[value.native]
            return variant_name(record, union, member)
        typ = type_(value.native)
        # A lone callback stands for its registration, and a native
        # registration takes its callbacks as constructor arguments.
        if (
            typ is None
            or value.union_member
            or values.single_callback(value)
            or value.registration
            and values.native_only(value)
            or member.name not in {f.name for f in values.fields(value)}
        ):
            return None
        return f"{typ}.{identifier(member.name)}"

    def flag(value, mask_flag) -> str | None:
        return f"{public_name(value.native)}.{identifier(mask_flag.member)}"

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=constant,
        field=field,
        flag=flag,
        extra={
            f"{COMPLETION_RESULT}.{name}": f"CommandCompletion.{camel(name)}"
            for name in ("generation", "render_update_generation")
        },
        # Native queues each call, and the isolate that registered the callback
        # dispatches the queue from a receive port.
        threads=(
            "The callback runs asynchronously in the isolate and zone that "
            "registered it, after native queues the call."
        ),
    )


def attachment_name(owned):
    return "New" + pascal(owned.handle.stem)


def _lower(api: Api | BoundApi):
    from .dart_operations import lower_function

    methods, generated, unsupported = defaultdict(list), [], {}
    bound = compile_api(api, TARGETS)
    values = Values(bound)
    root_owners = values.root_owners
    unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    for plan in bound.operations:
        if plan.name in HOST_ONLY:
            continue
        try:
            # Failed attempts cannot leave unrenderable value fragments behind.
            local_values = Values(bound)
            owner, body = lower_function(plan, local_values)
            # The ABI version check reads the C version, so reading it skips
            # the check.
            if owner == "Globals" and not plan.unchecked:
                body = abi_checked(body)
            # A factory is a static method of the owner of the handle it
            # creates.
            if plan.factory:
                owner, body = plan.factory, "  static " + body.removeprefix("  ")
            body = doc(bound, plan.name, "  ") + body
            local_values.render()
            values.used.update(local_values.used)
            values.results.update(local_values.results)
            methods[owner].append(body)
            if plan.receiver and plan.completion and plan.completion.discardable:
                _, discarded = lower_function(plan, local_values, discard=True)
                methods[owner + "Discarding"].append(discarded)
            if plan.receiver and plan.completion and plan.completion.optional:
                _, disposed = lower_function(plan, local_values, dispose=True)
                methods[owner].append(doc(bound, plan.name, "  ") + disposed)
            generated.append(plan.name)
            if owner != "Globals" and plan.registrations and not plan.owned_outputs:
                root_owners.add(owner)
        except Unsupported as error:
            unsupported[plan.name] = f"{plan.function.location}: {error}"
    return methods, generated, unsupported, values


def render_scoped_views(bound, generated, values):
    chunks = []
    emitted = set()
    diagnostic = set()
    for plan in bound.operations:
        if not plan.view or plan.name not in generated:
            continue
        diagnostic.add(
            bound.source.functions_by_name[plan.view.owner.view_begin].diagnostic
        )
        value = plan.outputs[0].value.element
        public = values.public(value)
        if public in emitted:
            continue
        emitted.add(public)
        getters = []
        for name, typ, field in values.members(value):
            # The scope guards only what the view's owner lends; a copy of any
            # other member stays readable after the scope ends.
            native_pointer = field.value.kind == "native_pointer" and field.scoped
            if native_pointer:
                result = f"ScopedNativePointer(_value.{name}.address, checkValid: _scope.checkActive, debugName: '{public}.{name}')"
            elif field.scoped:
                result = f"_scope.active(_value).{name}"
            else:
                result = f"_value.{name}"
            getters.append(
                f"  {'ScopedNativePointer' if native_pointer else typ} get {name} => {result};"
            )
        method = view_method(method_name(plan, bound))
        chunks.append(
            f"/// A `{public}` whose native objects are readable only while the\n"
            f"/// callback of `{owner_names(plan.view.owner)[0]}.{method}` runs. Its\n"
            "/// other members stay readable after the callback returns.\n"
            f"final class Scoped{public} {{\n  Scoped{public}._(this._scope, this._value);\n  final {public} _value;\n  final _NativeViewScope _scope;\n"
            + "\n".join(getters)
            + "\n}\n"
        )
    if chunks and diagnostic != {True}:
        raise Unsupported("borrowed view scopes require a diagnostic begin")
    return "\n".join(chunks)


def render_owner(native, handle, bodies, bound, roots):
    public, native_type = owner_names(handle)
    parameters, fields = [], []
    if handle.parent in bound.public_handles:
        parameters.append("this._parent")
        fields.append(
            "  // Keeps the parent owner reachable while this owner lives.\n"
            f"  // ignore: unused_field\n  final {owner_names(bound.handles[handle.parent])[0]} _parent;"
        )
    if roots:
        fields.append(
            "  // Roots this owner's callback registrations for as long as it lives.\n"
            "  @override\n  final _callbackRoots = _CallbackRoots();"
        )
    fields.append(f"  final NativeHandleState<{native_type}> _state;")
    commands = [
        p
        for p in bound.operations
        if p.completion
        and p.completion.discardable
        and any(i.name == p.receiver and i.value.native == native for i in p.inputs)
    ]
    # Calls that consume their receiver reach it through _state, so an owner
    # whose every call does may use _handle only as another call's input.
    unused = not commands and not any(
        re.search(r"\b_handle\b", body) for body in bodies
    )
    fields.append(
        ("  // ignore: unused_element\n" if unused else "")
        + f"  {native_type} get _handle => _state.handle;"
    )
    if commands:
        fields.append(
            f"  {public}Discarding get discarding => {public}Discarding._(this);"
        )
    parameters.append(f"{native_type} handle")
    return (
        "/// Issued native handle id.\n"
        f"extension type const {native_type}(int raw) implements NativeHandle {{}}\n\n"
        "/// Owner of one native handle.\n"
        + (f"///\n{summary}" if (summary := doc(bound, native)) else "")
        + f"final class {public} implements Finalizable{', _CallbackRootOwner' if roots else ''} {{\n"
        f"  {public}._({', '.join(parameters)}) : _state = NativeHandleState(handle, '{public}');\n"
        + "\n".join(fields)
        + "\n\n"
        + (
            "  /// The native handle id, which runtime events report as their source.\n"
            "  ///\n"
            "  /// It stays readable after close, so events drained later still match\n"
            "  /// this handle.\n"
            if native in bound.event_sources
            else "  /// The native handle id.\n"
        )
        + "  int get id => _state.handleId;\n\n"
        "  /// Whether the binding has accepted a close, dispose, discard, or release\n"
        "  /// of this handle.\n"
        "  bool get isClosed => _state.isClosed;\n\n"
        "  @override\n"
        f"  String toString() => '{public}(id=0x${{id.toRadixString(16)}}, closed=$isClosed)';\n\n"
        + "".join(bodies)
        + "}\n"
    )


def render_attachments(bound, generated):
    """Pair each owner a call creates at once with the completion that
    reports it ready."""
    chunks = {}
    for plan in bound.operations:
        if plan.name not in generated or not plan.completion:
            continue
        for owned in plan.completion.immediate_owners:
            name = attachment_name(owned)
            field = camel(output_member(owned.parameter))
            chunks[name] = (
                f"/// A new {field} and the completion that reports it ready.\n"
                f"final class {name} {{\n"
                f"  const {name}(this.{field}, this.ready);\n\n"
                f"  /// The {field}, which the caller owns before it is ready.\n"
                f"  final {owner_names(owned.handle)[0]} {field};\n\n"
                f"  /// Completes once the {field} is ready, or fails with its status.\n"
                "  /// Await it or call `ignore()`, because the zone reports a failure\n"
                "  /// that nothing handles.\n"
                "  final Future<void> ready;\n}\n"
            )
    return [chunks[name] for name in sorted(chunks)]


def lower(api: Api | BoundApi) -> Lowered:
    from . import dart_native

    methods, generated, unsupported, values = _lower(api)
    bound = values.bound
    # A callback that must answer on its native thread has no Dart form, so its
    # registration carries native rules instead, and the operations that only
    # such a callback calls have no caller.
    registrations = [v for v in values.used.values() if v.registration]
    for name in async_hosts.rule_answered_operations(bound, registrations):
        unsupported.pop(name, None)
    declarations, conversions = values.render()
    return Lowered(
        {
            OPERATIONS: operations(bound, methods, generated, values, conversions),
            VALUES: "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            "part of 'values.dart';\n\n" + declarations,
            dart_native.PATH: dart_native.generate(bound),
        },
        unsupported,
        generated,
    )


def operations(bound, methods, generated, values, conversions) -> str:
    parts = re.split(
        r"(?m)(?=^(?:final raw\.\w+ _deliver\w+ =|(?:Pointer<raw\.\w+>|_NativeRegistration<raw\.\w+>|List<\w+>|\w+) _(?:write|read|prepare|deliver)\w+\())",
        conversions,
    )
    conversion_map = {}
    for part in parts:
        match = re.search(r"\s(_(?:write|read|prepare|deliver)\w+)[( ]", part)
        if match:
            conversion_map[match[1]] = part
    descriptors = "".join(
        f"final {name} = {code};\n" for name, code in sorted(values.results.items())
    )
    needed = set(
        re.findall(
            r"_(?:write|read|prepare|deliver)\w+",
            "".join(body for bodies in methods.values() for body in bodies)
            + descriptors,
        )
    )
    while True:
        expanded = needed | set(
            re.findall(
                r"_(?:write|read|prepare|deliver)\w+",
                "".join(conversion_map.get(name, "") for name in needed),
            )
        )
        if expanded == needed:
            break
        needed = expanded
    conversions = "".join(
        part for name, part in conversion_map.items() if name in needed
    )
    chunks = [
        "// Generated from the C headers by tools/bindgen. Do not edit.\npart of 'runtime.dart';\n",
        conversions,
        descriptors,
    ]
    chunks.extend(methods.get("Globals", []))
    for native, handle in sorted(bound.public_handles.items()):
        if discarded := methods.get(native + "Discarding"):
            public, native_type = owner_names(handle)
            chunks.append(
                f"extension type {public}Discarding._({public} _owner) {{\n"
                f"  {native_type} get _handle => _owner._handle;\n"
                + (
                    "  _CallbackRoots get _callbackRoots => _owner._callbackRoots;\n"
                    if native in values.root_owners
                    else ""
                )
                + "".join(discarded)
                + "}\n"
            )
        chunks.append(
            render_owner(
                native,
                handle,
                methods.get(native, []),
                bound,
                native in values.root_owners,
            )
        )
    chunks.extend(render_attachments(bound, generated))
    chunks.append(render_scoped_views(bound, generated, values))
    return "\n".join(chunks)


def abi_checked(body: str) -> str:
    """Validate the C ABI first in an entry point that needs no existing handle.

    Receiver operations are reachable only through a handle that such an entry
    point produced, so only globals carry the check.
    """
    head, newline, rest = body.partition("\n")
    if head.rstrip().endswith("{"):
        return f"{head}\n    ensureAbiVersion();{newline}{rest}"
    signature, arrow, expression = body.partition(" => ")
    if not arrow or not expression.rstrip().endswith(";"):
        raise Unsupported("global entry point needs an ABI check insertion point")
    return (
        f"{signature} {{\n    ensureAbiVersion();\n"
        f"    return {expression.rstrip()[:-1]};\n  }}\n"
    )
