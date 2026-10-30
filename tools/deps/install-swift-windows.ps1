$ErrorActionPreference = 'Stop'
$prefix = $env:MISE_TOOL_INSTALL_PATH
$version = $env:MISE_TOOL_VERSION
$toolchainBin = Join-Path $prefix "Toolchains/$version+Asserts/usr/bin"
$installer = Join-Path $toolchainBin 'swift-installer.exe'
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

# Swift 6.4.1's SwiftPM manifest references two private assemblies that the CLI
# MSI omits. Use the installed runtime DLLs with the assembly version SwiftPM
# requests; swiftRegexBuilder's embedded manifest uses a different build number.
$runtimeBin = Join-Path $prefix "Runtimes/$version/usr/bin"
$architecture = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLowerInvariant()
if ($architecture -eq 'x64') { $architecture = 'amd64' }
foreach ($assembly in @('FoundationXML', 'swiftRegexBuilder')) {
    $directory = Join-Path $toolchainBin $assembly
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    Copy-Item (Join-Path $runtimeBin "$assembly.dll") $directory -Force
    @"
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity type="win32" name="$assembly" version="$version.0" processorArchitecture="$architecture" />
  <file name="$assembly.dll" />
</assembly>
"@ | Set-Content -Path (Join-Path $directory "$assembly.manifest") -Encoding UTF8
}

$env:Path = "$runtimeBin;$env:Path"
& (Join-Path $toolchainBin 'swift-package.exe') --version
if ($LASTEXITCODE -ne 0) { throw "SwiftPM startup failed: $LASTEXITCODE" }
