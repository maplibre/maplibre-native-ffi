"""Render native ownership transitions for Swift handle wrappers."""

import textwrap

from ..model import Unsupported
from ..names import camel


def consumed(plan, owner, values):
    function = plan.function
    # An answer ends its receiver on every outcome, through its own
    # adapter; any other call that always consumes is a void release.
    if (
        plan.consumes == "always"
        and not plan.answers
        and (plan.completion or function.return_type.kind != "void")
    ):
        raise Unsupported(
            f"Swift: {function.name}: unconditional consumption requires a void immediate release"
        )
    handle = receiver_handle(plan)
    if not handle:
        raise Unsupported(
            f"Swift: {function.name}: release inputs require scoped ownership conversion"
        )
    from .swift import checked, doc, member_doc, native_call

    documented = member_doc(values.bound, function.name)
    if len(plan.inputs) != 1 or not plan.release:
        from .swift import identifier
        from .swift_dynamic_values import encode

        declarations, args = [], []
        for parameter in plan.inputs:
            if parameter.name == plan.receiver:
                args.append("&raw" if parameter.value.kind == "reference" else "raw")
                continue
            values.add(parameter.value)
            local = identifier(camel(parameter.name))
            declarations.append(f"{local}: {values.public(parameter.value)}")
            args.append(encode(values, parameter.value, local))
        # A release with inputs is idempotent. An answer keeps its name, ends
        # its receiver whatever it returns, and a repeated one reaches native.
        name = identifier(method_name(plan))
        consume = "answer" if plan.answers else "closeOnce"
        return (
            f"""public extension {owner} {{
{doc(values.bound, function.name, "  ")}  func {name}({", ".join(declarations)}) throws {{
    let arena = NativeInputArena()
    defer {{ withExtendedLifetime(arena) {{}} }}
    try handle.{consume} {{ live in
      {"var" if "&raw" in args else "let"} raw = live.raw
      {checked(native_call(function, *args))}
    }}
  }}
}}
""",
            None,
        )
    method = disposal(plan, handle)
    if plan.completion:
        return (
            f"""{documented}func {method}() async throws {{
  guard let future = try startRelease() else {{ return }}
  try await future.value()
}}
internal func startRelease() throws -> NativeFuture<Void>? {{
  try nativeStartClose {{ raw, completion, diagnostic in {native_call(function, "raw", "completion")} }}
}}
""",
            owner,
        )
    # The closure's parameters select the overload: a status-returning release
    # or dispose takes the diagnostic, so its status is always checked. A void
    # release can't fail, so it doesn't throw.
    call = native_call(function, *disposer_arguments(function, "raw"))
    if function.return_type.kind == "void":
        synchronous = f"""{documented}func {method}() {{
  nativeClose {{ raw in {call} }}
}}
"""
    else:
        synchronous = f"""{documented}func {method}() throws {{
  try nativeClose {{ raw, diagnostic in {call} }}
}}
"""
    return synchronous, owner


def receiver_handle(plan):
    """The handle that a consuming operation's receiver holds, if any."""
    receiver = next(
        parameter for parameter in plan.inputs if parameter.name == plan.receiver
    )
    return receiver.value.handle or (
        receiver.value.element.handle if receiver.value.element else None
    )


def disposal(plan, handle):
    """The method of a release, dispose, or discard of `handle`, which the
    shared plan names."""
    return camel(plan.member)


def method_name(plan):
    """The name of the method that consumes an operation's receiver: a
    release, dispose, or discard by its shared name, and an answer by its
    own."""
    return camel(plan.member)


def disposer_arguments(function, raw):
    """The arguments before the diagnostic of `function` when it disposes a
    handle: the handle alone."""
    if len(function.parameters) != 1:
        raise Unsupported(f"Swift: {function.name} disposes with more than its handle")
    return [raw]


def owner_name(handle):
    from .swift import name

    return name(handle.stem) + "Handle"


