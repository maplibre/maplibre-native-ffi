"""Compile Go records and native storage from resolved C value plans."""

import os
from dataclasses import replace

from ..model import ModelError
from ..semantic import ValuePlan
from .go import name, native_identifier


def public(native):
    return name(native.removeprefix("mln_"))


def field(value):
    return ".".join(native_identifier(part) for part in value.split("."))


def scalar(ctype, carrier=None):
    portable = {
        "size_t": "uint",
        "uintptr_t": "uint",
        "ptrdiff_t": "int",
        "intptr_t": "int",
        **{
            f"{sign}int{bits}_t": f"{sign}int{bits}"
            for sign in ("", "u")
            for bits in (8, 16, 32, 64)
        },
    }
    if carrier in portable:
        return portable[carrier]
    return {
        "void": "struct{}",
        "bool": "bool",
        "_Bool": "bool",
        "char": "int8",
        "signed char": "int8",
        "unsigned char": "uint8",
        "short": "int16",
        "unsigned short": "uint16",
        "int": "int32",
        "unsigned int": "uint32",
        "long": "int",
        "unsigned long": "uint",
        "long long": "int64",
        "unsigned long long": "uint64",
        "float": "float32",
        "double": "float64",
    }[carrier or ctype.canonical]


def absent(value):
    return value.nullable or value.optional == "empty"


