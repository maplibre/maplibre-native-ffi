"""Render Swift callback descriptors from registration metadata."""

from .swift import SCALARS, camel, doc, identifier, name


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
    from .swift_dynamic_values import decode, encode, zero

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
            fields.append(
                f"{doc(values.bound, f'{plan.native}.{field.name}', '  ')}  public var {local}: ({typ})?"
            )
            arguments.append(f"{local}: ({typ})? = nil")
            assignments.append(f"    self.{local} = {local}")
            defaults.append(f"    self.{local} = nil")
            materialize.append(
                f"    raw.{identifier(field.name)} = {local} == nil ? nil : invoke{public}{name(field.name)}"
            )
        else:
            optional = field.presence and field.presence.mask
            typ = values.public(field.value) + ("?" if optional else "")
            fields.append(
                f"{doc(values.bound, f'{plan.native}.{field.name}', '  ')}  public var {local}: {typ}"
            )
            arguments.append(
                f"{local}: {typ} = "
                + (
                    f"{public}.default.{local}"
                    if plan.default
                    else zero(field.value, typ)
                )
            )
            assignments.append(f"    self.{local} = {local}")
            captured = decode(values, field.value, "raw." + identifier(field.name))
            if optional:
                path = ".".join(
                    identifier(part) for part in field.presence.mask.split(".")
                )
                bit = field.presence.bit
                present = f"raw.{path} & {bit}.rawValue != 0"
                mark = f"raw.{path} |= {bit}.rawValue"
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
    # Native returns a registration only as its own default, so a registration
    # without one has no copy.
    copy = (
        f"""  public static var `default`: Self {{ try! Self(raw: {initial}) }}
  init(raw: {plan.native}) throws {{
{chr(10).join(defaults)}
  }}
"""
        if plan.native in values.bound.returned
        else "  public static var `default`: Self { Self() }\n"
    )
    # A callback that calls back only into the receiver that registered it
    # carries that receiver weakly, so the native root does not keep it alive.
    rooted = (
        "NativeOwnedCallback(owner: arena.receiver, value: self)"
        if registration.receiver_owned
        else "self"
    )
    stored = f"NativeOwnedCallback<{public}>" if registration.receiver_owned else public
    declarations = f"""{doc(values.bound, plan.native)}public struct {public}: Sendable {{
{chr(10).join(fields)}
  public init({", ".join(arguments)}) {{
{chr(10).join(assignments)}
  }}
{copy}  func nativeValue(arena: NativeInputArena) throws -> {plan.native} {{
    var raw = {initial}{sized}
{chr(10).join(materialize)}
    if {root_needed} {{
      raw.{registration.user_data} = arena.callback({rooted})
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
        # A failure that names a constant of a C enum converts to the raw
        # integer result; a numeric failure is that result already.
        numeric = bool(callback.failure) and callback.failure.lstrip("-").isdigit()
        suffix = "" if typed_enum or numeric else ".rawValue"
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
                else "receiver"
                if policy.owner_parameter is None
                else "response" + name(policy.owner_parameter)
            )
            guard = f"  let admission = NativeCallbackGuard.enter(owner: {policy_owner}, operations: allowed{public}{name(field.name)})\n  defer {{ admission.end() }}\n"
        elif callback.reentry == "forbid":
            guard = "  let admission = NativeCallbackGuard.enter(owner: nil, operations: [])\n  defer { admission.end() }\n"
        value = "box.value.value" if registration.receiver_owned else "box.value"
        if registration.receiver_owned:
            setup += f"  guard let receiver = box.value.owner else {{ {fail or 'return'} }}\n"
        call = f"{value}.{identifier(camel(field.name))}?({args})"
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
  let box = Unmanaged<GeneratedCallbackBox<{stored}>>.fromOpaque({identifier(callback.context)}).takeUnretainedValue()
{setup}{guard}  do {{ {invoke} }} catch {{ NativeDiagnostics.report(.callbackError(callback: "{callback.native}", error: error)); {fail} }}
}}
"""
    return declarations
