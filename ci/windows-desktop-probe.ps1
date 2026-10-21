$ErrorActionPreference = "Stop"

Write-Output "OS: $([Environment]::OSVersion.VersionString)"
Write-Output "User interactive: $([Environment]::UserInteractive)"
Write-Output "Process session: $((Get-Process -Id $PID).SessionId)"
Write-Output "Session name: $env:SESSIONNAME"
Write-Output "Image: $env:ImageOS / $env:ImageVersion"
Get-CimInstance Win32_VideoController |
  Select-Object Name, DriverVersion, CurrentHorizontalResolution, CurrentVerticalResolution |
  Format-List
query session

Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class DesktopProbe {
    [DllImport("user32.dll")]
    public static extern int GetSystemMetrics(int index);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr OpenInputDesktop(uint flags, bool inherit, uint access);
    [DllImport("user32.dll")]
    public static extern bool CloseDesktop(IntPtr desktop);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr CreateWindowEx(uint exStyle, string windowClass,
        string name, uint style, int x, int y, int width, int height,
        IntPtr parent, IntPtr menu, IntPtr instance, IntPtr parameter);
    [DllImport("user32.dll")]
    private static extern bool DestroyWindow(IntPtr window);
    [DllImport("user32.dll")]
    private static extern IntPtr GetDC(IntPtr window);
    [DllImport("user32.dll")]
    private static extern int ReleaseDC(IntPtr window, IntPtr dc);
    [DllImport("gdi32.dll")]
    private static extern IntPtr CreateCompatibleDC(IntPtr dc);
    [DllImport("gdi32.dll")]
    private static extern bool DeleteDC(IntPtr dc);
    [DllImport("gdi32.dll")]
    private static extern IntPtr CreateDIBSection(IntPtr dc, byte[] info,
        uint usage, out IntPtr bits, IntPtr section, uint offset);
    [DllImport("gdi32.dll")]
    private static extern IntPtr SelectObject(IntPtr dc, IntPtr obj);
    [DllImport("gdi32.dll")]
    private static extern bool DeleteObject(IntPtr obj);
    [DllImport("gdi32.dll", SetLastError = true)]
    private static extern bool StretchBlt(IntPtr destination, int x, int y,
        int width, int height, IntPtr source, int sx, int sy, int sw, int sh, uint op);

    public static void ProbePresentation() {
        IntPtr window = CreateWindowEx(0, "STATIC", "GDI presentation probe",
            0x80000000, 0, 0, 8, 8, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero);
        Console.WriteLine("Hidden window created: " + (window != IntPtr.Zero));
        if (window == IntPtr.Zero) return;
        IntPtr destination = GetDC(window);
        IntPtr source = CreateCompatibleDC(destination);
        byte[] info = new byte[44];
        BitConverter.GetBytes(40).CopyTo(info, 0);
        BitConverter.GetBytes(8).CopyTo(info, 4);
        BitConverter.GetBytes(-8).CopyTo(info, 8);
        BitConverter.GetBytes((ushort)1).CopyTo(info, 12);
        BitConverter.GetBytes((ushort)32).CopyTo(info, 14);
        IntPtr bits;
        IntPtr bitmap = CreateDIBSection(source, info, 0, out bits, IntPtr.Zero, 0);
        IntPtr previous = SelectObject(source, bitmap);
        bool copied = StretchBlt(destination, 0, 0, 8, 8, source, 0, 0, 8, 8, 0x00CC0020);
        int error = Marshal.GetLastWin32Error();
        Console.WriteLine("GDI StretchBlt: " + copied + ", last error: " + error);
        SelectObject(source, previous);
        DeleteObject(bitmap);
        DeleteDC(source);
        ReleaseDC(window, destination);
        DestroyWindow(window);
    }
}
'@

Write-Output "Screen: $([DesktopProbe]::GetSystemMetrics(0)) x $([DesktopProbe]::GetSystemMetrics(1))"
Write-Output "Display monitors: $([DesktopProbe]::GetSystemMetrics(80))"
$desktop = [DesktopProbe]::OpenInputDesktop(0, $false, 1)
if ($desktop -eq [IntPtr]::Zero) {
  Write-Output "OpenInputDesktop failed: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())"
} else {
  Write-Output "Input desktop is accessible"
  [DesktopProbe]::CloseDesktop($desktop) | Out-Null
}
[DesktopProbe]::ProbePresentation()
# A diagnostic command's exit code must not prevent the suites from running.
exit 0
