<div align="center">
<h1 style='font-size: 1.8rem'>
☕ + 🦀 = ❤️‍🔥
</h1>
<h3 style='font-size: 1.6rem'>Java JNI Bindings Generator</h3>
</div>

## Introduction 👋

Welcome to java-bindgen, an easy-to-use Java JNI (Java Native Interface)
bindings generator and CLI tool for building Java JARs. This tool simplifies the
process of integrating Rust and Java, enabling you to leverage Rust's
performance and safety in your Java applications.

## Goal 🚀

Develop a robust and safe framework that facilitates seamless and secure
integration between Java and Rust using JNI, minimizing the complexity and risks
associated with native code interoperability.

## Features 🎖️

- Convenient error handling using `JResult<T, JException>` with propagation to
  the Java layer.
- Automatic type conversion for Java primitives like `String`, `byte[]`, `int`,
  `long`, `float`, `boolean`, etc.
- Custom types with `#[derive(JavaClass)]` for seamless integration.
- Integrated Logger `#[derive(JLogger)]` for better debugging and logging
  support.
- Rust error `stack trace` attached to Java Exceptions for improved error
  diagnostics.
- Support for Java `java.util.List<E>` with Rust `JList<E>`.
- Support for Java nullable types in Rust using `Option<T>`.

<br />
<br />

## Prerequisites

Install Rust and Cargo:

- [Linux/MacOS/Unix](https://www.rust-lang.org/tools/install)
- [Windows](https://forge.rust-lang.org/infra/other-installation-methods.html#other-ways-to-install-rustup)

<br />

## Rust Project Setup 🦀

Install `java-pack` CLI 🛠️

```sh
cargo install java-pack --version <version>
```

Example:

```sh
cargo install java-pack --version 0.1.0-alpha.4
```

Add `java-bindgen` dependency

```sh
cargo add java-bindgen
```

Add `Cargo.toml` configuration:

```toml
[package.metadata.java-bindgen]
package = "your.java.package"
```

Set `crate-type`:

```toml
[lib]
crate-type = ["cdylib"]
```

### Verify Your Configuration 🔦

To confirm your setup, run the following command:

```sh
java-pack info
```

<br />
<br />

## Example

☢️ The following examples do not compile due to missing configurations in the
Cargo.toml file. ☢️

lib.rs

```rust compile_fail
use java_bindgen::prelude::*;

#[derive(Default, JavaClass)]
struct User {
    name: String,
}

#[java_bindgen]
fn getUser() -> JResult<User> {
    Ok(User {
        name: "Tom".to_string(),
    })
}
```

Building jar 🫙

```sh
java-pack build
```

Produces the following Java interface and User class:

```java
public static native User getUser();
```

```java
@ToString
@Getter
@Setter
@AllArgsConstructor
public class User {
	String name;
}
```

<br />

## Testing 💯

Create Java test project:

```sh
java-pack new-test
```

Add Test:

```java
@Test
public void should_get_user() {
    UserClass user = TestMacro.getUser();
    assertEquals("Tom", user.getName());
}
```

Run tests:

```sh
java-pack test
```

<br />

## Cross-Platform Build

<table>
  <tr>
    <th colspan=2 rowspan=2>Target</th> <th align=center colspan=4><center>Build Platform</center></th>
  </tr>
  <tr>
    <td>Windows</td>
    <td>Linux</td>
     <td>MacOS</td>
  </tr>
  <tr>
  <td rowspan=3><b>JVM</b></td>
    <td>Windows</td> <td>✅</td><td>✅*</td><td>❔</td>
  </tr>
  <tr>
    <td>Linux</td> <td><b>WSL</b> 🐧</td><td>✅</td><td>❔</td>
  </tr>
    <tr>
    <td>MacOS</td> <td>🚫</td><td>🚫</td><td>❔</td>
  </tr>
</table>

**Legend**

- ✅ Supported
- 🚫 Unsupported
- ❔ No Data

### *Linux -> Windows

Install linker: `x86_64-w64-mingw32-gcc`

```sh
sudo apt install mingw-w64`
```

Build `.dll` and `.so`

```sh
cargo build --target=x86_64-pc-windows-gnu --target=x86_64-unknown-linux-gnu
```

<br />

## Safety 🛡️

Although this crate forbids `unsafe` code, the underlying `JNI` (Java Native
Interface) itself is **not inherently safe**. Therefore, thorough **testing is
required** to ensure that your software is safe to run.

🚨 Any Rust panic that is not handled on the Rust side will cause the JVM to
crash. 🚨

<br />

# Project 📦

#### Project structure 📌

- `java-bindgen` - lib
- `java-pack` - cli tool
- `java-bindgen-macro` - macro system
- `java-bindgen-core` - shared lib

#### Project status 🚧

Project is in early state of development. Each release is prior tested but api
changes will most likely to happen in the future as the project progress.

#### Roadmap 📆

To be determined. If you like the project, please consider giving it a ⭐,
filing an issue ❗, or submitting a pull request (PR) ✅. Your feedback and
contributions are highly appreciated!

- [GitHub](https://github.com/PawelJastrzebski/java-bindgen)

#### Alpha ❗

This crate was developed and tested on Linux so more tests are needed to ensure
that it works on all platforms. Multiplatform jar support allso needs more
testing.

<br />

## More Examples 🤖

#### Logger

lib.rs

```rust compile_fail
use java_bindgen::prelude::*;

#[derive(JLogger)]
struct Log();

#[java_bindgen]
fn test_logger<'a>(env: &mut JNIEnv<'a>, name: String) -> JResult<()> {
    let logger = Log::init(env);
    let msg = format!("Hello {name}, Welcome to Rust!");
    logger.info(msg, env);
    Ok(())
}
```

output

```sh
[main] INFO  com.test.macro.TestMacro  - Hello Java Bindgen, Welcome to Rust!
```

#### Exception Handling

Rust

```rust compile_fail
#[java_bindgen]
fn raw_object_to_string<'a>(env: &mut JNIEnv<'a>, input: JObject<'a>) -> JResult<String> {
    let input_str: String = input.into_rust(env)?;
    Ok(input_str)
}
```

Java signature:

```java
String raw_object_to_string(Object input)
```

When Java pass non String Object

```sh
java.lang.UnsupportedOperationException:
Rust Error:  JNI call failed
   Cause: Cast failed [JObject -> String]
