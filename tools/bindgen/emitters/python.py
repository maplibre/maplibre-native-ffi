"""Generate Python operations, values, stubs, and their PyO3 calls together."""

from __future__ import annotations

import ast
from collections import defaultdict
from dataclasses import replace
from keyword import iskeyword
from textwrap import dedent

from tools.bindgen import async_hosts, docs
from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters.rust import (
    RUST_KEYWORDS,
    counted_owner,
    native_call,
)
from tools.bindgen.model import Api, CType, Function, ModelError, Record
from tools.bindgen.semantic import (
    BoundApi,
    DefaultSupport,
    DisposeSupport,
    HandlePlan,
    OperationPlan,
    output_member,
)

from .python_values import Values, ok, public_name, rust_field, scalar_type

SCALARS = {
    "double": ("f64", "float"),
    "float": ("f32", "float"),
    "bool": ("bool", "bool"),
    "_Bool": ("bool", "bool"),
    "uint32_t": ("u32", "int"),
    "int32_t": ("i32", "int"),
    "uint64_t": ("u64", "int"),
    "int64_t": ("i64", "int"),
    "size_t": ("usize", "int"),
}
OWNERS: dict[str, str] = {}
RUNTIME_EXPORTS: set[str] = set()
# The platforms that runtime wheels serve. One API wheel serves them all, so it
# leaves out only what none of them can use.
TARGETS = frozenset({"android", "linux", "macos", "windows"})


def ctype(type_: CType) -> str:
    return type_.declaration or type_.spelling.removeprefix("const ")


def pascal(name: str) -> str:
    return "".join(part.capitalize() for part in name.split("_"))


def event_sources(api: Api) -> set[str]:
    """The handles that a runtime event can name as its source."""
    return {
        "mln_" + value.name.removeprefix("MLN_RUNTIME_EVENT_SOURCE_").lower()
        for enum in api.enums
        if enum.name == "mln_runtime_event_source_type"
        for value in enum.values
    }


def public_dispose(handle: HandlePlan) -> bool:
    """Whether a handle has a C dispose besides a void release: a separate
    dispose, or a release that disposes and also takes a completion."""
    return bool(
        handle.dispose
        and (handle.dispose != handle.release or handle.dispose_completion)
    )


def unsupported(function: Function | Record, reason: str) -> ModelError:
    return ModelError([f"{function.location}: Python: {function.name}: {reason}"])


# Members every handle owner defines. The release operation alone becomes
# `close`; any other operation with one of these names would shadow a member.
OWNER_MEMBERS = {"state", "close", "dispose", "id", "is_closed"}
LOCAL_NAMES = {
    "self",
    "py",
    "call",
    "read",
    "reservation",
    "state",
    "completion",
    "handle",
    "status",
    "map_future",
    "tuple",
    "map_error",
    "completion_value",
    "completion_slice",
    "copied_string_view",
    "submit_python_future",
    "submit_python_command_future",
}


def safe_identifier(name: str) -> bool:
    return (
        name.isascii()
        and name.isidentifier()
        and not name.startswith("_")
        and not iskeyword(name)
        and name not in RUST_KEYWORDS
    )


def result_converter(
    plan: OperationPlan, values: Values
) -> tuple[str, str, str, str | None]:
    result = plan.result
    if result is None:
        return "None", "None", "py_none", None
    if result.kind == "handle" and result.ownership == "owned" and result.handle:
        owner = OWNERS.get(result.native)
        if owner is None:
            raise unsupported(
                plan.function, "owned result needs a supported handle constructor"
            )
        # SAFETY: the completion transfers its one handle to the converter.
        converter = f"|py, result| unsafe {{ {owner}::adopt(py, completion_value::<sys::{result.native}>(result)?, Vec::new()) }}"
        return "Any", owner, converter, None
    values.supported(result)
    public = values.type(result)
    if result.kind == "array" and result.element:
        element = result.element
        copied = values.copy(element, "value")
        converter = f"""|py, result| {{
            {"if result.value.is_null() { return Ok(py.None()); }" if result.nullable else ""}
            let list = PyList::empty(py);
            for value in generated_completion_values::<sys::{element.native}>(result)? {{
                list.append({copied})?;
            }}
            Ok(list.into_any().unbind())
        }}"""
    else:
        native = SCALARS.get(result.native, ("sys::" + result.native, ""))[0]
        copied = values.copy(result, "value")
        converter = f"|py, result| {{ let value = completion_value::<{native}>(result)?; {ok(copied)} }}"
        if result.nullable:
            converter = f"|py, result| {{ if result.value.is_null() {{ return Ok(py.None()); }} let value = completion_value::<{native}>(result)?; {ok(copied)} }}"
    return "Any", public, converter, None


def borrows_memory(value):
    if value.kind in {"buffer", "array"}:
        return True
    if value.kind == "reference" and value.element:
        return borrows_memory(value.element)
    return any(borrows_memory(field.value) for field in value.fields)


def owned_native(value, expression, callbacks=False):
    """Adopts an output handle that an accepted call transferred."""
    roots = "callback_roots.clone()" if callbacks else "Vec::new()"
    return f"unsafe {{ {OWNERS[value.native]}::adopt(py, {expression}, {roots}) }}"


