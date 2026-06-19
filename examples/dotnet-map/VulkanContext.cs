using System.Runtime.InteropServices;
using Maplibre.Native;
using Maplibre.Native.Render;
using Silk.NET.Core.Native;
using Silk.NET.GLFW;
using Silk.NET.Vulkan;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed unsafe partial class VulkanContext : IGraphicsContext
{
    private const string KhrSwapchainExtension = "VK_KHR_swapchain";
    private const string KhrPortabilityEnumerationExtension = "VK_KHR_portability_enumeration";
    private const string KhrPortabilitySubsetExtension = "VK_KHR_portability_subset";
    private const string ExtDebugUtilsExtension = "VK_EXT_debug_utils";
    private const int GlfwPlatform = 0x00050003;
    private const int GlfwPlatformWayland = 0x00060003;

    private readonly GlfwWindow window;
    private readonly Vk vk;
    private Instance instance;
    private SurfaceKHR surface;
    private PhysicalDevice physicalDevice;
    private Device device;
    private Queue graphicsQueue;
    private uint graphicsQueueFamilyIndex;
    private bool closed;

    static VulkanContext()
    {
        NativeLibraryResolver.Register();
    }

    private VulkanContext(GlfwWindow window, Vk vk)
    {
        this.window = window;
        this.vk = vk;
    }

    public RenderBackend Backend => RenderBackend.Vulkan;

    public nint WindowHandle => window.NativeHandle;

    public GlfwWindow Window => window;

    public bool ShouldClose => window.ShouldClose;

    public bool CanRenderFrame => window.CanRenderFrame();

    public Vk Api => vk;

    public Instance Instance => instance;

    public SurfaceKHR Surface => surface;

    public PhysicalDevice PhysicalDevice => physicalDevice;

    public Device Device => device;

    public Queue GraphicsQueue => graphicsQueue;

    public uint GraphicsQueueFamilyIndex => graphicsQueueFamilyIndex;

    public static VulkanContext Create(string title, int width, int height)
    {
        SelectWaylandOnLinux();
        var vk = new Vk(Vk.CreateDefaultContext(NativeLibraryResolver.VulkanLibraryCandidates()));
        GlfwWindow? window = null;
        VulkanContext? context = null;

        try
        {
            window = GlfwWindow.Create(
                title,
                width,
                height,
                glfw =>
                {
                    if (!glfw.VulkanSupported())
                    {
                        throw new InvalidOperationException(
                            "GLFW reports Vulkan is not supported."
                        );
                    }

                    ValidateWaylandOnLinux();
                    glfw.WindowHint(WindowHintClientApi.ClientApi, ClientApi.NoApi);
                }
            );
            context = new VulkanContext(window, vk);
            context.CreateInstance();
            context.CreateSurface();
            context.PickPhysicalDeviceAndQueue();
            context.CreateDevice();
            Console.WriteLine(
                $"GLFW {window.Glfw.GetVersionString()}, Vulkan queue family {context.graphicsQueueFamilyIndex}{context.PlatformStatus()}"
            );
            return context;
        }
        catch
        {
            if (context is not null)
            {
                context.Dispose();
            }
            else
            {
                window?.Dispose();
                vk.Dispose();
            }

            throw;
        }
    }

    private static void SelectWaylandOnLinux()
    {
        if (OperatingSystem.IsLinux() && HasWaylandDisplay())
        {
            GlfwNativeAccess.InitHint(GlfwPlatform, GlfwPlatformWayland);
        }
    }

    private static void ValidateWaylandOnLinux()
    {
        if (!OperatingSystem.IsLinux())
        {
            return;
        }

        if (!HasWaylandDisplay())
        {
            Console.Error.WriteLine(
                "WAYLAND_DISPLAY is not set; Linux runtime support for this example targets Wayland."
            );
            return;
        }

        var platform = GlfwNativeAccess.GetPlatform();
        if (platform != GlfwPlatformWayland)
        {
            throw new InvalidOperationException(
                $"GLFW did not select Wayland; selected platform={platform}."
            );
        }
    }

    private static bool HasWaylandDisplay() =>
        !string.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable("WAYLAND_DISPLAY"));

    private string PlatformStatus() =>
        OperatingSystem.IsLinux() ? $", platform {GlfwNativeAccess.GetPlatform()}" : "";

    public VulkanContextDescriptor Descriptor() =>
        new()
        {
            Instance = NativePointer.FromBorrowedAddress(instance.Handle),
            PhysicalDevice = NativePointer.FromBorrowedAddress(physicalDevice.Handle),
            Device = NativePointer.FromBorrowedAddress(device.Handle),
            Queue = NativePointer.FromBorrowedAddress(graphicsQueue.Handle),
            GraphicsQueueFamilyIndex = graphicsQueueFamilyIndex,
            GetInstanceProcAddr = NativePointer.FromBorrowedAddress(
                (nint)vk.GetInstanceProcAddr(instance, "vkGetInstanceProcAddr")
            ),
            GetDeviceProcAddr = NativePointer.FromBorrowedAddress(
                (nint)vk.GetDeviceProcAddr(device, "vkGetDeviceProcAddr")
            ),
        };

    public NativePointer SurfacePointer() =>
        NativePointer.FromBorrowedAddress((nint)surface.Handle);

    public Viewport ReadViewport() => window.ReadViewport();

    public void Resize(Viewport viewport)
    {
        _ = viewport;
    }

    public void PollEvents()
    {
        window.PollEvents();
    }

    public void FinishFrame() { }

    public void WaitIdle()
    {
        if (device.Handle != 0)
        {
            Check(vk.DeviceWaitIdle(device), "vkDeviceWaitIdle");
        }
    }

    public void Dispose()
    {
        if (closed)
        {
            return;
        }

        closed = true;
        if (device.Handle != 0)
        {
            vk.DeviceWaitIdle(device);
            vk.DestroyDevice(device, null);
            device = default;
        }

        if (surface.Handle != 0)
        {
            vkDestroySurfaceKHR(instance.Handle, surface.Handle, 0);
            surface = default;
        }

        if (instance.Handle != 0)
        {
            vk.DestroyInstance(instance, null);
            instance = default;
        }

        vk.Dispose();
        window.Dispose();
    }

    private void CreateInstance()
    {
        var available = InstanceExtensions();
        var extensions = RequiredGlfwInstanceExtensions();
        var enablePortability = available.Contains(KhrPortabilityEnumerationExtension);
        if (enablePortability)
        {
            extensions.Add(KhrPortabilityEnumerationExtension);
        }

        if (available.Contains(ExtDebugUtilsExtension))
        {
            extensions.Add(ExtDebugUtilsExtension);
        }

        var appName = SilkMarshal.StringToPtr("dotnet-map", NativeStringEncoding.UTF8);
        var engineName = SilkMarshal.StringToPtr("maplibre-native-ffi", NativeStringEncoding.UTF8);
        var extensionNames = SilkMarshal.StringArrayToPtr(
            extensions.ToArray(),
            NativeStringEncoding.UTF8
        );
        try
        {
            var app = new ApplicationInfo
            {
                SType = StructureType.ApplicationInfo,
                PApplicationName = (byte*)appName,
                PEngineName = (byte*)engineName,
                ApiVersion = Vk.Version10,
            };
            var createInfo = new InstanceCreateInfo
            {
                SType = StructureType.InstanceCreateInfo,
                PApplicationInfo = &app,
                EnabledExtensionCount = checked((uint)extensions.Count),
                PpEnabledExtensionNames = (byte**)extensionNames,
                Flags = enablePortability
                    ? InstanceCreateFlags.EnumeratePortabilityBitKhr
                    : InstanceCreateFlags.None,
            };

            Check(vk.CreateInstance(&createInfo, null, out instance), "vkCreateInstance");
            Console.WriteLine(
                "Enabled Vulkan instance extensions: " + string.Join(", ", extensions)
            );
        }
        finally
        {
            SilkMarshal.Free(appName);
            SilkMarshal.Free(engineName);
            SilkMarshal.Free((nint)extensionNames);
        }
    }

    private void CreateSurface()
    {
        var outSurface = new VkNonDispatchableHandle(0);
        Check(
            (Result)
                window.Glfw.CreateWindowSurface(
                    new VkHandle(instance.Handle),
                    window.Handle,
                    null,
                    &outSurface
                ),
            "glfwCreateWindowSurface"
        );
        surface = new SurfaceKHR(outSurface.Handle);
    }

    private void PickPhysicalDeviceAndQueue()
    {
        uint count = 0;
        Check(
            vk.EnumeratePhysicalDevices(instance, &count, null),
            "vkEnumeratePhysicalDevices(count)"
        );
        if (count == 0)
        {
            throw new InvalidOperationException("No Vulkan physical devices found.");
        }

        var devices = stackalloc PhysicalDevice[checked((int)count)];
        Check(vk.EnumeratePhysicalDevices(instance, &count, devices), "vkEnumeratePhysicalDevices");

        for (uint i = 0; i < count; i++)
        {
            var queueFamily = FindGraphicsPresentQueueFamily(devices[i]);
            if (queueFamily < 0)
            {
                continue;
            }

            physicalDevice = devices[i];
            graphicsQueueFamilyIndex = checked((uint)queueFamily);
            return;
        }

        throw new InvalidOperationException(
            "No Vulkan device has a graphics queue that can present."
        );
    }

    private int FindGraphicsPresentQueueFamily(PhysicalDevice candidate)
    {
        uint count = 0;
        vk.GetPhysicalDeviceQueueFamilyProperties(candidate, &count, null);
        var families = stackalloc QueueFamilyProperties[checked((int)count)];
        vk.GetPhysicalDeviceQueueFamilyProperties(candidate, &count, families);

        for (uint i = 0; i < count; i++)
        {
            if ((families[i].QueueFlags & QueueFlags.GraphicsBit) == 0)
            {
                continue;
            }

            uint presentSupported = 0;
            Check(
                vkGetPhysicalDeviceSurfaceSupportKHR(
                    candidate.Handle,
                    i,
                    surface.Handle,
                    &presentSupported
                ),
                "vkGetPhysicalDeviceSurfaceSupportKHR"
            );
            if (presentSupported != 0)
            {
                return checked((int)i);
            }
        }

        return -1;
    }

    private void CreateDevice()
    {
        var available = DeviceExtensions(physicalDevice);
        if (!available.Contains(KhrSwapchainExtension))
        {
            throw new InvalidOperationException(
                "Selected Vulkan device does not support VK_KHR_swapchain."
            );
        }

        var extensions = new List<string> { KhrSwapchainExtension };
        if (available.Contains(KhrPortabilitySubsetExtension))
        {
            extensions.Add(KhrPortabilitySubsetExtension);
        }

        var extensionNames = SilkMarshal.StringArrayToPtr(
            extensions.ToArray(),
            NativeStringEncoding.UTF8
        );
        try
        {
            var priority = 1.0f;
            var queueInfo = new DeviceQueueCreateInfo
            {
                SType = StructureType.DeviceQueueCreateInfo,
                QueueFamilyIndex = graphicsQueueFamilyIndex,
                QueueCount = 1,
                PQueuePriorities = &priority,
            };
            var createInfo = new DeviceCreateInfo
            {
                SType = StructureType.DeviceCreateInfo,
                QueueCreateInfoCount = 1,
                PQueueCreateInfos = &queueInfo,
                EnabledExtensionCount = checked((uint)extensions.Count),
                PpEnabledExtensionNames = (byte**)extensionNames,
            };

            Check(vk.CreateDevice(physicalDevice, &createInfo, null, out device), "vkCreateDevice");
            vk.GetDeviceQueue(device, graphicsQueueFamilyIndex, 0, out graphicsQueue);
            Console.WriteLine("Enabled Vulkan device extensions: " + string.Join(", ", extensions));
        }
        finally
        {
            SilkMarshal.Free((nint)extensionNames);
        }
    }

    private HashSet<string> RequiredGlfwInstanceExtensions()
    {
        var required = new HashSet<string>(StringComparer.Ordinal);
        var pointer = window.Glfw.GetRequiredInstanceExtensions(out var count);
        if (pointer is null || count == 0)
        {
            throw new InvalidOperationException("GLFW did not return Vulkan instance extensions.");
        }

        for (var i = 0; i < count; i++)
        {
            required.Add(Marshal.PtrToStringUTF8((nint)pointer[i]) ?? string.Empty);
        }

        return required;
    }

    private HashSet<string> InstanceExtensions()
    {
        uint count = 0;
        Check(
            vk.EnumerateInstanceExtensionProperties((byte*)null, &count, null),
            "vkEnumerateInstanceExtensionProperties(count)"
        );
        var properties = stackalloc ExtensionProperties[checked((int)count)];
        Check(
            vk.EnumerateInstanceExtensionProperties((byte*)null, &count, properties),
            "vkEnumerateInstanceExtensionProperties"
        );
        return ExtensionSet(properties, count);
    }

    private HashSet<string> DeviceExtensions(PhysicalDevice candidate)
    {
        uint count = 0;
        Check(
            vk.EnumerateDeviceExtensionProperties(candidate, (byte*)null, &count, null),
            "vkEnumerateDeviceExtensionProperties(count)"
        );
        var properties = stackalloc ExtensionProperties[checked((int)count)];
        Check(
            vk.EnumerateDeviceExtensionProperties(candidate, (byte*)null, &count, properties),
            "vkEnumerateDeviceExtensionProperties"
        );
        return ExtensionSet(properties, count);
    }

    private static HashSet<string> ExtensionSet(ExtensionProperties* properties, uint count)
    {
        var extensions = new HashSet<string>(StringComparer.Ordinal);
        for (var i = 0; i < count; i++)
        {
            byte* extensionName = properties[i].ExtensionName;
            var name = Marshal.PtrToStringUTF8((nint)extensionName);
            if (!string.IsNullOrEmpty(name))
            {
                extensions.Add(name);
            }
        }

        return extensions;
    }

    public static void Check(Result result, string operation)
    {
        if (result != Result.Success)
        {
            throw new InvalidOperationException($"{operation} failed with {result}.");
        }
    }

    [LibraryImport("vulkan", EntryPoint = "vkGetPhysicalDeviceSurfaceSupportKHR")]
    private static partial Result vkGetPhysicalDeviceSurfaceSupportKHR(
        nint physicalDevice,
        uint queueFamilyIndex,
        ulong surface,
        uint* supported
    );

    [LibraryImport("vulkan", EntryPoint = "vkDestroySurfaceKHR")]
    private static partial void vkDestroySurfaceKHR(nint instance, ulong surface, nint allocator);
}
