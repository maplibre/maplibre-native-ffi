"""Generate Python operations, values, stubs, and their PyO3 calls together."""

from __future__ import annotations

import ast
from collections import defaultdict
from dataclasses import replace
from keyword import iskeyword
from textwrap import dedent

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters.rust import RUST_KEYWORDS
from tools.bindgen.model import Api, CType, Function, ModelError, Record
from tools.bindgen.semantic import BoundApi, DecisionPlan, HandlePlan, OperationPlan

from .python_values import Values, public_name, scalar_type

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
# Callback decision protocols by the native handle type they issue. Their
# owners hold the core decision state instead of a NativeHandleState.
DECISIONS: dict[str, DecisionPlan] = {}


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
        if owner is None or result.native in DECISIONS:
            raise unsupported(
                plan.function, "owned result needs a supported handle constructor"
            )
        converter = f"""|py, result| {{
            let raw = completion_value::<sys::{result.native}>(result)?;
            let state = unsafe {{ NativeHandleState::from_handle(raw, "{result.native}") }}.map_err(map_error)?.with_disposal(generated_dispose_{result.native});
            Py::new(py, {owner} {{ state: Arc::new(Mutex::new(state)) }}).map(|value| value.into_any())
        }}"""
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
        converter = f"|py, result| {{ let value = completion_value::<{native}>(result)?; Ok({copied}) }}"
        if result.nullable:
            converter = f"|py, result| {{ if result.value.is_null() {{ return Ok(py.None()); }} let value = completion_value::<{native}>(result)?; Ok({copied}) }}"
    return "Any", public, converter, None


def borrows_memory(value):
    if value.kind in {"buffer", "array"}:
        return True
    if value.kind == "reference" and value.element:
        return borrows_memory(value.element)
    return any(borrows_memory(field.value) for field in value.fields)