def operation(
    plan: OperationPlan, values: Values, *, discard=False, exit=False
) -> tuple[str, str, str, str, str | None]:
    """Lower one operation. `exit` lowers a handle's scoped release as the
    private `_exit` that `__exit__` calls, which skips a released handle and
    passes a null completion to a release that takes an optional one."""
    function = plan.function
    dropped_completion = None
    if exit and plan.completion and plan.completion.optional:
        dropped_completion = plan.completion.parameter
        plan = replace(plan, completion=None)
    if plan.role == "support" and not exit:
        raise unsupported(
            function, "support operation is consumed by the value/runtime compiler"
        )
    receiver_name = plan.receiver or plan.scoped_receiver
    receiver = next((p for p in plan.inputs if p.name == receiver_name), None)
    scoped = plan.scoped_receiver is not None
    receiver_pointer = receiver is not None and receiver.value.kind == "reference"
    if receiver_pointer and receiver.value.element:
        receiver = replace(receiver, value=receiver.value.element)
    if receiver is not None and receiver.value.native not in OWNERS and not scoped:
        raise unsupported(function, "receiver needs a supported handle owner")
    native_owner = receiver.value.native if receiver else "mln"
    owner = (
        public_name(native_owner) + "Scope"
        if scoped
        else OWNERS[native_owner]
        if receiver
        else ""
    )
    if scoped:
        values.supported(receiver.value)
    name = plan.member
    release = bool(
        receiver
        and receiver.value.handle
        and plan.name == receiver.value.handle.release
    )
    if release:
        name = "close"
    if plan.view:
        name = plan.view.stem
    if iskeyword(name) or name in RUST_KEYWORDS:
        name += "_"
    if not safe_identifier(name):
        raise unsupported(function, f"method identifier {name!r} is not safe")
    if exit:
        name = "_exit"
    elif name in OWNER_MEMBERS and not release:
        raise unsupported(
            function, f"method identifier {name!r} collides with an owner member"
        )
    if plan.consumes and receiver is None:
        raise unsupported(function, "consuming operation needs a matching owner state")
    rust_parameters, py_parameters, arguments, setup = (
        [],
        [],
        {},
        ["        let mut call = GeneratedCall::new(py);"],
    )
    python_arguments, native_signature = [], []
    counts = {
        p.value.length for p in plan.inputs if p.value.kind in {"array", "buffer"}
    }
    # Caller-sized text passes its storage, capacity, and length to a closure
    # that the runtime calls again when the text did not fit.
    text = plan.caller_sized_text
    if text:
        counts.add(text.value.length)
    used_names = set(LOCAL_NAMES) | {p.name for p in plan.inputs}
    for parameter in plan.inputs:
        if (receiver and parameter.name == receiver.name) or parameter.name in counts:
            continue
        value = parameter.value
        # An options record may be omitted, as may a release's inputs, since
        # every handle closes with no arguments. Any other record is the
        # subject of the call, which C requires.
        record = value.element if value.kind == "reference" else value
        has_default = bool(
            record
            and record.default
            and (release or exit or record.native.endswith("_options"))
        )
        local = parameter.name
        if not safe_identifier(local) or local in LOCAL_NAMES:
            local = "input_" + local
            while local in used_names:
                local += "_"
        used_names.add(local)
        if value.kind == "handle" and value.native in OWNERS:
            public = OWNERS[value.native]
            rust = f"&{public}"
            setup.append(f"        let {local}_handle = {local}.input();")
            arguments[parameter.name] = f"{local}_handle"
        else:
            values.supported(value, input=True)
            public = values.type(value, input=True)
        if value.kind == "handle" and value.native in OWNERS:
            pass
        elif value.kind == "scalar":
            rust = scalar_type(value)
            arguments[parameter.name] = local
        elif value.kind == "enum":
            rust = "sys::" + value.native
            arguments[parameter.name] = local
        else:
            rust = "&Bound<'_, PyAny>"
            if value.nullable or value.optional == "empty" or has_default:
                rust = "Option<Bound<'_, PyAny>>"
                setup.append(
                    f"        let {local} = {local}.unwrap_or_else(|| py.None().into_bound(py));"
                )
            if value.kind == "buffer" and value.ctype.pointee and value.length != "nul":
                setup.append(
                    f"        let {local}_view = storage.buffer({local}.clone(), {str(value.encoding == 'utf8').lower()})?;"
                )
                arguments[parameter.name] = f"{local}_view.data.cast()"
                arguments[value.length] = f"{local}_view.size"
            elif value.kind == "array" and value.ctype.kind != "array":
                if value.element is None:
                    raise unsupported(function, "array requires an element plan")
                converted = values.input(value.element, "item")
                setup.append(
                    f"        let {local}_values = generated_items({local}, |item| {ok(converted)})?;"
                )
                arguments[parameter.name] = f"{local}_values.as_ptr()"
                if value.length and value.length.isdigit():
                    setup.append(
                        f'        if {local}_values.len() != {value.length} {{ return Err(invalid_argument_error("wrong fixed array length")); }}'
                    )
                else:
                    arguments[value.length] = f"{local}_values.len()"
            elif value.kind == "reference" and value.element:
                converted = values.input(value.element, local + ".clone()")
                if value.nullable:
                    converted = values.input(value.element, local)
                    setup.append(
                        f"        let {local}_value = generated_maybe(&{local}, |{local}| {ok(converted)})?;"
                    )
                    arguments[parameter.name] = f"generated_pointer(&{local}_value)"
                else:
                    mutable = (
                        not value.ctype.pointee.const if value.ctype.pointee else False
                    )
                    setup.append(
                        f"        let {'mut ' if mutable else ''}{local}_value = {converted};"
                    )
                    arguments[parameter.name] = (
                        f"&{'mut ' if mutable else ''}{local}_value"
                    )
            else:
                converted = values.input(value, local + ".clone()")
                setup.append(f"        let {local}_value = {converted};")
                arguments[parameter.name] = f"{local}_value"
        rust_parameters.append(f"{local}: {rust}")
        absent = value.nullable or value.optional == "empty" or has_default
        if has_default and "None" not in public:
            public += " | None"
        py_parameters.append(f"{local}: {public}" + (" = None" if absent else ""))
        python_arguments.append(local + "._native" if value.kind == "handle" else local)
        native_signature.append(local + ("=None" if absent else ""))
    for i, declaration in enumerate(py_parameters):
        if declaration.endswith(" = None") and any(
            not later.endswith(" = None") for later in py_parameters[i + 1 :]
        ):
            py_parameters[i] = declaration.removesuffix(" = None")
            native_signature[i] = native_signature[i].removesuffix("=None")
    setup.insert(1, "        let storage = &mut call.storage;")
    abandon = bool(
        receiver
        and receiver.value.handle
        and plan.name == receiver.value.handle.abandon
    )
    if scoped:
        setup.append(
            f"        let handle = self.scope.pointer(self.native)? as *mut sys::{native_owner};"
        )
    elif plan.consumes or abandon:
        # A synchronous release is idempotent; an answer that consumes its
        # receiver, like an asynchronous release, reaches native again.
        idempotent = (
            "false"
            if (plan.completion and not exit) or (plan.consumes and not plan.release)
            else "true"
        )
        setup += [
            f"        let Some({'mut ' if plan.consumes else ''}reservation) = self.reserve({idempotent})? else {{ return Ok(py.None()); }};",
            "        let mut handle = reservation.handle();"
            if receiver_pointer
            else "        let handle = reservation.handle();",
        ]
    elif (
        receiver
        and not plan.completion
        and not plan.view
        and plan.execution == "immediate"
        and any(
            borrows_memory(output.value)
            for output in plan.outputs
            if not output.sized_by and not output.result_length
        )
    ):
        setup += [
            "        let read = self.read()?;",
            "        let handle = read.native;",
        ]
    elif receiver:
        setup += [
            "        let handle = self.input();",
        ]
    if receiver:
        arguments[receiver.name] = (
            "&mut handle" if receiver_pointer and not scoped else "handle"
        )
    outputs = []
    product = None
    for output in plan.outputs:
        if text and output.name == text.result_length:
            continue
        if output is text:
            setup.append(f"        let mut {output.name}: Vec<u8> = Vec::new();")
            for control in (output.name, output.value.length, output.result_length):
                arguments[control] = control
            outputs.append((output.name, output.value))
            continue
        value = output.value.unwrapped
        if value is None:
            raise unsupported(function, "output requires an element value")
        if (
            value.kind == "handle"
            and value.ownership == "owned"
            and value.native in OWNERS
        ):
            pass
        else:
            values.supported(value)
        if output.sized_by:
            element = value.element
            native = (
                scalar_type(element)
                if element.kind == "scalar"
                else "sys::" + element.native
            )
            count = arguments[value.length]
            init = (
                f"unsafe {{ sys::{element.default}() }}"
                if element.default
                else "unsafe { std::mem::zeroed() }"
            )
            setup.append(
                f"        let mut {output.name}: Vec<{native}> = (0..{count}).map(|_| {init}).collect();"
            )
            for field in element.fields:
                if field.role == "size":
                    setup.append(
                        f"        for value in &mut {output.name} {{ value.size = std::mem::size_of::<{native}>() as _; }}"
                    )
            arguments[output.name] = f"{output.name}.as_mut_ptr()"
            outputs.append((output.name, value))
            continue
        native = (
            scalar_type(value) if value.kind == "scalar" else "sys::" + value.native
        )
        init = (
            f"unsafe {{ sys::{value.default}() }}"
            if value.default
            else "unsafe { std::mem::zeroed() }"
        )
        setup.append(f"        let mut {output.name}: {native} = {init};")
        if value.kind == "record":
            for field in value.fields:
                if field.role == "size":
                    setup.append(
                        f"        {output.name}.{field.name} = std::mem::size_of::<{native}>() as _;"
                    )
        arguments[output.name] = f"&mut {output.name}"
        outputs.append((output.name, value))

    def copy_output(output, value):
        if text and output == text.name:
            return f"PyBytes::new(py, &{output}).into_any().unbind()"
        if any(p.name == output and p.sized_by for p in plan.outputs):
            copied = values.copy(value.element, "*element")
            return f"generated_list(py, &{output}, |element| {ok(copied)})?"
        return values.copy(value, output)

    if plan.completion:
        arguments[plan.completion.parameter] = "completion"
        if discard:
            native_result = public_result = "None"
            helper, converter = "discard", None
        elif plan.execution == "command":
            native_result = public_result = "Future[CommandCompletion]"
            helper, converter = "command", None
        else:
            native_result, public_result, converter, _ = result_converter(plan, values)
            native_result, public_result = (
                f"Future[{native_result}]",
                f"Future[{public_result}]",
            )
            helper = "complete"
        call = native_call(function, [arguments[p.name] for p in function.parameters])
        # The converters run outside the native call's unsafe contract.
        body = [
            *setup,
            *(
                [
                    "        let convert = "
                    + converter.replace(
                        "|py, result|",
                        "|py: Python<'_>, result: &sys::mln_completion_result|",
                        1,
                    )
                    + ";"
                ]
                if converter
                else []
            ),
            f"        unsafe {{ call.{helper}(|completion, diagnostic| {call}"
            + (", convert" if converter else "")
            + ") }",
        ]
        expression = f"self._native.{name}({', '.join(python_arguments)})"
        if plan.consumes:
            body[-1] += "?;"
            body[-1] = body[-1].replace(
                "        unsafe {", "        let future = unsafe {", 1
            )
            body.extend(["        reservation.commit();", "        Ok(future)"])
        if outputs:
            if any(
                value.kind != "handle" or value.ownership != "owned"
                for _, value in outputs
            ):
                raise unsupported(
                    function,
                    "completion with immediate values requires a result product",
                )
            body[-1] += "?;"
            body[-1] = body[-1].replace(
                "        unsafe {", "        let future = unsafe {", 1
            )
            for output, value in outputs:
                values.disposed.add(value.native)
                body.append(
                    f"        let mut {output}_owner = GeneratedOwnedOutput::new({output}, generated_dispose_{value.native});"
                )
            for output, value in outputs:
                body.append(
                    f"        let {output}_python = {owned_native(value, output + '_owner.take()', bool(plan.registrations))}?;"
                )
            body.append("        let result = PyDict::new(py);")
            for output, _ in outputs:
                body.append(
                    f'        result.set_item("{output_member(output)}", {output}_python)?;'
                )
            body.append('        result.set_item("ready", future)?;')
            body.append("        Ok(result.into_any().unbind())")
            product_name = public_name(function.name) + "Result"
            product_fields = [
                f"    {output_member(output)}: {OWNERS[value.native]}"
                for output, value in outputs
            ]
            product_fields.append(f"    ready: {public_result}")
            product = (
                "class "
                + product_name
                + "(NamedTuple):\n"
                + "\n".join(product_fields)
                + "\n"
            )
            parent_names = {
                owner.parameter: owner.parent_parameter for owner in plan.owned_outputs
            }
            public_copies = []
            for output, value in outputs:
                parent = parent_names.get(output)
                parent = (
                    "self" if receiver and parent == receiver.name else parent or "None"
                )
                public_copies.append(
                    f"{output_member(output)}=_adopt_value(raw[{output_member(output)!r}], {OWNERS[value.native]!r}, {parent})"
                )
            public_copies.append('ready=raw["ready"]')
            expression = f"{product_name}(" + ", ".join(public_copies) + ")"
            public_result = product_name
            native_result = "dict[str, Any]"
        elif plan.result:
            converted = values.facade_copy(plan.result, "value")
            if converted != "value":
                expression = f"map_future({expression}, lambda value: {converted})"
    else:
        if dropped_completion:
            arguments[dropped_completion] = "std::ptr::null()"
        call = native_call(function, [arguments[p.name] for p in function.parameters])
        if text:
            call = f"generated_caller_sized_text(&mut {text.name}, |{text.name}, {text.value.length}, {text.result_length}| {call})"
        status = plan.status
        absence = plan.absence
        body = setup + [
            f"        let result = unsafe {{ call.status_unless(sys::{absence.status}, |diagnostic| {call}) }};"
            if absence
            else f"        let result = unsafe {{ call.status(|diagnostic| {call}) }};"
            if status
            else f"        let result = unsafe {{ call.run(|| {call}) }};"
        ]
        if status:
            if plan.consumes == "always":
                body.append("        reservation.commit();")
            body.append(
                "        if !result? { return Ok(PyList::empty(py).into_any().unbind()); }"
                if plan.drain
                else "        if !result? { return Ok(py.None()); }"
                if absence
                else "        result?;"
            )
            if plan.consumes and plan.consumes != "always":
                body.append("        reservation.commit();")
        elif plan.result:
            values.supported(plan.result)
            outputs.insert(0, ("result", plan.result))
        elif ctype(function.return_type) != "void":
            raise unsupported(function, "return needs a resolved value")
        else:
            body[-1] = body[-1].replace("let result = ", "")
            if plan.consumes:
                body.append("        reservation.commit();")
        public_result = "None"
        native_result = "Any"
        expression = f"self._native.{name}({', '.join(python_arguments)})"
        if plan.drain:
            # The batch releases when this frame ends, after the copy or as
            # the copy fails.
            drain = plan.drain
            batch = drain.batch.value.unwrapped
            values.disposed.add(batch.native)
            body.append(
                f"        let _{drain.batch.name} = GeneratedOwnedOutput::new({drain.batch.name}, generated_dispose_{batch.native});"
            )
            copied = values.copy(
                drain.slice.value,
                f"{drain.view.name}.{rust_field(drain.slice.name)}",
                scope=drain.view.name,
            )
            body.append(f"        {ok(copied)}")
            public_result = values.type(drain.slice.value)
            expression = values.facade_copy(drain.slice.value, expression)
        elif not outputs:
            body.append("        Ok(py.None())")
        elif len(outputs) == 1:
            output, value = outputs[0]
            if value.kind == "handle" and value.ownership == "owned":
                result_owner = OWNERS[value.native]
                body.append(
                    f"        {owned_native(value, output, bool(plan.registrations))}"
                )
                public_result = result_owner
                parent = (
                    "self"
                    if receiver and value.handle and value.handle.parent
                    else "None"
                )
                if absence:
                    public_result += " | None"
                    expression = f"_maybe(lambda raw: _adopt_value(raw, {result_owner!r}, {parent}), {expression})"
                else:
                    expression = (
                        f"_adopt_value({expression}, {result_owner!r}, {parent})"
                    )
            else:
                if absence:
                    raise unsupported(
                        function, "absence requires an owned handle output"
                    )
                body.append(f"        {ok(copy_output(output, value))}")
                public_result = values.type(value)
                expression = values.facade_copy(value, expression)
        else:
            body.append("        let dict = PyDict::new(py);")
            for output, value in outputs:
                body.append(
                    f'        dict.set_item("{output_member(output)}", {copy_output(output, value)})?;'
                )
            body.append("        Ok(dict.into_any().unbind())")
            product_name = public_name(function.name) + "Result"
            product = (
                "class "
                + product_name
                + "(NamedTuple):\n"
                + "\n".join(
                    f"    {output_member(output)}: {values.type(value)}"
                    for output, value in outputs
                )
                + "\n"
            )
            expression = (
                f"{product_name}("
                + ", ".join(
                    values.facade_copy(value, f"raw[{output_member(output)!r}]")
                    for output, value in outputs
                )
                + ")"
            )
            public_result = product_name
    if plan.registrations:
        if len(outputs) > 1:
            raise unsupported(
                function, "multiple owners require an explicit callback owner"
            )
        if plan.completion and not outputs:
            index = next(i for i, line in enumerate(body) if f"{helper}(" in line)
            body[index] = (
                body[index].replace(
                    "        unsafe {", "        let future = unsafe {", 1
                )
                + "?;"
            )
            body.append("        Ok(future)")
        if plan.completion:
            accepted = (
                next(i for i, line in enumerate(body) if "let future = " in line) + 1
            )
        else:
            accepted = next(i for i, line in enumerate(body) if "result?;" in line) + 1
        has_owned_output = any(
            value.kind == "handle" and value.ownership == "owned"
            for _, value in outputs
        )
        declined = next(
            (r.accepted_unless for r in plan.registrations if r.accepted_unless), None
        )
        accept = (
            f"if !{declined} {{ call.accept_callbacks() }} else {{ Vec::new() }}"
            if declined
            else "call.accept_callbacks()"
        )
        if not has_owned_output and not receiver:
            body.insert(accepted, f"        {accept};")
        else:
            body.insert(accepted, f"        let callback_roots = {accept};")
        if not has_owned_output and receiver:
            body.insert(
                accepted + 1,
                "        self.state().retain_callback_roots(callback_roots);",
            )
    if not any("storage" in line for line in body[2:]):
        body = [line for line in body if "let storage =" not in line]
    body = compact(body)
    rust_arguments = ", ".join(["&self", "py: Python<'_>", *rust_parameters])
    rust = f"""    #[pyo3(signature = ({", ".join(native_signature)}))]
    fn {"_discard_" if discard else ""}{name}({rust_arguments}) -> PyResult<Py<PyAny>> {{
{chr(10).join(body)}
    }}
"""
    if plan.view:
        if len(outputs) != 1:
            raise unsupported(function, "borrowed view needs one typed output")
        invoke = f"self._native.{name}({', '.join(python_arguments)})"
        converted = values.facade_copy(outputs[0][1], "raw")
        expression = f"_view(self, lambda: {invoke}, lambda raw: {converted})"
        public_result = f"AbstractContextManager[{public_result}]"
    native_stub_parameters = py_parameters
    signature = ", ".join(["self", *py_parameters])
    facade_setup = (
        f"        raw = self._native.{name}({', '.join(python_arguments)})\n"
        if product
        else ""
    )
    documentation = docs.docstring(
        values.api.doc(function.name),
        "        ",
        ("Native resources remain valid only inside the `with` block.",)
        if plan.view
        else (),
    )
    facade = f"""    def {name}({signature}) -> {public_result}:
{documentation}{facade_setup}        return {expression}
"""
    if discard:
        facade = facade.replace(
            f"self._native.{name}(", f"self._native._discard_{name}("
        )
        stub = f"    def _discard_{name}({', '.join(['self', *native_stub_parameters])}) -> None: ...\n"
        return owner, rust, facade, stub, product
    stub = f"    def {name}({', '.join(['self', *native_stub_parameters])}) -> {native_result}: ...\n"
    if not receiver:
        rust = (
            dedent(rust)
            .replace("#[pyo3(signature", "#[pyfunction]\n#[pyo3(signature")
            .replace("(&self, ", "(")
        )
        facade = (
            dedent(facade)
            .replace("(self, ", "(")
            .replace("(self)", "()")
            .replace("self._native.", "_native.")
        )
        stub = dedent(stub).replace("(self, ", "(").replace("(self)", "()")
    return owner, rust, facade, stub, product


