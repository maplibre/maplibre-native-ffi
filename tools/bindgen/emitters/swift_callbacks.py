"""Render Swift callback descriptors from registration metadata."""

from ..model import ModelError
from .swift import SCALARS, camel, checked, identifier, name, native_call


def contains_callback(value):
    return (
        bool(value.registration)
        or value.kind == "callback"
        or any(contains_callback(field.value) for field in value.fields)
    )


def closure_type(values, callback):
    parameters = [
        values.public(parameter.value)
        for parameter in callback.parameters
        if parameter.name != callback.context
    ]
    result = (
        "Void"
        if callback.result.native == "void"
        or any(
            parameter.value.element and parameter.value.element.response
            for parameter in callback.parameters
        )
        else values.public(callback.result)
    )
    return f"@Sendable ({', '.join(parameters)}) throws -> {result}"


def raw_type(type_):
    if type_.kind == "pointer":
        if type_.pointee.kind == "void":
            return (
                "UnsafeRawPointer?"
                if type_.pointee.const
                else "UnsafeMutableRawPointer?"
            )
        return f"{'UnsafePointer' if type_.pointee.const else 'UnsafeMutablePointer'}<{raw_type(type_.pointee)}> ?".replace(
            "> ?", ">?"
        )
    return SCALARS.get(
        type_.spelling,
        type_.declaration
        or SCALARS.get(
            type_.canonical,
            {
                "char": "CChar",
                "void": "Void",
                "int": "CInt",
                "unsigned int": "CUnsignedInt",
            }.get(
                type_.spelling.removeprefix("const "),
                type_.spelling.removeprefix("const "),
            ),
        ),
    )


