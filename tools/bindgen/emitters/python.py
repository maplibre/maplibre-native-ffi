"""Generate Python operations, values, stubs, and their PyO3 calls together."""

from __future__ import annotations

import ast
from collections import defaultdict
from dataclasses import replace
from keyword import iskeyword
from textwrap import dedent

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters.rust import RUST_KEYWORDS, native_call
from tools.bindgen.emitters.rust_callbacks import decision_table
from tools.bindgen.model import Api, CType, Function, ModelError, Record
from tools.bindgen.semantic import (
    BoundApi,
    DecisionPlan,
    DefaultSupport,
    DisposeSupport,
    HandlePlan,
    OperationPlan,
    output_member,
    support_relation,
    view_support,
)

from .python_values import Values, ok, public_name, scalar_type

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
# Public owner class names by native handle type.
OWNERS: dict[str, str] = {}
# Runtime adapter exports, which no owner disposes through.
RUNTIME_EXPORTS: set[str] = set()


def owned_decision(bound: BoundApi, native: str) -> DecisionPlan | None:
    """The decision protocol that issues an owned handle type, if any.

    Its owner holds the core decision state instead of a NativeHandleState.
    """
    return bound.decisions.get(native) if native in OWNERS else None


def ctype(type_: CType) -> str:
    return type_.declaration or type_.spelling.removeprefix("const ")


def pascal(name: str) -> str:
    return "".join(part.capitalize() for part in name.split("_"))


def unsupported(function: Function | Record, reason: str) -> ModelError:
    return ModelError([f"{function.location}: Python: {function.name}: {reason}"])


