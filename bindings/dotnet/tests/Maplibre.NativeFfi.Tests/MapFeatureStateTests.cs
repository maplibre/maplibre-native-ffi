using System.Text;
using System.Text.Json;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class MapFeatureStateTests
{
    private static FeatureStateSelector Selector(string? stateKey = null) =>
        new()
        {
            SourceId = "geo",
            FeatureId = "42",
            StateKey = stateKey,
        };

    private static JsonElement ParseState(byte[] state) =>
        JsonDocument.Parse(Encoding.UTF8.GetString(state)).RootElement;

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task FeatureStateRoundTripsThroughTheMapStore()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        // Missing feature state reads back as an empty JSON object.
        var missing = ParseState(
            await map.GetFeatureStateAsync(Selector(), TestContext.Current.CancellationToken)
        );
        Assert.Equal(JsonValueKind.Object, missing.ValueKind);
        Assert.Empty(missing.EnumerateObject());

        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetFeatureStateAsync(Selector(), """{"hover":true,"rank":2}"""u8.ToArray()),
            MaplibreStatus.Ok
        );

        var state = ParseState(
            await map.GetFeatureStateAsync(Selector(), TestContext.Current.CancellationToken)
        );
        Assert.True(state.GetProperty("hover").GetBoolean());
        Assert.Equal(2, state.GetProperty("rank").GetInt32());

        // Removing one key leaves the others in place.
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveFeatureStateAsync(Selector("hover")),
            MaplibreStatus.Ok
        );
        var remaining = ParseState(
            await map.GetFeatureStateAsync(Selector(), TestContext.Current.CancellationToken)
        );
        Assert.False(remaining.TryGetProperty("hover", out _));
        Assert.Equal(2, remaining.GetProperty("rank").GetInt32());

        // Removing without a key clears the feature's state.
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveFeatureStateAsync(Selector()),
            MaplibreStatus.Ok
        );
        var cleared = ParseState(
            await map.GetFeatureStateAsync(Selector(), TestContext.Current.CancellationToken)
        );
        Assert.Empty(cleared.EnumerateObject());
    }

    [BindingSpecTest("BND-025")]
    [Fact]
    public async Task FeatureStateCommandsValidateSelectorShape()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        var withoutFeature = new FeatureStateSelector { SourceId = "geo" };

        // Set and get require a feature ID.
        await Assert.ThrowsAsync<InvalidArgumentException>(() =>
            map.SetFeatureStateAsync(withoutFeature, """{"hover":true}"""u8.ToArray())
        );
        await Assert.ThrowsAsync<InvalidArgumentException>(() =>
            map.GetFeatureStateAsync(withoutFeature, TestContext.Current.CancellationToken)
        );

        // Remove accepts a bare state key only alongside a feature ID.
        await Assert.ThrowsAsync<InvalidArgumentException>(() =>
            map.RemoveFeatureStateAsync(
                new FeatureStateSelector { SourceId = "geo", StateKey = "hover" }
            )
        );

        // Set requires one JSON object.
        await Assert.ThrowsAsync<InvalidArgumentException>(() =>
            map.SetFeatureStateAsync(Selector(), """["not-an-object"]"""u8.ToArray())
        );
    }
}
