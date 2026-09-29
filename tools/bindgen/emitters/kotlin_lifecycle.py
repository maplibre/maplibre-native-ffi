"""Consume owners at the native admission boundary, retaining failed releases."""

from tools.bindgen.names import camel

from .kotlin_values import name, native_call


def operation(plan, values, platform):
    from .kotlin_ir import call_arguments, receiver_value, signature

    method, params, inputs, _result, result_type = signature(plan, values)
    family = name(receiver_value(plan).native)
    declarations = []
    for (parameter_name, parameter_type), parameter in zip(params, inputs, strict=True):
        default = ""
        value = (
            parameter.value.element
            if parameter.value.kind == "reference"
            else parameter.value
        )
        if platform == "commonMain":
            if parameter_type.endswith("?"):
                default = " = null"
            elif value.default:
                default = (
                    " = GeneratedApi."
                    + camel(value.default.removeprefix("mln_"))
                    + "()"
                )
        declarations.append(f"{parameter_name}: {parameter_type}{default}")
    parameters = ", ".join(declarations)
    result_type = "Deferred<Unit>" if plan.completion else "Unit"
    if platform == "commonMain":
        return f"  public fun {method}({parameters}): {result_type}\n"
    arguments = call_arguments(plan, values, platform)
    receiver_index = next(
        i for i, p in enumerate(plan.inputs) if p.name == plan.receiver
    )
    arguments[receiver_index] = "owner"
    setup = []
    receiver = plan.inputs[receiver_index].value
    if receiver.kind == "reference":
        alloc = (
            "arena.allocate(ValueLayout.JAVA_LONG).also { it.set(ValueLayout.JAVA_LONG, 0, owner) }"
            if platform == "jvmMain"
            else "arena.alloc<ULongVar>().also { it.value = owner }"
            if platform == "nativeMain"
            else "LongPointer(1L).put(owner)"
        )
        setup.append("val holder = " + alloc)
        arguments[receiver_index] = (
            "holder.ptr" if platform == "nativeMain" else "holder"
        )
    prefix = (
        "MapLibreNativeC."
        if platform == "jvmMain"
        else "MaplibreNativeC."
        if platform == "androidMain"
        else ""
    )
    if plan.completion:
        arguments.append("completion")
    native = native_call(plan.function, prefix, arguments)
    arena = (
        "Arena.ofConfined().use { arena -> "
        if platform == "jvmMain"
        else "memScoped { val arena = this; "
        if platform == "nativeMain"
        else "PointerScope().use { arena -> "
    )
    setup.append(native)
    call = arena + "; ".join(setup) + " }"
    if plan.completion:
        call = "CompletionBridge.unitChecked { completion -> " + call + " }"
    hook = "bindingRetire" if plan.completion else "bindingClose"
    guard = f'org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(owner.toLong(), "{plan.name}")'
    return f'  public actual fun {method}({parameters}): {result_type} {{ org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation("{plan.name}"); return {hook}{family} {{ owner -> {guard}; {call} }} }}\n'