# Members every handle owner defines. The release operation alone becomes
# `close`; any other operation with one of these names would shadow a member.
OWNER_MEMBERS = {"state", "closed", "close", "id"}
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
        if owner is None or owned_decision(values.api, result.native):
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
        copied = values.copy(element, "*value")
        converter = f"""|py, result| {{
            {"if result.value.is_null() { return Ok(py.None()); }" if result.nullable else ""}
            let list = PyList::empty(py);
            for value in generated_completion_slice::<sys::{element.native}>(result)? {{
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
    plan: OperationPlan, values: Values
) -> tuple[str, str, str, str, str | None]:
    function = plan.function
    if plan.direct_registrations:
        from .python_callbacks import direct_operation

        return direct_operation(plan, values)
    if plan.role == "support":
        raise unsupported(
            function, "support operation is consumed by the value/runtime compiler"
        )
    if any(p.consumes for p in plan.inputs if p.name != plan.receiver):
        raise unsupported(function, "consuming parameter requires an owner transaction")
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
    decision = owned_decision(values.api, native_owner)
    name = plan.member
    release = bool(
        receiver
        and receiver.value.handle
        and plan.name == receiver.value.handle.release
    )
    if release:
        name = "close"
    if plan.view:
        name = "with_" + plan.view.stem
    if iskeyword(name) or name in RUST_KEYWORDS:
        name += "_"
    if not safe_identifier(name):
        raise unsupported(function, f"method identifier {name!r} is not safe")
    if name in OWNER_MEMBERS and not release:
        raise unsupported(
            function, f"method identifier {name!r} collides with an owner member"
        )
    if plan.consumes and receiver is None:
        raise unsupported(function, "consuming operation needs a matching owner state")
    receiver_identity = (
        "self.native as u64"
        if scoped
        else "maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle())"
        if decision
        else "self.admission()"
        if receiver
        else "0"
    )
    rust_parameters, py_parameters, arguments, setup = (
        [],
        [],
        {},
        [
            f'        let mut call = GeneratedCall::new(py, "{plan.name}", {receiver_identity})?;'
        ],
    )
    python_arguments, native_signature = [], []
    counts = {
        p.value.length for p in plan.inputs if p.value.kind in {"array", "buffer"}
    }
    used_names = set(LOCAL_NAMES) | {p.name for p in plan.inputs}
    for parameter in plan.inputs:
        if (receiver and parameter.name == receiver.name) or parameter.name in counts:
            continue
        value = parameter.value
        has_default = bool(
            value.default
            or (value.kind == "reference" and value.element and value.element.default)
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
            setup.append(f"        let {local}_handle = {local}.input()?;")
            arguments[parameter.name] = f"{local}_handle"
        else:
            values.supported(value, input=True)
            public = values.type(value)
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
    if decision:
        if not plan.consumes and plan.name != decision.complete:
            setup.append(
                "        let handle = self.state.issued_handle();"
                if plan.receiver_access == "issued"
                else "        let handle = self.state.native_for_call().map_err(map_error)?;"
            )
    elif scoped:
        setup.append(
            f"        let handle = self.scope.pointer(self.native)? as *mut sys::{native_owner};"
        )
    elif plan.consumes or abandon:
        closed = "completed_python_future(py)" if plan.completion else "Ok(py.None())"
        setup += [
            f"        let Some({'mut ' if plan.consumes else ''}reservation) = self.reserve()? else {{ return {closed}; }};",
            "        let mut handle = reservation.handle();"
            if receiver_pointer
            else "        let handle = reservation.handle();",
        ]
    elif (
        receiver
        and not plan.completion
        and any(borrows_memory(output.value) for output in plan.outputs)
    ):
        setup += [
            "        let read = self.read()?;",
            "        let handle = read.handle;",
        ]
    elif receiver:
        setup += [
            "        let handle = self.live()?;",
        ]
    if receiver:
        arguments[receiver.name] = (
            "&mut handle" if receiver_pointer and not scoped else "handle"
        )
    outputs = []
    product = None
    for output in plan.outputs:
        value = (
            output.value.element if output.value.kind == "reference" else output.value
        )
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
    if plan.completion:
        arguments[plan.completion.parameter] = "completion"
        if plan.execution == "command":
            native_result = public_result = "Future[CommandCompletion]"
            helper, converter = "command", None
        else:
            native_result, public_result, converter, _ = result_converter(plan, values)
            native_result, public_result = (
                f"Future[{native_result}]",
                f"Future[{public_result}]",
            )
            helper = "complete"
        discard = None
        if (
            plan.result
            and plan.result.kind == "handle"
            and plan.result.ownership == "owned"
        ):
            helper = "complete_owned"
            values.disposed.add(plan.result.native)
            discard = f"|result| {{ if !result.value.is_null() && result.value_count == 1 {{ unsafe {{ generated_dispose_{plan.result.native}(result.value.cast::<sys::{plan.result.native}>().read()); }} }} }}"
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
            *(
                [
                    f"        let discard: unsafe fn(&sys::mln_completion_result) = {discard};"
                ]
                if discard
                else []
            ),
            f"        unsafe {{ call.{helper}(|completion, diagnostic| {call}"
            + (", convert" if converter else "")
            + (", discard" if discard else "")
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
            body.append('        result.set_item("completion", future)?;')
            body.append("        Ok(result.into_any().unbind())")
            product_name = public_name(function.name) + "Result"
            product_fields = [
                f"    {output_member(output)}: {OWNERS[value.native]}"
                for output, value in outputs
            ]
            product_fields.append(f"    completion: {public_result}")
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
            public_copies.append('completion=raw["completion"]')
            expression = f"{product_name}(" + ", ".join(public_copies) + ")"
            public_result = product_name
            native_result = "dict[str, Any]"
        elif plan.result:
            if plan.result.kind == "handle":
                result_owner = OWNERS[plan.result.native]
                expression = f"_adopt_future({expression}, {result_owner!r}, {'self' if plan.result.handle.parent else 'None'})"
            else:
                converted = values.facade_copy(plan.result, "value")
                if converted != "value":
                    expression = f"map_future({expression}, lambda value: {converted})"
    else:
        call = native_call(function, [arguments[p.name] for p in function.parameters])
        if decision and plan.name == decision.handle.release:
            body = [line for line in setup if "let storage =" not in line] + [
                # Native release retires the cancel registration and its root.
                "        unsafe { call.run(|| self.state.close()) };",
                "        Ok(py.None())",
            ]
            rust = (
                "    fn close(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {\n"
                + "\n".join(body)
                + "\n    }\n"
            )
            return (
                owner,
                rust,
                "    def close(self) -> None:\n        return self._native.close()\n",
                "    def close(self) -> None: ...\n",
                None,
            )
        status = plan.status
        body = setup + [
            f"        let result = unsafe {{ call.status(|diagnostic| {call}) }};"
            if status
            else f"        let result = unsafe {{ call.run(|| {call}) }};"
        ]
        if decision and plan.name == decision.complete:
            body[-1] = (
                f"        let result = unsafe {{ call.run(|| self.state.complete_with(|handle| maplibre_core::check(|diagnostic| {call}))) }}.map_err(map_error);"
            )
        if status:
            if plan.consumes == "always":
                body.append("        reservation.commit();")
            body.append("        result?;")
            if abandon:
                state = "self.state()"
                body.append(f"        {state}.views_valid = false;")
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
        if not outputs:
            body.append("        Ok(py.None())")
        elif len(outputs) == 1:
            output, value = outputs[0]
            if value.kind == "handle" and value.ownership == "owned":
                result_owner = OWNERS[value.native]
                body.append(
                    f"        {owned_native(value, output, bool(plan.registrations))}"
                )
                public_result = result_owner
                expression = f"_adopt_value({expression}, {result_owner!r}, {'self' if receiver and value.handle and value.handle.parent else 'None'})"
            else:
                body.append(f"        {ok(values.copy(value, output))}")
                public_result = values.type(value)
                expression = values.facade_copy(value, expression)
        else:
            body.append("        let dict = PyDict::new(py);")
            for output, value in outputs:
                body.append(
                    f'        dict.set_item("{output_member(output)}", {values.copy(value, output)})?;'
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
            # Submission validates acceptance before transferring native roots.
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
        body.insert(accepted, "        let callback_roots = call.accept_callbacks();")
        if not outputs:
            if not receiver:
                raise unsupported(
                    function, "global registration requires a process root"
                )
            owner_state = "self.state()"
            body.insert(
                accepted + 1,
                f"        {owner_state}.retain_callback_roots(callback_roots);",
            )
    if not any("storage" in line for line in body[2:]):
        body = [line for line in body if "let storage =" not in line]
    body = compact(body)
    rust_arguments = ", ".join(["&self", "py: Python<'_>", *rust_parameters])
    rust = f"""    #[pyo3(signature = ({", ".join(native_signature)}))]
    fn {name}({rust_arguments}) -> PyResult<Py<PyAny>> {{
{chr(10).join(body)}
    }}
