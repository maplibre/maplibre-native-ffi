"""Lower Rust operations over the completion and handle runtimes.

Every operation converts its parameters the same way. Its shape decides how
the call runs and what the method returns: an immediate call returns its value
or status, a drain or view wraps that call, and a completion returns a future.
"""

from __future__ import annotations

from dataclasses import dataclass, field, replace
from typing import assert_never

from ..model import Unsupported
from ..protocol import TEXTURE_READBACK_RESULT
from ..semantic import OperationPlan
from .rust import (
    SCALARS,
    ctype,
    doc,
    identifier,
    is_text,
    method_name,
    native_call,
    receiver_native,
)
from .rust_dynamic_values import decode, dynamic, encode

# Emitted bodies use these as crate paths, so no parameter can take them.
RESERVED_LOCALS = {"sys", "convert", "queue", "on_complete"}


def local_name(name: str) -> str:
    """A parameter's Rust name, which is its C name."""
    name = identifier(name)
    if name in RESERVED_LOCALS:
        raise Unsupported(f"parameter {name} shadows a name the emitted body uses")
    return name


def adopt_owned(owned, raw, value_types):
    public = value_types.handle_types.get(owned.handle.native)
    if not public:
        raise Unsupported(f"{owned.handle.native}: owner runtime unavailable")
    return public, f"{public}::adopt({raw})"


def result_rule(plan: OperationPlan, value_types) -> tuple[str, str]:
    """The completion's public result type and the converter that copies it."""
    result = plan.result
    if result is None:
        return "()", "completion::unit"
    if result.ownership != "borrowed":
        raise Unsupported("completion result requires resolved ownership transfer")
    array = result.kind == "array"
    value = result.element if array else replace(result, nullable=False)
    assert value is not None
    value_types.add(value, "out")
    public = value_types.public(value)
    raw = SCALARS.get(value.native, "sys::" + value.native)
    copied = decode(value_types, value, "value")
    rule = (
        ("optional_list" if result.nullable else "list")
        if array
        else ("optional" if result.nullable else "value")
    )
    public = f"Vec<{public}>" if array else public
    public = f"Option<{public}>" if result.nullable else public
    if copied == "unsafe { from_native(value) }?":
        return public, f"completion::{rule}::<{raw}, _>"
    copy = {
        "value": "copy_value",
        "optional": "optional_value",
        "list": "copy_slice",
        "optional_list": "optional_slice",
    }[rule]
    return (
        public,
        f"|result| {{ let value = completion::{copy}::<{raw}>(result)?; Ok({_map(rule, copied)}) }}",
    )


def _map(rule: str, copied: str) -> str:
    each = f"|value| -> Result<_> {{ Ok({copied}) }}"
    return {
        "value": copied,
        "optional": f"value.map({each}).transpose()?",
        "list": f"value.into_iter().map({each}).collect::<Result<Vec<_>>>()?",
        "optional_list": f"value.map(|value| value.into_iter().map({each}).collect::<Result<Vec<_>>>()).transpose()?",
    }[rule]


@dataclass
class Body:
    """A method under construction: the locals that its body binds, and what
    the parameter conversions add to its signature, arguments, and lines."""

    names: dict[str, str]
    call: str
    value: str
    future: str
    # The receiver's local and how the native call's closure binds it.
    target: str | None
    binding: str
    args: list[str]
    signature: list[str]
    prelude: list[str] = field(default_factory=list)
    setup: list[str] = field(default_factory=list)
    outputs: list[tuple[str, str]] = field(default_factory=list)
    uses_call: bool = False


def operation(
    plan: OperationPlan, value_types, *, discard=False, queued=False
) -> tuple[str, str]:
    """An operation's owner module and method. With `discard`, the method
    passes no completion. With `queued`, it is a method of the owner's queued
    view, whose queue holds the completion's handler and the registrations'
    handlers."""
    # A queued call takes the registrations of its queue's kind.
    value_types.queue_kind = "K" if queued else ""
    try:
        return _operation(plan, value_types, discard=discard, queued=queued)
    finally:
        value_types.queue_kind = ""


