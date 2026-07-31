# rustls-platform-verifier patches

Mise acquires
[`rustls-platform-verifier`](https://github.com/rustls/rustls-platform-verifier)
at commit `1099f161bfc5e3ac7f90aad88b1bf788e72906cb` (`v/0.6.2`) and applies
these patches in order:

1. `0001-namespace-android-verifier.patch` moves the Kotlin package and matching
   Rust JNI descriptors under the MapLibre Native FFI namespace.
2. `0002-use-android-revocation-policy.patch` removes the additional
   `PKIXRevocationChecker` pass and retains Android's trust-manager policy. It
   follows the resolution used by
   [Element X Android](https://github.com/element-hq/element-x-android/commit/f62030ac5b6b4ef9a3247a4d168f91e8a1ebd467)
   for
   [`rustls-platform-verifier#221`](https://github.com/rustls/rustls-platform-verifier/issues/221).
3. `0003-add-modification-notices.patch` marks the two substantively changed
   source files as modified, which Apache-2.0 section 4(b) requires of anyone
   redistributing them.

`NOTICE` states the same modifications for recipients of the compiled artifacts.
Gradle packages it beside the upstream licenses in the Android AARs.

When updating upstream, change the pinned tag and commit together, then rebase
all three patches and refresh `NOTICE`. The mise dependency acquisition fails if
the tag resolves to a different commit or any patch no longer applies.
