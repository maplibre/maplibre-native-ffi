---
title: API Reference
description: Generated API reference documentation for each language binding.
sidebar:
  order: 1
---

Each binding publishes idiomatic HTML reference documentation under this site.

| Binding | Reference                                        |
| ------- | ------------------------------------------------ |
| C       | [C API](/maplibre-native-ffi/reference/c/)       |
| Java    | [Java API](/maplibre-native-ffi/reference/java/) |
| Rust    | [Rust API](/maplibre-native-ffi/reference/rust/) |
| Zig     | [Zig API](/maplibre-native-ffi/reference/zig/)   |

Regenerate all API reference trees before building the docs site:

```bash
mise run //docs:api
```

Or build the full site (API reference plus guides):

```bash
mise run //docs:build
```
