"""Lower Go operations from shared ownership and value plans."""

from dataclasses import replace

from ..model import ModelError
from ..semantic import output_member
from .go import GO_KEYWORDS, documented_operation, name, native_call
from .go_values import Values, public

# Members every handle owner declares, which no operation may take.
OWNER_MEMBERS = {"Close", "Dispose", "Id", "IsClosed", "String"}


def element(value):
    return value.unwrapped


def camel(value):
    first, *rest = value.split("_")
    return first + "".join(name(part) for part in rest)


def method_name(plan, receiver, handle):
    method = name(plan.member)
    if handle and handle.handle and plan.name == handle.handle.release:
        return "Close"
    if receiver and method in OWNER_MEMBERS:
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
                f"func() *C.char {{ if {expr} == nil {{ return nil }}; return arena.cstring(*{expr}) }}()"
                if value.nullable
                else f"arena.cstring({expr})"
            )
        if value.ctype.pointee:
            if value.nullable:
                return f"({values.c_type(value)})(func() unsafe.Pointer {{ if {expr} == nil {{ return nil }}; p := arena.bytes([]byte(*{expr})); if p == nil {{ p = arena.allocate(1) }}; return p }}())"
            data = f"[]byte({expr})" if value.encoding == "utf8" else expr
            return f"({values.c_type(value)})(arena.bytes({data}))"
        if value.nullable:
            return f"func() C.mln_buffer_view {{ if {expr} == nil {{ return C.mln_buffer_view{{}} }}; v := bindingView(*{expr}, arena); if v.data == nil {{ v.data = arena.allocate(1) }}; return v }}()"
        return f"bindingView({expr}, arena)"
    if value.kind == "array":
        if value.ctype.kind == "array":
            values.fail(value, "fixed array parameters need a conversion")
        converted = f"bindingArray({expr}, arena, {converter(values, value.element)})"
        if value.nullable:
            native = values.c_type(value.element)
            return f"func() *{native} {{ p := {converted}; if {expr} != nil && p == nil {{ p = (*{native})(arena.allocate(1)) }}; return p }}()"
        return converted
    values.fail(value, "missing native input conversion")


