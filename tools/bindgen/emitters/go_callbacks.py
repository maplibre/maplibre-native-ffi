"""Compile callback signatures, C entry points, and Go lifetime scopes."""

import textwrap
from dataclasses import replace

from .. import async_hosts
from ..model import ModelError
from .go import native_identifier
from .go_values import name, public


def plain(plan):
    registration = plan.registration
    omitted = {*registration.callbacks, registration.user_data, registration.release}
    return replace(
        plan,
        registration=None,
        fields=tuple(f for f in plan.fields if f.name not in omitted),
    )


def validate(values, plan):
    for f in plain(plan).fields:
        if f.public:
            values.require(f.value, input=True)
    for member in plan.registration.callbacks:
        callback = values.api.callbacks[
            next(f.value.native for f in plan.fields if f.name == member)
        ]
        if not callback.context:
            raise ModelError([f"{plan.native}: callback requires a context"])
        for p in callback.arguments:
            if callback.decision and p.name == callback.decision.parameter:
                continue
            values.require(p.value)
        if callback.result.native != "void":
            values.require(callback.result)


def signature(values, callback):
    args = [values.type(p.value) for p in callback.arguments]
    result = (
        "" if callback.result.native == "void" else " " + values.type(callback.result)
    )
    return "func(" + ", ".join(args) + ")" + result


def c_decl(value, identifier, mutable=False):
    spelling = value.ctype.spelling
    if mutable:
        spelling = spelling.replace("const ", "")
    return spelling + " " + identifier


def sources(values, plan):
    go, header, c = [], [], []
    descriptor = plan.registration
    for member in descriptor.callbacks:
        callback = values.api.callbacks[
            next(f.value.native for f in plan.fields if f.name == member)
        ]
        symbol = "binding_" + plan.native + "_" + member
        export = "mlnGo_" + plan.native + "_" + member
        ret = callback.result.ctype.spelling
        params = ", ".join(c_decl(p.value, p.name) for p in callback.parameters)
        mutable_params = ", ".join(
            c_decl(p.value, p.name, True) for p in callback.parameters
        )
        header.append(f"{ret} {symbol}({params});")
        c.append(f"extern {ret} {export}({mutable_params});")
        arguments = ", ".join(
            (
                "(" + p.value.ctype.spelling.replace("const ", "") + ")"
                if p.value.ctype.pointee
                else ""
            )
            + p.name
            for p in callback.parameters
        )
        call = f"{export}({arguments})"
        body = f"{call};" if ret == "void" else f"return {call};"
        c.append(f"{ret} {symbol}({params}) {{ {body} }}")
        parameters = list(callback.arguments)
        go_params = ", ".join(
            "native_" + native_identifier(p.name) + " " + values.c_type(p.value)
            for p in callback.parameters
        )
        go_ret = (
            "" if ret == "void" else " (result " + values.c_type(callback.result) + ")"
        )
        failure = (
            ""
            if ret == "void"
            else "result = "
            + (
                "C." + callback.failure
                if callback.failure.startswith("MLN_")
                else callback.failure
            )
        )
        setup = [failure]
        copied = []
        decision = callback.decision
        if decision:
            owner = values.owner(decision.handle.native)
            setup.append(
                f"request := adopt{owner}(uint64(native_{decision.parameter}), nil); defer func() {{ if uint32(result) != uint32(C.{decision.accept}) {{ request.state.released.Store(true) }}; runtime.KeepAlive(request) }}()"
            )
        # A handle the callback owns is adopted first; a callback that never
        # runs or panics closes it, so native never waits on it.
        owned = [
            p
            for p in parameters
            if p.value.kind == "handle"
            and p.value.ownership == "owned"
            and not (decision and p.name == decision.parameter)
        ]
        for p in owned:
            setup.append(
                f"owned_{p.name} := adopt{values.owner(p.value.native)}(uint64(native_{p.name}), nil)"
            )
        closes = "".join(f"owned_{p.name}.Close(); " for p in owned)
        setup.append(
            "defer func() { if failure := recover(); failure != nil { "
            + f'bindingReportCallbackPanic("{callback.native}", failure); '
            + closes
            + failure
            + " } }()"
        )
        if any((p.value.unwrapped).response for p in parameters):
            setup.append("scope := bindingNewScope(); defer scope.alive.Store(false)")
        for p in parameters:
            value = p.value.unwrapped
            if decision and p.name == decision.parameter:
                copied.append("request")
            elif value.kind == "handle" and value.ownership == "owned":
                copied.append("owned_" + p.name)
            elif value.response:
                copied.append(
                    f"&{public(value.native)}Scope{{native: native_{p.name}, scope: scope}}"
                )
            else:
                copied.append(
                    values.copy(p.value, "native_" + native_identifier(p.name))
                )
        setup.append(
            f"callbacks, ok := bindingCallbackValue[{public(plan.native)}](native_{callback.context}); if !ok || callbacks.{name(member)} == nil {{ {closes}return }}"
        )
        invoke = f"callbacks.{name(member)}({', '.join(copied)})"
        setup.append(
            invoke
            if ret == "void"
            else f"result = {values.c_type(callback.result)}({invoke})"
        )
        go.append(
            f"//export {export}\nfunc {export}({go_params}){go_ret} {{ {'; '.join(line for line in setup if line)}; return }}"
        )
    return "\n\n".join(go), "\n".join(header), "\n".join(c)