def descriptor(values, plan):
    from .swift_dynamic_values import decode, encode

    registration = plan.registration
    public = values.public(plan)
    callbacks = set(registration.callbacks)
    fields, arguments, assignments, materialize, defaults = [], [], [], [], []
    for field in plan.fields:
        local = identifier(camel(field.name))
        if field.role == "count":
            array = next(
                member for member in plan.fields if member.value.length == field.name
            )
            source = identifier(camel(array.name))
            length = (
                f"{source}?.count ?? 0"
                if array.value.nullable or array.value.optional
                else f"{source}.count"
            )
            materialize.append(
                f"    raw.{identifier(field.name)} = try NativeInputArena.count({length})"
            )
            continue
        if not field.public:
            continue
        if field.name in callbacks:
            callback = values.bound.callbacks[field.value.native]
            typ = closure_type(values, callback)
            fields.append(f"  public var {local}: ({typ})?")
            arguments.append(f"{local}: ({typ})? = nil")
            assignments.append(f"    self.{local} = {local}")
            defaults.append(f"    self.{local} = nil")
            materialize.append(
                f"    raw.{identifier(field.name)} = {local} == nil ? nil : invoke{public}{name(field.name)}"
            )
        else:
            optional = field.presence and field.presence.mask
            typ = values.public(field.value) + ("?" if optional else "")
            fields.append(f"  public var {local}: {typ}")
            arguments.append(f"{local}: {typ} = {public}.default.{local}")
            assignments.append(f"    self.{local} = {local}")
            captured = decode(values, field.value, "raw." + identifier(field.name))
            if optional:
                path = ".".join(
                    identifier(part) for part in field.presence.mask.split(".")
                )
                bit = field.presence.bit
                present = f"raw.{path} & {bit}.rawValue != 0" if bit else f"raw.{path}"
                mark = f"raw.{path} |= {bit}.rawValue" if bit else f"raw.{path} = true"
                defaults.append(f"    self.{local} = {present} ? {captured} : nil")
                materialize.append(
                    f"    if let item = {local} {{ {mark}; raw.{identifier(field.name)} = {encode(values, field.value, 'item')} }}"
                )
            else:
                defaults.append(f"    self.{local} = {captured}")
                materialize.append(
                    f"    raw.{identifier(field.name)} = {encode(values, field.value, local)}"
                )
    initial = f"{plan.default}()" if plan.default else f"{plan.native}()"
    root_needed = " || ".join(
        f"{identifier(camel(field))} != nil" for field in registration.callbacks
    )
    sized = (
        f"\n    raw.size = UInt32(MemoryLayout<{plan.native}>.size)"
        if any(field.role == "size" for field in plan.fields)
        else ""
    )
    declarations = f"""public struct {public}: Sendable {{
{chr(10).join(fields)}
  public init({", ".join(arguments)}) {{
{chr(10).join(assignments)}
  }}
  public static var `default`: Self {{ try! Self(raw: {initial}) }}
  init(raw: {plan.native}) throws {{
{chr(10).join(defaults)}
  }}
  func nativeValue(arena: NativeInputArena) throws -> {plan.native} {{
    var raw = {initial}{sized}
{chr(10).join(materialize)}
    if {root_needed} {{
      raw.{registration.user_data} = arena.callback(self)
      raw.{registration.release} = releaseGeneratedCallback
    }}
    return raw
  }}
}}
"""
    for field in plan.fields:
        if field.name not in callbacks:
            continue
        callback = values.bound.callbacks[field.value.native]
        parameters = ", ".join(
            f"{identifier(parameter.name)}: {raw_type(parameter.value.ctype)}"
            for parameter in callback.parameters
        )
        result = raw_type(callback.result.ctype)
        result = "Void" if callback.result.native == "void" else result
        responses = [
            parameter
            for parameter in callback.parameters
            if parameter.value.element and parameter.value.element.response
        ]
        args = ", ".join(
            "decisionOwner"
            if callback.decision and parameter.name == callback.decision.parameter
            else "response" + name(parameter.name)
            if parameter in responses
            else decode(values, parameter.value, identifier(parameter.name))
            for parameter in callback.parameters
            if parameter.name != callback.context
        )
        typed_enum = callback.result.ctype.canonical.startswith("enum ")
        suffix = "" if typed_enum else ".rawValue"
        fail = "" if result == "Void" else f"return {callback.failure}{suffix}"
        setup = "".join(
            f"  guard let {identifier(parameter.name)} else {{ {fail or 'return'} }}\n  let response{name(parameter.name)} = {values.public(parameter.value.element)}(pointer: {identifier(parameter.name)})\n  defer {{ response{name(parameter.name)}.expire() }}\n"
            for parameter in responses
        )
        if callback.decision:
            decision = callback.decision
            from .swift_ownership import owner_name

            setup += f"  guard let decisionOwner = try? {owner_name(decision.handle)}(adopting: {identifier(decision.parameter)}, pendingDecision: true) else {{ {fail} }}\n"
        guard = ""
        if callback.reentry_policy:
            policy = callback.reentry_policy
            operations = ", ".join(f'"{operation}"' for operation in policy.operations)
            declarations += f"private let allowed{public}{name(field.name)}: Set<String> = [{operations}]\n"
            policy_owner = (
                "decisionOwner"
                if callback.decision
                else "response" + name(policy.owner_parameter)
            )
            guard = f"  let admission = NativeCallbackGuard.enter(owner: {policy_owner}, operations: allowed{public}{name(field.name)})\n  defer {{ admission.end() }}\n"
        elif callback.reentry == "forbid":
            guard = "  let admission = NativeCallbackGuard.enter(owner: nil, operations: [])\n  defer { admission.end() }\n"
        call = f"box.value.{identifier(camel(field.name))}?({args})"
        invoke = (
            f"try {call}"
            if result == "Void" or responses
            else f"return try {call} ?? {callback.failure}{suffix}"
        )
        if callback.decision:
            decision = callback.decision
            invoke = f"let accepted = try {call}.rawValue == {decision.accept}{suffix}; return decisionOwner.handle.finishDecision(accepted: accepted) ? {decision.accept}{suffix} : {decision.pass_through}{suffix}"
            fail = f"return decisionOwner.handle.finishDecision(accepted: false) ? {decision.accept}{suffix} : {decision.pass_through}{suffix}"
        if responses:
            invoke += f"; return MLN_STATUS_OK{suffix}"
        declarations += f"""private func invoke{public}{name(field.name)}({parameters}) -> {result} {{
  guard let {identifier(callback.context)} else {{ {"return " + callback.failure + suffix if result != "Void" else "return"} }}
  let box = Unmanaged<GeneratedCallbackBox<{public}>>.fromOpaque({identifier(callback.context)}).takeUnretainedValue()
{setup}{guard}  do {{ {invoke} }} catch {{ NativeDiagnostics.report(.callbackError(callback: "{callback.native}", error: error)); {fail} }}
}}
"""
    return declarations


