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
    try NativeCallbackGuard.check(owner: self, operation: "{function.name}")
    try mapNativeFailure {{
      let arena = NativeInputArena()
      defer {{ withExtendedLifetime(arena) {{}} }}
      try handle.closeOnce {{ live in
        var raw = live.raw
        {checked(native_call(function, *args))}
      }}
    }}
  }}
}}
""",
            None,
        )
    is_release = function.name == handle.release
    method = "close" if is_release else "dispose"
    if plan.completion:
        return (
            f"""{documented}func close() async throws {{
  guard let future = try startClose() else {{ return }}
  try await mapNativeFailure {{ try await future.value() }}
}}
internal func startClose() throws -> NativeFuture<Void>? {{
  try nativeStartClose("{function.name}") {{ raw, completion, diagnostic in {native_call(function, "raw", "completion")} }}
}}
""",
            owner,
        )
    # The closure's parameters select the overload: a status-returning close
    # takes the diagnostic, so its status is always checked.
    call = native_call(function, "raw")
    parameters = "raw" if function.return_type.kind == "void" else "raw, diagnostic"
    return (
        f"""{documented}func {method}() throws {{
  try nativeClose("{function.name}") {{ {parameters} in {call} }}
}}
""",
        owner,
    )


def owner_name(handle):
    """The public owner class of a handle plan."""
    from .swift import name

    return name(handle.stem) + "Handle"


def decision_handles(bound):
    return set(bound.decisions)


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
        cleanup = handle.dispose or handle.release
        function = bound.source.functions_by_name[cleanup]
        # Abandoned disposal has no caller to report a diagnostic to.
        expression = f"{cleanup}(raw{', nil' if function.diagnostic else ''})"
        body = (
            f"{expression}; return true"
            if function.return_type.kind == "void"
            else f"{expression} == MLN_STATUS_OK"
        )
        chunks.append(
            f"struct {raw}: NativeHandle {{ let raw: UInt64; func disposeAbandoned() -> Bool {{ {body} }} }}\n"
        )
        decision = handle.native in decision_handles(bound)
        parent = (
            f", parent: {owner_name(bound.handles[handle.parent])}"
            if handle.parent
            else ""
        )
        passed = ", parent: parent" if handle.parent else ""
        if decision:
            parent += ", pendingDecision: Bool = false"
            passed += ", pendingDecision: pendingDecision"
        from .swift import doc

        chunks.append(f'''{doc(bound, handle.native)}public final class {owner}: @unchecked Sendable, NativeReceiver {{
  let handle: NativeHandleBox<{raw}>
  init(adopting raw: {handle.native}{parent}) throws {{
    handle = try NativeHandleBox(typeName: "{owner}", handle: {raw}(raw: raw){passed})
  }}
  public var isClosed: Bool {{ handle.isClosed }}
  public var id: UInt64 {{ handle.issued.raw }}
  func requireLiveHandle() throws -> {raw} {{ try handle.requireLive() }}
}}
''')
    return "\n".join(chunks)