def public_handles(bound):
    def handles(value):
        if value.handle:
            yield value.handle.native
        if value.element:
            yield from handles(value.element)

    used = set()
    for operation in bound.operations:
        # A drain releases its batch itself.
        drained = operation.drain.batch.name if operation.drain else None
        for parameter in (*operation.inputs, *operation.outputs):
            if parameter.name != drained:
                used.update(handles(parameter.value))
        if operation.result:
            used.update(handles(operation.result))
    return [bound.handles[native] for native in sorted(used)]


def owner_declarations(bound):
    chunks = []
    for handle in public_handles(bound):
        owner = owner_name(handle)
        raw = "Native" + owner
        cleanup = handle.cleanup or handle.release
        function = bound.source.functions_by_name[cleanup]
        # Abandoned disposal has no caller to report a diagnostic to.
        arguments = disposer_arguments(function, "raw")
        if function.diagnostic:
            arguments.append("nil")
        expression = f"{cleanup}({', '.join(arguments)})"
        body = (
            f"{expression}; return true"
            if function.return_type.kind == "void"
            else f"{expression} == MLN_STATUS_OK"
        )
        chunks.append(
            f"struct {raw}: NativeHandle {{ let raw: UInt64; func disposeAbandoned() -> Bool {{ {body} }} }}\n"
        )
        parent = (
            f", parent: {owner_name(bound.handles[handle.parent])}"
            if handle.parent
            else ""
        )
        passed = ", parent: parent" if handle.parent else ""
        from .swift import doc

        id_doc = (
            "  /// The native handle, which no later handle in the process reuses\n"
            "  /// and runtime events report as their source. It stays valid after\n"
            "  /// the handle is released.\n"
            if handle.native in bound.event_sources
            else "  /// The native handle, which no later handle in the process reuses.\n"
            "  /// It stays valid after the handle is released.\n"
        )
        # A callback's borrowed owner never releases or disposes its handle.
        borrow = (
            f"  init(borrowing raw: {handle.native}) throws {{\n"
            f'    handle = try NativeHandleState(typeName: "{owner}", handle: {raw}(raw: raw), borrowed: true)\n'
            "  }\n"
            if handle.native in bound.borrowed_handles
            else ""
        )
        readied = bound.readied_handles.get(handle.native)
        # The task holds only the completion, not this owner, so a pending
        # readiness never keeps the owner alive. A handle that no attach
        # created, such as a borrowed one, has none and is ready.
        readiness = "  private let readiness: Task<Void, Error>?\n" if readied else ""
        ready_parameter = ", ready: Task<Void, Error>" if readied else ""
        ready_assignment = "    readiness = ready\n" if readied else ""
        if readied:
            borrow = borrow.replace(
                "borrowed: true)\n", "borrowed: true)\n    readiness = nil\n", 1
            )
        ready_member = (
            "".join(
                f"  /// {line}\n"
                for line in textwrap.wrap(
                    f"Returns once the {readied} is ready, and throws a "
                    "``MaplibreError`` when it fails to become ready. The "
                    f"{readied} takes calls before then.",
                    74,
                )
            )
            + "  public var ready: Void {\n"
            "    get async throws { try await readiness?.value }\n"
            "  }\n"
            if readied
            else ""
        )
        chunks.append(f'''{doc(bound, handle.native)}public final class {owner}: @unchecked Sendable, NativeReceiver, Identifiable, Hashable, CustomStringConvertible {{
  let handle: NativeHandleState<{raw}>
{readiness}  init(adopting raw: {handle.native}{parent}{ready_parameter}) throws {{
    handle = try NativeHandleState(typeName: "{owner}", handle: {raw}(raw: raw){passed})
{ready_assignment}  }}
{borrow}{ready_member}{id_doc}  public var id: UInt64 {{ handle.value.raw }}
  /// Whether the binding has accepted a release, dispose, or discard of this
  /// handle.
  public var isClosed: Bool {{ handle.isClosed }}
  public var description: String {{ handle.description }}
  public static func == (lhs: {owner}, rhs: {owner}) -> Bool {{ lhs.id == rhs.id }}
  public func hash(into hasher: inout Hasher) {{ hasher.combine(id) }}
}}
''')
    return "\n".join(chunks)
