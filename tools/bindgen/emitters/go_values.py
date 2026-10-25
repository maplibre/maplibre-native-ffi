"""Compile Go records and native storage from resolved C value plans."""

from dataclasses import replace

from ..model import ModelError
from ..semantic import enum_member_prefix
from .go import doc, name, native_identifier


def is_text(value):
    """Whether a buffer is a Go string: UTF-8 text and JSON."""
    return value.encoding in {"utf8", "json"}


def public(native):
    return name(native.removeprefix("mln_"))


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


class Values:
    def __init__(self, api):
        self.api = api
        self.records = {}
        self.enums = {}
        self.inputs = set()
        self.unions = {}
        self.views = {}

    def fail(self, value, reason):
        raise ModelError([f"Go: {value.native}: {reason}"])

    def slice(self, value):
        """The array field of a `kind=slice` record, which Go presents as a
        slice in place of the record."""
        return self.api.slices.get(value.native) if value.kind == "record" else None

    def require(self, value, input=False):
        if array := self.slice(value):
            self.require(array.value, input)
            return
        if value.response:
            self.records[value.native] = value
            return
        if value.registration:
            from .go_callbacks import validate

            self.records[value.native] = value
            self.inputs.add(value.native)
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
            self.require(value.element, input)
        elif value.kind == "record":
            members = list(self.members(value))
            names = [member for member, _, _ in members]
            if len(names) != len(set(names)):
                self.fail(value, "public field names collide")
            self.records[value.native] = value
            if input:
                self.inputs.add(value.native)
            for f in value.fields:
                if f.public:
                    self.require(f.value, input)
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
        if array := self.slice(value):
            return self.type(array.value, optional)
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
            result = "string" if is_text(value) else "[]byte"
        elif value.kind == "array":
            result = "[]" + self.type(value.element)
        elif value.kind == "reference":
            result = self.type(value.element)
        else:
            self.fail(value, "missing Go type")
        if (
            optional
            and value.nullable
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
        # A record that holds only a union presents the union in its place.
        if parent.union_member:
            return public(parent.native)
        return public(parent.native) + name(member.name)

    def unknown(self, value):
        """Whether a union keeps tags this binding predates as UnknownVariant:
        only one that native returns does."""
        return value.native in self.api.returned

    def union_input(self, union, expr, dest, tag):
        """Writes the variant `expr` of `union` into the storage `dest` and
        its tag into `tag`."""
        arms = []
        for variant in union.fields:
            write = f"*(*C.{variant.value.native})(unsafe.Pointer(&{dest}))"
            arms.append(
                f"case {self.union_name(union)}{name(variant.name)}Variant: {self.native(variant.value, 'variant.Value', write)}"
            )
        # A pointer to a variant, or a type embedding one, also satisfies the
        # sealed interface, so every union rejects what the switch doesn't name.
        arms.append('default: arena.fail("unknown input union variant")')
        return f'if {expr} == nil {{ arena.fail("missing union variant") }}; {tag}({expr}.bindingTag()); switch variant := {expr}.(type) {{ {"; ".join(arms)} }}'

    def tag_write(self, record, union):
        """The start of the assignment that writes a variant's tag."""
        tag = next(f.value for f in record.fields if f.name == union.tag)
        return f"raw.{native_identifier(union.tag)} = {self.c_type(tag)}"

    def union_record(self, value, member):
        """The copy and the native conversion of a record that holds only a
        union, which Go presents as the union's interface. Conversion writes
        the size, and the tag from the variant."""
        typename = public(value.native)
        native, union = value.native, member.value
        place = "raw." + native_identifier(member.name)
        chunks = []
        if native in self.api.returned:
            chunks.append(
                f"func copy{typename}(raw C.{native}) {typename} {{ {self.union_copy(union, place, 'raw')} }}"
            )
        if native in self.inputs and native in self.api.inputs:
            init = f"C.{value.default}()" if value.default else f"C.{native}{{}}"
            lines = [f"raw := {init}"]
            lines.extend(
                f"raw.{native_identifier(f.name)} = bindingCountLike(raw.{native_identifier(f.name)}, int(unsafe.Sizeof(raw)))"
                for f in value.fields
                if f.role == "size"
            )
            lines.append(
                self.union_input(union, "input", place, self.tag_write(value, union))
            )
            chunks.append(
                f"func native{typename}(input {typename}, arena *bindingArena) C.{native} {{ {'; '.join(lines)}; return raw }}"
            )
        return chunks

    def owner(self, native):
        return name(self.api.handles[native].stem) + "Handle"

    def members(self, value):
        for f in value.fields:
            if not f.public:
                continue
            yield (
                name(f.name),
                replace(f.value, nullable=True)
                if f.presence and f.presence.mask
                else f.value,
                f,
            )
        for flag in value.mask_flags:
            yield name(flag.member), None, flag

    def copy(self, value, expr, scope="raw"):
        if expr.startswith("*"):
            expr = f"({expr})"
        if value.kind in {"scalar", "enum"}:
            return f"{self.type(value)}({expr})"
        if value.kind == "native_pointer":
            return f"uintptr(unsafe.Pointer({expr}))"
        if array := self.slice(value):
            return self.copy(
                array.value, f"{expr}.{native_identifier(array.name)}", expr
            )
        if value.kind == "record":
            return f"copy{public(value.native)}({expr})"
        if value.kind == "reference":
            converted = self.copy(value.element, "*" + expr, scope)
            if value.nullable:
                return f"func() {self.type(value)} {{ if {expr} == nil {{ return nil }}; value := {converted}; return &value }}()"
            return converted
        if value.kind == "buffer":
            if value.ctype.pointee:
                size = (
                    value.length
                    if value.length.isdigit()
                    else f"{scope}.{native_identifier(value.length)}"
                )
                pointer = f"unsafe.Pointer({expr})"
            else:
                pointer, size = f"{expr}.data", f"{expr}.size"
            copy = f"bindingBytes({pointer}, uint64({size}))"
            if is_text(value):
                copy = f"bindingString({pointer}, uint64({size}))"
            return (
                f"func() {self.type(value)} {{ if {pointer} == nil {{ return nil }}; value := {copy}; return &value }}()"
                if value.nullable
                else copy
            )
        if value.kind == "array":
            count = (
                value.length
                if value.length.isdigit()
                else f"{scope}.{native_identifier(value.length)}"
            )
            output = self.type(value)
            elem = value.element
            ctype = elem.native
            source = f"&{expr}[0]" if value.ctype.kind == "array" else expr
            stride = (
                f"{scope}.{native_identifier(value.stride)}"
                if value.stride
                else f"unsafe.Sizeof(C.{ctype}{{}})"
                if elem.kind == "record"
                else f"unsafe.Sizeof(*{source})"
            )
            copy = self.copy(elem, "item")
            return f"func() {output} {{ {f'if {source} == nil {{ return nil }}; ' if value.nullable else ''}length := bindingLength(uint64({count})); result := make({output}, length); for i := range result {{ item := *(*C.{ctype})(bindingElement(unsafe.Pointer({source}), i, uint64({stride}), unsafe.Sizeof(*{source}), unsafe.Alignof(*{source}))); result[i] = {copy}; }}; return result }}()"
        if value.kind == "union":
            return f"func() {self.type(value)} {{ {self.union_copy(value, expr, scope)} }}()"
        self.fail(value, "missing copied value conversion")

    def union_copy(self, value, expr, scope):
        """Statements that return the variant of the union `expr` that the
        tag in `scope` selects."""
        tag = f"{scope}.{native_identifier(value.tag)}"
        arms = []
        for f in value.fields:
            read = f"*(*C.{f.value.native})(unsafe.Pointer(&{expr}))"
            arms.append(
                f"case C.{f.presence.variant}: return {self.union_name(value)}{name(f.name)}Variant{{Value: {self.copy(f.value, read)}}}"
            )
        if value.empty_variant:
            arms.append(f"case C.{value.empty_variant[0]}: return nil")
        if not self.unknown(value):
            self.fail(value, "only a union that native returns is copied")
        return f"switch {tag} {{ {'; '.join(arms)} }}; return UnknownVariant{{Tag: uint32({tag})}}"

    def native(self, value, expr, dest, scope="raw"):
        if value.kind == "buffer" and value.nullable:
            converted = self.native(
                replace(value, nullable=False), f"(*{expr})", dest, scope
            )
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
            data = f"[]byte({expr})" if is_text(value) else expr
            inner = f"arena.bytes({data})"
            if not value.ctype.pointee:
                inner = (
                    f"C.mln_buffer_view{{data: {inner}, size: C.size_t(len({expr}))}}"
                )
            else:
                inner = f"({self.c_type(value)})({inner})"
            code = f"{dest} = {inner}"
            if value.length and not value.length.isdigit():
                code += f"; {scope}.{native_identifier(value.length)} = bindingCountLike({scope}.{native_identifier(value.length)}, len({expr}))"
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
                    setup += f"; {scope}.{native_identifier(value.length)} = bindingCountLike({scope}.{native_identifier(value.length)}, len({expr}))"
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
            prefix = enum_member_prefix((key for key, _ in value.enum_values), native)
            constants = "\n".join(
                doc(self.api, key, None)
                + f"{typename}{name(key.removeprefix(prefix).lower())} {typename} = {typename}(C.{key})"
                for key, number in value.enum_values
            )
            chunks.append(
                doc(self.api, native, typename)
                + f"type {typename} {scalar(value.enum_underlying or value.ctype, value.scalar_carrier)}\nconst (\n{constants}\n)\n"
            )
            if value.enum_kind != "bitmask":
                cases = "\n".join(
                    f'case {number}: return "{typename}{name(key.removeprefix(prefix).lower())}"'
                    for number, key in {
                        number: key for key, number in value.enum_values
                    }.items()
                )
                chunks.append(
                    f'func (value {typename}) String() string {{ switch value {{ {cases} }}; return fmt.Sprintf("{typename}(%d)", value) }}'
                )
                if native == "mln_status":
                    # Go error strings are lowercase words that name the package.
                    messages = "\n".join(
                        f'case {number}: return "maplibre: {key.removeprefix(prefix).lower().replace("_", " ")}"'
                        for number, key in {
                            number: key for key, number in value.enum_values
                        }.items()
                    )
                    chunks.append(
                        f'func (value Status) Error() string {{ switch value {{ {messages} }}; return fmt.Sprintf("maplibre: status %d", int32(value)) }}'
                    )
            if value.enum_kind == "bitmask":
                chunks.append(
                    f"func (value {typename}) Has(flags {typename}) bool {{ return value & flags == flags }}"
                )
                # String names each single-bit flag; wider members are unions
                # of those bits.
                flags = ", ".join(
                    f'{{{number}, "{typename}{name(key.removeprefix(prefix).lower())}"}}'
                    for key, number in value.enum_values
                    if number > 0 and number & (number - 1) == 0
                )
                chunks.append(
                    f"func (value {typename}) String() string {{ return bindingFlagString(uint64(value), []bindingFlag{{{flags}}}) }}"
                )
        for native, value in sorted(self.unions.items()):
            typename = self.union_name(value)
            owner = next(
                (
                    record
                    for record in self.records.values()
                    if record.union_member
                    and record.union_member.value.native == native
                ),
                value,
            )
            # The unexported method seals the interface to its own variants,
            # and to UnknownVariant only when native returns the union.
            marker = "is" + typename
            chunks.append(
                doc(self.api, owner.native, typename)
                + f"type {typename} interface {{ bindingTag() uint32; {marker}() }}"
            )
            if self.unknown(value):
                chunks.append(f"func (UnknownVariant) {marker}() {{}}")
            for f in value.fields:
                wrapper = typename + name(f.name) + "Variant"
                chunks.append(
                    doc(self.api, f"{native}.{f.name}", wrapper)
                    + f"type {wrapper} struct {{ Value {self.type(f.value)} }}\nfunc ({wrapper}) bindingTag() uint32 {{ return uint32(C.{f.presence.variant}) }}\nfunc ({wrapper}) {marker}() {{}}"
                )
        for native, value in sorted(self.records.items()):
            typename = public(native)
            if value.response:
                chunks.append(
                    f"type {typename}Scope struct {{ native *C.{native}; scope *bindingScope }}\n"
                    f"func (response *{typename}Scope) target() bindingTarget {{ if response == nil {{ return bindingScoped(nil) }}; return bindingScoped(response.scope) }}"
                )
                continue
            from .go_callbacks import (
                accepts_native,
                callback_doc,
                native_constructor,
                plain,
                registration_input,
                signature,
            )

            if member := value.union_member:
                chunks.extend(self.union_record(value, member))
                continue
            visible = plain(value) if value.registration else value
            members = list(self.members(visible))
            fields = [
                (
                    doc(self.api, source.name, None)
                    if v is None
                    else doc(self.api, f"{native}.{source.name}", None)
                )
                + f"{member} {self.type(v) if v else 'bool'}"
                for member, v, source in members
            ]
            if value.registration:
                for member in value.registration.callbacks:
                    callback = self.api.callbacks[
                        next(f.value.native for f in value.fields if f.name == member)
                    ]
                    fields.append(
                        callback_doc(
                            doc(self.api, f"{native}.{member}", None), callback
                        )
                        + f"{name(member)} {signature(self, callback)}"
                    )
                if accepts_native(self, value):
                    fields.append("native *bindingNativeRegistration")
            body = (
                "\n" + "\n".join(fields) + "\n"
                if any("//" in field for field in fields)
                else f" {'; '.join(fields)} "
            )
            chunks.append(
                doc(self.api, native, typename) + f"type {typename} struct {{{body}}}"
            )
            if value.registration and accepts_native(self, value):
                chunks.append(native_constructor(self, value))
            if native not in self.api.values:
                continue
            if native in self.api.returned:
                lines = []
                for member, v, f in members:
                    if v and v.registration:
                        continue
                    if v is None:
                        lines.append(
                            f"result.{member} = raw.{native_identifier(f.mask)} & C.{f.name} != 0"
                        )
                        continue
                    convert = self.copy(f.value, "raw." + native_identifier(f.name))
                    if f.presence and f.presence.mask:
                        condition = f"raw.{native_identifier(f.presence.mask)} & C.{f.presence.bit} != 0"
                        if v.kind == "array":
                            lines.append(
                                f"if {condition} {{ result.{member} = {convert} }}"
                            )
                        else:
                            copied = self.type(replace(v, nullable=False))
                            lines.append(
                                f"result.{member} = bindingPresent({condition}, func() {copied} {{ return {convert} }})"
                            )
                    else:
                        lines.append(f"result.{member} = {convert}")
                chunks.append(
                    f"func copy{typename}(raw C.{native}) {typename} {{ var result {typename}; {'; '.join(lines)}; return result }}"
                )
            if native in self.inputs and native in self.api.inputs:
                init = f"C.{value.default}()" if value.default else f"C.{native}{{}}"
                lines = [f"raw := {init}"]
                for f in visible.fields:
                    if f.role == "size":
                        lines.append(
                            f"raw.{native_identifier(f.name)} = bindingCountLike(raw.{native_identifier(f.name)}, int(unsafe.Sizeof(raw)))"
                        )
                    elif f.role == "presence_mask":
                        lines.append(f"raw.{native_identifier(f.name)} = 0")
                for member, v, f in members:
                    expr = "input." + member
                    if v is None:
                        lines.append(
                            f"if {expr} {{ raw.{native_identifier(f.mask)} |= C.{f.name} }}"
                        )
                        continue
                    if v.kind == "union":
                        lines.append(
                            self.union_input(
                                v,
                                expr,
                                f"raw.{native_identifier(f.name)}",
                                self.tag_write(value, v),
                            )
                        )
                        continue
                    body = self.native(
                        f.value,
                        "(*" + expr + ")"
                        if f.presence and f.presence.mask and v.kind != "array"
                        else expr,
                        "raw." + native_identifier(f.name),
                    )
                    if f.presence and f.presence.mask:
                        mask = f"raw.{native_identifier(f.presence.mask)} |= C.{f.presence.bit}"
                        convert = self.converter(f.value)
                        if convert:
                            lines.append(
                                f"bindingMasked(&raw.{native_identifier(f.presence.mask)}, C.{f.presence.bit}, &raw.{native_identifier(f.name)}, {expr}, arena, {convert})"
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
            for member, v, f in self.members(value):
                result = self.type(v) if v else "bool"
                method = (
                    "Unsafe" + member
                    if v and v.kind in {"native_pointer", "union"}
                    else member
                )
                # The scope guards only what the view's owner lends; a copy of
                # any other member stays readable after the scope ends.
                if v and f.scoped:
                    chunks.append(
                        f"func (view {typename}) {method}() ({result},error) {{ return bindingCall(func() {result} {{ runtime.LockOSThread(); defer runtime.UnlockOSThread(); view.scope.check(); return view.value.{member} }}) }}"
                    )
                else:
                    chunks.append(
                        f"func (view {typename}) {method}() {result} {{ return view.value.{member} }}"
                    )
        return "\n\n".join(chunks)
