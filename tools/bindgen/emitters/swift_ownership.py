"""Render native ownership transitions for Swift handle wrappers."""

from ..model import ModelError


def consumed(plan, owner, values):
    function = plan.function
    if plan.consumes == "always" and (
        plan.completion or function.return_type.kind != "void"
    ):
        raise ModelError(
            [
                f"Swift: {function.name}: unconditional consumption requires a void immediate release"
            ]
        )
    receiver = next(
        parameter for parameter in plan.inputs if parameter.name == plan.receiver
    )
    handle = receiver.value.handle or (
        receiver.value.element.handle if receiver.value.element else None
    )
    if not handle:
        raise ModelError(
            [
                f"Swift: {function.name}: release inputs require scoped ownership conversion"
            ]
        )
    from .swift import checked, doc, native_call

    documented = doc(values.bound, function.name)
    if len(plan.inputs) != 1:
        from .swift import camel, identifier
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
        return (
            f"""public extension {owner} {{
{doc(values.bound, function.name, "  ")}  func release({", ".join(declarations)}) throws {{
    let arena = NativeInputArena()
    defer {{ withExtendedLifetime(arena) {{}} }}
    try handle.closeOnce {{ live in
      var raw = live.raw
      {checked(native_call(function, *args))}
    }}
  }}
}}
""",
            None,
        )
    # A release that is also the dispose closes asynchronously, so its
    # synchronous form is the dispose.
    method = (
        "close"
        if function.name == handle.release and not handle.dispose_completion
        else "discard"
        if function.name == handle.discard
        else "dispose"
    )
    # The closure's parameters select the overload: a status-returning close
    # takes the diagnostic, so its status is always checked. A void release
    # can't fail, so its close throws only when a batch refuses it.
    call = native_call(function, *disposer_arguments(handle, function, "raw"))
    if infallible_close(handle, function):
        synchronous = f"""{documented}func {method}() {{
  nativeClose {{ raw in {call} }}
}}
"""
    elif function.return_type.kind != "void":
        synchronous = f"""{documented}func {method}() throws {{
  try nativeClose {{ raw, diagnostic in {call} }}
}}
"""
    else:
        synchronous = f"""{documented}func {method}() throws {{
  try nativeGuardedClose {{ raw in {call} }}
}}
"""
    if plan.completion:
        asynchronous = f"""{documented}func close() async throws {{
  guard let future = try startClose() else {{ return }}
  try await future.value()
}}
internal func startClose() throws -> NativeFuture<Void>? {{
  try nativeStartClose {{ raw, completion, diagnostic in {native_call(function, "raw", "completion")} }}
}}
"""
        # A release that is also the dispose passes a completion, and the
        # dispose passes none.
        if plan.completion.optional:
            return asynchronous + synchronous, owner
        return asynchronous, owner
    return synchronous, owner


def disposer_arguments(handle, function, raw):
    """The arguments before the diagnostic of `function` when it disposes `handle`.

    A disposer reports nothing, so it passes nil for a dispose's optional
    completion.
    """
    if handle.dispose_completion and function.name == handle.dispose:
        return [raw, "nil"]
    return [raw]


def protects_copies(handle):
    """Whether the handle lends memory that its getters copy after they return."""
    return handle.native == "mln_render_frame_batch"


def infallible_close(handle, function):
    """Whether `function`, a release of `handle`, closes it without a throw."""
    return function.return_type.kind == "void" and not protects_copies(handle)


def owner_name(handle):
    """The public owner class of a handle plan."""
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
        for parameter in (*operation.inputs, *operation.outputs):
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
        arguments = disposer_arguments(handle, function, "raw")
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
        if protects_copies(handle):
            passed += ", protectsCopies: true"
        from .swift import doc

        chunks.append(f'''{doc(bound, handle.native)}public final class {owner}: @unchecked Sendable, NativeReceiver, Identifiable, Hashable, CustomStringConvertible {{
  let handle: NativeHandleState<{raw}>
  init(adopting raw: {handle.native}{parent}) throws {{
    handle = try NativeHandleState(typeName: "{owner}", handle: {raw}(raw: raw){passed})
  }}
  /// The native handle, which no later handle in the process reuses. It
  /// stays valid after the handle closes.
  public var id: UInt64 {{ handle.value.raw }}
  public var description: String {{ handle.description }}
  public static func == (lhs: {owner}, rhs: {owner}) -> Bool {{ lhs.id == rhs.id }}
  public func hash(into hasher: inout Hasher) {{ hasher.combine(id) }}
}}
''')
    return "\n".join(chunks)
