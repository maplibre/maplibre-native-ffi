$ErrorActionPreference = 'Stop'
$prefix = $env:MISE_TOOL_INSTALL_PATH
$installer = Join-Path $prefix "Toolchains/$($env:MISE_TOOL_VERSION)+Asserts/usr/bin/swift-installer.exe"
$arguments = @(
    '/quiet', '/norestart',
    "InstallRoot=`"$prefix`"",
    'OptionsInstallAndroidPlatform=0',
    'OptionsInstallDBG=0',
    'OptionsInstallPy=0',
    'OptionsInstallIDE=0'
)
$process = Start-Process -FilePath $installer -ArgumentList $arguments -Wait -PassThru
if ($process.ExitCode -notin @(0, 3010)) {
    throw "Swift installer exited $($process.ExitCode)"
}