def accepts_native(values, plan):
    """Whether a registration also takes a native registration, whose function
    pointers, context, and release native code in the host supplies."""
    return async_hosts.accepts_native(values.api, plan)


def parameter(member):
    word = name(member)
    return native_identifier(word[:1].lower() + word[1:])


def native_constructor(values, plan):
    """The constructor of a registration's native form, which takes the C
    members of the registration in declaration order."""
    descriptor = plan.registration
    typename = public(plan.native)
    members = (*descriptor.callbacks, descriptor.user_data, descriptor.release)
    params = [parameter(member) for member in members]
    context, release = params[-2:]
    article = "an" if typename[0] in "AEIOU" else "a"
    listed = ", ".join(params[:-1]) + ", and " + params[-1]
    text = (
        f"NewNative{typename} returns {article} {typename} from C values that "
        f"native code in the host supplies: {listed} are the members of "
        f"{plan.native}, which the binding copies unchanged. The first call "
        f"that carries the {typename} consumes it. Native code releases "
        f"{context} if it accepts the call, and otherwise the binding calls "
        f"{release}. A later call returns StatusInvalidState."
    )
    comment = "".join(f"// {line}\n" for line in textwrap.wrap(text, 77))
    callbacks = ", ".join(params[:-2])
    return (
        comment
        + f"func NewNative{typename}({', '.join(params)} uintptr) {typename} {{ "
        + f"return {typename}{{native: bindingNewNativeRegistration({context}, {release}, {callbacks})}} }}"
    )


def native_input(values, plan):
    """Copies a registration's native form into its C struct unchanged."""
    descriptor = plan.registration
    typename = public(plan.native)

    def ctype(member):
        return next(f.value.native for f in plan.fields if f.name == member)

    def address(expr):
        return f"C.binding_address(C.uintptr_t({expr}))"

    funcs = " || ".join(
        "input." + name(member) + " != nil" for member in descriptor.callbacks
    )
    lines = [
        "registration := arena.native(input.native)",
        f'if {funcs} {{ arena.fail("{typename} carries both a native registration and a func") }}',
    ]
    for index, member in enumerate(descriptor.callbacks):
        callback = address(f"registration.callbacks[{index}]")
        lines.append(
            f"raw.{native_identifier(member)} = (C.{ctype(member)})({callback})"
        )
    lines += [
        f"raw.{native_identifier(descriptor.user_data)} = {address('registration.userData')}",
        f"raw.{native_identifier(descriptor.release)} = (C.{ctype(descriptor.release)})({address('registration.release')})",
    ]
    return "; ".join(lines)


def registration_input(values, plan):
    descriptor = plan.registration
    enabled = " || ".join(
        "input." + name(member) + " != nil" for member in descriptor.callbacks
    )
    release_type = next(
        f.value.native for f in plan.fields if f.name == descriptor.release
    )
    release = "binding_release"
    # A native registration takes the place of the funcs.
    native = (
        f"if input.native != nil {{ {native_input(values, plan)} }} else "
        if accepts_native(values, plan)
        else ""
    )
    lines = [
        f"{native}if {enabled} {{",
        f"raw.{native_identifier(descriptor.user_data)} = arena.register(input)",
        f"raw.{native_identifier(descriptor.release)} = (C.{release_type})(C.{release})",
    ]
    for member in descriptor.callbacks:
        ctype = next(f.value.native for f in plan.fields if f.name == member)
        lines.append(
            f"if input.{name(member)} != nil {{ raw.{native_identifier(member)} = (C.{ctype})(C.binding_{plan.native}_{member}) }}"
        )
    lines.append("}")
    return "; ".join(lines)
