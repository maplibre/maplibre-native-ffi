"""Compile callback signatures, C entry points, and Go lifetime scopes."""

from dataclasses import replace

from ..model import ModelError
from .go_values import field, name, public


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
        for p in callback.parameters:
            if (
                p.name == callback.context
                or callback.decision
                and p.name == callback.decision.parameter
            ):
                continue
            values.require(p.value)
        if callback.result.native != "void":
            values.require(callback.result)


def signature(values, callback):
    args = [
        values.type(p.value) for p in callback.parameters if p.name != callback.context
    ]
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
        policy = callback.reentry_policy
        guard, unguard = "", ""
        if callback.reentry != "allow":
            operations = policy.operations if policy else ()
            names = ", ".join("binding_operation_" + op for op in operations) or "0"
            owner = (
                f"mlnGoCallbackOwner({callback.context})"
                if policy and policy.owner_parameter is None
                else f"(uint64_t)(uintptr_t){policy.owner_parameter}"
                if policy
                else "0"
            )
            guard = f"static const uint32_t operations[] = {{{names}}}; binding_policy policy = {{NULL, operations, {len(operations)}, {owner}}}; binding_policy_enter(&policy);"
            unguard = "binding_policy_leave(&policy);"
        call = f"{export}({arguments})"
        body = (
            f"{call}; {unguard}"
            if ret == "void"
            else f"{ret} result = {call}; {unguard} return result;"
        )
        c.append(f"{ret} {symbol}({params}) {{ {guard} {body} }}")
        parameters = [p for p in callback.parameters if p.name != callback.context]
        go_params = ", ".join(
            "native_" + field(p.name) + " " + values.c_type(p.value)
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
        setup = ["runtime.LockOSThread(); defer runtime.UnlockOSThread()", failure]
        copied = []
        decision = callback.decision
        if decision:
            owner = values.owner(decision.handle.native)
            setup.append(
                f"request := adopt{owner}(uint64(native_{decision.parameter}), nil); request.state.beginDecision(); defer func() {{ result = {values.c_type(callback.result)}(request.state.finishDecision(uint32(result), uint32(C.{decision.accept}), uint32(C.{decision.pass_through}))); runtime.KeepAlive(request) }}()"
            )
        setup.append(
            "defer func() { if failure := recover(); failure != nil { "
            + f'bindingReportCallbackPanic("{callback.native}", failure); '
            + failure
            + " } }()"
        )
        if any(
            (p.value.element if p.value.kind == "reference" else p.value).response
            for p in parameters
        ):
            setup.append("scope := bindingNewScope(); defer scope.alive.Store(false)")
        for p in parameters:
            value = p.value.element if p.value.kind == "reference" else p.value
            if decision and p.name == decision.parameter:
                copied.append("request")
            elif value.response:
                copied.append(
                    f"&{public(value.native)}Scope{{native: native_{p.name}, scope: scope}}"
                )
            else:
                copied.append(values.copy(p.value, "native_" + field(p.name)))
        setup.append(
            f"callbacks, ok := bindingCallbackValue[{public(plan.native)}](native_{callback.context}); if !ok || callbacks.{name(member)} == nil {{ return }}"
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


def registration_input(values, plan):
    descriptor = plan.registration
    enabled = " || ".join(
        "input." + name(member) + " != nil" for member in descriptor.callbacks
    )
    release_type = next(
        f.value.native for f in plan.fields if f.name == descriptor.release
    )
    release = (
        "binding_release_forbid"
        if descriptor.release_reentry == "forbid"
        else "binding_release"
    )
    # A callback without an owner parameter calls back only into the receiver
    # that registered it, which the ticket records.
    identity = "arena.identity" if descriptor.receiver_owned else "0"
    lines = [
        f"if {enabled} {{",
        f"raw.{field(descriptor.user_data)} = arena.register(input, {identity})",
        f"raw.{field(descriptor.release)} = (C.{release_type})(C.{release})",
    ]
    for member in descriptor.callbacks:
        ctype = next(f.value.native for f in plan.fields if f.name == member)
        lines.append(
            f"if input.{name(member)} != nil {{ raw.{field(member)} = (C.{ctype})(C.binding_{plan.native}_{member}) }}"
        )
    lines.append("}")
    return "; ".join(lines)