def direct_operation(plan, values):
    from .swift_dynamic_values import decode
    from .swift_ownership import owner_name

    (registration,) = plan.direct_registrations
    parameter = next(
        parameter
        for parameter in plan.inputs
        if parameter.name == registration.callback
    )
    callback = values.bound.callbacks[parameter.value.native]
    values.add(parameter.value)
    typ = closure_type(values, callback)
    function = plan.function.name
    receiver = next(
        (
            parameter.value
            for parameter in plan.inputs
            if parameter.name == plan.receiver
        ),
        None,
    )
    owner = owner_name(receiver.handle) if receiver else "Maplibre"
    method = identifier(camel(plan.member))
    thunk = "invoke" + name(function)
    params = ", ".join(
        f"{identifier(parameter.name)}: {raw_type(parameter.value.ctype)}"
        for parameter in callback.parameters
    )
    args = ", ".join(
        decode(values, parameter.value, identifier(parameter.name))
        for parameter in callback.parameters
        if parameter.name != callback.context
    )
    result = (
        "Void" if callback.result.native == "void" else raw_type(callback.result.ctype)
    )
    fail = "return" if result == "Void" else "return " + callback.failure
    invoke = (
        f"try box.value({args})"
        if result == "Void"
        else f"return try box.value({args})"
    )
    if not registration.release_callback:
        raise ModelError([f"{function}: direct callback requires a native release"])
    policy = callback.reentry_policy
    if policy and not (receiver and policy.registration_owner):
        raise ModelError([f"{function}: direct callback requires a registration owner"])
    # A callback restricted to its registration owner carries that owner weakly,
    # so the native root does not keep the handle alive.
    stored = f"NativeOwnedCallback<{typ}>" if policy else typ
    lookup = f"guard let {identifier(callback.context)} else {{ {fail} }}\n  let box = Unmanaged<GeneratedCallbackBox<{stored}>>.fromOpaque({identifier(callback.context)}).takeUnretainedValue()"
    if policy:
        operations = ", ".join(f'"{operation}"' for operation in policy.operations)
        lookup += f"\n  guard let owner = box.value.owner else {{ {fail} }}"
        guard = f"let admission = NativeCallbackGuard.enter(owner: owner, operations: [{operations}])"
        invoke = invoke.replace("box.value(", "box.value.value(")
    else:
        guard = "let admission = NativeCallbackGuard.enter(owner: nil, operations: [])"
    optional = receiver is None
    arguments = []
    for parameter in plan.function.parameters:
        if parameter.name == plan.receiver:
            arguments.append("access.handle.raw")
        elif parameter.name == registration.callback:
            arguments.append(f"callback == nil ? nil : {thunk}" if optional else thunk)
        elif parameter.name == registration.user_data:
            arguments.append("token")
        elif parameter.name == registration.release_callback:
            arguments.append("releaseGeneratedCallback")
        elif parameter.name == registration.accepted_unless:
            arguments.append("&rejected")
        else:
            raise ModelError([f"{function}: unsupported direct callback parameter"])
    value = (
        "NativeOwnedCallback(owner: self, value: callback)" if policy else "callback"
    )
    token = (
        "let token = callback.map { arena.callback($0) }"
        if optional
        else f"let token = arena.callback({value})"
    )
    call = native_call(plan.function, *arguments)
    lines = [
        "let arena = NativeInputArena()",
        "defer { withExtendedLifetime(arena) {} }",
    ]
    if receiver:
        lines += [
            "let access = try handle.borrow()",
            "defer { access.end(); withExtendedLifetime(self) {} }",
        ]
    lines.append(token)
    if registration.accepted_unless:
        # A rejected registration stores nothing, so the arena releases it.
        lines += [
            "var rejected = false",
            checked(call),
            "if !rejected { arena.accept() }",
            "return rejected",
        ]
        result_type = " -> Bool"
    else:
        lines.append(checked(f"arena.submit {{ {call} }}"))
        result_type = ""
    parameter_type = f"({typ})?" if optional else f"@escaping {typ}"
    body = f"""  {"static " if optional else ""}func {method}(_ callback: {parameter_type}) throws{result_type} {{
    {"try NativeAbi.ensureCompatible()" + chr(10) + "    " if optional else ""}try NativeCallbackGuard.check(owner: {"nil" if optional else "self"}, operation: "{function}")
    {"return " if result_type else ""}try mapNativeFailure {{
      {(chr(10) + "      ").join(lines)}
    }}
  }}"""
    return (
        f"""public extension {owner} {{
{body}
}}
private func {thunk}({params}) -> {result} {{
  {lookup}
  {guard}
  defer {{ admission.end() }}
  do {{ {invoke} }} catch {{ NativeDiagnostics.report(.callbackError(callback: "{callback.native}", error: error)); {fail} }}
}}
""",
        None,
    )
