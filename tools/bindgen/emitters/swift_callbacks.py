"""Render Swift callback descriptors from registration metadata."""

from dataclasses import replace

from .. import docs
from ..model import ModelError
from ..semantic import required_callbacks
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


def alias(callback):
    """The public type alias of a callback type."""
    return name(callback.native.removeprefix("mln_"))


def closure_type(values, callback):
    """The closure type, whose parameter names document the C parameters,
    since a Swift function type takes no argument labels."""
    parameters = [
        f"_ {identifier(camel(parameter.name))}: {values.public(parameter.value)}"
        for parameter in callback.arguments
    ]
    # A status callback returns nothing and throws to fail.
    result = (
        "Void"
        if callback.result.native == "void" or callback.status
        else values.public(callback.result)
    )
    effect = " throws" if throws(callback) else ""
    return f"@Sendable ({', '.join(parameters)}){effect} -> {result}"


def declaration(values, value):
    """The public type alias of a callback value."""
    callback = values.bound.callbacks[value.native]
    if callback.status:
        fallback = camel(callback.failure.removeprefix("MLN_STATUS_").lower())
        note = (
            "Returning succeeds. Throwing a `MaplibreError` whose status is not "
            "`Status.ok` fails the call with that status. Any other thrown error "
            f"fails it with `Status.{fallback}` and is reported to standard "
            "error."
        )
    elif throws(callback):
        note = (
            f"A thrown error answers `{callback.failure}` and is reported to "
            "standard error."
        )
    else:
        note = None
    found = values.bound.doc(value.native)
    if found is not None and note:
        found = replace(found, notes=(*found.notes, note))
    return f"{docs.line_comment(found)}public typealias {alias(callback)} = {closure_type(values, callback)}\n"


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


def unsupported_default(plan, field):
    message = (
        f"{plan.native}.{field.name}: a registration that requires a callback "
        "takes no native default for a field without presence"
    )
    raise ModelError([message])


def descriptor(values, plan):
    from .swift_dynamic_values import decode, encode, zero

    registration = plan.registration
    public = values.public(plan)
    callbacks = set(registration.callbacks)
    required = {f.name for f in required_callbacks(plan)}
    fields, arguments, assignments, materialize, defaults = [], [], [], [], []
    for field in plan.fields:
        local = identifier(camel(field.name))
        if field.role == "count":
            array = next(
                member for member in plan.fields if member.value.length == field.name
            )
            source = identifier(camel(array.name))
            length = (
                f"{source}?.count ?? 0" if array.value.nullable else f"{source}.count"
            )
            materialize.append(
                f"    raw.{identifier(field.name)} = try NativeInputArena.count({length})"
            )
            continue
        if not field.public:
            continue
        if field.name in required:
            typ = alias(values.bound.callbacks[field.value.native])
            fields.append(
                f"{doc(values.bound, f'{plan.native}.{field.name}', '  ')}  public var {local}: {typ}"
            )
            arguments.append(f"{local}: @escaping {typ}")
            assignments.append(f"    self.{local} = {local}")
            defaults.append(f"    self.{local} = {local}")
            materialize.append(
                f"    raw.{identifier(field.name)} = invoke{public}{name(field.name)}"
            )
        elif field.name in callbacks:
            callback = values.bound.callbacks[field.value.native]
            typ = alias(callback)
            fields.append(
                f"{doc(values.bound, f'{plan.native}.{field.name}', '  ')}  public var {local}: {typ}?"
            )
            arguments.append(f"{local}: {typ}? = nil")
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
                    # A registration that requires a callback has no default
                    # value, so an omitted field leaves the native default.
                    "nil"
                    if required and optional
                    else unsupported_default(plan, field)
                    if required and plan.default
                    else f"{public}.default.{local}"
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
    root = (
        f"    raw.{registration.user_data} = arena.callback(self)\n"
        f"    raw.{registration.release} = releaseGeneratedCallback"
    )
    if not required:
        root_needed = " || ".join(
            f"{identifier(camel(field))} != nil" for field in registration.callbacks
        )
        root = f"    if {root_needed} {{\n" + root.replace("    ", "      ") + "\n    }"

    sized = (
        f"\n    raw.size = UInt32(MemoryLayout<{plan.native}>.size)"
        if any(field.role == "size" for field in plan.fields)
        else ""
    )
    # Native returns a registration only as its own default, so a registration
    # without one has no copy.
    # A registration that requires a callback has no default value, and its
    # copy takes the required callbacks.
    taken = "".join(
        f", {identifier(camel(f.name))}: @escaping {alias(values.bound.callbacks[f.value.native])}"
        for f in required_callbacks(plan)
    )
    copy = (
        (
            ""
            if required
            else f"  public static var `default`: Self {{ try! Self(raw: nativeDefault({plan.default})) }}\n"
        )
        + f"""  init(raw: {plan.native}{taken}) throws {{
{chr(10).join(defaults)}
  }}
"""
        if plan.native in values.bound.returned
        else ""
        if required
        else "  public static var `default`: Self { Self() }\n"
    )
    stored = public
    declarations = f"""{doc(values.bound, plan.native)}public struct {public}: Sendable {{
{chr(10).join(fields)}
  public init({", ".join(arguments)}) {{
{chr(10).join(assignments)}
  }}
{copy}  func nativeValue(arena: NativeInputArena) throws -> {plan.native} {{
    var raw = {initial}{sized}
{chr(10).join(materialize)}
{root}
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
        boolean = callback.failure in {"true", "false"}
        suffix = "" if typed_enum or numeric or boolean else ".rawValue"
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
        optional = "" if field.name in required else "?"
        call = f"{'try ' if throws(callback) else ''}{value}.{identifier(camel(field.name))}{optional}({args})"
        invoke = (
            f"{call}"
            if result == "Void" or callback.status
            else f"return {call}"
            if not optional
            else f"return {call} ?? {callback.failure}{suffix}"
        )
        if callback.status:
            # A status callback succeeds by returning and fails by throwing.
            invoke += f"; return MLN_STATUS_OK{suffix}"
        # The synchronous release of a handle whose release is also its
        # dispose is the dispose.
        disposal = "".join(
            f"try? owned{name(p.name)}.dispose(); "
            if p.value.handle.dispose_completion
            else f"owned{name(p.name)}.close(); "
            if values.bound.source.functions_by_name[
                p.value.handle.release
            ].return_type.kind
            == "void"
            else f"try? owned{name(p.name)}.close(); "
            for p in owned
        )
        fail = disposal + fail
        # A thrown MaplibreError with a failing status is a deliberate failure,
        # which native receives without a report.
        deliberate = (
            f" catch let error as MaplibreError where error.status != .ok {{ {disposal}return mln_status(rawValue: error.status.rawValue) }}"
            if callback.status
            else ""
        )
        body = (
            f'  do {{ {invoke} }}{deliberate} catch {{ reportToStandardError("{callback.native} failed: \\(error)\\n"); {fail} }}'
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