def scoped_release(handle: HandlePlan, bound: BoundApi) -> str:
    """The operation that `__exit__` calls.

    A release that reports teardown through a completion would return a future
    that leaving the block drops, so the block disposes the handle instead, as
    it does a handle whose release refuses some states. A dispose that is also
    the release disposes with a null completion.
    """
    release = bound.operations_by_name.get(handle.release)
    if handle.dispose and release is not None and release.completion is not None:
        return handle.dispose
    return handle.scoped_release


def disposal(bound: BoundApi, handle: HandlePlan, name: str, receiver: str) -> str:
    """A native call that disposes `receiver` with `name`, passing a null
    completion to a dispose that is also the release, and a null diagnostic."""
    function = bound.source.functions_by_name[name]
    arguments = [
        "std::ptr::null()" if p.name == handle.dispose_completion else receiver
        for p in function.parameters
    ]
    return native_call(function, arguments, diagnostic="std::ptr::null_mut()")


def compact(body: list[str]) -> list[str]:
    """Checks a status where the call makes it, and returns a final copy as is."""
    result = []
    for line in body:
        if (
            line == "        result?;"
            and result
            and result[-1].startswith("        let result = ")
        ):
            result[-1] = (
                "        "
                + result[-1].strip().removeprefix("let result = ")[:-1]
                + "?;"
            )
            continue
        result.append(line)
    last = result[-1].strip()
    if last.startswith("Ok(") and last.endswith("?)") and last.count("?") == 1:
        result[-1] = "        " + last[3:-2]
    return result


