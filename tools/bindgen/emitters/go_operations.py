"""Lower Go operations from shared ownership and value plans."""

from dataclasses import replace

from ..model import ModelError
from .go import GO_KEYWORDS, name
from .go_values import Values, absent, public


def element(value):
    return value.element if value.kind == "reference" else value


def operation(plan, values):
    if plan.role == "support":
        raise ModelError(
            [f"{plan.name}: support relationship is emitted with its owner or value"]
        )
    if plan.direct_registrations:
        from .go_callbacks import direct_operation

        return direct_operation(plan, values)
    receiver_name = plan.receiver or plan.scoped_receiver
    receiver = next((p for p in plan.inputs if p.name == receiver_name), None)
    handle = element(receiver.value) if receiver else None
    method = name(
        plan.function.metadata.get(
            "name",
            plan.name.removeprefix(
                (handle.native.removesuffix("_handle") + "_") if handle else "mln_"
            ),
        )
    )
    method = method.removeprefix("Mln")
    if handle and handle.handle and plan.name == handle.handle.release:
        method = "Close"
    elif receiver and method in {"Close", "IsClosed", "ID"}:
        raise ModelError([f"{plan.name}: {method} is reserved for owner state"])
    signature, setup, arguments = [], [], {}
    aliases = []
    public_names = set()
    internal_names = {f"input{i}" for i in range(len(plan.inputs))} | {
        "receiver",
        "callback",
    }
    count_parameters = {
        p.value.length
        for p in plan.inputs
        if p.value.kind in {"array", "buffer"} and p.value.length
    }
    for index, p in enumerate(plan.inputs):
        if p.name == receiver_name or p.name in count_parameters:
            continue
        value = p.value
        values.require(value, input=True)
        local = f"input{index}"
        parts = p.name.split("_")
        label = parts[0] + "".join(name(part) for part in parts[1:])
        while label in GO_KEYWORDS or label in internal_names or label in public_names:
            label += "_"
        public_names.add(label)
        signature.append(f"{label} {values.type(value)}")
        aliases.append(f"{local} := {label}")
        if value.kind == "handle":
            setup.append(
                f'if {local} == nil || {local}.bindingOwner == nil {{ arena.fail("nil input handle") }}; {local}Raw, {local}Done := {local}.bindingAcquire(false); defer {local}Done()'
            )
            arguments[p.name] = f"C.{value.native}({local}Raw)"
            continue
        if value.kind == "array":
            setup.append(f"var {local}Raw *C.{value.element.native}")
            # Parameter counts belong to the operation rather than a record.
            converted = values.native(
                replace(value, length=str(0)), local, local + "Raw"
            )
            setup.append(converted)
            if value.length and value.length.isdigit():
                setup.append(
                    f'if len({local}) != {value.length} {{ arena.fail("wrong fixed array length") }}'
                )
            elif value.length:
                count = next(p for p in plan.inputs if p.name == value.length)
                arguments[value.length] = (
                    f"bindingCount[C.{count.value.native}](len({local}))"
                )
        else:
            setup.append(f"var {local}Raw {values.c_type(value)}")
            converted_value = value
            if (
                value.kind == "buffer"
                and value.length
                and value.length != "nul"
                and not value.length.isdigit()
            ):
                converted_value = replace(value, length="0")
                count = next(p for p in plan.inputs if p.name == value.length)
                length = f"len({local})"
                if absent(value):
                    length = local + "Length"
                    setup.append(
                        f"{length} := 0; if {local} != nil {{ {length} = len(*{local}) }}"
                    )
                arguments[value.length] = (
                    f"bindingCount[C.{count.value.native}]({length})"
                )
            setup.append(values.native(converted_value, local, local + "Raw"))
        arguments[p.name] = local + "Raw"
    decision = next(
        (
            c.decision
            for c in values.api.callbacks.values()
            if c.decision and handle and c.decision.handle.native == handle.native
        ),
        None,
    )
    completing = bool(decision and plan.name == decision.complete)
    closing_decision = bool(decision and plan.name == decision.handle.release)
    if receiver:
        if completing:
            setup.insert(
                0,
                "raw, finishCompletion := receiver.state.reserveCompletion(); accepted := false; defer func() { finishCompletion(accepted); runtime.KeepAlive(receiver) }()",
            )
        elif plan.scoped_receiver:
            values.require(handle)
            setup.insert(0, "receiver.scope.check(); raw := receiver.native")
        elif plan.consumes:
            setup.insert(
                0,
                "raw, transaction := receiver.state.reserveClose(); defer transaction.finish(); defer runtime.KeepAlive(receiver)",
            )
        elif plan.receiver_access == "issued":
            setup.insert(
                0, "raw := receiver.state.issued; defer runtime.KeepAlive(receiver)"
            )
        else:
            setup.insert(
                0,
                "raw, done := receiver.bindingAcquire("
                + str(
                    any(
                        o.value.lifetime == "owner"
                        or element(o.value).lifetime == "owner"
                        for o in plan.outputs
                    )
                    or bool(plan.view)
                ).lower()
                + "); defer done()",
            )
        arguments[receiver.name] = (
            "raw" if plan.scoped_receiver else f"C.{handle.native}(raw)"
        )
        if receiver.value.kind == "reference" and not plan.scoped_receiver:
            setup.append(f"receiverRaw := C.{handle.native}(raw)")
            arguments[receiver.name] = "&receiverRaw"
    outputs = []
    for p in plan.outputs:
        value = element(p.value)
        values.require(value)
        local = "output" + name(p.name)
        init = (
            f"C.{value.default}()" if value.default else f"*new({values.c_type(value)})"
        )
        setup.append(f"{local} := {init}")
        for member in value.fields:
            if member.role == "size":
                setup.append(
                    f"{local}.{member.name} = {values.c_type(member.value)}(unsafe.Sizeof({local}))"
                )
        arguments[p.name] = "&" + local
        outputs.append((p, value, local))
    output_types = []
    conversions = []
    for p, value, local in outputs:
        output_types.append(values.type(value))
        if value.kind == "handle":
            parent = "receiver" if value.handle.parent and receiver else "nil"
            conversions.append(
                f"adopt{values.owner(value.native)}(uint64({local}), {parent})"
            )
        else:
            conversions.append(values.copy(value, local, local))
    completion_type = None
    converter = None
    if plan.completion:
        arguments[plan.completion.parameter] = "completion"
        result = plan.result
        if plan.execution == "command":
            completion_type, converter = "CommandCompletion", "completionCommand"
        elif result is None:
            completion_type, converter = "struct{}", "completionUnit"
        else:
            values.require(result)
            completion_type = values.type(result)
            if result.kind == "handle":
                parent = "receiver" if result.handle.parent and receiver else "nil"
                convert = f"adopt{values.owner(result.native)}(uint64(raw), {parent})"
                body = f"raw, err := completionValue[C.{result.native}](result); if err != nil {{ return nil, err }}; return {convert}, nil"
            elif result.kind == "array":
                copy = values.copy(result.element, "item")
                body = (
                    (
                        "if result.value == nil { return nil,nil }; "
                        if result.nullable
                        else ""
                    )
                    + f"items, err := completionSlice[C.{result.element.native}](result); if err != nil {{ return nil, err }}; copied := make({completion_type}, len(items)); for i,item := range items {{ copied[i] = {copy} }}; return copied,nil"
                )
            else:
                copy = values.copy(replace(result, nullable=False), "raw")
                if result.nullable:
                    body = f"if result.value == nil {{ return nil,nil }}; raw,err := completionValue[C.{result.native}](result); if err != nil {{ return nil,err }}; copied := {copy}; return &copied,nil"
                else:
                    body = f"raw,err := completionValue[C.{result.native}](result); if err != nil {{ var zero {completion_type}; return zero,err }}; return {copy},nil"
            converter = f"func(result *C.mln_completion_result) ({completion_type},error) {{ {body} }}"
        output_types.append(f"*Future[{completion_type}]")
        conversions.append("future")
    elif (
        not outputs
        and plan.function.return_type.canonical != "void"
        and plan.function.return_type.spelling != "mln_status"
    ):
        values.require(plan.result)
        output_types.append(values.type(plan.result))
        conversions.append(values.copy(plan.result, "nativeResult"))
    result_type = output_types[0] if len(output_types) == 1 else "struct{}"
    products = ""
    if len(output_types) > 1:
        result_type = name(plan.name.removeprefix("mln_")) + "Result"
        names = [name(p.name.removeprefix("out_")) for p, _, _ in outputs] + (
            ["Completion"] if plan.completion else []
        )
        products = (
            f"type {result_type} struct {{ "
            + "; ".join(f"{n} {t}" for n, t in zip(names, output_types, strict=True))
            + " }\n"
        )
        converted = (
            result_type
            + "{"
            + ", ".join(f"{n}: {c}" for n, c in zip(names, conversions, strict=True))
            + "}"
        )
    else:
        converted = conversions[0] if conversions else "struct{}{}"
    params = ", ".join(arguments[p.name] for p in plan.function.parameters)
    call = f"C.{plan.name}({params})"
    invocation = f"bindingCheck(func() int32 {{ return int32({call}) }})"
    if plan.completion:
        invocation = f"future, err := startCompletion(func(completion *C.mln_completion) int32 {{ return int32({call}) }}, {converter}); if err != nil {{ panic(bindingFailure{{err}}) }}"
    elif plan.function.return_type.canonical == "void":
        invocation = call
    elif plan.function.return_type.spelling != "mln_status":
        invocation = f"nativeResult := {call}"
    adoptions = []
    for index, ((p, value, local), conversion) in enumerate(zip(outputs, conversions)):
        if value.kind == "handle":
            adopted = f"adopted{index}"
            adoptions.append(f"{adopted} := {conversion}")
            converted = converted.replace(conversion, adopted)
    if adoptions:
        invocation += "; " + "; ".join(adoptions)
    if plan.registrations:
        owner = (
            f"adopted{next(i for i, (_, v, _) in enumerate(outputs) if v.kind == 'handle')}.bindingOwner"
            if adoptions
            else "receiver.bindingOwner"
            if receiver
            else "nil"
        )
        invocation += f"; arena.accept({owner})"
    if completing:
        invocation += "; accepted = true"
    if plan.consumes == "always":
        invocation = "transaction.commit(); " + invocation
    elif plan.consumes:
        invocation += "; transaction.commit()"
    identity = (
        "uint64(uintptr(unsafe.Pointer(receiver.native)))"
        if plan.scoped_receiver
        else "receiver.state.issued"
        if receiver
        else "0"
    )
    admission = f"admitted := bindingAdmission(C.binding_operation_{plan.name}, {identity}); defer admitted()"
    if receiver:
        guard = (
            "receiver == nil || receiver.scope == nil"
            if plan.scoped_receiver
            else "receiver == nil || receiver.bindingOwner == nil || receiver.state == nil"
        )
        admission = (
            f'if {guard} {{ panic(bindingFailure{{newBindingError(ErrInvalidState, "nil handle")}}) }}; '
            + admission
        )
    setup.insert(0, admission)
    if closing_decision:
        body = (
            admission
            + "; receiver.state.closeDecision(); runtime.KeepAlive(receiver); return struct{}{}"
        )
        return f"func (receiver *{values.owner(handle.native)}) Close() error {{ _, err := bindingCall(func() struct{{}} {{ {body} }}); return err }}\n"
    if plan.consumes:
        empty = (
            f"completedFuture({completion_type}{{}})"
            if completion_type
            else f"*new({result_type})"
        )
        invocation = f"if raw == 0 {{ return {empty} }}; " + invocation
    if plan.view:
        if len(outputs) != 1 or plan.completion:
            raise ModelError(
                [f"{plan.name}: borrowed view requires a single immediate result"]
            )
        value = outputs[0][1]
        values.views[value.native] = value
        view_type = public(value.native) + "View"
        signature.append(f"callback func({view_type}) error")
        method = "With" + method.removeprefix("Get")
        setup.append('if callback == nil { arena.fail("view callback is nil") }')
        if plan.view.owner.view_begin:
            setup.append(
                f"var token unsafe.Pointer; bindingCheck(func() int32 {{ return int32(C.{plan.view.owner.view_begin}(C.{handle.native}(raw), &token)) }}); defer C.{plan.view.owner.view_end}(token)"
            )
        setup.append("scope := bindingNewScope(); defer scope.alive.Store(false)")
        invocation += f"; if err := callback({view_type}{{value: {converted}, scope: scope}}); err != nil {{ panic(bindingFailure{{err}}) }}"
        converted, result_type, output_types = "struct{}{}", "struct{}", []
    body = f"{'; '.join(aliases)}; arena := &bindingArena{{}}; defer arena.close(); {'; '.join(setup)}; {invocation}; return {converted}"
    receiver_signature = (
        f"(receiver *{public(handle.native) + 'Scope' if plan.scoped_receiver else values.owner(handle.native)}) "
        if receiver
        else ""
    )
    if not output_types:
        return (
            products
            + f"func {receiver_signature}{method}({', '.join(signature)}) error {{ _,err := bindingCall(func() struct{{}} {{ {body} }}); return err }}\n"
        )
    return (
        products
        + f"func {receiver_signature}{method}({', '.join(signature)}) ({result_type},error) {{ return bindingCall(func() {result_type} {{ {body} }}) }}\n"
    )


def lower(bound):
    values = Values(bound)
    chunks, generated, errors = (
        [],
        [],
        {key: "\n".join(reasons) for key, reasons in bound.unsupported.items()},
    )
    for plan in bound.operations:
        try:
            chunks.append(operation(plan, values))
            generated.append(plan.name)
        except ModelError as error:
            errors[plan.name] = str(error)
    return values, chunks, generated, errors
