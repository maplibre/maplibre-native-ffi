"""Group multiple C output parameters in a single copied Kotlin result."""

from .kotlin_values import Unsupported, identifier, name, native_call


def signature(plan, values):
    result_name = name(plan.name) + "Result"
    fields = []
    for parameter in plan.outputs:
        value = (
            parameter.value.element
            if parameter.value.kind == "reference"
            else parameter.value
        )
        values.check(value)
        if value.kind not in {"scalar", "enum", "record", "buffer"}:
            raise Unsupported("output parameter requires copied value storage")
        fields.append((identifier(parameter.name.removeprefix("out_")), value))
    values.multiple[result_name] = fields
    return result_name


def operation(plan, values, platform):
    from .kotlin_ir import admission, call_arguments
    from .kotlin_ir import signature as operation_signature

    method, parameters, _inputs, _, result_type = operation_signature(plan, values)
    params = ", ".join(f"{n}: {t}" for n, t in parameters)
    if platform == "commonMain":
        return f"  public fun {method}({params}): {result_type}\n"
    args = call_arguments(plan, values, platform)
    setup, copied = [], []
    for index, (field, value) in enumerate(values.multiple[result_type]):
        var = f"out{index}"
        if value.kind in {"record", "buffer"}:
            native = value.native
            allocate = (
                f"{native}.allocate(arena)"
                if platform == "jvmMain"
                else f"arena.alloc<{native}>()"
                if platform == "nativeMain"
                else f"MaplibreNativeC.{native}()"
            )
            setup.append(f"val {var} = {allocate}")
            if any(f.role == "size" for f in value.fields):
                setup.append(
                    f"{native}.size({var}, {native}.sizeof().toInt())"
                    if platform == "jvmMain"
                    else f"{var}.size = sizeOf<{native}>().toUInt()"
                    if platform == "nativeMain"
                    else f"{var}.size({var}.sizeof())"
                )
            raw = var
        else:
            typ, layout = values.scalar(value)
            native_var = typ + "Var"
            pointers = {
                "BOOLEAN": "BoolPointer",
                "BYTE": "BytePointer",
                "SHORT": "ShortPointer",
                "INT": "IntPointer",
                "LONG": "LongPointer",
                "FLOAT": "FloatPointer",
                "DOUBLE": "DoublePointer",
            }
            allocate = (
                f"arena.allocate(ValueLayout.JAVA_{layout})"
                if platform == "jvmMain"
                else f"arena.alloc<{native_var}>()"
                if platform == "nativeMain"
                else f"{pointers[layout]}(1L)"
            )
            setup.append(f"val {var} = {allocate}")
            raw = (
                f"{var}.get(ValueLayout.JAVA_{layout}, 0)"
                if platform == "jvmMain"
                else f"{var}.value"
                if platform == "nativeMain"
                else f"{var}.get()"
            )
        args.append(var + (".ptr" if platform == "nativeMain" else ""))
        copied.append(field + " = " + values.cast_public(value, raw, platform))
    prefix = (
        "MapLibreNativeC."
        if platform == "jvmMain"
        else "MaplibreNativeC."
        if platform == "androidMain"
        else ""
    )
    setup.append(native_call(plan.function, prefix, args))
    setup.append(result_type + "(" + ", ".join(copied) + ")")
    arena = (
        "Arena.ofConfined().use { arena -> "
        if platform == "jvmMain"
        else "memScoped { val arena = this; "
        if platform == "nativeMain"
        else "PointerScope().use { arena -> "
    )
    return (
        f"  public actual fun {method}({params}): {result_type} = "
        + arena
        + admission(plan)
        + ";\n    "
        + "\n    ".join(setup)
        + "\n  }\n"
    )
