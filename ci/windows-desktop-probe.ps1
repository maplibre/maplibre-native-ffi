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
# A diagnostic command's exit code must not prevent the suites from running.
exit 0
