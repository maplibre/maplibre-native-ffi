"""Lower Go operations from shared ownership and value plans."""

from dataclasses import replace

from ..model import ModelError
from ..semantic import output_member
from .go import GO_KEYWORDS, documented_operation, name, native_call
from .go_values import Values, absent, public


def element(value):
    return value.element if value.kind == "reference" else value


def method_name(plan, receiver, handle):
    method = name(plan.member)
    if handle and handle.handle and plan.name == handle.handle.release:
        return "Close"
    if receiver and method in {"Close", "IsClosed", "ID"}:
        raise ModelError([f"{plan.name}: {method} is reserved for owner state"])
    return method


def argument(values, value, expr):
    """A Go expression that converts expr to the native value of a parameter."""
    if value.kind in {"scalar", "enum"}:
        return f"{values.c_type(value)}({expr})"
    if value.kind == "native_pointer":
        return f"({values.c_type(value)})(C.binding_address(C.uintptr_t({expr})))"
    if value.kind == "record":
        return f"native{public(value.native)}({expr}, arena)"
    if value.kind == "handle":
        return f"C.{value.native}(arena.lease({expr}.owner()))"
    if value.kind == "reference":
        if value.nullable:
            return f"bindingStoreOptional({expr}, arena, {converter(values, value.element)})"
        return f"bindingStore({argument(values, value.element, expr)}, arena)"
    if value.kind == "buffer":
        if value.length == "nul":
            return (
                f"bindingOptionalCString({expr}, arena)"
                if absent(value)
                else f"arena.cstring({expr})"
            )
        if value.ctype.pointee:
            if value.nullable:
                return f"({values.c_type(value)})(bindingNullableBytes({expr}, arena))"
            if value.optional == "empty":
                return f"({values.c_type(value)})(bindingOptionalBytes({expr}, arena))"
            data = f"[]byte({expr})" if value.encoding == "utf8" else expr
            return f"({values.c_type(value)})(arena.bytes({data}))"
        if value.nullable:
            return f"bindingNullableView({expr}, arena)"
        if value.optional == "empty":
            return f"bindingOptionalView({expr}, arena)"
        return f"bindingView({expr}, arena)"
    if value.kind == "array":
        if value.ctype.kind == "array":
            values.fail(value, "fixed array parameters need a conversion")
        helper = "bindingNullableArray" if value.nullable else "bindingArray"
        return f"{helper}({expr}, arena, {converter(values, value.element)})"
    values.fail(value, "missing native input conversion")


def converter(values, value):
    """A function value that converts one binding value and the arena to native."""
    if value.kind == "record":
        return f"native{public(value.native)}"
    if (
        value.kind == "buffer"
        and not absent(value)
        and not value.ctype.pointee
        and value.length != "nul"
    ):
        return f"bindingView[{values.type(value)}]"
    native = (
        f"C.{value.native}"
        if value.kind in {"record", "handle", "union"}
        else values.c_type(value)
    )
    return f"func(item {values.type(value)}, arena *bindingArena) {native} {{ return {argument(values, value, 'item')} }}"


def copier(values, value):
    """A function value that copies one native value to its binding value."""
    if value.kind == "record":
        return f"copy{public(value.native)}"
    if value.kind == "buffer" and not value.ctype.pointee and value.length != "nul":
        text = "Text" if value.encoding == "utf8" else "Bytes"
        if value.optional == "empty" and not value.nullable:
            return f"copyOptionalView{text}"
        if not absent(value):
            return f"copyView{text}"
    return f"func(raw C.{value.native}) {values.type(value)} {{ return {values.copy(value, 'raw')} }}"