Rust Backtrace:
   0: <core::result::Result<T,E> as java_bindgen::exception::JavaCatch<T>>::j_catch_cause
             at /Projects/java_bindgen/src/exception.rs:145:17
   1: <jni::wrapper::objects::jobject::JObject as java_bindgen::interop::java_to_rust::IntoRustType<alloc::string::String>>::into_rust
             at /Projects/java_bindgen/src/interop.rs:87:13
   2: test_macro::raw_input_type::raw_input_type_2
             at /Projects/java_bindgen/examples/test-macro/src/lib.rs:390:33
   3: Java_com_test_macro_TestMacro_raw_1input_1type_12
             at /Projects/java_bindgen/examples/test-macro/src/lib.rs:388:5
   4: <unknown>
```

#### Complex Types

Rust

```rust compile_fail
#[derive(Default, JavaClass)]
struct Element {
    parent: Node,
    children: JList<Node>,
}

#[derive(Default, JavaClass)]
struct Node {
    node_id: i32,
}

#[java_bindgen]
fn add_new_node(node: Node, element: Element) -> JResult<Element> {
    let mut update = element;
    update.children.add(node);
    Ok(update)
}
```

Java

```java
Node parent = new Node(1);
Node child = new Node(2);
Element element = Element.builder().children(new LinkedList<>()).parent(parent).build();

Element updated = Lib.add_new_node(child, element);
System.out.println("Updated: " + updated);
```

output

```sh
Updated: Element(parent=Node(node_id=1), children=[Node(node_id=2)])
```

<br />

## Full Examples 🧭

For full examples visit:
[github.com/java-bindgen/examples](https://github.com/PawelJastrzebski/java-bindgen/tree/main/examples)

<br />

## Acknowledgments 💌

Thank you to the [jni](https://crates.io/crates/jni) team!

This project strongly relies on the jni crate, and without your hard work, it
would not have been possible.

<br />
<br />