def owned_native(value, expression, callbacks=False):
    roots = ".with_callback_roots(callback_roots.clone())" if callbacks else ""
    state = f'unsafe {{ NativeHandleState::from_handle({expression}, "{value.native}") }}.map_err(map_error)?.with_disposal(generated_dispose_{value.native}){roots}'
    owner = OWNERS[value.native]
    extra = ""
    return f"{owner} {{ state: Arc::new(Mutex::new({state})){extra} }}"


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
    decision = DECISIONS.get(native_owner)
    name = function.name.removeprefix(
        native_owner.removesuffix("_handle") + "_"
    ).removeprefix("mln_")
    release = bool(
        receiver
        and receiver.value.handle
        and plan.name == receiver.value.handle.release
    )
    if release:
        name = "close"
    if plan.view:
        name = "with_" + name.removeprefix("get_")
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
        else "self.state().live_handle().map(maplibre_core::handle::NativeHandle::to_raw).unwrap_or(0)"
        if receiver
        else "0"
    )
    rust_parameters, py_parameters, arguments, setup = (
        [],
        [],
        {},
        [f'        generated_check_operation("{plan.name}", {receiver_identity})?;'],
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
        if value.lifetime != "call":
            raise unsupported(
                function, f"parameter {local} requires retained input storage"
            )
        if value.kind == "handle" and value.native in OWNERS:
            public = OWNERS[value.native]
            rust = f"&{public}"
            setup.append(
                f'        let {local}_handle = {local}.state().live_handle().ok_or_else(|| invalid_state_error("input handle is closed"))?;'
            )
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
                setup.extend(
                    [
                        f"        let mut {local}_values = Vec::new();",
                        f"        for item in {local}.try_iter()? {{ let item = item?; {local}_values.push({converted}); }}",
                    ]
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
                    setup.append(
                        f"        let {local}_value = if {local}.is_none() {{ None }} else {{ Some({converted}) }};"
                    )
                    arguments[parameter.name] = (
                        f"{local}_value.as_ref().map_or(std::ptr::null(), |value| value)"
                    )
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
    setup.insert(0, "        let storage = &mut GeneratedInputStorage::default();")
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
            f"        let Some({'mut ' if plan.consumes else ''}reservation) = GeneratedHandleReservation::new(&self.state)? else {{ return {closed}; }};",
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
            "        let read = GeneratedReadReservation::new(&self.state)?;",
            "        let handle = read.handle;",
        ]
    elif receiver:
        setup += [
            '        let handle = self.state().live_handle().ok_or_else(|| invalid_state_error("handle is closed"))?;',
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
            helper, converter = "submit_python_command_future", None
        else:
            native_result, public_result, converter, _ = result_converter(plan, values)
            native_result, public_result = (
                f"Future[{native_result}]",
                f"Future[{public_result}]",
            )
            helper = "submit_python_future"
        discard = None
        if (
            plan.result
            and plan.result.kind == "handle"
            and plan.result.ownership == "owned"
        ):
            helper = "submit_python_owned_future"
            discard = f"|result| {{ if !result.value.is_null() && result.value_count == 1 {{ unsafe {{ generated_dispose_{plan.result.native}(result.value.cast::<sys::{plan.result.native}>().read()); }} }} }}"
        call_args = ", ".join(arguments[p.name] for p in function.parameters)
        body = setup + [
            f"        {helper}(py, |completion| unsafe {{ generated_native_call(py, || sys::{function.name}({call_args})) }}"
            + (f", {converter}" if converter else "")
            + (f", {discard}" if discard else "")
            + ")"
        ]
        expression = f"self._native.{name}({', '.join(python_arguments)})"
        if plan.consumes:
            body[-1] += "?;"
            body[-1] = body[-1].replace(
                f"        {helper}(", f"        let future = {helper}(", 1
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
                f"        {helper}(", f"        let future = {helper}(", 1
            )
            for output, value in outputs:
                body.append(
                    f"        let mut {output}_owner = GeneratedOwnedOutput::new({output}, generated_dispose_{value.native});"
                )
            for output, value in outputs:
                body.append(
                    f"        let {output}_python = Py::new(py, {owned_native(value, output + '_owner.take()', bool(plan.registrations))})?;"
                )
            body.append("        let result = PyDict::new(py);")
            for output, _ in outputs:
                body.append(
                    f'        result.set_item("{output.removeprefix("out_")}", {output}_python)?;'
                )
            body.append('        result.set_item("completion", future)?;')
            body.append("        Ok(result.into_any().unbind())")
            product_name = public_name(function.name) + "Result"
            product_fields = [
                f"    {output.removeprefix('out_')}: {OWNERS[value.native]}"
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
                    f"{output.removeprefix('out_')}=_adopt_value(raw[{output.removeprefix('out_')!r}], {OWNERS[value.native]!r}, {parent})"
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
        call_args = ", ".join(arguments[p.name] for p in function.parameters)
        if decision and plan.name == decision.handle.release:
            body = [line for line in setup if "let storage =" not in line] + [
                "        unsafe { generated_native_call(py, || self.state.close()) };",
                "        self.__clear__();",
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
        body = setup + [
            f"        let result = unsafe {{ generated_native_call(py, || sys::{function.name}({call_args})) }};"
        ]
        decision_complete = decision and plan.name == decision.complete
        if decision_complete:
            body[-1] = (
                f"        let result = unsafe {{ generated_native_call(py, || self.state.complete_with(|handle| maplibre_core::check(sys::{function.name}({call_args})))) }};"
            )
        if decision_complete:
            body.append("        if result.is_ok() { self.__clear__(); }")
        if ctype(function.return_type) == "mln_status":
            if plan.consumes == "always":
                body.append("        reservation.commit();")
            body.append(
                "        result.map_err(map_error)?;"
                if decision_complete
                else "        maplibre_core::check(result).map_err(map_error)?;"
            )
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
                    f"        Py::new(py, {owned_native(value, output, bool(plan.registrations))}).map(|value| value.into_any())"
                )
                public_result = result_owner
                expression = f"_adopt_value({expression}, {result_owner!r}, {'self' if receiver and value.handle and value.handle.parent else 'None'})"
            else:
                body.append(f"        Ok({values.copy(value, output)})")
                public_result = values.type(value)
                expression = values.facade_copy(value, expression)
        else:
            body.append("        let dict = PyDict::new(py);")
            for output, value in outputs:
                body.append(
                    f'        dict.set_item("{output.removeprefix("out_")}", {values.copy(value, output)})?;'
                )
            body.append("        Ok(dict.into_any().unbind())")
            product_name = public_name(function.name) + "Result"
            product = (
                "class "
                + product_name
                + "(NamedTuple):\n"
                + "\n".join(
                    f"    {output.removeprefix('out_')}: {values.type(value)}"
                    for output, value in outputs
                )
                + "\n"
            )
            expression = (
                f"{product_name}("
                + ", ".join(
                    values.facade_copy(value, f"raw[{output.removeprefix('out_')!r}]")
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
                    f"        {helper}(", f"        let future = {helper}(", 1
                )
                + "?;"
            )
            body.append("        Ok(future)")
        if plan.completion:
            accepted = (
                next(i for i, line in enumerate(body) if "let future = " in line) + 1
            )
        else:
            accepted = (
                next(
                    i
                    for i, line in enumerate(body)
                    if "maplibre_core::check(result)" in line
                )
                + 1
            )
        body.insert(
            accepted, "        let callback_roots = storage.accept_callbacks();"
        )
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
    if not any("storage" in line for line in body[1:]):
        body = [line for line in body if "let storage =" not in line]
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


def state_owner(owner: str, handle: HandlePlan) -> str:
    """Emit the PyO3 class for a handle whose ownership NativeHandleState tracks."""
    native = handle.native
    read_scope = (
        f"GeneratedReadScope::with_native::<sys::{native}, _>(py, Arc::clone(&self.state), sys::{handle.view_begin}, sys::{handle.view_end})"
        if handle.view_begin
        else f"GeneratedReadScope::new::<sys::{native}, _>(Arc::clone(&self.state))"
    )
    return f"""
#[pyclass(name = "_{owner}")]
struct {owner} {{ state: Arc<Mutex<NativeHandleState<sys::{native}>>> }}
impl {owner} {{
    fn state(&self) -> MutexGuard<'_, NativeHandleState<sys::{native}>> {{
        self.state.lock().unwrap_or_else(|p| p.into_inner())
    }}
}}
#[pymethods]
impl {owner} {{
    #[getter]
    fn closed(&self) -> bool {{ self.state().is_closed() }}
    #[getter]
    fn id(&self) -> u64 {{ self.state().issued_id() }}
    fn __traverse__(&self, visit: pyo3::gc::PyVisit<'_>) -> Result<(), pyo3::gc::PyTraverseError> {{
        self.state().traverse_callbacks(&visit)
    }}
    fn __clear__(&self) {{
        let callbacks = self.state().take_callbacks();
        drop(callbacks);
    }}
    fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {{
        let _ = py;
        generated_check_reentry()?;
        {read_scope}
    }}
}}
"""


def decision_owner(owner: str) -> str:
    """Emit the PyO3 class for a handle issued by a callback decision protocol."""
    return f"""
#[pyclass(name = "_{owner}")]
struct {owner} {{
    cancel_root: Mutex<Option<Arc<Mutex<Option<Py<PyAny>>>>>>,
    // Dropped by hand, with the GIL released; see the Drop impl below.
    state: ManuallyDrop<Arc<maplibre_core::resource::ResourceRequestHandleState>>,
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
        let root = self.cancel_root.lock().unwrap_or_else(|p| p.into_inner());
        if let Some(root) = root.as_ref() {{
            if let Some(callback) = root.lock().unwrap_or_else(|p| p.into_inner()).as_ref() {{
                visit.call(callback)?;
            }}
        }}
        Ok(())
    }}
    fn __clear__(&self) {{
        let root = self.cancel_root.lock().unwrap_or_else(|p| p.into_inner()).take();
        drop(root);
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
            name: public_name(name)
            + ("" if public_name(name).endswith("Handle") else "Handle")
            for name in bound.public_handles
        }
    )
    DECISIONS.clear()
    DECISIONS.update(
        {
            callback.decision.handle.native: callback.decision
            for callback in bound.callbacks.values()
            if callback.decision and callback.decision.handle.native in OWNERS
        }
    )
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
        )
        try:
            owner, native, public, stub, record = operation(plan, values)
        except ModelError as error:
            values.records, values.inputs, values.enums, values.outputs = saved_values
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
        if (
            plan.role != "support"
            or not (plan.support_for or "").startswith("default:")
            or plan.result is None
        ):
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
        name = "_default_" + plan.result.native.removeprefix("mln_")
        default_names.append(name)
        default_native.append(
            f"#[pyfunction]\nfn {name}(py: Python<'_>) -> PyResult<Py<PyAny>> {{ generated_check_reentry()?; let value = unsafe {{ sys::{plan.name}() }}; Ok({values.copy(plan.result, 'value')}) }}\n"
        )
    files = {}
    rust_values, public_values = values.sources()
    rust_values += "\n".join(default_native)
    for handle in bound.handles.values():
        if not handle.dispose or handle.dispose in bound.source.runtime_exports:
            continue
        if f"generated_dispose_{handle.native}" not in rust_values + "".join(
            "".join(methods) for methods in rust.values()
        ):
            continue
        disposer = bound.source.functions_by_name[handle.dispose]
        call = f"unsafe {{ sys::{handle.dispose}(handle) }}"
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
            decision_owner(owner)
            if native_owner in DECISIONS
            else state_owner(owner, bound.handles[native_owner])
        )
    scope_owners = {
        name: public_name(name) + "Scope"
        for name, value in values.records.items()
        if value.response
    }
    global_native = "\n".join(rust.get("", []))
    global_names = default_names + [
        plan.name.removeprefix("mln_")
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
        f'"""{notice}"""\n\nfrom __future__ import annotations\n\nfrom concurrent.futures import Future\nfrom dataclasses import dataclass\nfrom typing import TYPE_CHECKING, Any, Callable, NamedTuple, TypeVar\nfrom . import _native\nfrom ._completion import CommandCompletion\nfrom ._future import map_future\nfrom ._operation import GeneratedOperations, _adopt_future, _adopt_value, _with_view\nR = TypeVar("R")\n{imports}\nif TYPE_CHECKING:\n    from ._generated_owners import {", ".join([*OWNERS.values(), *scope_owners.values()]) or "__name__"}\n\n'
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
    scope_owners = {
        name: public_name(name) + "Scope"
        for name, value in values.records.items()
        if value.response
    }
    for native, owner in scope_owners.items():
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
        plan.name.removeprefix("mln_")
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
    emitted = "\n".join(files.values())
    for plan in bound.operations:
        if plan.role == "support" and f"sys::{plan.name}(" in emitted:
            errors.pop(plan.name, None)
    return files, generated, errors


def generate(api: Api | BoundApi) -> dict[str, str]:
    return lower(api)[0]


def coverage(api: Api | BoundApi) -> dict:
    _, generated, errors = lower(api)
    bound = compile_api(api)
    support = {
        plan.name: plan.support_for
        for plan in bound.operations
        if plan.role == "support" and plan.name not in errors
    }
    return {"generated": generated, "support": support, "unsupported": errors}
