# Contributing

Discuss changes in `#maplibre` on the
[OSM-US Slack](https://slack.openstreetmap.us/).

## Before Making Changes

See the
[development overview](https://maplibre.org/maplibre-native-ffi/development/)
for platform setup, pinned tools, local commands, tests, and examples.

Read [concepts](https://maplibre.org/maplibre-native-ffi/concepts/) before
changing behavior. Read the
[C API Conventions](https://maplibre.org/maplibre-native-ffi/development/c-conventions/)
before changing public C interfaces or C ABI behavior. Read the
[Binding specification](https://maplibre.org/maplibre-native-ffi/development/binding-specification/)
before changing language bindings or generated binding reference docs.

Keep pull requests focused on one reviewable change. The reviewer should be able
to connect the use case, public behavior, implementation, and validation without
separating unrelated work.

## Pull Requests

Open a pull request when the change is ready for review and include:

- the problem or use case;
- the public API or behavior change, if any;
- the validation you ran;
- platform limitations or native MapLibre behavior you checked;

If you use AI assistance, follow the [AI policy](./AI_POLICY.md).
