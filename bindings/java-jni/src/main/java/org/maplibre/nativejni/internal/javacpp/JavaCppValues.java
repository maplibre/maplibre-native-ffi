package org.maplibre.nativejni.internal.javacpp;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.json.JsonValue.Member;

/** Owns temporary JavaCPP descriptor storage borrowed by one synchronous C ABI call. */
public final class JavaCppValues {
  private JavaCppValues() {}

  public static StringViewScope stringView(String value) {
    return new StringViewScope(value);
  }

  public static JsonScope json(JsonValue value) {
    return new JsonScope(value);
  }

  public static StringViewsScope stringViews(String[] values) {
    return new StringViewsScope(values);
  }

  public static JsonValue jsonValue(MaplibreNativeC.mln_json_value value) {
    return switch (value.type()) {
      case MaplibreNativeC.MLN_JSON_VALUE_TYPE_NULL -> JsonValue.nullValue();
      case MaplibreNativeC.MLN_JSON_VALUE_TYPE_BOOL -> JsonValue.of(value.data_bool_value());
      case MaplibreNativeC.MLN_JSON_VALUE_TYPE_UINT -> JsonValue.unsigned(value.data_uint_value());
      case MaplibreNativeC.MLN_JSON_VALUE_TYPE_INT -> JsonValue.of(value.data_int_value());
      case MaplibreNativeC.MLN_JSON_VALUE_TYPE_DOUBLE -> JsonValue.of(value.data_double_value());
      case MaplibreNativeC.MLN_JSON_VALUE_TYPE_STRING ->
          JsonValue.of(string(value.data_string_value()));
      case MaplibreNativeC.MLN_JSON_VALUE_TYPE_ARRAY -> jsonArray(value.data_array_value());
      case MaplibreNativeC.MLN_JSON_VALUE_TYPE_OBJECT -> jsonObject(value.data_object_value());
      default ->
          throw new IllegalArgumentException("unknown native JSON value type: " + value.type());
    };
  }

  public static String string(MaplibreNativeC.mln_string_view view) {
    if (view == null || view.size() == 0 || view.data() == null || view.data().isNull()) {
      return "";
    }
    var bytes = new byte[Math.toIntExact(view.size())];
    view.data().get(bytes, 0, bytes.length);
    return new String(bytes, StandardCharsets.UTF_8);
  }

  public static final class StringViewScope implements AutoCloseable {
    private final BytePointer bytes;
    private final MaplibreNativeC.mln_string_view view;

    private StringViewScope(String value) {
      var utf8 = value == null ? new byte[0] : value.getBytes(StandardCharsets.UTF_8);
      this.bytes = new BytePointer(Math.max(utf8.length, 1));
      if (utf8.length > 0) {
        this.bytes.put(utf8);
      }
      this.view = new MaplibreNativeC.mln_string_view();
      this.view.data(utf8.length == 0 ? null : bytes);
      this.view.size(utf8.length);
    }

    public MaplibreNativeC.mln_string_view view() {
      return view;
    }

    @Override
    public void close() {
      bytes.close();
      view.close();
    }
  }

  private static JsonValue jsonArray(MaplibreNativeC.mln_json_array array) {
    var values = new ArrayList<JsonValue>(Math.toIntExact(array.value_count()));
    var nativeValues = array.values();
    for (var i = 0; i < array.value_count(); i++) {
      values.add(jsonValue(nativeValues.getPointer(i)));
    }
    return JsonValue.array(values);
  }

  private static JsonValue jsonObject(MaplibreNativeC.mln_json_object object) {
    var members = new ArrayList<Member>(Math.toIntExact(object.member_count()));
    var nativeMembers = object.members();
    for (var i = 0; i < object.member_count(); i++) {
      var member = nativeMembers.getPointer(i);
      members.add(new Member(string(member.key()), jsonValue(member.value())));
    }
    return JsonValue.object(members);
  }

  public static final class StringViewsScope implements AutoCloseable {
    private final List<StringViewScope> views = new ArrayList<>();
    private final MaplibreNativeC.mln_string_view nativeViews;

