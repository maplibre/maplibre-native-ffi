"""Compile callback signatures, C entry points, and Go lifetime scopes."""

from dataclasses import replace

from ..model import ModelError
from .go import native_call
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
                if policy and policy.registration_owner
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
        if values.api.callbacks[release_type].reentry == "forbid"
        else "binding_release"
    )
    lines = [
        f"if {enabled} {{",
        f"raw.{field(descriptor.user_data)} = arena.register(input, 0)",
        f"raw.{field(descriptor.release)} = (C.{release_type})(C.{release})",
    ]
    for member in descriptor.callbacks:
        ctype = next(f.value.native for f in plan.fields if f.name == member)
        lines.append(
            f"if input.{name(member)} != nil {{ raw.{field(member)} = (C.{ctype})(C.binding_{plan.native}_{member}) }}"
        )
    lines.append("}")
    return "; ".join(lines)


def direct_operation(plan, values):
    from ..semantic import FieldPlan, RegistrationDescriptorPlan, ValuePlan

    registration = plan.direct_registrations[0]
    if not registration.release_callback:
        raise ModelError([f"{plan.name}: callback requires a release relationship"])
    callback_parameter = next(p for p in plan.inputs if p.name == registration.callback)
    callback = values.api.callbacks[callback_parameter.value.native]
    descriptor = ValuePlan(
        kind="record",
        native=plan.name + "_registration",
        ctype=callback_parameter.value.ctype,
        fields=(FieldPlan(registration.callback, callback_parameter.value),),
        registration=RegistrationDescriptorPlan(
            (registration.callback,),
            registration.user_data,
            registration.release_callback,
        ),
    )
    validate(values, descriptor)
    values.records[descriptor.native] = descriptor
    typename = public(descriptor.native)
    callback_type = signature(values, callback)
    symbol = "binding_" + descriptor.native + "_" + registration.callback
    args = {
        registration.callback: "nativeCallback",
        registration.user_data: "context",
        registration.release_callback: "nativeRelease",
    }
    setup = ["arena := &bindingArena{}; defer arena.close()"]
    receiver = next((p for p in plan.inputs if p.name == plan.receiver), None)
    identity = "receiver.state.issued" if receiver else "0"
    setup.append(f"bindingAdmission(C.binding_operation_{plan.name}, {identity})")
    setup.append(
        f"var context unsafe.Pointer; if callback != nil {{ context = arena.register({typename}{{{name(registration.callback)}: callback}}, {identity}) }}"
    )
    release_parameter = next(
        p for p in plan.inputs if p.name == registration.release_callback
    )
    release = (
        "binding_release_forbid"
        if values.api.callbacks[release_parameter.value.native].reentry == "forbid"
        else "binding_release"
    )
    setup.append(
        f"var nativeCallback C.{callback.native}; var nativeRelease C.{release_parameter.value.native}; if callback != nil {{ nativeCallback = (C.{callback.native})(C.{symbol}); nativeRelease = (C.{release_parameter.value.native})(C.{release}) }}"
    )
    owner = "nil"
    if receiver:
        setup.insert(
            0,
            'if receiver == nil || receiver.bindingOwner == nil { panic(bindingFailure{newBindingError(ErrInvalidState,"nil handle")}) }',
        )
        args[receiver.name] = f"C.{receiver.value.native}(raw)"
        setup.append("raw, done := receiver.bindingAcquire(false); defer done()")
        owner = "receiver.bindingOwner"
    result, returned, accept = "struct{}", "struct{}{}", f"arena.accept({owner})"
    if registration.accepted_unless:
        output = next(p for p in plan.outputs if p.name == registration.accepted_unless)
        if len(plan.outputs) != 1:
            raise ModelError([f"{plan.name}: unsupported direct callback outputs"])
        args[output.name] = "&rejected"
        setup.append(f"var rejected {values.c_type(output.value.element)}")
        result, returned = "bool", "bool(rejected)"
        # A rejected registration stores nothing, so the arena releases it.
        accept = f"if !bool(rejected) {{ {accept} }}"
    elif plan.outputs:
        raise ModelError([f"{plan.name}: unsupported direct callback outputs"])
    call = native_call(plan.function, *(args[p.name] for p in plan.function.parameters))
    setup.append(
        f"bindingCheck(func(diagnostic *C.mln_diagnostic) int32 {{ return int32({call}) }})"
    )
    setup.append(accept)
    setup.append(f"return {returned}")
    body = f"bindingCall(func() {result} {{ {'; '.join(setup)} }})"
    if receiver:
        signature_ = f"func (receiver *{values.owner(receiver.value.native)}) {name(plan.member)}(callback {callback_type})"
    else:
        signature_ = f"func {name(plan.member)}(callback {callback_type})"
    if result == "bool":
        return f"{signature_} (bool, error) {{ return {body} }}"
    return f"{signature_} error {{ _, err := {body}; return err }}"
