package maplibre

// JSONValueType identifies the active field in a JSONValue.
type JSONValueType uint8

const (
	JSONValueTypeNull JSONValueType = iota
	JSONValueTypeBool
	JSONValueTypeString
	JSONValueTypeInt
	JSONValueTypeUint
	JSONValueTypeDouble
	JSONValueTypeArray
	JSONValueTypeObject
)

// JSONMember is one ordered object member. Object values may contain repeated
// member names.
type JSONMember struct {
	Name  string
	Value JSONValue
}

// JSONMembers is an ordered JSON object member list.
type JSONMembers []JSONMember

// JSONValue is a low-level structured JSON value. The zero value represents
// JSON null. Double values must be finite when passed to native APIs.
type JSONValue struct {
	Type   JSONValueType
	Bool   bool
	String string
	Int    int64
	Uint   uint64
	Double float64
	Array  []JSONValue
	Object JSONMembers
}

// Equal reports whether two JSON values are structurally equal, comparing only the field the type
// selects and preserving array element order, object member order, and repeated member names. Use
// this instead of ==, which does not compile for structs holding slices.
func (v JSONValue) Equal(other JSONValue) bool {
	if v.Type != other.Type {
		return false
	}
	switch v.Type {
	case JSONValueTypeNull:
		return true
	case JSONValueTypeBool:
		return v.Bool == other.Bool
	case JSONValueTypeString:
		return v.String == other.String
	case JSONValueTypeInt:
		return v.Int == other.Int
	case JSONValueTypeUint:
		return v.Uint == other.Uint
	case JSONValueTypeDouble:
		return v.Double == other.Double
	case JSONValueTypeArray:
		if len(v.Array) != len(other.Array) {
			return false
		}
		for index := range v.Array {
			if !v.Array[index].Equal(other.Array[index]) {
				return false
			}
		}
		return true
	case JSONValueTypeObject:
		if len(v.Object) != len(other.Object) {
			return false
		}
		for index := range v.Object {
			if v.Object[index].Name != other.Object[index].Name ||
				!v.Object[index].Value.Equal(other.Object[index].Value) {
				return false
			}
		}
		return true
	default:
		return false
	}
}

// Clone returns an independent deep copy of this JSON value.
func (v JSONValue) Clone() JSONValue {
	cloned := v
	if v.Array != nil {
		cloned.Array = make([]JSONValue, len(v.Array))
		for index := range v.Array {
			cloned.Array[index] = v.Array[index].Clone()
		}
	}
	if v.Object != nil {
		cloned.Object = make(JSONMembers, len(v.Object))
		for index := range v.Object {
			cloned.Object[index] = JSONMember{
				Name:  v.Object[index].Name,
				Value: v.Object[index].Value.Clone(),
			}
		}
	}
	return cloned
}

// JSONNull returns a null JSON value.
func JSONNull() JSONValue {
	return JSONValue{Type: JSONValueTypeNull}
}

// JSONBool returns a bool JSON value.
func JSONBool(value bool) JSONValue {
	return JSONValue{Type: JSONValueTypeBool, Bool: value}
}

// JSONString returns a string JSON value.
func JSONString(value string) JSONValue {
	return JSONValue{Type: JSONValueTypeString, String: value}
}

// JSONInt returns a signed integer JSON value.
func JSONInt(value int64) JSONValue {
	return JSONValue{Type: JSONValueTypeInt, Int: value}
}

// JSONUint returns an unsigned integer JSON value.
func JSONUint(value uint64) JSONValue {
	return JSONValue{Type: JSONValueTypeUint, Uint: value}
}

// JSONDouble returns a double JSON value.
func JSONDouble(value float64) JSONValue {
	return JSONValue{Type: JSONValueTypeDouble, Double: value}
}

// JSONArray returns an array JSON value.
func JSONArray(values ...JSONValue) JSONValue {
	out := make([]JSONValue, len(values))
	copy(out, values)
	return JSONValue{Type: JSONValueTypeArray, Array: out}
}

// JSONObject returns an ordered object JSON value.
func JSONObject(members ...JSONMember) JSONValue {
	out := make(JSONMembers, len(members))
	copy(out, members)
	return JSONValue{Type: JSONValueTypeObject, Object: out}
}