def _operation(plan: OperationPlan, value_types, *, discard, queued) -> tuple[str, str]:
    function = plan.function
    if plan.consumes == "always" and function.return_type.kind != "void":
        raise Unsupported(
            "status-returning unconditional consumption requires a commit-on-error adapter"
        )
    if plan.scoped_receiver:
        scoped = next(
            p.value.element for p in plan.inputs if p.name == plan.scoped_receiver
        )
        value_types.add(scoped)
        return "global", ""
    receiver = receiver_native(plan)
    if receiver is not None and receiver not in value_types.owners:
        raise Unsupported(f"receiver {receiver} needs a generated runtime adapter")
    if plan.factory and plan.factory not in value_types.owners:
        raise Unsupported(f"factory {plan.factory} needs a generated owner")
    # A factory is an associated function of the handle that it creates.
    module = (
        value_types.owners[receiver]
        if receiver
        else value_types.owners[plan.factory]
        if plan.factory
        else "global"
    )
    body = begin(plan, receiver)
    convert(plan, value_types, body, receiver)
    method = plan.member
    if discard and plan.completion and plan.completion.optional:
        # The release passes the completion, and the dispose, which keeps the
        # C name, passes none.
        method = function.name.removeprefix(f"{receiver}_")
    if method in {"native", "submit_command", "submit_query", "drop", "adopt"} or (
        method == "close" and not plan.consumes
    ):
        raise Unsupported("method name is reserved by the handle runtime")
    method = method_name(plan) if method == plan.member else identifier(method)
    closure, native = native_closure(plan, body)
    if queued and plan.completion and not discard:
        public, ending = queued_completion(plan, value_types, body, closure, native)
        return module, method_code(
            plan, value_types, body, method, public, ending, queued=True
        )
    match plan.shape:
        case "discard" | "release" | "answer" | "thread_start" | "owned" | "call":
            public, lines, returned = immediate(
                plan, value_types, body, closure, native
            )
            ending = [*lines, f"Ok({returned})"]
        case "drain":
            public, ending = drained(plan, value_types, body, closure, native)
        case "view":
            # A view's getter is a method of the view guard, which `lower`
            # declares around it.
            module = "view"
            public, ending = viewed(plan, value_types, body, closure, native)
        case "command":
            public, ending = pending(
                body,
                *(
                    discarded(body, closure, native, queued=queued)
                    if discard
                    else commanded(plan, body, closure, native)
                ),
            )
        case "attach" | "query" | "run" | "release_async":
            public, ending = pending(
                body,
                *(
                    discarded(body, closure, native)
                    if discard
                    else completed(plan, value_types, body, closure, native)
                ),
            )
        case _:
            assert_never(plan.shape)
    return module, method_code(
        plan, value_types, body, method, public, ending, discard=discard, queued=queued
    )


# Results that a queued handler reads from its batch rather than copying, by
# the queued value that lends them.
LENT_RESULTS = {TEXTURE_READBACK_RESULT: "delivery::Readback"}


def queued_value(plan: OperationPlan, value_types) -> str:
    """The `QueuedValue` that a queued completion lends its handler."""
    if plan.shape == "command":
        if plan.result is not None:
            raise Unsupported("command receipt cannot discard a value payload")
        return "delivery::Command"
    result = plan.result
    if result is None:
        return "delivery::Unit"
    if lent := LENT_RESULTS.get(result.native):
        return lent
    _, converter = result_rule(plan, value_types)
    for rule, value in (
        ("value", "Copied"),
        ("optional", "CopiedOptional"),
        ("list", "CopiedList"),
    ):
        prefix = f"completion::{rule}::<"
        if converter.startswith(prefix) and converter.endswith(", _>"):
            raw = converter.removeprefix(prefix).removesuffix(", _>")
            array = result.kind == "array"
            element = result.element if array else replace(result, nullable=False)
            public = value_types.public(element)
            return f"delivery::{value}<{raw}, {public}>"
    raise Unsupported("queued result requires a native copy rule")