    private StringViewsScope(String[] values) {
      this.nativeViews = new MaplibreNativeC.mln_string_view(values.length);
      for (var i = 0; i < values.length; i++) {
        var view = stringView(values[i]);
        views.add(view);
        nativeViews.position(i).put(view.view());
      }
      nativeViews.position(0);
    }

    public MaplibreNativeC.mln_string_view views() {
      return nativeViews;
    }

    public long count() {
      return views.size();
    }

    @Override
    public void close() {
      nativeViews.close();
      for (var i = views.size() - 1; i >= 0; i--) {
        views.get(i).close();
      }
    }
  }

  public static final class JsonScope implements AutoCloseable {
    private final List<Pointer> owned = new ArrayList<>();
    private final MaplibreNativeC.mln_json_value root;

    private JsonScope(JsonValue value) {
      this.root = jsonValue(value);
    }

    public MaplibreNativeC.mln_json_value value() {
      return root;
    }

    @Override
    public void close() {
      for (var i = owned.size() - 1; i >= 0; i--) {
        owned.get(i).close();
      }
    }

    private <T extends Pointer> T own(T pointer) {
      owned.add(pointer);
      return pointer;
    }

    private MaplibreNativeC.mln_json_value jsonValue(JsonValue value) {
      var out = own(new MaplibreNativeC.mln_json_value());
      out.size(out.sizeof());
      switch (value) {
        case JsonValue.Null ignored -> out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_NULL);
        case JsonValue.Bool node -> {
          out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_BOOL);
          out.data_bool_value(node.value());
        }
        case JsonValue.UInt node -> {
          out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_UINT);
          out.data_uint_value(node.value());
        }
        case JsonValue.Int node -> {
          out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_INT);
          out.data_int_value(node.value());
        }
        case JsonValue.DoubleValue node -> {
          out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_DOUBLE);
          out.data_double_value(node.value());
        }
        case JsonValue.StringValue node -> {
          out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_STRING);
          out.data_string_value(stringView(node.value()));
        }
        case JsonValue.Array node -> {
          out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_ARRAY);
          out.data_array_value(jsonArray(node));
        }
        case JsonValue.ObjectValue node -> {
          out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_OBJECT);
          out.data_object_value(jsonObject(node));
        }
      }
      return out;
    }

    private MaplibreNativeC.mln_json_array jsonArray(JsonValue.Array node) {
      var array = own(new MaplibreNativeC.mln_json_array());
      var values = node.values();
      if (!values.isEmpty()) {
        var nativeValues = own(new MaplibreNativeC.mln_json_value(values.size()));
        for (var i = 0; i < values.size(); i++) {
          nativeValues.position(i).put(jsonValue(values.get(i)));
        }
        nativeValues.position(0);
        array.values(nativeValues);
      }
      array.value_count(values.size());
      return array;
    }

    private MaplibreNativeC.mln_json_object jsonObject(JsonValue.ObjectValue node) {
      var object = own(new MaplibreNativeC.mln_json_object());
      var members = node.members();
      if (!members.isEmpty()) {
        var nativeMembers = own(new MaplibreNativeC.mln_json_member(members.size()));
        for (var i = 0; i < members.size(); i++) {
          var member = members.get(i);
          nativeMembers.position(i);
          nativeMembers.key(stringView(member.key()));
          nativeMembers.value(jsonValue(member.value()));
        }
        nativeMembers.position(0);
        object.members(nativeMembers);
      }
      object.member_count(members.size());
      return object;
    }

    private MaplibreNativeC.mln_string_view stringView(String value) {
      var utf8 = value == null ? new byte[0] : value.getBytes(StandardCharsets.UTF_8);
      var bytes = own(new BytePointer(Math.max(utf8.length, 1)));
      if (utf8.length > 0) {
        bytes.put(utf8);
      }
      var view = own(new MaplibreNativeC.mln_string_view());
      view.data(utf8.length == 0 ? null : bytes);
      view.size(utf8.length);
      return view;
    }
  }
}