def operation(plan, values):
    if plan.role == "support":
        raise ModelError(
            [f"{plan.name}: support relationship is emitted with its owner or value"]
        )
    receiver_name = plan.receiver or plan.scoped_receiver
    receiver = next((p for p in plan.inputs if p.name == receiver_name), None)
    handle = element(receiver.value) if receiver else None
    decision = values.api.decisions.get(handle.native) if handle else None
    method = method_name(plan, receiver, handle)
    operation_id = f"C.binding_operation_{plan.name}"
    if decision and plan.name == decision.handle.release:
        return f"func (receiver *{values.owner(handle.native)}) Close() error {{ return bindingCloseDecision(receiver.owner(), {operation_id}) }}\n"
    if plan.view:
        method = "With" + name(plan.view.stem)
    internal = {
        "arena",
        "raw",
        "completion",
        "diagnostic",
        "receiver",
        "result",
        "future",
        "item",
        "adopted",
        "handle",
        "callback",
        "token",
        "scope",
    }
    labels = {}
    for p in plan.function.parameters:
        parts = p.name.split("_")
        label = parts[0] + "".join(name(part) for part in parts[1:])
        if p.name in {o.name for o in plan.outputs}:
            label = "out" + name(output_member(p.name))
        while label in GO_KEYWORDS or label in internal or label in labels.values():
            label += "_"
        labels[p.name] = label
    counts = {
        p.value.length: p
        for p in plan.inputs
        if p.value.kind in {"array", "buffer"}
        and p.value.length
        and p.value.length != "nul"
        and not p.value.length.isdigit()
    }
    signature, setup, arguments = [], [], {}
    for p in plan.inputs:
        if p.name == receiver_name:
            continue
        if p.name in counts:
            counted = counts[p.name]
            length = (
                f"bindingLen({labels[counted.name]})"
                if counted.value.kind == "buffer" and absent(counted.value)
                else f"len({labels[counted.name]})"
            )
            arguments[p.name] = f"bindingCount[C.{p.value.native}]({length})"
            continue
        value = p.value
        values.require(value, input=True)
        signature.append(f"{labels[p.name]} {values.type(value)}")
        if value.kind == "array" and value.length and value.length.isdigit():
            values.fail(value, "fixed-length array parameters need a check")
        arguments[p.name] = argument(values, value, labels[p.name])
    body_setup = []
    if plan.scoped_receiver:
        values.require(handle)
        arguments[receiver.name] = "receiver.native"
    elif receiver:
        arguments[receiver.name] = f"C.{handle.native}(raw)"
        if receiver.value.kind == "reference":
            body_setup.append(f"handle := C.{handle.native}(raw)")
            arguments[receiver.name] = "&handle"
    outputs = []
    for p in plan.outputs:
        value = element(p.value)
        values.require(value)
        local = labels[p.name]
        setup.append(
            f"{local} := C.{value.default}()"
            if value.default
            else f"var {local} {values.c_type(value)}"
        )
        for member in value.fields:
            if member.role == "size":
                setup.append(
                    f"{local}.{member.name} = {values.c_type(member.value)}(unsafe.Sizeof({local}))"
                )
        arguments[p.name] = "&" + local
        outputs.append((p, value, local))
    owned = {o.parameter: o for o in plan.owned_outputs}

    def parent_of(owner):
        if owner is None or owner.parent_parameter is None:
            return "nil"
        if owner.parent_parameter == receiver_name:
            return "receiver"
        return labels[owner.parent_parameter]

    output_types, conversions, adoptions = [], [], []
    for p, value, local in outputs:
        output_types.append(values.type(value))
        if value.kind == "handle":
            adopted = f"adopted{len(adoptions)}" if len(outputs) > 1 else "adopted"
            adoptions.append(
                f"{adopted} := adopt{values.owner(value.native)}(uint64({local}), {parent_of(owned.get(p.name))})"
            )
            conversions.append(adopted)
        else:
            conversions.append(values.copy(value, local, local))
    if adoptions and plan.registrations:
        first = conversions[
            next(i for i, (_, v, _) in enumerate(outputs) if v.kind == "handle")
        ]
        adoptions.append(f"arena.accept({first}.bindingOwner)")
    for registration in plan.registrations:
        if registration.accepted_unless:
            # Native kept nothing from a declined registration.
            declined = next(
                local
                for p, _, local in outputs
                if p.name == registration.accepted_unless
            )
            adoptions.append(f"if bool({declined}) {{ arena.decline() }}")
    if plan.consumes == "always":
        access = "Consuming"
    elif plan.consumes:
        access = "Closing"
    elif decision and plan.name == decision.complete:
        access = "Completing"
    elif plan.receiver_access == "issued":
        access = "Issued"
    elif plan.view or any(
        o.value.lifetime == "owner" or element(o.value).lifetime == "owner"
        for o in plan.outputs
    ):
        access = "Read"
    else:
        access = "Live"
    if plan.scoped_receiver:
        target = f"receiver.target({operation_id})"
    elif receiver:
        target = f"binding{access}(receiver.owner(), {operation_id})"
    else:
        target = f"bindingGlobal({operation_id})"
    result_type = None
    if plan.completion:
        arguments[plan.completion.parameter] = "completion"
        result = plan.result
        if plan.execution == "command":
            completion_type, convert = "CommandCompletion", "completionCommand"
        elif result is None:
            completion_type, convert = "struct{}", "completionUnit"
        else:
            values.require(result)
            completion_type = values.type(result)
            if result.kind == "handle":
                parent = parent_of(plan.completion.result_owner)
                convert = f"completionOf(func(raw C.{result.native}) {completion_type} {{ return adopt{values.owner(result.native)}(uint64(raw), {parent}) }})"
            elif result.kind == "array":
                if result.stride or result.item_buffer:
                    values.fail(result, "array result needs a copy adapter")
                helper = (
                    "completionNullableListOf"
                    if result.nullable
                    else "completionListOf"
                )
                convert = f"{helper}({copier(values, result.element)})"
            else:
                content = replace(result, nullable=False)
                convert = f"completionOf({copier(values, content)})"
                if result.nullable:
                    convert = f"completionNullable({convert})"
        output_types.append(f"*Future[{completion_type}]")
        conversions.append("future")
    elif (
        not outputs
        and plan.function.return_type.canonical != "void"
        and not plan.status
    ):
        values.require(plan.result)
        result_type = values.type(plan.result)
    call = native_call(
        plan.function, *(arguments[p.name] for p in plan.function.parameters)
    )
    products = ""
    if len(output_types) > 1:
        returned = name(plan.name.removeprefix("mln_")) + "Result"
        names = [name(output_member(p.name)) for p, _, _ in outputs] + (
            ["Completion"] if plan.completion else []
        )
        products = (
            f"type {returned} struct {{ "
            + "; ".join(f"{n} {t}" for n, t in zip(names, output_types, strict=True))
            + " }\n"
        )
        converted = (
            returned
            + "{"
            + ", ".join(f"{n}: {c}" for n, c in zip(names, conversions, strict=True))
            + "}"
        )
    elif output_types:
        returned, converted = output_types[0], conversions[0]
    else:
        returned = converted = None
    closure_setup = "".join(line + "\n" for line in body_setup)
    if plan.view:
        if plan.completion:
            raise ModelError(
                [f"{plan.name}: borrowed view requires an immediate result"]
            )
        value = outputs[0][1]
        values.views[value.native] = value
        view_type = public(value.native) + "View"
        signature.append(f"callback func({view_type}) error")
        opened = native_call(
            values.api.source.functions_by_name[plan.view.begin],
            f"C.{handle.native}(raw)",
            "token",
        )
        begin = f"func(raw uint64, token *unsafe.Pointer, diagnostic *C.mln_diagnostic) int32 {{\nreturn int32({opened})\n}}"
        end = f"func(token unsafe.Pointer) {{ C.{plan.view.end}(token) }}"
        get = f"func(raw uint64, diagnostic *C.mln_diagnostic) int32 {{\nreturn int32({call})\n}}"
        wrap = f"func(scope *bindingScope) {view_type} {{ return {view_type}{{value: {conversions[0]}, scope: scope}} }}"
        body = "".join(line + "\n" for line in setup)
        body += (
            f"return bindingWithView({target}, callback, {begin}, {end}, {get}, {wrap})"
        )
        return f"func (receiver *{values.owner(handle.native)}) {method}({', '.join(signature)}) error {{\n{body}\n}}\n"
    if plan.completion:
        start = f"func(arena *bindingArena, raw uint64, completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32 {{\n{closure_setup}return int32({call})\n}}"
        if outputs:
            result_closure = (
                f"func(arena *bindingArena, future *Future[{completion_type}]) {returned} {{\n"
                + "".join(line + "\n" for line in adoptions)
                + f"return {converted}\n}}"
            )
            invocation = f"return bindingStartWith({target}, {start}, {convert}, {result_closure})"
        else:
            invocation = f"return bindingStart({target}, {start}, {convert})"
    elif plan.function.return_type.canonical == "void" or result_type:
        if outputs:
            values.fail(
                plan.result or outputs[0][1], "direct calls cannot have outputs"
            )
        if result_type:
            returned = result_type
            converted = values.copy(plan.result, call)
            invocation = f"return bindingDirect({target}, func(arena *bindingArena, raw uint64) {returned} {{\n{closure_setup}return {converted}\n}})"
        else:
            invocation = f"_, err := bindingDirect({target}, func(arena *bindingArena, raw uint64) struct{{}} {{\n{closure_setup}{call}\nreturn struct{{}}{{}}\n}})\nreturn err"
    else:
        check = f"func(arena *bindingArena, raw uint64, diagnostic *C.mln_diagnostic) int32 {{\n{closure_setup}return int32({call})\n}}"
        if plan.absence:
            # A handle is absent as nil; a value becomes a pointer that is.
            if outputs[0][1].kind != "handle":
                returned = "*" + returned
                adoptions.append(f"value := {converted}")
                converted = "&value"
            result_closure = (
                f"func(arena *bindingArena) {returned} {{\n"
                + "".join(line + "\n" for line in adoptions)
                + f"return {converted}\n}}"
            )
            invocation = f"return bindingGetUnless({target}, int32(C.{plan.absence.status}), {check}, {result_closure})"
        elif outputs:
            result_closure = (
                f"func(arena *bindingArena) {returned} {{\n"
                + "".join(line + "\n" for line in adoptions)
                + f"return {converted}\n}}"
            )
            invocation = f"return bindingGet({target}, {check}, {result_closure})"
        else:
            invocation = f"return bindingDo({target}, {check})"
    receiver_type = (
        public(handle.native) + "Scope"
        if plan.scoped_receiver
        else values.owner(handle.native)
        if receiver
        else None
    )
    receiver_signature = f"(receiver *{receiver_type}) " if receiver else ""
    results = f"({returned}, error)" if returned else "error"
    body = "".join(line + "\n" for line in setup) + invocation
    return (
        products
        + f"func {receiver_signature}{method}({', '.join(signature)}) {results} {{\n{body}\n}}\n"
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
            chunks.append(
                documented_operation(bound, plan.name, operation(plan, values))
            )
            generated.append(plan.name)
        except ModelError as error:
            errors[plan.name] = str(error)
    return values, chunks, generated, errors