def queued_completion(
    plan: OperationPlan, value_types, body: Body, closure: str, native: str
) -> tuple[str, list[str]]:
    """A completing call whose completion the view's queue delivers to a
    handler, after the outputs it returns at once."""
    value = queued_value(plan, value_types)
    body.signature.append(f"on_complete: impl CompletionHandler<K, {value}>")
    queue = "queue" if plan.consumes or not body.target else "self.1"
    enqueue = f"{queue}.enqueue({body.call}, on_complete, {closure} {native})"
    if not body.outputs:
        return "()", [enqueue]
    public, output = joined(body)
    return public, [f"{enqueue}?;", f"Ok({output})"]


def begin(plan: OperationPlan, receiver: str | None) -> Body:
    """The method's locals, with the receiver as its first argument."""
    function = plan.function
    parameters = function.parameters[1:] if receiver else function.parameters
    names = {parameter.name: local_name(parameter.name) for parameter in parameters}
    # Parameters keep their C names, so a local the body binds steps aside.
    taken = {*names.values(), *(local_name(p.name) for p in function.parameters[:1])}

    def body_local(name: str) -> str:
        while name in taken:
            name += "_"
        return name

    if receiver:
        target = local_name(function.parameters[0].name)
        pointer_receiver = bool(function.parameters[0].type.pointee)
        args = [f"&mut {target}" if pointer_receiver else target]
        binding = f"mut {target}" if pointer_receiver else target
    else:
        target, args, binding = None, [], "_"
    body = Body(
        names,
        body_local("call"),
        body_local("value"),
        body_local("future"),
        target,
        binding,
        args,
        ["&self"] if receiver else [],
    )
    return body