def state_owner(owner: str, handle: HandlePlan, bound: BoundApi) -> str:
    """Declare the PyO3 class for a handle whose ownership NativeHandleState tracks."""
    native = handle.native
    counted = counted_owner(bound, native)
    read_scope = (
        f", |py, owner| GeneratedReadScope::with_native::<sys::{native}>(py, Arc::clone(&owner.state), sys::{handle.view_begin}, sys::{handle.view_end})"
        if handle.view_begin
        else ""
    )
    dispose = (
        f"Some(generated_dispose_{native})"
        if handle.cleanup and handle.cleanup not in RUNTIME_EXPORTS
        else "None"
    )
    exit_release = "None"
    if handle.observable_root_release:
        exit_release = f"Some(generated_exit_release_{native})"
        call = native_call(
            bound.source.functions_by_name[handle.release],
            ["handle", "completion"],
            diagnostic="std::ptr::null_mut()",
        )
        definitions = f'unsafe extern "C" fn generated_exit_release_{native}(handle: sys::{native}, completion: *const sys::mln_completion) -> sys::mln_status {{ unsafe {{ {call} }} }}\n'
    else:
        definitions = ""
    return (
        definitions
        + f'generated_owner!({owner}, "_{owner}", {native}, {dispose}, {exit_release}, {str(counted).lower()}{read_scope});\n'
    )


