"""Render Swift callback descriptors from registration metadata."""

from .swift import SCALARS, camel, doc, identifier, name


def contains_callback(value):
    return (
        bool(value.registration)
        or value.kind == "callback"
        or any(contains_callback(field.value) for field in value.fields)
    )


def throws(callback):
    """Whether a host closure may throw, since native has a fallback result."""
    return callback.result.native != "void"


def closure_type(values, callback):
    parameters = [values.public(parameter.value) for parameter in callback.arguments]
    result = (
        "Void"
        if callback.result.native == "void"
        or any(
            parameter.value.element and parameter.value.element.response
            for parameter in callback.parameters
        )
        else values.public(callback.result)
    )
    effect = " throws" if throws(callback) else ""
    return f"@Sendable ({', '.join(parameters)}){effect} -> {result}"


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
                path = identifier(field.presence.mask)
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
        f"""  public static var `default`: Self {{ try! Self(raw: nativeDefault({plan.default})) }}
  init(raw: {plan.native}) throws {{
{chr(10).join(defaults)}
  }}
"""
        if plan.native in values.bound.returned
        else "  public static var `default`: Self { Self() }\n"
    )
    rooted = "self"
    stored = public
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
        # A handle the callback owns is adopted before the host runs, and
        # closed when the host throws, so native never waits on it.
        owned = [
            parameter
            for parameter in callback.arguments
            if parameter.value.kind == "handle" and parameter.value.ownership == "owned"
        ]
        args = ", ".join(
            "owned" + name(parameter.name)
            if parameter in owned
            else "response" + name(parameter.name)
            if parameter in responses
            else decode(values, parameter.value, identifier(parameter.name))
            for parameter in callback.arguments
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
        from .swift_ownership import disposer_arguments, owner_name

        for parameter in owned:
            handle = parameter.value.handle
            release = values.bound.source.functions_by_name[handle.release]
            arguments = disposer_arguments(handle, release, identifier(parameter.name))
            if release.diagnostic:
                arguments.append("nil")
            setup += f"  guard let owned{name(parameter.name)} = try? {owner_name(handle)}(adopting: {identifier(parameter.name)}) else {{ {handle.release}({', '.join(arguments)}); {fail or 'return'} }}\n"
        value = "box.value"
        call = f"{'try ' if throws(callback) else ''}{value}.{identifier(camel(field.name))}?({args})"
        invoke = (
            f"{call}"
            if result == "Void" or responses
            else f"return {call} ?? {callback.failure}{suffix}"
        )

        if responses:
            invoke += f"; return MLN_STATUS_OK{suffix}"
        if owned:
            # The synchronous release of a handle whose release is also its
            # dispose is the dispose.
            fail = " ".join(
                f"try? owned{name(p.name)}.dispose();"
                if p.value.handle.dispose_completion
                else f"owned{name(p.name)}.close();"
                if values.bound.source.functions_by_name[
                    p.value.handle.release
                ].return_type.kind
                == "void"
                else f"try? owned{name(p.name)}.close();"
                for p in owned
            ) + (" " + fail if fail else "")
        body = (
            f'  do {{ {invoke} }} catch {{ reportToStandardError("{callback.native} failed and native received its fallback: \\(error)\\n"); {fail} }}'
            if "try " in invoke
            else f"  {invoke}"
        )
        declarations += f"""private func invoke{public}{name(field.name)}({parameters}) -> {result} {{
  guard let {identifier(callback.context)} else {{ {"return " + callback.failure + suffix if result != "Void" else "return"} }}
  let box = Unmanaged<GeneratedCallbackBox<{stored}>>.fromOpaque({identifier(callback.context)}).takeUnretainedValue()
{setup}{body}
}}
"""
    return declarations