def convert(plan: OperationPlan, value_types, body: Body, receiver: str | None):
    """Converts each parameter after the receiver into the native call's
    argument, and each output into the method's result."""
    function = plan.function
    parameters = function.parameters[1:] if receiver else function.parameters
    input_plans = {parameter.name: parameter for parameter in plan.inputs}
    output_plans = {parameter.name: parameter for parameter in plan.outputs}
    lengths = {}
    for parameter in plan.inputs:
        value = parameter.value
        if (
            value.kind in {"array", "buffer"}
            and value.length
            and value.length != "nul"
            and not value.length.isdigit()
        ):
            source = body.names[parameter.name]
            lengths[value.length] = (
                f"{source}.map_or(0, |value| value.len())"
                if value.nullable
                else f"{source}.len()"
            )

    # Caller-sized text passes its storage, capacity, and length to a closure
    # that the runtime calls again when the text did not fit.
    text = plan.caller_sized_text
    text_parameters = (
        {text.name, text.value.length, text.result_length} if text else set()
    )

    for parameter in parameters:
        native, local = ctype(parameter.type), body.names[parameter.name]
        pointee = parameter.type.pointee
        parameter_plan = input_plans.get(parameter.name)
        value_plan = parameter_plan.value if parameter_plan else None
        if parameter.name in text_parameters:
            assert text is not None
            body.args.append(local)
            if parameter.name == text.name:
                body.prelude.append(f"let mut {local} = Vec::new();")
                body.outputs.append(("String", f"convert::utf8({local})?"))
        elif parameter.name in lengths:
            # Counts precede the conversions, which shadow their slices.
            body.prelude.append(
                f"let {local} = convert::count({lengths[parameter.name]})?;"
            )
            body.args.append(local)
        elif plan.completion and parameter.name == plan.completion.parameter:
            body.args.append(local)
        elif value_plan and value_plan.kind == "enum":
            value_types.add(value_plan)
            body.signature.append(f"{local}: {value_types.public(value_plan)}")
            body.args.append(
                f"{local}.bits()"
                if value_plan.enum_kind == "bitmask"
                else f"{local}.as_raw()"
            )
        elif value_plan and value_plan.kind == "array" and value_plan.element:
            element = value_plan.element
            value_types.add(element, "in")
            public = value_types.public(element)
            body.signature.append(
                f"{local}: &[&str]"
                if element.kind == "buffer" and is_text(element)
                else f"{local}: &[{public}]"
            )
            if value_plan.length and value_plan.length.isdigit():
                body.prelude.append(
                    f'if {local}.len() != {value_plan.length} {{ return Err(Error::invalid_argument("{parameter.name} requires {value_plan.length} elements")); }}'
                )
            body.setup.append(f"let {local} = {body.call}.array({local})?;")
            body.uses_call = True
            body.args.append(local)
        elif value_plan and value_plan.kind == "handle":
            public = value_types.handle_types.get(value_plan.native)
            if not public:
                raise Unsupported(f"{value_plan.native}: owner runtime unavailable")
            body.signature.append(f"{local}: &{public}")
            body.prelude.append(f"let {local} = {local}.inner.native();")
            body.args.append(local)
        elif value_plan and value_plan.kind in {"scalar", "native_pointer"}:
            body.signature.append(f"{local}: {value_types.public(value_plan)}")
            body.args.append(local)
        elif native in SCALARS:
            body.signature.append(f"{local}: {SCALARS[native]}")
            body.args.append(local)
        elif (
            value_plan
            and value_plan.kind == "buffer"
            and value_plan.length != "nul"
            and value_plan.buffer_form != "view"
        ):
            public = "&str" if is_text(value_plan) else "&[u8]"
            optional = value_plan.nullable
            body.signature.append(
                f"{local}: {'Option<' + public + '>' if optional else public}"
            )
            body.setup.append(
                f"let {local} = {encode(value_types, value_plan, local)};"
            )
            body.args.append(local)
        elif value_plan and value_plan.buffer_form == "view":
            if value_plan.encoding not in {"utf8", "json", "bytes"}:
                raise Unsupported(f"parameter {parameter.name} needs buffer encoding")
            optional = value_plan.nullable
            public = "&str" if is_text(value_plan) else "&[u8]"
            body.signature.append(
                f"{local}: {'Option<' + public + '>' if optional else public}"
            )
            body.setup.append(f"let {local} = {body.call}.input(&{local})?;")
            body.uses_call = True
            body.args.append(local)
        elif (
            pointee
            and ctype(pointee) == "char"
            and pointee.const
            and is_text(value_plan)
        ):
            body.signature.append(f"{local}: &str")
            body.setup.append(f"let {local} = {body.call}.input({local})?;")
            body.uses_call = True
            body.args.append(local)
        elif (
            value_plan
            and value_plan.kind == "reference"
            and value_plan.unwrapped.kind == "buffer"
        ):
            element = value_plan.unwrapped
            public = "&str" if is_text(element) else "&[u8]"
            body.signature.append(
                f"{local}: {'Option<' + public + '>' if value_plan.nullable else public}"
            )
            body.setup.append(
                f"let {local} = {body.call}.optional_reference({local})?;"
                if value_plan.nullable
                else f"let {local} = {body.call}.reference({local})?;"
            )
            body.uses_call = True
            body.args.append(local)
        elif (
            native in value_types.bound.values
            and value_types.bound.values[native].kind == "record"
        ):
            value = value_types.bound.values[native]
            value_types.add(value, "in")
            public = value_types.public(value)
            body.signature.append(
                f"{local}: &{public}" if dynamic(value) else f"{local}: {public}"
            )
            body.setup.append(f"let {local} = {body.call}.input(&{local})?;")
            body.uses_call = True
            body.args.append(local)
        elif (
            pointee
            and ctype(pointee) in value_types.bound.values
            and value_types.bound.values[ctype(pointee)].kind == "record"
            and value_plan is not None
            and parameter_plan is not None
            and parameter_plan.direction == "in"
            and value_plan.kind == "reference"
        ):
            value = value_types.bound.values[ctype(pointee)]
            value_types.add(value, "in")
            public = f"{'' if value.registration else '&'}{value_types.public(value)}"
            if value_plan.nullable:
                body.signature.append(f"{local}: Option<{public}>")
                body.setup.append(
                    f"let {local} = {body.call}.optional_reference({local}.as_ref())?;"
                )
            else:
                body.signature.append(f"{local}: {public}")
                body.setup.append(f"let {local} = {body.call}.reference(&{local})?;")
            body.uses_call = True
            body.args.append(local)
        elif plan.drain and parameter.name == plan.drain.batch.name:
            # The drain copies the batch's view and releases the batch, so no
            # owner holds it.
            batch = plan.drain.batch.value.unwrapped
            body.prelude.append(f"let mut {local} = sys::{batch.native}(0);")
            body.args.append(f"&mut {local}")
        elif plan.drain and parameter.name == plan.drain.view.name:
            value = plan.drain.view.value.unwrapped
            public = value_types.add(value, "out")
            assert pointee is not None
            raw_type = ctype(pointee)
            body.prelude += [
                f"let mut {local}: sys::{raw_type} = unsafe {{ std::mem::zeroed() }};",
                f"{local}.size = std::mem::size_of::<sys::{raw_type}>() as _;",
            ]
            body.args.append(f"&mut {local}")
            body.outputs.append((public, decode(value_types, value, local)))
        elif parameter.name in output_plans:
            output = output_plans[parameter.name]
            value = output.value
            assert pointee is not None
            if output.sized_by:
                element = value.element
                assert element is not None and value.length is not None
                value_types.add(element, "out")
                raw_type = SCALARS.get(ctype(pointee), "sys::" + ctype(pointee))
                body.prelude.append(
                    f"let mut {local}: Vec<{raw_type}> = (0..{lengths[value.length]}).map(|_| unsafe {{ std::mem::zeroed() }}).collect();"
                )
                if element.kind == "record" and any(
                    field.role == "size" for field in element.fields
                ):
                    body.prelude.append(
                        f"for value in &mut {local} {{ value.size = std::mem::size_of::<{raw_type}>() as _; }}"
                    )
                body.args.append(f"{local}.as_mut_ptr()")
                copied = decode(value_types, element, "item")
                body.outputs.append(
                    (
                        f"Vec<{value_types.public(element)}>",
                        f"{local}.into_iter().map(|item| Ok({copied})).collect::<Result<Vec<_>>>()?",
                    )
                )
                continue
            value = value.unwrapped
            if value.kind == "handle":
                owned = next(
                    (
                        owner
                        for owner in plan.owned_outputs
                        if owner.parameter == parameter.name
                    ),
                    None,
                )
                if not owned:
                    raise Unsupported("handle output lacks ownership transfer")
                public, converted = adopt_owned(owned, local, value_types)
                body.prelude.append(f"let mut {local} = sys::{value.native}(0);")
                body.args.append(f"&mut {local}")
                body.outputs.append((public, converted + "?"))
                continue
            value_types.add(value, "out")
            public = value_types.public(value)
            if dynamic(value):
                raise Unsupported("borrowed output record needs its native owner scope")
            raw_type = ctype(pointee)
            initial = (
                "Default::default()"
                if value.kind in {"scalar", "enum"}
                else "unsafe { std::mem::zeroed() }"
            )
            body.prelude.append(
                f"let mut {local}: {SCALARS.get(raw_type, 'sys::' + raw_type)} = {initial};"
            )
            if value.kind == "record" and any(
                field.role == "size" for field in value.fields
            ):
                body.prelude.append(
                    f"{local}.size = std::mem::size_of::<sys::{raw_type}>() as _;"
                )
            body.args.append(f"&mut {local}")
            body.outputs.append((public, decode(value_types, value, local)))
        else:
            raise Unsupported(
                f"parameter {parameter.name}: {parameter.type.spelling} needs ownership/shape lowering"
            )


