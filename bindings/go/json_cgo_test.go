package maplibre

import (
	"math"
	"reflect"
	"testing"
)

func TestCJSONValuePreservesObjectOrderDuplicateKeysAndIntegerWidth(t *testing.T) {
	input := JSONObject(
		JSONMember{Name: "signed", Value: JSONInt(-7)},
		JSONMember{Name: "duplicate", Value: JSONUint(1<<63 + 5)},
		JSONMember{Name: "duplicate", Value: JSONArray(
			JSONDouble(1.25),
			JSONObject(JSONMember{Name: "nested", Value: JSONBool(true)}),
		)},
		JSONMember{Name: "null", Value: JSONNull()},
	)

	materializer := newCJSONMaterializer()
	raw, err := materializer.value(input)
	if err != nil {
		t.Fatalf("value(): %v", err)
	}
	got, err := cJSONValue(&raw)
	materializer.free()
	if err != nil {
		t.Fatalf("cJSONValue(): %v", err)
	}

	if !reflect.DeepEqual(got, input) {
		t.Fatalf("copied JSON = %#v, want %#v", got, input)
	}
	if got.Object[0].Value.Type != JSONValueTypeInt || got.Object[0].Value.Int != -7 {
		t.Fatalf("signed member = %#v, want signed int -7", got.Object[0])
	}
	if got.Object[1].Name != "duplicate" || got.Object[2].Name != "duplicate" {
		t.Fatalf("duplicate member names/order = %#v, want adjacent duplicate names", got.Object)
	}
	if got.Object[1].Value.Type != JSONValueTypeUint || got.Object[1].Value.Uint != 1<<63+5 {
		t.Fatalf("unsigned member = %#v, want uint width preserved", got.Object[1])
	}
}

func TestCJSONMaterializerRejectsNonFiniteDouble(t *testing.T) {
	materializer := newCJSONMaterializer()
	defer materializer.free()

	if _, err := materializer.value(JSONDouble(math.NaN())); err == nil {
		t.Fatal("value(JSONDouble(NaN)) error = nil, want error")
	}
}
