using System.Reflection;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Internal.Loader;

internal static class NativeLibraryLoader
{
    internal const uint ExpectedAbiVersion = NativeMethods.MLN_C_VERSION;

    private static string DefaultLibraryName =>
        OperatingSystem.IsIOS() || OperatingSystem.IsTvOS() || OperatingSystem.IsMacCatalyst()
            ? "@rpath/MaplibreNativeC.framework/MaplibreNativeC"
            : NativeMethods.LibraryName;

    private static readonly object Gate = new();
    private static bool installed;
    private static bool abiValidated;
    private static nint resolvedHandle;
    private static bool explicitlyLoaded;

    internal static void Load(string libraryPath)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(libraryPath);
        lock (Gate)
        {
            if (resolvedHandle != 0)
            {
                if (explicitlyLoaded)
                {
                    ValidateLoadedAbiLocked();
                    return;
                }

                throw new InvalidOperationException(
                    "The MapLibre Native library has already been resolved through the standard lookup order; call LoadNativeLibrary(path) before any native entry point."
                );
            }

            var handle = NativeLibrary.Load(libraryPath);
            try
            {
                resolvedHandle = ValidateAndCacheResolvedHandle(handle);
            }
            catch
            {
                resolvedHandle = 0;
                explicitlyLoaded = false;
                abiValidated = false;
                throw;
            }
            explicitlyLoaded = true;
            EnsureResolverInstalledLocked();
        }
    }

    internal static void EnsureLoaded()
    {
        lock (Gate)
        {
            EnsureLoadedLocked();
        }
    }

    /// <summary>
    /// Resolves the library through the standard lookup order without checking its ABI version,
    /// so that the version stays readable from a library that fails the check. The next call
    /// through <see cref="EnsureLoaded"/> checks it.
    /// </summary>
    internal static void EnsureResolved()
    {
        lock (Gate)
        {
            EnsureResolverInstalledLocked();
            if (
                resolvedHandle == 0
                && NativeLibrary.TryLoad(
                    DefaultLibraryName,
                    typeof(NativeMethods).Assembly,
                    null,
                    out var handle
                )
            )
            {
                resolvedHandle = handle;
            }
        }
    }

    private static nint ResolveLibrary(
        string libraryName,
        Assembly assembly,
        DllImportSearchPath? searchPath
    )
    {
        if (libraryName != NativeMethods.LibraryName)
        {
            return 0;
        }

        lock (Gate)
        {
            if (resolvedHandle != 0)
            {
                return resolvedHandle;
            }

            if (NativeLibrary.TryLoad(DefaultLibraryName, assembly, searchPath, out var handle))
            {
                return ValidateAndCacheResolvedHandle(handle);
            }

            return 0;
        }
    }

    private static void EnsureLoadedLocked()
    {
        EnsureResolverInstalledLocked();
        ResolveLibrary(NativeMethods.LibraryName, typeof(NativeMethods).Assembly, null);
        ValidateLoadedAbiLocked();
    }

    private static void EnsureResolverInstalledLocked()
    {
        if (installed)
        {
            return;
        }

        NativeLibrary.SetDllImportResolver(typeof(NativeMethods).Assembly, ResolveLibrary);
        installed = true;
    }

    private static unsafe uint ReadAbiVersion(nint handle)
    {
        var version = NativeLibrary.GetExport(handle, "mln_c_version");
        return ((delegate* unmanaged[Cdecl]<uint>)version)();
    }

    private static nint ValidateAndCacheResolvedHandle(nint handle)
    {
        try
        {
            ValidateAbiVersion(ReadAbiVersion(handle));
        }
        catch
        {
            NativeLibrary.Free(handle);
            throw;
        }

        resolvedHandle = handle;
        abiValidated = true;
        return resolvedHandle;
    }

    private static void ValidateLoadedAbiLocked()
    {
        if (abiValidated)
        {
            return;
        }

        ValidateAbiVersion(NativeMethods.mln_c_version());
        abiValidated = true;
    }

    internal static void ValidateAbiVersion(uint actualVersion)
    {
        if (actualVersion == ExpectedAbiVersion)
        {
            return;
        }

        throw new AbiVersionMismatchException(ExpectedAbiVersion, actualVersion);
    }
}