def native_closure(plan: OperationPlan, body: Body) -> tuple[str, str]:
    """The closure head that the runtime calls, and the native call it makes."""
    function = plan.function
    if plan.completion:
        closure = (
            f"|{body.binding}, {local_name(plan.completion.parameter)}, out_diagnostic|"
        )
    elif function.return_type.kind != "void" and not plan.result:
        closure = f"|{body.binding}, out_diagnostic|"
    else:
        closure = f"|{body.binding}|"
    native = native_call(function, body.args, diagnostic="out_diagnostic")
    if text := plan.caller_sized_text:
        assert text.value.length is not None and text.result_length is not None
        storage, capacity, length = (
            body.names[text.name],
            body.names[text.value.length],
            body.names[text.result_length],
        )
        native = f"convert::caller_sized_text(&mut {storage}, |{storage}, {capacity}, {length}| {native})"
    return closure, f"unsafe {{ {native} }}"


def joined(body: Body) -> tuple[str, str]:
    """The outputs' public type and copy, as a tuple when there are several."""
    outputs = body.outputs
    if len(outputs) == 1:
        return outputs[0]
    return (
        "(" + ", ".join(item[0] for item in outputs) + ")",
        "(" + ", ".join(item[1] for item in outputs) + ")",
    )


def immediate(
    plan: OperationPlan, value_types, body: Body, closure: str, native: str
) -> tuple[str, list[str], str]:
    """An immediate call's public result, the lines that run it, and the
    value that the method returns."""
    function = plan.function
    public, output = joined(body)
    if function.return_type.kind == "void":
        return public, [f"{body.call}.run({closure} {native});"], output
    if plan.result:
        value_types.add(plan.result, "out")
        return (
            value_types.public(plan.result),
            [f"let {body.value} = {body.call}.run({closure} {native});"],
            decode(value_types, plan.result, body.value),
        )
    if not function.diagnostic:
        raise Unsupported(f"{function.name}: status return has no diagnostic")
    if plan.absence:
        return (
            f"Option<{public}>",
            [
                f"if !{body.call}.status_unless(sys::{plan.absence.status}, {closure} {native})? {{",
                "    return Ok(None);",
                "}",
            ],
            f"Some({output})",
        )
    declined = next(
        (r.accepted_unless for r in plan.registrations if r.accepted_unless),
        None,
    )
    return (
        public,
        [
            f"{body.call}.status_registered({closure} {{ let status = {native}; (status, !{body.names[declined]}) }})?;"
            if declined
            else f"{body.call}.status({closure} {native})?;"
        ],
        output,
    )


