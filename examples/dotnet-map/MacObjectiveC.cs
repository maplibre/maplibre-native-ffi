using System.Runtime.InteropServices;

namespace Maplibre.Native.Examples.DotnetMap;

internal static partial class MacObjectiveC
{
    private const int RtldNow = 0x2;
    private const int RtldLocal = 0x4;
    private const uint CfStringEncodingUtf8 = 0x08000100;
    private static readonly Dictionary<string, nint> Classes = new();
    private static readonly Dictionary<string, nint> Selectors = new();
    private static readonly Dictionary<string, nint> Frameworks = new();

    public static nint AllocInit(string className)
    {
        return SendPointer(SendPointer(Class(className), "alloc"), "init");
    }

    public static nint Retain(nint obj)
    {
        return obj == 0 ? 0 : SendPointer(obj, "retain");
    }

    public static void Release(nint obj)
    {
        if (obj != 0)
        {
            SendVoid(obj, "release");
        }
    }

    public static nint MetalSystemDefaultDevice()
    {
        LoadFramework("Metal");
        return MTLCreateSystemDefaultDevice();
    }

    public static nint CfString(string value)
    {
        var bytes = System.Text.Encoding.UTF8.GetBytes(value);
        var buffer = Marshal.AllocHGlobal(bytes.Length + 1);
        try
        {
            Marshal.Copy(bytes, 0, buffer, bytes.Length);
            Marshal.WriteByte(buffer, bytes.Length, 0);
            return CFStringCreateWithCString(0, buffer, CfStringEncodingUtf8);
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    public static string? NsString(nint nsString)
    {
        return nsString == 0 ? null : Marshal.PtrToStringUTF8(SendPointer(nsString, "UTF8String"));
    }

    public static nint SendPointer(nint receiver, string selectorName)
    {
        return objc_msgSend_retIntPtr(receiver, Selector(selectorName));
    }

    public static nint SendPointer(nint receiver, string selectorName, nint argument)
    {
        return objc_msgSend_retIntPtr_IntPtr(receiver, Selector(selectorName), argument);
    }

    public static nint SendPointer(nint receiver, string selectorName, nint first, nint second)
    {
        return objc_msgSend_retIntPtr_IntPtr_IntPtr(
            receiver,
            Selector(selectorName),
            first,
            second
        );
    }

    public static nint SendPointer(
        nint receiver,
        string selectorName,
        nint first,
        nint second,
        nint third
    )
    {
        return objc_msgSend_retIntPtr_IntPtr_IntPtr_IntPtr(
            receiver,
            Selector(selectorName),
            first,
            second,
            third
        );
    }

    public static void SendVoid(nint receiver, string selectorName)
    {
        objc_msgSend_void(receiver, Selector(selectorName));
    }

    public static void SendVoid(nint receiver, string selectorName, bool argument)
    {
        objc_msgSend_void_bool(receiver, Selector(selectorName), argument);
    }

    public static void SendVoid(nint receiver, string selectorName, nint argument)
    {
        objc_msgSend_void_IntPtr(receiver, Selector(selectorName), argument);
    }

    public static void SendVoid(nint receiver, string selectorName, nint first, ulong second)
    {
        objc_msgSend_void_IntPtr_ulong(receiver, Selector(selectorName), first, second);
    }

    public static void SendVoid(nint receiver, string selectorName, ulong argument)
    {
        objc_msgSend_void_ulong(receiver, Selector(selectorName), argument);
    }

    public static void SendVoid(
        nint receiver,
        string selectorName,
        ulong first,
        ulong second,
        ulong third
    )
    {
        objc_msgSend_void_ulong_ulong_ulong(receiver, Selector(selectorName), first, second, third);
    }

    public static void SendVoid(nint receiver, string selectorName, long argument)
    {
        objc_msgSend_void_long(receiver, Selector(selectorName), argument);
    }

    public static void SendVoid(nint receiver, string selectorName, double argument)
    {
        objc_msgSend_void_double(receiver, Selector(selectorName), argument);
    }

    public static void SendSize(nint receiver, string selectorName, double width, double height)
    {
        objc_msgSend_void_CGSize(receiver, Selector(selectorName), new CGSize(width, height));
    }

    public static Pool AutoreleasePool()
    {
        return new Pool(AllocInit("NSAutoreleasePool"));
    }

    public static string ErrorDescription(nint error)
    {
        return error == 0
            ? "unknown Objective-C error"
            : NsString(SendPointer(error, "localizedDescription")) ?? "unknown Objective-C error";
    }

    private static nint Class(string name)
    {
        if (Classes.TryGetValue(name, out var cached))
        {
            return cached;
        }

        LoadFrameworkForClass(name);
        var cls = objc_getClass(name);
        if (cls == 0)
        {
            throw new InvalidOperationException($"Objective-C class not found: {name}");
        }

        Classes.Add(name, cls);
        return cls;
    }

    private static void LoadFrameworkForClass(string className)
    {
        if (className.StartsWith("CA", StringComparison.Ordinal))
        {
            LoadFramework("QuartzCore");
        }
        else if (className.StartsWith("MTL", StringComparison.Ordinal))
        {
            LoadFramework("Metal");
        }
        else
        {
            LoadFramework("Foundation");
        }
    }

    private static nint Selector(string name)
    {
        if (Selectors.TryGetValue(name, out var cached))
        {
            return cached;
        }

        var selector = sel_registerName(name);
        if (selector == 0)
        {
            throw new InvalidOperationException($"Objective-C selector not found: {name}");
        }

        Selectors.Add(name, selector);
        return selector;
    }

    private static nint LoadFramework(string name)
    {
        if (Frameworks.TryGetValue(name, out var cached))
        {
            return cached;
        }

        var path = $"/System/Library/Frameworks/{name}.framework/{name}";
        var handle = dlopen(path, RtldNow | RtldLocal);
        if (handle == 0)
        {
            throw new InvalidOperationException($"Failed to load framework {path}: {DlError()}");
        }

        Frameworks.Add(name, handle);
        return handle;
    }

    private static string DlError()
    {
        var error = dlerror();
        return error == 0 ? "unknown dlopen error" : Marshal.PtrToStringUTF8(error) ?? "unknown";
    }

    [LibraryImport("/usr/lib/libobjc.A.dylib", StringMarshalling = StringMarshalling.Utf8)]
    private static partial nint objc_getClass(string name);

    [LibraryImport("/usr/lib/libobjc.A.dylib", StringMarshalling = StringMarshalling.Utf8)]
    private static partial nint sel_registerName(string name);

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial nint objc_msgSend_retIntPtr(nint receiver, nint selector);

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial nint objc_msgSend_retIntPtr_IntPtr(
        nint receiver,
        nint selector,
        nint argument
    );

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial nint objc_msgSend_retIntPtr_IntPtr_IntPtr(
        nint receiver,
        nint selector,
        nint first,
        nint second
    );

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial nint objc_msgSend_retIntPtr_IntPtr_IntPtr_IntPtr(
        nint receiver,
        nint selector,
        nint first,
        nint second,
        nint third
    );

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial void objc_msgSend_void(nint receiver, nint selector);

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial void objc_msgSend_void_bool(
        nint receiver,
        nint selector,
        [MarshalAs(UnmanagedType.Bool)] bool argument
    );

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial void objc_msgSend_void_IntPtr(
        nint receiver,
        nint selector,
        nint argument
    );

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial void objc_msgSend_void_IntPtr_ulong(
        nint receiver,
        nint selector,
        nint first,
        ulong second
    );

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial void objc_msgSend_void_ulong(
        nint receiver,
        nint selector,
        ulong argument
    );

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial void objc_msgSend_void_ulong_ulong_ulong(
        nint receiver,
        nint selector,
        ulong first,
        ulong second,
        ulong third
    );

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial void objc_msgSend_void_long(nint receiver, nint selector, long argument);

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial void objc_msgSend_void_double(
        nint receiver,
        nint selector,
        double argument
    );

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_msgSend")]
    private static partial void objc_msgSend_void_CGSize(
        nint receiver,
        nint selector,
        CGSize argument
    );

    [LibraryImport("/System/Library/Frameworks/Metal.framework/Metal")]
    private static partial nint MTLCreateSystemDefaultDevice();

    [LibraryImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")]
    private static partial nint CFStringCreateWithCString(
        nint allocator,
        nint cString,
        uint encoding
    );

    [LibraryImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")]
    public static partial void CFRelease(nint value);

    [LibraryImport("libSystem.dylib", StringMarshalling = StringMarshalling.Utf8)]
    private static partial nint dlopen(string path, int mode);

    [LibraryImport("libSystem.dylib")]
    private static partial nint dlerror();

    [StructLayout(LayoutKind.Sequential)]
    private readonly record struct CGSize(double Width, double Height);

    public sealed class Pool : IDisposable
    {
        private nint pool;

        internal Pool(nint pool)
        {
            this.pool = pool;
        }

        public void Dispose()
        {
            if (pool == 0)
            {
                return;
            }

            SendVoid(pool, "drain");
            pool = 0;
        }
    }
}
