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