def drained(
    plan: OperationPlan, value_types, body: Body, closure: str, native: str
) -> tuple[str, list[str]]:
    """A drain, which returns no elements when nothing is queued and releases
    the batch that it copies the slice from."""
    function = plan.function
    assert plan.drain is not None and plan.absence is not None
    if not function.diagnostic:
        raise Unsupported(f"{function.name}: status return has no diagnostic")
    batch = plan.drain.batch.value.unwrapped
    release = value_types.bound.handles[batch.native].release
    if value_types.bound.source.functions_by_name[release].return_type.kind != "void":
        raise Unsupported(f"{release}: a drained batch's release must not fail")
    public, output = joined(body)
    return public, [
        f"if !{body.call}.status_unless(sys::{plan.absence.status}, {closure} {native})? {{",
        "    return Ok(Vec::new());",
        "}",
        "// SAFETY: native published the batch to this call alone.",
        f"let _batch = unsafe {{ crate::handle::Drained::new({body.names[plan.drain.batch.name]}, sys::{release}) }};",
        f"Ok({output})",
    ]


def viewed(
    plan: OperationPlan, value_types, body: Body, closure: str, native: str
) -> tuple[str, list[str]]:
    """A getter of its owner's view guard, whose output borrows the guard,
    since native keeps the objects it names valid only until the scope ends."""
    assert plan.view is not None
    public, lines, returned = immediate(plan, value_types, body, closure, native)
    return f"Lent<'_, {public}>", [*lines, f"Ok(Lent::new({returned}))"]


def discarded(
    body: Body, closure: str, native: str, *, queued: bool = False
) -> tuple[str, str]:
    """A completing call that passes no completion. A queued one goes through
    its view's queue, which refuses it once closed."""
    if queued:
        return "()", f"self.1.discard({body.call}, {closure} {native})"
    return "()", f"{body.call}.discard({closure} {native})"


def commanded(
    plan: OperationPlan, body: Body, closure: str, native: str
) -> tuple[str, str]:
    """A command, whose completion reports its disposition."""
    if plan.result is not None:
        raise Unsupported("command receipt cannot discard a value payload")
    return (
        "NativeFuture<CommandCompletion>",
        f"{body.call}.command({closure} {native})",
    )


def completed(
    plan: OperationPlan, value_types, body: Body, closure: str, native: str
) -> tuple[str, str]:
    """A completion that delivers its result, or only success."""
    result, converter = result_rule(plan, value_types)
    return (
        f"NativeFuture<{result}>",
        f"{body.call}.complete({closure} {native}, {converter})",
    )


