using System.Collections.Immutable;
using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Tests;

/// <summary>The graphics APIs that tests/graphics drives, as mln_test_graphics.h numbers them.</summary>
public enum TestGraphicsBackend : uint
{
    Metal = 1,
    Vulkan = 2,
    Egl = 3,
    Wgl = 4,
}

/// <summary>
/// A device or context from tests/graphics, which stands in for the host's. The handles in
/// <see cref="Context" /> stay valid until the object is disposed.
/// </summary>
/// <remarks>
/// The library and every handle it creates belong to one thread at a time, so a fixture creates,
/// uses, and disposes it on the thread that drives its sessions.
/// </remarks>
internal sealed partial class TestGraphics : IDisposable
{
    private const string Library = "mln_test_graphics";
    private nint handle;

    private TestGraphics(nint handle, TestGraphicsContext context)
    {
        this.handle = handle;
        Context = context;
    }

    internal TestGraphicsContext Context { get; }

    internal static TestGraphics Create(TestGraphicsBackend backend)
    {
        var created = mln_test_graphics_create((uint)backend);
        if (created == 0)
            throw new InvalidOperationException(
                $"tests/graphics could not create a {backend} context: {LastError()}"
            );
        if (!mln_test_graphics_get_context(created, out var context))
        {
            var error = LastError();
            mln_test_graphics_destroy(created);
            throw new InvalidOperationException(
                $"tests/graphics has no {backend} context: {error}"
            );
        }
        return new TestGraphics(created, context);
    }

    /// <summary>Makes an EGL or WGL context current on the calling thread.</summary>
    internal void MakeCurrent()
    {
        if (!mln_test_graphics_make_current(handle))
            throw new InvalidOperationException(
                $"tests/graphics could not make its context current: {LastError()}"
            );
    }

    /// <summary>
    /// Creates a texture that a borrowed texture descriptor can name. Dispose it before this
    /// object.
    /// </summary>
    internal TestGraphicsTexture CreateTexture(uint width, uint height)
    {
        var texture = mln_test_graphics_texture_create(handle, width, height);
        if (texture == 0)
            throw new InvalidOperationException(
                $"tests/graphics could not create a texture: {LastError()}"
            );
        if (!mln_test_graphics_texture_get_info(texture, out var info))
        {
            var error = LastError();
            mln_test_graphics_texture_destroy(texture);
            throw new InvalidOperationException(
                $"tests/graphics could not describe a texture: {error}"
            );
        }
        return new TestGraphicsTexture(texture, info);
    }

    public void Dispose()
    {
        if (handle != 0)
        {
            mln_test_graphics_destroy(handle);
            handle = 0;
        }
    }

    private static string LastError() =>
        Marshal.PtrToStringUTF8(mln_test_graphics_last_error()) ?? "no reason recorded";

    [LibraryImport(Library)]
    private static partial nint mln_test_graphics_last_error();

    [LibraryImport(Library)]
    private static partial nint mln_test_graphics_create(uint backend);

    [LibraryImport(Library)]
    private static partial void mln_test_graphics_destroy(nint graphics);

    [LibraryImport(Library)]
    [return: MarshalAs(UnmanagedType.U1)]
    private static partial bool mln_test_graphics_get_context(
        nint graphics,
        out TestGraphicsContext context
    );

    [LibraryImport(Library)]
    [return: MarshalAs(UnmanagedType.U1)]
    private static partial bool mln_test_graphics_make_current(nint graphics);

    [LibraryImport(Library)]
    private static partial nint mln_test_graphics_texture_create(
        nint graphics,
        uint width,
        uint height
    );

    [LibraryImport(Library)]
    internal static partial void mln_test_graphics_texture_destroy(nint texture);

    [LibraryImport(Library)]
    [return: MarshalAs(UnmanagedType.U1)]
    private static partial bool mln_test_graphics_texture_get_info(
        nint texture,
        out TestGraphicsTextureInfo info
    );
}

/// <summary>A texture from <see cref="TestGraphics.CreateTexture" />.</summary>
internal sealed class TestGraphicsTexture : IDisposable
{
    private nint handle;

    internal TestGraphicsTexture(nint handle, TestGraphicsTextureInfo info)
    {
        this.handle = handle;
        Info = info;
    }

    internal TestGraphicsTextureInfo Info { get; }

    public void Dispose()
    {
        if (handle != 0)
        {
            TestGraphics.mln_test_graphics_texture_destroy(handle);
            handle = 0;
        }
    }
}

/// <summary>mln_test_graphics_context: only the fields of the context's backend are set.</summary>
[StructLayout(LayoutKind.Sequential)]
internal struct TestGraphicsContext
{
    /// <summary>The names of the device extensions that the Vulkan device enabled.</summary>
    internal readonly unsafe ImmutableArray<string> VulkanDeviceExtensionNames()
    {
        var names = (nint*)VulkanDeviceExtensions;
        var builder = ImmutableArray.CreateBuilder<string>((int)VulkanDeviceExtensionCount);
        for (var index = 0; index < VulkanDeviceExtensionCount; index++)
            builder.Add(Marshal.PtrToStringUTF8(names[index])!);
        return builder.MoveToImmutable();
    }

    internal uint Backend;
    internal uint VulkanQueueFamilyIndex;
    internal nint MetalDevice;
    internal nint VulkanInstance;
    internal nint VulkanPhysicalDevice;
    internal nint VulkanDevice;
    internal nint VulkanQueue;
    internal nint VulkanGetInstanceProcAddr;
    internal nint VulkanGetDeviceProcAddr;
    internal nint VulkanDeviceExtensions;
    internal uint VulkanDeviceExtensionCount;
    internal nint EglDisplay;
    internal nint EglConfig;
    internal nint EglContext;
    internal nint WglDeviceContext;
    internal nint WglContext;
    internal nint GetProcAddress;
}

/// <summary>mln_test_graphics_texture_info: only the fields of the texture's backend are set.</summary>
[StructLayout(LayoutKind.Sequential)]
internal struct TestGraphicsTextureInfo
{
    internal ulong VulkanImage;
    internal ulong VulkanImageView;
    internal nint MetalTexture;
    internal uint Width;
    internal uint Height;
    internal uint Format;
    internal uint VulkanFinalLayout;
    internal uint OpenGlTexture;
    internal uint OpenGlTarget;
}