"""
    if plan.view:
        if len(outputs) != 1:
            raise unsupported(function, "borrowed view needs one typed output")
        invoke = f"self._native.{name}({', '.join(python_arguments)})"
        converted = values.facade_copy(outputs[0][1], "raw")
        expression = (
            f"_with_view(self, lambda: {invoke}, lambda raw: {converted}, callback)"
        )
        py_parameters.append(f"callback: Callable[[{public_result}], R]")
        public_result = "R"
    native_stub_parameters = [
        p for p in py_parameters if not (plan.view and p.startswith("callback:"))
    ]
    signature = ", ".join(["self", *py_parameters])
    facade_setup = (
        f"        raw = self._native.{name}({', '.join(python_arguments)})\n"
        if product
        else ""
    )
    documentation = f"Call {function.name}."
    if plan.view:
        documentation += " Native resources remain valid only during the callback."
    facade = f'''    def {name}({signature}) -> {public_result}:
        """{documentation}"""
{facade_setup}        return {expression}
'''
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
    read_scope = (
        f"|py, owner| GeneratedReadScope::with_native::<sys::{native}, _>(py, Arc::clone(&owner.state), sys::{handle.view_begin}, sys::{handle.view_end})"
        if handle.view_begin
        else f"|_, owner| GeneratedReadScope::new::<sys::{native}, _>(Arc::clone(&owner.state))"
    )
    dispose = (
        f"Some(generated_dispose_{native})"
        if handle.dispose and handle.dispose not in RUNTIME_EXPORTS
        else "None"
    )
    exit_release = "None"
    if handle.observable_root_release:
        # Interpreter shutdown starts this release and waits for its completion.
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
        + f'generated_owner!({owner}, "_{owner}", {native}, {dispose}, {exit_release}, {read_scope});\n'
    )


def decision_owner(owner: str, handle: HandlePlan) -> str:
    """Emit the PyO3 class for a handle issued by a callback decision protocol."""
    return f"""