def pending(body: Body, completed: str, expression: str) -> tuple[str, list[str]]:
    """A completing call's future, after the outputs it returns at once."""
    if not body.outputs:
        return completed, [expression]
    public = "(" + ", ".join([*(item[0] for item in body.outputs), completed]) + ")"
    converted = ", ".join([*(item[1] for item in body.outputs), body.future])
    return public, [f"let {body.future} = {expression}?;", f"Ok(({converted}))"]


def method_code(
    plan: OperationPlan,
    value_types,
    body: Body,
    method: str,
    public: str,
    ending: list[str],
    *,
    discard: bool = False,
    queued: bool = False,
) -> str:
    """The method around a call's ending: the receiver it takes or consumes,
    its return type, and its documentation."""
    function = plan.function
    consuming = bool(plan.consumes)
    mutable = "mut " if body.uses_call or not plan.completion else ""
    prelude, setup, call = body.prelude, body.setup, body.call
    # A queued call puts its registrations on the view's queue.
    on_queue = (".queue(queue)" if consuming else ".queue(self.1)") if queued else ""
    owned_queue = queued and (consuming or body.target is None)
    if owned_queue:
        # A consuming call is the owner's own method, and a global call a
        # function, so each takes the queue.
        on_queue = ".queue(queue)"
        method = f"{method}_queued"
        body.signature.insert(1 if consuming else 0, "queue: &DeliveryQueue<K>")
    if consuming:
        if body.target is None:
            raise Unsupported("consumption requires an owned receiver")
        if body.outputs:
            raise Unsupported("consumption with outputs requires an owner transaction")
        if ending[0].startswith(f"{call}.status("):
            ending = [ending[0].removesuffix("?;")]
        # The call takes the owner, and a refusal hands it back with the
        # error, so no value outlives its handle.
        body.signature[0] = "self"
        lines = [
            *prelude,
            f"self.consume(|{body.target}| {{",
            f"    let {mutable}{call} = Call::new({body.target}){on_queue};",
            *("    " + line for line in [*setup, *ending]),
            "})",
        ]
    else:
        receiver_self = (
            "self.0" if discard or queued else "self.owner" if plan.view else "self"
        )
        lines = [
            f"let {mutable}{call} = {receiver_self}.inner.call(){on_queue};"
            if body.target is not None
            else f"let {mutable}{call} = Call::global(){on_queue};"
        ]
        lines += [*prelude, *setup, *ending]
    returns = (
        f" -> std::result::Result<{public}, (Self, Error)>"
        if consuming
        else f" -> Result<{public}>"
    )
    # A call with no status and an infallible conversion returns its value.
    if (
        not consuming
        and lines[-1].startswith("Ok(")
        and not any("?" in line for line in lines)
    ):
        lines[-1] = lines[-1].removeprefix("Ok(").removesuffix(")")
        returns = f" -> {public}"
        if public == "()":
            lines.pop()
            returns = ""
    # An entry point that takes no handle can be a program's first call, so it
    # checks the C ABI version before anything reaches C. The version itself
    # is what the check reads, so it never fails.
    if body.target is None and not consuming and not plan.unchecked:
        lines.insert(
            1,
            "crate::abi::validate_abi_version()?;"
            if returns.startswith(" -> Result")
            else "crate::abi::require_abi_version();",
        )

    def has_pointer(value):
        return (
            False
            if value.registration
            else value.kind == "native_pointer"
            or bool(value.element and has_pointer(value.element))
            or any(has_pointer(field.value) for field in value.fields if field.public)
        )

    unsafe_input = any(
        has_pointer(parameter.value)
        for parameter in plan.inputs
        if parameter.name != plan.receiver and not parameter.value.registration
    )
    safety_doc = (
        "    ///\n    /// # Safety\n    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.\n"
        if unsafe_input
        else ""
    )
    generic = "<K: QueueKind>" if owned_queue else ""
    return (
        doc(value_types.bound, function.name, "    ")
        + safety_doc
        + f"    pub {'unsafe ' if unsafe_input else ''}fn {method}{generic}({', '.join(body.signature)}){returns} {{\n"
        + "\n".join("        " + line for line in lines)
        + "\n    }\n"
    )
