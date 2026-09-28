"""Copy native array strides and per-item byte arenas before their owner releases."""

from .kotlin_values import name


def read_strided(record, field, values, platform):
    value = field.value
    element = value.element
    native = element.native
    get = lambda key: values.field(record, key, "source", platform)
    count, stride = get(value.length), get(value.stride)
    pointer = get(field.name)
    if platform == "jvmMain":
        size = f"{native}.sizeof()"
        item = f"{pointer}.reinterpret(count * stride).asSlice(index.toLong() * stride, {size})"
    elif platform == "nativeMain":
        size = f"sizeOf<{native}>()"
        item = f"({pointer}!!.reinterpret<ByteVar>() + index.toLong() * stride)!!.reinterpret<{native}>().pointed"
    else:
        size = f"MaplibreNativeC.{native}().sizeof().toLong()"
        item = f"MaplibreNativeC.{native}(org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer({pointer}.address() + index.toLong() * stride))"
    setup = f"val count = ({count}).toLong(); val stride = ({stride}).toLong(); require(count in 0..Int.MAX_VALUE.toLong()); require(stride >= {size}); "
    args = "item"
    if value.item_buffer:
        plan = value.item_buffer
        offset = values.field(element, plan.offset, "item", platform)
        length = values.field(element, plan.length, "item", platform)
        data, data_size = get(plan.data), get(plan.size)
        setup += f"val dataSize = ({data_size}).toLong(); "
        validation = f"val offset = ({offset}).toLong(); val length = ({length}).toLong(); require(offset >= 0 && length >= 0 && offset <= dataSize && length <= dataSize - offset); "
        if platform == "jvmMain":
            raw = f"{data}.reinterpret(dataSize).asSlice(offset, length)"
        elif platform == "nativeMain":
            raw = f"({data}?.reinterpret<ByteVar>()?.plus(offset))"
        else:
            raw = f"BytePointer({data}).position(offset)"
        message = (
            f"GeneratedValues.readRawBytes({raw}, length"
            + (".toULong()" if platform == "nativeMain" else "")
            + ")"
        )
        if plan.encoding == "utf8":
            message += ".decodeToString()"
        args += ", " + message
    else:
        validation = ""
    return f"run {{ {setup}List(count.toInt()) {{ index -> val item = {item}; {validation}GeneratedValues.read{name(native)}({args}) }} }}"
