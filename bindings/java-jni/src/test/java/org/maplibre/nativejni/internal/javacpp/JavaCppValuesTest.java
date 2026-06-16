package org.maplibre.nativejni.internal.javacpp;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.List;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.json.JsonValue;

// Support invariant for BND-064 and BND-067: JavaCPP conversion is the internal
// materializer that carries structured JSON across the JNI boundary.
final class JavaCppValuesTest {
  @Test
  void bnd064AndBnd067JsonValuesRoundTripStructuredSemantics() {
    var value =
        JsonValue.object(
            List.of(
                new JsonValue.Member("key", JsonValue.of(1L)),
                new JsonValue.Member("key", JsonValue.unsigned(-1L)),
                new JsonValue.Member(
                    "nested",
                    JsonValue.array(
                        List.of(
                            JsonValue.nullValue(),
                            JsonValue.of(true),
                            JsonValue.of(1.5),
                            JsonValue.of("text")))),
                new JsonValue.Member(
                    "object",
                    JsonValue.object(List.of(new JsonValue.Member("inner", JsonValue.of(-2L)))))));

    try (var scope = JavaCppValues.json(value)) {
      assertEquals(value, JavaCppValues.jsonValue(scope.value()));
    }
  }
}