def converter(values, value):
    """A function value that converts one binding value and the arena to native."""
    if value.kind == "record":
        return f"native{public(value.native)}"
    if (
        value.kind == "buffer"
        and not value.nullable
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
        if not value.nullable:
            return f"copyView{text}"
    return f"func(raw C.{value.native}) {values.type(value)} {{ return {values.copy(value, 'raw')} }}"


def copies_borrowed(value):
    if value.kind in {"array", "buffer", "reference"}:
        return True
    return any(copies_borrowed(field.value) for field in value.fields)


def needs_read(plan):
    return bool(
        plan.receiver
        and not plan.completion
        and not plan.consumes
        and not plan.owned_outputs
        and not plan.view
        and any(
            copies_borrowed(element(p.value)) and element(p.value).kind != "handle"
            for p in plan.outputs
            if not p.sized_by and not p.result_length
        )
    )


def operation(plan, values, *, discard=False, dispose=False):
    """Lower one operation. `dispose` lowers a handle's C dispose as its
    synchronous `Dispose`, which passes a null completion to a dispose that is
    also the release."""
    dropped_completion = None
    if dispose and plan.completion:
        dropped_completion = plan.completion.parameter
        plan = replace(plan, completion=None)
    if plan.role == "support" and not dispose:
        raise ModelError(
            [f"{plan.name}: support relationship is emitted with its owner or value"]
        )
    receiver_name = plan.receiver or plan.scoped_receiver
    receiver = next((p for p in plan.inputs if p.name == receiver_name), None)
    handle = element(receiver.value) if receiver else None
    method = "Dispose" if dispose else method_name(plan, receiver, handle)
    if plan.view:
        method = "With" + name(plan.view.stem)
    elif (
        receiver
        and not plan.completion
        and len(method) > 3
        and method.startswith("Get")
        and all(p.name == receiver_name for p in plan.inputs)
    ):
        # Go getters drop Get; queries that return a Future keep it.
        method = method.removeprefix("Get")
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
        label = camel(p.name)
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
    # Caller-sized text passes its storage, capacity, and length to a closure
    # that the runtime calls again when the text did not fit.
    text = plan.caller_sized_text
    signature, setup, body_setup, arguments = [], [], [], {}
    if dropped_completion:
        arguments[dropped_completion] = "nil"
    for p in plan.inputs:
        if p.name == receiver_name:
            continue
        if text and p.name == text.value.length:
            arguments[p.name] = labels[p.name]
            continue
        if p.name in counts:
            counted = counts[p.name]
            length = (
                f"func() int {{ if {labels[counted.name]} == nil {{ return 0 }}; return len(*{labels[counted.name]}) }}()"
                if counted.value.kind == "buffer" and counted.value.nullable
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
        if text:
            # Converted once, before the calls that fit the text.
            local = "native" + name(p.name)
            body_setup.append(f"{local} := {arguments[p.name]}")
            arguments[p.name] = local
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
        local = labels[p.name]
        if text and p.name == text.result_length:
            arguments[p.name] = local
            continue
        if p is text:
            setup.append(f"var {local} []byte")
            arguments[p.name] = local
            outputs.append((p, p.value, local))
            continue
        value = element(p.value)
        values.require(value)
        if p.sized_by:
            # Native writes one element per input element into Go memory that
            # holds no Go pointers, which cgo lets a call borrow.
            count = f"len({labels[p.sized_by]})"
            raw_type = values.c_type(value.element)
            setup.append(f"{local} := make([]{raw_type}, {count})")
            for member in value.element.fields:
                if member.role == "size":
                    setup.append(
                        f"for index := range {local} {{ {local}[index].{member.name} = {values.c_type(member.value)}(unsafe.Sizeof({local}[index])) }}"
                    )
            arguments[p.name] = f"unsafe.SliceData({local})"
            outputs.append((p, value, local))
            continue
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
        return "receiver"

    output_types, conversions, adoptions = [], [], []
    for p, value, local in outputs:
        if p is text:
            output_types.append("[]byte")
            conversions.append(local)
            continue
        output_types.append(values.type(value))
        if p.sized_by:
            copied = values.copy(value.element, "item")
            conversions.append(
                f"func() {values.type(value)} {{ result := make({values.type(value)}, len({local})); for index, item := range {local} {{ result[index] = {copied} }}; return result }}()"
            )
        elif value.kind == "handle":
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
    elif needs_read(plan):
        access = "Read"
    else:
        access = "Owned"
    if plan.scoped_receiver:
        target = "receiver.target()"
    elif receiver:
        target = f"binding{access}(receiver.owner())"
    else:
        target = "bindingGlobal()"
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
                helper = "completionListOf"
                convert = f"{helper}({copier(values, result.element)})"
                if result.nullable:
                    convert = f"func(result *C.mln_completion_result) ({completion_type}, error) {{ if result.value == nil {{ return nil, nil }}; return {convert}(result) }}"
            else:
                content = replace(result, nullable=False)
                convert = f"completionOf({copier(values, content)})"
                if result.nullable:
                    convert = f"completionNullable({convert})"
        if not discard:
            output_types.append(f"*Future[{completion_type}]")
            conversions.append("future")
    elif (
        not outputs
        and plan.function.return_type.canonical != "void"
        and not plan.status
    ):
        values.require(plan.result)
        result_type = values.type(plan.result)
    # A global call with no inputs, no status, and a plain result can't fail.
    infallible = bool(
        result_type
        and not receiver
        and not signature
        and plan.result.kind in {"scalar", "enum", "native_pointer"}
    )
    call = native_call(
        plan.function, *(arguments[p.name] for p in plan.function.parameters)
    )
    if text:
        storage, capacity, length = (
            labels[text.name],
            labels[text.value.length],
            labels[text.result_length],
        )
        call = f"bindingCallerSizedText(&{storage}, func({storage} *C.char, {capacity} C.size_t, {length} *C.size_t) int32 {{ return int32({call}) }})"
    # Several outputs return as Go results, named after the C outputs and,
    # for a completion, ready. The runtime carries them in an unnamed struct.
    results = None
    if len(output_types) > 1:
        names = [camel(output_member(p.name)) for p, _, _ in outputs] + (
            ["ready"] if plan.completion else []
        )
        taken = set(names) & {labels[p.name] for p in plan.inputs}
        if taken:
            raise ModelError([f"{plan.name}: results {sorted(taken)} name parameters"])
        results = list(zip(names, output_types, strict=True))
        returned = "struct{ " + "; ".join(f"{n} {t}" for n, t in results) + " }"
        converted = returned + "{" + ", ".join(conversions) + "}"
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
        if discard:
            invocation = f"return callDiscard({target}, {start})"
        elif outputs:
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
        if result_type and infallible:
            returned = result_type
            invocation = f"return {values.copy(plan.result, call)}"
        elif result_type:
            returned = result_type
            converted = values.copy(plan.result, call)
            invocation = f"return bindingDirect({target}, func(arena *bindingArena, raw uint64) {returned} {{\n{closure_setup}return {converted}\n}})"
        else:
            invocation = f"_, err := bindingDirect({target}, func(arena *bindingArena, raw uint64) struct{{}} {{\n{closure_setup}{call}\nreturn struct{{}}{{}}\n}})\nreturn err"
    else:
        check = f"func(arena *bindingArena, raw uint64, diagnostic *C.mln_diagnostic) int32 {{\n{closure_setup}return int32({call})\n}}"
        if plan.absence:
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
    if discard:
        receiver_signature = f"(view {receiver_type}Discarding) "
        setup.insert(0, "receiver := view.owner")
    if results:
        invocation = (
            invocation.replace("return ", "result, err := ", 1)
            + "\nreturn "
            + ", ".join(f"result.{n}" for n, _ in results)
            + ", err"
        )
        returns = "(" + ", ".join(f"{n} {t}" for n, t in results) + ", err error)"
    elif infallible:
        returns = returned
    else:
        returns = f"({returned}, error)" if returned else "error"
    body = "".join(line + "\n" for line in setup) + invocation
    return f"func {receiver_signature}{method}({', '.join(signature)}) {returns} {{\n{body}\n}}\n"


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