#[pyclass(name = "_{owner}")]
struct {owner} {{
    // The accepted cancel callback, which native owns until it retires.
    cancel_root: Mutex<std::sync::Weak<GeneratedCallbackRoot>>,
    // Dropped by hand, with the GIL released; see the Drop impl below.
    state: ManuallyDrop<Arc<maplibre_core::decision::DecisionHandleState<sys::{handle.native}>>>,
}}
impl Drop for {owner} {{
    fn drop(&mut self) {{
        // SAFETY: drop runs once, and nothing reads the field after this take.
        let state = unsafe {{ ManuallyDrop::take(&mut self.state) }};
        // The last reference releases the native handle, and release waits for
        // a cancel callback running on a MapLibre thread. That callback needs
        // the GIL this thread holds while collecting the Python owner, so the
        // release runs detached.
        generated_finalize(move || drop(state));
    }}
}}
#[pymethods]
impl {owner} {{
    #[getter]
    fn closed(&self) -> bool {{ self.state.native_for_call().is_err() }}
    #[getter]
    fn id(&self) -> u64 {{ maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle()) }}
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {{
        let root = self.cancel_root.lock().unwrap_or_else(|p| p.into_inner()).upgrade();
        if let Some(root) = root {{
            for callback in root.lock().unwrap_or_else(|p| p.into_inner()).iter() {{
                visit.call(callback)?;
            }}
        }}
        Ok(())
    }}
    fn __clear__(&self) {{
        let root = self.cancel_root.lock().unwrap_or_else(|p| p.into_inner()).upgrade();
        if let Some(root) = root {{
            let callbacks = std::mem::take(&mut *root.lock().unwrap_or_else(|p| p.into_inner()));
            drop(callbacks);
        }}
    }}
}}
"""


def lower(api: Api | BoundApi) -> tuple[dict[str, str], list[str], dict[str, str]]:
    rust, facade, stubs = defaultdict(list), defaultdict(list), defaultdict(list)
    generated, errors, records = [], {}, set()
    bound = compile_api(api)
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
    for plan in bound.operations:
        function = plan.function
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
            f"#[pyfunction]\nfn {name}(py: Python<'_>) -> PyResult<Py<PyAny>> {{ generated_check_reentry()?; let value = unsafe {{ sys::{plan.name}() }}; {ok(values.copy(plan.result, 'value'))} }}\n"
        )
    files = {}
    rust_values, public_values = values.sources()
    rust_values += "\n".join(default_native)
    for handle in bound.handles.values():
        if not handle.dispose or handle.dispose in bound.source.runtime_exports:
            continue
        state_owned = handle.native in OWNERS and not owned_decision(
            bound, handle.native
        )
        if not state_owned and handle.native not in values.disposed:
            continue
        support = bound.operations_by_name.get(handle.dispose)
        if support and isinstance(support.support, DisposeSupport):
            errors.pop(handle.dispose, None)
        disposer = bound.source.functions_by_name[handle.dispose]
        # Finalization reports only whether disposal succeeded.
        call = f"unsafe {{ {native_call(disposer, ['handle'], diagnostic='std::ptr::null_mut()')} }}"
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
        files["src/generated_operations.rs"] += (
            decision_owner(owner, bound.handles[native_owner])
            + decision_table(bound, bound.decisions[native_owner], owner)
            if owned_decision(bound, native_owner)
            else state_owner(owner, bound.handles[native_owner], bound)
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
        f"# {notice}\nfrom concurrent.futures import Future\nfrom typing import Any, Callable\nfrom ._completion import CommandCompletion\nfrom ._generated_values import *\n\n"
        + "\n".join(
            f"class _{owner}:\n    closed: bool\n    id: int\n{''.join(methods)}"
            for owner, methods in sorted(stubs.items())
            if owner
        )
    )
    files["python/maplibre_native_ffi/_native.pyi"] += "\n".join(stubs.get("", []))
    files["python/maplibre_native_ffi/_native.pyi"] += "\n" + "\n".join(
        f"def {name}() -> Any: ..." for name in default_names
    )
    for owner in OWNERS.values():
        files["python/maplibre_native_ffi/_native.pyi"] = files[
            "python/maplibre_native_ffi/_native.pyi"
        ].replace(f": {owner}", f": _{owner}")
    imports = "\n".join(
        f"from ._generated_values import {public_name(record)}"
        for record in sorted(values.records | values.enums)
        if not values.records.get(record, values.enums.get(record)).response
    )
    files["python/maplibre_native_ffi/_generated_operations.py"] = (
        f'"""{notice}"""\n\nfrom __future__ import annotations\n\nfrom concurrent.futures import Future\nfrom dataclasses import dataclass\nfrom typing import TYPE_CHECKING, Any, Callable, NamedTuple, TypeVar\nfrom . import _native\nfrom ._completion import CommandCompletion\nfrom ._future import map_future\nfrom ._operation import GeneratedOperations, _adopt_future, _adopt_value, _with_view\nfrom ._generated_values import _maybe\nR = TypeVar("R")\n{imports}\nif TYPE_CHECKING:\n    from ._generated_owners import {", ".join([*OWNERS.values(), *scope_owners.values()]) or "__name__"}\n\n'
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
    files["python/maplibre_native_ffi/_generated_values.py"] = (
        f'"""{notice}"""\n\nfrom __future__ import annotations\nfrom dataclasses import dataclass\nfrom enum import IntFlag\nfrom typing import TYPE_CHECKING, Callable\nfrom ._enum import UnknownIntEnum\n'
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
    def __init__(self):
        raise TypeError("response scopes are supplied during callbacks")

    @classmethod
    def _from_native(cls, native):
        value = cls.__new__(cls)
        value._native = native
        return value
""")
    for owner in OWNERS.values():
        owner_classes.append(f'''class {owner}(_{owner}Operations, NativeHandleMixin):
    _handle_name = "{owner}"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner
''')
    files["python/maplibre_native_ffi/_generated_owners.py"] = (
        f'"""{notice}"""\nfrom ._lifecycle import NativeHandleMixin\n'
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
    bound = compile_api(api)
    _, generated, errors = lower(bound)
    support = {
        plan.name: support_relation(plan)
        for plan in bound.operations
        if plan.support and plan.name not in errors
    } | view_support(bound, generated)
    return {"generated": generated, "support": support, "unsupported": errors}