class Values:
    def __init__(self, api):
        self.api = api
        self.records = {}
        self.enums = {}
        self.inputs = set()
        self.outputs = set()
        self.unions = {}
        self.arenas = {}
        self.views = {}

    def fail(self, value, reason):
        raise ModelError([f"Go: {value.native}: {reason}"])

    def require(self, value, input=False):
        if (
            input
            and value.kind in {"buffer", "array", "reference"}
            and value.lifetime != "call"
        ):
            self.fail(value, "retained input requires a lifetime adapter")
        if value.response:
            self.records[value.native] = value
            return
        if value.registration:
            from .go_callbacks import validate

            self.records[value.native] = value
            self.inputs.add(value.native)
            self.outputs.add(value.native)
            validate(self, value)
            return
        if value.kind == "scalar":
            scalar(value.ctype, value.scalar_carrier)
        elif value.kind == "enum":
            self.enums[value.native] = value
        elif value.kind == "native_pointer":
            pass
        elif value.kind == "buffer":
            if value.encoding not in {"utf8", "bytes", "json"}:
                self.fail(value, "buffer requires encoding")
        elif value.kind in {"reference", "array"} and value.element:
            if value.item_buffer:
                self.arenas[value.element.native] = value.item_buffer
            self.require(value.element, input)
        elif value.kind == "record":
            members = list(self.members(value))
            names = [member for member, _, _, _ in members]
            if len(names) != len(set(names)):
                self.fail(value, "public field names collide")
            self.records[value.native] = value
            (self.inputs if input else self.outputs).add(value.native)
            for f in value.fields:
                if f.role not in {
                    "size",
                    "count",
                    "stride",
                    "arena",
                    "reserved",
                    "presence_mask",
                }:
                    self.require(f.value, input)
            if input and value.default and value.native not in self.outputs:
                self.require(value, input=False)
            for group in value.presence_groups:
                if len(group.fields) > 1:
                    self.require(self.group(value, group), input)
        elif value.kind == "union":
            if not value.tag:
                self.fail(value, "union requires a discriminator")
            self.unions[value.native] = value
            for f in value.fields:
                self.require(f.value, input)
        elif value.kind == "handle" and value.handle:
            pass
        else:
            self.fail(value, f"unsupported value kind {value.kind}")

    def type(self, value, optional=True):
        if value.response:
            return "*" + public(value.native) + "Scope"
        if value.kind == "scalar":
            result = scalar(value.ctype, value.scalar_carrier)
        elif value.kind == "union":
            result = self.union_name(value)
        elif value.kind in {"record", "enum"}:
            result = public(value.native)
        elif value.kind == "handle":
            result = "*" + self.owner(value.native)
        elif value.kind == "native_pointer":
            result = "uintptr"
        elif value.kind == "buffer":
            result = "string" if value.encoding == "utf8" else "[]byte"
        elif value.kind == "array":
            result = "[]" + self.type(value.element)
        elif value.kind == "reference":
            result = self.type(value.element)
        else:
            self.fail(value, "missing Go type")
        if (
            optional
            and absent(value)
            and value.kind not in {"array", "handle", "union"}
        ):
            result = "*" + result
        return result

    def union_name(self, value):
        parent, member = next(
            (p, f)
            for p in self.records.values()
            for f in p.fields
            if f.value.native == value.native
        )
        return public(parent.native) + name(member.name)

    def owner(self, native):
        result = public(native)
        return result if result.endswith("Handle") else result + "Handle"

    def group(self, value, group):
        if group.type:
            return self.api.values[group.type]
        prefix = os.path.commonprefix(group.fields).rsplit("_", 1)[0] + "_"
        fields = tuple(
            replace(
                next(f for f in value.fields if f.name == n),
                name=n.removeprefix(prefix),
                presence=None,
            )
            for n in group.fields
        )
        return ValuePlan(
            kind="record",
            native=value.native + "_" + group.mask.removeprefix("has_"),
            ctype=value.ctype,
            fields=fields,
        )

    def members(self, value):
        groups = {
            n: g for g in value.presence_groups if len(g.fields) > 1 for n in g.fields
        }
        for f in value.fields:
            if f.role in {
                "size",
                "count",
                "stride",
                "arena",
                "reserved",
                "presence_mask",
                "tag",
            }:
                continue
            arena = self.arenas.get(value.native)
            if arena and f.name in {arena.offset, arena.length}:
                continue
            if f.name in groups:
                group = groups[f.name]
                if group.fields[0] != f.name:
                    continue
                bits = [g.bit for g in value.presence_groups if g.bit]
                prefix = os.path.commonprefix(bits).rsplit("_", 1)[0] + "_"
                member = name(
                    group.bit.removeprefix(prefix).lower()
                    if group.bit
                    else group.mask.removeprefix("has_")
                )
                yield member, replace(self.group(value, group), nullable=True), f, group
            else:
                yield (
                    name(f.name),
                    replace(f.value, nullable=True)
                    if f.presence and f.presence.mask
                    else f.value,
                    f,
                    None,
                )
        for flag in value.mask_flags:
            bits = [g.bit for g in value.presence_groups if g.bit] + [
                f.name for f in value.mask_flags
            ]
            prefix = os.path.commonprefix(bits).rsplit("_", 1)[0] + "_"
            yield name(flag.name.removeprefix(prefix).lower()), None, flag, None

    def copy(self, value, expr, scope="raw"):
        if expr.startswith("*"):
            expr = f"({expr})"
        if value.kind in {"scalar", "enum"}:
            return f"{self.type(value)}({expr})"
        if value.kind == "native_pointer":
            return f"uintptr(unsafe.Pointer({expr}))"
        if value.kind == "record":
            return f"copy{public(value.native)}({expr})"
        if value.kind == "reference":
            converted = self.copy(value.element, "*" + expr, scope)
            if value.nullable:
                return f"func() {self.type(value)} {{ if {expr} == nil {{ return nil }}; value := {converted}; return &value }}()"
            return converted
        if value.kind == "buffer":
            if value.length == "nul":
                copy = f"C.GoString({expr})"
                return (
                    f"func() *string {{ if {expr} == nil {{ return nil }}; value := {copy}; {'if value == "" { return nil }; ' if value.optional == 'empty' else ''}return &value }}()"
                    if absent(value)
                    else copy
                )
            if value.ctype.pointee:
                size = (
                    value.length
                    if value.length.isdigit()
                    else f"{scope}.{field(value.length)}"
                )
                pointer = f"unsafe.Pointer({expr})"
            else:
                pointer, size = f"{expr}.data", f"{expr}.size"
            copy = f"bindingBytes({pointer}, uint64({size}))"
            if value.encoding == "utf8":
                copy = f"bindingString({pointer}, uint64({size}))"
            condition = (
                f"{size} == 0" if value.optional == "empty" else f"{pointer} == nil"
            )
            return (
                f"func() {self.type(value)} {{ if {condition} {{ return nil }}; value := {copy}; return &value }}()"
                if absent(value)
                else copy
            )
        if value.kind == "array":
            count = (
                value.length
                if value.length.isdigit()
                else f"{scope}.{field(value.length)}"
            )
            output = self.type(value)
            elem = value.element
            ctype = elem.native
            source = f"&{expr}[0]" if value.ctype.kind == "array" else expr
            stride = (
                f"{scope}.{field(value.stride)}"
                if value.stride
                else f"unsafe.Sizeof(C.{ctype}{{}})"
                if elem.kind == "record"
                else f"unsafe.Sizeof(*{source})"
            )
            copy = self.copy(elem, "item")
            enrichment = ""
            if value.item_buffer:
                arena = value.item_buffer
                enrichment = f"result[i].{name(arena.field)} = bindingArenaString(unsafe.Pointer({scope}.{field(arena.data)}), uint64({scope}.{field(arena.size)}), uint64(item.{field(arena.offset)}), uint64(item.{field(arena.length)}));"
            return f"func() {output} {{ {f'if {source} == nil {{ return nil }}; ' if value.nullable else ''}length := bindingLength(uint64({count})); result := make({output}, length); for i := range result {{ item := *(*C.{ctype})(bindingElement(unsafe.Pointer({source}), i, uint64({stride}), unsafe.Sizeof(*{source}), unsafe.Alignof(*{source}))); result[i] = {copy}; {enrichment} }}; return result }}()"
        if value.kind == "union":
            tag = f"{scope}.{field(value.tag)}"
            arms = []
            for f in value.fields:
                read = f"*(*C.{f.value.native})(unsafe.Pointer(&{expr}))"
                arms.append(
                    f"case C.{f.presence.variant}: return {self.union_name(value)}{name(f.name)}Variant{{Value: {self.copy(f.value, read)}}}"
                )
            if value.empty_variant:
                arms.append(f"case C.{value.empty_variant[0]}: return nil")
            return f"func() {self.type(value)} {{ switch {tag} {{ {'; '.join(arms)} }}; return UnknownVariant{{Tag: uint32({tag})}} }}()"
        self.fail(value, "missing copied value conversion")

    def native(self, value, expr, dest, scope="raw"):
        if value.kind == "buffer" and absent(value):
            converted = self.native(
                replace(value, nullable=False, optional=None), f"(*{expr})", dest, scope
            )
            if value.nullable and value.length != "nul":
                pointer = dest if value.ctype.pointee else dest + ".data"
                assignment = (
                    f"({self.c_type(value)})(arena.allocate(1))"
                    if value.ctype.pointee
                    else "arena.allocate(1)"
                )
                converted += f"; if {pointer} == nil {{ {pointer} = {assignment} }}"
            return f"if {expr} != nil {{ {converted} }}"
        if value.kind in {"scalar", "enum"}:
            return f"{dest} = {self.c_type(value)}({expr})"
        if value.kind == "native_pointer":
            return f"{dest} = ({self.c_type(value)})(C.binding_address(C.uintptr_t({expr})))"
        if value.kind == "record":
            return f"{dest} = native{public(value.native)}({expr}, arena)"
        if value.kind == "reference":
            inner = self.native(
                value.element, "*" + expr if value.nullable else expr, "*pointer"
            )
            alloc = f"pointer := (*C.{value.element.native})(arena.allocate(unsafe.Sizeof(*{dest}))); {inner}; {dest} = pointer"
            return (
                f"if {expr} != nil {{ {alloc} }}"
                if value.nullable
                else "{ " + alloc + " }"
            )
        if value.kind == "buffer":
            data = f"[]byte({expr})" if value.encoding == "utf8" else expr
            inner = f"arena.bytes({data})"
            if value.length == "nul":
                inner = f"arena.cstring({expr})"
            elif not value.ctype.pointee:
                inner = (
                    f"C.mln_buffer_view{{data: {inner}, size: C.size_t(len({expr}))}}"
                )
            else:
                inner = f"({self.c_type(value)})({inner})"
            code = f"{dest} = {inner}"
            if value.length and value.length != "nul" and not value.length.isdigit():
                code += f"; {scope}.{field(value.length)} = bindingCountLike({scope}.{field(value.length)}, len({expr}))"
            return code
        if value.kind == "array":
            ctype = value.element.native
            count = f"len({expr})"
            pointer = f"(*C.{ctype})(arena.array({count}, unsafe.Sizeof(*{dest})))"
            if value.ctype.kind == "array":
                setup = f'if len({expr}) != {value.length} {{ arena.fail("wrong fixed array length") }}'
                slot = f"{dest}[i]"
            else:
                setup = f"{dest} = {pointer}"
                if value.nullable:
                    setup += f"; if {expr} != nil && {dest} == nil {{ {dest} = (*C.{ctype})(arena.allocate(1)) }}"
                setup += f"; items := unsafe.Slice({dest}, len({expr}))"
                slot = "items[i]"
                if value.length and not value.length.isdigit():
                    setup += f"; {scope}.{field(value.length)} = bindingCountLike({scope}.{field(value.length)}, len({expr}))"
            return (
                "{ "
                + setup
                + f"; for i, item := range {expr} {{ {self.native(value.element, 'item', slot)} }}"
                + " }"
            )
        self.fail(value, "missing native input conversion")

    def converter(self, value):
        """A function value that converts one binding value and the arena to
        native, for the kinds whose conversion writes nothing else."""
        if value.kind == "record":
            return f"native{public(value.native)}"
        if value.kind in {"scalar", "enum"}:
            go = self.type(value, optional=False)
            helper = "bindingBool" if go == "bool" else "bindingNumber"
            return f"{helper}[{go}, {self.c_type(value)}]"
        return None

    def c_type(self, value):
        if value.ctype.pointee:
            pointee = value.ctype.pointee
            return (
                "unsafe.Pointer"
                if pointee.canonical == "void"
                else "*C." + pointee.spelling.removeprefix("const ")
            )
        return "C." + (
            value.ctype.spelling.removeprefix("const ")
            if value.kind in {"scalar", "enum"}
            else value.native
        )

    def sources(self):
        chunks = []
        for native, value in sorted(self.enums.items()):
            typename = public(native)
            prefix = (
                os.path.commonprefix([key for key, _ in value.enum_values]).rsplit(
                    "_", 1
                )[0]
                + "_"
            )
            constants = "\n".join(
                f"{typename}{name(key.removeprefix(prefix).lower())} {typename} = {typename}(C.{key})"
                for key, number in value.enum_values
            )
            chunks.append(
                f"type {typename} {scalar(value.enum_underlying or value.ctype, value.scalar_carrier)}\nconst (\n{constants}\n)\n"
            )
            if value.enum_kind == "bitmask":
                chunks.append(
                    f"func (value {typename}) Has(flags {typename}) bool {{ return value & flags == flags }}"
                )
        for native, value in sorted(self.unions.items()):
            typename = self.union_name(value)
            chunks.append(f"type {typename} interface {{ bindingTag() uint32 }}")
            for f in value.fields:
                wrapper = typename + name(f.name) + "Variant"
                chunks.append(
                    f"type {wrapper} struct {{ Value {self.type(f.value)} }}\nfunc ({wrapper}) bindingTag() uint32 {{ return uint32(C.{f.presence.variant}) }}"
                )
        for native, value in sorted(self.records.items()):
            typename = public(native)
            if value.response:
                chunks.append(
                    f"type {typename}Scope struct {{ native *C.{native}; scope *bindingScope }}\n"
                    f"func (response *{typename}Scope) target(operation uint32) bindingTarget {{ if response == nil {{ return bindingScoped(nil, 0, operation) }}; return bindingScoped(response.scope, uint64(uintptr(unsafe.Pointer(response.native))), operation) }}"
                )
                continue
            from .go_callbacks import plain, registration_input, signature

            visible = plain(value) if value.registration else value
            members = list(self.members(visible))
            fields = [
                f"{member} {self.type(v) if v else 'bool'}"
                for member, v, _, _ in members
            ]
            if value.registration:
                for member in value.registration.callbacks:
                    callback = self.api.callbacks[
                        next(f.value.native for f in value.fields if f.name == member)
                    ]
                    fields.append(f"{name(member)} {signature(self, callback)}")
            if arena := self.arenas.get(native):
                fields.append(f"{name(arena.field)} string")
            chunks.append(f"type {typename} struct {{ {'; '.join(fields)} }}")
            if native not in self.api.values:
                continue
            if native in self.outputs or value.default:
                lines = []
                for member, v, f, group in members:
                    if v is None:
                        lines.append(
                            f"result.{member} = raw.{field(f.mask)} & C.{f.name} != 0"
                        )
                        continue
                    if group:
                        inner = self.group(value, group)
                        body = "; ".join(
                            f"inner.{name(child.name)} = {self.copy(child.value, 'raw.' + field(source))}"
                            for child, source in zip(
                                inner.fields, group.fields, strict=True
                            )
                        )
                        convert = f"func() {public(inner.native)} {{ var inner {public(inner.native)}; {body}; return inner }}()"
                    else:
                        convert = self.copy(f.value, "raw." + field(f.name))
                    if f.presence and f.presence.mask:
                        condition = (
                            f"raw.{field(f.presence.mask)} & C.{f.presence.bit} != 0"
                            if f.presence.bit
                            else f"bool(raw.{field(f.presence.mask)})"
                        )
                        if v.kind == "array":
                            lines.append(
                                f"if {condition} {{ result.{member} = {convert} }}"
                            )
                        else:
                            copied = (
                                public(inner.native)
                                if group
                                else self.type(replace(v, nullable=False))
                            )
                            lines.append(
                                f"result.{member} = bindingPresent({condition}, func() {copied} {{ return {convert} }})"
                            )
                    else:
                        lines.append(f"result.{member} = {convert}")
                chunks.append(
                    f"func copy{typename}(raw C.{native}) {typename} {{ var result {typename}; {'; '.join(lines)}; return result }}"
                )
            if native in self.inputs:
                init = f"C.{value.default}()" if value.default else f"C.{native}{{}}"
                lines = [f"raw := {init}"]
                for f in visible.fields:
                    if f.role == "size":
                        lines.append(
                            f"raw.{field(f.name)} = bindingCountLike(raw.{field(f.name)}, int(unsafe.Sizeof(raw)))"
                        )
                    elif f.role == "presence_mask":
                        lines.append(
                            f"raw.{field(f.name)} = {'false' if f.value.ctype.canonical in {'bool', '_Bool'} else '0'}"
                        )
                for member, v, f, group in members:
                    expr = "input." + member
                    if v is None:
                        lines.append(
                            f"if {expr} {{ raw.{field(f.mask)} |= C.{f.name} }}"
                        )
                        continue
                    if v.kind == "union":
                        arms = []
                        for variant in v.fields:
                            write = f"*(*C.{variant.value.native})(unsafe.Pointer(&raw.{field(f.name)}))"
                            arms.append(
                                f"case {self.union_name(v)}{name(variant.name)}Variant: {self.native(variant.value, 'variant.Value', write)}"
                            )
                        tag_type = next(
                            f.value for f in value.fields if f.name == v.tag
                        )
                        tag = f"raw.{field(v.tag)} = {self.c_type(tag_type)}({expr}.bindingTag())"
                        body = f'if {expr} == nil {{ arena.fail("missing union variant") }}; {tag}; switch variant := {expr}.(type) {{ {"; ".join(arms)}; default: arena.fail("unknown input union variant") }}'
                        if v.empty_variant:
                            body = f"if {expr} == nil {{ raw.{field(v.tag)} = C.{v.empty_variant[0]} }} else {{ {body} }}"
                        lines.append(body)
                        continue
                    if group:
                        inner = self.group(value, group)
                        body = "; ".join(
                            self.native(
                                child.value,
                                expr + "." + name(child.name),
                                "raw." + field(dest),
                            )
                            for child, dest in zip(
                                inner.fields, group.fields, strict=True
                            )
                        )
                    else:
                        body = self.native(
                            f.value,
                            "(*" + expr + ")"
                            if f.presence and f.presence.mask and v.kind != "array"
                            else expr,
                            "raw." + field(f.name),
                        )
                    if f.presence and f.presence.mask:
                        mask = (
                            f"raw.{field(f.presence.mask)} |= C.{f.presence.bit}"
                            if f.presence.bit
                            else f"raw.{field(f.presence.mask)} = true"
                        )
                        convert = None if group else self.converter(f.value)
                        if convert and f.presence.bit:
                            lines.append(
                                f"bindingMasked(&raw.{field(f.presence.mask)}, C.{f.presence.bit}, &raw.{field(f.name)}, {expr}, arena, {convert})"
                            )
                        elif convert:
                            lines.append(
                                f"bindingFlagged(&raw.{field(f.presence.mask)}, &raw.{field(f.name)}, {expr}, arena, {convert})"
                            )
                        else:
                            lines.append(f"if {expr} != nil {{ {body}; {mask} }}")
                    else:
                        lines.append(body)
                if value.registration:
                    lines.append(registration_input(self, value))
                chunks.append(
                    f"func native{typename}(input {typename}, arena *bindingArena) C.{native} {{ {'; '.join(lines)}; return raw }}"
                )
            if value.default:
                chunks.append(
                    f"func Default{typename}() {typename} {{ return copy{typename}(C.{value.default}()) }}"
                )
        for native, value in self.views.items():
            typename = public(native) + "View"
            chunks.append(
                f"type {typename} struct {{ value {public(native)}; scope *bindingScope }}"
            )
            for member, v, _, _ in self.members(value):
                result = self.type(v) if v else "bool"
                method = (
                    "Unsafe" + member
                    if v and v.kind in {"native_pointer", "union"}
                    else member
                )
                chunks.append(
                    f"func (view {typename}) {method}() ({result},error) {{ return bindingCall(func() {result} {{ runtime.LockOSThread(); defer runtime.UnlockOSThread(); view.scope.check(); return view.value.{member} }}) }}"
                )
        return "\n\n".join(chunks)