def lower(api: Api | BoundApi) -> tuple[dict[str, str], list[str], dict[str, str]]:
    rust, facade, stubs = defaultdict(list), defaultdict(list), defaultdict(list)
    generated, errors, records = [], {}, set()
    discards = defaultdict(list)
    bound = compile_api(api, TARGETS)
    api = bound.source
    OWNERS.clear()
    OWNERS.update(
        {
            name: pascal(handle.stem) + "Handle"
            for name, handle in bound.public_handles.items()
        }
    )
    RUNTIME_EXPORTS.clear()
    RUNTIME_EXPORTS.update(bound.source.runtime_exports)
    values = Values(bound)
    for value in bound.public_values.values():
        if value.kind == "enum":
            values.supported(value)
    errors.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    exits = {
        scoped_release(handle, bound)
        for handle in bound.handles.values()
        if handle.native in OWNERS
    }
    for plan in bound.operations:
        function = plan.function
        if plan.name in exits:
            owner, native, _, stub, _ = operation(plan, values, exit=True)
            rust[owner].append(native)
            stubs[owner].append(stub)
        saved_values = (
            dict(values.records),
            dict(values.inputs),
            dict(values.enums),
            set(values.outputs),
            set(values.disposed),
        )
        try:
            owner, native, public, stub, record = operation(plan, values)
        except ModelError as error:
            (
                values.records,
                values.inputs,
                values.enums,
                values.outputs,
                values.disposed,
            ) = saved_values
            errors[function.name] = str(error)
            continue
        rust[owner].append(native)
        facade[owner].append(public)
        stubs[owner].append(stub)
        if plan.receiver and plan.completion and plan.completion.discardable:
            _, discarded_native, discarded_public, discarded_stub, _ = operation(
                plan, values, discard=True
            )
            rust[owner].append(discarded_native)
            stubs[owner].append(discarded_stub)
            discards[owner].append(discarded_public)
        generated.append(function.name)
        if record:
            records.add(record)
    default_native = []
    default_names = []
    for plan in bound.operations:
        if not isinstance(plan.support, DefaultSupport) or plan.result is None:
            continue
        saved = (
            dict(values.records),
            dict(values.inputs),
            dict(values.enums),
            set(values.outputs),
        )
        try:
            values.supported(plan.result)
        except ModelError:
            values.records, values.inputs, values.enums, values.outputs = saved
            continue
        values.defaults.add(plan.result.native)
        errors.pop(plan.name, None)
        name = "_default_" + plan.result.native.removeprefix("mln_")
        default_names.append(name)
        default_native.append(
            f"#[pyfunction]\nfn {name}(py: Python<'_>) -> PyResult<Py<PyAny>> {{ let value = unsafe {{ sys::{plan.name}() }}; {ok(values.copy(plan.result, 'value'))} }}\n"
        )
    files = error_sources()
    rust_values, public_values = values.sources()
    rust_values += "\n".join(default_native)
    for handle in bound.handles.values():
        if not handle.cleanup or handle.cleanup in bound.source.runtime_exports:
            continue
        state_owned = handle.native in OWNERS
        if not state_owned and handle.native not in values.disposed:
            continue
        support = bound.operations_by_name.get(handle.cleanup)
        if support and isinstance(support.support, DisposeSupport):
            errors.pop(handle.cleanup, None)
        disposer = bound.source.functions_by_name[handle.cleanup]
        call = f"unsafe {{ {disposal(bound, handle, handle.cleanup, 'handle')} }}"
        if ctype(disposer.return_type) == "void":
            call += "; sys::MLN_STATUS_OK"
        rust_values += f'\nunsafe extern "C" fn generated_dispose_{handle.native}(handle: sys::{handle.native}) -> sys::mln_status {{ {call} }}\n'

    notice = "Generated from the C headers by tools/bindgen. Do not edit."
    files["src/generated_operations.rs"] = (
        "// "
        + notice
        + "\n\n"
        + rust_values
        + "\n"
        + "\n".join(
            f"#[pymethods]\nimpl {owner} {{\n{''.join(methods)}}}\n"
            for owner, methods in sorted(rust.items())
            if owner
        )
    )
    for native_owner, owner in OWNERS.items():
        files["src/generated_operations.rs"] += state_owner(
            owner, bound.handles[native_owner], bound
        )
    scope_owners = {
        name: public_name(name) + "Scope"
        for name, value in values.records.items()
        if value.response
    }
    global_native = "\n".join(rust.get("", []))
    global_names = default_names + [
        plan.member
        for plan in bound.operations
        if plan.name in generated
        and plan.receiver is None
        and plan.scoped_receiver is None
    ]
    files["src/generated_operations.rs"] += (
        global_native
        + "\nfn register_generated_functions(module: &Bound<'_, PyModule>) -> PyResult<()> {\n"
        + "    module.add_class::<GeneratedReadScope>()?;\n"
        + "\n".join(
            f"    module.add_class::<{owner}>()?;"
            for owner in [*OWNERS.values(), *scope_owners.values()]
        )
        + "\n".join(
            f"    module.add_function(wrap_pyfunction!({name}, module)?)?;"
            for name in global_names
        )
        + "\n    Ok(())\n}\n"
    )
    files["python/maplibre_native_ffi/_native.pyi"] = (
        f"# {notice}\nfrom collections.abc import Buffer, Sequence\nfrom concurrent.futures import Future\nfrom typing import Any, Callable\nfrom ._completion import CommandCompletion\nfrom ._generated_values import *\n\n"
        + "\n".join(
            f"class _{owner}:\n    _is_closed: bool\n    id: int\n{''.join(methods)}"
            for owner, methods in sorted(stubs.items())
            if owner
        )
    )
    files["python/maplibre_native_ffi/_native.pyi"] += "\n".join(stubs.get("", []))
    if any(
        value.registration and async_hosts.accepts_native(bound, value)
        for value in values.records.values()
    ):
        files["python/maplibre_native_ffi/_native.pyi"] += (
            "\nclass _NativeRegistration:\n"
            "    def __init__(self, kind: str, callbacks: tuple[int, ...], user_data: int, release: int) -> None: ...\n"
        )
    files["python/maplibre_native_ffi/_native.pyi"] += "\n" + "\n".join(
        f"def {name}() -> Any: ..." for name in default_names
    )
    for owner in OWNERS.values():
        files["python/maplibre_native_ffi/_native.pyi"] = files[
            "python/maplibre_native_ffi/_native.pyi"
        ].replace(f": {owner}", f": _{owner}")
    for owner in discards:
        facade[owner].append(
            f"    @property\n    def discarding(self) -> {owner}Discarding:\n        return {owner}Discarding(self)\n"
        )
    imports = "\n".join(
        f"from ._generated_values import {public_name(record)}"
        for record in sorted(values.records | values.enums)
        if not values.records.get(record, values.enums.get(record)).response
    )
    files["python/maplibre_native_ffi/_generated_operations.py"] = (
        f'"""{notice}"""\n\nfrom __future__ import annotations\n\nfrom collections.abc import Buffer, Sequence\nfrom concurrent.futures import Future\nfrom contextlib import AbstractContextManager\nfrom dataclasses import dataclass\nfrom typing import TYPE_CHECKING, Any, Callable, NamedTuple\nfrom . import _native\nfrom ._completion import CommandCompletion\nfrom ._future import map_future\nfrom ._operation import GeneratedOperations, _adopt_value, _view\nfrom ._generated_values import _maybe\n{imports}\nif TYPE_CHECKING:\n    from ._generated_owners import {", ".join([*OWNERS.values(), *scope_owners.values()]) or "__name__"}\n\n'
        + "\n".join(sorted(records))
        + "\n".join(
            f"class _{owner}Operations(GeneratedOperations):\n    _native: _native._{owner}\n\n{chr(10).join(methods)}"
            for owner, methods in sorted(facade.items())
            if owner
        )
    )
    files["python/maplibre_native_ffi/_generated_operations.py"] += "\n".join(
        facade.get("", [])
    )
    files["python/maplibre_native_ffi/_generated_operations.py"] += "\n".join(
        f"class {owner}Discarding:\n    __slots__ = ('_native',)\n    _native: _native._{owner}\n\n    def __init__(self, owner: _{owner}Operations):\n        self._native = owner._native\n\n"
        + "\n".join(methods)
        for owner, methods in discards.items()
    )
    files["python/maplibre_native_ffi/_generated_values.py"] = (
        f'"""{notice}"""\n\nfrom __future__ import annotations\nfrom collections.abc import Buffer, Sequence\nfrom dataclasses import dataclass, field\nfrom enum import IntFlag\nfrom typing import TYPE_CHECKING, Protocol\nfrom ._enum import UnknownIntEnum\n'
        + (
            "from ._operation import _wrap_response\n"
            if any(value.response for value in values.records.values())
            else ""
        )
        + f"\nif TYPE_CHECKING:\n    from ._generated_owners import {', '.join([*OWNERS.values(), *scope_owners.values()]) or '__name__'}\n\n"
        + public_values
    )
    for owner in scope_owners.values():
        files["src/generated_operations.rs"] += f"""\n#[pyclass(name = "_{owner}")]
struct {owner} {{ native: usize, scope: GeneratedCallbackScope }}
"""
    owner_classes = []
    for owner in scope_owners.values():
        owner_classes.append(f"""class {owner}(_{owner}Operations):
    __module__ = "maplibre_native_ffi"

    def __init__(self):
        raise TypeError("response scopes are supplied during callbacks")

    @classmethod
    def _from_native(cls, native):
        value = cls.__new__(cls)
        value._native = native
        return value
""")
    sources = event_sources(api)
    for native, owner in OWNERS.items():
        handle = bound.handles[native]
        # A handle without scoped cleanup, such as a frame, has no `with` block.
        mixin = (
            "NativeHandleMixin"
            if handle.scoped_release is None
            else "ScopedHandleMixin"
        )
        id_doc = (
            "The native handle value, which runtime events report as their source.\n\n"
            "        It stays readable after close, so events drained later still match\n"
            "        the handle that produced them.\n        "
            if native in sources
            else "The native handle value. It stays readable after close."
        )
        # The dispose is the scoped release, which `__exit__` also calls.
        dispose = (
            f"""
    def dispose(self) -> None:
{docs.docstring(bound.doc(handle.dispose), "        ")}        self._native._exit()
"""
            if public_dispose(handle)
            else ""
        )
        owner_classes.append(f'''class {owner}(_{owner}Operations, {mixin}):
{docs.docstring(bound.doc(native), "    ")}    __module__ = "maplibre_native_ffi"
    _handle_name = "{owner}"

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

    @property
    def id(self) -> int:
        """{id_doc}"""
        return int(self._native.id)
{dispose}''')
    files["python/maplibre_native_ffi/_generated_owners.py"] = (
        f'"""{notice}"""\nfrom ._lifecycle import NativeHandleMixin, ScopedHandleMixin\n'
        + "\n".join(
            f"from ._generated_operations import _{owner}Operations"
            for owner in [*OWNERS.values(), *scope_owners.values()]
        )
        + "\n\n"
        + "\n".join(owner_classes)
    )
    public_values_names = sorted(
        node.name
        for node in ast.parse(public_values).body
        if isinstance(node, ast.ClassDef)
    )
    public_globals = sorted(
        plan.member
        for plan in bound.operations
        if plan.name in generated
        and plan.receiver is None
        and plan.scoped_receiver is None
    )
    public_globals.extend(f"{owner}Discarding" for owner in discards)
    public_owners = {
        "_generated_owners": list(OWNERS.values()) + list(scope_owners.values())
    }
    exports = (
        public_values_names
        + public_globals
        + list(OWNERS.values())
        + list(scope_owners.values())
    )
    files["python/maplibre_native_ffi/api.py"] = (
        f'"""{notice}"""\n'
        + "\n".join(
            f"from ._generated_values import {name}" for name in public_values_names
        )
        + "\n"
        + "\n".join(
            f"from ._generated_operations import {name}" for name in public_globals
        )
        + "\n"
        + "\n".join(
            f"from .{module} import {name}"
            for module, names in public_owners.items()
            for name in names
        )
        + "\n\n__all__ = "
        + repr(sorted(set(exports)))
        + "\n"
    )
    return files, generated, errors


