using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Tests that change process-global state: the log callback, the network status, the trace
/// listeners, or the finalizer queue that a forced collection drains.
/// </summary>
/// <remarks>
/// xUnit runs this collection alone, after every parallel collection has finished, so no other
/// test's callbacks, leaks, or log records reach these tests.
/// </remarks>
[CollectionDefinition(nameof(GlobalState), DisableParallelization = true)]
public sealed class GlobalState;
