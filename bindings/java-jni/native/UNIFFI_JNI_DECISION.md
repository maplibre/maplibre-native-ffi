# uniffi-bindgen-java-jni suitability decision

Audience: Java JNI binding maintainers. Category: Explanation / decision record.

## Decision

Do not rewrite the Java JNI binding to `uniffi-bindgen-java-jni` in its current
state.

The tool is promising, but the current generator does not meet the soundness bar
for this binding. The spike stopped before broad rewrite churn.

## What was tried

The spike used the upstream `moheng233/uniffi-bindgen-java-jni` repository at
`85ca522f7237fa6aa329e56fee2fedf819a78740` and ran its bundled `simple` example,
which covers the same categories we need for the Java JNI binding:

- top-level functions with primitive and string values;
- object construction, method calls, and destruction;
- records and enums;
- Java callback interfaces and callback handle registration.

Commands run:

```sh
cd /tmp/uniffi-bindgen-java-jni
cargo test
rm -rf examples/simple/generated
cargo run -- \
  --source examples/simple/src/simple.udl \
  --config examples/simple/uniffi.toml \
  --java-out-dir examples/simple/generated/java \
  --rust-out-dir examples/simple/generated/rust-glue \
  --main-crate-path examples/simple
cd examples/simple/generated/rust-glue
cargo build
cd ../..
javac -cp generated/java -d /tmp/uniffi-simple-classes TestSimple.java
java -ea \
  -Djava.library.path=generated/rust-glue/target/debug \
  -cp generated/java:/tmp/uniffi-simple-classes \
  TestSimple
```

## Evidence

The generator's Rust unit tests passed: 45 tests passed.

The generated Java/Rust example did not pass end-to-end. The first failure was a
corrupted string result from the generated `greet` binding:

```text
greet("世界") = :8\0\0\0\0\0\0\0\2\0\0\0\0\0
java.lang.AssertionError: greet failed
```

Inspection of the generated Rust glue showed that `rustbuffer_to_jni_bytebuffer`
creates a Java direct `ByteBuffer` over the UniFFI `RustBuffer` data pointer and
then immediately destroys the `RustBuffer`. Java then reads through a direct
buffer that points at freed native memory.

A temporary local edit that skipped `rb.destroy()` allowed the string path to
work, but the object lifecycle path still failed. A fresh calculator-only test
reported:

```text
add=150 val1=0 sub=0 val2=0
```

That means the generated object method path did not preserve object state across
calls in the bundled example.

The generated Java also uses deprecated finalizers for object cleanup and the
generated glue crate has a native-library filename collision with the main crate
in the bundled example. These are secondary concerns; the string use-after-free
and object-state failure are enough to stop the rewrite.

## Impact on this binding

The Java JNI binding requires sound string and buffer transfer, object
ownership, close behavior, diagnostics, and callback lifecycle handling. The
observed failures occur in the generator's own representative example before any
MapLibre-specific complexity is introduced.

Adopting this tool now would require carrying local generator patches for memory
safety and object handle semantics. That would leave exactly the tooling debt
the rewrite is meant to remove.

## Recommended next options

1. Evaluate a direct C-to-Java JNI generator, especially JavaCPP, against the C
   ABI. This may remove more bridge code than a Rust-side JNI generator.
2. Evaluate flapigen if keeping a Rust facade remains preferred.
3. Revisit `uniffi-bindgen-java-jni` only after upstream fixes the generated
   RustBuffer ownership and object handle behavior and its own end-to-end
   example passes without local edits.