def generate(api: Api | BoundApi) -> dict[str, str]:
    return lower(api)[0]


def coverage(api: Api | BoundApi) -> dict:
    bound = compile_api(api, TARGETS)
    _, generated, errors = lower(bound)
    support = [
        plan.name
        for plan in bound.operations
        if plan.support and plan.name not in errors
    ]
    return {
        "generated": generated,
        "support": support,
        "unsupported": errors,
    }


def error_sources():
    return {
        "python/maplibre_native_ffi/errors.py": '"""Generated from C headers by tools/bindgen. Do not edit."""\n\nfrom ._error import AbiVersionMismatchError, MaplibreError\n\n__all__ = ["AbiVersionMismatchError", "MaplibreError"]\n',
        "src/generated_errors.rs": """// Generated from C headers by tools/bindgen. Do not edit.
mod py_errors {
    pyo3::import_exception!(maplibre_native_ffi.errors, MaplibreError);
    pyo3::import_exception!(maplibre_native_ffi.errors, AbiVersionMismatchError);
}
fn map_error(error: Error) -> PyErr {
    match error {
        Error::Status { status, diagnostic, generation } => py_errors::MaplibreError::new_err((status.as_raw(), diagnostic, generation)),
        Error::AbiVersionMismatch { expected, actual } => py_errors::AbiVersionMismatchError::new_err((expected, actual)),
    }
}
""",
    }
